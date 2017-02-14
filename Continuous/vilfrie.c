/*
	File:		vilfrie.c

	Contains:	Max external object generating a random values with Pareto distribution.

	Written by:	Peter Castine

	Copyright:	© 2006 Peter Castine

	Change History (most recent first):

         <1>   23Ð3Ð2006    pc      first checked in.
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

const char	kClassName[]	= "lp.vilfrie";			// Class name


	// Indices for STR# resource
enum {
	strIndexInBang		= lpStrIndexLastStandard + 1,
	strIndexInAlpha,
	strIndexInBeta,
	
	strIndexTheOutlet,
	
	strIndexInLeft		= strIndexInBang,
	strIndexOutLeft		= strIndexTheOutlet
	};




#pragma mark ¥ Object Structure

typedef struct {
	LITTER_CORE_OBJECT(Object, coreObject);
	
	tTaus88DataPtr	tausData;
	
	double			alpha,
					beta,
					negAlpha1;			// Cache -1/alpha
					
	} objPareto;



#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark ¥ Object Message Handlers

/******************************************************************************************
 *
 *	VilfrieBang(me)
 *
 ******************************************************************************************/


static void
VilfrieBang(
	objPareto* me)
	
	{
	double	p;
	
	if (me->alpha > 0.0) {
		p  = ULong2Unit_ZO( Taus88(me->tausData) );
		p  = pow(p, me->negAlpha1);
		p *= me->beta;
		}
	else p = me->beta;
			
	outlet_float(me->coreObject.o_outlet, p);
	}

static void
VilfrieMap(
	objPareto*	me,
	double		iVal)
	
	{
	
	if (0.0 < iVal && iVal <= 1.0) {
		double p = me->beta;
		
		if (me->alpha > 0.0)
			p *= pow(iVal, me->negAlpha1);
		
		outlet_float(me->coreObject.o_outlet, p);
		}
	
	}
	

/******************************************************************************************
 *
 *	VilfrieAlpha(me, iAlpha)
 *	VilfrieBeta(me, iBeta)
 *	
 *	Set parameters. Make sure nothing bad is happening.
 *	
 ******************************************************************************************/

static void
VilfrieAlpha(
	objPareto*	me,
	double		iAlpha)
	
	{
	if (iAlpha < 0.0)
		iAlpha = 0.0;
	
	me->alpha		= iAlpha;
	me->negAlpha1	= -1.0 / iAlpha;
	}

static void
VilfrieBeta(
	objPareto*	me,
	double		iBeta)
	
	{
	if (iBeta < 0.0)
		iBeta = 0.0;
	
	me->beta = iBeta;
	}
	

/******************************************************************************************
 *
 *	VilfrieSeed(me, iSeed)
 *
 ******************************************************************************************/

static void VilfrieSeed(objPareto* me, long iSeed)	
	{ Taus88Seed(me->tausData, (unsigned long) iSeed); }


#pragma mark -
#pragma mark ¥ Attribute/Information Functions

/******************************************************************************************
 *
 *	VilfrieAssist(me, iBox, iDir, iArgNum, oCStr)
 *	VilfrieTattle(me)
 *
 ******************************************************************************************/

static void VilfrieAssist(objPareto* me, void* box, long iDir, long iArgNum, char* oCStr)
	{
	#pragma unused(me, box)
	
	LitterAssist(iDir, iArgNum, strIndexInLeft, strIndexOutLeft, oCStr);
	}

static void
VilfrieTattle(
	objPareto* me)
	
	{
	
	post("%s state",
			kClassName);
	post("  parameters (%lf, %lf)", me->alpha, me->beta);
	post("  Cached value of -1/alpha: %lf", me->negAlpha1);
	
	}


/******************************************************************************************
 *
 *	DoExpect()
 *
 *	Helper function for calculating expected values (mean, standard deviation, etc.)
 *	
 ******************************************************************************************/

static double DoExpect(objPareto* me, eExpectSelector iSel)
	{
	double	result	= 0.0 / 0.0,					// Initially undefined
			alpha	= me->alpha,
			beta	= me->beta;
	
	switch(iSel) {
	case expMean:
		if (beta == 0.0)
			result = 0.0;
		else if (alpha > 1.0) {
			result  = alpha * beta;
			result /= alpha - 1.0;
			}
		break;
	case expMedian:
		result = (alpha > 0.0)
					? beta * pow(2.0, -me->negAlpha1)
					: beta;
		break;
	case expMode:
	case expMin:
		result = beta;
		break;
	case expVar:
	case expStdDev:
		if (beta == 0.0)
			result = 0.0;
		else if (alpha > 2.0) {
			result = alpha * beta * beta;
			
			alpha -= 1.0; result /= alpha * alpha * (alpha - 1.0);
			
			if (iSel == expStdDev) result = sqrt(result);
			}
		break;
	case expSkew:
		if (beta == 0.0)
			result = 0.0;
		else if (alpha > 3.0) {
			result  = sqrt((alpha - 2.0) / alpha);
			result *= alpha + 1.0;
			result /= alpha - 3.0;
			result += result;
			}
		break;
	case expKurtosis:
		if (beta == 0.0)
			result = 0.0;
		if (alpha > 4.0) {
			result  = alpha * alpha * alpha;
			result += alpha * alpha;
			result -= 6.0 * alpha;
			result -= 2.0;
			result /= alpha * (alpha - 3.0) * (alpha - 4.0);
			result *= 6.0;
			}
		break;
	case expEntropy:
		if (beta == 0.0)
			result = 0.0;
		else {
			result  = log(alpha / beta);
			result += me->negAlpha1;
			result -= 1.0;
			}
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

	static t_max_err VilfrieGetMin(objPareto* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMin), ioArgC, ioArgV); }
	static t_max_err VilfrieGetMax(objPareto* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMax), ioArgC, ioArgV); }
	static t_max_err VilfrieGetMean(objPareto* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMean), ioArgC, ioArgV); }
	static t_max_err VilfrieGetMedian(objPareto* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMedian), ioArgC, ioArgV); }
	static t_max_err VilfrieGetMode(objPareto* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMode), ioArgC, ioArgV); }
	static t_max_err VilfrieGetVar(objPareto* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expVar), ioArgC, ioArgV); }
	static t_max_err VilfrieGetStdDev(objPareto* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expStdDev), ioArgC, ioArgV); }
	static t_max_err VilfrieGetSkew(objPareto* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expSkew), ioArgC, ioArgV); }
	static t_max_err VilfrieGetKurtosis(objPareto* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expKurtosis), ioArgC, ioArgV); }
	static t_max_err VilfrieGetEntropy(objPareto* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expEntropy), ioArgC, ioArgV); }
	
	static t_max_err VilfrieSetAttrAlpha(objPareto* me, void* iAttr, long iArgC, Atom iArgV[])
		{
		
		if (iArgC > 0 && iArgV != NIL)
			VilfrieAlpha(me, AtomGetFloat(iArgV));
		
		return MAX_ERR_NONE;
		}
	
	static t_max_err VilfrieSetAttrBeta(objPareto* me, void* iAttr, long iArgC, Atom iArgV[])
		{
		
		if (iArgC > 0 && iArgV != NIL)
			VilfrieBeta(me, AtomGetFloat(iArgV));
		
		return MAX_ERR_NONE;
		}
	
	static inline void
	AddInfo(void)
		{
		Object*	attr;
		Symbol*	symFloat64		= gensym("float64");
		
		// Read-Write Attributes
		attr = attr_offset_new(	"alpha", symFloat64, 0,
								NULL, (method) VilfrieSetAttrAlpha,
								calcoffset(objPareto, alpha));
		class_addattr(gObjectClass, attr);
		attr = attr_offset_new(	"beta", symFloat64, 0,
								NULL, (method) VilfrieSetAttrBeta,
								calcoffset(objPareto, beta));
		class_addattr(gObjectClass, attr);
		
		// Read-Only Attributes
		attr = attribute_new("min", symFloat64, kAttrFlagsReadOnly, (method) VilfrieGetMin, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("max", symFloat64, kAttrFlagsReadOnly, (method) VilfrieGetMax, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("mean", symFloat64, kAttrFlagsReadOnly, (method) VilfrieGetMean, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("median", symFloat64, kAttrFlagsReadOnly, (method) VilfrieGetMedian, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("mode", symFloat64, kAttrFlagsReadOnly, (method) VilfrieGetMode, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("var", symFloat64, kAttrFlagsReadOnly, (method) VilfrieGetVar, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("stddev", symFloat64, kAttrFlagsReadOnly, (method) VilfrieGetStdDev, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("skew", symFloat64, kAttrFlagsReadOnly, (method) VilfrieGetSkew, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("kurtosis", symFloat64, kAttrFlagsReadOnly, (method) VilfrieGetKurtosis, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("entropy", symFloat64, kAttrFlagsReadOnly, (method) VilfrieGetEntropy, NULL);
		class_addattr(gObjectClass, attr);
		}


static void
VilfrieTell(
	objPareto*	me,
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

static void VilfrieInfo(objPareto* me)
	{ LitterInfo(kClassName, &me->coreObject, (method) VilfrieTattle); }

static inline void AddInfo(void)
	{ LitterAddCant((method) VilfrieInfo, "info"); }
		
static void VilfrieTell(objPareto* me, Symbol* iTarget, Symbol* iMsg)
	{ LitterExpect((tExpectFunc) DoExpect, (Object*) me, iMsg, iTarget, TRUE); }

#endif


#pragma mark -
#pragma mark ¥ Class Message Handlers

/******************************************************************************************
 *
 *	VilfrieNew(iOracle)
 *
 ******************************************************************************************/

static void VilfrieFree(objPareto* me)
	{ Taus88Free(me->tausData); }							// Taus88Free is NIL-safe

static void*
VilfrieNew(
	double	iAlpha,
	double	iBeta,
	long	iSeed)
	
	{
	const double	kDefParam	= 1.0;					// The same default for both a and b
	
	objPareto*		me			= NIL;
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
	me = (objPareto*) LitterAllocateObject();
	
	floatin(me, 2);									// parameter b inlet
	floatin(me, 1);									// parameter a inlet
	
	floatout(me);									// Access through me->coreObject.o_outlet
	
	// Store object components
	me->tausData	= myTausStuff;
	VilfrieAlpha(me, iAlpha);
	VilfrieBeta(me, iBeta);
	
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
						A_DEFFLOAT,					// Optional arguments:	1: a
						A_DEFFLOAT,					//						2: b
						A_DEFLONG,					// 						3: seed
						A_NOTHING
						};
	
	LITTER_CHECKTIMEOUT(kClassName);
	
	// Standard Max setup() call
	LitterSetupClass(	kClassName,
						sizeof(objPareto),
						LitterCalcOffset(objPareto),	// Class object size
						(method) VilfrieNew,			// Instance creation function
						(method) VilfrieFree,			// Custom deallocation function
						NIL,							// No menu function
						myArgTypes);				
	
	// Messages
	LITTER_TIMEBOMB LitterAddBang((method) VilfrieBang);
	LITTER_TIMEBOMB addfloat((method) VilfrieMap);
	LitterAddMess1	((method) VilfrieAlpha,		"ft1",		A_DEFFLOAT);
	LitterAddMess1	((method) VilfrieBeta,		"ft2",		A_DEFFLOAT);
	LitterAddMess1	((method) VilfrieSeed,		"seed",		A_DEFLONG);
	LitterAddMess2	((method) VilfrieTell,		"tell", A_SYM, A_SYM);
	LitterAddCant	((method) VilfrieTattle,	"dblclick");
	LitterAddMess0	((method) VilfrieTattle,	"tattle");
	LitterAddCant	((method) VilfrieAssist,	"assist");
	
	AddInfo();
	
	// Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();
	
	}


