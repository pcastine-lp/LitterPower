/*
	File:		-.c

	Contains:	Max/MSP external object calculating differences. All inlets are "hot".

	Written by:	Peter Castine

	Copyright:	© 2003 Peter Castine

	Change History (most recent first):

         <7>      9–1–06    pc      Update to use LitterAssistResFrag() utility function.
         <6>    13–12–04    pc      Update to use proxy_getinlet(). Also update function names to
                                    match object name for the benefit of crash logs.
         <5>     11–1–04    pc      Update for modified LitterInit()
         <4>      8–1–04    pc      Update for Windows.
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

const char*	kClassName		= "lp.delta";			// Class name

	// Indices for STR# resource
enum {
	strIndexAnyIn			= lpStrIndexLastStandard + 1,
	strIndexLROut,											// Left - Right
	strIndexRLOut,											// Right - Left
	strIndexAbsOut,											// Abs(Left - Right)
	
	strIndexInt,
	strIndexFloat
	};

#pragma mark • Type Definitions


#pragma mark • Object Structure

typedef struct {
	Object		coreObject;
	
	long		inletNum;			// Max tells us which inlet was used here
	double		left,
				right;
	tOutletPtr	outDiffRL,
				outDiffAbs;
	Boolean		doFloats;			// Are we doing integer arithmetic or fp?
	} objDelta;
typedef objDelta* objDeltaPtr;


#pragma mark • Global Variables

#pragma mark • Function Prototypes

	// Class message functions
objDeltaPtr	DeltaNew(SymbolPtr, short, Atom[]);

	// Object message functions
static void DeltaBang(objDelta*);
static void DeltaFloat(objDelta*, double);
static void DeltaInt(objDelta*, long);
static void DeltaList(objDelta*, Symbol*, short, Atom*);
static void DeltaSet(objDelta*, double);
static void DeltaTattle(objDelta*);
static void	DeltaAssist(objDelta*, tBoxPtr, long, long, char*);
static void	DeltaInfo(objDelta*);

#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark • Inline Functions

#pragma mark -

/******************************************************************************************
 *
 *	main()
 *	
 *	Standard Max External Object Entry Point Function
 *	
 ******************************************************************************************/

void
main()
	
	{
	LITTER_CHECKTIMEOUT(kClassName);
	
	// Standard Max setup() call
	setup(	&gObjectClass,					// Pointer to our class definition
			(method) DeltaNew,				// Instantiation method  
			NIL,							// No custom deallocation function
			(short) sizeof(objDelta),		// Class object size
			NIL,							// No menu function
			A_GIMME,						// Parse initialization arguments ourselves
			0);								// to determine if we're doing ints or floats

	// Messages
	LITTER_TIMEBOMB addbang	((method) DeltaBang);
	LITTER_TIMEBOMB addint	((method) DeltaInt);
	LITTER_TIMEBOMB addfloat((method) DeltaFloat);
	LITTER_TIMEBOMB addmess	((method) DeltaList,	"list",			A_GIMME, 0);
	addmess	((method) DeltaSet,		"set",			A_FLOAT, 0);
	addmess	((method) DeltaAssist,	"assist",		A_CANT, 0);
	addmess	((method) DeltaInfo,	"info",			A_CANT, 0);
	addmess ((method) DeltaTattle,	"tattle",	 	A_NOTHING);
	addmess ((method) DeltaTattle,	"dblclick", 	A_CANT, 0);
	
	// Initialize Litter Library
	LitterInit(kClassName, 0);

	}

#pragma mark -
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	DeltaNew(iSym, iArgCount, iArgVec)
 *
 ******************************************************************************************/

objDelta*
DeltaNew(
	SymbolPtr	sym,								// must be 'lp.-'
	short		iArgCount,
	Atom		iArgVec[])
	
	{
	#pragma unused(sym)
	
	objDelta*	me			= NIL;
	Boolean		doFloats	= false;
	double		left		= 0.0,
				right		= 0.0;
	
	
	// Check argument list to see if there are any floats
	// ASSERT: iArgCoung >= 0;
	switch (iArgCount) {
	default:
		error("%s: ignoring extra arguments", (char*) kClassName);
		// fall into next case
	case 2:
		if (iArgVec[1].a_type == A_FLOAT) {
			doFloats = true;
			right = iArgVec[1].a_w.w_float;
			}
		else right = (double) AtomGetLong(iArgVec + 1);
		// fall into next case
	case 1:
		if (iArgVec[0].a_type == A_FLOAT) {
			doFloats = true;
			left = iArgVec[0].a_w.w_float;
			}
		else left = (double) AtomGetLong(iArgVec);
		// this is cheesy, but fall into next case;
	case 0:
		break;
		}
	
	// Let Max allocate us
	me = (objDelta*) newobject(gObjectClass);
	if (me == NIL) goto punt;
	
	// Add right inlet
	proxy_new(&me->coreObject, 1, &me->inletNum);
	
	// Add three outlet, right-to-left
	// We can grab the first one from the coreObject component
	if (doFloats) {
		 me->outDiffAbs = floatout(me);
		 me->outDiffRL	= floatout(me);
		 floatout(me);
		 }
	else {
		 me->outDiffAbs = intout(me);
		 me->outDiffRL	= intout(me);
		 intout(me);
		}
	
	// Any other initialization
	me->left		= left;
	me->right		= right;
	me->doFloats	= doFloats;
	
punt:
	return me;
	}

#pragma mark -
#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	DeltaBang(me)
 *
 ******************************************************************************************/

void
DeltaBang(
	objDelta* me)
	
	{
	double	lr = me->left - me->right,
			rl = -lr;
	
	
	// Send out differences in right-to-left order
	if (me->doFloats) {
		outlet_float(me->outDiffAbs, (lr > 0) ? lr : rl);
		outlet_float(me->outDiffRL, rl);
		outlet_float(me->coreObject.o_outlet, lr);
		}
	else {
		long	longLR = (long) lr,
				longRL = -longLR;
		outlet_int(me->outDiffAbs, (long) ((longLR > 0) ? longLR : longRL));
		outlet_int(me->outDiffRL,(long) (longRL));
		outlet_int(me->coreObject.o_outlet, (long) longLR);
		}
	
	}


/******************************************************************************************
 *
 *	DeltaInt(me, iVal)
 *	DeltaFloat(me, iVal)
 *	DeltaList(me, iVal)
 *	
 *	We implement a custom list handler so that only the end result is send out, rather
 *	then sending all the intermediary results
 *
 ******************************************************************************************/

void
DeltaSet(
	objDelta*	me,
	double		iVal)
	
	{
	if (ObjectGetInlet((Object*) me, me->inletNum) == 1)
		me->right = iVal;
	else me->left = iVal;
	}


void DeltaInt(objDelta* me, long iVal)
	{ DeltaSet(me, (double) iVal); DeltaBang(me); }

void
DeltaFloat(objDelta* me, double iVal)
	{ DeltaSet(me, iVal); DeltaBang(me); }

void
DeltaList(
	objDelta*	me,
	Symbol*		sym,			// ignore, must be "list"
	short		iArgCount,
	Atom*		iAtoms)
	
	{
	#pragma unused(sym)
	
	const long kInletNum = ObjectGetInlet((Object*) me, me->inletNum);
	
	switch (iArgCount + kInletNum) {
	default:
		error("%s: truncating list to first %ld items",
				(char*) kClassName, (long) iArgCount - kInletNum - 1);
		// fall into next case
	case 2:
		me->right = AtomGetFloat(iAtoms + 1 - kInletNum);
		if (kInletNum > 0)
			break;
		// otherwise fall into next case
	case 1:
		me->left = AtomGetFloat(iAtoms);
		break;
	case 0:
		// This can't happen here
		break;
		}
	
	// Send result out the bottom...
	DeltaBang(me);
	
	}

/******************************************************************************************
 *
 *	DeltaTattle(me)
 *
 * 	Tell all we know.
 *
 ******************************************************************************************/

void
DeltaTattle(
	objDelta*	me)
	
	{
	char	kTypeStr[]	= "\t subtracting %s values";
	
	
	// Object state
	post("%s status", (char*) kClassName);
	
	// ... and then stuff specific to Diff
	if (me->doFloats) {
		post((char*) kTypeStr, "floating point");
		post("\t operands: %lf - %lf", me->left, me->right);
		}
	else {
		post((char*) kTypeStr, "integer");
		post("\t operands: %ld - %ld", (long) me->left, (long) me->right);
		}
	
	}

/******************************************************************************************
 *
 *	DeltaAssist(me, iBox, iDir, iArgNum, oCStr)
 *	DeltaInfo(me)
 *
 *	Fairly generic Assist Method
 *
 ******************************************************************************************/

void
DeltaAssist(
	objDelta*	me,
	tBoxPtr		box,
	long		iDir,
	long		iArgNum,
	char*		oCStr)
	
	{
	#pragma unused(box)
	
	short	fragIndex = me->doFloats ? strIndexFloat : strIndexInt;
	
	
	if (iDir == ASSIST_INLET)		// Both inlets use the same string
		iArgNum = 0;
	
	LitterAssistResFrag(iDir, iArgNum, strIndexAnyIn, strIndexLROut, oCStr, fragIndex);
	
	}

void DeltaInfo(objDelta* me)
	{ LitterInfo(kClassName, (tObjectPtr) me, (method) DeltaTattle); }

