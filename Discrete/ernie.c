/*
	File:		ernie.c

	Contains:	Max external object implementing a "finite urn" model.

	Written by:	Peter Castine

	Copyright:	© 2001-2002 Peter Castine

	Change History (most recent first):

         <8>   23–3–2006    pc      Update #includes for new LitterLib organization. (Implementation
                                    of expect message postponed).
         <7>     9/14/05    pc      Remove call to ErnieTattle() from refer method
         <6>     15–1–04    pc      Avoid possible memory leak if seed argument is used.
         <5>     14–1–04    pc      Update for Windows.
         <4>    7–7–2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <3>  30–12–2002    pc      Drop faux 'Vers' resource and related modifications. Add object
                                    version to DoInfo().
         <2>  28–11–2002    pc      Tidy up after initial check in.
         <1>  28–11–2002    pc      Initial check in.
		30-Jun-2001:	Modified to use Taus88DataPtr (which may be null to indicate use
						of the global Taus88 data pool).
		12-Apr-2001:	First implementation.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "LitterLib.h"
#include "TrialPeriodUtils.h"
#include "Taus88.h"
#include "MiscUtils.h"

#pragma mark • Constants

const char*		kClassName		= "lp.ernie";			// Class name
const long		kDefDataSize	= 128;					// Same as table

	// Indices for STR# resource
enum {
	strIndexInBang		= lpStrIndexLastStandard + 1,

	strIndexOutErnie
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
	LITTER_CORE_OBJECT(Object, coreObject);
	
	voidPtr			resetOutlet;		// Main outlet accessed through coreObject.o_outlet
	
	tTaus88DataPtr	tausData;
	
	unsigned		dataSize;			// Different "kinds" of ball, viz: [0 .. dataSize-1]
		// Need two dynamically allocated vectors...
		// Short would do the trick. However, table uses long so we will, too.
	long*			master;						// Initial state of urn
	long*			state;						// Current state (after removing balls)
		// Cache total number of balls in both master and state vectors
	long			totalBalls,
					ballsInUrn;
	
	Boolean			maxMemory;			// False if using OS Memory allocation; true if
										// we're using Max' getbytes();
	} objErnie;


#pragma mark Function Prototypes

static void ErnieSize(objErnie*, long);
static void ErnieClear(objErnie*);
static void ErnieFree(objErnie*);
static void ErnieReset(objErnie*);
static void ErnieConst(objErnie*, long);
	



#pragma mark -

/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	ErnieNew(iOracle)
 *
 ******************************************************************************************/

static void*
ErnieNew(
	long	iDataSize,
	long	iSeed)
	
	{
	objErnie*			me				= NIL;
	tTaus88DataPtr	myTaus88Stuff	= NIL;
	
	
	// Run through initialization parameters from right to left, handling defaults
	if (iSeed != 0) {
		myTaus88Stuff = Taus88New(iSeed);
		goto noMoreDefaults;
		}
	
	if (iDataSize == 0)
		iDataSize = kDefDataSize;
noMoreDefaults:
	// Finished checking intialization parameters

	// Let Max allocate us and our outlets. We only use the default inlet
	me = (objErnie*) LitterAllocateObject();

	if (me == NIL) goto punt;
	
	me->resetOutlet	= bangout(me);
	intout(me);							// Access through me->coreObject.o_outlet
	
	// Initialize object components
	me->tausData	= myTaus88Stuff;
	me->dataSize	= 0;				// Start from clean slate before calling ErnieSize()
	me->master		= NIL;
	me->state		= NIL;
	ErnieSize(me, iDataSize);
	if (me->master == NIL || me->state == NIL) {
		goto punt;
		}
	ErnieClear(me);
	
	return me;

	// We only get here if something bad happened
punt:
	if (me != NIL) {
		ErnieFree(me);
		freeobject((t_object*) me);
		}
	
	return NIL;
	}

/******************************************************************************************
 *
 *	ErnieFree(iOracle)
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

static void
ErnieFree(
	objErnie*	me)
	
	{
	
	Taus88Free(me->tausData);					// Taus88Free is NIL-safe
	
	DisposeMemory(me->master, me->state, me->maxMemory ? sizeof(long) * me->dataSize : 0);
	
	}


#pragma mark -
#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	ErnieBang(me)
 *
 ******************************************************************************************/

static void
ErnieBang(
	objErnie* me)
	
	{
	long	theBall = -1;			// This is the result defined for the degenerate case
									// of "no balls in the system"; this value also allows
									// the main loop condition to be streamlined.
	
	
	if (me->totalBalls > 0) {
		long	magic;
		long*	sp;
		
		if (me->ballsInUrn == 0) {
			ErnieReset(me);
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
 *	ErnieTable(me, iTable)
 *	ErnieSet(me, iSym, iArgC, iArgV)
 *	ErnieClear(me)
 *	ErnieConst(me, iVal)
 *	ErnieSize(me, iSize)
 *	ErnieReset(me)
 *	
 *	Set parameters and data. Make sure nothing bad is happening.
 *	
 ******************************************************************************************/

	static void RecalcMasterTotal(objErnie* me)
		{
		unsigned	dataSize	= me->dataSize;
		long*		data		= me->master;
		long		total		= 0;
		
		while (dataSize-- > 0) {
			total += *(data++);
			}
		
		me->totalBalls = total;
		}

static void
ErnieTable(
	objErnie*	me,
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
			ErnieConst(me, 0);
			me->dataSize	+= size;
			me->master		-= size;
			me->state		-= size;
			}
		
		BlockMoveData(*tableData, me->master, size * sizeof(long));
		
		RecalcMasterTotal(me);
		ErnieReset(me);
		}
	
	else error("%s: can't find table named %s", kClassName, iTable->s_name);
	
	}
	
static void
ErnieSet(
	objErnie*	me,
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
		ErnieConst(me, 0);
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
	ErnieReset(me);
	
	}
	
static void ErnieClear(objErnie*	me)
	{ ErnieConst(me, 0); }

static void
ErnieConst(
	objErnie*	me,
	long	iVal)
	
	{
	long*		dataPtr	= me->master;
	unsigned	ds		= me->dataSize;
	// Nothing Enter/ExitCallback()-sensitive here;
	
	me->totalBalls = ds * iVal;
	while (ds-- > 0) {
		*(dataPtr++) = iVal;
		}
	
	ErnieReset(me);
	
	}
	
	
static void
ErnieSize(
	objErnie*	me,
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
		ErnieReset(me);
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
		ErnieConst(me, 0);
		me->master		-= oldSize;
		me->state		-= oldSize;
		me->dataSize	=  iSize;
		me->totalBalls	=  oldTotal;
		}
	ErnieReset(me);
	
	}


static void
ErnieReset(
	objErnie* me)
	
	{
	
	BlockMoveData(me->master, me->state, me->dataSize * sizeof(long));
	me->ballsInUrn = me->totalBalls;
	
	outlet_bang(me->resetOutlet);
	
	}


/******************************************************************************************
 *
 *	ErnieCount(me, iWhat)
 *	
 *	Send information out left outlet. The iWhat parameter determines whether count/sum is
 *	made on all balls (when iWhat is zero), only the balls still in the urn (for positive
 *	iWhat), or balls already taken out of the urn (negative values). 
 *	
 ******************************************************************************************/

static void
ErnieCount(
	objErnie*	me,
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
 *	ErnieSeed(me, iSeed)
 *
 ******************************************************************************************/

static void ErnieSeed(objErnie* me, long iSeed)
	{ Taus88Seed(me->tausData, (unsigned long) iSeed); }


#pragma mark -
#pragma mark • Attribute/Information Functions

/******************************************************************************************
 *
 *	ErnieTattle(me)
 *	ErnieAssist
 *
 ******************************************************************************************/

static void
ErnieTattle(
	objErnie* me)
	
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

static void
ErnieAssist(
	objErnie*	me,
	void*		box,
	long		iDir,
	long		iArgNum,
	char*		oCStr)
	
	{
	#pragma unused(me, box)
	
	LitterAssist(iDir, iArgNum, strIndexInBang, strIndexOutErnie, oCStr);
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
	objErnie*		me,
	eExpectSelector	iSel)
	
	{
	double	result	= 0.0 / 0.0;
	int		i, r;
	
	if (me->totalBalls > 0) switch(iSel) {
	case expMean:
		r = 0;
		i = me->dataSize;
		while (--i >= 0)
			r += i * me->master[i];
		result = (double) r / me->totalBalls;
		break;
	case expMedian:
		r = 0;
		i = me->totalBalls / 2;
		do	{
			i -= me->master[r];
			if (i <= 0)
				break;
			r += 1;
			} while (true);
		result = r;
		break;
	case expMode:
		r = 0;
		i = me->dataSize;
		while (--i >= 0) {
			if (r < me->master[i]) {
				r = me->master[i];
				result = i;
				}
			else if (r == me->master[i]) result = 0.0 / 0.0;
			}
		// !! Doesn't report multiple modes, which we ought to
		break;
	case expVar:
		// Need to calculate this... doable but not yet done !!
		break;
	case expStdDev:
		// Need to calculate this... doable but not yet done !!
		break;
	case expSkew:
		// Need to calculate this... doable but not yet done !!
		break;
	case expKurtosis:
		// Need to calculate this... doable but not yet done !!
		break;
	case expMin:
		for (i = 0; i < me->dataSize; i += 1)
			if (me->master[i] > 0) break;
		result = i;
		break;
	case expMax:
		for (i = me->dataSize; i > 0; )
			if (me->master[--i] > 0) break;
		result = i;
		break;
	case expEntropy:
		// Need to calculate this... doable but not yet done !!
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

	// These have not yet been implemented
	static t_max_err ErnieGetMin(objErnie* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expMin), ioArgC, ioArgV);
		}
	static t_max_err ErnieGetMax(objErnie* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expMax), ioArgC, ioArgV);
		}
	static t_max_err ErnieGetMean(objErnie* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expMean), ioArgC, ioArgV);
		}
	static t_max_err ErnieGetMedian(objErnie* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expMedian), ioArgC, ioArgV);
		}
	
		// Ernie may well have a multi-modal distribution, so we can't use the standard
		// DoExpect() function, which assumes unimodality.
		// But we've not built the multi-modal solution  yet...
	static t_max_err ErnieGetMode(objErnie* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expMode), ioArgC, ioArgV);
		}

//	static t_max_err ErnieGetVar(objErnie* me, void* iAttr, long* ioArgC, Atom** ioArgV)
//		{ 
//		#pragma unused(iAttr)
//		
//		return LitterGetAttrFloat(DoExpect(me, expVar), ioArgC, ioArgV);
//		}
//	static t_max_err ErnieGetStdDev(objErnie* me, void* iAttr, long* ioArgC, Atom** ioArgV)
//		{ 
//		#pragma unused(iAttr)
//		
//		return LitterGetAttrFloat(DoExpect(me, expStdDev), ioArgC, ioArgV);
//		}
//	static t_max_err ErnieGetSkew(objErnie* me, void* iAttr, long* ioArgC, Atom** ioArgV)
//		{ 
//		#pragma unused(iAttr)
//		
//		return LitterGetAttrFloat(DoExpect(me, expSkew), ioArgC, ioArgV);
//		}
//	static t_max_err ErnieGetKurtosis(objErnie* me, void* iAttr, long* ioArgC, Atom** ioArgV)
//		{ 
//		#pragma unused(iAttr)
//		
//		return LitterGetAttrFloat(DoExpect(me, expKurtosis), ioArgC, ioArgV);
//		}
//	static t_max_err ErnieGetEntropy(objErnie* me, void* iAttr, long* ioArgC, Atom** ioArgV)
//		{ 
//		#pragma unused(iAttr)
//		
//		return LitterGetAttrFloat(DoExpect(me, expEntropy), ioArgC, ioArgV);
//		}
	
	static inline void
	AddInfo(void)
		{
		Object*	attr;
		Symbol*	symFloat64		= gensym("float64");
		Symbol*	symLong			= gensym("long");
		
		// Read-Write Attributes
//		attr = attr_offset_new("p", symFloat64, 0, NULL, NULL, calcoffset(objErnie, prob));
//		class_addattr(gObjectClass, attr);
//		attr = attr_offset_new("n", symLong, 0, NULL, NULL, calcoffset(objErnie, nTrials));
//		class_addattr(gObjectClass, attr);
		
		// Read-Only Attributes
		attr = attr_offset_new(	"balls", symLong, kAttrFlagsReadOnly, NULL, NULL,
								calcoffset(objErnie, totalBalls));
		class_addattr(gObjectClass, attr);
		
		attr = attr_offset_new(	"ballsLeft", symLong, kAttrFlagsReadOnly, NULL, NULL,
								calcoffset(objErnie, ballsInUrn));
		class_addattr(gObjectClass, attr);
		
		attr = attribute_new("min", symLong, kAttrFlagsReadOnly, (method) ErnieGetMin, NULL);
		class_addattr(gObjectClass, attr);
		
		attr = attribute_new("max", symLong, kAttrFlagsReadOnly, (method) ErnieGetMax, NULL);
		class_addattr(gObjectClass, attr);
		
		attr = attribute_new("mean", symFloat64, kAttrFlagsReadOnly, (method) ErnieGetMean, NULL);
		class_addattr(gObjectClass, attr);
		
		attr = attribute_new("median", symFloat64, kAttrFlagsReadOnly, (method) ErnieGetMedian, NULL);
		class_addattr(gObjectClass, attr);
		
		attr = attribute_new("mode", symFloat64, kAttrFlagsReadOnly, (method) ErnieGetMode, NULL);
		class_addattr(gObjectClass, attr);
		
//		attr = attribute_new("var", symFloat64, kAttrFlagsReadOnly, (method) ErnieGetVar, NULL);
//		class_addattr(gObjectClass, attr);
//		attr = attribute_new("stddev", symFloat64, kAttrFlagsReadOnly, (method) ErnieGetStdDev, NULL);
//		class_addattr(gObjectClass, attr);
//		attr = attribute_new("skew", symFloat64, kAttrFlagsReadOnly, (method) ErnieGetSkew, NULL);
//		class_addattr(gObjectClass, attr);
//		attr = attribute_new("kurtosis", symFloat64, kAttrFlagsReadOnly, (method) ErnieGetKurtosis, NULL);
//		class_addattr(gObjectClass, attr);
//		attr = attribute_new("entropy", symFloat64, kAttrFlagsReadOnly, (method) ErnieGetEntropy, NULL);
//		class_addattr(gObjectClass, attr);
		}

static void
ErnieTell(
	objErnie*	me,
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

static void ErnieInfo(objErnie* me)
	{ LitterInfo(kClassName, &me->coreObject, (method) ErnieTattle); }

static inline void AddInfo(void)
	{ LitterAddCant((method) ErnieInfo, "info"); }
		
static void ErnieTell(objErnie* me, Symbol* iTarget, Symbol* iMsg)
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
						A_DEFLONG,		// Data size		(Default: kMinLong)
						A_DEFLONG,		// Seed				(Default: Zero -- autoseed)
						A_NOTHING
						};
	
	LITTER_CHECKTIMEOUT(kClassName);
	
	// Standard Max setup() call
	LitterSetupClass(kClassName,
					 sizeof(objErnie),			// Class object size
					 LitterCalcOffset(objErnie),
					 (method) ErnieNew,			// Instance creation function
					 (method) ErnieFree,		// Deallocation function
					 NIL,						// No menu function
					 myArgTypes);				// See above		
	
	// Messages
	LITTER_TIMEBOMB LitterAddBang	((method) ErnieBang);
	LitterAddMess1	((method) ErnieTable,		"refer",	A_SYM);
	LitterAddGimme	((method) ErnieSet,			"set");
	LitterAddMess1	((method) ErnieSeed,		"seed",		A_DEFLONG);
	LitterAddMess0	((method) ErnieClear,		"clear");
	LitterAddMess0	((method) ErnieClear,		"zero");
	LitterAddMess1	((method) ErnieConst,		"const",	A_LONG);
	LitterAddMess0	((method) ErnieReset,		"reset");
	LitterAddMess1	((method) ErnieSize,		"size",		A_LONG);
	LitterAddMess1	((method) ErnieCount,		"count",	A_DEFLONG);
	LitterAddMess2	((method) ErnieTell,		"tell",		A_SYM, A_SYM);
	LitterAddCant	((method) ErnieTattle,		"dblclick");
	LitterAddMess0	((method) ErnieTattle,		"tattle");
	LitterAddCant	((method) ErnieAssist,		"assist");
	
	AddInfo();
	
	//Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();
	
	}

