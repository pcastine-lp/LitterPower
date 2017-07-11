/*
	File:		cuthbert.c

	Contains:	Max external object generating random numbers without repetition of recent
				values.

	Written by:	Peter Castine

	Copyright:	© 2007 Peter Castine

	Change History (most recent first):

*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "LitterLib.h"
#include "TrialPeriodUtils.h"
#include "Taus88.h"
#include "MiscUtils.h"

#pragma mark • Constants

const char*		kClassName	= "lp.cuthbert";			// Class name

const long		kDefMemory	= 1;

	// Indices for STR# resource
enum {
	strIndexInBang		= lpStrIndexLastStandard + 1,
	strIndexInMemLen,

	strIndexOutCuthbert,
	
	strIndexInLeft		= strIndexInBang,
	strIndexOutLeft		= strIndexOutCuthbert
	};


#pragma mark Macros

	// Ugly macro to change BlockMoveData() to memcpy() on Windows.
	// I don't understand why BlockMoveData() isn't working on Windows,
	// but this change makes things work
#ifdef WIN_VERSION
	#define BlockMoveData(SRC, DEST, BYTES)		memcpy(DEST, SRC, BYTES)
#endif

#pragma mark • Type Definitions



#pragma mark • Object Structure

typedef struct {
	Object			coreObject;
	
	tTaus88DataPtr	tausData;
	
	long			valCount,		// Produce values from 0 <= x < valCount
					curMemLen,		// The last curMemLen values won't be repeated
					wantMemLen,		// The user may have asked for longer memory
									// than is possible with the current valCount
					memSize;		// Size of memory buffer, >= sizeof(long) * curMemLen
	long*			memory;			// Initial state of urn
	long*			state;						// Current state (after removing balls)
		// Cache total number of balls in both master and state vectors
	long			totalBalls,
					ballsInUrn;
	
	} tCuthbert;


#pragma mark • Global Variables



#pragma mark • Function Prototypes

	// Class message functions
void*	CuthbertNew	(long, long);
void	CuthbertFree	(tCuthbert*);

	// Object message functions
static void CuthbertBang	(tCuthbert*);
static void CuthbertTable	(tCuthbert*, Symbol*);
static void CuthbertSet	(tCuthbert*, Symbol*, short, Atom*);
static void CuthbertClear	(tCuthbert*);
static void CuthbertConst	(tCuthbert*, long);
static void CuthbertReset	(tCuthbert*);
static void CuthbertSize	(tCuthbert*, long);
static void CuthbertCount	(tCuthbert*, long);
static void CuthbertSeed	(tCuthbert*, long);
static void CuthbertTattle(tCuthbert*);
static void	CuthbertAssist(tCuthbert*, void* , long , long , char*);
static void	CuthbertInfo	(tCuthbert*);


#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark • Inline Functions



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
	LITTER_CHECKTIMEOUT(kClassName);
	
	// Standard Max setup() call
	setup(	&gObjectClass,				// Pointer to our class definition
			(method) CuthbertNew,			// Instance creation function
			(method) CuthbertFree,			// Deallocation function
			(short) sizeof(tCuthbert),		// Class object size
			NIL,						// No menu function
			A_DEFLONG,					// Optional arguments:	1: Data size
			A_DEFLONG,					// 						2: seed
			0);		
	

	// Messages
	LITTER_TIMEBOMB LitterAddBang	((method) CuthbertBang);
	addmess	((method) CuthbertTable,		"refer",	A_SYM, 0);
	addmess	((method) CuthbertSet,			"set",		A_GIMME, 0);
	addmess ((method) CuthbertSeed,		"seed",		A_DEFLONG, 0);
	addmess ((method) CuthbertClear,		"clear",	A_NOTHING);
	addmess ((method) CuthbertClear,		"zero",		A_NOTHING);
	addmess ((method) CuthbertConst,		"const",	A_DEFLONG, 0);
	addmess ((method) CuthbertReset,		"reset",	A_NOTHING);
	addmess	((method) CuthbertSize,		"size",		A_LONG, 0);
	addmess	((method) CuthbertCount,		"count",	A_DEFLONG, 0);
	addmess	((method) CuthbertTattle,		"dblclick",	A_CANT, 0);
	addmess	((method) CuthbertTattle,		"tattle",	A_NOTHING);
	addmess	((method) CuthbertAssist,		"assist",	A_CANT, 0);
	addmess	((method) CuthbertInfo,		"info",		A_CANT, 0);
	
	//Initialize Litter Library
	LitterInit(kClassName, 0);
	
	}



#pragma mark -
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	CuthbertNew(iOracle)
 *
 ******************************************************************************************/

void*
CuthbertNew(
	long	iDataSize,
	long	iSeed)
	
	{
	tCuthbert*			me				= NIL;
	tTaus88DataPtr	myTaus88Stuff	= NIL;
	
	
	// Run through initialization parameters from right to left, handling defaults
	if (iSeed != 0) {
		myTaus88Stuff = Taus88New(iSeed);
		goto noMoreDefaults;
		}
	
	if (iDataSize == 0)
		iDataSize = kDefMemory;
noMoreDefaults:
	// Finished checking intialization parameters

	// Let Max allocate us and our outlets. We only use the default inlet
	me = (tCuthbert*) LitterAllocateObject

	if (me == NIL) goto punt;
	
	me->resetOutlet	= bangout(me);
	intout(me);							// Access through me->coreObject.o_outlet
	
	// Initialize object components
	me->tausData	= myTaus88Stuff;
	me->dataSize	= 0;				// Start from clean slate before calling CuthbertSize()
	me->master		= NIL;
	me->state		= NIL;
	CuthbertSize(me, iDataSize);
	if (me->master == NIL || me->state == NIL) {
		goto punt;
		}
	CuthbertClear(me);
	
	return me;

	// We only get here if something bad happened
punt:
	if (me != NIL) {
		CuthbertFree(me);
		freeobject((t_object*) me);
		}
	
	return NIL;
	}

/******************************************************************************************
 *
 *	CuthbertFree(iOracle)
 *
 ******************************************************************************************/

	static void DeferDisposePtr(Ptr iPtr, Symbol* sym, short argc, Atom argv[])
		{
		#pragma unused(sym, argc, argv)
		
		DisposePtr(iPtr);
		}
	
	static void DisposeMemory(long* iMaster, long* iState, long memBytes)
		{
		
		// Non-zero memBytes indicates that we are using Max' own memory allocation
		if (memBytes > 0) {
			if (iMaster != NIL)	freebytes(iMaster, memBytes);
			if (iState != NIL)	freebytes(iState, memBytes);
			}
		
		// If we are using Mac OS memory allocation, remember that even though the memory
		// was allocated outside an interrupt, DisposeMemory() may be called inside an
		// interrupt
		else if (isr() != 0) {
			if (iMaster != NIL)	defer(iMaster, (method) DeferDisposePtr, NIL, 0, NIL);
			if (iState != NIL)	defer(iState, (method) DeferDisposePtr, NIL, 0, NIL);
			}
		
		else {
			if (iMaster != NIL)	DisposePtr((Ptr) iMaster);
			if (iState != NIL)	DisposePtr((Ptr) iState);
			}
		}

void
CuthbertFree(
	tCuthbert*	me)
	
	{
	
	Taus88Free(me->tausData);					// Taus88Free is NIL-safe
	
	DisposeMemory(me->master, me->state, me->maxMemory ? sizeof(long) * me->dataSize : 0);
	
	}

#pragma mark -
#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	CuthbertBang(me)
 *
 ******************************************************************************************/

void
CuthbertBang(
	tCuthbert* me)
	
	{
	long	theBall = -1;			// This is the result defined for the degenerate case
									// of "no balls in the system"; this value also allows
									// the main loop condition to be streamlined.
	
	
	if (me->totalBalls > 0) {
		long	magic;
		long*	sp;
		
		if (me->ballsInUrn == 0) {
			CuthbertReset(me);
			}
		
		// ULong2Unit_Zo returns value in range			[0.0 .. 1.0)
		// * b	  (b = ballsInUrn) results in range		[0.0 .. b)
		// + 1.0			results in range			[1.0 .. b + 1.0)
		// conversion to int results in range			[1 .. b] w/equiprob. for each value
		// QED
		magic = ((double) me->ballsInUrn) * ULong2Unit_Zo(Taus88(me->tausData)) + 1.0;
		
		// This initial value allows the loop below to increment the pointer as part of
		// the loop condition.
		sp = me->state - 1;
		
		// ASSERT: loop terminates for all values of magic in [1 .. b] due to
		// b == sum(sp[0] .. sp[dataSize-1])
		do {
			theBall += 1;
			} while ((magic -= *++sp) > 0);
		
		// Update state of urn
		*sp				-= 1;
		me->ballsInUrn	-= 1;
		}
	
	outlet_int(me->coreObject.o_outlet, theBall);
	
	}


/******************************************************************************************
 *
 *	CuthbertTable(me, iTable)
 *	CuthbertSet(me, iSym, iArgC, iArgV)
 *	CuthbertClear(me)
 *	CuthbertConst(me, iVal)
 *	CuthbertSize(me, iSize)
 *	CuthbertReset(me)
 *	
 *	Set parameters and data. Make sure nothing bad is happening.
 *	
 ******************************************************************************************/

	static void RecalcMasterTotal(tCuthbert* me)
		{
		unsigned	dataSize	= me->dataSize;
		long*		data		= me->master;
		long		total		= 0;
		
		while (dataSize-- > 0) {
			total += *(data++);
			}
		
		me->totalBalls = total;
		}

void
CuthbertTable(
	tCuthbert*	me,
	Symbol*	iTable)
	
	{
	long**	tableData;			// Isn't there a typedef that could be used???
	long	size;
	
	if (table_get(iTable, &tableData, &size) == 0) {
		if (size > me->dataSize) size = me->dataSize;	// Don't read more than we can
		else if (size < me->dataSize) {
			// Cheesy way to zero out values not specified in the table
			me->dataSize	-= size;
			me->master		+= size;
			me->state		+= size;
			CuthbertConst(me, 0);
			me->dataSize	+= size;
			me->master		-= size;
			me->state		-= size;
			}
		
		BlockMoveData(*tableData, me->master, size * sizeof(long));
		
		RecalcMasterTotal(me);
		CuthbertReset(me);
		}
	
	else error("%s: can't find table named %s", kClassName, iTable->s_name);
	
	}
	
void
CuthbertSet(
	tCuthbert*	me,
	Symbol*	sym,				// Is always gensym("set")
	short	iArgC,
	Atom*	iArgV)
	
	{
	#pragma unused(sym)
	
	long*	dataPtr = me->master;
	long	total	= 0;
	// Nothing Enter/ExitCallback()-sensitive here;
	
	if (iArgC > me->dataSize)	iArgC = me->dataSize;
	else if (iArgC < me->dataSize) {
		// Cheesy way to zero out values not specified in the Atom vector
		me->dataSize	-= iArgC;
		me->master		+= iArgC;
		me->state		+= iArgC;
		CuthbertConst(me, 0);
		me->dataSize	+= iArgC;
		me->master		-= iArgC;
		me->state		-= iArgC;
		}
	while (iArgC-- > 0) {
		long	val;
		switch (iArgV->a_type) {
			case A_LONG:
				val = iArgV->a_w.w_long;
				break;
			case A_FLOAT:
				val = iArgV->a_w.w_float;
				break;
			default:
				// We don't like this to happen, but we won't kvetch
				val = 0;
				break;
			}
		*(dataPtr++) = val;
		total += val;
		iArgV += 1;
		}
	me->totalBalls = total;
	CuthbertReset(me);
	
	}
	
void
CuthbertClear(
	tCuthbert*	me)
	
	{
	
	CuthbertConst(me, 0);		// That's all. Don't even need Enter/ExitCallback mantra
	
	}

void
CuthbertConst(
	tCuthbert*	me,
	long	iVal)
	
	{
	long*		dataPtr	= me->master;
	unsigned	ds		= me->dataSize;
	// Nothing Enter/ExitCallback()-sensitive here;
	
	me->totalBalls = ds * iVal;
	while (ds-- > 0) {
		*(dataPtr++) = iVal;
		}
	
	CuthbertReset(me);
	
	}
	
	
void
CuthbertSize(
	tCuthbert*	me,
	long	iSize)
	
	{
	const long	kMaxSize = 4095;	// Largest number of "balls" we can reliably allocate
	
	long*	newMaster;
	long*	newState;
	Boolean	maxMemory;
	long	sizeBytes;				// Use this for a couple of purposes
						
	if (iSize > kMaxSize) {
			error("%s can't handle more than %lf different kinds of 'ball'.",
					kClassName, kMaxSize);
			iSize = kMaxSize;
			}
	else if (iSize <= 0) {
		error("%s size parameter must be positive");
		return;
		}
	
	if (iSize == me->dataSize) {
		// Nothin' to do...
		// ... 'cept that the documentation asserts that a size message always entails a reset
		CuthbertReset(me);
		return;
		}
				
	sizeBytes = sizeof(long) * iSize;						// New buffer size in bytes
	maxMemory = (isr() != 0);								// User getbytes() or NewPtr() ?
	
	if (maxMemory) {
		newMaster	= (long*) getbytes(sizeBytes);
		newState	= (long*) getbytes(sizeBytes);
		}
	else {
		// We could NewPtrClr() here or try SetPtrSize(), but that would entail even
		// more special casing. Also note that we are not trying for Temp Memory.
		// That, too, would entail more special case handling.
		// Using handles on the Mac Memory side might be worth the overhead of extra
		// special-case handling, and we might try that in a future version...
		newMaster	= (long*) NewPtr(sizeBytes);
		newState	= (long*) NewPtr(sizeBytes);
		}
	
	if (newMaster == NIL || newState == NIL) {
		// Aw, shucks.
		// Let's try to articulate the problem a little better than getbytes() does
		error("%s: Max cannot allocate the required memory for size message.");
		post ("    Try a smaller value.");
		DisposeMemory(newMaster, newState, maxMemory ? sizeBytes : 0);
		return;
		}
	
	// Any old data to copy and and dispose of?
	if (me->dataSize > 0) {
		if (me->dataSize < iSize)
			sizeBytes = sizeof(long) * me->dataSize;	// Now number of bytes to move
		BlockMoveData(	me->master, newMaster, sizeBytes);
		BlockMoveData(	me->master, newState, sizeBytes);
		DisposeMemory(me->master, me->state, me->maxMemory ? sizeof(long) * me->dataSize : 0);
		}
	me->maxMemory	= maxMemory;
	me->master		= newMaster;
	me->state		= newState;
	
	// What's the best way to recalculate total, zero new entries, etc.?
	if (me->dataSize > iSize || me->dataSize == 0) {
		// Memory has shrunk; we need to recalc the master total
		// ... or this could be the first time through
		// This could be made faster by checking if (iSize > me->dataSize/2 && me->dataSize != 0).
		// Maybe someday.
		// Such an optimization would also entail moving the deallocation around. Pain.
		me->dataSize = iSize;
		RecalcMasterTotal(me);
		}
	else {
		// Memory has expanded. Master total is good, but we need to set the counts of the
		// "new" balls to 0. Variant on cheesy trick.
		long	oldTotal	= me->totalBalls,
				oldSize		= me->dataSize;
		me->master		+= oldSize;
		me->state		+= oldSize;
		me->dataSize	=  iSize - me->dataSize;
		CuthbertConst(me, 0);
		me->master		-= oldSize;
		me->state		-= oldSize;
		me->dataSize	=  iSize;
		me->totalBalls	=  oldTotal;
		}
	CuthbertReset(me);
	
	}


void
CuthbertReset(
	tCuthbert* me)
	
	{
	
	BlockMoveData(me->master, me->state, me->dataSize * sizeof(long));
	me->ballsInUrn = me->totalBalls;
	
	outlet_bang(me->resetOutlet);
	
	}


/******************************************************************************************
 *
 *	CuthbertCount(me, iWhat)
 *	
 *	Send information out left outlet. The iWhat parameter determines whether count/sum is
 *	made on all balls (when iWhat is zero), only the balls still in the urn (for positive
 *	iWhat), or balls already taken out of the urn (negative values). 
 *	
 ******************************************************************************************/

void
CuthbertCount(
	tCuthbert*	me,
	long	iWhat)
	
	{
	long	result = 0;
	
	if (iWhat == 0)
			result = me->totalBalls;
	else if (iWhat > 0)
			result = me->ballsInUrn;
	else	result = me->totalBalls - me->ballsInUrn;
	
	outlet_int(me->coreObject.o_outlet, result);
	
	}


/******************************************************************************************
 *
 *	CuthbertSeed(me, iSeed)
 *
 ******************************************************************************************/

void CuthbertSeed(tCuthbert* me, long iSeed)
	{ Taus88Seed(me->tausData, (unsigned long) iSeed); }


/******************************************************************************************
 *
 *	CuthbertTattle(me)
 *
 *	Post state information
 *
 ******************************************************************************************/

void
CuthbertTattle(
	tCuthbert* me)
	
	{
	
	post("%s state",
			kClassName);
	post("  number of kinds of balls in urn is %lu",
			me->dataSize);
	post("  total number of balls currently in urn is %lu (initial state is %lu)",
			me->ballsInUrn,
			me->totalBalls);
	post("  Current memory allocation is using %s",
			me->maxMemory ? "getbytes()" : "NewPtr()");
	
	}


/******************************************************************************************
 *
 *	CuthbertAssist
 *
 *	Fairly generic Assist Method
 *
 ******************************************************************************************/

void
CuthbertAssist(
	tCuthbert*	me,
	void*	box,
	long	iDir,
	long	iArgNum,
	char*	oCStr)
	
	{
	#pragma unused(me, box)
	
	LitterAssist(iDir, iArgNum, strIndexInBang, strIndexOutCuthbert, oCStr);
	}

/******************************************************************************************
 *
 *	CuthbertInfo(me)
 *
 *	Fairly basic Info Method
 *
 ******************************************************************************************/

void
CuthbertInfo(
	tCuthbert* me)
	
	{
	
	LitterInfo(kClassName, &me->coreObject, (method) CuthbertTattle);
	
	}

