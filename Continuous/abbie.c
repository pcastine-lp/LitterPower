/*
	File:		abbie.c

	Contains:	Max external object generating a random values with Beta distribution.

	Written by:	Peter Castine

	Copyright:	© 2001-2002 Peter Castine

	Change History (most recent first):

         <8>   23Ð3Ð2006    pc      Update for new LitterLib structure. Improve RNG algorithm
         <7>     18Ð2Ð06    pc      Add support for expect message.
         <6>     15Ð1Ð04    pc      Avoid possible memory leak if seed argument is used.
         <5>     11Ð1Ð04    pc      Update for Windows.
         <4>    7Ð7Ð2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <3>  30Ð12Ð2002    pc      Drop faux 'Vers' resource and related modifications. Add object
                                    version to DoInfo().
         <2>  28Ð11Ð2002    pc      Tidy up after initial check-in.
         <1>  28Ð11Ð2002    pc      Initial check in.
		30-Jun-2001:	Modified to use Taus88 instead of TT800 as default generator.
						Also update to use new ULong2Unitxx() functions.
		13-Apr-2001:	Merge in arc sine functionality (case of a = b = 0.5). Rename
						accordingly
		4-Apr-2001:		First implementation.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark ¥ Include Files

#include "LitterLib.h"
#include "RNGDistBeta.h"			// this #includes Taus88.h, MiscUtils.h, and math.h
#include "TrialPeriodUtils.h"


#pragma mark ¥ Constants

const char	kClassName[]		= "lp.abbie";			// Class name


	// Indices for STR# resource
enum {
	strIndexInBang		= lpStrIndexLastStandard + 1,
	strIndexInA,
	strIndexInB,
	
	strIndexTheOutlet
	};


#pragma mark ¥ Type Definitions

typedef union betaParams {
			tJKParams	jk;
			tBBParams	bb;
			tBCParams	bc;
			} uBetaParams;


#pragma mark ¥ Object Structure

typedef struct {
	LITTER_CORE_OBJECT(Object, coreObject);
	
	tTaus88DataPtr	tausData;
	
	double			alpha,
					beta;
					
	eBetaAlg		whichAlg;
	uBetaParams		params;
	} objBeta;


#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark ¥ Object Message Handlers

/******************************************************************************************
 *
 *	AbbieBang(me)
 *
 ******************************************************************************************/


static void
AbbieBang(
	objBeta* me)
	
	{
	double		randVal;
	eBetaAlg	whichAlg = me->whichAlg;
	
	if (whichAlg == algUndef) {
		double	alpha	= me->alpha,
				beta	= me->beta;
		
		whichAlg = me->whichAlg = RecommendBetaAlg(alpha, beta);
		
		switch (whichAlg) {
		case algJoehnk:
			CalcJKParams(&me->params.jk, alpha, beta);
			break;
		case algChengBB:
			CalcBBParams(&me->params.bb, alpha, beta);
			break;
		case algChengBC:
			CalcBCParams(&me->params.bc, alpha, beta);
			break;
		default:
			// No special parameter calculations for other generators
			break;
			}
		
		}
	
	// ASSERT: whichAlg != algUndef
	switch (whichAlg) {
		case algIndeterm:
			randVal = 0.0;
			if (((long) Taus88(me->tausData)) < 0) break;
			// otherwise, fall into next case
		case algConstOne:
			randVal = 1.0;
			break;
		
		case algConstZero:
			randVal = 0.0;
			break;
		
		case algUniform:
			// a == b == 1: Uniform distribution
			randVal = ULong2Unit_ZO( Taus88(me->tausData) );
			break;
		
		case algArcSine:
			// a == b == 0.5: Arc Sine distribution
			randVal = GenArcsineTaus88(me->tausData);
			break;
		
		case algJoehnk:
			randVal = GenBetaJKTaus88(&me->params.jk, me->tausData);
			break;
		
		case algChengBB:
			randVal = GenBetaBBTaus88(&me->params.bb, me->tausData);
			break;
		
		case algChengBC:
			randVal = GenBetaBCTaus88(&me->params.bc, me->tausData);
			break;
		
		default:
			// This can never happen
		#if defined(__DEBUG__) && __DEBUG__
			error("%s: unknown algorigthm %ld", kClassName, (long) whichAlg);
		#endif
			return;						// Exit function early rather, 
			break;
		}
		
	outlet_float(me->coreObject.o_outlet, randVal);
	
	}


/******************************************************************************************
 *
 *	AbbieMu(me, iMu)
 *	AbbieSigma(me, iSigma)
 *	
 *	Set parameters. Make sure nothing bad is happening.
 *	
 ******************************************************************************************/

static void
AbbieAlpha(
	objBeta*	me,
	double		iAlpha)
	
	{
	if (iAlpha < 0.0)
		iAlpha = 0.0;
	
	me->alpha		= iAlpha;
	me->whichAlg	= algUndef;
	}

static void
AbbieBeta(
	objBeta*	me,
	double		iBeta)
	
	{
	if (iBeta < 0.0)
		iBeta = 0.0;
	
	me->beta		= iBeta;
	me->whichAlg	= algUndef;
	}
	

/******************************************************************************************
 *
 *	AbbieSeed(me, iSeed)
 *
 ******************************************************************************************/

static void AbbieSeed(objBeta* me, long iSeed)	
	{ Taus88Seed(me->tausData, (unsigned long) iSeed); }


#pragma mark -
#pragma mark ¥ Attribute/Information Functions

/******************************************************************************************
 *
 *	AbbieAssist(me, iBox, iDir, iArgNum, oCStr)
 *	AbbieTattle(me)
 *
 ******************************************************************************************/

static void AbbieAssist(objBeta* me, void* box, long iDir, long iArgNum, char* oCStr)
	{
	#pragma unused(me, box)
	
	LitterAssist(iDir, iArgNum, strIndexInBang, strIndexTheOutlet, oCStr);
	}

static void
AbbieTattle(
	objBeta* me)
	
	{
	
	post("%s state",
			kClassName);
	post("  parameters (%lf, %lf)", me->alpha, me->beta);
	
	switch (me->whichAlg) {
	case algConstZero:
		post("  --> Distribution is always 0");
		break;
	case algConstOne:
		post("  --> Distribution is always 1");
		break;
	case algIndeterm:
		post("  --> Distribution is indeterminate (but either 0 or 1)");
		break;
	case algUniform:
		post("  --> Distribution degenerates to uniform distribution");
		break;
	case algArcSine:
		post("  --> Special case: Arc Sine distribution");
		break;
	case algJoehnk:
		post("  --> Using Jšhnk algorithm");
		break;
	case algChengBB:
		post("  --> Using Cheng BB algorithm");
		break;
	case algChengBC:
		post("  --> Using Cheng BC algorithm");
		break;
	default:
		// Algorithm choice must be pending
		post("  algorithm not yet determined");
		break;
		}
	
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
	objBeta*		me,
	eExpectSelector iSel)
	{
	double	result	= 0.0 / 0.0,					// Initially undefined
			alpha	= me->alpha,
			beta	= me->beta,
			x;										// Multi-purpose register
	
	switch(iSel) {
	case expMin:
		result = 0.0;
		break;
	case expMax:
		result = 1.0;
		break;
	case expMean:
		result = (alpha == beta)					// Special case this to get correct
					? 0.5							// results for alpha = beta = 0
					: alpha / (alpha + beta);
		break;
	case expMedian:
		// Undefined
		break;
	case expMode:
		if (alpha >= 1.0 && beta >= 1.0) {
			// Also need to make sure that at least one of the two parameters is
			// strictly larger than one. The following test serves that purpose while
			// simultaneously calculating an intermediary result we need.
			double ab2 = alpha + beta - 2.0;
			if (ab2 > 0.0) result = (alpha - 1.0) / ab2;
			}
		// ...otherwise the mode remains undefined
		break;
	case expVar:
	case expStdDev:
		x = alpha + beta;
		result  = 1.0 / x;
		result *= result;
		result *= alpha * beta;
		result /= (x + 1.0);
		if (iSel == expStdDev)
			result = sqrt(result);
		break;
	case expSkew:
		x = alpha + beta;
		result  = beta - alpha;
		result *= sqrt(x + 1.0);
		result += result;
		result /= (x + 2.0);
		break;
	case expKurtosis:
		x = alpha + beta;
		result  = alpha * alpha * alpha;
		result -= alpha * alpha * (beta + beta - 1.0);
		result += beta * beta * (beta + 1.0);
		result -= 2.0 * alpha * beta * (beta + 2.0);
		result /= x * (x + 2.0) * (x + 3.0);
		result *= 6.0;
		break;
	case expEntropy:
		// Undefined
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

	static t_max_err AbbieGetMin(objBeta* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMin), ioArgC, ioArgV); }
	static t_max_err AbbieGetMax(objBeta* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMax), ioArgC, ioArgV); }
	static t_max_err AbbieGetMean(objBeta* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMean), ioArgC, ioArgV); }
//	static t_max_err AbbieGetMedian(objBeta* me, void* iAttr, long* ioArgC, Atom** ioArgV)
//		{ return LitterGetAttrFloat(DoExpect(me, expMedian), ioArgC, ioArgV); }
	static t_max_err AbbieGetMode(objBeta* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMode), ioArgC, ioArgV); }
	static t_max_err AbbieGetVar(objBeta* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expVar), ioArgC, ioArgV); }
	static t_max_err AbbieGetStdDev(objBeta* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expStdDev), ioArgC, ioArgV); }
	static t_max_err AbbieGetSkew(objBeta* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expSkew), ioArgC, ioArgV); }
	static t_max_err AbbieGetKurtosis(objBeta* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expKurtosis), ioArgC, ioArgV); }
//	static t_max_err AbbieGetEntropy(objBeta* me, void* iAttr, long* ioArgC, Atom** ioArgV)
//		{ return LitterGetAttrFloat(DoExpect(me, expEntropy), ioArgC, ioArgV); }
	
	static t_max_err AbbieSetAttrAlpha(objBeta* me, void* iAttr, long iArgC, Atom iArgV[])
		{
		
		if (iArgC > 0 && iArgV != NIL)
			AbbieAlpha(me, AtomGetFloat(iArgV));
		
		return MAX_ERR_NONE;
		}
	
	static t_max_err AbbieSetAttrBeta(objBeta* me, void* iAttr, long iArgC, Atom iArgV[])
		{
		
		if (iArgC > 0 && iArgV != NIL)
			AbbieBeta(me, AtomGetFloat(iArgV));
		
		return MAX_ERR_NONE;
		}
	
	static inline void
	AddInfo(void)
		{
		Object*	attr;
		Symbol*	symFloat64		= gensym("float64");
		
		// Read-Write Attributes
		attr = attr_offset_new(	"alpha", symFloat64, 0,
								NULL, (method) AbbieSetAttrAlpha,
								calcoffset(objBeta, alpha));
		class_addattr(gObjectClass, attr);
		attr = attr_offset_new(	"beta", symFloat64, 0,
								NULL, (method) AbbieSetAttrBeta,
								calcoffset(objBeta, beta));
		class_addattr(gObjectClass, attr);
		
		// Read-Only Attributes
		attr = attribute_new("min", symFloat64, kAttrFlagsReadOnly, (method) AbbieGetMin, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("max", symFloat64, kAttrFlagsReadOnly, (method) AbbieGetMax, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("mean", symFloat64, kAttrFlagsReadOnly, (method) AbbieGetMean, NULL);
		class_addattr(gObjectClass, attr);
//		attr = attribute_new("median", symFloat64, kAttrFlagsReadOnly, (method) AbbieGetMedian, NULL);
//		class_addattr(gObjectClass, attr);
		attr = attribute_new("mode", symFloat64, kAttrFlagsReadOnly, (method) AbbieGetMode, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("var", symFloat64, kAttrFlagsReadOnly, (method) AbbieGetVar, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("stddev", symFloat64, kAttrFlagsReadOnly, (method) AbbieGetStdDev, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("skew", symFloat64, kAttrFlagsReadOnly, (method) AbbieGetSkew, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("kurtosis", symFloat64, kAttrFlagsReadOnly, (method) AbbieGetKurtosis, NULL);
		class_addattr(gObjectClass, attr);
//		attr = attribute_new("entropy", symFloat64, kAttrFlagsReadOnly, (method) AbbieGetEntropy, NULL);
//		class_addattr(gObjectClass, attr);
		}

static void
AbbieTell(
	objBeta*	me,
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

static void AbbieInfo(objBeta* me)
	{ LitterInfo(kClassName, &me->coreObject, (method) AbbieTattle); }

static inline void AddInfo(void)
	{ LitterAddCant((method) AbbieInfo, "info"); }
		
static void AbbieTell(objBeta* me, Symbol* iTarget, Symbol* iAttrName)
	{ LitterExpect((tExpectFunc) DoExpect, (Object*) me, iAttrName, iTarget, TRUE); }

#endif




#pragma mark -
#pragma mark ¥ Class Message Handlers

/******************************************************************************************
 *
 *	AbbieNew(iAlpha, iBeta, iSeed)
 *	AbbieFree(me)
 *
 ******************************************************************************************/

static void*
AbbieNew(
	double	iAlpha,
	double	iBeta,
	long	iSeed)
	
	{
	const double	kDefParam	= 0.5;					// The same default for both a and b
	
	objBeta*		me			= NIL;
	tTaus88DataPtr	myTausStuff	= NIL;
	
	// Run through initialization parameters from right to left, handling defaults
	if (iSeed != 0) {
		myTausStuff = Taus88New(iSeed);
		goto noMoreDefaults;
		}
	
	if (iBeta == 0.0)
		iBeta = kDefParam;
	else goto noMoreDefaults;
	
	if (iAlpha == 0.0)
		iAlpha = kDefParam;
noMoreDefaults:
	// Finished checking intialization parameters
	// ----------------------------------------------------

	// Let Max allocate us, our inlets, and outlets
	me = (objBeta*) LitterAllocateObject();
	
	floatin(me, 2);									// parameter b inlet
	floatin(me, 1);									// parameter a inlet
	
	floatout(me);									// Access through me->coreObject.o_outlet
	
	// Store object components
	me->tausData	= myTausStuff;
	me->whichAlg	= algUndef;
	AbbieAlpha(me, iAlpha);
	AbbieBeta(me, iBeta);
	
	return me;
	}

static void AbbieFree(objBeta* me)
	{ Taus88Free(me->tausData); }							// Taus88Free is NIL-safe


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
						A_DEFFLOAT,					// Optional arguments:	1: a
						A_DEFFLOAT,					//						2: b
						A_DEFLONG,					// 						3: seed
						A_NOTHING
						};
	
	LITTER_CHECKTIMEOUT(kClassName);
	
	// Standard Max setup() call
	LitterSetupClass(	kClassName,
						sizeof(objBeta),			// Class object size
						LitterCalcOffset(objBeta),
						(method) AbbieNew,			// Instance creation function
						(method) AbbieFree,			// Custom deallocation function
						NIL,						// No menu function
						myArgTypes);				
	

	// Messages
	LITTER_TIMEBOMB LitterAddBang	((method) AbbieBang);
	LitterAddMess1	((method) AbbieAlpha,	"ft1",	A_FLOAT);
	LitterAddMess1	((method) AbbieBeta,	"ft2",	A_FLOAT);
	LitterAddMess1	((method) AbbieSeed,	"seed",	A_DEFLONG);
	LitterAddMess2	((method) AbbieTell,	"tell", A_SYM, A_SYM);
	LitterAddMess0	((method) AbbieTattle,	"tattle");
	LitterAddCant	((method) AbbieTattle,	"dblclick");
	LitterAddCant	((method) AbbieAssist,	"assist");
	
	AddInfo();
	
	// Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();
	
	}



