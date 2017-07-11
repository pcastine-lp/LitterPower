/*
	File:		lbj.gsss.c

	Contains:	Max/Jitter external object generating normally distributed random values.

	Written by:	Peter Castine

	Copyright:	© 2003-2005 Peter Castine

	Change History (most recent first):

        <4+>   14–6–2006    pc      Figured out how to add custom set/get methods to array
                                    attributes (for skew & kurtosis).
         <4>      8–6–06    pc      Make mu/sigma configurable on a plane-by-plane basis. Attempt to
                                    add skew & kurtosis as read-only attributes.
         <3>    8–5–2006    pc      Update for new LitterLib organization. Inline Gauss RNG. Tidy up
                                    as we understand better how Jitter works.
         <2>   23–3–2006    pc      Update to use faster RNG algorithm.
         <1>     11–1–06    pc      first checked in.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "LitterLib.h"
#include "TrialPeriodUtils.h"
#include "RNGGauss.h"


#pragma mark • Constants

const char	kMaxClassName[]		= "lbj.norm",			// Class name for Max
			kJitClassName[]		= "lbj-norm";			// Class name for Jitter

	// Indices for STR# resource
enum {
	strIndexInBang		= lpStrIndexLastStandard + 1,
	
	strIndexOutWhite,
	strIndexOutDump,
	
	strIndexInLeft		= strIndexInBang,
	strIndexOutLeft		= strIndexOutWhite
	};


#pragma mark • Type Definitions



#pragma mark • Object Structure

typedef struct {
	Object		coreObject;
	voidPtr		obex;					// The magic extended object thing.
	} msobNorm;							// Mac Shell Object

typedef struct {
	Object		coreObject;
	
	char		clip;								// Use this as a Boolean
	long		planeCount;
	double		mean[JIT_MATRIX_MAX_PLANECOUNT],	// Mean, one per plane
				sigma[JIT_MATRIX_MAX_PLANECOUNT];	// Variance, one per plane
	
	
	} jcobNorm;							// Jitter Core Object


#pragma mark • Global Variables

void*			gNormJitClass	= NIL;
Messlist*		gNormMaxClass	= NIL;


#pragma mark • Function Prototypes

	// Max methods/functions
static void*NormNewMaxShell	(Symbol*, long, Atom*);
static void	NormFreeMaxShell(msobNorm*);

static void NormOutputMatrix(msobNorm*);
static void NormTattle	(msobNorm*);
static void	NormAssist	(msobNorm*, void* , long , long , char*);
static void	NormInfo	(msobNorm*);

	// Jitter methods/functions
static t_jit_err NormJitInit		(void);


#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark • Inline Functions

// The following const and static inlines should be moved to RNGGauss
const double	kKRMagic		= 2.2160358672;
const UInt32	kKRBodyThresh	= 3797053464,
				kKRReg1Thresh	= 3914058587,
				kKRReg2Thresh	= 4117674588,
				kKRReg3Thresh	= 4180338716;

static inline double NormKRHelper(double x)
	{ return 0.3989422804 * exp(-0.5 * x * x) - 0.1800251911 * (kKRMagic - x); }

static inline double
NormTaus88Body(
	UInt32* s1,
	UInt32* s2,
	UInt32* s3)
	
	{
	double result;
	
	result  = ULong2Unit_ZO( Taus88Process(s1, s2, s3) );
	result += ULong2Unit_ZO( Taus88Process(s1, s2, s3) );
	result -= 1.0;
	result *= kKRMagic;
	
	return result;
	}

static inline double
NormTaus88Region1(
	UInt32* s1,
	UInt32* s2,
	UInt32* s3)
	
	{
	
	do	{
		double	u1		= ULong2Unit_ZO( Taus88Process(s1, s2, s3) ),
				u2		= ULong2Unit_ZO( Taus88Process(s1, s2, s3) ),
				sign	= 1.0,
				result;
		
		if (u2 < u1) {
			// Swap, generate negative variate
			double temp = u1;
			u1 = u2;
			u2 = temp;
			sign = -1.0;
			}
	   
	    result = 0.4797274042 - 0.5955071380 * u1;
	    if (result < 0.0)
	    	continue;									// No good
	    if (u2 <= 0.8055779244
		    	|| 0.0533775495 * (u2-u1) <= NormKRHelper(result)) {
			return sign * result;						// We have a variate
			}
		} while (true);
	
	}

static inline double
NormTaus88Region2(
	UInt32* s1,
	UInt32* s2,
	UInt32* s3)
	
	{
	
	do	{
		double	u1		= ULong2Unit_ZO( Taus88Process(s1, s2, s3) ),
				u2		= ULong2Unit_ZO( Taus88Process(s1, s2, s3) ),
				sign	= 1.0,
				result;
		
		if (u2 < u1) {
			// Swap, generate negative variate
			double temp = u1;
			u1 = u2;
			u2 = temp;
			sign = -1.0;
			}
	   
	    result = 0.4797274042 + 1.1054736610 * u1;
	    if (u2 <= 0.8728349767
		    	|| 0.0492644964 * (u2-u1) <= NormKRHelper(result)) {
			return sign * result;						// We have a variate
			}
		} while (true);
	
	}

static inline double
NormTaus88Region3(
	UInt32* s1,
	UInt32* s2,
	UInt32* s3)
	
	{
	do	{
		double	u1		= ULong2Unit_ZO( Taus88Process(s1, s2, s3) ),
				u2		= ULong2Unit_ZO( Taus88Process(s1, s2, s3) ),
				sign	= 1.0,
				result;
		
		if (u2 < u1) {
			// Swap, generate negative variate
			double temp = u1;
			u1 = u2;
			u2 = temp;
			sign = -1.0;
			}
	   
	    result = kKRMagic - 0.6308348019 * u1;
		if (u2 <= 0.7555915317
				|| 0.0342405038 * (u2 - u1) <= NormKRHelper(result))
			return sign * result;						// We have a variate
		} while (true);
	}
	
static inline double
NormTaus88Tail(
	UInt32* s1,
	UInt32* s2,
	UInt32* s3)
	
	{
	double result;
	
	do	{
		double	u1		= ULong2Unit_ZO( Taus88Process(s1, s2, s3) ),
				u2		= ULong2Unit_ZO( Taus88Process(s1, s2, s3) ),
				sign	= 1.0;
		
		result  = -log(u2);
		result += result;
		result += kKRMagic * kKRMagic;
		// ASSERT (result == A * A - 2 * log(u2))
		
		if (result * u1 * u1 < kKRMagic * kKRMagic) {	// We have a variate
			result = sqrt(result);
			if (Taus88Process(s1, s2, s3) & 0x01)		// Toss a coin to determine sign
		    	result = -result;
		    break;
		    }
		} while (true);
	
	return result;
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
	
	NormJitInit();
	
	// Standard Max setup() call
	setup(	&gNormMaxClass,					// Pointer to our class definition
			(method) NormNewMaxShell,		// Instance creation function
			(method) NormFreeMaxShell,		// Custom deallocation function
			(short) sizeof(msobNorm),	// Class object size
			NIL,							// No menu function
			A_GIMME,						// Jitter objects always parse their own arguments
			0);		
	
	// Jitter Magic... 
	p = max_jit_classex_setup(calcoffset(msobNorm, obex));
	q = jit_class_findbyname(gensym((char*) kJitClassName));    
    max_jit_classex_mop_wrap(p, q, kAttr); 		
    max_jit_classex_standard_wrap(p, q, 0); 	
	LITTER_TIMEBOMB max_addmethod_usurp_low((method) NormOutputMatrix, "outputmatrix");	
	
	// Back to adding messages...
	addmess	((method) NormTattle,	"dblclick",	A_CANT, 0);
	addmess	((method) NormTattle,	"tattle",	A_NOTHING);
	addmess	((method) NormAssist,	"assist",	A_CANT, 0);
	addmess	((method) NormInfo,		"info",		A_CANT, 0);
	
	// Initialize Litter Library
	LitterInit(kMaxClassName, 0);
	}


#pragma mark -
#pragma mark • Internal functions


#pragma mark -
#pragma mark • Max Shell Methods

/******************************************************************************************
 *
 *	NormNewMaxShell(iSym, iArgC, iArgV)
 *
 ******************************************************************************************/

static void*
NormNewMaxShell(
	SymbolPtr	sym,
	long		iArgC,
	Atom		iArgV[])
	
	{
	#pragma unused(sym)
	
	msobNorm*		me			= NIL;
	void*			jitObj		= NIL;
	Symbol*			classSym	= gensym((char*) kJitClassName);
	
	me = (msobNorm*) max_jit_obex_new(gNormMaxClass, classSym);
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
 *	NormFreeMaxShell(me)
 *
 ******************************************************************************************/

static void
NormFreeMaxShell(
	msobNorm* me)
	
	{
	max_jit_mop_free(me);
	jit_object_free(max_jit_obex_jitob_get(me));
	max_jit_obex_free(me);
	}


/******************************************************************************************
 *
 *	NormOutputMatrix(me)
 *
 ******************************************************************************************/

static void
NormOutputMatrix(
	msobNorm* me)
	
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
 *	NormTattle(me)
 *	NormInfo(me)
 *	NormAssist(me, iBox, iDir, iArgNum, oCStr)
 *
 *	Litter responses to standard Max messages
 *
 ******************************************************************************************/

void
NormTattle(
	msobNorm* me)
	
	{
	#pragma unused(me)
	
	post("%s state", kMaxClassName);
	// Need to access these as attributes ???
	//post("  mean: %ld", me->mu);
	//post("  sigma: %ld", me->sigma);
	
	}

void NormInfo(msobNorm* me)
	{ LitterInfo(kMaxClassName, &me->coreObject, (method) NormTattle); }

void NormAssist(msobNorm* me, void* box, long iDir, long iArgNum, char* oCStr)
	{
	#pragma unused(me, box)
	
	LitterAssist(iDir, iArgNum, strIndexInLeft, strIndexOutLeft, oCStr);
	}


#pragma mark -
#pragma mark • Jitter Methods

/******************************************************************************************
 *
 *	NormJitMatrixCalc(me, inputs, outputs)
 *
 ******************************************************************************************/

static jcobNorm* NormJitNew()
	{
	int			i;
	jcobNorm*	me = (jcobNorm*) jit_object_alloc(gNormJitClass);
	
	me->clip		= false;
	me->planeCount	= 0;
	
	for (i = 0; i < JIT_MATRIX_MAX_PLANECOUNT; i += 1) {
		me->mean[i]		= 0.0;
		me->sigma[i]	= 0.0;
		}
	
	return me;
	}

static void NormJitFree(jcobNorm* me)
	{
	#pragma unused(me)
	/* nothing to do */
	}


#pragma mark -
#pragma mark • Matrix Calculations

/******************************************************************************************
 *
 *	GausCharVector(iMu, iSigma, iCount, oMatrix)
 *	GausCharVectorClip(iMu, iSigma, iCount, oMatrix)
 *	GausLongVector(iMu, iSigma, iCount, oMatrix)
 *	GausLongVectorClip(iMu, iSigma, iCount, oMatrix)
 *	GausFloatVector(iMu, iSigma, iCount, oMatrix)
 *	GausDoubleVector(iMu, iSigma, iCount, oMatrix)
 *
 ******************************************************************************************/

static void
GausCharVector(
	double			iMu,
	double			iSigma,
	long			iCount,
	t_jit_op_info*	oMatrix)
	
	{
	Byte*		data	= (Byte*) oMatrix->p;
	long		stride  = oMatrix->stride;
	
	if (iSigma == 0.0) {
		char mu = iMu;											// typecast once
		while (iCount-- > 0) {
			*data = mu;
			data += stride;
			}
		}
	
	else {
		tTaus88Data* seeds	= Taus88GetGlobals();
		UInt32		s1, s2, s3;
	
		Taus88Load(seeds, &s1, &s2, &s3);
		
		while (iCount-- > 0) {
			UInt32	u = Taus88Process(&s1, &s2, &s3);
			double	norm;

			if (u < kKRBodyThresh)
			     norm = NormTaus88Body(&s1, &s2, &s3);
			else if (u < kKRReg1Thresh) 
				 norm = NormTaus88Region1(&s1, &s2, &s3);
			else if (u < kKRReg2Thresh)
				 norm = NormTaus88Region2(&s1, &s2, &s3);
			else if (u < kKRReg3Thresh)
				 norm = NormTaus88Region3(&s1, &s2, &s3);
			else norm = NormTaus88Tail(&s1, &s2, &s3);
			
			// Apply parameters, clip to valid range
			norm *= iSigma;
			norm += iMu + 0.5;									// Round
			
			*data = norm;
			data += stride;
			}
		
		Taus88Store(seeds, s1, s2, s3);
		}
		
	}

static void
GausCharVectorClip(
	double			iMu,
	double			iSigma,
	long			iCount,
	t_jit_op_info*	oMatrix)
	{
	Byte*		data	= (Byte*) oMatrix->p;
	long		stride  = oMatrix->stride;
	
	if (iSigma == 0.0) {
		char mu = (iMu < 0.0) ? 0 : (iMu > 255.0) ? 255 : iMu;	// typecast/clip once
		while (iCount-- > 0) {
			*data = mu;
			data += stride;
			}
		}
	
	else {
		tTaus88Data* seeds	= Taus88GetGlobals();
		UInt32		s1, s2, s3;
	
		Taus88Load(seeds, &s1, &s2, &s3);
		
		while (iCount-- > 0) {
			UInt32	u = Taus88Process(&s1, &s2, &s3);
			double	norm;

			if (u < kKRBodyThresh)
			     norm = NormTaus88Body(&s1, &s2, &s3);
			else if (u < kKRReg1Thresh) 
				 norm = NormTaus88Region1(&s1, &s2, &s3);
			else if (u < kKRReg2Thresh)
				 norm = NormTaus88Region2(&s1, &s2, &s3);
			else if (u < kKRReg3Thresh)
				 norm = NormTaus88Region3(&s1, &s2, &s3);
			else norm = NormTaus88Tail(&s1, &s2, &s3);
			
			// Apply parameters, clip to valid range
			norm *= iSigma;
			norm += iMu + 0.5;								// Round
			
			if		(norm <= 0.0)	*data = 0;
			else if (norm >= 255.0)	*data = 255;
			else					*data = norm;
			
			data += stride;
			}
		
		Taus88Store(seeds, s1, s2, s3);
		}
		
	}


static void
GausLongVector(
	double			iMu,
	double			iSigma,
	long			iCount,
	t_jit_op_info*	oMatrix)
	{
	long*		data	= (long*) oMatrix->p;
	long		stride  = oMatrix->stride;
	
	if (iSigma == 0.0) {
		long mu = iMu;											// typecast once
		
		while (iCount-- > 0) {
			*data = mu;
			data += stride;
			}
		}
	
	else {
		tTaus88Data* seeds	= Taus88GetGlobals();
		UInt32		s1, s2, s3;
		
		Taus88Load(seeds, &s1, &s2, &s3);
	
		while (iCount-- > 0) {
			UInt32	u = Taus88Process(&s1, &s2, &s3);
			double	norm;
			
			if (u < kKRBodyThresh)
			     norm = NormTaus88Body(&s1, &s2, &s3);
			else if (u < kKRReg1Thresh) 
				 norm = NormTaus88Region1(&s1, &s2, &s3);
			else if (u < kKRReg2Thresh)
				 norm = NormTaus88Region2(&s1, &s2, &s3);
			else if (u < kKRReg3Thresh)
				 norm = NormTaus88Region3(&s1, &s2, &s3);
			else norm = NormTaus88Tail(&s1, &s2, &s3);
			
			// Apply parameters
			norm *= iSigma;
			norm += iMu;
			
			*data = norm + (norm >= 0.0 ? 0.5 : -0.5);			// signed round!
			data += stride;
			}
	
		Taus88Store(seeds, s1, s2, s3);
		}
		
	}

static void
GausLongVectorClip(
	double			iMu,
	double			iSigma,
	long			iCount,
	t_jit_op_info*	oMatrix)
	{
	long*		data	= (long*) oMatrix->p;
	long		stride  = oMatrix->stride;
	
	if (iSigma == 0.0) {
		long mu;
		
		// typecast/clip once
		if (iMu < (double) kLongMin)
			 mu = kLongMin;
		else if (iMu < 0.0)
			 mu = iMu - 0.5;
		else if (iMu < (double) kLongMax - 0.5)
			 mu = iMu + 0.5;
		else mu = kLongMax;
		
		while (iCount-- > 0) {
			*data = mu;
			data += stride;
			}
		}
	
	else {
		tTaus88Data* seeds	= Taus88GetGlobals();
		UInt32		s1, s2, s3;
		
		Taus88Load(seeds, &s1, &s2, &s3);
	
		while (iCount-- > 0) {
			UInt32	u = Taus88Process(&s1, &s2, &s3);
			double	norm;
			
			if (u < kKRBodyThresh)
			     norm = NormTaus88Body(&s1, &s2, &s3);
			else if (u < kKRReg1Thresh) 
				 norm = NormTaus88Region1(&s1, &s2, &s3);
			else if (u < kKRReg2Thresh)
				 norm = NormTaus88Region2(&s1, &s2, &s3);
			else if (u < kKRReg3Thresh)
				 norm = NormTaus88Region3(&s1, &s2, &s3);
			else norm = NormTaus88Tail(&s1, &s2, &s3);
			
			// Apply parameters, clip to valid range
			norm *= iSigma;
			norm += iMu;
			
			if (norm < (double) kLongMin)
				*data = (double) kLongMin;
			else if (norm > (double) kLongMax)
				*data = (double) kLongMax;
			else *data = norm + (norm >= 0.0 ? 0.5 : -0.5);		// Round, take sign into account
			
			data += stride;
			}
	
		Taus88Store(seeds, s1, s2, s3);
		}
		
	}

static void
GausFloatVector(
	double			iMu,
	double			iSigma,
	long			iCount,
	t_jit_op_info*	oMatrix)
	
	{
	float*		data	= (float*) oMatrix->p;
	long		stride  = oMatrix->stride;
	tTaus88Data* seeds	= Taus88GetGlobals();
	UInt32		s1, s2, s3;
	
	Taus88Load(seeds, &s1, &s2, &s3);
	
	while (iCount-- > 0) {
		UInt32	u = Taus88Process(&s1, &s2, &s3);
		double	norm;
		
		if (u < kKRBodyThresh)
		     norm = NormTaus88Body(&s1, &s2, &s3);
		else if (u < kKRReg1Thresh) 
			 norm = NormTaus88Region1(&s1, &s2, &s3);
		else if (u < kKRReg2Thresh)
			 norm = NormTaus88Region2(&s1, &s2, &s3);
		else if (u < kKRReg3Thresh)
			 norm = NormTaus88Region3(&s1, &s2, &s3);
		else norm = NormTaus88Tail(&s1, &s2, &s3);
		
		// Apply parameters
		norm *= iSigma;
		norm += iMu;
		
		*data = norm;
		data += stride;
		}
	
	Taus88Store(seeds, s1, s2, s3);
	}

static void
GausDoubleVector(
	double			iMu,
	double			iSigma,
	long			iCount,
	t_jit_op_info*	oMatrix)
	
	{
	double*		data	= (double*) oMatrix->p;
	long		stride  = oMatrix->stride;
	tTaus88Data* seeds	= Taus88GetGlobals();
	UInt32		s1, s2, s3;
	
	Taus88Load(seeds, &s1, &s2, &s3);
	
	while (iCount-- > 0) {
		UInt32	u = Taus88Process(&s1, &s2, &s3);
		double	norm;
		
		if (u < kKRBodyThresh)
		     norm = NormTaus88Body(&s1, &s2, &s3);
		else if (u < kKRReg1Thresh) 
			 norm = NormTaus88Region1(&s1, &s2, &s3);
		else if (u < kKRReg2Thresh)
			 norm = NormTaus88Region2(&s1, &s2, &s3);
		else if (u < kKRReg3Thresh)
			 norm = NormTaus88Region3(&s1, &s2, &s3);
		else norm = NormTaus88Tail(&s1, &s2, &s3);
		
		// Apply parameters.
		norm *= iSigma;
		norm += iMu;
		
		*data = norm;
		data += stride;
		}
	
	Taus88Store(seeds, s1, s2, s3);
	}

/******************************************************************************************
 *
 *	RecurseDimensions(iDimCount, iDimVec, iPlaneCount, iMInfo, iBOP)
 *
 ******************************************************************************************/

static void
RecurseDimensions(
	jcobNorm*			me,
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
			double	mu		= me->mean[0],
					sigma	= me->sigma[0];
			
			for (i = iPlaneCount - 1; i > 0; i -= 1) {
				if (me->mean[i] != mu || me->sigma[i] != sigma) {
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
			if (me->clip)
				 for (i = 0; i < iDimVec[1]; i += 1) for (j = 0; j < iPlaneCount; j += 1) {
					outOpInfo.p = iBOP + i * iMInfo->dimstride[1] + j;
					GausCharVectorClip(me->mean[j], me->sigma[j], n, &outOpInfo);
					}
			else for (i = 0; i < iDimVec[1]; i += 1) for (j = 0; j < iPlaneCount; j += 1) {
					outOpInfo.p = iBOP + i * iMInfo->dimstride[1] + j;
					GausCharVector(me->mean[j], me->sigma[j], n, &outOpInfo);
					}
			}
		
		else if (iMInfo->type == _jit_sym_long) {
			if (me->clip)
				 for (i = 0; i < iDimVec[1]; i += 1) for (j = 0; j < iPlaneCount; j += 1) {
					outOpInfo.p = iBOP + i * iMInfo->dimstride[1] + j * sizeof(long);
					GausLongVectorClip(me->mean[j], me->sigma[j], n, &outOpInfo);
					}
			else for (i = 0; i < iDimVec[1]; i += 1) for (j = 0; j < iPlaneCount; j += 1) {
					outOpInfo.p = iBOP + i * iMInfo->dimstride[1] + j * sizeof(long);
					GausLongVector(me->mean[j], me->sigma[j], n, &outOpInfo);
					}
			}
		
		else if (iMInfo->type == _jit_sym_float32) {
			// Float always clips
			for (i = 0; i < iDimVec[1]; i += 1) for (j = 0; j < iPlaneCount; j += 1) {
				outOpInfo.p = iBOP + i * iMInfo->dimstride[1] + j * sizeof(float);
				GausFloatVector(me->mean[j], me->sigma[j], n, &outOpInfo);
				}
			}
		
		else if (iMInfo->type == _jit_sym_float64) {
			// Double never clips
			for (i = 0; i < iDimVec[1]; i += 1) for (j = 0; j < iPlaneCount; j += 1) {
				outOpInfo.p = iBOP + i * iMInfo->dimstride[1] + j * sizeof(double);
				GausDoubleVector(me->mean[j], me->sigma[j], n, &outOpInfo);
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
 *	NormJitMatrixCalc(me, inputs, outputs)
 *
 ******************************************************************************************/

static t_jit_err
NormJitMatrixCalc(
	jcobNorm*	me,
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
	

#pragma mark -
#pragma mark • Attribute functions

/******************************************************************************************
 *
 *	NormGetSkewKurtosis(me)
 *
 ******************************************************************************************/

	static t_jit_err 
	NormGetSkewKurtosis(
		jcobNorm*	me,
		void*		iAttr,
		long*		ioArgCount,
		Atom**		ioArgVec)
		
		{
		#pragma unused(iAttr)
		
		t_jit_err	myErr = JIT_ERR_NONE;
		int			i;
	
		if (*ioArgCount == 0 || *ioArgVec == NIL) {
			*ioArgVec = (Atom*) getbytes(me->planeCount * sizeof(Atom));
			if (*ioArgVec != NIL)
				*ioArgCount = me->planeCount;
			else {
				myErr = JIT_ERR_OUT_OF_MEM;
				goto punt;
				}
			}
		
		for (i = 0; i < me->planeCount; i += 1)
			AtomSetFloat(*ioArgVec + i, 0.0);
				
	punt:	
		return myErr;
		}
	

#pragma mark -
#pragma mark • Jitter Initialization

/******************************************************************************************
 *
 *	NormJitInit(me)
 *
 ******************************************************************************************/

	// !! convenience type, should move to MaxUtils.h or such
	typedef t_jit_object* tJitObjPtr;
	
t_jit_err
NormJitInit(void) 

	{
	const long	kAttrRWFlags	= JIT_ATTR_GET_DEFER_LOW | JIT_ATTR_SET_USURP_LOW,
				kAttrROnlyFlags	= JIT_ATTR_GET_DEFER_LOW | JIT_ATTR_SET_OPAQUE_USER;
	const int	kJitterInlets	= 0,	// No inlets for Jitter Matrices
				kJitterOutlets	= 1;	// One outlet for Jitter Matrices (Gaussian noise)
										// The Max shell object will create add'l outlets 
										// but that's none of our business.
	
	tJitObjPtr	mop,
				attr;
	
	gNormJitClass = jit_class_new(	(char*) kJitClassName,
									(method) NormJitNew,
									(method) NormJitFree,		// ?? Could be NIL ??
									sizeof(jcobNorm),
									A_CANT, 0L
									); 
	
	
	// Add matrix operator
	mop = jit_object_new(_jit_sym_jit_mop, kJitterInlets, kJitterOutlets);
	jit_class_addadornment(gNormJitClass, mop);
	
	// Add methods
	jit_class_addmethod(gNormJitClass,
						(method) NormJitMatrixCalc,
						"matrix_calc",
						A_CANT, 0L);
	
	// Add attributes
		// Clip
	attr = jit_object_new(	_jit_sym_jit_attr_offset,
							"clip",
							_jit_sym_char,
							kAttrRWFlags,
							(method) NIL, (method) NIL,
							calcoffset(jcobNorm, clip)
							);
	jit_attr_addfilterset_clip(attr, 0, 1, true, true);			// Only allow canonical true settings
	jit_class_addattr(gNormJitClass, attr);
	
		// mu/mean
	attr = jit_object_new(	_jit_sym_jit_attr_offset_array,
							"mu",
							_jit_sym_float64,
							JIT_MATRIX_MAX_PLANECOUNT,
							kAttrRWFlags,
							(method) NIL, (method) NIL,
							calcoffset(jcobNorm, planeCount),
							calcoffset(jcobNorm, mean)
							);
	jit_class_addattr(gNormJitClass, attr);
	
		// sigma/standard deviation
	attr = jit_object_new(	_jit_sym_jit_attr_offset_array,
							"sigma",
							_jit_sym_float64,
							JIT_MATRIX_MAX_PLANECOUNT,
							kAttrRWFlags,
							(method) NIL, (method) NIL,
							calcoffset(jcobNorm, planeCount),
							calcoffset(jcobNorm, sigma)
							);
	jit_attr_addfilterset_clip(attr, 0.0, 0.0, true, false);	// Must be non-negative
	jit_class_addattr(gNormJitClass, attr);
	
		// skew and kurtosis
		// Constant for all values
	attr = jit_object_new(	_jit_sym_jit_attr_offset_array,
							"skew",
							_jit_sym_float64,
							JIT_MATRIX_MAX_PLANECOUNT,
							kAttrROnlyFlags,
							(method) NormGetSkewKurtosis, (method) NIL,
							(long) 0, (long) 0);				// Dummy values
	jit_class_addattr(gNormJitClass, attr);
	attr = jit_object_new(	_jit_sym_jit_attr_offset_array,
							"kurtosis",
							_jit_sym_float64,
							JIT_MATRIX_MAX_PLANECOUNT,
							kAttrROnlyFlags,
							(method) NormGetSkewKurtosis, (method) NIL,
							(long) 0, (long) 0);				// Dummy values
	jit_class_addattr(gNormJitClass, attr);
	
	// Register class and go
	jit_class_register(gNormJitClass);
	return JIT_ERR_NONE;
	}
