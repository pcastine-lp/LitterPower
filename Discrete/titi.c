/*
	File:		titi.c

	Contains:	Max external object implementing the Titi random number generator.

	Written by:	Peter Castine

	Copyright:	© 2001-2002 Peter Castine

	Change History (most recent first):

         <6>     15Ð3Ð06    pc      Add expect message
         <5>     14Ð1Ð04    pc      Update for Windows.
         <4>    7Ð7Ð2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <3>  30Ð12Ð2002    pc      Drop faux 'Vers' resource and related modifications. Add object
                                    version to DoInfo().
         <2>  28Ð11Ð2002    pc      Tidy up after initial check in.
         <1>  28Ð11Ð2002    pc      Initial check in.
		14-Apr-2001:	LPP Overhaul.
		31-Mar-2001:	First implementation. Based on M. Matsumoto's code, but tweaked
						for speed.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark ¥ Include Files

#include "LitterLib.h"
#include "TrialPeriodUtils.h"
#include "TT800.h"
#include "MiscUtils.h"
#include "UniformExpectations.h"

#pragma mark ¥ Constants

const char*	kClassName		= "lp.titi";			// Class name


	// Indices for STR# resource
enum {
	strIndexBang		= lpStrIndexLastStandard + 1,
	strIndexMin,
	strIndexMax,
	
	strIndexTheOutlet,
	
	strIndexLeftInlet	= strIndexBang,
	strIndexLeftOutlet	= strIndexTheOutlet
	};



#pragma mark ¥ Object Structure

typedef struct {
	LITTER_CORE_OBJECT(Object, coreObject);
	
	tTT800DataPtr	ttStuff;
	
	long			min,
					max;
	} objTT800;


#pragma mark -

/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark ¥ Object Message Handlers

/******************************************************************************************
 *
 *	TitiBang(me)
 *
 *	Grab a new random number from Titi. Scale if necessary.
 *
 ******************************************************************************************/

static void
TitiBang(
	objTT800* me)
	
	{
	long	tt,
			min		= me->min,
			max		= me->max;
	
	if (min < max) {
		double	scale = (double) max - (double) min + 1.0;
		
		tt = TT800(me->ttStuff);
		tt = scale * ULong2Unit_ZO(tt) + min;
		}
	else tt = (min == max) ? min : 0;
	
	outlet_int(me->coreObject.o_outlet, tt);
	
	}


/******************************************************************************************
 *
 *	TitiMin(me, iMin)
 *	TitiMax(me, iMax)
 *	TitiSeed(me, iSeed)
 *
 ******************************************************************************************/

static void TitiMin(objTT800* me, long iMin)	{ me->min = iMin; }
static void TitiMax(objTT800* me, long iMax)	{ me->max = iMax; }

static void TitiSeed(objTT800* me, long iSeed)
	{ TT800Seed(me->ttStuff, (unsigned long) iSeed); }


/******************************************************************************************
 *
 *	TitiTattle(me)
 *	TitiAssist(me, iBox, iDir, iArgNum, oCStr)
 *	TitiInfo(me)
 *
 ******************************************************************************************/

static void
TitiTattle(
	objTT800* me)
	
	{
	
	post("%s state",
			kClassName);
	post("  Range: [%ld .. %ld]",
			me->min, me->max);
	if (me->min > me->max)
		post("  NB: Ignoring range while min > max");
	
	}


static void
TitiAssist(
	objTT800*	me,
	void*		iBox,
	long		iDir,
	long		iArgNum,
	char*		oCStr)
	
	{
	#pragma unused(iBox)
	
	if (iDir == ASSIST_INLET)
		LitterAssist(iDir, iArgNum, strIndexLeftInlet, 0, oCStr);
		
	else {
		long	min = me->min,
				max = me->max;
		
		if (min >= max) {
			min = kLongMin;
			max = kLongMax;
			}
		
		LitterAssistVA(iDir, iArgNum, 0, strIndexLeftOutlet, oCStr, min, max);
		}
	
	}

#pragma mark -
#pragma mark ¥ Class Message Handlers

/******************************************************************************************
 *
 *	TitiNew(iMin, iMax, iSeed)
 *	TitiFree(me)
 *
 ******************************************************************************************/

static void*
TitiNew(
	long	iMin,
	long	iMax,
	long	iSeed)
	
	{
	objTT800*		me			= NIL;
	tTT800DataPtr	myTTStuff	= NIL;
	
	// Run through initialization parameters from right to left, handling defaults
	if (iSeed == 0) TT800Init();
	else {
		myTTStuff = TT800New(iSeed);
		goto noMoreDefaults;
		}
	
	if (iMax == 0) {
		if (iMin > 0) {
			iMax = iMin;
			iMin = 0;
			}
		else if (iMin == 0) {
			iMin = kLongMin;
			iMax = kLongMax;
			}
		}
noMoreDefaults:
	// Finished checking intialization parameters
	
	// Let Max allocate us, our inlets, and outlets
	me = (objTT800*) LitterAllocateObject();
	
	intin(me, 2);										// Max inlet
	intin(me, 1);										// Min inlet
	
	intout(me);					// Access main outlet through me->coreObject.o_outlet;
	
	// Store object components
	me->ttStuff	= myTTStuff;
	me->min		= iMin;
	me->max		= iMax;
	
	return me;
	}


static void TitiFree(objTT800* me)
	{ TT800Free(me->ttStuff); }								// TitiFree() is NULL-safe


#pragma mark -
#pragma mark ¥ Attribute/Information Functions

/******************************************************************************************
 *
 *	DoExpect()
 *
 *	Helper function for calculating expected values (mean, standard deviation, etc.)
 *	
 ******************************************************************************************/

static double DoExpect(objTT800* me, eExpectSelector iSel)
	{ return UniformExpectationsDiscrete(me->min, me->max, iSel); }

/******************************************************************************************
 *
 *	Attribute Funtions
 *
 *	Functions for Inspector/Get Info... dialog
 *	
 ******************************************************************************************/

#if LITTER_USE_OBEX

	static t_max_err TitiGetMean(objTT800* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expMean), ioArgC, ioArgV);
		}
	static t_max_err TitiGetMedian(objTT800* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expMedian), ioArgC, ioArgV);
		}
	static t_max_err TitiGetMode(objTT800* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expMode), ioArgC, ioArgV);
		}
	static t_max_err TitiGetVar(objTT800* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expVar), ioArgC, ioArgV);
		}
	static t_max_err TitiGetStdDev(objTT800* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expStdDev), ioArgC, ioArgV);
		}
	static t_max_err TitiGetSkew(objTT800* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expSkew), ioArgC, ioArgV);
		}
	static t_max_err TitiGetKurtosis(objTT800* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expKurtosis), ioArgC, ioArgV);
		}
	static t_max_err TitiGetEntropy(objTT800* me, void* iAttr, long* ioArgC, Atom** ioArgV)
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
		attr = attr_offset_new("min", symLong, 0, NULL, NULL, calcoffset(objTT800, min));
		class_addattr(gObjectClass, attr);
		attr = attr_offset_new("max", symLong, 0, NULL, NULL, calcoffset(objTT800, max));
		class_addattr(gObjectClass, attr);
		
		// Read-Only Attributes
		attr = attribute_new("mean", symFloat64, kAttrFlagsReadOnly, (method) TitiGetMean, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("median", symFloat64, kAttrFlagsReadOnly, (method) TitiGetMedian, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("mode", symFloat64, kAttrFlagsReadOnly, (method) TitiGetMode, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("var", symFloat64, kAttrFlagsReadOnly, (method) TitiGetVar, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("stddev", symFloat64, kAttrFlagsReadOnly, (method) TitiGetStdDev, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("skew", symFloat64, kAttrFlagsReadOnly, (method) TitiGetSkew, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("kurtosis", symFloat64, kAttrFlagsReadOnly, (method) TitiGetKurtosis, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("entropy", symFloat64, kAttrFlagsReadOnly, (method) TitiGetEntropy, NULL);
		class_addattr(gObjectClass, attr);
		}

static void
TitiTell(
	objTT800*	me,
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

static void TitiInfo(objTT800* me)
	{ LitterInfo(kClassName, &me->coreObject, (method) TitiTattle); }

static inline void AddInfo(void)
	{ LitterAddCant((method) TitiInfo, "info"); }
		
static void TitiTell(objTT800* me, Symbol* iTarget, Symbol* iMsg)
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
						A_DEFLONG,					// Optional argument:	1. Minimum
						A_DEFLONG,					//						2. Maximum
						A_DEFLONG,					//						3. Seed
						A_NOTHING
						};
	
	LITTER_CHECKTIMEOUT(kClassName);
	
	LitterSetupClass(	kClassName,
						sizeof(objTT800),				// Class object size
						LitterCalcOffset(objTT800),	// Magic "Obex" Calculation
						(method) TitiNew,				// Instance creation function
						(method) TitiFree,				// Custom deallocation function
						NIL,							// No menu function
						myArgTypes);					// See above
	

	// Messages
	LITTER_TIMEBOMB LitterAddBang	((method) TitiBang);
	LitterAddMess1	((method) TitiMin,		"in1",		A_LONG);
	LitterAddMess1	((method) TitiMax,		"in2",		A_LONG);
	LitterAddMess1	((method) TitiSeed,		"seed",		A_DEFLONG);
	LitterAddMess2	((method) TitiTell,		"tell",		A_SYM, A_SYM);
	LitterAddCant	((method) TitiTattle,	"dblclick");
	LitterAddMess0	((method) TitiTattle,	"tattle");
	LitterAddCant	((method) TitiAssist,	"assist");
	
	AddInfo();
	
	//Initialize Litter Library
	LitterInit(kClassName, 0);
	
	}

