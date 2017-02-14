/*
	File:		kg.c

	Contains:	Max external object mapping I Ching hexagrams to a range of other than
				64 items, based on methods used by John Cage

	Written by:	Peter Castine

	Copyright:	© 2001-2002 Peter Castine

	Change History (most recent first):

         <9>   26Ð4Ð2006    pc      Update for new LitterLib organization.
         <8>     26Ð2Ð05    pc      Finally kill all references to the legacy <SetUpA4.h> and
                                    <A4Stuff.h> stuff. 
                                    <A4Stuff.h> stuff. 
         <7>     11Ð1Ð04    pc      Update for modified LitterInit()
         <6>      8Ð1Ð04    pc      Update for Windows.
         <5>    6Ð7Ð2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <4>  30Ð12Ð2002    pc      Add object version to KgInfo()
         <3>  30Ð12Ð2002    pc      Use 'STR#' resource instead of faux 'Vers' resource for storing
                                    version information used at run-time.
         <2>  28Ð11Ð2002    pc      Tidy up after initial check in.
         <1>  28Ð11Ð2002    pc      Initial check in.
		10-Jul-2001:	First implementation
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark ¥ Include Files

#include "IChingCore.h"							// #includes LitterLib.h
#include "TrialPeriodUtils.h"
#include "Taus88.h"


#pragma mark ¥ Constants

const char*	kClassName		= "lp.kg";			// Class name

	// Indices for STR# resource
enum {
	strIndexInPresent		= lpStrIndexLastStandard + 1,
	strIndexInFuture,
	
	strIndexOutPresent,
	strIndexOutFuture
	};

	// This is pretty big while allowing use of a reasonably compact representation
#ifdef __GNUC__
	// ...but this is a lame way of having to do it
	#define		kMaxItems	256
#else
	const int kMaxItems	= 256;
#endif

#pragma mark ¥ Type Definitions


#pragma mark ¥ Object Structure

typedef struct {
	Object	coreObject;
	
	voidPtr	futureOut;					// Main outlet accessed through coreObject.o_outlet
	
	int		future,						// 1 <= future <= 64
			items;						// 0 <= items <= kMaxItems
	Byte	mainThresh[kMaxItems],
			futureThresh[kMaxItems];
	
	Boolean	doYarrow;
	} tChingMap;


#pragma mark ¥ Global Variables

#pragma mark ¥ Function Prototypes

	// Class message functions
static void*	KgNew	(long, Symbol*);

	// Object message functions
static void		KgMainIn	(tChingMap*, long);
static void		KgFutureIn	(tChingMap*, long);
static void		KgBang		(tChingMap*);
static void		KgYarrow	(tChingMap*);
static void		KgCoin		(tChingMap*);
static void		KgSize		(tChingMap*, long);

	// Object information
static void		KgTattle	(tChingMap*);
static void		KgAssist	(tChingMap*, tBoxPtr , long , long , char*);
static void		KgInfo		(tChingMap*);

#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark ¥ Inline Functions

#pragma mark -

/******************************************************************************************
 *
 *	main()
 *	
 *	Standar Max External Object Entry Point Function
 *	
 ******************************************************************************************/

void
main()
	
	{
	LITTER_CHECKTIMEOUT(kClassName);
	
	// Standard Max setup() call
	setup(	&gObjectClass,				// Pointer to our class definition
			(method) KgNew,					// Instance creation function
			NIL,							// No custom deallocation function
			sizeof(tChingMap),				// Class object size
			NIL,							// No menu function
			A_DEFLONG,						// Two arguments:	1. # of items
			A_DEFSYM,						// 					2. coin/yarrow
			0);		
	
	// Messages
	LITTER_TIMEBOMB addbang	((method) KgBang);
	LITTER_TIMEBOMB addint	((method) KgMainIn);
	addinx	((method) KgFutureIn, 1);
	addmess ((method) KgYarrow,		"yarrow",	A_NOTHING);
	addmess ((method) KgCoin,		"coin",		A_NOTHING);
	addmess ((method) KgSize,		"size",		A_LONG, 0);
	addmess	((method) KgTattle,		"dblclick",	A_CANT, 0); 
	addmess	((method) KgTattle,		"tattle",	A_NOTHING); 
	addmess	((method) KgAssist,		"assist",	A_CANT, 0);
	addmess	((method) KgInfo,		"info",		A_CANT, 0);
	
	// Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();

	}


#pragma mark -
#pragma mark ¥ Helper Functions

/******************************************************************************************
 *
 *	SetThreshholds(me)
 *
 ******************************************************************************************/

static void
SetThreshholds(
	tChingMap* me)
	
	{
	tOracle		theOracle;
	ChingFunc	theFunc = me->doYarrow ? ThrowSticks : TossCoins;
	
	BytePtr		curItemMain,
				curItemFuture;
	int			i,
				mainSum,
				futureSum;
	double		mainFactor,
				futureFactor,
				mainThresh,
				futureThresh;
	
	
	// 1: Choose weights
	curItemMain		= me->mainThresh;
	curItemFuture	= me->futureThresh;
	mainSum			= 0,
	futureSum		= 0;
	for (i = 1; i <= me->items; i += 1) {		// Rely on compiler to optimize loop
												// condition, which is not particularly
												// efficient.
		theFunc(&theOracle);
		mainSum		+= *curItemMain++
					 = theOracle.mainHexagram;
		futureSum	+= *curItemFuture++
					 = theOracle.futureHexagram;
		}
	
	// 2: Set weight factors
	mainFactor		= 64.0 / (double) mainSum;
	futureFactor	= 64.0 / (double) futureSum;
	
	// 3: Set threshholds according to weight
	curItemMain		= me->mainThresh;
	curItemFuture	= me->futureThresh;
	mainThresh		= 0.0;						// Use double to avoid truncation errors.
	futureThresh	= 0.0;						// And despite double prec. we hard-wire
	for (i = 1; i < me->items; i += 1) {			// the final value to be safe.
		mainThresh			+= *curItemMain * mainFactor;
		futureThresh		+= *curItemFuture * futureFactor;
		*curItemMain++		= mainThresh + 0.5;
		*curItemFuture++	= futureThresh + 0.5;
		}
	*curItemMain	= 64;
	*curItemFuture	= 64;
	
	//
	// SIDE EFFECT!!!
	//
	me->future = 0;
	
	}


#pragma mark -
#pragma mark ¥ Class Message Handlers

/******************************************************************************************
 *
 *	KgNew(iOracle)
 *
 ******************************************************************************************/

void*
KgNew(
	long	iSize,
	Symbol* iMethod)
	
	{
	const int	kDefSize	= 2;
	
	tChingMap*	me			= NIL;
	
	
	// Check up on default size.
	// The symbol argument is handled a bit cheasily in the initialization below 
	if (iSize <= 0)
		iSize = kDefSize;
	else if (iSize > kMaxItems)
		iSize = kMaxItems;
	
	// Let Max allocate the object, its outlets, and inlets.
	me = (tChingMap*) newobject(gObjectClass);
		if (me == NIL) goto punt;
	
	me->futureOut = intout(me);
	intout(me);									// Access through me->coreObject.o_outlet
	
	intin(me, 1);
	
	
	//
	// Initialize components
	//
	
	me->doYarrow = iMethod == gensym("yarrow");
	KgSize(me, iSize);							// This also initializes the threshhold
												// arrays, as well as resetting any
												// pending hexagrams
	
	//
	// All done
	//
	
	return me;

	// Poor man's exception handling
punt:
	if (me != NIL)
		freeobject((Object*) me);
	
	return NIL;
	}

#pragma mark -
#pragma mark ¥ Object Message Handlers

/******************************************************************************************
 *
 *	KgMainIn(me, iHex)
 *	KgFutureIn(me, iHex)
 *
 ******************************************************************************************/

void KgMainIn(
	tChingMap*	me,
	long		iHex)
	
	{
	
	if (0 < iHex && iHex <= 64)	{
		long	future;
		BytePtr	curThresh;
		
		future	= me->future;
		if (future == 0)
			future = iHex;
		
		// Calculate mapping of main hexagram
		curThresh = me->mainThresh;
		while (*curThresh++ < iHex)
			{ /* Do nothing; condition does the work */ }
		iHex = curThresh - me->mainThresh;
		
		// Do same thing with future hexagram
		curThresh = me->futureThresh;
		while (*curThresh++ < future)
			{ /* Do nothing; condition does the work */ }
		future = curThresh - me->futureThresh;
		
		// Output values	
		outlet_int(me->futureOut, future);
		outlet_int(me->coreObject.o_outlet, iHex);
		}
	
	// Whatever happens, clear old future input
	me->future = 0;
	
	}

void KgFutureIn(
	tChingMap*	me,
	long		iHex)
	
	{
	
	if (0 < iHex && iHex <= 64)
		me->future = iHex;
	
	}

/******************************************************************************************
 *
 *	KgBang(me)
 *	KgCoin(me)
 *	KgYarrow(me)
 *	KgSize(me, iSize)
 *
 ******************************************************************************************/

void KgBang(tChingMap* me)
	{ SetThreshholds(me); }

void KgCoin(tChingMap* me)
	{ me->doYarrow = false; SetThreshholds(me); }

void KgYarrow(tChingMap* me)
	{ me->doYarrow = true; SetThreshholds(me); }

void KgSize(tChingMap* me, long iSize)
	
	{
	if (0 < iSize && iSize <= kMaxItems) {
		me->items = iSize;
		SetThreshholds(me);
		}
	}


/******************************************************************************************
 *
 *	KgTattle(me)
 *
 ******************************************************************************************/

void
KgTattle(
	tChingMap* me)
	
	{
	int		i,
			items = me->items;
	BytePtr	curThresh;
	Atom	ant;
	
	post("%s state:", (char*) kClassName);
	post("  Method: %s", me->doYarrow ? "yarrow sticks" : "coins");
	post("  Number of items: %ld", items);
	post("  Main hexagram threshholds:");
	curThresh = me->mainThresh;
	for (i = items; i > 0; i -= 1) {
		SetLong(&ant, *curThresh++);
		postatom(&ant);
		}
	post("  Future hexagram threshholds:");
	curThresh = me->futureThresh;
	for (i = items; i > 0; i -= 1) {
		SetLong(&ant, *curThresh++);
		postatom(&ant);
		}
	
	}

/******************************************************************************************
 *
 *	KgAssist
 *	KgInfo(me)
 *
 ******************************************************************************************/

void
KgAssist(
	tChingMap*	me,
	tBoxPtr		box,
	long		iDir,
	long		iArgNum,
	char		oCDestStr[])
	
	{
	#pragma unused(me, box)
	
	LitterAssist(iDir, iArgNum, strIndexInPresent, strIndexOutPresent, oCDestStr);
	}

void KgInfo(tChingMap* me)
	{ LitterInfo(kClassName, &me->coreObject, (method) KgTattle); }


