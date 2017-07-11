/*
	File:		alea.c

	Contains:	Litter replacement for the RTC alea abstraction.

	Written by:	Peter Castine

	Copyright:	© 2006 Peter Castine

	Change History (most recent first):

*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "LitterLib.h"
#include "Taus88.h"

#if 0
	// Temporarily disable the trial period stuff while we're on a machine that doesn't
	// have the nifty header file installed
	#include "TrialPeriodUtils.h"
#else
	#define LITTER_CHECKTIMEOUT(X)
	#define LITTER_TIMEBOMB
#endif

#pragma mark • Constants

const char*	kClassName		= "lp.ale";			// Class name

	// Have to use enum because GCC and some other compilers do not consider const int
	// to be a constant expression.
enum {
	kMaxListLen			= 255
	};


	// Indices for STR# resource
enum {
	strIndexLeftIn			= lpStrIndexLastStandard + 1,
	strIndexLeftOut
	};


#pragma mark • Type Definitions



#pragma mark • Object Structure

typedef struct {
	Object			coreObject;
	
	tOutletPtr		outletReset;
	
	tTaus88DataPtr	tausData;
	
	Atom			items[kMaxListLen];
	short			itemCount,
					itemsLeft;
	
	} objAle;


#pragma mark • Global Variables

#pragma mark • Function Prototypes

	// Class message functions
objAle*		AleNew(long);
void		AleFree(objAle*);

	// Object message functions
static void AleBang(objAle*);
static void AleInt(objAle*, long);
static void AleFloat(objAle*, double);
static void AleList(objAle*, Symbol*, short, Atom[]);
static void AleSet(objAle*, Symbol*, short, Atom[]);
static void AleAnything(objAle*, Symbol*, short, Atom[]);
static void AleTattle(objAle*);
static void	AleAssist(objAle*, void*, long, long, char*);
static void	AleInfo(objAle*);

#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

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
	setup(	&gObjectClass,					// Pointer to our class definition
			(method) AleNew,				// Instantiation method  
			(method) AleFree,				// Custom deallocation function
			(short) sizeof(objAle),			// Class object size
			NIL,							// No menu function
			A_DEFLONG,						// If non-zero initialize & maintain a
			0);								// private seed pool
	
	// Messages
	LITTER_TIMEBOMB addbang	((method) AleBang);
	LITTER_TIMEBOMB addint	((method) AleInt);
	LITTER_TIMEBOMB addfloat((method) AleFloat);
	LITTER_TIMEBOMB addmess	((method) AleList,		"list", A_GIMME, 0);
	LITTER_TIMEBOMB addmess	((method) AleAnything,	"anything", A_GIMME, 0);
	addmess	((method) AleSet,		"set",			A_GIMME, 0);
	addmess	((method) AleAssist,	"assist",		A_CANT, 0);
	addmess	((method) AleInfo,		"info",			A_CANT, 0);
	addmess ((method) AleTattle,	"tattle",	 	A_NOTHING);
	addmess ((method) AleTattle,	"dblclick", 	A_CANT, 0);
	
	// Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();
	
	}

#pragma mark -
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	AleNew(iSym, iArgCount, iArgVec)
 *	AleFree(me)
 *
 ******************************************************************************************/

objAle*
AleNew(
	long iSeed)
	
	{
	objAle*	me = NIL;
	int				i;
	
	// Create object
	me = (objAle*) newobject(gObjectClass);
		if (me == NIL) goto punt;			// Poor man's exception handling
	
	// Add inlets and outlets
	me->outletReset = bangout(me);
	outlet_new(me, NIL);					// Access through me->coreObject.o_outlet
	
	// Initialize object members
	me->tausData	= NIL;					// Try to allocate later, if requested by user
	me->itemCount	= 0;
	me->itemsLeft	= 0;
	
	// Slightly anal-retentive, but safety first
	for (i = 0; i < kMaxListLen; i += 1) {
		me->items[i].a_type		= A_NOTHING;
		me->items[i].a_w.w_long	= 0;
		}
	
	// Now that we're squeaky clean, check if we need to allocate our own seed base
	if (iSeed != 0)
		me->tausData = Taus88New(iSeed);
	
punt:
	return me;
	}

void AleFree(objAle* me)
	{
	Taus88Free(me->tausData);		// Taus88Free() is null-safe
	}

#pragma mark -
#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	AleBang(me)
 *	AleSet(me, sym, iArgCount, iArgVec)
 *
 *	AleInt(me, iVal)
 *	AleFloat(me, iVal)
 *	AleList(me, sym, iArgCount, iArgVec)
 *	
 ******************************************************************************************/

void
AleBang(
	objAle* me)
	
	{
	UInt32	itemsLeft,
			chosenOne;
	
	if (me->itemCount <= 0)
		return;
	
	itemsLeft = me->itemsLeft;
	if (itemsLeft <= 0) {
		itemsLeft = me->itemCount;
		outlet_bang(me->outletReset);
		}
	 
	 chosenOne = (itemsLeft > 1)
					? ((Taus88(me->tausData) & 0x0000ffff) * itemsLeft) >> 16
					: 0;
	 	// ASSERT: 0 <= chosenOne < itemsLeft <= me->itemCount
	
	switch (me->items[chosenOne].a_type) {
	case A_LONG:
		outlet_int(me->coreObject.o_outlet, me->items[chosenOne].a_w.w_long);
		break;
	case A_FLOAT:
		outlet_float(me->coreObject.o_outlet, me->items[chosenOne].a_w.w_float);
		break;
	case A_SYM:
		outlet_anything(me->coreObject.o_outlet, me->items[chosenOne].a_w.w_sym, 0, NIL);
		break;
	default:
		// Can't happen, nothing to do
		break;
		}
	
	// Put the chosen one one the side so it doesn't get used again in this cycle
	me->itemsLeft = itemsLeft -= 1;
	if (chosenOne < itemsLeft)
		AtomSwap(me->items + chosenOne, me->items + itemsLeft);
	
	}

void
AleSet(
	objAle*	me,
	Symbol*			sym,					// Not used, must be gensym("set")
	short			iArgCount,
	Atom			iArgVec[])
	
	{
	#pragma unused(sym)	
	
	int i;
	
	// Just in case some non-standard object sends a list longer than the standard max
	if (iArgCount > kMaxListLen) {
		iArgCount = kMaxListLen;
		error("%s: items truncated", kClassName);
		}
	
	for (i = 0; i < iArgCount; i += 1)
		me->items[i] = iArgVec[i];
	
	me->itemCount = iArgCount;
	me->itemsLeft = iArgCount;
	
	}


void AleInt(objAle* me, long iVal)
	{
	Atom a;
	
	AtomSetLong(&a, iVal);
	
	AleSet(me, NIL, 1, &a);
	AleBang(me);
	
	}

void AleFloat(objAle* me, double iVal)
	{
	Atom a;
	
	AtomSetFloat(&a, iVal);
	
	AleSet(me, NIL, 1, &a);
	AleBang(me);
	
	}
	

void
AleList(
	objAle*	me,
	Symbol*			sym,										// unused, must be "list"
	short			iArgCount,
	Atom			iArgVec[])
	
	{
	#pragma unused(sym)
	
	AleSet(me, NIL, iArgCount, iArgVec);
	AleBang(me);
	
	}

/******************************************************************************************
 *
 *	AleAnything(me)
 *
 *	Sort of special case: we treat arbitrary messages with parameter lists as if they were
 *	a list beginning with a symbol.
 *
 ******************************************************************************************/

void
AleAnything(
	objAle*	me,
	Symbol*			iSym,
	short			iArgCount,
	Atom			iArgVec[])
	
	{
	int i;
	
	// Just in case some non-standard object sends a list longer than the standard max
	if (iArgCount >= kMaxListLen) {
		iArgCount = kMaxListLen - 1;
		error("%s: items truncated", kClassName);
		}
	
	AtomSetSym(&me->items[0], iSym);
	
	for (i = 0; i < iArgCount; i += 1)
		me->items[i+1] = iArgVec[i];
	
	me->itemCount = iArgCount + 1;
	me->itemsLeft = me->itemCount;
	
	AleBang(me);
	
	}




/******************************************************************************************
 *
 *	AleTattle(me)
 *
 * 	Tell all we know.
 *
 ******************************************************************************************/

void
AleTattle(
	objAle*	me)
	
	{
	
	// Object state
	post("%s status", kClassName);
	
	// ... and then stuff specific to Ale
	post("  chosing from %ld items", (long) me->itemCount);
	post("  %ld items left in pool", (long) me->itemsLeft);
	
	}

/******************************************************************************************
 *
 *	AleAssist(me, iBox, iDir, iArgNum, oCStr)
 *	AleInfo(me)
 *
 ******************************************************************************************/

void
AleAssist(
	objAle*	me,
	tBoxPtr			box,				// unused
	long			iDir,
	long			iArgNum,
	char*			oCStr)
	
	{
	#pragma unused(me, box)
	
	LitterAssist(iDir, iArgNum, strIndexLeftIn, strIndexLeftOut, oCStr);
	
	}

void AleInfo(objAle* me)
	{ LitterInfo(kClassName, (tObjectPtr) me, (method) AleTattle); }


