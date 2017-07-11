/*
	File:		lbj.expo.c

	Contains:	Max/Jitter external object generating normally distributed random values.

	Written by:	Peter Castine

	Copyright:	© 2006 Peter Castine

	Change History (most recent first):

*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "LitterLib.h"
#include "TrialPeriodUtils.h"
#include "MiscUtils.h"
#include "Taus88.h"


#pragma mark • Constants

const char	kMaxClassName[]		= "lbj.expo",			// Class name for Max
			kJitClassName[]		= "lbj-expo";			// Class name for Jitter

const int	kMaxNN			= 31;

	// Indices for STR# resource
enum {
	strIndexInBang		= lpStrIndexLastStandard + 1,
	
	strIndexOutExpo,
	strIndexOutDump,
	
	strIndexInLeft		= strIndexInBang,
	strIndexOutLeft		= strIndexOutExpo
	};


#pragma mark • Type Definitions



#pragma mark • Object Structure

typedef struct {
	Object		coreObject;
	voidPtr		obex;					// The magic extended object thing.
	} msobExpo;							// Mac Shell Object

typedef struct {
	Object		coreObject;
	
	long		ltCount,				// Same counts for lambda and tau
				locCount,
				symCount;
	double		lambda[JIT_MATRIX_MAX_PLANECOUNT],
				tau[JIT_MATRIX_MAX_PLANECOUNT],
				loc[JIT_MATRIX_MAX_PLANECOUNT];
	eSymmetry	sym[JIT_MATRIX_MAX_PLANECOUNT];
	
	} jcobExpo;										// Jitter Core Object


#pragma mark • Global Variables

void*			gExpoJitClass	= NIL;
Messlist*		gExpoMaxClass	= NIL;


#pragma mark • Function Prototypes

	// Max methods/functions
static void*ExpoNewMaxShell	(Symbol*, long, Atom*);
static void	ExpoFreeMaxShell(msobExpo*);

static void ExpoOutputMatrix(msobExpo*);
static void ExpoTattle	(msobExpo*);
static void	ExpoAssist	(msobExpo*, void* , long , long , char*);
static void	ExpoInfo	(msobExpo*);

	// Jitter methods/functions
static t_jit_err ExpoJitInit		(void);


#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark • Inline Functions




#pragma mark -

/******************************************************************************************
 *
 *	main()
 *	
 *	Standard Max External Object Entry Point Function
 *	
 ******************************************************************************************/

void
main(void)
	
	{
	const long kAttr = MAX_JIT_MOP_FLAGS_OWN_OUTPUTMATRIX | MAX_JIT_MOP_FLAGS_OWN_JIT_MATRIX;
	
	voidPtr	p,									// Have to guess about what these two do
			q;									// Not much is documented in the Jitter SDK
	
	LITTER_CHECKTIMEOUT(kMaxClassName);
	ExpoJitInit();
	
	// Standard Max setup() call
	setup(	&gExpoMaxClass,					// Pointer to our class definition
			(method) ExpoNewMaxShell,		// Instance creation function
			(method) ExpoFreeMaxShell,		// Custom deallocation function
			(short) sizeof(msobExpo),		// Class object size
			NIL,							// No menu function
			A_GIMME,						// Jitter objects always parse their own arguments
			0);		
	
	// Jitter Magic... 
	p = max_jit_classex_setup(calcoffset(msobExpo, obex));
	q = jit_class_findbyname(gensym((char*) kJitClassName));    
    max_jit_classex_mop_wrap(p, q, kAttr); 		
    max_jit_classex_standard_wrap(p, q, 0); 	
	LITTER_TIMEBOMB max_addmethod_usurp_low((method) ExpoOutputMatrix, "outputmatrix");	
	
	// Back to adding messages...
	addmess	((method) ExpoTattle,	"dblclick",	A_CANT, 0);
	addmess	((method) ExpoTattle,	"tattle",	A_NOTHING);
	addmess	((method) ExpoAssist,	"assist",	A_CANT, 0);
	addmess	((method) ExpoInfo,		"info",		A_CANT, 0);
	
	// Initialize Litter Library
	LitterInit(kMaxClassName, 0);
	}


#pragma mark -
#pragma mark • Internal functions



#pragma mark -
#pragma mark • Max Shell Methods

/******************************************************************************************
 *
 *	ExpoNewMaxShell(iSym, iArgC, iArgV)
 *
 ******************************************************************************************/

static void*
ExpoNewMaxShell(
	SymbolPtr	sym,
	long		iArgC,
	Atom		iArgV[])
	
	{
	#pragma unused(sym)
	
	msobExpo*	me			= NIL;
	void*		jitObj		= NIL;
	Symbol*		classSym	= gensym((char*) kJitClassName);
	
	me = (msobExpo*) max_jit_obex_new(gExpoMaxClass, classSym);
	if (me == NIL) goto punt;
		
	jitObj = jit_object_new(classSym);
		if (jitObj == NIL) goto punt;
	
	max_jit_mop_setup_simple(me, jitObj, iArgC, iArgV);			
	max_jit_attr_args(me, iArgC, iArgV);

	return me;
	// ------------------------------------------------------------------------
	// End of normal processing
	
	// Poor man's exception handling
punt:
	error("%s: could not allocate object", kMaxClassName);
	if (me != NIL)
		freeobject(&me->coreObject);
		
	return NIL;
	}

/******************************************************************************************
 *
 *	ExpoFreeMaxShell(me)
 *
 ******************************************************************************************/

static void
ExpoFreeMaxShell(
	msobExpo* me)
	
	{
	max_jit_mop_free(me);
	jit_object_free(max_jit_obex_jitob_get(me));
	max_jit_obex_free(me);
	}


/******************************************************************************************
 *
 *	ExpoOutputMatrix(me)
 *
 ******************************************************************************************/

static void
ExpoOutputMatrix(
	msobExpo* me)
	
	{
	void*		mop = max_jit_obex_adornment_get(me, _jit_sym_jit_mop);
	t_jit_err	err = noErr;	
	
	// Sanity check: don't output if mop is NIL
	if (mop == NIL)
		return;
	
	if (max_jit_mop_getoutputmode(me) == 1) {
		err = (t_jit_err) jit_object_method(
								max_jit_obex_jitob_get(me), 
								_jit_sym_matrix_calc,
								jit_object_method(mop, _jit_sym_getinputlist),
								jit_object_method(mop, _jit_sym_getoutputlist));
		
		if (err == noErr)						
				max_jit_mop_outputmatrix(me);
		else	jit_error_code(me, err); 
		}
		
	}


/******************************************************************************************
 *
 *	ExpoTattle(me)
 *	ExpoInfo(me)
 *	ExpoAssist(me, iBox, iDir, iArgNum, oCStr)
 *
 *	Litter responses to standard Max messages
 *
 ******************************************************************************************/

void
ExpoTattle(
	msobExpo* me)
	
	{
	#pragma unused(me)
	
	post("%s state", kMaxClassName);
	// Need to access these as attributes ???
	//post("  mean: %ld", me->mu);
	//post("  sigma: %ld", me->sigma);
	
	}

void ExpoInfo(msobExpo* me)
	{ LitterInfo(kMaxClassName, &me->coreObject, (method) ExpoTattle); }

void ExpoAssist(msobExpo* me, void* box, long iDir, long iArgNum, char* oCStr)
	{
	#pragma unused(me, box)
	
	LitterAssist(iDir, iArgNum, strIndexInLeft, strIndexOutLeft, oCStr);
	}


#pragma mark -
#pragma mark • Jitter Methods

/******************************************************************************************
 *
 *	ExpoJitMatrixCalc(me, inputs, outputs)
 *
 ******************************************************************************************/

static jcobExpo* ExpoJitNew()
	{
	int			i;
	jcobExpo*	me = (jcobExpo*) jit_object_alloc(gExpoJitClass);
	
	me->ltCount		= 1;
	me->locCount	= 1;
	me->symCount	= 1;
	
	for (i = 0; i < JIT_MATRIX_MAX_PLANECOUNT; i += 1) {
		me->lambda[i]	= 1.0;
		me->tau[i]		= 1.0;
		me->loc[i]		= 0.0;
		me->sym[i]		= symPos;
		}
	
	return me;
	}

static void ExpoJitFree(jcobExpo* me)
	{
	#pragma unused(me)
	/* nothing to do */
	}


/******************************************************************************************
 *
 *	RecurseDimensions(iDimCount, iDimVec, iPlaneCount, iMInfo, iBOP)
 *
 ******************************************************************************************/

	// We have this in a couple of LBJ .c files.
	// Should move it to MiscUtil.h
	static inline Byte Unit2Byte(double x)
		{ return (Byte) (255.0 * x + 0.5); }

static void
ExpoCharVector(
	double			iTau,
	double			iLoc,
	eSymmetry		iSym,
	long			iCount,
	t_jit_op_info*	oMatrix)
	
	{
	const double kInf = 1.0 / 0.0;
	
	Byte*	data	= (Byte*) oMatrix->p;
	long	stride  = oMatrix->stride;
	UInt32	s1, s2, s3;
	
	Taus88LoadGlobal(&s1, &s2, &s3);
	
	// Sanity check
	if		(iLoc < 0.0)	iLoc = 0.0;
	else if	(iLoc > 255.0)	iLoc = 255.0;
	else					iLoc += 0.5;				// for rounding
	
	if (iTau > 0.0 && iTau < kInf) {
		iTau *= 256.0;
		
		switch (iSym) {
			UInt32 x;
		
		default:							// Must be symPos
			while (iCount-- > 0) {
				x = Taus88Process(&s1, &s2, &s3);
				x = iLoc + iTau * Unit2Exponential( ULong2Unit_zO(x) );
				*data = CLAMP(x, 0, 255);
				data += stride;
				}
			break;
		
		case symNeg:
			while (iCount-- > 0) {
				x = Taus88Process(&s1, &s2, &s3);
				x = iLoc - iTau * Unit2Exponential( ULong2Unit_zO(x) );
				*data = CLAMP(x, 0, 255);
				data += stride;
				}
			break;
		
		case symSym:
			while (iCount-- > 0) {
				x = Taus88Process(&s1, &s2, &s3);
				x = iLoc + iTau * Unit2Laplace( ULong2Unit_zO(x) );
				*data = CLAMP(x, 0, 255);
				data += stride;
				}
			break;
			}
		}
	
	else if (iTau == 0.0) {
		// Fill with zeroes
		if (stride == 1) {
			// Funky optimization
			long*	longData = (long*) data;
			long	longCount = iCount >> 2;
			
			while (longCount-- >  0)
				*longData++ = 0;
			
			iCount &= 0x00000003;
			data = (Byte*) longData;
			}
		
		// Any bytes not yet accounted for?
		while (iCount-- > 0) {
			*data = 0;
			data += stride;
			}
		}
	
	else {
		// Generate a uniform distribution
		long longCount = iCount >> 2;
		
		iCount &= 0x00000003;
		
		if (stride == 1) {
			// Funky optimization #1
			long*	longData = (long*) data;
			
			while (longCount-- >  0)
				*longData++ = Taus88Process(&s1, &s2, &s3);
			
			data = (Byte*) longData;
			}
		else {
			// Funky optimization #2
			while (longCount-- > 0) {
				UInt32	fourBytes = Taus88Process(&s1, &s2, &s3);
				int		i = 4;
				
				do	{
					*data = fourBytes;
					data += stride;
					fourBytes >>= 8;
					} while (--i > 0);
				}
			}
		
		// Any bytes not yet accounted for?
		if (iCount > 0) {
			UInt32 lastBytes = Taus88Process(&s1, &s2, &s3);
			
			do	{
				*data = lastBytes;
				data += stride;
				lastBytes >>= 8;
				} while (--iCount > 0);
			}
		}
		
	
	Taus88StoreGlobal(s1, s2, s3);
	}
	
static void
ExpoLongVector(
	double			iTau,
	double			iLoc,
	eSymmetry		iSym,
	long			iCount,
	t_jit_op_info*	oMatrix)
	
	{
	long*	data	= (long*) oMatrix->p;
	long	stride  = oMatrix->stride;
	UInt32	s1, s2, s3;
	
	Taus88LoadGlobal(&s1, &s2, &s3);
	
	// Sanity check
	// Prepare for rounding while we're at it
	if (iLoc < 0.0) {
		if (iLoc < (double) kLongMin)
			 iLoc = (double) kLongMin - 0.5;
		else iLoc -= 0.5;
		}
	else {
		if (iLoc > (double) kLongMax)
			 iLoc = (double) kLongMax + 0.5;
		else iLoc += 0.5;
		}
	
	if (iTau > 0.0) switch (iSym) {
		UInt32 x;
		
	default:								// Must be symPos
		while (iCount-- > 0) {
			x = Taus88Process(&s1, &s2, &s3);
			*data = iLoc + iTau * Unit2Exponential( ULong2Unit_zO(x) );
			data += stride;
			}
		break;
	
	case symNeg:
		while (iCount-- > 0) {
			x = Taus88Process(&s1, &s2, &s3);
			*data = iLoc - iTau * Unit2Exponential( ULong2Unit_zO(x) );
			data += stride;
			}
		break;
	
	case symSym:
		while (iCount-- > 0) {
			// Generate a random 32-bit integer
			x = Taus88Process(&s1, &s2, &s3);
			*data = iLoc + iTau * Unit2Laplace( ULong2Unit_zO(x) );
			data += stride;
			}
		break;
		}
		
	else {
		// Generate a uniform distribution
		if (stride == 1) while (iCount-- >  0)
			*data++ = Taus88Process(&s1, &s2, &s3);
		
		else while (iCount-- > 0) {
			*data = Taus88Process(&s1, &s2, &s3);
			data += stride;
			}
		}
	
	Taus88StoreGlobal(s1, s2, s3);
	}

static void 
ExpoFloatVector(
	double			iTau,
	double			iLoc,
	eSymmetry		iSym,
	long			iCount,
	t_jit_op_info*	oMatrix)
	
	{
	float*	data	= (float*) oMatrix->p;
	long	stride  = oMatrix->stride;
	UInt32	s1, s2, s3;
	
	Taus88LoadGlobal(&s1, &s2, &s3);
	
	CLAMP(iLoc, 0.0, 1.0);					// No rounding worries, just CLAMP
	
	if (iTau > 0.0) switch (iSym) {
		UInt32 x;
		
	default:							// Must be symPos
		while (iCount-- > 0) {
			x = Taus88Process(&s1, &s2, &s3);
			*data = iLoc + iTau * Unit2Exponential( ULong2Unit_zO(x) );
			data += stride;
			}
		break;
	
	case symNeg:
		while (iCount-- > 0) {
			x = Taus88Process(&s1, &s2, &s3);
			*data = iLoc - iTau * Unit2Exponential( ULong2Unit_zO(x) );
			data += stride;
			}
		break;
	
	case symSym:
		while (iCount-- > 0) {
			x = Taus88Process(&s1, &s2, &s3);
			*data = iLoc + iTau * Unit2Laplace( ULong2Unit_zO(x) );
			data += stride;
			}
		break;
		}
		
	else {
		if (stride == 1) while (iCount-- > 0)
			*data++ = ULong2Unit_ZO( Taus88Process(&s1, &s2, &s3) );
		else while (iCount-- > 0) {
			*data = ULong2Unit_ZO( Taus88Process(&s1, &s2, &s3) );
			data += stride;
			}
		}
	
	Taus88StoreGlobal(s1, s2, s3);
	}

static void 
ExpoDoubleVector(
	double			iTau,
	double			iLoc,
	eSymmetry		iSym,
	long			iCount,
	t_jit_op_info*	oMatrix)
	
	{
	double*	data	= (double*) oMatrix->p;
	long	stride  = oMatrix->stride;
	UInt32	s1, s2, s3;
	
	Taus88LoadGlobal(&s1, &s2, &s3);
	
	if (iTau > 0.0) {
		UInt32 x;
		
		switch (iSym) {
		default:							// Must be symPos
			while (iCount-- > 0) {
				x = Taus88Process(&s1, &s2, &s3);
				*data = iLoc + iTau * Unit2Exponential( ULong2Unit_zO(x) );
				data += stride;
				}
			break;
		
		case symNeg:
			while (iCount-- > 0) {
				x = Taus88Process(&s1, &s2, &s3);
				*data = iLoc - iTau * Unit2Exponential( ULong2Unit_zO(x) );
				data += stride;
				}
			break;
		
		case symSym:
			while (iCount-- > 0) {
				x = Taus88Process(&s1, &s2, &s3);
				*data = iLoc + iTau * Unit2Laplace( ULong2Unit_zO(x) );
				data += stride;
				}
			break;
			}
		
		}
	
	else {
		if (stride == 1) while (iCount-- > 0)
			*data++ = ULong2Unit_ZO( Taus88Process(&s1, &s2, &s3) );
		else while (iCount-- > 0) {
			*data = ULong2Unit_ZO( Taus88Process(&s1, &s2, &s3) );
			data += stride;
			}
		}
	
	Taus88StoreGlobal(s1, s2, s3);
	}

static void
RecurseDimensions(
	jcobExpo*			me,
	long				iDimCount,
	long				iDimVec[],
	long				iPlaneCount,
	t_jit_matrix_info*	iMInfo,
	char*				iBOP)
	
	{
	long 			i, j, n;
	t_jit_op_info	outOpInfo;
		
	if (iDimCount < 1)		// For safety: this also catches invalid (negative) values
		return;
	
	switch(iDimCount) {
	case 1:
		iDimVec[1] = 1;
		// fall into next case...
	case 2:
		n = iDimVec[0];
		outOpInfo.stride = iMInfo->dim[0] > 1
							? iMInfo->planecount
							: 0;
		
		// Test if it might be appropriate to flatten to a single plane
		if (iPlaneCount > 1) {
			Boolean		flatten = true;
			double		tau	= me->tau[0],
						loc	= me->loc[0];
			eSymmetry	sym = me->sym[0];
			
			for (i = iPlaneCount - 1; i > 0; i -= 1) {
				if (me->tau[i] != tau || me->loc[i] != loc || me->sym[i] != sym) {
					flatten = false;
					break;
					}
				}
			
			if (flatten) {
				n					*= iPlaneCount;
				iPlaneCount			 = 1;
				outOpInfo.stride	 = 1;
				}
			}
		
		if (iMInfo->type == _jit_sym_char) {
			for (i = 0; i < iDimVec[1]; i += 1) for (j=0; j < iPlaneCount; j += 1) {
				outOpInfo.p = iBOP + i * iMInfo->dimstride[1] + j;
				ExpoCharVector(me->tau[j], me->loc[j], me->sym[j], n, &outOpInfo);
				}
			}
		
		else if (iMInfo->type == _jit_sym_long) {
			for (i = 0; i < iDimVec[1]; i += 1) for (j=0; j < iPlaneCount; j += 1) {
				outOpInfo.p = iBOP + i * iMInfo->dimstride[1] + j * sizeof(long);
				ExpoLongVector(me->tau[j], me->loc[j], me->sym[j], n, &outOpInfo);
				}
			}
		
		else if (iMInfo->type == _jit_sym_float32) {
			for (i = 0; i < iDimVec[1]; i += 1) for (j=0; j < iPlaneCount; j += 1) {
				outOpInfo.p = iBOP + i * iMInfo->dimstride[1] + j * sizeof(float);
				ExpoFloatVector(me->tau[j], me->loc[j], me->sym[j], n, &outOpInfo);
				}
			}
		
		else if (iMInfo->type == _jit_sym_float64) {
			for (i = 0; i < iDimVec[1]; i += 1) for (j=0; j < iPlaneCount; j += 1) {
				outOpInfo.p = iBOP + i * iMInfo->dimstride[1] + j * sizeof(double);
				ExpoDoubleVector(me->tau[j], me->loc[j], me->sym[j], n, &outOpInfo);
				}
			
			}
		
		break;
	
	default:
		// Larger values
		for	(i = 0; i < iDimVec[iDimCount-1]; i += 1) {
			char* op  = iBOP  + i * iMInfo->dimstride[iDimCount-1];
			RecurseDimensions(me, iDimCount - 1, iDimVec, iPlaneCount, iMInfo, op);
			}
		}
	
	}


/******************************************************************************************
 *
 *	ExpoJitMatrixCalc(me, inputs, outputs)
 *
 ******************************************************************************************/

static t_jit_err
ExpoJitMatrixCalc(
	jcobExpo*	me,
	void*		inputs,
	void*		outputs)
	
	{
	#pragma unused(inputs)
	
	t_jit_err			err = JIT_ERR_NONE;
	long				outSaveLock;
	t_jit_matrix_info	outMInfo;
	char*				outMData;
	void*				outMatrix = jit_object_method(outputs, _jit_sym_getindex, 0);

	// Sanity check
	if ((me == NIL) || (outMatrix == NIL))
		return JIT_ERR_INVALID_PTR;
	
	outSaveLock = (long) jit_object_method(outMatrix, _jit_sym_lock, 1);
	
	jit_object_method(outMatrix, _jit_sym_getinfo, &outMInfo);
	jit_object_method(outMatrix, _jit_sym_getdata, &outMData);
	
	if (outMData != NIL) {
		// Copy dimensions to our private buffer
		long	i,
				dim[JIT_MATRIX_MAX_DIMCOUNT];
		
		for (i = 0; i < outMInfo.dimcount; i += 1)
			dim[i] = outMInfo.dim[i];
				
		// Call the workhorse function
		RecurseDimensions(	me,
							outMInfo.dimcount,
							dim,
							outMInfo.planecount,
							&outMInfo,
							outMData);
		}
	else err = JIT_ERR_INVALID_OUTPUT;
	
	jit_object_method(outMatrix, _jit_sym_lock, outSaveLock);
	
	return err;
	}
	

/******************************************************************************************
 *
 *	ExpoJitInit(me)
 *
 ******************************************************************************************/

	static void ExpoSetLambda(jcobExpo* me, void* attr, long iArgC, Atom iArgVec[])
		{
		#pragma unused(attr)
		
		int i;
		
		// Sanity checks
		if (iArgC <= 0 || iArgVec == NIL)
			return;
		if (iArgC > JIT_MATRIX_MAX_PLANECOUNT)
			iArgC = JIT_MATRIX_MAX_PLANECOUNT;
		
		for (i = 0; i < iArgC; i += 1) {
			double lambda = AtomGetFloat(&iArgVec[i]);
			
			// Last sanity check
			if (lambda < 0.0)
				lambda = 0.0;
			
			// Not worth the effort to check if this is a real change in attribute value,
			// just store the numbers and go on to the next Atom
			me->lambda[i]	= lambda;
			me->tau[i]		= 1.0 / lambda;
			}
		
		// Fill up unspecified values by "splatting" the specified values.
		// This way we have valid entries for any planecount at matrix_calc time.
		for ( ; i < JIT_MATRIX_MAX_PLANECOUNT; i += 1) {
			int j = i - iArgC;		// Assert: j > 0
			
			me->lambda[i]	= me->lambda[j];
			me->tau[i]		= me->tau[j];
			}
		
		}
	
	static void ExpoSetTau(jcobExpo* me, void* attr, long iArgC, Atom iArgVec[])
		{
		#pragma unused(attr)
		
		int i;
		
		// Sanity checks
		if (iArgC <= 0 || iArgVec == NIL)
			return;
		if (iArgC > JIT_MATRIX_MAX_PLANECOUNT)
			iArgC = JIT_MATRIX_MAX_PLANECOUNT;
		
		for (i = 0; i < iArgC; i += 1) {
			double	tau	= AtomGetFloat(&iArgVec[i]);
			
			// Last sanity check
			if (tau < 0.0)
				tau = 0.0;
			
			// Not worth the effort to check if this is a real change in attribute value,
			// just store the numbers and go on to the next Atom
			me->lambda[i]	= 1.0 / tau;
			me->tau[i]		= tau;
			}
		
		// Fill up unspecified values by "splatting" the specified values.
		// This way we have valid entries for any planecount at matrix_calc time.
		for ( ; i < JIT_MATRIX_MAX_PLANECOUNT; i += 1) {
			int j = i - iArgC;		// Assert: j > 0
			
			me->lambda[i]	= me->lambda[j];
			me->tau[i]		= me->tau[j];
			}
		
		}
	
	
	
	// !! convenience type, should move to MaxUtils.h or such
	typedef t_jit_object* tJitObjPtr;

t_jit_err
ExpoJitInit(void) 

	{
	const long	kAttrRWFlags	= JIT_ATTR_GET_DEFER_LOW | JIT_ATTR_SET_USURP_LOW,
				kAttrROnlyFlags	= JIT_ATTR_GET_DEFER_LOW | JIT_ATTR_SET_OPAQUE_USER;
	const int	kJitterInlets	= 0,	// No inlets for Jitter Matrices
				kJitterOutlets	= 1;	// One outlet for Jitter Matrices (Beta noise)
	
	tJitObjPtr	mop,
				attr;
	
	gExpoJitClass = jit_class_new(	(char*) kJitClassName,
									(method) ExpoJitNew,
									(method) ExpoJitFree,		// ?? Could be NIL ??
									sizeof(jcobExpo),
									A_CANT, 0L
									); 
	
	// Add matrix operator
	mop = jit_object_new(_jit_sym_jit_mop, kJitterInlets, kJitterOutlets);
	jit_class_addadornment(gExpoJitClass, mop);
	
	// Add methods
	jit_class_addmethod(gExpoJitClass,
						(method) ExpoJitMatrixCalc,
						"matrix_calc",
						A_CANT, 0L);
	
	// Add attributes
		// lambda
	attr = jit_object_new(	_jit_sym_jit_attr_offset_array,
							"lambda",
							_jit_sym_float64,
							JIT_MATRIX_MAX_PLANECOUNT,
							kAttrRWFlags,
							(method) NIL, (method) ExpoSetLambda,
							calcoffset(jcobExpo, ltCount),
							calcoffset(jcobExpo, lambda)
							);
	jit_class_addattr(gExpoJitClass, attr);
	attr = jit_object_new(	_jit_sym_jit_attr_offset_array,
							"tau",
							_jit_sym_float64,
							JIT_MATRIX_MAX_PLANECOUNT,
							kAttrRWFlags,
							(method) NIL, (method) ExpoSetTau,
							calcoffset(jcobExpo, ltCount),
							calcoffset(jcobExpo, tau)
							);
	jit_class_addattr(gExpoJitClass, attr);
	
		// location
	attr = jit_object_new(	_jit_sym_jit_attr_offset_array,
							"loc",
							_jit_sym_float64,
							JIT_MATRIX_MAX_PLANECOUNT,
							kAttrRWFlags,
							(method) NIL, (method) NIL,
							calcoffset(jcobExpo, locCount),
							calcoffset(jcobExpo, loc)
							);
	jit_class_addattr(gExpoJitClass, attr);
	
		// symmetry
	attr = jit_object_new(	_jit_sym_jit_attr_offset_array,
							"sym",
							_jit_sym_char,
							JIT_MATRIX_MAX_PLANECOUNT,
							kAttrRWFlags,
							(method) NIL, (method) NIL,
							calcoffset(jcobExpo, symCount),
							calcoffset(jcobExpo, sym)
							);
		// Only allow values -1 (symNeg), 0 (symSym), and 1 (symPos)
	jit_attr_addfilterset_clip(attr, -1, 1, true, true);	
	jit_class_addattr(gExpoJitClass, attr);
	
	
	// Register class and go
	jit_class_register(gExpoJitClass);
	return JIT_ERR_NONE;
	}
