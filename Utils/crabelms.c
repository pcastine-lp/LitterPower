/*
	File:		crabelms.c

	Contains:	Litter implementation replacing James McCartney's orphaned scramble.

	Written by:	Peter Castine

	Copyright:	© 2006 Peter Castine

	Change History (most recent first):

*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark ¥ Include Files

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

#pragma mark ¥ Constants

const char*	kClassName		= "lp.crabelms";			// Class name

#ifdef __GNUC__
	// Braindead GCC
	#define	kMaxListLen		255
#else
	const int kMaxListLen	= 255;
#endif

	// Indices for STR# resource
enum {
	strIndexLeftIn			= lpStrIndexLastStandard + 1,
	strIndexLeftOut
	};


#pragma mark ¥ Type Definitions



#pragma mark ¥ Object Structure

typedef struct {
	Object			coreObject;
	
	tTaus88DataPtr	tausData;
	
	int				len;					// Actual number of items in list
											// This member couls be as small as a byte with
											// the current value of kMaxListLen, but
											// padding will fill up whatever space an int
											// takes, and ints are more convenient in
											// for processing.
	Atom			silt[kMaxListLen];		// list, possibly scrambled
	} objCrabElms;


#pragma mark ¥ Global Variables

#pragma mark ¥ Function Prototypes

	// Class message functions
objCrabElms*	CrabElmsNew(long);
void			CrabElmsFree(objCrabElms*);

	// Object message functions
static void CrabElmsBang(objCrabElms*);
static void CrabElmsFloat(objCrabElms*, double);
static void CrabElmsInt(objCrabElms*, long);
static void CrabElmsList(objCrabElms*, Symbol*, short, Atom[]);
static void CrabElmsSet(objCrabElms*, Symbol*, short, Atom[]);
static void CrabElmsAnything(objCrabElms*, Symbol*, short, Atom[]);
static void CrabElmsTattle(objCrabElms*);
static void	CrabElmsAssist(objCrabElms*, void*, long, long, char*);
static void	CrabElmsInfo(objCrabElms*);

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
	setup(	&gObjectClass,						// Pointer to our class definition
			(method) CrabElmsNew,				// Instantiation method  
			(method) CrabElmsFree,				// Custom deallocation function
			(short) sizeof(objCrabElms),		// Class object size
			NIL,								// No menu function
			A_DEFLONG,							// If non-zero initialize & maintain a
			0);									// private seed pool
	
	// Messages
	LITTER_TIMEBOMB addbang	((method) CrabElmsBang);
	LITTER_TIMEBOMB addint	((method) CrabElmsInt);
	LITTER_TIMEBOMB addfloat((method) CrabElmsFloat);
	LITTER_TIMEBOMB addmess	((method) CrabElmsList,		"list", A_GIMME, 0);
	LITTER_TIMEBOMB addmess	((method) CrabElmsAnything, "anything", A_GIMME, 0);
	addmess	((method) CrabElmsSet,		"set",			A_FLOAT, 0);
	addmess	((method) CrabElmsAssist,	"assist",		A_CANT, 0);
	addmess	((method) CrabElmsInfo,		"info",			A_CANT, 0);
	addmess ((method) CrabElmsTattle,	"tattle",	 	A_NOTHING);
	addmess ((method) CrabElmsTattle,	"dblclick", 	A_CANT, 0);
	
	// Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();
	
	}

#pragma mark -
#pragma mark ¥ Class Message Handlers

/******************************************************************************************
 *
 *	CrabElmsNew(iSym, iArgCount, iArgVec)
 *	CrabElmsFree(me)
 *
 ******************************************************************************************/

objCrabElms*
CrabElmsNew(
	long	iSeed)
	
	{
	objCrabElms*	me = NIL;
	int				i;
	
	// Create object
	me = (objCrabElms*) newobject(gObjectClass);
		if (me == NIL) goto punt;			// Poor man's exception handling
	
	// Add inlets and outlets
	listout(me);							// Access through me->coreObject.o_outlet
	
	// Initialize object members
	me->tausData	= NIL;					// Try to allocate later, if requested by user
	me->len			= 0;
	
	// Slightly anal-retentive, but safety first
	for (i = 0; i < kMaxListLen; i += 1) {
		me->silt[i].a_type		= A_NOTHING;
		me->silt[i].a_w.w_long	= 0;
		}
	
	// Now that we're squeaky clean, check if we need to allocate our own seed base
	if (iSeed != 0)
		me->tausData = Taus88New(iSeed);
	
punt:
	return me;
	}

void CrabElmsFree(objCrabElms* me)
	{
	Taus88Free(me->tausData);		// Taus88Free() is null-safe
	}

#pragma mark -
#pragma mark ¥ Object Message Handlers

/******************************************************************************************
 *
 *	CrabElmsBang(me)
 *	CrabElmsSet(me, sym, iArgCount, iArgVec)
 *
 *	CrabElmsInt(me, iVal)
 *	CrabElmsFloat(me, iVal)
 *	CrabElmsList(me, sym, iArgCount, iArgVec)
 *	
 ******************************************************************************************/

void
CrabElmsBang(
	objCrabElms* me)
	
	{
	int		c = me->len;
	
	// Quick check if we really need to 'scramble' items
	// Zero-item and single-item lists don't need processing
	if (c > 1) {
		Atom*			l = me->silt;
		tTaus88DataPtr	t = me->tausData;
		UInt32			s1, s2, s3;
		
		// Set up for inlined Taus88 random numbers
		if (t == NIL)
			t = Taus88GetGlobals();
		Taus88Load(t, &s1, &s2, &s3);	
		
		do	{
			UInt32	r = Taus88Process(&s1, &s2, &s3),
					j = ((r & 0x0000ffff) * c) >> 16;	// ASSERT: 0 <= j < c
			
			if (j > 0)
				AtomSwap(l, l + j);
			l += 1;
			c -= 1;
			
			if (c > 0) {
				r >>= 16;
				j = (r * c) >> 16;
				if (j > 0)
					AtomSwap(l, l + j);
				l += 1;
				c -= 1;
				}
			
			} while (c > 0);
		
		// Update Taus88 seed pool
		Taus88Store(t, s1, s2, s3);
		}
	
	
	outlet_list(me->coreObject.o_outlet, NIL, me->len, me->silt);
	}

void
CrabElmsSet(
	objCrabElms*	me,
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
		me->silt[i] = iArgVec[i];
	
	me->len = iArgCount;
	
	}


void CrabElmsInt(objCrabElms* me, long iVal)
	{
	Atom a;
	
	AtomSetLong(&a, iVal);
	
	CrabElmsSet(me, NIL, 1, &a);
	CrabElmsBang(me);
	
	}

void CrabElmsFloat(objCrabElms* me, double iVal)
	{
	Atom a;
	
	AtomSetFloat(&a, iVal);
	
	CrabElmsSet(me, NIL, 1, &a);
	CrabElmsBang(me);
	
	}
	

void
CrabElmsList(
	objCrabElms*	me,
	Symbol*			sym,										// unused, must be "list"
	short			iArgCount,
	Atom			iArgVec[])
	
	{
	#pragma unused(sym)
	
	CrabElmsSet(me, NIL, iArgCount, iArgVec);
	CrabElmsBang(me);
	
	}

/******************************************************************************************
 *
 *	CrabElmsAnything(me)
 *
 *	Sort of special case: we treat arbitrary messages with parameter lists as if they were
 *	a list beginning with a symbol.
 *
 ******************************************************************************************/

void
CrabElmsAnything(
	objCrabElms*	me,
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
	
	AtomSetSym(&me->silt[0], iSym);
	
	for (i = 0; i < iArgCount; i += 1)
		me->silt[i+1] = iArgVec[i];
	
	me->len = iArgCount + 1;
	
	CrabElmsBang(me);
	
	}




/******************************************************************************************
 *
 *	CrabElmsTattle(me)
 *
 * 	Tell all we know.
 *
 ******************************************************************************************/

void
CrabElmsTattle(
	objCrabElms*	me)
	
	{
	
	// Object state
	post("%s status", kClassName);
	
	// ... and then stuff specific to CrabElms
	post("  scrambling %ld items", (long) me->len);
	
	}

/******************************************************************************************
 *
 *	CrabElmsAssist(me, iBox, iDir, iArgNum, oCStr)
 *	CrabElmsInfo(me)
 *
 ******************************************************************************************/

void
CrabElmsAssist(
	objCrabElms*	me,
	tBoxPtr			box,				// unused
	long			iDir,
	long			iArgNum,
	char*			oCStr)
	
	{
	#pragma unused(me, box)
	
	LitterAssist(iDir, iArgNum, strIndexLeftIn, strIndexLeftOut, oCStr);
	
	}

void CrabElmsInfo(objCrabElms* me)
	{ LitterInfo(kClassName, (tObjectPtr) me, (method) CrabElmsTattle); }


