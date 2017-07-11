/*
	File:		/.c

	Contains:	Max/MSP external object calculating division and modulo. All inlets are "hot".

	Written by:	Peter Castine

	Copyright:	© 2003 Peter Castine

	Change History (most recent first):

         <8>      9–1–06    pc      Update to use LitterAssistResFrag() utility function.
         <7>    14–10–05    pc      Assistance method was using, incorrectly, the same string for
                                    both inlets. Fixed (part of the 2.0.2 revision).
         <6>    13–12–04    pc      Add 3rd outlet for modulo (as opposed to remainder). Update to
                                    use proxy_getinlet()
         <5>     11–1–04    pc      Update for modified LitterInit()
         <4>      8–1–04    pc      Update for Windows.
         <3>    6–7–2003    pc      Explicit tattle message takes A_NOTHING as arg list (not
                                    A_CANT). Fixed.
         <2>   30–3–2003    pc      Update kClassName
         <1>   11–3–2003    pc      First implementation, initial check-in
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "LitterLib.h"
#include "TrialPeriodUtils.h"

#pragma mark • Constants

const char*	kClassName		= "lp.logos";			// Class name

	// Indices for STR# resource
enum {
	strIndexNumInlet	= lpStrIndexLastStandard + 1,
	strIndexDenomInlet,
	
	strIndexQuotOutlet,										// Result
	strIndexRemdOutlet,										// Remainder
	strIndexModOutlet,										// Mod
	
	strIndexInt,
	strIndexFloat,
	
	strIndexLeftIn		= strIndexNumInlet,
	strIndexLeftOut		= strIndexQuotOutlet
	};

#pragma mark • Type Definitions


#pragma mark • Object Structure

typedef struct {
	Object		coreObject;
	
	long		inletNum;			// Max tells us which inlet was used here
	tWord		num,				// Numerator
				denom;				// Denominator
	tOutletPtr	outRemd,			// Remainder
				outMod;				// Modulo
	Boolean		doFloats;			// Are we doing integer arithmetic or fp?
	} objLogos;
typedef objLogos* objLogosPtr;


#pragma mark • Global Variables

#pragma mark • Function Prototypes

	// Class message functions
objLogosPtr	LogosNew(SymbolPtr, short, Atom[]);

	// Object message functions
static void LogosBang(objLogos*);
static void LogosFloat(objLogos*, double);
static void LogosInt(objLogos*, long);
static void LogosList(objLogos*, Symbol*, short, Atom*);
static void LogosSet(objLogos*, double);
static void LogosTattle(objLogos*);
static void	LogosAssist(objLogos*, tBoxPtr, long, long, char*);
static void	LogosInfo(objLogos*);

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
			(method) LogosNew,				// Instantiation method  
			NIL,							// No custom deallocation function
			(short) sizeof(objLogos),		// Class object size
			NIL,							// No menu function
			A_GIMME,						// Parse initialization arguments ourselves
			0);								// to determine if we're doing ints or floats
	
	// Messages
	LITTER_TIMEBOMB addbang	((method) LogosBang);
	LITTER_TIMEBOMB addint	((method) LogosInt);
	LITTER_TIMEBOMB addfloat((method) LogosFloat);
	LITTER_TIMEBOMB addmess	((method) LogosList,	"list",			A_GIMME, 0);
	addmess	((method) LogosSet,		"set",			A_FLOAT, 0);
	addmess	((method) LogosAssist,	"assist",		A_CANT, 0);
	addmess	((method) LogosInfo,	"info",			A_CANT, 0);
	addmess ((method) LogosTattle,	"tattle",	 	A_NOTHING);
	addmess ((method) LogosTattle,	"dblclick", 	A_CANT, 0);
	
	// Initialize Litter Library
	LitterInit(kClassName, 0);

	}

#pragma mark -
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	LogosNew(iSym, iArgCount, iArgVec)
 *
 ******************************************************************************************/

objLogosPtr
LogosNew(
	SymbolPtr	sym,								// must be 'lp./'
	short		iArgCount,
	Atom		iArgVec[])
	
	{
	#pragma unused(sym)
	
	objLogos*	me			= NIL;
	Boolean		doFloats	= false;
	double		num			= 1.0,
				denom		= 1.0;
	
	
	// Check argument list to see if there are any floats
	// ASSERT: iArgCoung >= 0;
	switch (iArgCount) {
	default:
		error("%s: ignoring extra arguments", kClassName);
		// fall into next case
	case 2:
		if (iArgVec[1].a_type == A_FLOAT) {
			doFloats = true;
			denom = iArgVec[1].a_w.w_float;
			}
		else denom = (double) AtomGetLong(iArgVec + 1);
		// fall into next case
	case 1:
		if (iArgVec[0].a_type == A_FLOAT) {
			doFloats = true;
			num = iArgVec[0].a_w.w_float;
			}
		else num = (double) AtomGetLong(iArgVec);
		// this is cheesy, but fall into next case;
	case 0:
		break;
		}
	
	// Let Max allocate us
	me = (objLogos*) newobject(gObjectClass);
	if (me == NIL) goto punt;
	
	// Add right inlet
	proxy_new(&me->coreObject, 1, &me->inletNum);
	
	// Add two outlets, right-to-left
	// We can grab the first one from the coreObject component
	if (doFloats) {
		me->outMod	= floatout(me);
		me->outRemd	= floatout(me);
		floatout(me);
		}
	else {
		me->outMod	= intout(me);
		me->outRemd	= intout(me);
		intout(me);
		}
	
	// Any other initialization
	if (doFloats) {
		me->num.w_float		= num;
		me->denom.w_float	= denom;
		}
	else {
		me->num.w_long		= (long) num;
		me->denom.w_long	= (long) denom;
		}
	me->doFloats	= doFloats;
	
punt:
	return me;
	}

#pragma mark -
#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	LogosBang(me)
 *
 ******************************************************************************************/

void
LogosBang(
	objLogos* me)
	
	{
	
	if (me->doFloats) {
		// Copy values to registers and calculate results
		double	denom	= me->denom.w_float,
				num		= me->num.w_float,
				quot	= num / denom,
				remd,
				mod;
		
		if (denom != 0.0) {
			remd = num - ((long) quot) * denom;
			mod  = (denom >= 0.0)
					? ((remd >= 0.0) ? remd : remd + denom)
					: ((remd <= 0.0) ? remd : remd + denom);
			}
		else {
			// Special case for division by zero
			remd = num;
			mod  = 0.0;
			}
		
		// Send out quotient and modulo in right-to-left order
		outlet_float(me->outMod, mod);
		outlet_float(me->outRemd, remd);
		outlet_float(me->coreObject.o_outlet, quot);
		}
	
	else {
		// Copy values to registers and calculate results
		long	denom	= me->denom.w_long,
				num		= me->num.w_long,
				quot,
				remd,
				mod;
		
		if (denom != 0) {
			quot = num / denom;
			remd = num - quot * denom;
			mod  = (denom >= 0)
					? ((remd >= 0) ? remd : remd + denom)
					: ((remd <= 0) ? remd : remd + denom);
			}
		else {
			// Special case for division by zero.
			// This is critical on Windows, which can crash on div-0
			// Handle analagously to IEEE-floats (which go to +/-infinity),
			// but treat 0/0 as zero rather than NaN.
			quot = (num > 0) ? kLongMax : (num < 0) ? kLongMin : 0;
			remd = num;
			mod  = 0;
			}
		
		// Send out quotient and remainder in right-to-left order
		outlet_int(me->outMod, mod);
		outlet_int(me->outRemd, remd);
		outlet_int(me->coreObject.o_outlet, quot);
		}
	
	}


/******************************************************************************************
 *
 *	LogosSet(me, iVal)
 *	LogosInt(me, iVal)
 *	LogosFloat(me, iVal)
 *	
 ******************************************************************************************/

void
LogosSet(
	objLogos*	me,
	double		iVal)
	
	{
	
	if (me->doFloats) {
		if (ObjectGetInlet((Object*) me, me->inletNum) == 1)
			me->denom.w_float = iVal;
		else me->num.w_float = iVal;
		}
	else {
		if (ObjectGetInlet((Object*) me, me->inletNum) == 1)
			me->denom.w_long = (long) iVal;
		else me->num.w_long = (long) iVal;
		}
		
	}


void LogosInt(objLogos* me, long iVal)
	{ LogosSet(me, (double) iVal); LogosBang(me); }

void LogosFloat(objLogos* me, double iVal)
	{ LogosSet(me, iVal); LogosBang(me); }
	

/******************************************************************************************
 *
 *	LogosList(me, iVal)
 *	
 *	We implement a custom list handler so that only the end result is send out, rather
 *	then sending all the intermediary results
 *
 ******************************************************************************************/

void
LogosList(
	objLogos*	me,
	Symbol*		sym,				// ignore, must be "list"
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
		if (me->doFloats)
			 me->denom.w_float = AtomGetFloat(iAtoms + 1 - kInletNum);
		else me->denom.w_long = AtomGetLong(iAtoms + 1 - kInletNum);
		if (kInletNum > 0)
			break;
		// otherwise fall into next case
	case 1:
		if (me->doFloats)
			 me->num.w_float = AtomGetFloat(iAtoms);
		else me->num.w_long = AtomGetLong(iAtoms);
		break;
	case 0:
		// This can't happen here
		break;
		}
	
	// Send result out the bottom...
	LogosBang(me);
	
	}

/******************************************************************************************
 *
 *	LogosTattle(me)
 *
 * 	Tell all we know.
 *
 ******************************************************************************************/

void
LogosTattle(
	objLogos*	me)
	
	{
	char kTypeStr[]	= "\t dividing %s values";
	
	
	// Object state
	post("%s status", (char*) kClassName);
	
	// ... and then stuff specific to Logos
	if (me->doFloats) {
		post((char*) kTypeStr, "floating point");
		post("\t operands: %lf ÷ %lf", (double) me->num.w_float, (double) me->denom.w_float);
		}
	else {
		post((char*) kTypeStr, "integer");
		post("\t operands: %ld ÷ %ld", me->num.w_long, me->denom.w_long);
		}
	
	}

/******************************************************************************************
 *
 *	LogosAssist(me, iBox, iDir, iArgNum, oCStr)
 *
 *	Fairly generic Assist Method
 *
 ******************************************************************************************/

void
LogosAssist(
	objLogos*	me,
	tBoxPtr		box,
	long		iDir,
	long		iArgNum,
	char*		oCStr)
	
	{
	#pragma unused(box)
	
	short fragIndex = me->doFloats ? strIndexFloat : strIndexInt;
	
	LitterAssistResFrag(iDir, iArgNum, strIndexLeftIn, strIndexLeftOut, oCStr, fragIndex);
	
	}

/******************************************************************************************
 *
 *	LogosInfo(me)
 *
 *	Fairly basic Info Method
 *
 ******************************************************************************************/

void LogosInfo(objLogos* me)
	{ LitterInfo(kClassName, (tObjectPtr) me, (method) LogosTattle); }

