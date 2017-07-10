/*
	File:		lbj.pfishie.c

	Contains:	Max/Jitter external object generating Poisson distributed random values.

	Written by:	Peter Castine

	Copyright:	© 2003-2005 Peter Castine

	Change History (most recent first):
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "LitterLib.h"
#include "TrialPeriodUtils.h"
#include "RNGPoisson.h"

#ifdef WIN_VERSION
	#include "MoreMath.h"		// Need this for lgamma
#endif


#pragma mark • Constants

const char	kMaxClassName[]		= "lbj.pfishie",		// Class name for Max
			kJitClassName[]		= "lbj-pfishie";		// Class name for Jitter

const int	kMaxNN			= 31;

	// Indices for STR# resource
enum {
	strIndexInBang		= lpStrIndexLastStandard + 1,
	
	strIndexOutPoisson,
	strIndexOutDump,
	
	strIndexInLeft		= strIndexInBang,
	strIndexOutLeft		= strIndexOutPoisson
	};


#pragma mark • Type Definitions

	// We have this both here and in pfishie.c
	// Should move to RNGPoisson.h
typedef union {
	double			lambda;				// Just lambda
	tPoisInvParams	invParams;			// lambda & thresh
	tPoisRejParams	rejParams;			// lambda & lots more...
	// All typedef'd parameter structures *always* have lambda as first element!
	} uPoisParams;



#pragma mark • Object Structure

typedef struct {
	Object		coreObject;
	voidPtr		obex;						// The magic extended object thing.
	} msobPfishie;							// Mac Shell Object

typedef struct {
	Object		coreObject;
	
	long		lambdaCount;
	ePoisAlg	alg[JIT_MATRIX_MAX_PLANECOUNT];
	Boolean		flipped[JIT_MATRIX_MAX_PLANECOUNT];
	uPoisParams	params[JIT_MATRIX_MAX_PLANECOUNT];
	
	} jcobPfishie;							// Jitter Core Object


#pragma mark • Global Variables

void*			gPfishieJitClass	= NIL;
Messlist*		gPfishieMaxClass	= NIL;


#pragma mark • Function Prototypes

	// Max methods/functions
static void*PfishieNewMaxShell	(Symbol*, long, Atom*);
static void	PfishieFreeMaxShell	(msobPfishie*);

static void PfishieOutputMatrix	(msobPfishie*);
static void PfishieTattle		(msobPfishie*);
static void	PfishieAssist		(msobPfishie*, void* , long , long , char*);
static void	PfishieInfo			(msobPfishie*);

	// Jitter methods/functions
static t_jit_err PfishieJitInit		(void);


#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark • Inline Functions

/******************************************************************************************
 *
 *	GenPoissonDirTaus88Core()
 *	GenPoissonInvTaus88Core()
 *	GenPoissonRejTaus88Core()

 *	"xxxCore" functions should be moved to RNGPoisson.h
 *	Same for conditional #include for MoreMath.h
 *	
 ******************************************************************************************/

static inline long
GenPoissonDirTaus88Core(
	double		iThresh,
	UInt32*		s1,
	UInt32*		s2,
	UInt32*		s3)
	
	{
	long	p	= -1;
	double	x	= 1.0;
	
	do {
		p += 1;
		x *= ULong2Unit_Zo( Taus88Process(s1, s2, s3) );
		} while (x > iThresh);
	
	return p;
	}

static inline long
GenPoissonInvTaus88Core(
	const tPoisInvParams*	iParams,
	UInt32*					s1,
	UInt32*					s2,
	UInt32*					s3)
	
	{
	double	l = iParams->lambda,
			t = iParams->thresh,
			u = ULong2Unit_zo( Taus88Process(s1, s2, s3) ),
			p = 0.0;
	
	while (u > t) {
		u -= t;
		p += 1.0;
		t *= l / p;
		}
	
	return (long) p;
	}

static inline long
GenPoissonRejTaus88Core(
	const tPoisRejParams*	iParams,
	UInt32*					s1,
	UInt32*					s2,
	UInt32*					s3)
	
	{
	double	la = iParams->lambda,
			sq = iParams->sqrt2Lambda,
			ll = iParams->logLambda,
			lm = iParams->lambdaMagic,
			p,
			thresh;
	
	do  {
		double y;
		
	    do  {
			y = tan(kPi * ULong2Unit_ZO(Taus88Process(s1, s2, s3)));
			p = sq * y + la;
		    } while (p < 0.0);
		    
	    p = floor(p);
	    
	    thresh  = 0.9;
	    thresh *= y*y + 1.0;
	    thresh *= exp(p * ll - lgamma(p + 1.0) - lm);
	    
		} while (ULong2Unit_ZO( Taus88Process(s1, s2, s3) ) > thresh);
	
	return (long) p;
	}



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
	
	PfishieJitInit();
	
	// Standard Max setup() call
	setup(	&gPfishieMaxClass,					// Pointer to our class definition
			(method) PfishieNewMaxShell,		// Instance creation function
			(method) PfishieFreeMaxShell,		// Custom deallocation function
			(short) sizeof(msobPfishie),	// Class object size
			NIL,							// No menu function
			A_GIMME,						// Jitter objects always parse their own arguments
			0);		
	
	// Jitter Magic... 
	p = max_jit_classex_setup(calcoffset(msobPfishie, obex));
	q = jit_class_findbyname(gensym((char*) kJitClassName));    
    max_jit_classex_mop_wrap(p, q, kAttr); 		
    max_jit_classex_standard_wrap(p, q, 0); 	
	LITTER_TIMEBOMB max_addmethod_usurp_low((method) PfishieOutputMatrix, "outputmatrix");	
	
	// Back to adding messages...
	addmess	((method) PfishieTattle,	"dblclick",	A_CANT, 0);
	addmess	((method) PfishieTattle,	"tattle",	A_NOTHING);
	addmess	((method) PfishieAssist,	"assist",	A_CANT, 0);
	addmess	((method) PfishieInfo,		"info",		A_CANT, 0);
	
	// Initialize Litter Library
	LitterInit(kMaxClassName, 0);
	}


#pragma mark -
#pragma mark • Internal functions


#pragma mark -
#pragma mark • Max Shell Methods

/******************************************************************************************
 *
 *	PfishieNewMaxShell(iSym, iArgC, iArgV)
 *
 ******************************************************************************************/

static void*
PfishieNewMaxShell(
	SymbolPtr	sym,
	long		iArgC,
	Atom		iArgV[])
	
	{
	#pragma unused(sym)
	
	msobPfishie*	me			= NIL;
	void*			jitObj		= NIL;
	Symbol*			classSym	= gensym((char*) kJitClassName);
	
	me = (msobPfishie*) max_jit_obex_new(gPfishieMaxClass, classSym);
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
 *	PfishieFreeMaxShell(me)
 *
 ******************************************************************************************/

static void
PfishieFreeMaxShell(
	msobPfishie* me)
	
	{
	max_jit_mop_free(me);
	jit_object_free(max_jit_obex_jitob_get(me));
	max_jit_obex_free(me);
	}


/******************************************************************************************
 *
 *	PfishieOutputMatrix(me)
 *
 ******************************************************************************************/

static void
PfishieOutputMatrix(
	msobPfishie* me)
	
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
 *	PfishieTattle(me)
 *	PfishieInfo(me)
 *	PfishieAssist(me, iBox, iDir, iArgNum, oCStr)
 *
 *	Litter responses to standard Max messages
 *
 ******************************************************************************************/

void
PfishieTattle(
	msobPfishie* me)
	
	{
	#pragma unused(me)
	
	post("%s state", kMaxClassName);
	// Need to access these as attributes ???
	//post("  mean: %ld", me->mu);
	//post("  sigma: %ld", me->sigma);
	
	}

void PfishieInfo(msobPfishie* me)
	{ LitterInfo(kMaxClassName, &me->coreObject, (method) PfishieTattle); }

void PfishieAssist(msobPfishie* me, void* box, long iDir, long iArgNum, char* oCStr)
	{
	#pragma unused(me, box)
	
	LitterAssist(iDir, iArgNum, strIndexInLeft, strIndexOutLeft, oCStr);
	}


#pragma mark -
#pragma mark • Jitter Methods

/******************************************************************************************
 *
 *	PfishieJitMatrixCalc(me, inputs, outputs)
 *
 ******************************************************************************************/

	static void ClearPoisParams(uPoisParams* oParams)
		{
		oParams->rejParams.lambda		= 0.0;
		oParams->rejParams.sqrt2Lambda	= 0.0;
		oParams->rejParams.logLambda	= 0.0;
		oParams->rejParams.lambdaMagic	= 0.0;
		}

static jcobPfishie* PfishieJitNew()
	{
	int			i;
	jcobPfishie*	me = (jcobPfishie*) jit_object_alloc(gPfishieJitClass);
	
	me->lambdaCount	= 1;
	
	for (i = 0; i < JIT_MATRIX_MAX_PLANECOUNT; i += 1) {
		ClearPoisParams(me->params + i);
		
		me->alg[i]				= algDirect;
		me->flipped[i]			= false;
		me->params[i].lambda	= 1.0;
		}
	
	return me;
	}

static void PfishieJitFree(jcobPfishie* me)
	{
	#pragma unused(me)
	/* nothing to do */
	}


/******************************************************************************************
 *
 *	PfishieCharVector(iAlg, iFlipped, iParams, iCount, oMatrix)
 *
 ******************************************************************************************/

static void
PfishieCharVector(
	ePoisAlg		iAlg,
	Boolean			iFlipped,
	uPoisParams*	iParams,
	long			iCount,
	t_jit_op_info*	oMatrix)
	
	{
	Byte*			data	= (Byte*) oMatrix->p;
	long			stride  = oMatrix->stride;
	
	if (iAlg != algConstZero) {
		tTaus88Data*	tausData = Taus88GetGlobals();
		UInt32			s1, s2, s3, p;
		
		Taus88Load(tausData, &s1, &s2, &s3);
		
		switch (iAlg) {
		case algReject:
			if  (iFlipped) while (iCount-- > 0) {
				p = GenPoissonRejTaus88Core((const tPoisRejParams*) iParams, &s1, &s2, &s3);
				*data = 255 - p;
				data += stride;
				}
			else while (iCount-- > 0) {
				p = GenPoissonRejTaus88Core((const tPoisRejParams*) iParams, &s1, &s2, &s3);
				*data = p;
				data += stride;
				}
			break;
		
		case algInversion:
			if  (iFlipped) while (iCount-- > 0) {
				p = GenPoissonInvTaus88Core((const tPoisInvParams*) iParams, &s1, &s2, &s3);
				*data = 255 - p; 
				data += stride;
				}
			else while (iCount-- > 0) {
				p = GenPoissonInvTaus88Core((const tPoisInvParams*) iParams, &s1, &s2, &s3);
				*data = p;
				data += stride;
				}
			break;
		
		default:
			// Must be algDirect
			if  (iFlipped) while (iCount-- > 0) {
				p = GenPoissonDirTaus88Core(iParams->invParams.thresh, &s1, &s2, &s3);
				*data = 255 - p;
				data += stride;
				}
			else while (iCount-- > 0) {
				p = GenPoissonDirTaus88Core(iParams->invParams.thresh, &s1, &s2, &s3);
				*data = p;
				data += stride;
				}
			break;
			}
		
		Taus88Store(tausData, s1, s2, s3);
		}
	
	else {
		// Must be algConstZero
		if  (iFlipped) while (iCount-- > 0) {
			*data = 255;
			data += stride;
			}
		else while (iCount-- > 0) {
			*data = 0;
			data += stride;
			}
		}
		
	}
	

/******************************************************************************************
 *
 *	RecurseDimensions(iDimCount, iDimVec, iPlaneCount, iMInfo, iBOP)
 *
 ******************************************************************************************/

static void
RecurseDimensions(
	jcobPfishie*		me,
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
		
		// Test if it's appropriate to flatten to a single plane
		if (iPlaneCount > 1) {
			Boolean flatten = true;
			double	lambda	= me->params[0].lambda;
			
			for (i = iPlaneCount - 1; i > 0; i -= 1) {
				if (me->params[i].lambda != lambda) {
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
		
		for (i = 0; i < iDimVec[1]; i += 1) for (j=0; j < iPlaneCount; j += 1) {
			outOpInfo.p = iBOP + i * iMInfo->dimstride[1] + j;
			PfishieCharVector(me->alg[j], me->flipped[j], me->params + j, n, &outOpInfo);
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
 *	PfishieJitMatrixCalc(me, inputs, outputs)
 *
 ******************************************************************************************/

static t_jit_err
PfishieJitMatrixCalc(
	jcobPfishie*	me,
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
		RecurseDimensions(me, outMInfo.dimcount, dim, outMInfo.planecount, &outMInfo, outMData);
		}
	else err = JIT_ERR_INVALID_OUTPUT;
	
	jit_object_method(outMatrix, _jit_sym_lock, outSaveLock);
	
	return err;
	}
	

#pragma mark -
#pragma mark • Attribute Access methods

/******************************************************************************************
 *
 *	PfishieSetLambda(me, attr, iArgC, iArgVec)
 *	PfishieGetLambda(me, attr, iArgC, iArgVec)
 *
 ******************************************************************************************/

	static ePoisAlg MyRecommendation(double iLambda)
		{
		// Experiments in Jitter seem to indicate a slightly different set of threshholds
		// for selecting the fastest algorithm.
		// This is an area for further investigation.
		
		if		(iLambda > 5.0)		return algInversion;
		else if (iLambda > 0.0)		return algDirect;
		else if (iLambda == 0.0)	return algConstZero;
		else						return algUndef;		// This should never happen!
		
		}
	
static void
PfishieSetLambda(
	jcobPfishie*	me,
	void*			attr,
	long			iArgC,
	Atom			iArgVec[])
	
	{
	#pragma unused(attr)
	
	int i;
	
	// Sanity checks
	if (iArgC <= 0 || iArgVec == NIL)
		return;
	if (iArgC > JIT_MATRIX_MAX_PLANECOUNT)
		iArgC = JIT_MATRIX_MAX_PLANECOUNT;
	
	for (i = 0; i < iArgC; i += 1) {
		double	lambda	= AtomGetFloat(&iArgVec[i]);
		Boolean	flipped	= false;
		
		if (lambda < 0.0)
			lambda = 0.0;
		if (lambda > 255.0)
			lambda = 255.0;
			
		if (lambda > 127.5) {
			lambda = 255.0 - lambda;
			flipped = true;
			}
		me->flipped[i] = flipped;
		
		
		if (lambda == me->params[i].lambda)
			continue;					// Skip the following, could be a lot of work
		
		me->params[i].lambda = lambda;
		switch (me->alg[i] = MyRecommendation(lambda)) {
		case algReject:
			CalcPoisRejParams(&me->params[i].rejParams, lambda);
			break;
		case algInversion:
			CalcPoisInvParams(&me->params[i].invParams, lambda);
			break;
		case algDirect:
			me->params[i].invParams.lambda = lambda;
			me->params[i].invParams.thresh = CalcPoisDirThresh(lambda);
			break;
		default:
			// Must be algConstZero; algUndef can't hapen
			me->params[i].lambda	= 0.0;
			break;
			}
		
		}
	
	me->lambdaCount = iArgC;
	
	// "Splat" all parameters settings across the remaining slots in case planeCount
	// is larger than lambdaCount at matrix_calc time
	for ( ; i < JIT_MATRIX_MAX_PLANECOUNT; i += i) {
		int j = i - iArgC;		// ASSERT: j >= 0
		
		me->flipped[i]	= me->flipped[j];
		me->params[i]	= me->params[j];
		}
	
	}

static t_jit_err
PfishieGetLambda(
	jcobPfishie*	me,
	void*			attr, 
	long*			ioArgC, 
	Atom**			ioArgVec)
	
	{
	#pragma unused(attr)
	
	t_jit_err	myErr = JIT_ERR_NONE;
	int			i;

	if (*ioArgC <= 0 || *ioArgVec == NIL) {
		*ioArgVec = (Atom*) getbytes(me->lambdaCount * sizeof(Atom));
		if (*ioArgVec == NIL) {
			myErr = JIT_ERR_OUT_OF_MEM;
			goto punt;
			}
		}
	
	*ioArgC = me->lambdaCount;			// Is this kosher?? jit.p.bounds.c does this
										// inside the if block, but that seems wrong
										// to me
											
	for (i = 0; i < me->lambdaCount; i += 1)
		AtomSetFloat(*ioArgVec + i, me->flipped[i]
										? 255.0 - me->params[i].lambda
										: me->params[i].lambda);
			
punt:	
	return myErr;
	}

	
#pragma mark -
#pragma mark • Jitter Initialization

/******************************************************************************************
 *
 *	PfishieJitInit(me)
 *
 ******************************************************************************************/

	// !! convenience type, should move to MaxUtils.h or such
	typedef t_jit_object* tJitObjPtr;
	
t_jit_err
PfishieJitInit(void) 

	{
	const long	kAttrRWFlags	= JIT_ATTR_GET_DEFER_LOW | JIT_ATTR_SET_USURP_LOW,
				kAttrROnlyFlags	= JIT_ATTR_GET_DEFER_LOW | JIT_ATTR_SET_OPAQUE_USER;
	const int	kJitterInlets	= 0,	// No inlets for Jitter Matrices
				kJitterOutlets	= 1;	// One outlet for Jitter Matrices (Poisson noise)
										// The Max shell object will create add'l outlets 
										// but that's none of our business.
	
	tJitObjPtr	mop,
				attr;
	
	gPfishieJitClass = jit_class_new((char*) kJitClassName,
									 (method) PfishieJitNew,
									 (method) PfishieJitFree,
									 sizeof(jcobPfishie),
									 A_CANT, 0L
									 ); 
	
	
	// Add matrix operator
	mop = jit_object_new(_jit_sym_jit_mop, kJitterInlets, kJitterOutlets);
	jit_mop_single_type(mop, _jit_sym_char);	
	jit_class_addadornment(gPfishieJitClass, mop);
	
	// Add methods
	jit_class_addmethod(gPfishieJitClass,
						(method) PfishieJitMatrixCalc,
						"matrix_calc",
						A_CANT, 0L);
	
	// Add attributes
	
		// lambda
	attr = jit_object_new(	_jit_sym_jit_attr_offset_array,
							"lambda",
							_jit_sym_float64,
							JIT_MATRIX_MAX_PLANECOUNT,
							kAttrRWFlags,
							(method) PfishieGetLambda, (method) PfishieSetLambda,
							calcoffset(jcobPfishie, lambdaCount), (long) 0
							);
	jit_class_addattr(gPfishieJitClass, attr);
	
	// Register class and go
	jit_class_register(gPfishieJitClass);
	return JIT_ERR_NONE;
	}
