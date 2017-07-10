/*
	File:		lon.c

	Contains:	Max external object generating a log-normal distribution.

	Written by:	Peter Castine

	Copyright:	© 2001-2002 Peter Castine

	Change History (most recent first):

         <8>   24–3–2006    pc      Further updates for new LitterLib organization.
         <7>   23–3–2006    pc      Add support for expect message.
         <6>     15–1–04    pc      Avoid possible memory leak if seed argument is used.
         <5>     11–1–04    pc      Update for Windows.
         <4>    7–7–2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <3>  30–12–2002    pc      Drop faux 'Vers' resource and related modifications. Add object
                                    version to LonInfo().
         <2>  28–11–2002    pc      Tidy up after initial check-in.
         <1>  28–11–2002    pc      Initial check in.
		30-Jun-2001:	Modified to use Taus88 instead of TT800 as default generator.
						Also update to use new ULong2Unitxx() functions.
		10-May-2001:	First implementation.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "LitterLib.h"
#include "TrialPeriodUtils.h"
#include "RNGGauss.h"
#include "Taus88.h"
#include "MiscUtils.h"

#include <math.h>


#pragma mark • Constants

const char*	kClassName		= "lp.lonnie";			// Class name


	// Indices for STR# resource
enum {
	strIndexInBang		= lpStrIndexLastStandard + 1,
	strIndexInMean,
	strIndexInStDev,
	
	strIndexTheOutlet,
	
	strIndexLeftInlet	= strIndexInBang,
	strIndexLeftOutlet	= strIndexTheOutlet
	};


#pragma mark • Object Structure

typedef struct {
	LITTER_CORE_OBJECT(Object, coreObject);
	
	tTaus88DataPtr	tausData;
	
	double			mean,							// Nominal mean
					baseMean,						// Mean of the base normal distribution
					stdDev,							// Nominal standard deviation
					baseStdDev;						// Standard deviation of base normal dist.
	} objLogNorm;



#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	LonBang(me)
 *
 ******************************************************************************************/

static void
LonBang(
	objLogNorm* me)
	
	{
	double			l;
	
	if (me->mean > 0.0) {
		l = NormalKRTaus88(me->tausData);
		l *= me->baseStdDev;
		l += me->baseMean;
		l = exp(l);
		}
	
	else l = 0.0;											// Degenerate case
			
	outlet_float(me->coreObject.o_outlet, l);
	
	}


/******************************************************************************************
 *
 *	LonMean(me, iOrder)
 *	LonStdDev(me, iStdDev)
 *	LonSeed(me, iSeed)
 *	
 *	Set parameters, making sure nothing bad happens.
 *	
 ******************************************************************************************/
	static void UpdateBaseParams(objLogNorm* me)
		{
	    double	mean = me->mean;

	    if (mean > 0.0) {
			double	m2,				// Mean^2
					s2m2;			// Mean^2 + StdDev^2
			
			m2		= mean * mean;
			s2m2	= me->stdDev;
			s2m2	*= s2m2;
			s2m2	+= m2;
			
			me->baseMean	= log(m2 / sqrt(s2m2));
			me->baseStdDev	= sqrt(log(s2m2 / m2));
			}
		
		else {
			me->baseMean	= 0.0;
			me->baseStdDev	= 0.0;
			}
		
		}

static void LonMean(objLogNorm* me, double iMean)
	{
	me->mean = (iMean >= 0.0) ? iMean : 0.0;
	UpdateBaseParams(me);
	}

static void LonStdDev(objLogNorm* me, double iStdDev)
	{
	me->stdDev = iStdDev;
	UpdateBaseParams(me);
	}
	
static void LonSeed(objLogNorm* me, long iSeed)
	{ Taus88Seed(me->tausData, (unsigned long) iSeed); }

#pragma mark -
#pragma mark • Attribute/Information Functions

/******************************************************************************************
 *
 *	LonAssist(me, iBox, iDir, iArgNum, oCStr)
 *	LonTattle(me)
 *
 ******************************************************************************************/

static void
LonAssist(
	objLogNorm*	me,
	void*		iBox,
	long		iDir,
	long		iArgNum,
	char*		oCStr)
	
	{
	#pragma unused(me, iBox)
	
	LitterAssist(iDir, iArgNum, strIndexLeftInlet, strIndexLeftOutlet, oCStr);
	}

static void
LonTattle(
	objLogNorm* me)
	
	{
	
	post("%s state",
			kClassName);
	post("  mean = %lf (mean of base normal distribution is %lf)",
			me->mean,
			me->baseMean);
	if (me->mean == 0.0)
		post("   Note: Invalid data entered for mean. This also effects the base Std.Dev.");
	post("  Std. Dev. = %lf (sigma of base normal distribution is %lf)",
			me->stdDev,
			me->baseStdDev);
			
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
	objLogNorm*		me,
	eExpectSelector iSel)
	
	{
	double	result = 0.0 / 0.0;
	
	switch(iSel) {
	case expMean:
		result = me->mean;
		break;
	case expMedian:
		result = exp(me->baseMean);
		break;
	case expVar:
		result	= me->stdDev;
		result *= result;
		break;
	case expStdDev:
		result  = me->stdDev;
		break;
	case expMin:
		result = 0.0;
		break;
	case expMax:
		result = 1.0 / 0.0;
		break;
	default:
		{
		double sigma2;					// sigma^2 of base standard distribution
										// needed for all remaining values
		sigma2  = me->baseStdDev;
		sigma2 *= sigma2;
		
		switch (iSel) {
		case expMode:
			result = exp(me->baseMean - sigma2);
			break;
		case expSkew:
			sigma2  = me->baseStdDev;
			sigma2 *= sigma2;
			result  = exp(sigma2);
			result -= 1.0;
			result  = sqrt(result);
			result *= exp(sigma2) + 2.0;
			break;
		case expKurtosis:
			sigma2  = me->baseStdDev;
			sigma2 *= sigma2;
			result  = exp(4.0 * sigma2);
			result += 2.0 * exp(3.0 * sigma2);
			result += 3.0 * exp(sigma2 + sigma2);
			result -= 6.0;
			break;
		case expEntropy:
			sigma2  = me->baseStdDev;
			sigma2 *= sigma2;
			result  = 1.0 + log(k2pi * sigma2);
			result *= 0.5;
			break;
		default:
			break;
			}							// End inner switch
		}								// End default block
		}								// End outer switch
	
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

	static t_max_err LonGetMin(objLogNorm* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMin), ioArgC, ioArgV); }
	static t_max_err LonGetMax(objLogNorm* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMax), ioArgC, ioArgV); }
	static t_max_err LonGetMedian(objLogNorm* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMedian), ioArgC, ioArgV); }
	static t_max_err LonGetMode(objLogNorm* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMode), ioArgC, ioArgV); }
	static t_max_err LonGetVar(objLogNorm* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expVar), ioArgC, ioArgV); }
	static t_max_err LonGetSkew(objLogNorm* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expSkew), ioArgC, ioArgV); }
	static t_max_err LonGetKurtosis(objLogNorm* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expKurtosis), ioArgC, ioArgV); }
	static t_max_err LonGetEntropy(objLogNorm* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expEntropy), ioArgC, ioArgV); }
	
	static t_max_err LonSetAttrMean(objLogNorm* me, void* iAttr, long iArgC, Atom iArgV[])
		{
		
		if (iArgC > 0 && iArgV != NIL)
			LonMean(me, AtomGetFloat(iArgV));
		
		return MAX_ERR_NONE;
		}
	
	static t_max_err LonSetAttrStdDev(objLogNorm* me, void* iAttr, long iArgC, Atom iArgV[])
		{
		
		if (iArgC > 0 && iArgV != NIL)
			LonStdDev(me, AtomGetFloat(iArgV));
		
		return MAX_ERR_NONE;
		}
	
	static inline void
	AddInfo(void)
		{
		Object*	attr;
		Symbol*	symFloat64		= gensym("float64");
		
		// Read-Write Attributes
		attr = attr_offset_new(	"mean", symFloat64, 0,
								NULL, (method) LonSetAttrMean,
								calcoffset(objLogNorm, mean));
		class_addattr(gObjectClass, attr);
		attr = attr_offset_new(	"stddev", symFloat64, 0,
								NULL, (method) LonSetAttrStdDev,
								calcoffset(objLogNorm, stdDev));
		class_addattr(gObjectClass, attr);
		
		// Read-Only Attributes
		attr = attribute_new("min", symFloat64, kAttrFlagsReadOnly, (method) LonGetMin, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("max", symFloat64, kAttrFlagsReadOnly, (method) LonGetMax, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("median", symFloat64, kAttrFlagsReadOnly, (method) LonGetMedian, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("mode", symFloat64, kAttrFlagsReadOnly, (method) LonGetMode, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("var", symFloat64, kAttrFlagsReadOnly, (method) LonGetVar, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("skew", symFloat64, kAttrFlagsReadOnly, (method) LonGetSkew, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("kurtosis", symFloat64, kAttrFlagsReadOnly, (method) LonGetKurtosis, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("entropy", symFloat64, kAttrFlagsReadOnly, (method) LonGetEntropy, NULL);
		class_addattr(gObjectClass, attr);
		}

static void
LonTell(
	objLogNorm*	me,
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

static void LonInfo(objLogNorm* me)
	{ LitterInfo(kClassName, &me->coreObject, (method) LonTattle); }

static inline void AddInfo(void)
	{ LitterAddCant((method) LonInfo, "info"); }
		
static void LonTell(objLogNorm* me, Symbol* iTarget, Symbol* iMsg)
	{ LitterExpect((tExpectFunc) DoExpect, (Object*) me, iMsg, iTarget, TRUE); }


#endif


#pragma mark -
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	LonFree(me)
 *	LonNew(iMean, iStdDev, iSeed)
 *
 ******************************************************************************************/

static void LonFree(objLogNorm* me)
	{ Taus88Free(me->tausData); }							// Taus88Free is NIL-safe


static void*
LonNew(
	double	iMean,
	double	iStdDev,
	long	iSeed)
	
	{
	const double	kDefMean	= 1.0;
	const double	kDefStdDev	= 1.0;
	
	objLogNorm*		me			= NIL;
	tTaus88DataPtr	myTausStuff	= NIL;
	
	// Run through initialization parameters from right to left, handling defaults
	if (iSeed != 0) {
		myTausStuff = Taus88New(iSeed);
		goto noMoreDefaults;
		}
	
	if (iStdDev == 0.0)
		iStdDev = kDefStdDev;
	else goto noMoreDefaults;
	
	if (iMean == 0)
		iMean = kDefMean;
	
noMoreDefaults:
	// Finished checking intialization parameters
	
	// Let Max allocate us, our inlets (from right to left), and outlets
	me = (objLogNorm*) LitterAllocateObject();
	
	floatin(me, 2);
	floatin(me, 1);
	
	floatout(me);								// Access through me->coreObject.o_outlet
	
	// Initialize object components
	me->tausData	= myTausStuff;
	me->mean		= (iMean >= 0.0) ? iMean : 0.0;
	me->stdDev		= iStdDev;
	UpdateBaseParams(me);
	
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
						A_DEFFLOAT,				// Optional arguments:	1: Order
						A_DEFFLOAT,				//						2: Location
						A_DEFLONG,				//						3: Seed
						A_NOTHING
						};
	
	LITTER_CHECKTIMEOUT(kClassName);
	
	// Standard Max setup() call
	LitterSetupClass(	kClassName,
						sizeof(objLogNorm),
						LitterCalcOffset(objLogNorm),	// Class object size
						(method) LonNew,				// Instance creation function
						(method) LonFree,				// Custom deallocation function
						NIL,						// No menu function
						myArgTypes);				
	

	// Messages
	LITTER_TIMEBOMB LitterAddBang((method) LonBang);
	LitterAddMess1	((method) LonMean,		"ft1",	A_FLOAT);
	LitterAddMess1	((method) LonStdDev,	"ft2",	A_FLOAT);
	LitterAddMess1	((method) LonSeed,		"seed",	A_DEFLONG);
	LitterAddMess2	((method) LonTell,		"tell", A_SYM, A_SYM);
	LitterAddCant	((method) LonTattle,	"dblclick");
	LitterAddCant	((method) LonAssist,	"assist");
	
	AddInfo();
	
	// Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();
	
	}

