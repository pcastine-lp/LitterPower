/*
	File:		scamp~.c

	Contains:	[Yet Another] map/scale object.

	Written by:	Peter Castine

	Copyright:	© 2001-2002 Peter Castine

	Change History (most recent first):

        <11>    8Ð5Ð2006    pc      Fix name conflict between the global enum Symmetry and the
                                    mapping enum Symmetry (now eMapSym)
        <10>      9Ð1Ð06    pc      Update to use LitterAssistResFrag().
         <9>      8Ð9Ð04    pc      Fix bug in LitterAssist found by Holland Hopson.
         <8>     10Ð1Ð04    pc      Update for modified LitterInit()
         <7>      8Ð1Ð04    pc      Update for Windows.
         <6>    6Ð7Ð2003    pc      Sort out default range/symmetry.
         <5>    9Ð4Ð2003    pc      Add STR# resource for LitterAddClass(). Bump version number.
         <4>  30Ð12Ð2002    pc      Add object version to DoInfo()
         <3>  30Ð12Ð2002    pc      Use 'STR#' resource instead of faux 'Vers' resource for storing
                                    version information used at run-time.
         <2>  28Ð11Ð2002    pc      Tidy up after initial check in.
         <1>  28Ð11Ð2002    pc      Initial check in.
*/


/******************************************************************************************
	Previous History
		7-July-2001:	First implementation.
 ******************************************************************************************/

#pragma mark ¥ Include Files

#include "scampLib.h"
#include "TrialPeriodUtils.h"

#pragma mark ¥ Constants

const char	kClassName[]	= "lp.scamp~";	    		// Class name

// Indices for STR# resource
enum {
	strIndexInSig		= lpStrIndexLastStandard + 1,
	strIndexIn1,
	strIndexIn2,
	strIndexInMinOut,
	strIndexInMaxOut,
	strIndexInCurve,
	
	strIndexOutSig,
	strIndexOutRngXCeptionCount,
	
	strIndexFragScale,
	strIndexFragMap,
	strIndexFragFactor,
	strIndexFragInLow,
	strIndexFragOffset,
	strIndexFragInHigh,

	strIndexInLeft		= strIndexInSig,
	strIndexOutLeft		= strIndexOutSig
	};

// Indices for MSP Inlets and Outlets (don't count plain-vanilla inlets or outlets).
enum {
	inletSigIn		= 0,
	inlet1,						// Can be either Scale or MinIn
	inlet2,						// Can be either Offset or MaxIn
	inletMinOut,
	inletMaxOut,
	inletCurve,
	
	outletSigOut
	};


#pragma mark ¥ Function Prototypes

	// MSP message functions
#ifdef __MAX_MSP_OBJECT__
	void	ScampDSP(objScamp*, t_signal**, short*);
#endif

int*	ScampPerformStatic(int*);
int*	ScampPerformDynamic(int*);


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
main(void)
	
	{
	LITTER_CHECKTIMEOUT(kClassName);
	
	// Standard Max/MSP/Litter setup() call
	setup(	&gObjectClass,						// Pointer to our class definition
			(method) NewScamp,					// Instance creation function
			(method) dsp_free,              	// Custom deallocation function
			(short) sizeof(objScamp),			// Class object size
			NIL,								// No menu function
			A_GIMME,							// Variable-length argument list
			0);
	
	dsp_initclass();
	
	// Messages
	//
	ScampAddMessages();
	
	LITTER_TIMEBOMB addmess	((method) ScampDSP, 	"dsp",		A_CANT, 0);

	// Initialize Litter Library
	LitterInit(kClassName, 0);

	// Generate global symbols
	//
	ScampGenSymbols();

	
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
	Symbol*	sym,
	short	iArgC,
	Atom*	iArgV)
	
	{
	#pragma unused(sym)
	
	const double		kDefSlope		= sqrt(0.5),
						kDefInMin		= -1.0,
						kDefInMax		= 1.0,
						kDefOutMin		= -kDefSlope,
						kDefOutMax		= kDefSlope,
						kDefOffset		= 0.0,
						kDefCurve		= 0.0;
	const eActionType	kDefAction		= actStet;
	const eMapType		kDefMapType		= mapLin;
	const eMapSym		kDefSymmetry	= symPoint;//symNone;
	const tScampFlags	kDefFlags		= {false, false, true};
	
	objScamp*	me	= NIL;
	
	//
	// Let Max allocate us, our inlets, and our outlets
	//
	
	me = (objScamp*) newobject(gObjectClass);
	if (me == NIL) {
		error("%s: insufficient memory to create object", kClassName);
		goto punt;
		}
	
	dsp_setup(&(me->coreObject), 6);		// Inlets
	
	me->out2 = intout(me);					// Outlets
	outlet_new(me, "signal");

	// Set up to default state
	me->limCounter	= 0;
	me->input		= 0.0;
	me->inMin		= kDefInMin;
	me->inMax		= kDefInMax;
	me->slope		= kDefSlope;
	me->offset		= kDefOffset;
	me->curve		= kDefCurve;
	me->outMin		= kDefOutMin;
	me->outMax		= kDefOutMax;
	me->action		= kDefAction;
	me->mappingType	= kDefMapType;
	me->symmetry	= kDefSymmetry;
	me->flags		= kDefFlags;
	
	ScampParseArgs(me, iArgC, iArgV);

punt:
	return me;
	}

	

#pragma mark -
#pragma mark ¥ Utilities

/******************************************************************************************
 *
 *	Clip(iVal, iMin, iMax, ioCounter)
 *	Wrap(iVal, iMin, iMax, ioCounter)
 *	Reflect(iVal, iMin, iMax, ioCounter)
 *	
 *	UnitClip(iVal, ioCounter)					// Variants assuming range is [0.0 .. 1.0]
 *	UnitWrap(iVal, ioCounter)
 *	UnitReflect(iVal, ioCounter)
 *	
 ******************************************************************************************/

typedef double (*tActFunc)(double, double, double, long*);
typedef double (*tUnitActFunc)(double, long*);

static double Clip(double iVal, double iMin, double iMax, long* ioCounter)
	{
	if (iVal < iMin) {
		iVal = iMin;
		*ioCounter += 1;
		}
	else if (iVal > iMax) {
		iVal = iMax;
		*ioCounter += 1;
		}
	
	return iVal;
	}

static double UnitClip(double iVal, long* ioCounter)
	{
	if (iVal < 0.0) {
		iVal = 0.0;
		*ioCounter += 1;
		}
	else if (iVal > 1.0) {
		iVal = 1.0;
		*ioCounter += 1;
		}
	
	return iVal;
	}

static double Wrap(double iVal, double iMin, double iMax, long* ioCounter)
	{
	if (iVal < iMin) {
		double range = iMax - iMin;							// Compensate for C's lousy 
		iVal = fmod(iVal - iMin, range) + range + iMin;		// modulo arithmetic
		*ioCounter += 1;
		}
	else if (iVal > iMax) {
		iVal = fmod(iVal - iMin, iMax - iMin) + iMin;
		*ioCounter += 1;
		}
	
	return iVal;
	}

static double UnitWrap(double iVal, long* ioCounter)
	{
	if (iVal < 0.0) {										// Compensate for C's lousy 
		iVal = fmod(iVal, 1.0) + 1.0;						// modulo arithmetic
		*ioCounter += 1;
		}
	else if (iVal > 1.0) {
		iVal = fmod(iVal, 1.0);
		*ioCounter += 1;
		}
	
	return iVal;
	}


static double Reflect(double iVal, double iMin, double iMax, long* ioCounter)
	{
	if (iVal < iMin) {
		double	range	= iMax - iMin,
				range2	= range + range;
		
		// Compensate for C's lousy modulo arithmetic
		iVal = fmod(iVal - iMin, range2) + range2;
		if (iVal > range)
			iVal = range2 - iVal;
		iVal += iMin;
		*ioCounter += 1;
		}
	
	else if (iVal > iMax) {
		double	range	= iMax - iMin,
				range2	= range + range;
		
		iVal = fmod(iVal - iMin, range2);
		if (iVal > range)
			iVal = range2 - iVal;
		iVal += iMin;
		*ioCounter += 1;
		}
	
	return iVal;
	}

static double UnitReflect(double iVal, long* ioCounter)
	{
	if (iVal < 0.0) {
		iVal = fmod(iVal, 2.0) + 2.0;	// Compensate for C's lousy modulo arithmetic
		if (iVal > 1.0)
			iVal = 2.0 - iVal;
		*ioCounter += 1;
		}
	else if (iVal > 1.0) {
		iVal = fmod(iVal, 2.0);
		if (iVal > 1.0)
			iVal = 2.0 - iVal;
		*ioCounter += 1;
		}
	
	return iVal;
	}

/******************************************************************************************
 *
 *	Kink(iVal, iKink)
 *	Power(iVal, iExp)
 *	Square(iVal, iDummy)		(Dummy parameter so we can use the same prototype)
 *	Exponent(iVal, iBase)
 *
 ******************************************************************************************/

typedef double (*tMapFunc)(double, double);

static double Kink(double iVal, double iKink)
	{ return (iVal <= iKink / (iKink + 1.0)) ? iVal / iKink : iKink * (iVal - 1.0) + 1.0; }
static double Power(double iVal, double iExp)
	{ return pow(iVal, iExp); }
static double Square(double iVal, double exp)	
	{
	#pragma unused(exp)
	
	return iVal * iVal;
	}
		
static double Exponent(double iVal, double iBase)
	{ return iVal * pow(iBase, iVal - 1.0); }

#pragma mark -
#pragma mark ¥ Object Methods

/******************************************************************************************
 *
 *	ScampBang(me)
 *
 ******************************************************************************************/

void
ScampBang(
	objScamp* me)
	
	{
	outlet_int(me->out2, me->limCounter);
	me->limCounter = 0;
	}


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
	const char*	kActionStr[]	= {"stet", "clip", "wrap", "reflect"};
	const char*	kMapStr[]		= {"linear", "power", "exponential"};
	const char*	kSymStr[]		= {"none", "point", "axis"};
	
	post("%s state:", kClassName);
	
	if (me->flags.rangeInval) {
		post("  ...updating range parameters...");
		ScampCalcRange(me);
		}
	
	post("  static input value is %lf", me->input);
	if (me->flags.map) {
		post("  map mode: input range [%lf .. %lf]", me->inMin, me->inMax);
		post("    extrapolated scale %lf, extrapoloated offset %lf", me->slope, me->offset);
		 }
	else {
		post("  scale mode: scale %lf, offset %lf", me->slope, me->offset);
		post("    extrapolated range: [%lf .. %lf]", me->inMin, me->inMax);
		}
	post("  output range: [%lf .. %lf]", me->outMin, me->outMax);
	if (me->flags.degenerate)
		 post("  parameters degenerate to linear mapping");
	else post("  curve: %lf", me->curve);
	
	post("  range limiting action %ld (%s)",
		(long) me->action, kActionStr[me->action]); 
	post("  mapping type %ld (%s)",
		(long) me->mappingType, kMapStr[me->mappingType]); 
	post("  symmetry type %ld (%s)",
		(long) me->symmetry, kSymStr[me->symmetry]); 
	
	}

/******************************************************************************************
 *
 *	ScampAssist
 *
 ******************************************************************************************/

void
ScampAssist(
	objScamp*	me,
	void*		box,
	long		iDir,
	long		iArgNum,
	char		oCDestStr[])
	
	{
	#pragma unused(box)
	
	short strIndex = 0;
	
	if (iDir == ASSIST_INLET) {
		switch (iArgNum) {
		case inletSigIn:
			strIndex = me->flags.map ? strIndexFragMap : strIndexFragScale;
			break;
		case inlet1:
			strIndex = me->flags.map ? strIndexFragInLow : strIndexFragFactor;
			break;
		case inlet2:
			strIndex = me->flags.map ? strIndexFragInHigh : strIndexFragOffset;
			break;
		default:
			break;
			}
		}
		
	if (strIndex > 0)
		 LitterAssistResFrag(iDir, iArgNum, strIndexInLeft, 0, oCDestStr, strIndex);
	else LitterAssist(iDir, iArgNum, strIndexInLeft, strIndexOutLeft, oCDestStr);
	
	}

	

#pragma mark -
#pragma mark ¥ DSP Methods

/******************************************************************************************
 *
 *	ScampDSP(me, ioDSPVectors, iConnectCounts)
 *
 ******************************************************************************************/

void
ScampDSP(
	objScamp*	me,
	t_signal**	ioDSPVectors,
	short*		iConnectCounts)
	
	{

	if (iConnectCounts[outletSigOut] > 0) {
		Boolean	isStatic = iConnectCounts[inlet1] == 0
							&& iConnectCounts[inlet2] == 0
							&& iConnectCounts[inletMinOut] == 0
							&& iConnectCounts[inletMaxOut] == 0
							&& iConnectCounts[inletCurve] == 0;
		
		if (isStatic)
			dsp_add(ScampPerformStatic, 4,
					me, (long) ioDSPVectors[inletSigIn]->s_n,
					iConnectCounts[inletSigIn] > 0 ? ioDSPVectors[inletSigIn]->s_vec : NIL,
					ioDSPVectors[outletSigOut]->s_vec
					);
		else dsp_add(ScampPerformDynamic, 9,
					me, (long) ioDSPVectors[inletSigIn]->s_n,
					iConnectCounts[inletSigIn] > 0	? ioDSPVectors[inletSigIn]->s_vec	: NIL,
					iConnectCounts[inlet1] > 0		? ioDSPVectors[inlet1]->s_vec		: NIL,
					iConnectCounts[inlet2] > 0		? ioDSPVectors[inlet2]->s_vec		: NIL,
					iConnectCounts[inletMinOut] > 0	? ioDSPVectors[inletMinOut]->s_vec	: NIL,
					iConnectCounts[inletMaxOut] > 0	? ioDSPVectors[inletMaxOut]->s_vec	: NIL,
					iConnectCounts[inletCurve] > 0	? ioDSPVectors[inletCurve]->s_vec	: NIL,
					ioDSPVectors[outletSigOut]->s_vec
					);
		}
	
	}
	

/******************************************************************************************
 *
 *	PerformStatLinStet(me, iVecSize, iSigIn, oSigOut)
 *	PerformStatLinLim(me, iVecSize, iSigIn, oSigOut)
 *	PerformStatCurveStet(me, iVecSize, iSigIn, oSigOut)
 *	PerformStatCurveLim(me, iVecSize, iSigIn, oSigOut)
 *	
 *	ScampPerformStatic(iParams)
 *	
 *	DSP Perform methods for static parameters
 *
 ******************************************************************************************/

static void
PerformStatLinStet(
	objScamp*		me,
	long			iVecSize,
	tSampleVector	iSigIn,
	tSampleVector	oSigOut)
	
	{
	double	scale	= me->slope,
			offset	= me->offset;
	
	if (iSigIn != NIL) {
		double curSamp;
		
		if (me->symmetry == symAxis) do {
			curSamp  = *iSigIn++;
			curSamp *= (curSamp >= 0.0) ? scale : -scale;
			curSamp += offset;
			
			*oSigOut++ = curSamp;
			} while (--iVecSize > 0);
		
		else do {
			curSamp  = *iSigIn++;
			curSamp *= scale;
			curSamp += offset;
			
			*oSigOut++ = curSamp;
			} while (--iVecSize > 0);
		}
		 	
	else {							// Unusual situation: generating signal from all-static input
		double outVal = me->input;
		
		if (me->symmetry == symAxis && outVal < 0.0)
			outVal = -outVal;
		outVal *= scale;
		outVal += offset;
		
		do { *oSigOut++ = outVal; } while (--iVecSize > 0);
		}
	
	}
	
static void
PerformStatLinLim(
	objScamp*		me,
	long			iVecSize,
	tSampleVector	iSigIn,
	tSampleVector	oSigOut)

	{
	double		scale	= me->slope,
				offset	= me->offset,
				outMin	= me->outMin,
				outMax	= me->outMax;
	long*		ctr		= &me->limCounter;
	tActFunc	limFunc;
	
	switch (me->action) {
		case actWrap:		limFunc = Wrap;		break;
		case actReflect:	limFunc = Reflect;	break;
		default:			limFunc = Clip;		break;		// ASSERT: me->action == act Clip
		}
	
	if (iSigIn != NIL) {
		double curSamp;
		
		if (me->symmetry == symAxis) do {
			curSamp  = *iSigIn++;
			curSamp *= (curSamp >= 0.0) ? scale : -scale;
			curSamp += offset;
			
			*oSigOut++ = limFunc(curSamp, outMin, outMax, ctr);
			} while (--iVecSize > 0);
		
		else do {
			curSamp  = *iSigIn++;
			curSamp *= scale;
			curSamp += offset;
			
			*oSigOut++ = limFunc(curSamp, outMin, outMax, ctr);
			} while (--iVecSize > 0);
		}
		 	
	else {							// Unusual situation: generating signal from all-static input
		double outVal = me->input;
		
		if (me->symmetry == symAxis && outVal < 0.0)
			outVal = -outVal;
		outVal = limFunc(outVal * scale + offset, outMin, outMax, ctr);
		
		do { *oSigOut++ = outVal; } while (--iVecSize > 0);
		}
	
	}

static void
PerformStatCurveStet(
	objScamp*		me,
	long			iVecSize,
	tSampleVector	iSigIn,
	tSampleVector	oSigOut)
	
	{
	double		inOrigin	= me->inMin,
				inRange		= me->inMax - inOrigin,
				inScale		= 1.0 / inRange,
				outOffset	= me->outMin,
				outRange	= me->outMax - outOffset,
				curve		= me->curve;
	tMapFunc	mapFunc;
	
	switch (me->mappingType) {
	default:								// ASSERT: me->mappingType == mapLin
		mapFunc = Kink;
		break;			
	case mapPow:
		mapFunc = (curve == 2.0) ? Square : Power;
		break;
	case mapExp:
		mapFunc = Exponent;
		break;
		}
	
	if (iSigIn != NIL) switch (me->symmetry) {
		double curSamp;
		
		default:												// Must be symNone
			do {
				curSamp	 = (*iSigIn++ - inOrigin) * inScale;	// Optimization over MapToUnit()
				curSamp	 = mapFunc(curSamp, curve);
				*oSigOut++ = MapFromUnit(curSamp, outOffset, outRange);
				} while (--iVecSize > 0);
			break;
		
		case symAxis:
			inOrigin += inRange * 0.5;
			inScale += inScale;
			
			do {
				curSamp	 = *iSigIn++ - inOrigin;
				curSamp *= (curSamp >= 0.0) ? inScale : -inScale;
				curSamp	 = mapFunc(curSamp, curve);
				*oSigOut++ = MapFromUnit(curSamp, outOffset, outRange);
				} while (--iVecSize > 0);
			break;
		
		case symPoint:
			outRange	*= 0.5;
			outOffset	+= outRange;
			inOrigin	+= inRange * 0.5;
			inScale		+= inScale;
			
			do {
				curSamp = *iSigIn++ - inOrigin;
				
				if (curSamp >= 0.0) {
					curSamp *= inScale;
					curSamp  = mapFunc(curSamp, curve);
					*oSigOut++ = MapFromUnit(curSamp, outOffset, outRange);
					}
				else {
					curSamp *= -inScale;
					curSamp  = mapFunc(curSamp, curve);
					*oSigOut++ = MapFromUnit(curSamp, outOffset, -outRange);
					}
				
				} while (--iVecSize > 0);
			break;
		}
	
	else {							// Unusual situation: generating signal from all-static input
		double outVal = me->input - inOrigin;
		
		switch (me->symmetry) {
		default:
			// Must be symNone
			outVal *= inScale;
			outVal  = mapFunc(outVal, curve);
			outVal  = MapFromUnit(outVal, outOffset, outRange);
				
			do { *oSigOut++ = outVal; } while (--iVecSize > 0);
			break;
		
		case symAxis:
			inScale += inScale;							// Double scale factor for symmetry
			
			outVal -= inRange * 0.5;					// Correct origin
			outVal *= (outVal < 0.0) ? -inScale : inScale;
			outVal	= mapFunc(outVal, curve);
			outVal  = MapFromUnit(outVal, outOffset, outRange);
				
			do { *oSigOut++ = outVal; } while (--iVecSize > 0);
			break;
		
		case symPoint:
			inScale		+= inScale;						// Double scale factor for symmetry
			outRange	*= 0.5;							// and adjust output range
			outOffset	+= outRange;
			
			outVal -= inRange * 0.5;					// Correct origin
			if (outVal >= 0.0) {
				outVal *= inScale;
				outVal  = mapFunc(outVal, curve);
				outVal  = MapFromUnit(outVal, outOffset, outRange);
				}
			else {
				outVal *= -inScale;
				outVal  = mapFunc(outVal, curve);
				outVal  = MapFromUnit(outVal, outOffset, -outRange);
				}
			
			do { *oSigOut++ = outVal; } while (--iVecSize > 0);
			break;
			}
		}
	
	}
	
	
static void
PerformStatCurveLim(
	objScamp*		me,
	long			iVecSize,
	tSampleVector	iSigIn,
	tSampleVector	oSigOut)
	
	{
	double			inOrigin	= me->inMin,
					inRange		= me->inMax - inOrigin,
					inScale		= 1.0 / inRange,
					outOffset	= me->outMin,
					outRange	= me->outMax - outOffset,
					curve		= me->curve;
	long*			ctr		= &me->limCounter;
	tMapFunc		mapFunc;
	tUnitActFunc	limFunc;
	
	switch (me->mappingType) {
		default:		mapFunc = Kink;		break;			// ASSERT: me->mappingType == mapLin
		case mapPow:	mapFunc = Power;	break;
		case mapExp:	mapFunc = Exponent;	break;
		}
	
	switch (me->action) {
		default:			limFunc = UnitClip;		break;		// ASSERT: me->action == actClip
		case actWrap:		limFunc = UnitWrap;		break;
		case actReflect:	limFunc = UnitReflect;	break;
		}
	
	if (iSigIn != NIL) switch (me->symmetry) {
		double curSamp;
		
		default:
			// Must be symNone
			do {
				curSamp	 = (*iSigIn++ - inOrigin) * inScale;	// Optimization over MapToUnit()
				curSamp	 = limFunc(mapFunc(curSamp, curve), ctr);
				*oSigOut++ = MapFromUnit(curSamp, outOffset, outRange);
				} while (--iVecSize > 0);
			break;
		
		case symAxis:
			inOrigin += inRange * 0.5;
			inScale += inScale;
			
			do {
				curSamp	 = *iSigIn++ - inOrigin;	// Optimization over MapToUnit()
				curSamp *= (curSamp < 0.0) ? -inScale : inScale;
				curSamp	 = limFunc(mapFunc(curSamp, curve), ctr);
				*oSigOut++ = MapFromUnit(curSamp, outOffset, outRange);
				} while (--iVecSize > 0);
			break;
		
		case symPoint:
			inOrigin	+= inRange * 0.5;
			inScale		+= inScale;
			outRange	*= 0.5;
			outOffset	+= outRange;
			
			do {
				Boolean	flipPoint;
				
				curSamp	 = *iSigIn++;
				curSamp -= inOrigin;
				curSamp *= ( flipPoint = (curSamp < 0.0) ) ? inScale : -inScale;
				curSamp	 = limFunc(mapFunc(curSamp, curve), ctr);
				*oSigOut++ = MapFromUnit(flipPoint ? -curSamp : curSamp, outOffset, outRange);
				} while (--iVecSize > 0);
			break;
		}
	
	else {
		// Unusual situation: generating signal from all-static input
		double outVal = me->input - inOrigin;
		
		switch (me->symmetry) {
		default:
			// Must be symNone
			outVal *= inScale;
			outVal  = limFunc(mapFunc(outVal, curve), ctr);
			outVal  = MapFromUnit(outVal, outOffset, outRange);
				
			do { *oSigOut++ = outVal; } while (--iVecSize > 0);
			break;
		
		case symAxis:
			inScale += inScale;							// Double scale factor for symmetry
			
			outVal -= inRange * 0.5;					// Correct origin
			outVal *= (outVal < 0.0) ? -inScale : inScale;
			outVal	= limFunc(mapFunc(outVal, curve), ctr);
			outVal  = MapFromUnit(outVal, outOffset, outRange);
				
			do { *oSigOut++ = outVal; } while (--iVecSize > 0);
			break;
		
		case symPoint:
			inScale		+= inScale;						// Double scale factor for symmetry
			outRange	*= 0.5;							// and adjust output range
			outOffset	+= outRange;
			
			outVal -= inRange * 0.5;					// Correct origin
			{
			Boolean	flipPoint;
			
			outVal *= ( flipPoint = (outVal < 0.0) ) ? -inScale : inScale;
			outVal  = limFunc(mapFunc(outVal, curve), ctr);
			outVal  = MapFromUnit(flipPoint ? -outVal : outVal, outOffset, outRange);
			}
				
			do { *oSigOut++ = outVal; } while (--iVecSize > 0);
			break;
			}
		}
	
	}

int*
ScampPerformStatic(
	int*	iParams)
	
	{
	enum {
		paramFuncAddress	= 0,
		paramMe,
		paramVectorSize,
		paramInput,
		paramOutput,
		
		paramNextLink
		};
	
	objScamp*	me = (objScamp*) iParams[paramMe];
	tScampFlags	flags;
	
	if (me->coreObject.z_disabled)
		goto exit;
	
	flags = me->flags;
	
	if (flags.rangeInval)
		ScampCalcRange(me);
		
	if (flags.degenerate) {
		if (me->action == actStet)
			PerformStatLinStet(	me, (long) iParams[paramVectorSize],
								(tSampleVector) iParams[paramInput],
								(tSampleVector) iParams[paramOutput]);
		else PerformStatLinLim(	me, (long) iParams[paramVectorSize],
								(tSampleVector) iParams[paramInput],
								(tSampleVector) iParams[paramOutput]);
		}
	else {
		if (me->action == actStet)
			PerformStatCurveStet(me, (long) iParams[paramVectorSize],
								 (tSampleVector) iParams[paramInput],
								 (tSampleVector) iParams[paramOutput]);
		else PerformStatCurveLim(me, (long) iParams[paramVectorSize],
								 (tSampleVector) iParams[paramInput],
								 (tSampleVector) iParams[paramOutput]);
		}
	
exit:
	return iParams + paramNextLink;
	}
	


/******************************************************************************************
 *
 *	PerformDynLinStet(me, iVSize, iSigIn, iSig1, iSig2, iSigMin, iSigMax, iSigCurve, oSig)
 *	PerformDynLinLim(me, iVSize, iSigIn, iSig1, iSig2, iSigMin, iSigMax, iSigCurve, oSig)
 *	PerformDynCurveStet(me, iVSize, iSigIn, iSig1, iSig2, iSigMin, iSigMax, iSigCurve, oSig)
 *	PerformDynCurveLim(me, iVSize, iSigIn, iSig1, iSig2, iSigMin, iSigMax, iSigCurve, oSig)
 *	
 *	ScampPerformDynamic(iParams)
 *	
 *	DSP Perform methods for signal (ie, changing or dynamic) parameters
 *
 ******************************************************************************************/

static void
PerformDynLinStet(
	objScamp*		me,
	long			iVecSize,
	tSampleVector	iSigIn,
	tSampleVector	iSig1,
	tSampleVector	iSig2,
	tSampleVector	iSigOutMin,
	tSampleVector	iSigOutMax,
	tSampleVector	oSigOut)
	
	{
	double	statIn	= me->input,
			curSamp;
	
	if (me->flags.map) {
		// Modern "map" mode
		double	inMin	= me->inMin,
				inMax	= me->inMax,
				outMin	= me->outMin,
				outMax	= me->outMax;
		
		if (me->symmetry == symAxis) do {
			curSamp  = (iSigIn != NIL) ? *iSigIn++ : statIn;
			
			if (iSig1 != NIL)		inMin	= *iSig1++;
			if (iSig2 != NIL)		inMax	= *iSig2++;
			if (iSigOutMin != NIL)	outMin	= *iSigOutMin++;
			if (iSigOutMax != NIL)	outMax	= *iSigOutMax++;
			
			curSamp -= inMin;
			curSamp /= (inMax - inMin);
			curSamp *= (curSamp >= 0.0) ? (outMax - outMin) : (outMin - outMax);
			curSamp += outMin;
			
			*oSigOut++ = curSamp;
			} while (--iVecSize > 0);
		
		else do {
			curSamp  = (iSigIn != NIL) ? *iSigIn++ : statIn;
			
			if (iSig1 != NIL)		inMin	= *iSig1++;
			if (iSig2 != NIL)		inMax	= *iSig2++;
			if (iSigOutMin != NIL)	outMin	= *iSigOutMin++;
			if (iSigOutMax != NIL)	outMax	= *iSigOutMax++;
			
			curSamp -= inMin;
			curSamp /= inMax - inMin;
			curSamp *= outMax - outMin;
			curSamp += outMin;
			
			*oSigOut++ = curSamp;
			} while (--iVecSize > 0);
		}
	
	else {
		// Traditional "scale" mode
		double	scale	= me->slope,
				offset	= me->offset;
		
		if (me->symmetry == symAxis) do {
			curSamp  = (iSigIn != NIL) ? *iSigIn++ : statIn;
			
			if (iSig1 != NIL)	scale	= *iSig1++;
			if (iSig2 != NIL)	offset	= *iSig2++;
			
			curSamp *= (curSamp >= offset) ? scale : -scale;
			curSamp += offset;
			
			*oSigOut++ = curSamp;
			} while (--iVecSize > 0);
		
		else do {
			curSamp  = (iSigIn != NIL) ? *iSigIn++ : statIn;
			curSamp *= (iSig1 != NIL) ? *iSig1++ : scale;
			curSamp += (iSig2 != NIL) ? *iSig2++ : offset;
			
			*oSigOut++ = curSamp;
			} while (--iVecSize > 0);
		}
	
	}
	
static void
PerformDynLinLim(
	objScamp*		me,
	long			iVecSize,
	tSampleVector	iSigIn,
	tSampleVector	iSig1,
	tSampleVector	iSig2,
	tSampleVector	iSigOutMin,
	tSampleVector	iSigOutMax,
	tSampleVector	oSigOut)

	{
	double		statIn	= me->input,
				outMin	= me->outMin,
				outMax	= me->outMax,
				curSamp;
	long*		ctr		= &me->limCounter;
	tActFunc	limFunc;
	
	switch (me->action) {
		case actWrap:		limFunc = Wrap;		break;
		case actReflect:	limFunc = Reflect;	break;
		default:			limFunc = Clip;		break;		// ASSERT: me->action == act Clip
		}
	
	if (me->flags.map) {
		// Modern "map" mode
		double	inMin	= me->inMin,
				inMax	= me->inMax;
		
		if (me->symmetry == symAxis) do {
			curSamp  = (iSigIn != NIL) ? *iSigIn++ : statIn;
			
			if (iSig1 != NIL)		inMin	= *iSig1++;
			if (iSig2 != NIL)		inMax	= *iSig2++;
			if (iSigOutMin != NIL)	outMin	= *iSigOutMin++;
			if (iSigOutMax != NIL)	outMax	= *iSigOutMax++;
			
			curSamp -= inMin;
			curSamp /= (inMax - inMin);
			curSamp *= (curSamp >= 0.0) ? (outMax - outMin) : (outMin - outMax);
			curSamp += outMin;
			
			*oSigOut++ = limFunc(curSamp, outMin, outMax, ctr);
			} while (--iVecSize > 0);
		
		else do {
			curSamp  = (iSigIn != NIL) ? *iSigIn++ : statIn;
			
			if (iSig1 != NIL)		inMin	= *iSig1++;
			if (iSig2 != NIL)		inMax	= *iSig2++;
			if (iSigOutMin != NIL)	outMin	= *iSigOutMin++;
			if (iSigOutMax != NIL)	outMax	= *iSigOutMax++;
			
			curSamp -= inMin;
			curSamp /= inMax - inMin;
			curSamp *= outMax - outMin;
			curSamp += outMin;
			
			*oSigOut++ = limFunc(curSamp, outMin, outMax, ctr);
			} while (--iVecSize > 0);
		}
	
	else {
		// Traditional "scale" mode
		double	scale	= me->slope,
				offset	= me->offset;
		
		if (me->symmetry == symAxis) do {
			curSamp  = (iSigIn != NIL) ? *iSigIn++ : statIn;
			
			if (iSig1 != NIL)		scale	= *iSig1++;
			if (iSig2 != NIL)		offset	= *iSig2++;
			if (iSigOutMin != NIL)	outMin	= *iSigOutMin++;
			if (iSigOutMax != NIL)	outMax	= *iSigOutMax++;
			
			curSamp *= (curSamp >= offset) ? scale : -scale;
			curSamp += offset;
			
			*oSigOut++ = limFunc(curSamp, outMin, outMax, ctr);
			} while (--iVecSize > 0);
		
		else do {
			curSamp  = (iSigIn != NIL) ? *iSigIn++ : statIn;
			curSamp *= (iSig1 != NIL) ? *iSig1++ : scale;
			curSamp += (iSig2 != NIL) ? *iSig2++ : offset;
			
			if (iSigOutMin != NIL)	outMin	= *iSigOutMin++;
			if (iSigOutMax != NIL)	outMax	= *iSigOutMax++;
			
			*oSigOut++ = limFunc(curSamp, outMin, outMax, ctr);
			} while (--iVecSize > 0);
		}
	}

static void
PerformDynCurveStet(
	objScamp*		me,
	long			iVecSize,
	tSampleVector	iSigIn,
	tSampleVector	iSig1,
	tSampleVector	iSig2,
	tSampleVector	iSigOutMin,
	tSampleVector	iSigOutMax,
	tSampleVector	iSigCurve,
	tSampleVector	oSigOut)
	
	{
	double		statIn	= me->input,
				outMin	= me->outMin,
				outMax	= me->outMax,
				curve	= me->curve,
				inMin,
				inMax,
				curSamp;
	tMapFunc	mapFunc;
	
	switch (me->mappingType) {
		default:		mapFunc = Kink;		break;			// ASSERT: me->mappingType == mapLin
		case mapPow:	mapFunc = Power;	break;
		case mapExp:	mapFunc = Exponent;	break;
		}
	
	if (me->flags.map) {
		// Modern "map" mode
		inMin	= me->inMin;
		inMax	= me->inMax;
		
		switch (me->symmetry) {
		default:
			// Must be symNone
			do {
				curSamp  = (iSigIn != NIL) ? *iSigIn++ : statIn;
				
				if (iSig1 != NIL)		inMin	= *iSig1++;
				if (iSig2 != NIL)		inMax	= *iSig2++;
				if (iSigOutMin != NIL)	outMin	= *iSigOutMin++;
				if (iSigOutMax != NIL)	outMax	= *iSigOutMax++;
				if (iSigCurve != NIL)	curve	= *iSigCurve++;
				
				curSamp -= inMin;
				curSamp /= inMax - inMin;
				curSamp	 = mapFunc(curSamp, curve);
				curSamp	*= outMax - outMin;
				curSamp += outMin;
				
				*oSigOut++ = curSamp;
				} while (--iVecSize > 0);
			break;
		
		case symAxis:
			do {
				curSamp  = (iSigIn != NIL) ? *iSigIn++ : statIn;
				
				if (iSig1 != NIL)		inMin	= *iSig1++;
				if (iSig2 != NIL)		inMax	= *iSig2++;
				if (iSigOutMin != NIL)	outMin	= *iSigOutMin++;
				if (iSigOutMax != NIL)	outMax	= *iSigOutMax++;
				if (iSigCurve != NIL)	curve	= *iSigCurve++;
				
				curSamp -= inMin;
				curSamp /= (curSamp >= 0.0) ? (inMax - inMin) : (inMin - inMax);
				curSamp	 = mapFunc(curSamp, curve);
				curSamp	*= outMax - outMin;
				curSamp += outMin;
				
				*oSigOut++ = curSamp;
				} while (--iVecSize > 0);
			break;
		
		case symPoint:
			do {
				Boolean	flipPoint;
				
				curSamp  = (iSigIn != NIL) ? *iSigIn++ : statIn;
				
				if (iSig1 != NIL)		inMin	= *iSig1++;
				if (iSig2 != NIL)		inMax	= *iSig2++;
				if (iSigOutMin != NIL)	outMin	= *iSigOutMin++;
				if (iSigOutMax != NIL)	outMax	= *iSigOutMax++;
				if (iSigCurve != NIL)	curve	= *iSigCurve++;
				
				curSamp -= inMin;
				curSamp /= (flipPoint = (curSamp < 0.0)) ? (inMin - inMax) : (inMax - inMin);
				curSamp	 = mapFunc(curSamp, curve);
				curSamp	*= flipPoint ? (outMin - outMax) : (outMax - outMin);
				curSamp += outMin;
				
				*oSigOut++ = curSamp;
				} while (--iVecSize > 0);
			break;
			}
		}
	
	else {
		// Traditional "scale" mode
		// Strange thing to do with curves, but hey! why not?
		double	scale	= me->slope,
				offset	= me->offset,
				invScale;
		
		switch (me->symmetry) {
		default:
			// Must be symNone
			do {
				curSamp  = (iSigIn != NIL) ? *iSigIn++ : statIn;
				
				if (iSig1 != NIL)		scale	= *iSig1++;
				if (iSig2 != NIL)		offset	= *iSig2++;
				if (iSigOutMin != NIL)	outMin	= *iSigOutMin++;
				if (iSigOutMax != NIL)	outMax	= *iSigOutMax++;
				if (iSigCurve != NIL)	curve	= *iSigCurve++;
				
				// Extrapolate inMin/inMax from scale and offset
				invScale = 1.0 / scale;
				inMin = (outMin - offset) * invScale;
				inMax = inMin + (outMax - outMin) * invScale;
				
				curSamp -= inMin;
				curSamp /= inMax - inMin;
				curSamp	 = mapFunc(curSamp, curve);
				curSamp	*= outMax - outMin;
				curSamp += outMin;
				
				*oSigOut++ = curSamp;
				} while (--iVecSize > 0);
			break;
		
		case symAxis:
			do {
				curSamp  = (iSigIn != NIL) ? *iSigIn++ : statIn;
				
				if (iSig1 != NIL)		scale	= *iSig1++;
				if (iSig2 != NIL)		offset	= *iSig2++;
				if (iSigOutMin != NIL)	outMin	= *iSigOutMin++;
				if (iSigOutMax != NIL)	outMax	= *iSigOutMax++;
				if (iSigCurve != NIL)	curve	= *iSigCurve++;
				
				// Extrapolate inMin/inMax from scale and offset
				invScale = 1.0 / scale;
				inMin = (outMin - offset) * invScale;
				inMax = inMin + (outMax - outMin) * invScale;
				
				curSamp -= inMin;
				curSamp /= (curSamp >= 0.0) ? (inMax - inMin) : (inMin - inMax);
				curSamp	 = mapFunc(curSamp, curve);
				curSamp	*= outMax - outMin;
				curSamp += outMin;
				
				*oSigOut++ = curSamp;
				} while (--iVecSize > 0);
			break;
		
		case symPoint:
			do {
				Boolean	flipPoint;
				
				curSamp  = (iSigIn != NIL) ? *iSigIn++ : statIn;
				
				if (iSig1 != NIL)		scale	= *iSig1++;
				if (iSig2 != NIL)		offset	= *iSig2++;
				if (iSigOutMin != NIL)	outMin	= *iSigOutMin++;
				if (iSigOutMax != NIL)	outMax	= *iSigOutMax++;
				if (iSigCurve != NIL)	curve	= *iSigCurve++;
				
				// Extrapolate inMin/inMax from scale and offset
				invScale = 1.0 / scale;
				inMin = (outMin - offset) * invScale;
				inMax = inMin + (outMax - outMin) * invScale;
				
				curSamp -= inMin;
				curSamp /= (flipPoint = (curSamp < 0.0)) ? (inMin - inMax) : (inMax - inMin);
				curSamp	 = mapFunc(curSamp, curve);
				curSamp	*= flipPoint ? (outMin - outMax) : (outMax - outMin);
				curSamp += outMin;
				
				*oSigOut++ = curSamp;
				} while (--iVecSize > 0);
			break;
			}
		}
	
	}
	
	
static void
PerformDynCurveLim(
	objScamp*		me,
	long			iVecSize,
	tSampleVector	iSigIn,
	tSampleVector	iSig1,
	tSampleVector	iSig2,
	tSampleVector	iSigOutMin,
	tSampleVector	iSigOutMax,
	tSampleVector	iSigCurve,
	tSampleVector	oSigOut)
	
	{
	double			statIn	= me->input,
					outMin	= me->outMin,
					outMax	= me->outMax,
					curve	= me->curve,
					inMin,
					inMax,
					curSamp;
	long*			ctr		= &me->limCounter;
	tMapFunc		mapFunc;
	tUnitActFunc	limFunc;
	
	switch (me->mappingType) {
		default:		mapFunc = Kink;		break;			// ASSERT: me->mappingType == mapLin
		case mapPow:	mapFunc = Power;	break;
		case mapExp:	mapFunc = Exponent;	break;
		}
	
	switch (me->action) {
		default:			limFunc = UnitClip;		break;		// ASSERT: me->action == actClip
		case actWrap:		limFunc = UnitWrap;		break;
		case actReflect:	limFunc = UnitReflect;	break;
		}
	
	if (me->flags.map) {
		// Modern "map" mode
		inMin	= me->inMin;
		inMax	= me->inMax;
		
		switch (me->symmetry) {
		default:
			// Must be symNone
			do {
				curSamp  = (iSigIn != NIL) ? *iSigIn++ : statIn;
				
				if (iSig1 != NIL)		inMin	= *iSig1++;
				if (iSig2 != NIL)		inMax	= *iSig2++;
				if (iSigOutMin != NIL)	outMin	= *iSigOutMin++;
				if (iSigOutMax != NIL)	outMax	= *iSigOutMax++;
				if (iSigCurve != NIL)	curve	= *iSigCurve++;
				
				curSamp -= inMin;
				curSamp /= inMax - inMin;
				curSamp	 = limFunc(mapFunc(curSamp, curve), ctr);
				curSamp	*= outMax - outMin;
				curSamp += outMin;
				
				*oSigOut++ = curSamp;
				} while (--iVecSize > 0);
			break;
		
		case symAxis:
			do {
				curSamp  = (iSigIn != NIL) ? *iSigIn++ : statIn;
				
				if (iSig1 != NIL)		inMin	= *iSig1++;
				if (iSig2 != NIL)		inMax	= *iSig2++;
				if (iSigOutMin != NIL)	outMin	= *iSigOutMin++;
				if (iSigOutMax != NIL)	outMax	= *iSigOutMax++;
				if (iSigCurve != NIL)	curve	= *iSigCurve++;
				
				curSamp -= inMin;
				curSamp /= (curSamp >= 0.0) ? (inMax - inMin) : (inMin - inMax);
				curSamp	 = limFunc(mapFunc(curSamp, curve), ctr);
				curSamp	*= outMax - outMin;
				curSamp += outMin;
				
				*oSigOut++ = curSamp;
				} while (--iVecSize > 0);
			break;
		
		case symPoint:
			do {
				Boolean	flipPoint;
				
				curSamp  = (iSigIn != NIL) ? *iSigIn++ : statIn;
				
				if (iSig1 != NIL)		inMin	= *iSig1++;
				if (iSig2 != NIL)		inMax	= *iSig2++;
				if (iSigOutMin != NIL)	outMin	= *iSigOutMin++;
				if (iSigOutMax != NIL)	outMax	= *iSigOutMax++;
				if (iSigCurve != NIL)	curve	= *iSigCurve++;
				
				curSamp -= inMin;
				curSamp /= (flipPoint = (curSamp < 0.0)) ? (inMin - inMax) : (inMax - inMin);
				curSamp	 = limFunc(mapFunc(curSamp, curve) , ctr);
				curSamp	*= flipPoint ? (outMin - outMax) : (outMax - outMin);
				curSamp += outMin;
				
				*oSigOut++ = curSamp;
				} while (--iVecSize > 0);
			break;
			}
		}
	
	else {
		// Traditional "scale" mode
		// Strange thing to do with curves, but hey! why not?
		double	scale	= me->slope,
				offset	= me->offset,
				invScale;
		
		switch (me->symmetry) {
		default:
			// Must be symNone
			do {
				curSamp  = (iSigIn != NIL) ? *iSigIn++ : statIn;
				
				if (iSig1 != NIL)		scale	= *iSig1++;
				if (iSig2 != NIL)		offset	= *iSig2++;
				if (iSigOutMin != NIL)	outMin	= *iSigOutMin++;
				if (iSigOutMax != NIL)	outMax	= *iSigOutMax++;
				if (iSigCurve != NIL)	curve	= *iSigCurve++;
				
				// Extrapolate inMin/inMax from scale and offset
				invScale = 1.0 / scale;
				inMin = (outMin - offset) * invScale;
				inMax = inMin + (outMax - outMin) * invScale;
				
				curSamp -= inMin;
				curSamp /= inMax - inMin;
				curSamp	 = limFunc(mapFunc(curSamp, curve) , ctr);
				curSamp	*= outMax - outMin;
				curSamp += outMin;
				
				*oSigOut++ = curSamp;
				} while (--iVecSize > 0);
			break;
		
		case symAxis:
			do {
				curSamp  = (iSigIn != NIL) ? *iSigIn++ : statIn;
				
				if (iSig1 != NIL)		scale	= *iSig1++;
				if (iSig2 != NIL)		offset	= *iSig2++;
				if (iSigOutMin != NIL)	outMin	= *iSigOutMin++;
				if (iSigOutMax != NIL)	outMax	= *iSigOutMax++;
				if (iSigCurve != NIL)	curve	= *iSigCurve++;
				
				// Extrapolate inMin/inMax from scale and offset
				invScale = 1.0 / scale;
				inMin = (outMin - offset) * invScale;
				inMax = inMin + (outMax - outMin) * invScale;
				
				curSamp -= inMin;
				curSamp /= (curSamp >= 0.0) ? (inMax - inMin) : (inMin - inMax);
				curSamp	 = limFunc(mapFunc(curSamp, curve) , ctr);
				curSamp	*= outMax - outMin;
				curSamp += outMin;
				
				*oSigOut++ = curSamp;
				} while (--iVecSize > 0);
			break;
		
		case symPoint:
			do {
				Boolean	flipPoint;
				
				curSamp  = (iSigIn != NIL) ? *iSigIn++ : statIn;
				
				if (iSig1 != NIL)		scale	= *iSig1++;
				if (iSig2 != NIL)		offset	= *iSig2++;
				if (iSigOutMin != NIL)	outMin	= *iSigOutMin++;
				if (iSigOutMax != NIL)	outMax	= *iSigOutMax++;
				if (iSigCurve != NIL)	curve	= *iSigCurve++;
				
				// Extrapolate inMin/inMax from scale and offset
				invScale = 1.0 / scale;
				inMin = (outMin - offset) * invScale;
				inMax = inMin + (outMax - outMin) * invScale;
				
				curSamp -= inMin;
				curSamp /= (flipPoint = (curSamp < 0.0)) ? (inMin - inMax) : (inMax - inMin);
				curSamp	 = limFunc(mapFunc(curSamp, curve) , ctr);
				curSamp	*= flipPoint ? (outMin - outMax) : (outMax - outMin);
				curSamp += outMin;
				
				*oSigOut++ = curSamp;
				} while (--iVecSize > 0);
			break;
			}
		}
	
	}

int*
ScampPerformDynamic(
	int*	iParams)
	
	{
	enum {
		paramFuncAddress	= 0,
		paramMe,
		paramVectorSize,
		paramInput,
		param1,
		param2,
		paramMinOut,
		paramMaxOut,
		paramCurve,
		paramOutput,
		
		paramNextLink
		};
	
	objScamp*	me = (objScamp*) iParams[paramMe];
	tScampFlags	flags;
	
	if (me->coreObject.z_disabled)
		goto exit;
	
	flags = me->flags;
	
	if (flags.rangeInval
			&& (iParams[param1] == 0 || iParams[param2] == 0))		// Cheesy, should typecast
		ScampCalcRange(me);
		
	if (me->mappingType == mapLin
			|| (flags.degenerate && iParams[paramCurve] == 0)) {	// Cheesy, should typecast
		if (me->action == actStet)
			PerformDynLinStet(	me, (long) iParams[paramVectorSize],
								(tSampleVector) iParams[paramInput],
								(tSampleVector) iParams[param1],
								(tSampleVector) iParams[param2],
								(tSampleVector) iParams[paramMinOut],
								(tSampleVector) iParams[paramMaxOut],
								(tSampleVector) iParams[paramOutput]);
		else PerformDynLinLim(	me, (long) iParams[paramVectorSize],
								(tSampleVector) iParams[paramInput],
								(tSampleVector) iParams[param1],
								(tSampleVector) iParams[param2],
								(tSampleVector) iParams[paramMinOut],
								(tSampleVector) iParams[paramMaxOut],
								(tSampleVector) iParams[paramOutput]);
		}
	else {
		if (me->action == actStet)
			PerformDynCurveStet(me, (long) iParams[paramVectorSize],
								(tSampleVector) iParams[paramInput],
								(tSampleVector) iParams[param1],
								(tSampleVector) iParams[param2],
								(tSampleVector) iParams[paramMinOut],
								(tSampleVector) iParams[paramMaxOut],
								(tSampleVector) iParams[paramCurve],
								(tSampleVector) iParams[paramOutput]);
		else PerformDynCurveLim(me, (long) iParams[paramVectorSize],
								(tSampleVector) iParams[paramInput],
								(tSampleVector) iParams[param1],
								(tSampleVector) iParams[param2],
								(tSampleVector) iParams[paramMinOut],
								(tSampleVector) iParams[paramMaxOut],
								(tSampleVector) iParams[paramCurve],
								(tSampleVector) iParams[paramOutput]);
		}
	
exit:
	return iParams + paramNextLink;
	}
	
