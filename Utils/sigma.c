/*
	File:		+.c

	Contains:	Max/MSP external object Sum.

	Written by:	Peter Castine

	Copyright:	© 2003 Peter Castine

	Change History (most recent first):

        <10>      9–1–06    pc      Update to use LitterAssistResFrag() utility function.
         <9>    13–12–04    pc      Update to use proxy_getinlet(). Also update function names to
                                    match object name for the benefit of crash logs.
         <8>     11–1–04    pc      Update for modified LitterInit()
         <7>      8–1–04    pc      Add explantory comment to SumNew().
         <6>      8–1–04    pc      Use Max memory allocation instead of native Mac Memory Manager
                                    calls. Possible To Do: implement my own x-platform code to allow
                                    externals to access OS native memory functions (might be more
                                    flexible than the Max API) .
         <5>      8–1–04    pc      Update for Windows.
         <4>    8–7–2003    pc      Problem with list input. Fixed.
         <3>    6–7–2003    pc      Explicit tattle message takes A_NOTHING as arg list (not
                                    A_CANT). Fixed.
         <2>   30–3–2003    pc      Update kClassName
         <1>    8–3–2003    pc      Initial implementation & check-in
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "LitterLib.h"
#include "TrialPeriodUtils.h"

#pragma mark • Constants

const char*	kClassName		= "lp.sigma";			// Class name

	// Indices for STR# resource
enum {
	strIndexAnyInlet		= lpStrIndexLastStandard + 1,
	strIndexSumOutlet,
	
	strIndexInt,
	strIndexFloat,
	
	strIndexLeftIn			= strIndexAnyInlet,
	strIndexLeftOut			= strIndexSumOutlet
	};


#pragma mark • Type Definitions

	// Should add these to MaxUtils.h
typedef tWord*		tWordPtr;
typedef tWordPtr*	tWordHdl;

#pragma mark • Object Structure

typedef struct {
	Object		coreObject;
	
	Boolean		doFloats;			// Are we doing integer arithmetic or fp?
	long		inletNum,			// Max tells us which inlet was used here.
				inletCount;			// Number of inlets this object is managing
	tWordPtr	data;				// Vector of either floats or ints
	double		sum;				// Result. This is another way of not having to
									// worry about whether we're doing integer or fp
									// arithmetic.
	} objSigma;
typedef objSigma* objSigmaPtr;


#pragma mark • Global Variables

#pragma mark • Function Prototypes

	// Class message functions
objSigmaPtr	SigmaNew(SymbolPtr, short, Atom[]);
void		SigmaFree(objSigma*);

	// Object message functions
static void SigmaBang(objSigma*);
static void SigmaFloat(objSigma*, double);
static void SigmaInt(objSigma*, long);
static void SigmaList(objSigma*, Symbol*, short, Atom*);
static void SigmaSet(objSigma*, double);
static void SigmaTattle(objSigma*);
static void	SigmaAssist(objSigma*, void*, long, long, char*);
static void	SigmaInfo(objSigma*);

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
main()
	
	{
	LITTER_CHECKTIMEOUT(kClassName);
	
	// Standard Max setup() call
	setup(	&gObjectClass,					// Pointer to our class definition
			(method) SigmaNew,				// Instantiation method  
			(method) SigmaFree,				// Custom deallocation function
			(short) sizeof(objSigma),		// Class object size
			NIL,							// No menu function
			A_GIMME,						// Parse initialization arguments ourselves
			0);
	
	// Messages
	LITTER_TIMEBOMB addbang	((method) SigmaBang);
	LITTER_TIMEBOMB addint	((method) SigmaInt);
	LITTER_TIMEBOMB addfloat((method) SigmaFloat);
	LITTER_TIMEBOMB addmess	((method) SigmaList, "list", A_GIMME, 0);
	addmess	((method) SigmaSet,		"set",			A_FLOAT, 0);
	addmess	((method) SigmaAssist,	"assist",		A_CANT, 0);
	addmess	((method) SigmaInfo,	"info",			A_CANT, 0);
	addmess ((method) SigmaTattle,	"tattle",	 	A_NOTHING);
	addmess ((method) SigmaTattle,	"dblclick", 	A_CANT, 0);
	
	// Initialize Litter Library
	LitterInit(kClassName, 0);

	}

#pragma mark -
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	SigmaNew(iSym, iArgCount, iArgVec)
 *	SigmaFree(me)
 *
 ******************************************************************************************/

objSigma*
SigmaNew(
	SymbolPtr	sym,								// unused, must be 'lp.+'
	short		iArgCount,
	Atom		iArgVec[])
	
	{
	#pragma unused(sym)
	
	const short	kMinInlets	= 2,
				kInlets		= (iArgCount < kMinInlets) ? kMinInlets : iArgCount;
	
	objSigma*	me			= NIL;
	tWordPtr	data;
	double		sum;
	Boolean		doFloats	= false;
	short		c;
	AtomPtr		ap;
	
	
	// Scan argument list to see if there are any floats
	// This is most efficient when the first argument is a float
	for (c = iArgCount, ap = iArgVec; c > 0; c -= 1) {
		if (ap++->a_type == A_FLOAT) {
			doFloats = true;
			break;
			}
		}
	
	// Let Max allocate us
	me = (objSigma*) newobject(gObjectClass);
	if (me == NIL) goto punt;
	
	// Add inlets, right to left
	c = kInlets;
	while (--c > 0)
		proxy_new(&me->coreObject, c, &me->inletNum);
	
	// Add one outlet. We can grab this one from the coreObject component.
	if (doFloats)
		 floatout(me);
	else intout(me);
	
	// Allocate other memory
	me->inletCount = kInlets;
	me->data = data = (tWordPtr) getbytes(kInlets * sizeof(tWord));
	if (data == NIL) {
		freeobject((Object*) me);		// Requires that me->data be set to NIL
		goto punt;
		}
	
	// Any other initialization
	sum = 0.0;
	if (doFloats) for (c = 0; c < iArgCount; c += 1)
		sum += data[c].w_float = AtomGetFloat(iArgVec + c);
	else for (c = 0; c < iArgCount; c += 1) {
		sum += data[c].w_long = AtomGetLong(iArgVec + c);
		}
		
		// Since moving from native Mac OS Memory Manager functions to Max memory
		// allocation (for ease of cross-platform coding), we no longer have an
		// allocation call that autotically clears, so do it manually.
		// Note that the following works even if we're really doing floats.
		// Also note that the loop is only executed if iArgCount was zero or one
		// (using default arguments).
	while (c < kInlets) data[c++].w_long = 0;
	
	me->sum			= sum;
	me->doFloats	= doFloats;
	
punt:
	return me;
	}

void SigmaFree(objSigma* me)
	{ if (me->data != NIL) freebytes(me->data, me->inletCount * sizeof(tWord)); }

#pragma mark -
#pragma mark • Utilities

/******************************************************************************************
 *
 *	RecalcFloats(me)
 *	RecalcInts(me)
 *	UpdateSum(me)
 *
 ******************************************************************************************/

static void
RecalcFloats(
	objSigma* me)
	
	{
	double	newSum = 0.0;
	int		terms	= me->inletCount;
	tWord*	curWord	= me->data;
	
	// ASSERT: (me->doFloats)
	do { newSum += (curWord++)->w_float; }
		while (--terms > 0);
	
	me->sum = newSum;
	
	}

static void
RecalcInts(
	objSigma* me)
	
	{
	long	newSum = 0;
	int		terms	= me->inletCount;
	tWord*	curWord	= me->data;
	
	// ASSERT: (!me->doFloats)
	do { newSum += (curWord++)->w_long; }
		while (--terms > 0);
	
	me->sum = newSum;
	
	}

static void
UpdateSum(
	objSigma*	me,
	long		iIndex,
	double		iVal)
	
	{
	tWord*	curWord = me->data + iIndex;
	
	if (me->doFloats) {
		me->sum -= curWord->w_float;
		me->sum += curWord->w_float = iVal;
		}
	else {
		me->sum -= curWord->w_long;
		me->sum += curWord->w_long = (long) iVal;
		}
	
	}


#pragma mark -
#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	SigmaBang(me)
 *	SigmaSet(me, iVal)
 *	SigmaInt(me, iVal)
 *	SigmaFloat(me, iVal)
 *	
 *	We implement a custom list handler so that only the end result is send out, rather
 *	then sending all the intermediary results
 *
 ******************************************************************************************/

void
SigmaBang(
	objSigma* me)
	
	{
	
	if (me->doFloats)
		 outlet_float(me->coreObject.o_outlet, me->sum);
	else outlet_int(me->coreObject.o_outlet, (long) me->sum);
	
	}

void
SigmaSet(
	objSigma*	me,
	double	iVal)
	
	{
	const long kInlet = ObjectGetInlet((Object*) me, me->inletNum);
	
	switch (me->inletCount) {
	case 2:
		// Simplest & most common situation
		if (me->doFloats) {
			me->data[kInlet].w_float = iVal;
			me->sum = me->data[0].w_float + me->data[1].w_float;
			}
		else {
			me->data[kInlet].w_long = (long) iVal;
			me->sum = me->data[0].w_long + me->data[1].w_long;
			}
		break;
	
	case 3:
		// Also pretty simple
		if (me->doFloats) {
			me->data[kInlet].w_float = iVal;
			me->sum = me->data[0].w_float + me->data[1].w_float + me->data[2].w_float;
			}
		else {
			me->data[kInlet].w_long = (long) iVal;
			me->sum = me->data[0].w_long + me->data[1].w_long + me->data[2].w_long;
			}
		break;
	
	default:
		UpdateSum(me, kInlet, iVal);
		break;
		}
	
	}


void SigmaInt(objSigma* me, long iVal)
	{ SigmaSet(me, (double) iVal); SigmaBang(me); }

void SigmaFloat(objSigma* me, double iVal)
	{ SigmaSet(me, iVal); SigmaBang(me); }
	

/******************************************************************************************
 *
 *	SigmaList(me)
 *
 *	We implement a custom list handler so that only the end result is sent out, rather
 *	then sending all the intermediary results
 *
 ******************************************************************************************/


void
SigmaList(
	objSigma*	me,
	Symbol*		sym,											// unused, must be "list"
	short		iArgCount,
	Atom*		iAtoms)
	
	{
	#pragma unused(sym)
	
	long	inletCount	= me->inletCount,
			curInlet	= ObjectGetInlet((Object*) me, me->inletNum);
	tWord*	curWord		= me->data + curInlet;
	
	
	// Make sure list is not too long for us
	if (iArgCount > inletCount - curInlet) {
		iArgCount = inletCount - curInlet;
		error("%s: truncating list to first %ld items",
				(char*) kClassName, (long) iArgCount);
		}
	
	if (me->doFloats) {
		do { (curWord++)->w_float = AtomGetFloat(iAtoms++); }
			while (--iArgCount > 0);
		RecalcFloats(me);
		}
	
	else {
		do { (curWord++)->w_long = AtomGetLong(iAtoms++); }
			while (--iArgCount > 0);
		RecalcInts(me);
		}
	
	// Send result out the bottom...
	SigmaBang(me);
	
	}

/******************************************************************************************
 *
 *	SigmaTattle(me)
 *
 * 	Tell all we know.
 *
 ******************************************************************************************/

void
SigmaTattle(
	objSigma*	me)
	
	{
	char	kTypeStr[]	= "\t adding %ld %s values";
	
	// Object state
	post("%s status", (char*) kClassName);
	
	// ... and then stuff specific to Sum
	if (me->doFloats) {
		post((char*) kTypeStr, me->inletCount, "floating point");
		post("\t current sum: %lf", me->sum);
		}
	else {
		post((char*) kTypeStr, me->inletCount, "integer");
		post("\t current sum: %ld", (long) me->sum);
		}
	
	}

/******************************************************************************************
 *
 *	SigmaAssist(me, iBox, iDir, iArgNum, oCStr)
 *	SigmaInfo(me)
 *
 ******************************************************************************************/

void
SigmaAssist(
	objSigma*	me,
	tBoxPtr		box,				// unused
	long		iDir,
	long		iArgNum,
	char*		oCStr)
	
	{
	#pragma unused(box)
	
	short fragIndex = me->doFloats ? strIndexFloat : strIndexInt;
	
	if (iDir == ASSIST_INLET)							// All inlets use the same string
		iArgNum = 0;
	
	LitterAssistResFrag(iDir, iArgNum, strIndexLeftIn, strIndexLeftOut, oCStr, fragIndex);
	
	}

void SigmaInfo(objSigma* me)
	{ LitterInfo(kClassName, (tObjectPtr) me, (method) SigmaTattle); }


