/*
	File:		lbj.abbie.c

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
#include "RNGDistBeta.h"


#pragma mark • Constants

const char	kMaxClassName[]	= "lbj.abbie",			// Class name for Max
			kJitClassName[]	= "lbj-abbie";			// Class name for Jitter

const int	kMaxNN			= 31;

	// Indices for STR# resource
enum {
	strIndexInBang		= lpStrIndexLastStandard + 1,
	
	strIndexOutBeta,
	strIndexOutDump,
	
	strIndexInLeft		= strIndexInBang,
	strIndexOutLeft		= strIndexOutBeta
	};


#pragma mark • Type Definitions

	// Use this in both abbie.c and lbj.abbie.c
	// Maybe we should move it to RNGDistBeta.h
typedef union betaParams {
			tJKParams	jk;
			tBBParams	bb;
			tBCParams	bc;
			} uBetaParams;



#pragma mark • Object Structure

typedef struct {
	Object		coreObject;
	voidPtr		obex;					// The magic extended object thing.
	} msobAbbie;							// Mac Shell Object

typedef struct {
	Object		coreObject;
	
	long		alphaCount,
				betaCount;
	double		alpha[JIT_MATRIX_MAX_PLANECOUNT],	// Mean, one per plane
				beta[JIT_MATRIX_MAX_PLANECOUNT];	// Variance, one per plane
	eBetaAlg	alg[JIT_MATRIX_MAX_PLANECOUNT];
	uBetaParams	params[JIT_MATRIX_MAX_PLANECOUNT];
	
	} jcobAbbie;										// Jitter Core Object


#pragma mark • Global Variables

void*			gAbbieJitClass	= NIL;
Messlist*		gAbbieMaxClass	= NIL;


#pragma mark • Function Prototypes

	// Max methods/functions
static void*AbbieNewMaxShell	(Symbol*, long, Atom*);
static void	AbbieFreeMaxShell	(msobAbbie*);

static void AbbieOutputMatrix	(msobAbbie*);
static void AbbieTattle			(msobAbbie*);
static void	AbbieAssist			(msobAbbie*, void* , long , long , char*);
static void	AbbieInfo			(msobAbbie*);

	// Jitter methods/functions
static t_jit_err AbbieJitInit		(void);


#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark • Inline Functions


/******************************************************************************************
 *
 *	GenArcsineTaus88Core(s1, s2, s3)
 *	GenBetaJKTaus88Core(iParams, s1, s2, s3)
 *	GenArcsineTaus88Core(iParams, s1, s2, s3)
 *	GenArcsineTaus88Core(iParams, s1, s2, s3)
 *
 ******************************************************************************************/

static inline double GenArcsineTaus88Core(UInt32* s1,UInt32* s2,UInt32* s3)
	{ return 0.5 * (1.0 - sin( (ULong2Unit_ZO( Taus88Process(s1, s2, s3) ) - 0.5) * kPi)); }

static inline double
GenBetaJKTaus88Core(
	const tJKParams*	iParams,
	UInt32*				s1,
	UInt32*				s2,
	UInt32*				s3)
	
	{
	double	a1	= iParams->a1,
			b1	= iParams->b1,
			u1, u2;
	
	do  {
	    u1 = ULong2Unit_zO( Taus88Process(s1, s2, s3) ),
	    u2 = ULong2Unit_zO( Taus88Process(s1, s2, s3) );
		// The following is a slightly compressed implementation of the more explicit:
		//		y1 = u1 ^ 1/a
		//		y2 = u2 ^ 1/b;
		//		s = y1 + y2;
		u1 = pow(u1, a1);
		u2 = pow(u2, b1);
		u2 += u1;				// stash sum in u2 instead of separate register
		} while (u2 > 1.0);
	
    return u1 / u2;   
	}
	
static inline double
GenBetaBBTaus88Core(
	const tBBParams*	iParams,
	UInt32*				s1,
	UInt32*				s2,
	UInt32*				s3)
	
	{
	double	a		= iParams->a,
			b		= iParams->b,
			alpha	= iParams->alpha,
			beta	= iParams->beta,
			gamma	= iParams->gamma,
			r, s, t, v, w, z;
	
	do  {
	    double	u1 = ULong2Unit_zo( Taus88Process(s1, s2, s3) ),
	    		u2 = ULong2Unit_zo( Taus88Process(s1, s2, s3) );
		
		v  = beta * log(u1 / (1.0-u1)),
	    w  = a * exp(v);						// ?? Can this overflow ??
		z  = u1 * u1 * u2,
		r  = gamma * v - 1.3862944;
		s = a + r - w;
	    		
	    if (s + 2.609438 >= 5.0 * z) break;
	    t = log(z);
	    if (s > t) break;
		} while (r + alpha * log(alpha/(b+w)) < t);
	
    return iParams->mirror ? b / (b+w) : w / (b+w);   
	}

static inline double
GenBetaBCTaus88Core(
	const tBCParams*	iParams,
	UInt32*				s1,
	UInt32*				s2,
	UInt32*				s3)
	
	{
	double	a		= iParams->a,
			b		= iParams->b,
			alpha	= iParams->alpha,
			beta	= iParams->beta,
//			delta	= iParams->delta,
			k1		= iParams->k1,
			k2		= iParams->k2,
			v, w, z, bw1;
	
	do	{ 
        double	u1 = ULong2Unit_zo( Taus88Process(s1, s2, s3) ),
        		u2 = ULong2Unit_zo( Taus88Process(s1, s2, s3) );

        if (u1 < 0.5) {
		    v = u1 * u2;			// Temporarily misuse register v for the following test
		    z = u1 * v;
		    if (0.25 * u2 + z >= k1 + v) continue;
		    // ...otherwise fall through to the tests at the end of the loop
			}
		else {
		    z = u1 * u1 * u2;
		    if (z <= 0.25) {
				// We can skip the more complex loop test condition below. We still need to
				// calculate w and bw1, however.
				// FORTRAN implementations add extra code to trap potential
				// overflow/underflow conditions. Given that our parameters have, at most,
				// data from the float/integer domain, whereas the following is calculated
				// with 64-bit precision, overflow is almost impossible.
				w = a * pow(u1/(1.0-u1), beta);
				bw1 = 1.0 / (b + w);
				break;
			    }
		    if (z >= k2) continue;
		    // ...otherwise fall through to the tests at the end of the loop
			}
        
        // We still don't need to worry about overflow/underflow in calculating w, but the
        // intermediate result stored in the variable v is needed for the test condition
        v = beta * log(u1/(1.0-u1));
        w = a * exp(v);
        bw1 = 1.0 / (b + w);
		} while (pow(v * alpha * bw1, alpha) < 4.0 * z);
	        // Some implementations formulate the test as follows (or equivalent):
	        //
	        //		alpha * (log(alpha/(b+w) + v) + v) < log(z) + 1.3862943611
	        //
	        // The expression used needs one transcendental math function less, which
	        // is generally a Good Idea.
	
    return bw1 * (iParams->mirror ? b : w);   
	}



#pragma mark -

/******************************************************************************************
 *
 *	main()
 *	
 *	Standard Max External Object Entry Point Function
 *	
 ******************************************************************************************/

int
main(void)
	
	{
	const long kFlags = MAX_JIT_MOP_FLAGS_OWN_OUTPUTMATRIX
						+ MAX_JIT_MOP_FLAGS_OWN_JIT_MATRIX;
	
	voidPtr	p,									// Have to guess about what these two do
			q;									// Not much is documented in the Jitter SDK
	
	LITTER_CHECKTIMEOUT(kMaxClassName);
	
	AbbieJitInit();
	
	// Standard Max setup() call
	setup(	&gAbbieMaxClass,				// Pointer to our class definition
			(method) AbbieNewMaxShell,		// Instance creation function
			(method) AbbieFreeMaxShell,		// Custom deallocation function
			(short) sizeof(msobAbbie),		// Class object size
			NIL,							// No menu function
			A_GIMME,						// Jitter objects always parse their own arguments
			0);		
	
	// Jitter Magic... 
	p = max_jit_classex_setup(calcoffset(msobAbbie, obex));
	q = jit_class_findbyname(gensym((char*) kJitClassName));    
    max_jit_classex_mop_wrap(p, q, kFlags); 		
    max_jit_classex_standard_wrap(p, q, 0); 	
	LITTER_TIMEBOMB max_addmethod_usurp_low((method) AbbieOutputMatrix, "outputmatrix");	
	
	// Back to adding messages...
	addmess	((method) AbbieTattle,	"dblclick",	A_CANT, 0);
	addmess	((method) AbbieTattle,	"tattle",	A_NOTHING);
	addmess	((method) AbbieAssist,	"assist",	A_CANT, 0);
	addmess	((method) AbbieInfo,	"info",		A_CANT, 0);
	
	// Initialize Litter Library
	LitterInit(kMaxClassName, 0);
	
	}


#pragma mark -
#pragma mark • Internal functions



#pragma mark -
#pragma mark • Max Shell Methods

/******************************************************************************************
 *
 *	AbbieNewMaxShell(iSym, iArgC, iArgV)
 *
 ******************************************************************************************/

static void*
AbbieNewMaxShell(
	SymbolPtr	sym,
	long		iArgC,
	Atom		iArgV[])
	
	{
	#pragma unused(sym)
	
	msobAbbie*		me			= NIL;
	void*			jitObj		= NIL;
	Symbol*			classSym	= gensym((char*) kJitClassName);
	
	me = (msobAbbie*) max_jit_obex_new(gAbbieMaxClass, classSym);
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
 *	AbbieFreeMaxShell(me)
 *
 ******************************************************************************************/

static void
AbbieFreeMaxShell(
	msobAbbie* me)
	
	{
	max_jit_mop_free(me);
	jit_object_free(max_jit_obex_jitob_get(me));
	max_jit_obex_free(me);
	}


/******************************************************************************************
 *
 *	AbbieOutputMatrix(me)
 *
 ******************************************************************************************/

static void
AbbieOutputMatrix(
	msobAbbie* me)
	
	{
	void*		mop = max_jit_obex_adornment_get(me, _jit_sym_jit_mop);
	t_jit_err	myErr = noErr;	
	
	// Sanity check: don't output if mop is NIL
	if (mop == NIL)
		return;
	
	if (max_jit_mop_getoutputmode(me) == 1) {
		myErr = (t_jit_err) jit_object_method(
								max_jit_obex_jitob_get(me), 
								_jit_sym_matrix_calc,
								jit_object_method(mop, _jit_sym_getinputlist),
								jit_object_method(mop, _jit_sym_getoutputlist));
		
		if (myErr == noErr)						
				max_jit_mop_outputmatrix(me);
		else	jit_error_code(me, myErr); 
		}
		
	}


/******************************************************************************************
 *
 *	AbbieTattle(me)
 *	AbbieInfo(me)
 *	AbbieAssist(me, iBox, iDir, iArgNum, oCStr)
 *
 *	Litter responses to standard Max messages
 *
 ******************************************************************************************/

void
AbbieTattle(
	msobAbbie* me)
	
	{
	#pragma unused(me)
	
	post("%s state", kMaxClassName);
	// Need to access these as attributes ???
	//post("  mean: %ld", me->mu);
	//post("  sigma: %ld", me->sigma);
	
	}

void AbbieInfo(msobAbbie* me)
	{ LitterInfo(kMaxClassName, &me->coreObject, (method) AbbieTattle); }

void AbbieAssist(msobAbbie* me, void* box, long iDir, long iArgNum, char* oCStr)
	{
	#pragma unused(me, box)
	
	LitterAssist(iDir, iArgNum, strIndexInLeft, strIndexOutLeft, oCStr);
	}


#pragma mark -
#pragma mark • Jitter Methods

/******************************************************************************************
 *
 *	AbbieJitMatrixCalc(me, inputs, outputs)
 *
 ******************************************************************************************/

static jcobAbbie* AbbieJitNew()
	{
	int			i;
	jcobAbbie*	me = (jcobAbbie*) jit_object_alloc(gAbbieJitClass);
	
	for (i = 0; i < JIT_MATRIX_MAX_PLANECOUNT; i += 1) {
		me->alpha[i]		= 0.5;
		me->beta[i]			= 0.5;
		me->alg[i]			= algArcSine;
		me->params[i].jk.a1	= 2.0;					// Not strictly necessary
		me->params[i].jk.b1 = 2.0;
		}
	
	me->alphaCount	= 0;
	me->betaCount	=0;
	
	return me;
	}

static void AbbieJitFree(jcobAbbie* me)
	{
	#pragma unused(me)
	/* nothing to do */
	}


/******************************************************************************************
 *
 *	RecurseDimensions(iDimCount, iDimVec, iPlaneCount, iMInfo, iBOP)
 *
 ******************************************************************************************/

	static inline Byte Unit2Byte(double x)
		{ return (Byte) (255.0 * x + 0.5); }

static void
AbbieCharVector(
	eBetaAlg			iAlg,
	const uBetaParams*	iParams,
	long				iCount,
	t_jit_op_info*		oMatrix)
	
	{
	Byte*		data	= (Byte*) oMatrix->p;
	long		stride  = oMatrix->stride;
	
	if (iAlg == algConstZero) while (iCount-- > 0) {
		*data = 0;
		data += stride;
		}
	
	else if (iAlg == algConstOne) while (iCount-- > 0) {
		*data = 255;
		data += stride;
		}
	
	else {
		double			beta;
		tTaus88Data*	tausData = Taus88GetGlobals();
		UInt32			s1, s2, s3;
		
		Taus88Load(tausData, &s1, &s2, &s3);
		
		switch (iAlg) {
		case algArcSine:
			// a == b == 0.5: Arc Sine distribution
			while (iCount-- > 0) {
				beta = GenArcsineTaus88Core(&s1, &s2, &s3);
				*data = Unit2Byte(beta);
				data += stride;
				}
			break;
		
		case algJoehnk:
			while (iCount-- > 0) {
				beta = GenBetaJKTaus88Core((tJKParams*) iParams, &s1, &s2, &s3);
				*data = Unit2Byte(beta);
				data += stride;
				}
			break;
		
		case algChengBB:
			while (iCount-- > 0) {
				beta = GenBetaBBTaus88Core((tBBParams*) iParams, &s1, &s2, &s3);
				*data = Unit2Byte(beta);
				data += stride;
				}
			break;
		
		case algChengBC:
			while (iCount-- > 0) {
				beta = GenBetaBCTaus88Core((tBCParams*) iParams, &s1, &s2, &s3);
				*data = Unit2Byte(beta);
				data += stride;
				}
			break;
			
		case algIndeterm:
			while (iCount > 0) {
				UInt32	bits	= Taus88Process(&s1, &s2, &s3),
						bitCount = (iCount > 32) ? 32 : iCount;
						
				iCount -= bitCount;
				
				do	{
					*data = (bits & 0x01) ? 0 : 255;
					data += stride;
					bits >>= 1;
					bitCount -= 1;
					} while (bitCount > 0);
				}
			break;
		
		default:
			// a == b == 1: Uniform distribution
			while (iCount > 0) {
				UInt32	bytes	= Taus88Process(&s1, &s2, &s3),
						byteCount = (iCount > 4) ? 4 : iCount;
						
				iCount -= byteCount;
				
				do	{
					*data = bytes & 0x000000ff;
					data += stride;
					bytes >>= 4;
					byteCount -= 1;
					} while (byteCount > 0);
				}
			break;
			}
		
		Taus88Store(tausData, s1, s2, s3);
		}
	
	}
	
static void
AbbieLongVector(
	eBetaAlg			iAlg,
	const uBetaParams*	iParams,
	long				iCount,
	t_jit_op_info*		oMatrix)
	
	{
	long*		data	= (long*) oMatrix->p;
	long		stride  = oMatrix->stride;
	
	if (iAlg == algConstZero) while (iCount-- > 0) {
		*data = 0;
		data += stride;
		}
	
	else if (iAlg == algConstOne) while (iCount-- > 0) {
		*data = 255;
		data += stride;
		}
	
	else {
		double			beta;
		tTaus88Data*	tausData = Taus88GetGlobals();
		UInt32			s1, s2, s3;
		
		Taus88Load(tausData, &s1, &s2, &s3);
		
		switch (iAlg) {
		case algArcSine:
			// a == b == 0.5: Arc Sine distribution
			while (iCount-- > 0) {
				beta = GenArcsineTaus88Core(&s1, &s2, &s3);
				*data = Unit2Byte(beta);
				data += stride;
				}
			break;
		
		case algJoehnk:
			while (iCount-- > 0) {
				beta = GenBetaJKTaus88Core((tJKParams*) iParams, &s1, &s2, &s3);
				*data = Unit2Byte(beta);
				data += stride;
				}
			break;
		
		case algChengBB:
			while (iCount-- > 0) {
				beta = GenBetaBBTaus88Core((tBBParams*) iParams, &s1, &s2, &s3);
				*data = Unit2Byte(beta);
				data += stride;
				}
			break;
		
		case algChengBC:
			while (iCount-- > 0) {
				beta = GenBetaBCTaus88Core((tBCParams*) iParams, &s1, &s2, &s3);
				*data = Unit2Byte(beta);
				data += stride;
				}
			break;
			
		case algIndeterm:
			while (iCount > 0) {
				UInt32	bits	= Taus88Process(&s1, &s2, &s3),
						bitCount = (iCount > 32) ? 32 : iCount;
						
				iCount -= bitCount;
				
				do	{
					*data = (bits & 0x01) ? 0 : 255;
					data += stride;
					bits >>= 1;
					bitCount -= 1;
					} while (bitCount > 0);
				}
			break;
		
		default:
			// a == b == 1: Uniform distribution
			while (iCount > 0) {
				UInt32	bytes	= Taus88Process(&s1, &s2, &s3),
						byteCount = (iCount > 4) ? 4 : iCount;
						
				iCount -= byteCount;
				
				do	{
					*data = bytes & 0x000000ff;
					data += stride;
					bytes >>= 4;
					byteCount -= 1;
					} while (byteCount > 0);
				}
			break;
			}
		
		Taus88Store(tausData, s1, s2, s3);
		}
	
	}

static void 
AbbieFloatVector(
	eBetaAlg			iAlg,
	const uBetaParams*	iParams,
	long				iCount,
	t_jit_op_info*		oMatrix)
	
	{
	float*		data	= (float*) oMatrix->p;
	long		stride  = oMatrix->stride;
	
	if (iAlg == algConstZero) while (iCount-- > 0) {
		*data = 0.0;
		data += stride;
		}
	
	else if (iAlg == algConstOne) while (iCount-- > 0) {
		*data = 1.0;
		data += stride;
		}
	
	else {
		tTaus88Data*	tausData = Taus88GetGlobals();
		UInt32			s1, s2, s3;
		
		Taus88Load(tausData, &s1, &s2, &s3);
		
		switch (iAlg) {
		case algArcSine:
			// a == b == 0.5: Arc Sine distribution
			while (iCount-- > 0) {
				*data = GenArcsineTaus88Core(&s1, &s2, &s3);
				data += stride;
				}
			break;
		
		case algJoehnk:
			while (iCount-- > 0) {
				*data = GenBetaJKTaus88Core((tJKParams*) iParams, &s1, &s2, &s3);
				data += stride;
				}
			break;
		
		case algChengBB:
			while (iCount-- > 0) {
				*data = GenBetaBBTaus88Core((tBBParams*) iParams, &s1, &s2, &s3);
				data += stride;
				}
			break;
		
		case algChengBC:
			while (iCount-- > 0) {
				*data = GenBetaBCTaus88Core((tBCParams*) iParams, &s1, &s2, &s3);
				data += stride;
				}
			break;
			
		case algIndeterm:
			while (iCount > 0) {
				UInt32	bits	= Taus88Process(&s1, &s2, &s3),
						bitCount = (iCount > 32) ? 32 : iCount;
						
				iCount -= bitCount;
				
				do	{
					*data = (bits & 0x01) ? 0.0 : 1.0;
					data += stride;
					bits >>= 1;
					bitCount -= 1;
					} while (bitCount > 0);
				}
			break;
		
		default:
			// a == b == 1: Uniform distribution
			while (iCount-- > 0) {
				*data = ULong2Unit_ZO( Taus88Process(&s1, &s2, &s3) );
				data += stride;
				}
			break;
			}
		
		Taus88Store(tausData, s1, s2, s3);
		}
	
	}

static void 
AbbieDoubleVector(
	eBetaAlg			iAlg,
	const uBetaParams*	iParams,
	long				iCount,
	t_jit_op_info*		oMatrix)
	
	{
	double*		data	= (double*) oMatrix->p;
	long		stride  = oMatrix->stride;
	
	if (iAlg == algConstZero) while (iCount-- > 0) {
		*data = 0.0;
		data += stride;
		}
	
	else if (iAlg == algConstOne) while (iCount-- > 0) {
		*data = 1.0;
		data += stride;
		}
	
	else {
		tTaus88Data*	tausData = Taus88GetGlobals();
		UInt32			s1, s2, s3;
		
		Taus88Load(tausData, &s1, &s2, &s3);
		
		switch (iAlg) {
		case algArcSine:
			// a == b == 0.5: Arc Sine distribution
			while (iCount-- > 0) {
				*data = GenArcsineTaus88Core(&s1, &s2, &s3);
				data += stride;
				}
			break;
		
		case algJoehnk:
			while (iCount-- > 0) {
				*data = GenBetaJKTaus88Core((tJKParams*) iParams, &s1, &s2, &s3);
				data += stride;
				}
			break;
		
		case algChengBB:
			while (iCount-- > 0) {
				*data = GenBetaBBTaus88Core((tBBParams*) iParams, &s1, &s2, &s3);
				data += stride;
				}
			break;
		
		case algChengBC:
			while (iCount-- > 0) {
				*data = GenBetaBCTaus88Core((tBCParams*) iParams, &s1, &s2, &s3);
				data += stride;
				}
			break;
			
		case algIndeterm:
			while (iCount > 0) {
				UInt32	bits	= Taus88Process(&s1, &s2, &s3),
						bitCount = (iCount > 32) ? 32 : iCount;
						
				iCount -= bitCount;
				
				do	{
					*data = (bits & 0x01) ? 0.0 : 1.0;
					data += stride;
					bits >>= 1;
					bitCount -= 1;
					} while (bitCount > 0);
				}
			break;
		
		default:
			// a == b == 1: Uniform distribution
			while (iCount-- > 0) {
				*data = ULong2Unit_ZO( Taus88Process(&s1, &s2, &s3) );
				data += stride;
				}
			break;
			}
		
		Taus88Store(tausData, s1, s2, s3);
		}
	
	}

static void
RecurseDimensions(
	jcobAbbie*			me,
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
			double	alpha	= me->alpha[0],
					beta	= me->beta[0];
			
			for (i = iPlaneCount; --i > 0; ) {
				if (me->alpha[i] != alpha || me->beta[i] != beta) {
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
				AbbieCharVector(me->alg[j], me->params + j, n, &outOpInfo);
				}
			}
		
		else if (iMInfo->type == _jit_sym_long) {
			for (i = 0; i < iDimVec[1]; i += 1) for (j=0; j < iPlaneCount; j += 1) {
				outOpInfo.p = iBOP + i * iMInfo->dimstride[1] + j * sizeof(long);
				AbbieLongVector(me->alg[j], me->params + j, n, &outOpInfo);
				}
			}
		
		else if (iMInfo->type == _jit_sym_float32) {
			for (i = 0; i < iDimVec[1]; i += 1) for (j=0; j < iPlaneCount; j += 1) {
				outOpInfo.p = iBOP + i * iMInfo->dimstride[1] + j * sizeof(float);
				AbbieFloatVector(me->alg[j], me->params + j, n, &outOpInfo);
				}
			}
		
		else if (iMInfo->type == _jit_sym_float64) {
			for (i = 0; i < iDimVec[1]; i += 1) for (j=0; j < iPlaneCount; j += 1) {
				outOpInfo.p = iBOP + i * iMInfo->dimstride[1] + j * sizeof(double);
				AbbieDoubleVector(me->alg[j], me->params + j, n, &outOpInfo);
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
 *	AbbieJitMatrixCalc(me, inputs, outputs)
 *
 ******************************************************************************************/

static t_jit_err
AbbieJitMatrixCalc(
	jcobAbbie*	me,
	void*		inputs,
	void*		outputs)
	
	{
	#pragma unused(inputs)
	
	t_jit_err			myErr = JIT_ERR_NONE;
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
		long	dim[JIT_MATRIX_MAX_DIMCOUNT],
				i;
		
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
	else myErr = JIT_ERR_INVALID_OUTPUT;
	
	jit_object_method(outMatrix, _jit_sym_lock, outSaveLock);
	
	return myErr;
	}
	

/******************************************************************************************
 *
 *	AbbieJitInit(me)
 *
 ******************************************************************************************/

	static void AbbieSetAlpha(jcobAbbie* me, void* attr, long iArgC, Atom iArgVec[])
		{
		#pragma unused(attr)
		
		int i;
		
		// Sanity checks
		if (iArgC <= 0 || iArgVec == NIL)
			return;
		if (iArgC > JIT_MATRIX_MAX_PLANECOUNT)
			iArgC = JIT_MATRIX_MAX_PLANECOUNT;
		
		for (i = 0; i < JIT_MATRIX_MAX_PLANECOUNT; i += 1) {
			double alpha = AtomGetFloat(&iArgVec[i%iArgC]);
			
			// Final sanity check
			if (alpha < 0.0)
				alpha = 0.0;
			
			// Is this really a change in attribute value?
			// Updating is a lot of work, that's why we check before launching
			// into the next block of code.
			if (me->alpha[i] != alpha) {
				// OK. Update alpha, algorithm, recalc params
				const double beta = me->beta[i];
			
				me->alpha[i] = alpha;
				
				switch (me->alg[i] = RecommendBetaAlg(alpha, beta)) {
				case algJoehnk:
					CalcJKParams((tJKParams*) (me->params + i), alpha, beta);
					break;
				case algChengBB:
					CalcBBParams((tBBParams*) (me->params + i), alpha, beta);
					break;
				case algChengBC:
					CalcBCParams((tBCParams*) (me->params + i), alpha, beta);
					break;
				default:
					// No special parameter calculations for other generators
					break;
					}						// end switch (me->alg[i] ...)
				}							// end if (me=>alpha[i] != alpha)
			}								// end for (i = 0; ... )
		
		me->alphaCount = iArgC;
		
		}
	
	static void AbbieSetBeta(jcobAbbie* me, void* attr, long iArgC, Atom iArgVec[])
		{
		#pragma unused(attr)
		
		int i;
		
		// Sanity checks
		if (iArgC <= 0 || iArgVec == NIL)
			return;
		if (iArgC > JIT_MATRIX_MAX_PLANECOUNT)
			iArgC = JIT_MATRIX_MAX_PLANECOUNT;
		
		for (i = 0; i < JIT_MATRIX_MAX_PLANECOUNT; i += 1) {
			double beta = AtomGetFloat(&iArgVec[i%iArgC]);
			
			// Final sanity check
			if (beta < 0.0)
				beta = 0.0;
			
			// Is this really a change in attribute value?
			// Updating is a lot of work, that's why we check before launching
			// into the next block of code.
			if (me->beta[i] !=beta) {
				// OK. Update alpha, algorithm, recalc params
				const double alpha = me->alpha[i];
				
				me->beta[i] = beta;
				
				switch (me->alg[i] = RecommendBetaAlg(alpha, beta)) {
				case algJoehnk:
					CalcJKParams((tJKParams*) (me->params + i), alpha, beta);
					break;
				case algChengBB:
					CalcBBParams((tBBParams*) (me->params + i), alpha, beta);
					break;
				case algChengBC:
					CalcBCParams((tBCParams*) (me->params + i), alpha, beta);
					break;
				default:
					// No special parameter calculations for other generators
					break;
					}						// end switch (me->alg[i] ...)
				}							// end if (me=>beta[i] != beta)
			}								// end for (i = 0; ... )
		
		me->betaCount = iArgC;
		
		}
	
	
	// !! convenience type, should move to MaxUtils.h or such
	typedef t_jit_object* tJitObjPtr;

t_jit_err
AbbieJitInit(void) 

	{
	const long	kAttrRWFlags	= JIT_ATTR_GET_DEFER_LOW | JIT_ATTR_SET_USURP_LOW,
				kAttrROnlyFlags	= JIT_ATTR_GET_DEFER_LOW | JIT_ATTR_SET_OPAQUE_USER;
	const int	kJitterInlets	= 0,	// No inlets for Jitter Matrices
				kJitterOutlets	= 1;	// One outlet for Jitter Matrices (Beta noise)
	
	tJitObjPtr	mop,
				attr;
	
	gAbbieJitClass = jit_class_new(	(char*) kJitClassName,
									(method) AbbieJitNew,
									(method) AbbieJitFree,		// ?? Could be NIL ??
									sizeof(jcobAbbie),
									A_CANT, 0L
									); 
	
	// Add matrix operator
	mop = jit_object_new(_jit_sym_jit_mop, kJitterInlets, kJitterOutlets);
	jit_class_addadornment(gAbbieJitClass, mop);
	
	// Add methods
	jit_class_addmethod(gAbbieJitClass,
						(method) AbbieJitMatrixCalc,
						"matrix_calc",
						A_CANT, 0L);
	
	// Add attributes
		// alpha
	attr = jit_object_new(	_jit_sym_jit_attr_offset_array,
							"alpha",
							_jit_sym_float64,
							JIT_MATRIX_MAX_PLANECOUNT,
							kAttrRWFlags,
							NIL, (method) AbbieSetAlpha,
							calcoffset(jcobAbbie, alphaCount),
							calcoffset(jcobAbbie, alpha)
							);
	jit_class_addattr(gAbbieJitClass, attr);
	
		// beta
	attr = jit_object_new(	_jit_sym_jit_attr_offset_array,
							"beta",
							_jit_sym_float64,
							JIT_MATRIX_MAX_PLANECOUNT,
							kAttrRWFlags,
							NIL, (method) AbbieSetBeta,
							calcoffset(jcobAbbie, betaCount),
							calcoffset(jcobAbbie, beta)
							);
	jit_class_addattr(gAbbieJitClass, attr);
	
	
	// Register class and go
	jit_class_register(gAbbieJitClass);
	return JIT_ERR_NONE;
	}
