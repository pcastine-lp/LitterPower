/*
	File:		dicey.c

	Contains:	Max external object simulating the toss of dice.

	Written by:	Peter Castine

	Copyright:	© 2001-2002 Peter Castine

	Change History (most recent first):

         <7>   23Ð3Ð2006    pc      Update #includes for new LitterLib organization. (Implementation
                                    of expect message postponed).
         <6>     15Ð1Ð04    pc      Avoid possible memory leak if seed argument is used.
         <5>     14Ð1Ð04    pc      Update for Windows.
         <4>    7Ð7Ð2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <3>  30Ð12Ð2002    pc      Drop faux 'Vers' resource and related modifications. Add object
                                    version to DoInfo().
         <2>  28Ð11Ð2002    pc      Tidy up after initial check in.
         <1>  28Ð11Ð2002    pc      Initial check in.
		30-Jun-2001:	Modified to use Taus88 instead of TT800 as default generator.
						Also update to use new ULong2Unitxx() functions.
		14-Apr-2001:	First implementation.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark ¥ Include Files

#include "LitterLib.h"
#include "TrialPeriodUtils.h"
#include "Taus88.h"
#include "MiscUtils.h"
#include "UniformExpectations.h"


#pragma mark ¥ Constants

const char*	kClassName		= "lp.dicey";			// Class name
const char*	kPackageName	= "lp.dicey Package";	// Package name for 'vers'(2) resource

const long	kDefNDice		= 2,
			kDefNFaces		= 6,
			kMinFaces		= 1;			// How can a die have one face?
											// Don't ask me; ask a topologist.

	// Indices for STR# resource
enum {
	strIndexInBang		= lpStrIndexLastStandard + 1,
	strIndexInNDice,
	strIndexInNFaces,
	
	strIndexOutRange,						// Normal assist string
	strIndexOutNoDice,						// Assist string if nDice == 0
	strIndexOutNoChance,					// Assist string if nFaces == 1
	strIndexOutOverflow						// Assist string if nDice * nFaces > kLongMax
	};


#pragma mark ¥ Type Definitions



#pragma mark ¥ Object Structure

typedef struct {
	LITTER_CORE_OBJECT(Object, coreObject);
	
	tTaus88DataPtr	tausData;
	
	long			nDice,
					nFaces;
	} objDicey;


#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark ¥ Object Message Handlers

/******************************************************************************************
 *
 *	DiceyBang(me)
 *
 ******************************************************************************************/

static void
DiceyBang(
	objDicey* me)
	
	{
	long	c	= me->nDice,		// All of these are non-negative, but will not exceed
			f	= me->nFaces,		// kMaxLong, so don't need to declare unsigned.
			d;						// Initialize in if/else below
	
	
	if (f < 2) d = c;				// Shortcut for degenerate dice
	else {
		tTaus88DataPtr	tausData = me->tausData;
		double			ff		= f;				// Convert long to double now
		d = 0;
		while (c-- > 0) {
			// ULong2Unit_Zo returns value in range		[0.0 .. 1.0)
			// * ff				results in range			[0.0 .. ff)
			// + 1.0			results in range			[1.0 .. ff + 1.0)
			// Thus values are equally distributed over [1 .. 2), [2 .. 3), ..., [ff .. f+1)
			// After truncation to integer we have equiprobably distribution of 1, 2, .. f
			// QED
			d += ff * ULong2Unit_Zo(Taus88(tausData)) + 1.0;
			}
		}
	
	outlet_int(me->coreObject.o_outlet, d);
	
	}


/******************************************************************************************
 *
 *	DiceyNFaces(me, iProb)
 *	DiceyNDice(me, iNTrials)
 *	DiceySeed(me, iSeed)
 *	
 *	Set parameters. Make sure nothing bad is happening.
 *	
 ******************************************************************************************/

	// Number of dice must be non-negative
static void DiceyNDice(objDicey* me, long iNDice)
	{ me->nDice = (iNDice > 0) ? iNDice : 0; }

	// Number of faces must be >= 1 (and nFaces == 1 is pretty degenerate)
static void DiceyNFaces(objDicey* me, long iNFaces)
	{ me->nFaces = (iNFaces >=  kMinFaces) ? iNFaces : kMinFaces; }

static void DiceySeed(objDicey* me, long iSeed)
	{ if (me->tausData) Taus88Seed(me->tausData, (unsigned long) iSeed); }

#pragma mark -
#pragma mark ¥ Class Message Handlers

/******************************************************************************************
 *
 *	DiceyNew(iNDice, iNFaces, iSeed)
 *	DiceyFree(me)
 *
 ******************************************************************************************/

static void*
DiceyNew(
	long	iNDice,
	long	iNFaces,
	long	iSeed)
	
	{
	objDicey*			me				= NIL;
	tTaus88DataPtr	myTaus88Stuff	= NIL;
	
	// Run through initialization parameters from right to left, handling defaults
	if (iSeed != 0) {
		myTaus88Stuff = Taus88New(iSeed);
		goto noMoreDefaults;
		}
	
	if (iNFaces == 0)
		iNFaces = kDefNFaces;
	else goto noMoreDefaults;
	
	if (iNDice == 0)
		iNDice = kDefNDice;
noMoreDefaults:
	// Finished checking intialization parameters

	// Let Max allocate us, our inlets, and outlets
	me = (objDicey*) LitterAllocateObject();

	
	intin(me, 2);										// Number of faces
	intin(me, 1);										// Number of dice
	
	intout(me);					// Access main outlet through me->coreObject.o_outlet;
	
	// Initialize object components
	me->tausData	= myTaus88Stuff;
	DiceyNDice(me, iNDice);
	DiceyNFaces(me, iNFaces);
	
	return me;
	}

static void DiceyFree(objDicey* me)
	{ Taus88Free(me->tausData); }							// Taus88Free is NIL-safe

#pragma mark -
#pragma mark ¥ Attribute/Information Functions

/******************************************************************************************
 *
 *	DiceyTattle(me)
 *	DiceyAssist()
 *
 ******************************************************************************************/

	static inline Boolean CanOverflow(objDicey* me)
		{ return ((double) me->nDice) * ((double) me->nFaces) > ((double) kLongMax); }

static void
DiceyTattle(
	objDicey* me)
	
	{
	
	post("%s state",
			kClassName);
	post("  Throwing %ld dice with %ld faces each",
			me->nDice,
			me->nFaces);
	if ( CanOverflow(me) ) {
		post("  --> These parameters may cause arithmetic overflow.");
		if (Taus88(me->tausData) < 0)
				post("      I don't mind, but I thought you might like to know.");
		else	post("      Have you thought about using a Normal (Gauss) Distribution?");
		}
	
	}

static void
DiceyAssist(
	objDicey*	me,
	void*	box,
	long	iDir,
	long	iArgNum,
	char*	oCStr)
	
	{
	#pragma unused(box)
	
	short	outletString;
	long	min	= me->nDice,
			max	= min * me->nFaces;
	
	
	
		// Check for various "special" cases
	if (min == 0)				outletString = strIndexOutNoDice;
	else if (max == min)		outletString = strIndexOutNoChance;
	else if ( CanOverflow(me) )	outletString = strIndexOutOverflow;
	else						outletString = strIndexOutRange;
	
	LitterAssistVA(iDir, iArgNum, strIndexInBang, outletString, oCStr, min, max);
	
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
	objDicey*		me,
	eExpectSelector iSel)
	
	{
	long	n		= me->nDice;
	double	result	= UniformExpectationsDiscrete(1, me->nFaces, iSel);
	
	switch(iSel) {
	default:
		result *= n;				// Holds for all selectors except the following
		break;						// exceptions...
		
	case expMode:
		if  (n > 1)
			result = DoExpect(me, expMean);
		break;
	case expStdDev:
		result *= sqrt(n);
		break;
	case expKurtosis:
		result /= (n * n);
		break;
	case expEntropy:
		result += log2(n);
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

	static t_max_err DiceyGetMin(objDicey* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expMin), ioArgC, ioArgV);
		}
	static t_max_err DiceyGetMax(objDicey* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expMax), ioArgC, ioArgV);
		}
	static t_max_err DiceyGetMean(objDicey* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expMean), ioArgC, ioArgV);
		}
	static t_max_err DiceyGetMedian(objDicey* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expMedian), ioArgC, ioArgV);
		}
	static t_max_err DiceyGetMode(objDicey* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expMode), ioArgC, ioArgV);
		}
	static t_max_err DiceyGetVar(objDicey* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expVar), ioArgC, ioArgV);
		}
	static t_max_err DiceyGetStdDev(objDicey* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expStdDev), ioArgC, ioArgV);
		}
	static t_max_err DiceyGetSkew(objDicey* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expSkew), ioArgC, ioArgV);
		}
	static t_max_err DiceyGetKurtosis(objDicey* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expKurtosis), ioArgC, ioArgV);
		}
	static t_max_err DiceyGetEntropy(objDicey* me, void* iAttr, long* ioArgC, Atom** ioArgV)
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
		attr = attr_offset_new("dice", symLong, 0, NULL, NULL, calcoffset(objDicey, nDice));
		class_addattr(gObjectClass, attr);
		attr = attr_offset_new("faces", symLong, 0, NULL, NULL, calcoffset(objDicey, nFaces));
		class_addattr(gObjectClass, attr);
		
		// Read-Only Attributes
		attr = attribute_new("min", symLong, kAttrFlagsReadOnly, (method) DiceyGetMin, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("max", symLong, kAttrFlagsReadOnly, (method) DiceyGetMax, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("mean", symFloat64, kAttrFlagsReadOnly, (method) DiceyGetMean, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("median", symFloat64, kAttrFlagsReadOnly, (method) DiceyGetMedian, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("mode", symFloat64, kAttrFlagsReadOnly, (method) DiceyGetMode, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("var", symFloat64, kAttrFlagsReadOnly, (method) DiceyGetVar, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("stddev", symFloat64, kAttrFlagsReadOnly, (method) DiceyGetStdDev, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("skew", symFloat64, kAttrFlagsReadOnly, (method) DiceyGetSkew, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("kurtosis", symFloat64, kAttrFlagsReadOnly, (method) DiceyGetKurtosis, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("entropy", symFloat64, kAttrFlagsReadOnly, (method) DiceyGetEntropy, NULL);
		class_addattr(gObjectClass, attr);
		}

static void
DiceyTell(
	objDicey*	me,
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

static void DiceyInfo(objDicey* me)
	{ LitterInfo(kClassName, &me->coreObject, (method) DiceyTattle); }

static inline void AddInfo(void)
	{ LitterAddCant((method) DiceyInfo, "info"); }
		
static void DiceyTell(objDicey* me, Symbol* iTarget, Symbol* iMsg)
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
						A_DEFLONG,		// Number of dice of trials		(Default: 1)
						A_DEFLONG,		// Number of faces per die		(Default: 6)
						A_DEFLONG,		// Seed						(Default: Zero -- autoseed)
						A_NOTHING
						};
	
	LITTER_CHECKTIMEOUT(kClassName);
	
	// Standard Max setup() call
	LitterSetupClass(	kClassName,
						sizeof(objDicey),		// Class object size
						LitterCalcOffset(objDicey),	// Magic "Obex" Calculation
						(method) DiceyNew,			// Instance creation function
						(method) DiceyFree,			// Custom deallocation function
						NIL,						// No menu function
						myArgTypes);				// See above
	

	// Messages
	LITTER_TIMEBOMB LitterAddBang	((method) DiceyBang);
	LitterAddMess1	((method) DiceyNDice,	"in1",		A_LONG);
	LitterAddMess1	((method) DiceyNFaces,	"in2",		A_LONG);
	LitterAddMess1	((method) DiceySeed,	"seed",		A_DEFLONG);
	LitterAddMess2	((method) DiceyTell,	"tell",		A_SYM, A_SYM);
	LitterAddMess0	((method) DiceyTattle,	"tattle");
	LitterAddCant	((method) DiceyTattle,	"dblclick");
	LitterAddCant	((method) DiceyAssist,	"assist");
	
	AddInfo();
	
	//Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();
	
	}



