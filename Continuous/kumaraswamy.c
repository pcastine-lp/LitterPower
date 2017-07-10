/*
	File:		kumaraswamy.c

	Contains:	Max external object generating a Kumaraswamy distribution.

	Written by:	Peter Castine

	Copyright:	© 2001-2002 Peter Castine

	Change History (most recent first):

         <2>      4–3–06    pc      Improve mode calculation at extreme values.
         <1>     18–2–06    pc      first checked in.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "LitterLib.h"
#include "TrialPeriodUtils.h"
#include "Taus88.h"
#include "MiscUtils.h"
#include "UniformExpectations.h"

#include <math.h>

#ifdef WIN_VERSION
	#include "MoreMath.h"							// MSL on Windows doesn't have lgamma()
#endif

#pragma mark • Constants

const char	kClassName[]	= "lp.swamy";			// Class name


	// Indices for STR# resource
enum {
	strIndexInBang		= lpStrIndexLastStandard + 1,
	strIndexInAlpha,
	strIndexInBeta,
	
	strIndexTheOutlet,
	
	strIndexLeftInlet	= strIndexInBang,
	strIndexLeftOutlet	= strIndexTheOutlet
	};

	// Constants to keep track of special cases
enum SpecialCase {
	scGeneral			= 0,
	
	scUniform,						// a == b == 1
	
	scAlways0,						// a == 0 && b > 0
	scAlways1,						// a > 0 && b == 0
	scIndeterminate					// a == b == 0
	};


#pragma mark • Type Definitions

typedef enum SpecialCase eSpecialCase;


#pragma mark • Object Structure

typedef struct {
	LITTER_CORE_OBJECT(Object, coreObject);
	
	tTaus88DataPtr	tausData;
	
	double			alpha,
					beta,
					alpha1,				// Cache 1/alpha and 1/beta in these members
					beta1;
	
	eSpecialCase	exception;
	} objSwamy;



#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	SwamyBang(me)
 *
 ******************************************************************************************/


static void
SwamyBang(
	objSwamy* me)
	
	{
	double	k,
			a1 = me->alpha1,			// Per convention, if alpha/beta were set to zero
			b1 = me->beta1;				// then alpha1 and beta1 are also zeroed
	
	if (a1 == 0.0)
		k = (b1 == 0.0)
			? (double) (Taus88(me->tausData) & 0x01)
			: 1.0;
	
	else if (b1 == 0.0)
		k = 0.0;
	
	else {
		k = ULong2Unit_ZO(Taus88(me->tausData));
		
		k = pow(1.0 - pow(k, b1), a1);
		}
	
	outlet_float(me->coreObject.o_outlet, k);
	}


/******************************************************************************************
 *
 *	SwamyAlpha(me, iAlpha)
 *	SwamyBeta(me, iBeta)
 *	
 *	Set parameters, making sure nothing bad happens.
 *	
 ******************************************************************************************/

	static eSpecialCase CheckSpecialCase(double alpha, double beta)
		{
		eSpecialCase result;
		
		if (alpha == 0.0)
			result = (beta == 0.0) ? scIndeterminate : scAlways0;
		
		else if (beta == 0.0)
			result = scAlways1;
		
		else if (alpha == 1.0 && beta == 1.0)
			result = scUniform;
		
		else result = scGeneral;
		
		return result;
		}

static void 
SwamyAlpha(
	objSwamy*	me,
	double		iAlpha)
	
	{
	
	if (iAlpha > 0.0) {
		me->alpha	= iAlpha;
		me->alpha1	= 1.0 / iAlpha;
		}
	else me->alpha1 = me->alpha = iAlpha = 0.0;
	
	me->exception = CheckSpecialCase(iAlpha, me->beta);
	
	}

static void 
SwamyBeta(
	objSwamy*	me,
	double		iBeta)
	
	{
	
	if (iBeta > 0.0) {
		me->beta	= iBeta;
		me->beta1	= 1.0 / iBeta;
		}
	else me->beta1 = me->beta = iBeta = 0.0;
	
	me->exception = CheckSpecialCase(me->alpha, iBeta);
	
	}

/******************************************************************************************
 *
 *	SwamySeed(me, iSeed)
 *
 ******************************************************************************************/

static void SwamySeed(objSwamy* me,long iSeed)
	{ Taus88Seed(me->tausData, (unsigned long) iSeed); }

#pragma mark -
#pragma mark • Attribute/Information Functions

/******************************************************************************************
 *
 *	SwamyAssist(me, iBox, iDir, iArgNum, oCStr)
 *	SwamyTattle(me)
 *
 ******************************************************************************************/

static void SwamyAssist(objSwamy* me, void* box, long iDir, long iArgNum, char* oCStr)
	{
	#pragma unused(me, box)
	
	LitterAssist(iDir, iArgNum, strIndexInBang, strIndexTheOutlet, oCStr);
	}

static void
SwamyTattle(
	objSwamy* me)
	
	{
	
	post("%s state", kClassName);
	post("  alpha = %lf (1/alpha = %lf)", me->alpha, me->alpha1);
	post("  beta = %lf (1/beta = %lf)", me->beta, me->beta1);
	
	if (me->alpha == 0.0 || me->beta == 0.0)
		post("  warning: zero parameters are bad karma");
	
	}


/******************************************************************************************
 *
 *	DoExpect()
 *
 *	Helper function for calculating expected values (mean, standard deviation, etc.)
 *	
 ******************************************************************************************/

static double
DoExpect(objSwamy* me, eExpectSelector iSel)
	{
	double	result	= 0.0 / 0.0,		// Initially undefined
			a1	= me->alpha1,			// Per convention, if alpha/beta were set to
			b1	= me->beta1;			// zero then alpha1 and beta1 are also zeroed
	
	switch (me->exception) {
	default:							// General situation, calculate everything the
		switch (iSel) {					// hard way
		case expMean:
			{
			double b = me->beta;
			
			a1 += 1.0;					// We need (1 + 1/alpha), which is what we now
										// have in this register
			result  = lgamma(b);
			result += lgamma(a1);
			result -= lgamma(a1 + b);
			result  = b * exp(result);
			}
			break;
		case expMedian:
			result = pow(1.0 - pow(0.5, b1), a1);
			break;
		case expMode:
			{
			double	a  = me->alpha,
					ab = a * me->beta;
			
			a  -= 1.0;
			ab -= 1.0;
			
			if (a >= 0.0 && ab > 0.0) {	// Implicit requirement for formula
				result = pow((a) / (ab), a1);
				
				// Mode calculation can go out of bounds
				if		(result < 0.0) result = 0.0;
				else if (result > 1.0) result = 1.0;
				}
			}
			break;
		case expMin:
			result = 1.0;
			break;
		case expMax:
			result = 0.0;
			break;
		default:
			// Everything else is undefined
			break;
			}
		break;							// End of "true" Kumaraswamy dist.
	
	case scUniform:
		result = UniformExpectationsContinuous(0.0, 1.0, iSel);
		break;							// End of uniform dist.
		
	case scAlways0:
		switch (iSel) {
		case expMean:
		case expMedian:
		case expMode:
		case expMin:
		case expMax:
		case expEntropy:
			result = 0.0;
			break;
		default:
			// Everything else is undefined
			break;
			}
		break;						// End of one degenerate dist.
	
	case scAlways1:
		switch (iSel) {
		case expMean:
		case expMedian:
		case expMode:
		case expMin:
		case expMax:
			result = 1.0;
			break;
		case expEntropy:
			result = 0.0;
			break;
		default:
			// Everything else is undefined
			break;
			}
		break;						// End of another degenerate dist.
	
	case scIndeterminate:
		switch (iSel) {
		case expMean:
			result = 0.5;
			break;
		case expMin:
			result = 0.0;
			break;
		case expMax:
		case expEntropy:
			result = 1.0;
			break;	
		default:
			// Everything else is undefined
			break;
			}
		break;						// End of the really degenerate dist.
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

	static t_max_err SwamyGetMin(objSwamy* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMin), ioArgC, ioArgV); }
	static t_max_err SwamyGetMax(objSwamy* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMax), ioArgC, ioArgV); }
	static t_max_err SwamyGetMean(objSwamy* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMean), ioArgC, ioArgV); }
	static t_max_err SwamyGetMedian(objSwamy* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMedian), ioArgC, ioArgV); }
	static t_max_err SwamyGetMode(objSwamy* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMode), ioArgC, ioArgV); }
	static t_max_err SwamyGetVar(objSwamy* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expVar), ioArgC, ioArgV); }
	static t_max_err SwamyGetStdDev(objSwamy* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expStdDev), ioArgC, ioArgV); }
	static t_max_err SwamyGetSkew(objSwamy* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expSkew), ioArgC, ioArgV); }
	static t_max_err SwamyGetKurtosis(objSwamy* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expKurtosis), ioArgC, ioArgV); }
	static t_max_err SwamyGetEntropy(objSwamy* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expEntropy), ioArgC, ioArgV); }
	
	static t_max_err SwamySetAttrAlpha(objSwamy* me, void* iAttr, long iArgC, Atom iArgV[])
		{
		
		if (iArgC > 0 && iArgV != NIL)
			SwamyAlpha(me, AtomGetFloat(iArgV));
		
		return MAX_ERR_NONE;
		}
	
	static t_max_err SwamySetAttrBeta(objSwamy* me, void* iAttr, long iArgC, Atom iArgV[])
		{
		
		if (iArgC > 0 && iArgV != NIL)
			SwamyBeta(me, AtomGetFloat(iArgV));
		
		return MAX_ERR_NONE;
		}
	
	static inline void
	AddInfo(void)
		{
		Object*	attr;
		Symbol*	symFloat64		= gensym("float64");
		
		// Read-Write Attributes
		attr = attr_offset_new(	"alpha", symFloat64, 0,
								NULL, (method) SwamySetAttrAlpha,
								calcoffset(objSwamy, alpha));
		class_addattr(gObjectClass, attr);
		attr = attr_offset_new(	"beta", symFloat64, 0,
								NULL, (method) SwamySetAttrBeta,
								calcoffset(objSwamy, beta));
		class_addattr(gObjectClass, attr);
		
		// Read-Only Attributes
		attr = attribute_new("min", symFloat64, kAttrFlagsReadOnly, (method) SwamyGetMin, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("max", symFloat64, kAttrFlagsReadOnly, (method) SwamyGetMax, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("mean", symFloat64, kAttrFlagsReadOnly, (method) SwamyGetMean, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("median", symFloat64, kAttrFlagsReadOnly, (method) SwamyGetMedian, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("mode", symFloat64, kAttrFlagsReadOnly, (method) SwamyGetMode, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("var", symFloat64, kAttrFlagsReadOnly, (method) SwamyGetVar, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("stddev", symFloat64, kAttrFlagsReadOnly, (method) SwamyGetStdDev, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("skew", symFloat64, kAttrFlagsReadOnly, (method) SwamyGetSkew, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("kurtosis", symFloat64, kAttrFlagsReadOnly, (method) SwamyGetKurtosis, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("entropy", symFloat64, kAttrFlagsReadOnly, (method) SwamyGetEntropy, NULL);
		class_addattr(gObjectClass, attr);
		}

static void
SwamyTell(
	objSwamy*	me,
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

static void SwamyInfo(objSwamy* me)
	{ LitterInfo(kClassName, &me->coreObject, (method) SwamyTattle); }

static inline void AddInfo(void)
	{ LitterAddCant((method) SwamyInfo, "info"); }
		
static void SwamyTell(objSwamy* me, Symbol* iTarget, Symbol* iAttrName)
	{ LitterExpect((tExpectFunc) DoExpect, (Object*) me, iAttrName, iTarget, TRUE); }

#endif


#pragma mark -
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	SwamyNew(iOracle)
 *
 ******************************************************************************************/

static void
SwamyFree(objSwamy* me)
	{ Taus88Free(me->tausData); }						// Taus88Free() is NIL-pointer safe

static void*
SwamyNew(
	double	iAlpha,
	double	iBeta,
	long	iSeed)
	
	{
	const double	kDefParam	= 0.5;					// The same default for both a and b
	
	objSwamy*	me;
	
	// Let Max allocate us, our inlets (from right to left), and outlets
	me = (objSwamy*) LitterAllocateObject();
	
	floatin(me, 2);
	floatin(me, 1);
	
	floatout(me);								// Access through me->coreObject.o_outlet
	
	// Initialize object components
	me->tausData	= NIL;
	me->alpha		= kDefParam;
	me->alpha1		= 1.0 / kDefParam;
	me->beta		= kDefParam;
	me->beta1		= 1.0 / kDefParam;
	me->exception	= scGeneral;				// Default parameters avoid exceptional 
												// situations
	
	// Parse initialization parameters from right to left, overriding defaults as needed
	if (iSeed != 0)
		me->tausData = Taus88New(iSeed);
	
	if (iBeta != 0.0)
		SwamyBeta(me, iBeta);
	
	if (iAlpha != 0.0)
		SwamyAlpha(me, iAlpha);
	
	return me;
	}


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
						A_DEFFLOAT,					// Optional arguments:	1: Alpha
						A_DEFFLOAT,					//						2: Beta
						A_DEFLONG,					//						3: Seed
						A_NOTHING
						};
	
	// Standard Max setup() call
	LitterSetupClass(	kClassName,
						sizeof(objSwamy),
						LitterCalcOffset(objSwamy),			// Class object size
						(method) SwamyNew,			// Instance creation function
						(method) SwamyFree,			// Custom deallocation function
						NIL,						// No menu function
						myArgTypes);				

	// Messages
	LITTER_TIMEBOMB LitterAddBang((method) SwamyBang);
	LitterAddMess1	((method) SwamyAlpha,	"ft1",	A_FLOAT);
	LitterAddMess1	((method) SwamyBeta,	"ft2",	A_FLOAT);
	LitterAddMess1	((method) SwamySeed,	"seed",	A_DEFLONG);
	LitterAddMess2	((method) SwamyTell,	"tell", A_SYM, A_SYM);
	LitterAddCant	((method) SwamyTattle,	"dblclick");
	LitterAddCant	((method) SwamyAssist,	"assist");
	
	AddInfo();
	
	// Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();
	
	}



