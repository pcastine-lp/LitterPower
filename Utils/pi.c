/*
	File:		*.c

	Contains:	Max/MSP external object Product.

	Written by:	Peter Castine

	Copyright:	© 2003 Peter Castine

	Change History (most recent first):

         <8>      9–1–06    pc      Update to use LitterAssistResFrag() utility function.
         <7>    13–12–04    pc      Update to use proxy_getinlet(). Also updated function names to
                                    match object name for the benefit of crash logs.
         <6>     11–1–04    pc      Update for modified LitterInit()
         <5>      8–1–04    pc      Update for Windows.
         <4>    8–7–2003    pc      Problem with list input. Fixed.
         <3>    6–7–2003    pc      Explicit tattle message takes A_NOTHING as arg list (not
                                    A_CANT). Fixed.
         <2>   30–3–2003    pc      Update kClassName
         <1>    8–3–2003    pc      First implementation and check-in.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "LitterLib.h"
#include "TrialPeriodUtils.h"


#pragma mark • Constants

const char*	kClassName		= "lp.pi";			// Class name

	// Indices for STR# resource
enum {
	strIndexAnyInlet		= lpStrIndexLastStandard + 1,
	strIndexPiOutlet,
	
	strIndexInt,
	strIndexFloat,
	
	strIndexLeftIn			= strIndexAnyInlet,
	strIndexLeftOut			= strIndexPiOutlet
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
	double		product;			// Result. This is another way of not having to
									// worry about whether we're doing integer or fp
									// arithmetic.
	} objPi;
typedef objPi* objPiPtr;


#pragma mark • Global Variables

#pragma mark • Function Prototypes

	// Class message functions
objPiPtr	PiNew(SymbolPtr, short, Atom[]);
void		PiFree(objPi*);

	// Object message functions
static void PiBang(objPi*);
static void PiFloat(objPi*, double);
static void PiInt(objPi*, long);
static void PiList(objPi*, Symbol*, short, Atom*);
static void PiSet(objPi*, double);
static void PiTattle(objPi*);
static void	PiAssist(objPi*, void*, long, long, char*);
static void	PiInfo(objPi*);

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
			(method) PiNew,					// Instantiation method  
			(method) PiFree,				// Custom deallocation function
			(short) sizeof(objPi),			// Class object size
			NIL,							// No menu function
			A_GIMME,						// Parse initialization arguments ourselves
			0);
	
	// Messages
	LITTER_TIMEBOMB addbang	((method) PiBang);
	LITTER_TIMEBOMB addint	((method) PiInt);
	LITTER_TIMEBOMB addfloat((method) PiFloat);
	LITTER_TIMEBOMB addmess	((method) PiList,	"list",			A_GIMME, 0);
	addmess	((method) PiSet,	"set",			A_FLOAT, 0);
	addmess	((method) PiAssist,	"assist",		A_CANT, 0);
	addmess	((method) PiInfo,	"info",			A_CANT, 0);
	addmess ((method) PiTattle,	"tattle",	 	A_NOTHING);
	addmess ((method) PiTattle,	"dblclick", 	A_CANT, 0);
	
	// Initialize Litter Library
	LitterInit(kClassName, 0);

	}

#pragma mark -
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	PiNew(iSym, iArgCount, iArgVec)
 *	PiFree(me)
 *
 ******************************************************************************************/

objPi*
PiNew(
	SymbolPtr	sym,								// must be 'lp.*'
	short		iArgCount,
	Atom		iArgVec[])
	
	{
	#pragma unused(sym)
	
	const short	kMinInlets	= 2,
				kInlets		= (iArgCount < kMinInlets) ? kMinInlets : iArgCount;
	
	objPi*		me			= NIL;
	tWordPtr	data;
	double		product;
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
	me = (objPi*) newobject(gObjectClass);
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
	product = 1.0;
	if (doFloats)
		switch (iArgCount) {
		case 0:
			data[1].w_float = data[0].w_float = 1.0;
			break;
		case 1:
			data[1].w_float = 1.0;
			product = data[0].w_float = iArgVec[0].a_w.w_float;
			break;
		default:
			for (c = 0; c < iArgCount; c += 1)
				product *= data[c].w_float = AtomGetFloat(iArgVec + c);
			break;
			}
	else switch (iArgCount) {
		case 0:
			data[1].w_long = data[0].w_long = 1;
			break;
		case 1:
			data[1].w_long = 1;
			product = data[0].w_long = iArgVec[0].a_w.w_long;
			break;
		default:
			for (c = 0; c < iArgCount; c += 1)
				product *= data[c].w_long = AtomGetLong(iArgVec + c);
			break;
		}
	
	me->product		= product;
	me->doFloats	= doFloats;
	
punt:
	return me;
	}

void PiFree(objPi* me)
	{ if (me->data != NIL) freebytes(me->data, me->inletCount * sizeof(tWord)); }

#pragma mark -
#pragma mark • Utilities

/******************************************************************************************
 *
 *	RecalcFloats(me)
 *	RecalcInts(me)
 *	UpdateProduct(me)
 *
 ******************************************************************************************/

static void
RecalcFloats(
	objPi* me)
	
	{
	double	newProd = 1.0;
	int		terms	= me->inletCount;
	tWord*	curWord	= me->data;
	
	// ASSERT: (me->doFloats)
	do { newProd *= (curWord++)->w_float; }
		while (--terms > 0);
	
	me->product = newProd;
	
	}

static void
RecalcInts(
	objPi* me)
	
	{
	long	newProd = 1;
	int		terms	= me->inletCount;
	tWord*	curWord	= me->data;
	
	// ASSERT: (!me->doFloats)
	do { newProd *= (curWord++)->w_long; }
		while (--terms > 0);
	
	me->product = newProd;
	
	}

static void
UpdateProduct(
	objPi*	me,
	long		iIndex,
	double		iVal)
	
	{
	tWord*	curWord = me->data + iIndex;
	
	if (me->doFloats) {
		double curFloat = curWord->w_float;
		if (curFloat != 0.0) {
			me->product /= curFloat;
			me->product *= curWord->w_float = iVal;
			}
		else {
			curWord->w_float = iVal;
			RecalcFloats(me);
			}
		}
	else {
		
		if (curWord->w_long != 0) {
			me->product /= curWord->w_long;
			me->product *= curWord->w_long = (long) iVal;
			}
		else {
			curWord->w_long = (long) iVal;
			RecalcInts(me);
			}
		}
	
	}


#pragma mark -
#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	PiBang(me)
 *	PiSet(me, iVal)
 *	PiInt(me, iVal)
 *	PiFloat(me, iVal)
 *	
 ******************************************************************************************/

void
PiBang(
	objPi* me)
	
	{
	if (me->doFloats)
		 outlet_float(me->coreObject.o_outlet, me->product);
	else outlet_int(me->coreObject.o_outlet, (long) me->product);
	}


void PiSet(objPi* me, double iVal)
	{
	const long kInlet = ObjectGetInlet((Object*) me, me->inletNum);
	
	switch (me->inletCount) {
	case 2:
		// Simplest & most common situation
		if (me->doFloats) {
			me->data[kInlet].w_float = iVal;
			me->product = me->data[0].w_float * me->data[1].w_float;
			}
		else {
			me->data[kInlet].w_long = (long) iVal;
			me->product = me->data[0].w_long * me->data[1].w_long;
			}
		break;
	
	case 3:
		// Also pretty simple
		if (me->doFloats) {
			me->data[kInlet].w_float = iVal;
			me->product = me->data[0].w_float * me->data[1].w_float * me->data[2].w_float;
			}
		else {
			me->data[kInlet].w_long = (long) iVal;
			me->product = me->data[0].w_long * me->data[1].w_long * me->data[2].w_long;
			}
		break;
	
	default:
		UpdateProduct(me, kInlet, iVal);
		break;
		}
	
	}

void PiInt(objPi* me, long iVal)
	{ PiSet(me, (double) iVal); PiBang(me); }

void PiFloat(objPi* me, double iVal)
	{ PiSet(me, iVal); PiBang(me); }

/******************************************************************************************
 *
 *	PiList(me)
 *
 *	We implement a custom list handler so that only the end result is sent out, rather
 *	then sending all the intermediary results
 *
 ******************************************************************************************/

void
PiList(
	objPi*	me,
	Symbol* sym,													// ignore, must be "list"
	short	iArgCount,
	Atom*	iAtoms)
	
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
	
	PiBang(me);
	
	}


/******************************************************************************************
 *
 *	PiTattle(me)
 *
 * 	Tell all we know.
 *
 ******************************************************************************************/

void
PiTattle(
	objPi*	me)
	
	{
	char	kTypeStr[]	= "\t multiplying %ld %s values";
	
	
	// Object state
	post("%s status", (char*) kClassName);
	
	// ... and then stuff specific to Prod
	post((char*) kTypeStr, me->inletCount, me->doFloats ? "floating point" : "integer");
	post("\t current product: %lf", me->product);
	
	}

/******************************************************************************************
 *
 *	PiAssist(me, iBox, iDir, iArgNum, oCStr)
 *	PiInfo(me)
 *
 *	Fairly generic methods
 *
 ******************************************************************************************/

void
PiAssist(
	objPi*		me,
	tBoxPtr		box,
	long		iDir,
	long		iArgNum,
	char*		oCStr)
	
	{
	#pragma unused(box)
	
	short	fragIndex = me->doFloats ? strIndexFloat : strIndexInt;
	
	
	if (iDir == ASSIST_INLET)							// All inlets use the same string
		iArgNum = 0;
	
	LitterAssistResFrag(iDir, iArgNum, strIndexLeftIn, strIndexLeftOut, oCStr, fragIndex);
	
	}

void PiInfo(objPi* me)
	{ LitterInfo(kClassName, (tObjectPtr) me, (method) PiTattle); }


