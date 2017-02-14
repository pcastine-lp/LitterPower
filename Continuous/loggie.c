/*
	File:		loggie.c

	Contains:	Max external object generating a logistic distribution.

	Written by:	Peter Castine

	Copyright:	© 2001-2002 Peter Castine

	Change History (most recent first):

         <7>      4Ð3Ð06    pc      Add support for expect message.
         <6>     15Ð1Ð04    pc      Avoid possible memory leak if seed argument is used.
         <5>     11Ð1Ð04    pc      Update for Windows.
         <4>    7Ð7Ð2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <3>  30Ð12Ð2002    pc      Drop faux 'Vers' resource and related modifications. Add object
                                    version to LoggieInfo().
         <2>  28Ð11Ð2002    pc      Tidy up after initial check-in.
         <1>  28Ð11Ð2002    pc      Initial check in.
		30-Jun-2001:	Modified to use Taus88 instead of TT800 as default generator.
						Also update to use new ULong2Unitxx() functions.
		14-Apr-2001:	General LPP Overhaul
		2-Apr-2001:		First implementation.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark ¥ Include Files

#include "LitterLib.h"
#include "TrialPeriodUtils.h"
#include "Taus88.h"
#include "MiscUtils.h"

#include <math.h>


#pragma mark ¥ Constants

const char	kClassName[]	= "lp.loggie";			// Class name


	// Indices for STR# resource
enum {
	strIndexInBang		= lpStrIndexLastStandard + 1,
	strIndexInAlpha,
	strIndexInBeta,
	
	strIndexTheOutlet
	};


#pragma mark ¥ Object Structure

typedef struct {
	LITTER_CORE_OBJECT(Object, coreObject);
	voidPtr			theOutlet;
	
	tTaus88DataPtr	tausData;
	
	double			scale,
					beta;		
	} objLogiran;


#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark ¥ Object Message Handlers

/******************************************************************************************
 *
 *	LoggieBang(me)
 *
 ******************************************************************************************/

static void
LoggieBang(
	objLogiran* me)
	
	{
	double	logistic = me->scale;			// Initialize calculation
	
	if (logistic > 0.0) {
		double u = ULong2Unit_zo( Taus88(me->tausData) );
		
		u = 1.0 / u;
		u -= 1.0;
		u = -log(u);
		
		logistic *= (u + me->beta);
		}
	
	outlet_float(me->theOutlet, logistic);
	
	}

static void
LoggieFloat(
	objLogiran*	me,
	double		iVal)
	
	{
	
	if (0.0 < iVal && iVal < 1.0) {
		double	logistic = me->scale;		// Initialize calculation
		
		if (logistic > 0.0) {			
			iVal = 1.0 / iVal;
			iVal -= 1.0;
			iVal = -log(iVal);
			
			logistic *= (iVal + me->beta);
			}
		
		outlet_float(me->theOutlet, logistic);
		}
	
	}


/******************************************************************************************
 *
 *	LoggieAlpha(me, iAlpha)
 *	LoggieBeta(me, iBeta)
 *	
 *	Set parameters, make sure nothing bad happens.
 *	
 *	NB: We allow the user to set beta to 0 here (as opposed to initialization arguments,
 *		where 0 beta defaults to 1).
 *
 ******************************************************************************************/

static void LoggieAlpha(objLogiran* me, double iAlpha)
	{ me->scale = (iAlpha > 0.0) ? 1.0 / iAlpha : 0.0; }
	// The object was originally defined arse over tit: a scale/location would be much
	// more direct. Unfortunately, the literature I originally consulted suggested the
	// convention alpha (= 1/scale) and beta (location/scale). So that's what I've stuck
	// users with.


static void LoggieBeta	(objLogiran* me, double iBeta)
	{ me->beta = iBeta; }
	

/******************************************************************************************
 *
 *	LoggieSeed(me, iSeed)
 *
 ******************************************************************************************/

static void LoggieSeed(objLogiran* me, long iSeed)
	{ Taus88Seed(me->tausData, (unsigned long) iSeed); }


#pragma mark -
#pragma mark ¥ Attribute/Information Functions

/******************************************************************************************
 *
 *	LoggieTattle(me)
 *	LoggieAssist
 *
 ******************************************************************************************/

static void LoggieAssist(objLogiran* me, void* iBox, long iDir, long iArgNum, char* oCStr)
	{
	#pragma unused (me, iBox)
	
	 LitterAssist(iDir, iArgNum, strIndexInBang, strIndexTheOutlet, oCStr);
	 }

static void
LoggieTattle(
	objLogiran* me)
	
	{
	double	scale = me->scale;
	
	post("%s state",
			kClassName);
	post("  scale = %lf (i.e., alpha = %lf)",
			scale,
			(scale > 0.0) ? (1.0 / scale) : 0.0);
	if (scale == 0.0)
		post("   -> Illegal value for alpha was specified");
	post("  beta = %lf",
			me->beta);
	
	}

/******************************************************************************************
*
*	LoggieExpect(me, iSelector, iLabel, iTarget)
*
******************************************************************************************/

static double
DoExpect(objLogiran* me, eExpectSelector iSel)
	{
	const double kPiOverRoot3 = 1.8137993642;
	
	double	result	= 0.0 / 0.0,					// Initially undefined
			scale	= me->scale;
	
	if (scale != 0.) switch(iSel) {
	case expMean:
	case expMedian:
	case expMode:
		result = me->beta * scale;
		break;
	case expVar:
	case expStdDev:
		result = kPiOverRoot3 * scale;
		if (iSel == expVar)
			result *= result;
		break;
	case expSkew:
		result = 0.0;
		break;
	case expKurtosis:
		result = 1.2;
		break;
	case expMin:
		result = -1.0 / 0.0;
		break;
	case expMax:
		result = 1.0 / 0.0;
		break;
	case expEntropy:
		result = 2.0 - log2(scale);
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

	static t_max_err LoggieGetMin(objLogiran* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMin), ioArgC, ioArgV); }
	static t_max_err LoggieGetMax(objLogiran* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMax), ioArgC, ioArgV); }
	static t_max_err LoggieGetMean(objLogiran* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMean), ioArgC, ioArgV); }
	static t_max_err LoggieGetMedian(objLogiran* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMedian), ioArgC, ioArgV); }
	static t_max_err LoggieGetMode(objLogiran* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMode), ioArgC, ioArgV); }
	static t_max_err LoggieGetVar(objLogiran* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expVar), ioArgC, ioArgV); }
	static t_max_err LoggieGetStdDev(objLogiran* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expStdDev), ioArgC, ioArgV); }
	static t_max_err LoggieGetSkew(objLogiran* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expSkew), ioArgC, ioArgV); }
	static t_max_err LoggieGetKurtosis(objLogiran* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expKurtosis), ioArgC, ioArgV); }
	static t_max_err LoggieGetEntropy(objLogiran* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expEntropy), ioArgC, ioArgV); }
	
	static t_max_err LoggieSetAttrScale(objLogiran* me, void* iAttr, long iArgC, Atom iArgV[])
		{
		
		if (iArgC > 0 && iArgV != NIL) {
			double scale = AtomGetFloat(iArgV);
			
			LoggieAlpha(me, (scale > 0.0) ? 1.0 / scale : 0.0);
			}
		
		return MAX_ERR_NONE;
		}
	
	static t_max_err LoggieGetAttrScale(objLogiran* me, void* ioAttr, long* ioArgC, Atom** ioArgV)
		{
		double scale = me->scale;
		
		return LitterGetAttrFloat((scale > 0.0) ? 1.0 / scale : 0.0, ioArgC, ioArgV);
		}
	
	static t_max_err LoggieSetAttrLoc(objLogiran* me, void* iAttr, long iArgC, Atom iArgV[])
		{
		
		if (iArgC > 0 && iArgV != NIL)
			LoggieBeta(me, AtomGetFloat(iArgV));
		
		return MAX_ERR_NONE;
		}
	
	static inline void
	AddInfo(void)
		{
		Object*	attr;
		Symbol*	symFloat64		= gensym("float64");
		
		// Read-Write Attributes
		attr = attr_offset_new(	"alpha", symFloat64, 0,
								(method) LoggieGetAttrScale, (method) LoggieSetAttrScale, 0);
		class_addattr(gObjectClass, attr);
		attr = attr_offset_new(	"loc", symFloat64, 0,
								NULL, (method) LoggieSetAttrLoc,
								calcoffset(objLogiran, beta));
		class_addattr(gObjectClass, attr);
		
		// Read-Only Attributes
		attr = attribute_new("min", symFloat64, kAttrFlagsReadOnly, (method) LoggieGetMin, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("max", symFloat64, kAttrFlagsReadOnly, (method) LoggieGetMax, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("mean", symFloat64, kAttrFlagsReadOnly, (method) LoggieGetMean, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("median", symFloat64, kAttrFlagsReadOnly, (method) LoggieGetMedian, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("mode", symFloat64, kAttrFlagsReadOnly, (method) LoggieGetMode, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("var", symFloat64, kAttrFlagsReadOnly, (method) LoggieGetVar, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("stddev", symFloat64, kAttrFlagsReadOnly, (method) LoggieGetStdDev, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("skew", symFloat64, kAttrFlagsReadOnly, (method) LoggieGetSkew, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("kurtosis", symFloat64, kAttrFlagsReadOnly, (method) LoggieGetKurtosis, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("entropy", symFloat64, kAttrFlagsReadOnly, (method) LoggieGetEntropy, NULL);
		class_addattr(gObjectClass, attr);
		}

static void
LoggieTell(
	objLogiran*	me,
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

static void LoggieInfo(objLogiran* me)
	{ LitterInfo(kClassName, &me->coreObject, (method) LoggieTattle); }

static inline void AddInfo(void)
	{ LitterAddCant((method) LoggieInfo, "info"); }
		
static void LoggieTell(objLogiran* me, Symbol* iTarget, Symbol* iMsg)
	{ LitterExpect((tExpectFunc) DoExpect, (Object*) me, iMsg, iTarget, TRUE); }

#endif




#pragma mark -


#pragma mark ¥ Class Message Handlers

/******************************************************************************************
 *
 *	LoggieFree(me)
 *	LoggieNew(iAlpha, iBeta, iSeed)
 *
 ******************************************************************************************/

static void LoggieFree(objLogiran* me)
	{ Taus88Free(me->tausData); }							// Taus88Free is NIL-safe

static void*
LoggieNew(
	double	iAlpha,
	double	iBeta,
	long	iSeed)
	
	{
	const double	kDefAlpha	= 1.0;
	
	objLogiran*		me			= NIL;
	tTaus88DataPtr	myTausStuff	= NIL;
	
	// Run through initialization parameters from right to left, handling defaults
	if (iSeed == 0) Taus88Init();
	else {
		myTausStuff = Taus88New(iSeed);
		goto noMoreDefaults;
		}
	
	if (iBeta != 0.0) goto noMoreDefaults;			// Nothing special to do w/def. beta
	
	if (iAlpha == 0.0)
		iAlpha = kDefAlpha;
noMoreDefaults:
	// Finished checking intialization parameters

	// Let Max allocate us, our inlets, and outlets
	me = (objLogiran*) LitterAllocateObject();
	
	floatin(me, 2);									// beta inlet
	floatin(me, 1);									// alpha inlet
	
	me->theOutlet	= floatout(me);
	
	// Store object components
	me->tausData	= myTausStuff;
	LoggieAlpha(me, iAlpha);
	LoggieBeta(me, iBeta);
	
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
						A_DEFFLOAT,				// Optional arguments:	1: alpha
						A_DEFFLOAT,				//						2: beta
						A_DEFLONG,				// 						3: seed
						A_NOTHING
						};
	
	LITTER_CHECKTIMEOUT(kClassName);
	
	// Standard Max setup() call
	LitterSetupClass(	kClassName,
						sizeof(objLogiran),
						LitterCalcOffset(objLogiran),	// Class object size
						(method) LoggieNew,				// Instance creation function
						(method) LoggieFree,			// Custom deallocation function
						NIL,							// No menu function
						myArgTypes);				
	

	// Messages
	LITTER_TIMEBOMB LitterAddBang((method) LoggieBang);
	LITTER_TIMEBOMB addfloat((method) LoggieFloat);
	LitterAddMess1	((method) LoggieAlpha,		"ft1",		A_FLOAT);
	LitterAddMess1	((method) LoggieBeta,		"ft2",		A_FLOAT);
	LitterAddMess1 ((method) LoggieSeed,		"seed",		A_DEFLONG);
	LitterAddMess2 ((method) LoggieTell,		"tell", A_SYM, A_SYM);
	LitterAddCant	((method) LoggieTattle,		"dblclick");
	LitterAddCant	((method) LoggieAssist,		"assist");
	
	AddInfo();

	// Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();
	
	}


