/*
	File:		ginger.c

	Contains:	Max external object implementing I Ching coin and yarrow stick tosses.

	Written by:	Peter Castine

	Copyright:	© 2001-2002 Peter Castine

	Change History (most recent first):

        <11>     15–3–06    pc      Now need to #include "Taus88.h" because it has been split off
                                    from the main LitterLib.
        <10>     10–1–06    pc      Updated to use LitterAssistResFrag(). Windows uses more
                                    appropriate MSL version.
         <9>      5–3–05    pc      Finally get rid of the legacy SetUpA4 stuff from 68k days.
         <8>     14–1–04    pc      Fix change to constant kMaxResourceStrSize
         <7>     14–1–04    pc      Update for Windows
         <6>    7–7–2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <5>  30–12–2002    pc      Drop faux 'Vers' resource and related modifications. Add object
                                    version to DoInfo().
         <4>  29–12–2002    pc      Move our private P2CStr implementation (for use in real Carbon)
                                    to MaxLib.c
         <3>  29–12–2002    pc      Implement P2CStr() with Carbon's CopyPascalStringToC().
         <2>  28–11–2002    pc      Tidy up after initial check in.
         <1>  28–11–2002    pc      Initial check in.
		10-Jul-2001:	Moved core I Ching calculations to IChingCore.c in order to share
						the code with the lp.kg external.
		12-Apr-2001:	Renamde again to follow _new_ Litter conventions.
		31-Mar-2001:	Renamed to iching (following old Litter conventions)
		27-Mar-2001:	First implementation
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "IChingCore.h"
#include "TrialPeriodUtils.h"
#include "Taus88.h"


#pragma mark • Constants

const char*	kClassName		= "lp.ginger";			// Class name


	// Indices for STR# resource
enum {
	strIndexTheInlet		= lpStrIndexLastStandard + 1,
	
	strIndexOutPresent,
	strIndexOutFuture,
	strIndexOutDetails,
	
	strIndexFragCoin,
	strIndexFragYarrow
	};


#pragma mark • Type Definitions



#pragma mark • Object Structure

typedef struct {
	LITTER_CORE_OBJECT(Object, coreObject);
	
	voidPtr		futureOut,		// Outlets (main "present" outlet accessed through coreObject)
				oracleOut,
				detailOut;
	
	Atom		oracle[kMaxOracleLen],
				details[kDetailLen];
	
	eYinYang	type;
	} objIChing;


#pragma mark • Function Prototypes

	// Class message functions
//void*	GingerNew(Symbol*);

	// Object message functions
//static void GingerBang	(objIChing*);
static void GingerSet	(objIChing*, Symbol*);
//static void GingerYarrow(objIChing*);
//static void GingerCoin	(objIChing*);
//static void GingerZen	(objIChing*);
//static void	GingerAssist(objIChing*, void* , long , long , char*);
//static void	GingerInfo	(objIChing*);



#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	GingerNew(iOracle)
 *
 ******************************************************************************************/

static void*
GingerNew(
	Symbol* iOracle)
	
	{
	objIChing*	me			= NIL;
	
	
	// Let Max allocate us. iching has only the default inlet
	me = (objIChing*) LitterAllocateObject();

	
	// Add three outlets, from right to left
	me->detailOut	= listout(me);
	me->futureOut	= intout(me);
	intout(me);						// Accessed through me->coreObject.o_outlet
	
	// Set up initial oracle method according to initialization parameter
	if (iOracle == gensym(""))
			me->type = yyDefault;
	else	GingerSet(me, iOracle);
	
	return me;
	}

#pragma mark -
#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	GingerBang(me)
 *
 ******************************************************************************************/

static void
GingerBang(
	objIChing* me)
	
	{
	tOracle		theOracle;
	unsigned	lineIndex;
	tLinePtr	curLine;
	AtomPtr		curDetailAtom;
					
	if (me->type == yyCoin)
			TossCoins(&theOracle);
	else	ThrowSticks(&theOracle);
	
	// Set up elements in details lists
	curDetailAtom = me->details;
	for (	lineIndex = 1, curLine = theOracle.lines;
			lineIndex <= kLineCount;
			lineIndex += 1, curLine += 1) {
		
		SetLong(curDetailAtom++, curLine->components[0]);
		SetLong(curDetailAtom++, curLine->components[1]);
		SetLong(curDetailAtom++, curLine->components[2]);
		}
	
	outlet_list(me->detailOut, NULL, kDetailLen, me->details);
	outlet_int(me->futureOut, theOracle.futureHexagram);
	outlet_int(me->coreObject.o_outlet,theOracle.mainHexagram);
	}


/******************************************************************************************
 *
 *	GingerSet(me, iSource)
 *
 ******************************************************************************************/

static void
GingerSet(
	objIChing*	me,
	Symbol*		iOracle)
	
	{
	
	if (iOracle == gensym("coin")) {
		me->type = yyCoin;
		}
	else if (iOracle == gensym("yarrow")) {
		me->type = yyYarrow;
		}
	else error("%s doesn't support an oracle type named \"%s\"",
				(char*) kClassName, iOracle->s_name);
	
	}


/******************************************************************************************
 *
 *	GingerCoin(me)
 *	GingerYarrow(me)
 *
 ******************************************************************************************/

static void GingerCoin(objIChing* me)
	{ me->type = yyCoin; GingerBang(me); }

static void GingerYarrow(objIChing* me)
	{ me->type = yyYarrow; GingerBang(me); }


/******************************************************************************************
 *
 *	GingerZen(me)
 *
 ******************************************************************************************/

static void
GingerZen(
	objIChing* me)
	
	{
	#pragma unused(me)
	
	const char*	kZenStrings[] = {
						"If something is boring after two minutes…",
						"…try it for four.",
						"If it is still boring, try it for eight.",
						"Then try it for sixteen, thirty-two, and so on.",
						"Eventually one discovers that it's not boring at all but very interesting.",
						"— John Cage, _Silence_"
						};
	const int	kLastZenString	= 5;
	
	static int	sZenCounter		= 0;
	static long	sNextZenTime	= 0;
				
	long		now;
	
	
	now = XObjTickCount();
	if (now > sNextZenTime) {
		// Display text while incrementing counter, etc.
		int i = sZenCounter++;
		if (i <= kLastZenString) post((char*) (kZenStrings[i]));
		sNextZenTime = now + sZenCounter * 7200;
		}
	
	}



/******************************************************************************************
 *
 *	GingerAssist
 *
 *	Fairly generic Assist Method
 *
 ******************************************************************************************/

static void
GingerAssist(
	objIChing*	me,
	void*		box,
	long		iDir,
	long		iArgNum,
	char*		oCStr)
	
	{
	#pragma unused(box)
	
	if (iDir == ASSIST_INLET)
		 LitterAssistResFrag(iDir, iArgNum, strIndexTheInlet, 0, oCStr,
							 (me->type == yyCoin) ? strIndexFragCoin : strIndexFragYarrow);
	else LitterAssist(iDir, iArgNum, 0, strIndexOutPresent, oCStr);
	
	}

#pragma mark -
#pragma mark • Attribute/Information Functions

/******************************************************************************************
 *
 *	Attribute Funtions
 *
 *	Functions for Inspector/Get Info... dialog
 *	
 ******************************************************************************************/

#if LITTER_USE_OBEX

static inline void AddInfo(void)
	{ ; }
		
#else

static void GingerInfo(objIChing* me)
	{ LitterInfo(kClassName, &me->coreObject, (method) GingerZen); }

static inline void AddInfo(void)
	{ LitterAddCant((method) GingerInfo, "info"); }
		
//static void GingerTell(objIChing* me, Symbol* iTarget, Symbol* iMsg)
//	{ LitterExpect((tExpectFunc) DoExpect, (Object*) me, iMsg, iTarget, TRUE); }


#endif

/******************************************************************************************
 *
 *	GingerInfo(me)
 *
 *	Fairly basic Info Method
 *
 ******************************************************************************************/


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
						A_DEFSYM,		// One argument to set coin vs. yarrow choices.
										// Default: coin
						A_NOTHING
						};
	
	LITTER_CHECKTIMEOUT(kClassName);
	
	// Standard Max setup() call
	LitterSetupClass(	kClassName,
						sizeof(objIChing),				// Class object size
						LitterCalcOffset(objIChing),	// Magic "Obex" Calculations
						(method) GingerNew,				// Instance creation function
						NIL,							// No custom deallocation function
						NIL,							// No menu function
						myArgTypes);					// See above
	
	// Messages
	LITTER_TIMEBOMB LitterAddBang	((method) GingerBang);
	LITTER_TIMEBOMB LitterAddMess0	((method) GingerYarrow,	"yarrow");
	LITTER_TIMEBOMB LitterAddMess0	((method) GingerCoin,	"coin");
	LitterAddMess1	((method) GingerSet,		"set",		A_SYM);
	LitterAddMess0	((method) GingerZen,		"zen");
	LitterAddCant	((method) GingerZen,		"dblclick"); 
	LitterAddCant	((method) GingerAssist,		"assist");
	
	//Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();
	
	}




