/*
	File:		scampf.c

	Contains:	[Yet Another] map/scale object.

	Written by:	Peter Castine

	Copyright:	© 2001-2002 Peter Castine

	Change History (most recent first):

         <9>      9Ð1Ð06    pc      Correct some fibs in ScampTattle(). Update to use
                                    LitterAssistResFrag().
         <8>     14Ð1Ð04    pc      Fix assistance strings.
         <7>     10Ð1Ð04    pc      Update for modified LitterInit()
         <6>      8Ð1Ð04    pc      Update for Windows.
         <5>   30Ð3Ð2003    pc      Adapt for new scampi/scampf/scamp~ architecture. Add
                                    map/pow/exp/lin messages and initialization arguments. Migrate
                                    to new LitterInfo() call.
         <4>  30Ð12Ð2002    pc      Add object version to DoInfo()
         <3>  30Ð12Ð2002    pc      Use 'STR#' resource instead of faux 'Vers' resource for storing
                                    version information used at run-time.
         <2>  28Ð11Ð2002    pc      Tidy up after initial check in.
         <1>  28Ð11Ð2002    pc      Initial check in.
		15-Apr-2001:		First implementation.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark ¥ Include Files

#include "scampLib.h"
#include "TrialPeriodUtils.h"


#pragma mark ¥ Constants

#if (SCAMP_TARGET == SCAMPI)
	const char		kClassName[]	= "lp.scampi";			// Class name
#else
	const char		kClassName[]	= "lp.scampf";
#endif

// Indices for STR# resource
enum {
	strIndexInVal		= lpStrIndexLastStandard + 1,
	strIndexIn1,
	strIndexIn2,
	strIndexInMinOut,
	strIndexInMaxOut,
	strIndexInCurve,
	
	strIndexOutValue,
	strIndexOutNoSplit,
	strIndexOutIntSplit,
	strIndexOutSymSplit,
	
	strIndexFragScaleOffset,
	strIndexFragMap,
	strIndexFragFactor,
	strIndexFragOffset,
	strIndexFragInLow,
	strIndexFragInHigh,
	
	strIndexInLeft		= strIndexInVal,
	strIndexOutLeft		= strIndexOutValue
	};


#pragma mark ¥ Global Variables


#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark ¥ Inline Functions

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
	
	// Standard Max/Litter setup() call
	setup(	&gObjectClass,						// Pointer to our class definition
			(method) NewScamp,					// Instance creation function
			NIL,                				// No custom deallocation function
			(short) sizeof(objScamp),			// Class object size
			NIL,								// No menu function
			A_GIMME,							// Variable-length argument list
			0);
	
	// Generate global symbols
	//
	ScampGenSymbols();

#if (SCAMP_TARGET == SCAMPI)
	gTruncSym	= gensym("trunc");
	gRoundSym	= gensym("round");
	gFloorSym	= gensym("floor");
	gCeilSym	= gensym("ceil");
	gToInfSym	= gensym("toinf");
#endif
	
	// Messages
	//
	LITTER_TIMEBOMB ScampAddMessages();

	addftx	((method) ScampFloat1,		1);
	addftx	((method) ScampFloat2,		2);
	addftx	((method) ScampMinOut,		3);
	addftx	((method) ScampMaxOut,		4);
	addftx	((method) ScampCurve,		5);
	
	addmess	((method) ScampSet,			"set",		A_GIMME, 0);
	addmess	((method) ScampSplit,		"split",	A_GIMME, 0);

#if (SCAMP_TARGET == SCAMPI)
		// Rounding messages
	addmess ((method) ScampToZero,		"trunc",	A_NOTHING);
	addmess	((method) ScampToZero,		"tozero",	A_NOTHING);
	addmess	((method) ScampRound,		"round",	A_NOTHING);
	addmess	((method) ScampFloor,		"floor",	A_NOTHING);
	addmess	((method) ScampCeil,		"ceil",		A_NOTHING);
	addmess	((method) ScampToInf,		"toinf",	A_NOTHING);
#endif


	// Initialize Litter Library
	LitterInit(kClassName, 0);

	
	}


#pragma mark -
#pragma mark ¥ Instantiation

/******************************************************************************************
 *
 *	NewScamp(me)
 *	
 *	Uncharacteristically, we create the object first and then parse the argument list.
 *
 ******************************************************************************************/

objScamp*
NewScamp(
	Symbol*	iSym,
	short	iArgC,
	Atom*	iArgV)
	
	{
	#pragma unused (iSym)
	
	#if		(SCAMP_TARGET == SCAMPI)
		const double	kDefInMax		= 1.0,
						kDefOutMax		= 128.0;
		const eRoundMeth kDefRound		= roundToZero;
	#elif	(SCAMP_TARGET == SCAMPF)
		const double	kDefInMax		= 128.0,
						kDefOutMax		= 1.0;
	#endif
	
	const double		kDefSlope		= kDefOutMax / kDefInMax,
						kDefOffset		= 0.0,
						kDefInMin		= 0.0,
						kDefOutMin		= 0.0,
						kDefCurve		= 1.0;				// = CalcEffectiveCurve(0.0);
	const eActionType	kDefAction		= actStet;
	const eMapType		kDefMapType		= mapLin;
	const eSymmetry		kDefSymmetry	= symNone;
	const tScampFlags	kDefFlags		= {false, false, true};
	
	objScamp*	me	= NIL;
	
	
	
	//
	// Let Max allocate us, our inlets, and our outlets
	//
	
	me = (objScamp*) newobject(gObjectClass);
	if (me == NIL) {
		error("%s: insufficient memory to create object", (char*) kClassName);
		goto punt;
		}
	
	floatin(me, 5);					// Curve factor
	floatin(me, 4);					// Output Range upper bound
	floatin(me, 3);					// Output Range lower bound
	floatin(me, 2);					// Offset/Input range upper bound
	floatin(me, 1);					// Scaling factor/Input range lower bound
	
	me->out2 = outlet_new(me, NIL);
	floatout(me);					// Access this outlet via me->coreObject.o_outlet

	// Set up to default state
	me->split		= NIL;
	me->input		= 0.0;
	me->inMin		= kDefInMin;
	me->inMax		= kDefInMax;
	me->slope		= kDefSlope;
	me->offset		= kDefOffset;
	me->outMin		= kDefOutMin;
	me->outMax		= kDefOutMax;
	me->curve		= kDefCurve;
	
#if (SCAMP_TARGET == SCAMPI)
	me->rounding	= kDefRound;
#endif
	
	me->action		= kDefAction;
	me->mappingType = kDefMapType;
	me->symmetry	= kDefSymmetry;
	me->flags		= kDefFlags;
	
	// Parse arguments
	ScampParseArgs(me, iArgC, iArgV);
		
punt:
	return me;
	}

	
#pragma mark -
#pragma mark ¥ Calculation Methods

/******************************************************************************************
 *
 *	ScampBang(me)
 *	ScampSet(me, iSymbol, iArgC, iArgV)
 *
 ******************************************************************************************/

	static double
	ScampCalc(objScamp* me)
		
		{
		double	inOrigin,
				inRange,
				outOrigin	= me->outMin,
				outRange	= me->outMax - outOrigin,
				x;
		
		if (me->flags.rangeInval)
			ScampCalcRange(me);
		
		inOrigin = me->inMin;
		inRange = me->inMax - inOrigin;
		
		// Special handling for symmetry
		switch (me->symmetry) {
		case symPoint:
			outRange	/= 2.0;
			outOrigin	+= outRange;
			// fall into next case
		case symAxis:
			inRange		/= 2.0;
			inOrigin	+= inRange;
			// cheesy... but while we're falling through cases
		default:
			break;
			}
		
		// Map input value to unit square
		x = (me->input - inOrigin) / inRange;
		if (me->symmetry != symNone && x < 0.0)
			x = - x;
		
		// Calculate mapping inside unit	
		if (!me->flags.degenerate) {
			double k = me->curve;
			
			switch (me->mappingType) {
			default:										// mapLin
				if (x <= k / (k + 1.0))
					x /= k;
				else x = k * (x - 1.0) + 1.0;
				break;
			
			case mapPow:
				x = pow(x, k);
				break;
			
			case mapExp:
				x *= pow(k, x - 1.0);
				break;
				}
			}
		
		if (me->symmetry == symPoint && me->input < inOrigin)
			x = -x;
		
		// Map unit square to output range
		x *= outRange;
		x += outOrigin;
			
		return x;
		}

#if (SCAMP_TARGET == SCAMPI)
	static long DoRound(double iVal, eRoundMeth iMethod)
		{
		long result;
		
		switch (iMethod) {
			default:			result = iVal; break;					// roundToZero
			case roundRound:	result = (iVal < 0.0) ? iVal - 0.5 : iVal + 0.5; break;
			case roundFloor:	result = floor(iVal); break;
			case roundCeil:		result = ceil(iVal); break;
			case roundToInf:	result = (iVal < 0.0) ? floor(iVal) : ceil(iVal); break;
			}
		
		return result;
		}
#endif

void
ScampBang(
	objScamp* me)
	
	{
	double		outVal,
				min 		= me->outMin,
				max			= me->outMax;
	tOutletPtr	outlet		= LeftOutlet(me);
	
	outVal = ScampCalc(me);

#if (SCAMP_TARGET == SCAMPI)
	outVal = DoRound(outVal, me->rounding);
#endif
	
	if (outVal < min || outVal > max					// outside range bounds
			|| outVal != outVal) {						// NaN
		// Out of range. What needs to be done?
		
		// 0: Grab some values while we're declaring storage
		double	range = max - min;
		Symbol*	split = me->split;
		
		// 1: Correct value (if possible: requires a real number and a valid range)
		if (range > 0.0 && outVal == outVal) {
			switch (me->action) {
			case actClip:
				outVal = (outVal < min) ? min : max;
				break;
			
			case actWrap:
				outVal =fmod(outVal - min, range);
				if (outVal < 0)							// Compensate for C's lousy 
					outVal += range;					// modulo arithmetic
				outVal += min;
				break;
			
			case actReflect:
				{
				double range2 = range + range;
				outVal = fmod(outVal - min, range2);
				if (outVal < 0)							// Compensate for C's lousy 
					outVal += range2;					// modulo arithmetic
														// ASSERT: 0 <= outVal < range2
				if (outVal > range)
					outVal = range2 - outVal;			// ASSERT: 0 <= outVal < range
				outVal += min;							// ASSERT: min <= outVal < max
				}
				break;
			
			default:									// actStet
				break;
				}
			}
		
		// 2: Which outlet? Which message?
		if (split != NIL) {
			outlet = me->out2;
			if (split != kSplitValues) {
				outlet_anything(outlet, split, 0, NIL);
				outlet = NIL;								// Subvert the outlet_int() call
				}											// below
			}
		}
	
	if (outlet != NIL)
#if (SCAMP_TARGET == SCAMPI)
		outlet_int(outlet, outVal);
#else
		outlet_float(outlet, outVal);
#endif
	
	}

void
ScampSet(
	objScamp*	me,
	Symbol*		sym,
	short		iArgC,
	Atom*		iArgV)
	
	{
	#pragma unused(sym)
	
	me->input = (iArgC > 0) ? AtomGetFloat(iArgV) : 0.0;
	
	}


#pragma mark -
#pragma mark ¥ Split Message

/******************************************************************************************
 *
 *	ScampSplit(me, iSym, iArgC, iArgV);
 *
 ******************************************************************************************/

void
ScampSplit(
	objScamp*	me,
	Symbol*		sym,
	short		iArgC,
	Atom*		iArgV)
	
	{
	#pragma unused(sym)
	
	SymbolPtr	newSplitSym = NIL;
	
	if (iArgC > 0) {
		switch (iArgV->a_type) {
		case A_SYM:
			newSplitSym = iArgV->a_w.w_sym;
			break;
		case A_LONG:
			if (iArgV->a_w.w_long != 0)
				newSplitSym = kSplitValues;
			break;
		case A_FLOAT:
			if (iArgV->a_w.w_float != 0.0)
				newSplitSym = kSplitValues;
		default:
			// This shouldn't happen. 
			// And if it does, we clear the splitting option...
			// ...which means nothing to do here
			break;
			}
		}
	
	else newSplitSym = kSplitValues;
	
	me->split = newSplitSym;
	
	}


#pragma mark -
#pragma mark ¥ Rounding messages
/******************************************************************************************
 *
 *	ScampRound(me);
 *	ScampFloor(me);
 *	ScampCeil(me);
 *	ScampToInf(me);
 *	ScampToZero(me);
 *
 ******************************************************************************************/

#if	(SCAMP_TARGET == SCAMPI)
	void ScampRound(objScamp* me)		{ me->rounding = roundRound; }
	void ScampFloor(objScamp* me)		{ me->rounding = roundFloor; }
	void ScampCeil(objScamp* me)		{ me->rounding = roundCeil; }
	void ScampToInf(objScamp* me)		{ me->rounding = roundToInf; }
	void ScampToZero(objScamp* me)		{ me->rounding = roundToZero; }
#endif


#pragma mark -
#pragma mark ¥ Information Messaages

/******************************************************************************************
 *
 *	ScampTattle(me)
 *
 * 	Tell all we know.
 *
 ******************************************************************************************/

void
ScampTattle(
	objScamp*	me)
	
	{
	
	if (me->flags.rangeInval) ScampCalcRange(me);
	
	post("%s state:", kClassName);
	
	post("   current input value is %lf", me->input);
	
	if (!me->flags.map) {
		post("   scale = %lf", me->slope);
		post("   offset = %lf", me->offset);
		}
	post("   %sinput range is [%lf .. %lf]",
			me->flags.map ? "" : "nominal ", me->inMin, me->inMax);

#if (SCAMP_TARGET == SCAMPI)
	post("   output range is [%ld .. %ld]", me->outMin, me->outMax);
	post("   rounding method is %ld", (long) me->rounding);
	switch (me->rounding) {
		case roundToZero:	post("      to zero"); break;
		case roundRound:	post("      round"); break;
		case roundFloor:	post("      floor"); break;
		case roundCeil:		post("      ceiling"); break;
		case roundToInf:	post("      to +/- infinity"); break;
		default:			post("      invalid rounding method"); break;
		}
#else
	post("   output range is [%lf .. %lf]", me->outMin, me->outMax);
#endif
	
	post("   range correction %ld", (long) me->action);
	switch (me->action) {
		case actStet:		post("      no range correction"); break;
		case actClip:		post("      clip out-of-range values"); break;
		case actWrap:		post("      wrap out-of-range values"); break;
		case actReflect:	post("      reflect out-of-range values"); break;
		default:			post("      invalid range correction setting"); break;
		}
	
	post("   symmetry type %ld", me->symmetry);
	switch (me->symmetry) {
		case symNone:	post("      --none--"); break;	
		case symPoint:	post("      point symmetry"); break;	
		case symAxis:	post("      axis symmetry"); break;	
		default:		post("      invalid symmetry setting"); break;	
		}
	
	post("   mapping type %ld", me->mappingType);
	switch (me->mappingType) {
	case mapLin:
		post("      linear %s", me->flags.degenerate ? "" : "w/breakpoint factor");
		break;	
	case mapPow:
		post("      power/parabolic %s", me->flags.degenerate ? "(degenerate)" : "to power");
		break;	
	case symAxis:
		post("      exponential %s", me->flags.degenerate ? "(degenerate)" : "base");
		break;	
	default:
		post("      invalid symmetry setting");
		break;	
		}
	
	if (!me->flags.degenerate) PostFloat(me->curve);
	
	}

/******************************************************************************************
 *
 *	ScampAssist
 *
 *	Fairly generic Assist Method
 *
 ******************************************************************************************/

void
ScampAssist(
	objScamp*	me,
	void*		box,
	long		iDir,
	long		iArgNum,
	char*		oCStr)
	
	{
	#pragma unused(box)
	
	enum {
		inletInVal	= 0,
		inletScale,
		inletOffset,
		inletOutLow,
		inletOutHigh,
		
		outletMain	= 0,
		outletSplit
		};
	
	if (iDir == ASSIST_INLET) {
		short	strIndex;
		
		switch (iArgNum) {
		case inletInVal:
			strIndex = me->flags.map ? strIndexFragMap : strIndexFragScaleOffset;
			break;
		case inletScale:
			strIndex = me->flags.map ? strIndexFragInLow : strIndexFragFactor;
			break;
		case inletOffset:
			strIndex = me->flags.map ? strIndexFragInHigh : strIndexFragOffset;
			break;
		default:
			strIndex = 0;
			break;
			}
		
		if (strIndex > 0)
			 LitterAssistResFrag(iDir, iArgNum, strIndexInLeft, 0, oCStr, strIndex);
		else LitterAssist(iDir, iArgNum, strIndexInLeft, 0, oCStr);
		
		}
	
	else {
		// Do we need to customize the Assist string for the right ('split') outlet?
		short	outIndex;
		char*	splitSymName = NIL;
		
		switch ((long) me->split) {
		case (long) NIL:
			outIndex = strIndexOutNoSplit;
			break;
		case (long) kSplitValues:
			outIndex = strIndexOutIntSplit;
			break;
		default:
			// ASSERT: me->split points to a valid Symbol
			outIndex = strIndexOutSymSplit;
			splitSymName = me->split->s_name;
			}
		
		LitterAssistVA(iDir, iArgNum, strIndexInLeft, outIndex - 1, oCStr, splitSymName);
		}
	
	}

