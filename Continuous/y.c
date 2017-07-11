/*
	File:		y.c

	Contains:	Max external object generating a random values with Weibull distribution.

	Written by:	Peter Castine

	Copyright:	© 2001-2002 Peter Castine

	Change History (most recent first):

         <7>   23–3–2006    pc      Add support for expect message.
         <6>     15–1–04    pc      Avoid possible memory leak if seed argument is used.
         <5>     11–1–04    pc      Update for Windows.
         <4>    7–7–2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <3>  30–12–2002    pc      Drop faux 'Vers' resource and related modifications. Add object
                                    version to YInfo().
         <2>  28–11–2002    pc      Tidy up after initial check-in.
         <1>  28–11–2002    pc      Initial check in.
		30-Jun-2001:	Modified to use Taus88 instead of TT800 as default generator.
						Also update to use new ULong2Unitxx() functions.
		14-Apr-2001:	Overhaul for LPP
		4-Apr-2001:		First implementation.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "LitterLib.h"
#include "TrialPeriodUtils.h"
#include "Taus88.h"
#include "MiscUtils.h"
#include "MoreMath.h"

#include <math.h>


#pragma mark • Constants

const char	kClassName[]	= "lp.y";				// Class name



	// Indices for STR# resource
enum {
	strIndexInBang		= lpStrIndexLastStandard + 1,
	strIndexInS,
	strIndexInT,
	
	strIndexTheOutlet,
	
	strIndexLeftInlet	= strIndexInBang,
	strIndexLeftOutlet	= strIndexTheOutlet
	};


#pragma mark • Type Definitions



#pragma mark • Object Structure

typedef struct {
	LITTER_CORE_OBJECT(Object, coreObject);
	
	tTaus88DataPtr	tausData;
	
	double			scale,
					oneOverT;		
	} objWeibull;



#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	YBang(me)
 *
 ******************************************************************************************/

static void
YBang(
	objWeibull* me)
	
	{
	double	weibull = me->scale,				// Initialize calculation
			c		= me->oneOverT;
	
	if (weibull > 0.0 && c > 0.0) {
		double u = ULong2Unit_zO( Taus88(me->tausData) );
		
		weibull *= pow(log(1.0 / u), c);
		}
		
	outlet_float(me->coreObject.o_outlet, weibull);
	
	}


/******************************************************************************************
 *
 *	YScale(me, iScale)
 *	YCurve(me, iCurve)
 *	YSeed(me, iSeed)
 *	
 *	Set parameters, making sure nothing bad happens.
 *	
 *	NB: We allow the user to set scale to 0 here (as opposed to initialization arguments,
 *		where 0 scale defaults to 1). Otherwise both parameters must be positive
 *
 ******************************************************************************************/

static void YScale(objWeibull* me, double iVal)
	{ me->scale = (iVal >= 0.0) ? iVal : 0.0; }

static void YCurve(objWeibull* me, double iVal)
	{ me->oneOverT = (iVal >  0.0) ? 1.0 / iVal : 0.0; }
	
static void YSeed( objWeibull* me, long iSeed)
	{ Taus88Seed(me->tausData, (unsigned long) iSeed); }


#pragma mark -
#pragma mark • Attribute/Information Functions

/******************************************************************************************
 *
 *	YAssist()
 *	YTattle(me)
 *
 ******************************************************************************************/

static void YAssist(objWeibull* me, void* box, long iDir, long iArgNum, char* oCStr)
	{
	#pragma unused(me, box)
	
	LitterAssist(iDir, iArgNum, strIndexLeftInlet, strIndexLeftOutlet, oCStr);
	}

static void
YTattle(
	objWeibull* me)
	
	{
	double	oneOverT = me->oneOverT;
	
	post("%s state",
			kClassName);
	post("  s = %lf", me->scale);
	post("  1/t = %lf (i.e., t = %lf)",
			oneOverT,
			(oneOverT > 0.0) ? (1.0 / oneOverT) : 0.0);
	if (oneOverT == 0.0) {
		post("   -> Invalid value for t");
		}
	
	}


/******************************************************************************************
 *
 *	DoExpect(me, iSelector)
 *
 ******************************************************************************************/
	
static double 
DoExpect(
	objWeibull*		me,
	eExpectSelector	iSel)
	
	{
	const double	kLn2 = 0.6931471806,
					kEulerGamma	= 0.5772156649;
	
	double	result	= 0.0 / 0.0,			// Initially undefined
			scale	= me->scale,
			shape	= me->oneOverT;			// For our purposes, "shape" is the mult.
											// inverse of the curve parameter. This
											// is convenient for most calculations.			
	
	if (shape > 0.0) switch(iSel) {
	case expMean:
		result = scale * gamma(shape + 1.0);
		break;
	case expMedian:
		result = scale * pow(kLn2, shape);
		break;
	case expVar:
	case expStdDev:
		// ?? This is a bit brute force, intermediary results can overflow when the
		//		end result would not, particularly for standard deviation
		//		However, overflow conditions are be pretty rare (shape > 85 or so)
		result  = gamma(shape + 1.0);
		result *= -result;
		result += gamma(shape + shape + 1.0);
		result *= scale * scale;
		if (iSel == expStdDev)
			result = sqrt(result);
		break;
	case expSkew:
		// ?? More brute force
		{
		double	mean	= DoExpect(me, expMean),
				var		= DoExpect(me, expVar);
		
		result  = gamma(shape + shape + shape + 1.0);
		result *= scale * scale * scale;
		result -= 3.0 * mean * var;
		result -= mean * mean * mean;
		result /= var * sqrt(var);
		}
		break;
	case expKurtosis:
		// ?? Still brute force, but without the recursion used for skew above
		{
		double	gamma1 = gamma(shape + 1.0),
				gamma2 = gamma(shape + shape + 1.0),
				gamma3 = gamma(shape + shape + shape + 1.0);
		
			// Start off with what would be gamma4
		result  = gamma(4.0 * shape + 1.0);
		result -= 4.0 * gamma1 * gamma3;
		result -= 3.0 * gamma2 * gamma2;
		result += 12.0 * gamma1 * gamma1 * gamma2;
			// We now neeed (gamma1 ^ 2) and (gamma2 - gamma1^2)
		gamma1 *= gamma1;
		gamma2 -= gamma1;
			// Finish off calculation
		result -= 6.0 * gamma1 * gamma1;
		result /= gamma2 * gamma2;
		}
		break;
	case expMin:
		result = 0.0;
		break;
	case expMax:
		result = 1.0 / 0.0;
		break;
	case expEntropy:
		// Start off with a convenient little fib
		scale *= shape;
		
		result  = kEulerGamma * (1.0 - shape);
		result += pow(scale, 1.0 / shape);
		result += log(scale);
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

	static t_max_err YGetMin(objWeibull* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMin), ioArgC, ioArgV); }
	static t_max_err YGetMax(objWeibull* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMax), ioArgC, ioArgV); }
	static t_max_err YGetMean(objWeibull* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMean), ioArgC, ioArgV); }
	static t_max_err YGetMedian(objWeibull* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMedian), ioArgC, ioArgV); }
	static t_max_err YGetMode(objWeibull* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMode), ioArgC, ioArgV); }
	static t_max_err YGetVar(objWeibull* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expVar), ioArgC, ioArgV); }
	static t_max_err YGetStdDev(objWeibull* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expStdDev), ioArgC, ioArgV); }
	static t_max_err YGetSkew(objWeibull* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expSkew), ioArgC, ioArgV); }
	static t_max_err YGetKurtosis(objWeibull* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expKurtosis), ioArgC, ioArgV); }
	static t_max_err YGetEntropy(objWeibull* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expEntropy), ioArgC, ioArgV); }
	
	static t_max_err YSetAttrScale(objWeibull* me, void* iAttr, long iArgC, Atom iArgV[])
		{
		
		if (iArgC > 0 && iArgV != NIL)
			YScale(me, AtomGetFloat(iArgV));
		
		return MAX_ERR_NONE;
		}
	
	static t_max_err YSetAttrCurve(objWeibull* me, void* iAttr, long iArgC, Atom iArgV[])
		{
		
		if (iArgC > 0 && iArgV != NIL)
			YCurve(me, AtomGetFloat(iArgV));
		
		return MAX_ERR_NONE;
		}
	
	static t_max_err YGetAttrCurve(objWeibull* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{
		double curve = (me->oneOverT > 0.0) ? 1.0 / me->oneOverT : 0.0;
		
		return LitterGetAttrFloat(curve, ioArgC, ioArgV);
		}
	
	static inline void
	AddInfo(void)
		{
		Object*	attr;
		Symbol*	symFloat64		= gensym("float64");
		
		// Read-Write Attributes
		attr = attr_offset_new(	"scale", symFloat64, 0,
								NULL, (method) YSetAttrScale,
								calcoffset(objWeibull, scale));
		class_addattr(gObjectClass, attr);
		attr = attr_offset_new(	"shape", symFloat64, 0,
								(method) YGetAttrCurve, (method) YSetAttrCurve, 0);
		class_addattr(gObjectClass, attr);
		
		// Read-Only Attributes
		attr = attribute_new("min", symFloat64, kAttrFlagsReadOnly, (method) YGetMin, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("max", symFloat64, kAttrFlagsReadOnly, (method) YGetMax, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("mean", symFloat64, kAttrFlagsReadOnly, (method) YGetMean, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("median", symFloat64, kAttrFlagsReadOnly, (method) YGetMedian, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("mode", symFloat64, kAttrFlagsReadOnly, (method) YGetMode, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("var", symFloat64, kAttrFlagsReadOnly, (method) YGetVar, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("stddev", symFloat64, kAttrFlagsReadOnly, (method) YGetStdDev, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("skew", symFloat64, kAttrFlagsReadOnly, (method) YGetSkew, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("kurtosis", symFloat64, kAttrFlagsReadOnly, (method) YGetKurtosis, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("entropy", symFloat64, kAttrFlagsReadOnly, (method) YGetEntropy, NULL);
		class_addattr(gObjectClass, attr);
		}

static void
YTell(
	objWeibull*	me,
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

static void YInfo(objWeibull* me)
	{ LitterInfo(kClassName, &me->coreObject, (method) YTattle); }

static inline void AddInfo(void)
	{ LitterAddCant((method) YInfo, "info"); }
		
static void YTell(objWeibull* me, Symbol* iTarget, Symbol* iAttrName)
	{ LitterExpect((tExpectFunc) DoExpect, (Object*) me, iAttrName, iTarget, TRUE); }

#endif



#pragma mark -
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	YFree(me)
 *	YNew(iOracle)
 *
 ******************************************************************************************/

static void YFree(objWeibull* me)
	{ Taus88Free(me->tausData); }							// Taus88Free is NIL-safe

static void*
YNew(
	double	iScale,
	double	iCurve,
	long	iSeed)
	
	{
	const double	kDefParam		= 1.0;				// Same for both s and t
	
	objWeibull*		me			= NIL;
	tTaus88DataPtr	myTausStuff	= NIL;
	
	// Run through initialization parameters from right to left, handling defaults
	if (iSeed != 0) {
		myTausStuff = Taus88New(iSeed);
		goto noMoreDefaults;
		}
	
	if (iCurve == 0.0)
		iCurve = kDefParam;
	else goto noMoreDefaults;
	
	if (iScale == 0.0)
		iScale = kDefParam;
noMoreDefaults:
	// Finished checking intialization parameters

	// Let Max allocate us, our inlets, and outlets
	me = (objWeibull*) LitterAllocateObject();
	
	floatin(me, 2);												// t inlet: curve
	floatin(me, 1);												// s inlet: scale
	
	floatout(me);								// Access through me->coreObject.o_outlet
	
	// Store object components
	me->tausData = myTausStuff;
	YScale(me, iScale);
	YCurve(me, iCurve);
	
	return me;
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
	const tTypeList myArgTypes = {
						A_DEFFLOAT,		// Optional arguments:	1: scale (s)
						A_DEFFLOAT,		//						2: curve (t)
						A_DEFLONG,		// 						3: seed
										// If no seed specified, use global Taus88 data
						A_NOTHING
						};
	
	LITTER_CHECKTIMEOUT(kClassName);
	
	// Standard Max setup() call
	LitterSetupClass(	kClassName,
						sizeof(objWeibull),
						LitterCalcOffset(objWeibull),		// Class object size
						(method) YNew,			// Instance creation function
						(method) YFree,			// Custom deallocation function
						NIL,					// No menu function
						myArgTypes);		
	
	// Messages
	LITTER_TIMEBOMB LitterAddBang((method) YBang);
	LitterAddMess1	((method) YScale,	"ft1",	A_FLOAT);
	LitterAddMess1	((method) YCurve,	"ft2",	A_FLOAT);
	LitterAddMess1	((method) YSeed,	"seed",	A_DEFLONG);
	LitterAddMess2	((method) YTell,	"tell", A_SYM, A_SYM);
	LitterAddCant	((method) YTattle,	"dblclick");
	LitterAddMess0	((method) YTattle,	"tattle");
	LitterAddMess0	((method) YTattle,	"tattle");
	LitterAddCant	((method) YAssist,	"assist");
	
	AddInfo();
	
	// Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();
	
	}


