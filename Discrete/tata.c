/*
	File:		tata.c

	Contains:	Max external object implementing the Taus88 (Tausworthe) random number
				generator.

	Written by:	Peter Castine

	Copyright:	© 2001-2002 Peter Castine

	Change History (most recent first):

         <6>     27–2–06    pc      Add expect message, update for new LitterLib file organization.
         <5>     14–1–04    pc      Update for Windows.
         <4>    7–7–2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <3>  30–12–2002    pc      Drop faux 'Vers' resource and related modifications. Add object
                                    version to DoInfo().
         <2>  28–11–2002    pc      Tidy up after initial check in.
         <1>  28–11–2002    pc      Initial check in.
	Change History (most recent first):
		16-Apr-2001:	First implementation.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files


#include "LitterLib.h"
#include "TrialPeriodUtils.h"
#include "Taus88.h"
#include "MiscUtils.h"
#include "UniformExpectations.h"


#pragma mark • Constants

const char	kClassName[]		= "lp.tata";			// Class name


	// Indices for STR# resource
enum {
	strIndexBang		= lpStrIndexLastStandard + 1,
	strIndexMin,
	strIndexMax,
	
	strIndexTheOutlet
	};

#pragma mark • Object Structure

typedef struct {
	LITTER_CORE_OBJECT(Object, coreObject);

	tTaus88DataPtr	tausStuff;
	long			min,
					max;
	} objTaus88;


#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	TataNew(iMin, iMax, iSeed)
 *	TataFree(me)
 *
 ******************************************************************************************/

static void*
TataNew(
	long	iMin,
	long	iMax,
	long	iSeed)
	
	{
	const long	kDefMin	= kLongMin,
				kDefMax = kLongMax;
	
	objTaus88*		me			= NIL;
	tTaus88DataPtr	myTausStuff	= NIL;
	
	// Run through initialization parameters from right to left, handling defaults
	if (iSeed != 0) {
		myTausStuff = Taus88New(iSeed);
		goto noMoreDefaults;
		}
	
	if (iMax == 0) {
		if (iMin > 0) {
			iMax = iMin;
			iMin = 0;
			}
		else if (iMin == 0) {
			iMin = kDefMin;
			iMax = kDefMax;
			}
		}
noMoreDefaults:
	// Finished checking intialization parameters
	
	// Let Max allocate us, our inlets, and outlets
	me = (objTaus88*) LitterAllocateObject();
	
	intin(me, 2);										// Max inlet
	intin(me, 1);										// Min inlet
	
	intout(me);					// Access main outlet through me->coreObject.o_outlet;
	
	// Initialize and store object components
	me->tausStuff = myTausStuff;
	me->min		= iMin;
	me->max		= iMax;
	
	return me;
	}

static void TataFree(objTaus88* me)
	{ Taus88Free(me->tausStuff); }							// Taus88Free is NIL-safe


#pragma mark -
#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	TataBang(me)
 *
 *	Grab a new random number from Taus88. Scale if necessary.
 *
 ******************************************************************************************/

static void
TataBang(
	objTaus88* me)
	
	{
	long	taus,
			min		= me->min,
			max		= me->max;
	
	if (min < max) {
		double	scale = (double) max - (double) min + 1.0;
		
		taus = Taus88(me->tausStuff);
		taus = scale * ULong2Unit_Zo(taus) + min;
		}
	else taus = (min == max) ? min : 0;
	
	outlet_int(me->coreObject.o_outlet, taus);
	
	}


/******************************************************************************************
 *
 *	TataMin(me, iMin)
 *	TataMax(me, iMax)
 *	TataSeed(me, iSeed)
 *
 ******************************************************************************************/

static void TataMin(objTaus88* me, long iMin)	{ me->min = iMin; }
static void TataMax(objTaus88* me, long iMax)	{ me->max = iMax; }
static void TataSeed(objTaus88* me, long iSeed)	{ Taus88Seed(me->tausStuff, (UInt32) iSeed); }

#pragma mark -
#pragma mark • Attribute/Information Functions

/******************************************************************************************
 *
 *	TataAssist
 *	TataTattle(me)
 *
 ******************************************************************************************/

static void
TataAssist(
	objTaus88*	me,
	void*		box,
	long		iDir,
	long		iArgNum,
	char*		oCStr)
	
	{
	#pragma unused (box)
	
	long	min = me->min,
			max = me->max;
	
	if (min >= max) {
		min = kLongMin;
		max = kLongMax;
		}
	
	LitterAssistVA(iDir, iArgNum, strIndexBang, strIndexTheOutlet, oCStr, min, max);
	
	}

static void
TataTattle(
	objTaus88* me)
	
	{
	tTaus88DataPtr	myTausStuff = me->tausStuff;
	
	post("%s state", kClassName);
	post("  Range: [%ld .. %ld]", me->min, me->max);
	if (me->min > me->max)
		post("  Ignoring range while min > max");
	
	if (myTausStuff != NIL) {
		post("  Seeds: 0x%lx, 0x%lx, 0x%lx",
				myTausStuff->seed1, myTausStuff->seed2, myTausStuff->seed3);
		}
	else {
		post("  Using global Taus88 data");
		}
	
	}


/******************************************************************************************
 *
 *	DoExpect()
 *
 *	Helper function for calculating expected values (mean, standard deviation, etc.) for
 *	Taus88
 *	
 ******************************************************************************************/

static double
DoExpect(
	objTaus88*		me,
	eExpectSelector iSel)
	
	{
	return UniformExpectationsDiscrete(me->min, me->max, iSel);
	}


/******************************************************************************************
 *
 *	Attribute Funtions
 *
 *	Functions for Inspector/Get Info... dialog
 *	
 ******************************************************************************************/

#if LITTER_USE_OBEX

	static t_max_err TataGetMean(objTaus88* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expMean), ioArgC, ioArgV);
		}
	static t_max_err TataGetMedian(objTaus88* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expMedian), ioArgC, ioArgV);
		}
	static t_max_err TataGetMode(objTaus88* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expMode), ioArgC, ioArgV);
		}
	static t_max_err TataGetVar(objTaus88* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expVar), ioArgC, ioArgV);
		}
	static t_max_err TataGetStdDev(objTaus88* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expStdDev), ioArgC, ioArgV);
		}
	static t_max_err TataGetSkew(objTaus88* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expSkew), ioArgC, ioArgV);
		}
	static t_max_err TataGetKurtosis(objTaus88* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expKurtosis), ioArgC, ioArgV);
		}
	static t_max_err TataGetEntropy(objTaus88* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expEntropy), ioArgC, ioArgV);
		}
	
	static inline void
	AddInfo(void)
		{
		Object*	attr;
		Symbol*	symFloat64		= gensym("float64");
		Symbol*	symLong			= gensym("long");
		
		// Read-Write Attributes
		attr = attr_offset_new("min", symLong, 0, NULL, NULL, calcoffset(objTaus88, min));
		class_addattr(gObjectClass, attr);
		attr = attr_offset_new("max", symLong, 0, NULL, NULL, calcoffset(objTaus88, max));
		class_addattr(gObjectClass, attr);
		
		// Read-Only Attributes
		attr = attribute_new("mean", symFloat64, kAttrFlagsReadOnly, (method) TataGetMean, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("median", symFloat64, kAttrFlagsReadOnly, (method) TataGetMedian, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("mode", symFloat64, kAttrFlagsReadOnly, (method) TataGetMode, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("var", symFloat64, kAttrFlagsReadOnly, (method) TataGetVar, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("stddev", symFloat64, kAttrFlagsReadOnly, (method) TataGetStdDev, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("skew", symFloat64, kAttrFlagsReadOnly, (method) TataGetSkew, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("kurtosis", symFloat64, kAttrFlagsReadOnly, (method) TataGetKurtosis, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("entropy", symFloat64, kAttrFlagsReadOnly, (method) TataGetEntropy, NULL);
		class_addattr(gObjectClass, attr);
		}

static void
TataTell(
	objTaus88*	me,
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

static void TataInfo(objTaus88* me)
	{ LitterInfo(kClassName, &me->coreObject, (method) TataTattle); }

static inline void AddInfo(void)
	{ LitterAddCant((method) TataInfo, "info"); }
		
static void TataTell(objTaus88* me, Symbol* iTarget, Symbol* iAttrName)
	{ LitterExpect((tExpectFunc) DoExpect, (Object*) me, iAttrName, iTarget, TRUE); }

#endif

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
						A_DEFLONG,		// Minimum	(Default: kMinLong)
						A_DEFLONG,		// Maximum	(Default: kMaxLong)
						A_DEFLONG,		// Seed		(Default: Zero -- autoseed)
						A_NOTHING
						};
	
	LITTER_CHECKTIMEOUT(kClassName);
	
	LitterSetupClass(	kClassName,
						sizeof(objTaus88),
						LitterCalcOffset(objTaus88),
						(method) TataNew,
						(method) TataFree,
						NIL,
						myArgTypes);				
	
	// Messages
	LITTER_TIMEBOMB LitterAddBang((method) TataBang);
	LitterAddMess1	((method) TataMin,		"in1",		A_LONG);
	LitterAddMess1	((method) TataMax,		"in2",		A_LONG);
	LitterAddMess1	((method) TataSeed,		"seed",		A_DEFLONG);
	LitterAddMess2	((method) TataTell,		"tell",		A_SYM, A_SYM);
	LitterAddMess0	((method) TataTattle,	"tattle");
	LitterAddCant	((method) TataTattle,	"dblclick");
	LitterAddCant	((method) TataAssist,	"assist");

	AddInfo();
	
	//Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();
	
	}


