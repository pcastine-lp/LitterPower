/*
	File:		pfishie.c

	Contains:	Max external object generating a Poisson distribution.

	Written by:	Peter Castine

	Copyright:	© 2001-2002 Peter Castine

	Change History (most recent first):

         <8>   23–3–2006    pc      Implement alternate generating algorithms (more efficient for
                                    large lambda)
         <7>     15–3–06    pc      Add expect message
         <6>     15–1–04    pc      Avoid possible memory leak if seed argument is used.
         <5>     14–1–04    pc      Update for Windows.
         <4>    7–7–2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <3>  30–12–2002    pc      Drop faux 'Vers' resource and related modifications. Add object
                                    version to DoInfo().
         <2>  28–11–2002    pc      Tidy up after initial check in.
         <1>  28–11–2002    pc      Initial check in.
	Change History (most recent first):
		30-Jun-2001:	Modified to use Taus88 instead of TT800 as default generator.
		2-Apr-2001:		First implementation.
	
	To Do:
		Use rejection method for large lambda. _Numerical Recipes_ does this for lambda
		larger than 12, but I think the threshhold should be higher, around 20 or 25.
		It will be necessary to implement Gamma() and LnGamma(). There may be alternative
		methods.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "TrialPeriodUtils.h"
#include "RNGPoisson.h"


#pragma mark • Constants

const char*		kClassName		= "lp.pfishie";			// Class name


	// Indices for STR# resource
enum {
	strIndexInBang		= lpStrIndexLastStandard + 1,
	strIndexInLambda,
	
	strIndexTheOutlet
	};


#pragma mark • Type Definitions

typedef union {
	double			lambda;				// Just lambda
	tPoisInvParams	invParams;			// lambda & thresh
	tPoisRejParams	rejParams;
	} uPoisParams;

#pragma mark • Object Structure

typedef struct {
	LITTER_CORE_OBJECT(Object, coreObject);
	
	tTaus88DataPtr	tausData;
	ePoisAlg		alg;
	uPoisParams		params;
			
	} objPoisson;


#pragma mark • Function Prototypes

	// Object message functions
static void PfishieLambda(objPoisson*, double);


#pragma mark -

/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	PfishieNew(iOracle)
 *
 ******************************************************************************************/

static void*
PfishieNew(
	double	iLambda,
	long	iSeed)
	
	{
	const double	kDefLambda	= 1.0;
	
	objPoisson*		me			= NIL;
	tTaus88DataPtr	myTausData	= NIL;
	
	// Run through initialization parameters from right to left, handling defaults
	if (iSeed == 0) Taus88Init();						// Use Taus88's data pool
	else {
		myTausData = Taus88New(iSeed);
		goto noMoreDefaults;
		}
	
	if (iLambda == 0.0)
		iLambda = kDefLambda;
noMoreDefaults:
	// Finished checking intialization parameters
	
	
	// Let Max allocate us, our inlets, and outlets
	me = (objPoisson*) LitterAllocateObject();

	floatin(me, 1);							// lambda inlet
	intout(me);								// Access through me->coreObject.o_outlet
	
	// Store object components
	me->tausData	= myTausData;
	PfishieLambda(me, iLambda);				// Sets up all other members
	
	return me;
	}

static void PfishieFree(objPoisson* me)
	{ Taus88Free(me->tausData); }							// Taus88Free is NIL-safe


#pragma mark -
#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	PfishieBang(me)
 *
 ******************************************************************************************/

static void
PfishieBang(
	objPoisson* me)
	
	{
	long p;
	
	switch (me->alg) {
	case algReject:
		p = GenPoissonRejTaus88(&me->params.rejParams, me->tausData);
		break;
	case algInversion:
		p = GenPoissonInvTaus88(&me->params.invParams, me->tausData);
		break;
	case algDirect:
		p = GenPoissonDirTaus88(me->params.invParams.thresh, me->tausData);
		break;
	default:
		// Must be algConstZero
		// Or algUndef, which is an error, but our lambda method prevents that
		p = 0;
		break;
		}
	
	outlet_int(me->coreObject.o_outlet, p);
	}


/******************************************************************************************
 *
 *	PfishieLambda(me, iLambda)
 *	PfishieSeed(me, iSeed)
 *	
 *	Set parameter. Make sure nothing bad is happening.
 *	
 ******************************************************************************************/

static void PfishieLambda(objPoisson* me, double iLambda)
	{
	
	if (iLambda < 0.0)
		iLambda = 0.0;
	
	switch (me->alg = RecommendPoisAlg(iLambda)) {
	case algReject:
		CalcPoisRejParams(&me->params.rejParams, iLambda);
		break;
	case algInversion:
		CalcPoisInvParams(&me->params.invParams, iLambda);
		break;
	case algDirect:
		me->params.invParams.lambda	= iLambda;
		me->params.invParams.thresh = CalcPoisDirThresh(iLambda);
		break;
	default:
		// Must be algConstZero; algUndef can't hapen
		me->params.lambda	= 0.0;
		break;
		}
	}
	


static void PfishieSeed(objPoisson* me, long iSeed)
	{ Taus88Seed(me->tausData, (unsigned long) iSeed); }


#pragma mark -
#pragma mark • Attribute/Information Functions

/******************************************************************************************
 *
 *	PfishieTattle(me)
 *	PfishieAssist
 *
 ******************************************************************************************/

static void
PfishieTattle(
	objPoisson* me)
	
	{
	
	post("%s state",
			kClassName);
	post("  lambda = %lf", me->params.lambda);
	post("  Using algorithm:");
	switch (me->alg) {
	case algReject:
		post("    Ahrens & Dieter rejection method");
		post("    sqrt(2l) = %lf", me->params.rejParams.sqrt2Lambda);
		post("    ln(l)    = %lf", me->params.rejParams.logLambda);
		post("    magic    = %lf", me->params.rejParams.lambdaMagic);
		break;
	case algInversion:
		post("    Inversion");
		post("    threshhold = %lf", me->params.invParams.thresh);
		break;
	case algDirect:
		post("    Direct generation");
		post("    threshhold = %lf", me->params.invParams.thresh);
		break;
	case algConstZero:
		post("    Degenerate distribution (always zero)");
		break;
	default:
		post("    -- yet not determined --");
		}
		
	}

static void PfishieAssist(objPoisson* me, void* iBox, long iDir, long iArgNum, char* oCStr)
	
	{
	#pragma unused (me, iBox)
	
	LitterAssist(iDir, iArgNum, strIndexInBang, strIndexTheOutlet, oCStr);
	}



/******************************************************************************************
 *
 *	DoExpect()
 *
 *	Helper function for calculating expected values (mean, standard deviation, etc.)
 *	
 ******************************************************************************************/

static double
DoExpect(
	objPoisson*		me,
	eExpectSelector iSel)
	
	{
	double	result	= 0.0 / 0.0,
			lambda	= me->params.lambda;
	
	switch(iSel) {
	case expMean:
	case expVar:
		result = lambda;
		break;
	case expMedian:					// Median is said to be undefined ??
		break;
	case expMode:
		result = floor(lambda);		// Integer lambda -> dual mode (also lambda + 1)
		break;
	case expStdDev:
		result = sqrt(lambda);
		break;
	case expSkew:
		result = 1.0 / sqrt(lambda);
		break;
	case expKurtosis:
		result = 1.0 / lambda;
		break;
	case expMin:
		result = 0.0;
		break;
	case expMax:
		result = 1.0 / 0.0;			// Unbounded
		break;
	case expEntropy:
		// result = lambda * (1-log(lambda)) + exp(-lambda)
		//				* SUM-k-0-to-Inf (pow(lambda, k) * LogFactorial(K) / Factorial(k))
		//
		//	This could be caluclated sufficiently accurately with brute force...
		break;
	default:
		break;
		}
		
	return result;
	}


/******************************************************************************************
 *
 *	Attribute Funtions
 *
 *	Functions for Inspector/Get Info... dialog
 *	
 ******************************************************************************************/

#if LITTER_USE_OBEX

	static t_max_err PfishieGetMin(objPoisson* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expMin), ioArgC, ioArgV);
		}
	static t_max_err PfishieGetMax(objPoisson* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expMax), ioArgC, ioArgV);
		}
	static t_max_err PfishieGetMean(objPoisson* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expMean), ioArgC, ioArgV);
		}
//	static t_max_err PfishieGetMedian(objPoisson* me, void* iAttr, long* ioArgC, Atom** ioArgV)
//		{ 
//		#pragma unused(iAttr)
//		
//		return LitterGetAttrFloat(DoExpect(me, expMedian), ioArgC, ioArgV);
//		}
	static t_max_err PfishieGetMode(objPoisson* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expMode), ioArgC, ioArgV);
		}
	static t_max_err PfishieGetVar(objPoisson* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expVar), ioArgC, ioArgV);
		}
	static t_max_err PfishieGetStdDev(objPoisson* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expStdDev), ioArgC, ioArgV);
		}
	static t_max_err PfishieGetSkew(objPoisson* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expSkew), ioArgC, ioArgV);
		}
	static t_max_err PfishieGetKurtosis(objPoisson* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expKurtosis), ioArgC, ioArgV);
		}
//	static t_max_err PfishieGetEntropy(objPoisson* me, void* iAttr, long* ioArgC, Atom** ioArgV)
//		{ 
//		#pragma unused(iAttr)
//		
//		return LitterGetAttrFloat(DoExpect(me, expEntropy), ioArgC, ioArgV);
//		}
	
	static inline void
	AddInfo(void)
		{
		Object*	attr;
		Symbol*	symFloat64		= gensym("float64");
		Symbol*	symLong			= gensym("long");
		
		// Read-Write Attributes
//		attr = attr_offset_new("p", symFloat64, 0, NULL, NULL, calcoffset(objPoisson, prob));
//		class_addattr(gObjectClass, attr);
//		attr = attr_offset_new("n", symLong, 0, NULL, NULL, calcoffset(objPoisson, nTrials));
//		class_addattr(gObjectClass, attr);
		
		// Read-Only Attributes
		attr = attribute_new("min", symLong, kAttrFlagsReadOnly, (method) PfishieGetMin, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("max", symLong, kAttrFlagsReadOnly, (method) PfishieGetMax, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("mean", symFloat64, kAttrFlagsReadOnly, (method) PfishieGetMean, NULL);
		class_addattr(gObjectClass, attr);
//		attr = attribute_new("median", symFloat64, kAttrFlagsReadOnly, (method) PfishieGetMedian, NULL);
//		class_addattr(gObjectClass, attr);
		attr = attribute_new("mode", symFloat64, kAttrFlagsReadOnly, (method) PfishieGetMode, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("var", symFloat64, kAttrFlagsReadOnly, (method) PfishieGetVar, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("stddev", symFloat64, kAttrFlagsReadOnly, (method) PfishieGetStdDev, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("skew", symFloat64, kAttrFlagsReadOnly, (method) PfishieGetSkew, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("kurtosis", symFloat64, kAttrFlagsReadOnly, (method) PfishieGetKurtosis, NULL);
		class_addattr(gObjectClass, attr);
//		attr = attribute_new("entropy", symFloat64, kAttrFlagsReadOnly, (method) PfishieGetEntropy, NULL);
//		class_addattr(gObjectClass, attr);
		}

static void
PfishieTell(
	objPoisson*	me,
	Symbol*		iTarget,
	Symbol*		iAttrName)
	
	{
	long	argC = 0;
	Atom*	argV = NIL;
		
	if (object_attr_getvalueof(me, iAttrName, &argC, &argV) == MAX_ERR_NONE) {
		ForwardAnything(iTarget, iAttrName, argC, argV);
		freebytes(argV, argC * sizeof(Atom));	// ASSERT (argC > 0 && argV != NIL)
		}
	}

#else

static void PfishieInfo(objPoisson* me)
	{ LitterInfo(kClassName, &me->coreObject, (method) PfishieTattle); }

static inline void AddInfo(void)
	{ LitterAddCant((method) PfishieInfo, "info"); }
		
static void PfishieTell(objPoisson* me, Symbol* iTarget, Symbol* iMsg)
	{ LitterExpect((tExpectFunc) DoExpect, (Object*) me, iMsg, iTarget, TRUE); }

#endif

#pragma mark -

/******************************************************************************************
 *
 *	main()
 *	
 *	Standar Max External Object Entry Point Function
 *	
 ******************************************************************************************/

void
main(void)
	
	{
	const tTypeList myArgTypes = {
						A_DEFFLOAT,		// Optional arguments:	1. lamda
						A_DEFLONG,		// 						2. seed
										// If no seed specified, use global Taus88 data
						A_NOTHING
						};
	
	LITTER_CHECKTIMEOUT(kClassName);
	
	LitterSetupClass(	kClassName,
						sizeof(objPoisson),				// Class object size
						LitterCalcOffset(objPoisson),	// Magic "Obex" Calculation
						(method) PfishieNew,		// Instance creation function
						(method) PfishieFree,		// Custom deallocation function
						NIL,						// No menu function
						myArgTypes);				// See above
	
	// Messages
	LITTER_TIMEBOMB LitterAddBang	((method) PfishieBang);
	LitterAddMess1	((method) PfishieLambda,	"ft1",	A_FLOAT);
	LitterAddMess1	((method) PfishieSeed,		"seed",	A_DEFLONG);
	LitterAddMess2	((method) PfishieTell,		"tell",	A_SYM, A_SYM);
	LitterAddMess0	((method) PfishieTattle,	"tattle");
	LitterAddCant	((method) PfishieTattle,	"dblclick");
	LitterAddCant	((method) PfishieAssist,	"assist");
	
	AddInfo();
	
	//Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();
	
	}
