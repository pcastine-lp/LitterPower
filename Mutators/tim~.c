/*
	File:		tim~.c

	Contains:	Time-domain interval mutation.

	Written by:	Peter Castine

	Copyright:	© 2002 Peter Castine. All rights reserved.

	Change History (most recent first):

         <4>     10Ð1Ð04    pc      Update for Windows, using new LitterLib calls.
         <3>    7Ð7Ð2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <2>  30Ð12Ð2002    pc      Add object version to DoInfo()
         <1>  30Ð12Ð2002    pc      Initial Check-In
*/


/******************************************************************************************
	File:		tim~.c

	Contains:	Max external object implementing Larry Polansky's Interval Mutation
				algorithms.

	Written by:	Peter Castine

	Copyright:	© 2000-01 Peter Castine

	Change History (most recent first):
		15-Nov-2000:	First implementation (based on previous Mutator~.c)
									

 ******************************************************************************************/

#pragma mark ¥ Include Files

#include "TrialPeriodUtils.h"
#include "imLib.h"


#pragma mark ¥ Constants

const char*	kClassName		= "lp.tim~";			// Class name


#pragma mark ¥ Function Prototypes

void*	NewMutator(Symbol*, short, Atom*);

void	BuildDSPChain(tMutator*, t_signal**, short*);
int*	PerformMutator(int*);
	// The following routines are where the actual work is done.
	// The first parameters are always: Mutator object, vector size, Source signal, Target
	// signal, and Mutant signal. The three signals are guaranteed never to be NIL.
	// These parameters are all followed by omega. If the mutation is irregular, the 
	// parameter pi follows. And, finally, if the mutation is not using absolute values
	// there is a delta parameter.
	// With control-rate mutations, omega, delta and pi are floats (taken from the first
	// sample of the signal if non-NIL, otherwise the current values from the Mutator object
	// are taken).
void	CalcAbsIrregCR(	tMutator*, long,
						t_sample*, t_sample*, t_sample*,
						float, float);
void	CalcAbsIrregSR(	tMutator*, long,
						t_sample*, t_sample*, t_sample*,
						t_sample*, t_sample*);
void	CalcAbsUniformCR(tMutator*, long,
						t_sample*, t_sample*, t_sample*,
						float);
void	CalcAbsUniformSR(tMutator*, long,
						t_sample*, t_sample*, t_sample*,
						t_sample*);
void	CalcRelIrregCR(	tMutator*, long,
						t_sample*, t_sample*, t_sample*,
						float, float, float);
void	CalcRelIrregSR(	tMutator*, long,
						t_sample*, t_sample*, t_sample*,
						t_sample*, t_sample*, t_sample*);
void	CalcRelUniformCR(tMutator*, long,
						t_sample*, t_sample*, t_sample*,
						float, float);
void	CalcRelUniformSR(tMutator*, long,
						t_sample*, t_sample*, t_sample*,
						t_sample*, t_sample*);

	// Various Max messages
void	DoAssist(tMutator*, void* , long , long , char*);
void	DoInfo(tMutator*);
void	DoTattle(tMutator*);

#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

/******************************************************************************************
 *
 *	main()
 *	
 *	Standard MSP External Object Entry Point Function
 *	
 ******************************************************************************************/

void
main(void)
	
	{
	LITTER_CHECKTIMEOUT(kClassName);
	
	// Standard Max setup() call
	setup(	&gObjectClass,				// Pointer to our class definition
			(method) NewMutator,		// Instance creation function
			(method) dsp_free,			// Default deallocation function
			sizeof(tMutator),			// Class object size
			NIL,						// No menu function
			A_GIMME,					// We parse our arguments
			0);		
	
	// Max/MSP class initialization mantra
	dsp_initclass();

	// Very standard messages
	addfloat((method) DoFloat);
	
	// Fairly standard Max messages
	addmess	((method) DoAssist,		"assist",	A_CANT, 0);
	addmess	((method) DoInfo,		"info",		A_CANT, 0);
	addmess	((method) DoTattle,		"dblclick",	A_CANT, 0);
	addmess	((method) DoTattle,		"tattle",	A_NOTHING);
	
	// Vaguely standard messages
	addmess ((method) DoClear,		"clear",	A_NOTHING);
	
	// MSP-Level messages
	LITTER_TIMEBOMB addmess	((method) BuildDSPChain, "dsp",		A_CANT, 0);

	//Our messages
	addmess ((method) DoUSIM,		"usim",		A_NOTHING);
	addmess ((method) DoISIM,		"isim",		A_NOTHING);
	addmess ((method) DoUUIM,		"uuim",		A_NOTHING);
	addmess ((method) DoIUIM,		"iuim",		A_NOTHING);
	addmess ((method) DoWCM,		"wcm",		A_NOTHING);
	addmess ((method) DoLCM,		"lcm",		A_NOTHING);
	
	addmess	((method) DoAbsInt,		"abs",		A_NOTHING);
	addmess	((method) DoRelInt,		"rel",		A_DEFFLOAT, 0);
	
	
	// Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();

	}

#pragma mark -
#pragma mark ¥ Class Message Handlers

/******************************************************************************************
 *
 *	NewMutator(iSelector, iArgCount, iArgVector)
 *
 ******************************************************************************************/

void*
NewMutator(
	Symbol* iSelector,
	short	iArgCount,
	Atom*	iArgVector)
	
	{
	Boolean		gotInitType		= false,		// The gotInitXXX variables are for 
				gotInitOmega	= false,		// tracking which arguments were typed
				gotInitDelta	= false,		// into our object box.
				gotInitPi		= false;
	short		initType		= imDefault;	// Values to use if nothing specified
	float		initOmega		= 0.0,
				initDelta		= 0.0,
				initPi			= 0.0;
	
	tMutator*	me				= NIL;
	
	// Parse arguments
	while (iArgCount-- > 0) {
		switch (iArgVector->a_type) {
			case A_FLOAT:
				if (!gotInitOmega) {
					gotInitOmega = true;
					initOmega = iArgVector->a_w.w_float;
					if		(initOmega < 0.0)	initOmega = 0.0;
					else if	(1.0 < initOmega)	initOmega = 1.0;
					}
				else if (!gotInitDelta) {
					gotInitDelta = true;
					initDelta = iArgVector->a_w.w_float;
					if		(initDelta < -1.0)	initDelta = 0.0;
					else if	(1.0 < initDelta)	initDelta = 1.0;
					}
				else if (!gotInitPi) {
					gotInitPi = true;
					initPi = iArgVector->a_w.w_float;
					if		(initPi < 0.0)		initPi = 0.0;
					else if	(kMaxPi < initPi)	initPi = kMaxPi;
					}
				else goto punt_argument;					// don't need this argument
				break;
			
			case A_SYM:
				if (gotInitType) goto punt_argument;		// naughty argument
				
				gotInitType = true;
				initType = MapStrToMutationType(iArgVector->a_w.w_sym->s_name);
				if (initType < 0) {
					initType = imDefault;
					goto punt_argument;
					}
				
				break;
			
			default:
			punt_argument:
				error("%s received spurious argument ", iSelector->s_name);
				postatom(iArgVector);
				break;
			}
		iArgVector += 1;
		}
	
	// Let Max/MSP allocate us and give us inlets;
	// number of inlets depends upon which variant we are
	me = (tMutator*) newobject(gObjectClass);
	dsp_setup(&(me->coreObject), 5);
	
	// And two outlets, set these up right-to-left
	// 4-Jan-2002: now only one outlet
	outlet_new(me, "signal");			
	
	// Set up default/initial values
	Initialize(me, initType, initOmega, initPi, initDelta, gotInitDelta);
	
	return me;
	}

#pragma mark -
#pragma mark ¥ Object Message Handlers

/******************************************************************************************
 *
 *	DoAssist()
 *	DoInfo()
 *
 *	Fairly generic Assist/Info methods
 *	
 *	We don't use a lot of the function parameters.
 *
 ******************************************************************************************/

void DoAssist(tMutator* me, void* box, long iDir, long iArgNum, char* oCStr)
	{
	#pragma unused(me, box)
	
	LitterAssist(iDir, iArgNum, strIndexInSource, strIndexOutMutant, oCStr);
	}

void DoInfo(tMutator* me)
	{ LitterInfo(kClassName, &me->coreObject.z_ob, (method) DoTattle); }


/******************************************************************************************
 *
 *	DoTattle
 *
 *	Display current settings when the user double clicks on an object.
 *
 ******************************************************************************************/

void
DoTattle(
	tMutator* me)
	
	{
	
	post("%s state", kClassName);
	post("  Mutation type: %s", kLongNames[CalcMutationIndex(me)]);
	
	post("  Mutation index (if no signal): %lf", me->params.omega);
	post("  Mutation index signals updated at %s",
			(me->params.omegaSR) ? "sampling rate" : "control rate");
	
	if (me->params.irregular) {
		post("  Clumping factor (if no signal): %lf", me->params.pi);
		post("  Clumping factor signals updated at %s",
					(me->params.piSR) ? "sampling rate" : "control rate");
		}
		
	if (me->params.relInterval) {
		post("  Using Relative Intervals");
		post("    Delta Emphasis (if no signal): %lf", me->params.delta);
		post("    Delta Emphasis signals updated at %s",
					(me->params.deltaSR) ? "sampling rate" : "control rate");
		}
	else {
		post("  Using Absolute Intervals");
		}
	
	// Range Check information
//	switch (me->params.rangeCheck) {
//		case rangeLetBe:
//			post("  No out-of-range correction");
//			break;
//		case rangeClip:
//			post("  Out-of-range values clipped");
//			break;
//		case rangeWrap:
//			post("  Out-of-range values wrapped");
//			break;
//		case rangeReflect:
//			post("  Out-of-range values reflected back into range");
//			break;
//		default:
//			error("  rangeCheck %d (Unknown rangeCheck status)", (short) me->params.rangeCheck);
//			break;
//		}
//	if (me->params.rangeCheck != rangeLetBe) {
//		post("    Min: %lf, Max: %lf", me->params.min, me->params.max);
//		}
	
	}


#pragma mark -
#pragma mark ¥ DSP Methods

/******************************************************************************************
 *
 *	BuildDSPChain(me, ioDSPVectors, iConnectCounts)
 *
 ******************************************************************************************/

void
BuildDSPChain(
	tMutator*	me,
	t_signal**	ioDSPVectors,
	short*		iConnectCounts)
	
	{
	
	// Make sure we have connections at source, target, and mutant.
	// Otherwise there's no point in doing any DSP!
	if (iConnectCounts[inletSource] == 0
			|| iConnectCounts[inletTarget] == 0
			|| iConnectCounts[outletMutant] == 0)
		return;
	
	dsp_add(
		PerformMutator,
		8, me,
		(long) ioDSPVectors[inletSource]->s_n,
		ioDSPVectors[inletSource]->s_vec,
		ioDSPVectors[inletTarget]->s_vec,
		iConnectCounts[inletOmega] > 0 ? ioDSPVectors[inletOmega]->s_vec : NIL,
		iConnectCounts[inletPi] > 0 ? ioDSPVectors[inletPi]->s_vec : NIL,
		iConnectCounts[inletDelta] > 0 ? ioDSPVectors[inletDelta]->s_vec : NIL,
		ioDSPVectors[outletMutant]->s_vec
		);
	
	}
	

/******************************************************************************************
 *
 *	PerformMutator(iParams)
 *
 *	Parameter block contains 9 values:
 *		- Address of this function
 *		- The performing Mutator object
 *		- Current signal vector size
 *		- Source signal
 *		- Target signal
 *		- Mutation Index signal
 *		- Delta Emphasis signal
 *		- Clumping signal
 *		- Mutant signal (out)
 *
 *	This perform methods route to the specific mutation performance routine appropriate
 *	for the current value of mutationType.
 *
 ******************************************************************************************/

int*
PerformMutator(
	int* iParams)
	
	{
	enum {
		paramFuncAddress	= 0,
		paramMe,
		paramVectorSize,
		paramSource,
		paramTarget,
		paramOmega,
		paramPi,
		paramDelta,
		paramMutant,
		
		paramNextLink
		};
	
	tMutator*	me = (tMutator*) iParams[paramMe];
	
	if (me->coreObject.z_disabled) goto exit;
	
	// Parameters for the vector calculation method depend upon whether the mutation
	// is irregular or uniform; if absolute or relative intervals are used; and if
	// there are any signals that need to be sampled at audio rate.
	
	if (!me->params.irregular) {
		if (!me->params.relInterval) {
			// Absolute Intervals
			// The only parameter we have to worry about is the Mutation Index
			if (iParams[paramOmega] && (me->params.omegaSR)) {
				CalcAbsUniformSR(
						me, (long) iParams[paramVectorSize],
						(t_sample*) iParams[paramSource],
						(t_sample*) iParams[paramTarget],
						(t_sample*) iParams[paramMutant],
						(t_sample*) iParams[paramOmega]);
				}
			else {
				CalcAbsUniformCR(
						me, (long) iParams[paramVectorSize],
						(t_sample*) iParams[paramSource],
						(t_sample*) iParams[paramTarget],
						(t_sample*) iParams[paramMutant],
						iParams[paramOmega]
							? *((t_sample*) iParams[paramOmega])
							: me->params.omega);
				}
			}
		
		else {
			// Relative intervals
			// Weed to consider Mutation Index and Delta Emphasis
			if (iParams[paramOmega] && (me->params.omegaSR)
					|| iParams[paramDelta] && (me->params.deltaSR) ) {
				CalcRelUniformSR(
						me, (long) iParams[paramVectorSize],
						(t_sample*) iParams[paramSource],
						(t_sample*) iParams[paramTarget],
						(t_sample*) iParams[paramMutant],
						(t_sample*) iParams[paramOmega],
						(t_sample*) iParams[paramDelta]);
				}
			else {
				CalcRelUniformCR(
						me, (long) iParams[paramVectorSize],
						(t_sample*) iParams[paramSource],
						(t_sample*) iParams[paramTarget],
						(t_sample*) iParams[paramMutant],
						iParams[paramOmega]
							? *((t_sample*) iParams[paramOmega])
							: me->params.omega,
						iParams[paramDelta]
							? *((t_sample*) iParams[paramDelta])
							: me->params.delta);
				}
			}
		}
	
	// Irregular Mutations
	else if (!me->params.relInterval) {
		// Absolute Intervals
		// Possible signals in Omega and Pi
		if (iParams[paramOmega] && (me->params.omegaSR)
				|| iParams[paramPi] && (me->params.piSR)) {
			CalcAbsIrregSR(
				me, (long) iParams[paramVectorSize],
				(t_sample*) iParams[paramSource],
				(t_sample*) iParams[paramTarget],
				(t_sample*) iParams[paramMutant],
				(t_sample*) iParams[paramOmega],
				(t_sample*) iParams[paramPi]);
			}
		else {
			CalcAbsIrregCR(
				me, (long) iParams[paramVectorSize],
				(t_sample*) iParams[paramSource],
				(t_sample*) iParams[paramTarget],
				(t_sample*) iParams[paramMutant],
				iParams[paramOmega] ? *((t_sample*) iParams[paramOmega]) : me->params.omega,
				iParams[paramPi] ? *((t_sample*) iParams[paramPi]) : me->params.pi);
			}
		}
	
	// Irregular w/relative intervals; possible signals in Omega, Delta, and Pi
	else if (iParams[paramOmega] && (me->params.omegaSR)
				|| iParams[paramPi] && (me->params.piSR)
				|| iParams[paramDelta] && (me->params.deltaSR)) {
		CalcRelIrregSR(
			me, (long) iParams[paramVectorSize],
			(t_sample*) iParams[paramSource],
			(t_sample*) iParams[paramTarget],
			(t_sample*) iParams[paramMutant],
			(t_sample*) iParams[paramOmega],
			(t_sample*) iParams[paramPi],
			(t_sample*) iParams[paramDelta]);
		}
		
	else CalcRelIrregCR(
			me, (long) iParams[paramVectorSize],
			(t_sample*) iParams[paramSource],
			(t_sample*) iParams[paramTarget],
			(t_sample*) iParams[paramMutant],
			iParams[paramOmega] ? *((t_sample*) iParams[paramOmega]) : me->params.omega,
			iParams[paramPi] ? *((t_sample*) iParams[paramPi]) : me->params.pi,
			iParams[paramDelta] ? *((t_sample*) iParams[paramDelta]) : me->params.delta);
		
exit:
	return iParams + paramNextLink;
	}

/******************************************************************************************
 *
 *	CalcAbsIrregCR(me, iVectorSize, iSource, iTarget, oMutant, iOmega, iPi)
 *	CalcAbsIrregSR(me, iVectorSize, iSource, iTarget, oMutant, iOmega, iPi)
 *
 ******************************************************************************************/

void
CalcAbsIrregCR(
	tMutator*	me,
	long		iVectorSize,
	t_sample*	iSource,
	t_sample*	iTarget,
	t_sample*	oMutant,
	float		iOmega,
	float		iPi)
	
	{
	tMutate1Func
			mutateFunc	= me->params.function.irregularFunc;
	
	if (iOmega == 0.0) do {
		double curMutant = *iSource++;
		
		*oMutant++ = curMutant;
		} while (--iVectorSize > 0);
	
	else if (iOmega == 1.0) do {
		double curMutant = mutateFunc(*iSource++, *iTarget++);
		
		*oMutant++ = curMutant;
		} while (--iVectorSize > 0);
	
	else {
		// iOmega in (0.0 .. 1.0)
		unsigned long
				mutantToMutantThresh,
				sourceToMutantThresh,
				curThresh		= CalcMutantThreshholds(iOmega, iPi,
														&sourceToMutantThresh,
														&mutantToMutantThresh);
		unsigned wishMutants	= ((float) iVectorSize) * iOmega + 0.5;
		
		 do {
			double	curSource, curTarget, curMutant;
			curSource = *iSource++,
			curTarget = *iTarget++;
			if ( WannaMutateTight(curThresh, wishMutants, iVectorSize) ) {
				curThresh = mutantToMutantThresh;
				wishMutants -= 1;
				curMutant = mutateFunc(curSource, curTarget);
				}
			else {
				curThresh = sourceToMutantThresh;
				curMutant = curSource;
				}
			
			*oMutant++ = curMutant;
			} while (--iVectorSize > 0);
		}
	
	}

void
CalcAbsIrregSR(
	tMutator*	me,
	long		iVectorSize,
	t_sample*	iSource,
	t_sample*	iTarget,
	t_sample*	oMutant,
	t_sample*	iOmega,
	t_sample*	iPi)
	
	{
	Boolean	mutating	= stateIndeterminate;
	tMutate1Func
			mutateFunc	= me->params.function.irregularFunc;
	
	 do {
		double			curOmega	= iOmega ? *iOmega++ : me->params.omega,
						curPi		= iPi ? *iPi++ : me->params.pi,
						curMutant;
		unsigned long	mutateThresh;
		
		switch (mutating) {
			case stateSource:
				mutateThresh = CalcSourceToMutantThresh(
									curOmega,
									CalcOmegaPrime(curOmega, curPi));
				break;
			case stateTarget:
				mutateThresh = CalcMutantToMutantThresh(CalcOmegaPrime(curOmega, curPi));
				break;
			default:
				mutateThresh = CalcInitThresh(curOmega);
				break;
			}
			
		if ( WannaMutateStrict(mutateThresh) ) {
			mutating = stateTarget;
			curMutant = mutateFunc(*iSource++, *iTarget++);
			}
		else {
			mutating = stateSource;
			curMutant = *iSource++;
			iTarget += 1;				// Ignore target value
			}
		
		*oMutant++ = curMutant;
		} while (--iVectorSize > 0);
	
	}

/******************************************************************************************
 *
 *	CalcAbsUniformCR(me, iVectorSize, iSource, iTarget, oMutant, iOmega)
 *	CalcAbsUniformSR(me, iVectorSize, iSource, iTarget, oMutant, iOmega)
 *
 ******************************************************************************************/

void
CalcAbsUniformCR(
	tMutator*	me,
	long		iVectorSize,
	t_sample*	iSource,
	t_sample*	iTarget,
	t_sample*	oMutant,
	float		iOmega)
	
	{
	tMutateFunc mutateFunc	= me->params.function.uniformFunc;
	
	if (iOmega == 0.0) do {
		double curMutant = *iSource++;
		
		*oMutant++ = curMutant;
		} while (--iVectorSize > 0);
	
	else do {
		double curMutant = mutateFunc(*iSource++, *iTarget++, iOmega);
		
		*oMutant++ = curMutant;
		} while (--iVectorSize > 0);
	
	}
		
void
CalcAbsUniformSR(
	tMutator*	me,
	long		iVectorSize,
	t_sample*	iSource,
	t_sample*	iTarget,
	t_sample*	oMutant,
	t_sample*	iOmega)
	
	{
	tMutateFunc mutateFunc	= me->params.function.uniformFunc;
	
	 do {
		double curMutant = mutateFunc(*iSource++, *iTarget++, *iOmega++);
		
		*oMutant++ = curMutant;
		} while (--iVectorSize > 0);
	
	}

/******************************************************************************************
 *
 *	CalcRelIrregCR(me, iVectorSize, iSource, iTarget, oMutant, iOmega, iPi, iDelta)
 *	CalcRelIrregSR(me, iVectorSize, iSource, iTarget, oMutant, iOmega, iPi, iDelta)
 *
 ******************************************************************************************/

void
CalcRelIrregCR(
	tMutator*	me,
	long		iVectorSize,
	t_sample*	iSource,
	t_sample*	iTarget,
	t_sample*	oMutant,
	float		iOmega,
	float		iPi,
	float		iDelta)
	
	{
	double	prevSource	= me->history.source,
			prevTarget	= me->history.target,
			prevMutant	= me->history.mutant;
	Byte	prevState	= me->history.state;
	tEmphasisFunction
			emphFunc	=  (iDelta == 0.0)
							? EmphasizeZero 
							: (iDelta > 0.0) ? EmphasizePos : EmphasizeNeg;
	double	effectDelta = EffectiveDeltaValue(iDelta);
	tMutate1Func
			mutateFunc	= me->params.function.irregularFunc;
	
	if (iOmega == 0.0) {
		// We won't use the Target vector, but we must update prevTarget in case
		// the next call to this method needs the value. We need to do this now,
		// before we abuse iVectorSize as an index...
		prevTarget = iTarget[iVectorSize - 1];
		
		 do {
			double	curSource = *iSource++,
					curMutant;
			curMutant = emphFunc(prevMutant, curSource - prevSource, effectDelta);
			
			prevSource = curSource;
			*oMutant++ = prevMutant = curMutant;
			} while (--iVectorSize > 0);
		}
	
	else if (iOmega == 1.0) do {
		double	curSource = *iSource++,
				curTarget = *iTarget++,
				curMutant;
		curMutant = emphFunc(
						prevMutant,
						mutateFunc(curSource - prevSource, curTarget - prevTarget),
						effectDelta);
		
		prevSource = curSource;
		prevTarget = curTarget;
		*oMutant++ = prevMutant = curMutant;
		} while (--iVectorSize > 0);
	
	else {
		// iOmega in (0.0 .. 1.0)
		unsigned long
				mutantToMutantThresh,
				sourceToMutantThresh,
				curThresh		= CalcMutantThreshholds(iOmega, iPi,
														&sourceToMutantThresh,
														&mutantToMutantThresh);
		unsigned wishMutants	= ((float) iVectorSize) * iOmega + 0.5;
		
		do	{
			double	curSource	= *iSource++,
					curTarget	= *iTarget++,
					deltaMutant,
					curMutant;
			if ( WannaMutateTight(curThresh, wishMutants, iVectorSize) ) {
				curThresh = mutantToMutantThresh;
				wishMutants -= 1;
				deltaMutant = mutateFunc(curSource - prevSource, curTarget - prevTarget);
				}
			else {
				curThresh = sourceToMutantThresh;
				deltaMutant = curSource - prevSource;	// Mutant interval for Omega = 0 is just the source interval
				}
			curMutant = emphFunc(prevMutant, deltaMutant, effectDelta);
			
			prevSource = curSource;
			prevTarget = curTarget;
			*oMutant++ = prevMutant = curMutant;
			} while (--iVectorSize > 0);
		}
	
	me->history.source	= prevSource;
	me->history.target	= prevTarget;
	me->history.mutant	= prevMutant;
	me->history.state	= prevState;
	}
		

void
CalcRelIrregSR(
	tMutator*	me,
	long		iVectorSize,
	t_sample*	iSource,
	t_sample*	iTarget,
	t_sample*	oMutant,
	t_sample*	iOmega,
	t_sample*	iPi,
	t_sample*	iDelta)
	
	{
	Byte	mutating	= stateIndeterminate;
	double	prevSource	= me->history.source,
			prevTarget	= me->history.target,
			prevMutant	= me->history.mutant;
	tMutate1Func
			mutateFunc	= me->params.function.irregularFunc;
	
	do {
		double			curOmega	= iOmega ? *iOmega++ : me->params.omega,
						curPi		= iPi ? *iPi++ : me->params.pi,
						curDelta	= iDelta ? *iDelta++ : me->params.delta,
						deltaMutant,
						curMutant;
		unsigned long	mutateThresh;
		
		switch (mutating) {
			case stateSource:
				mutateThresh = CalcSourceToMutantThresh(
									curOmega,
									CalcOmegaPrime(curOmega, curPi));
				break;
			case stateTarget:
				mutateThresh = CalcMutantToMutantThresh(CalcOmegaPrime(curOmega, curPi));
				break;
			default:
				mutateThresh = CalcInitThresh(curOmega);
				break;
			}
		
		if ( WannaMutateStrict(mutateThresh) ) {
			mutating = stateTarget;
			deltaMutant = mutateFunc(*iSource - prevSource, *iTarget - prevTarget);
			}
		else {
			mutating = stateSource;
			deltaMutant = *iSource - prevSource;	// Mutant interval for Omega = 0 is
			}										// just the source interval
		curMutant = (curDelta >= 0.0)
						? EmphasizePos(prevMutant, deltaMutant, 1.0 - curDelta)
						: EmphasizeNeg(prevMutant, deltaMutant, 1.0 + curDelta);
		
		prevSource = *iSource++;
		prevTarget = *iTarget++;
		*oMutant++ = prevMutant = curMutant;
		} while (--iVectorSize > 0);
	
	me->history.source	= prevSource;
	me->history.target	= prevTarget;
	me->history.mutant	= prevMutant;
	}
		

/******************************************************************************************
 *
 *	CalcRelUniformCR(me, iVectorSize, iSource, iTarget, oMutant, iOmega, iDelta)
 *	CalcRelUniformSR(me, iVectorSize, iSource, iTarget, oMutant, iOmega, iDelta)
 *
 ******************************************************************************************/

void
CalcRelUniformCR(
	tMutator*	me,
	long		iVectorSize,
	t_sample*	iSource,
	t_sample*	iTarget,
	t_sample*	oMutant,
	float		iOmega,
	float		iDelta)
	
	{
	double	prevSource	= me->history.source,
			prevTarget	= me->history.target,
			prevMutant	= me->history.mutant;
	tEmphasisFunction
			emphFunc	=  (iDelta == 0.0)
							? EmphasizeZero 
							: (iDelta > 0.0) ? EmphasizePos : EmphasizeNeg;
	double	effectDelta = EffectiveDeltaValue(iDelta);
	
	if (iOmega == 0.0) {
		// We won't use the Target vector, but we must update prevTarget in case
		// the next call to this method needs the value. We need to do this now,
		// before we abuse iVectorSize as an index...
		prevTarget = iTarget[iVectorSize - 1];
		
		 do {
			double curSource, curMutant;
			curSource = *iSource++;
			curMutant = emphFunc(prevMutant, curSource - prevSource, effectDelta);
			
			prevSource = curSource;
			*oMutant++ = prevMutant = curMutant;
			} while (--iVectorSize > 0);
		}
	
	else {
		tMutateFunc mutateFunc	= me->params.function.uniformFunc;
		 do {
			double	curSource = *iSource++,
					curTarget = *iTarget++, 
					curMutant;
			curMutant = emphFunc(
							prevMutant,
							mutateFunc(curSource - prevSource, curTarget - prevTarget, iOmega),
							effectDelta);
			
			prevSource = curSource;
			prevTarget = curTarget;
			*oMutant++ = prevMutant = curMutant;
			} while (--iVectorSize > 0);
		}
	
	me->history.source	= prevSource;
	me->history.target	= prevTarget;
	me->history.mutant	= prevMutant;
	}
		
void
CalcRelUniformSR(
	tMutator*	me,
	long		iVectorSize,
	t_sample*	iSource,
	t_sample*	iTarget,
	t_sample*	oMutant,
	t_sample*	iOmega,
	t_sample*	iDelta)
	
	{
	double	prevSource	= me->history.source,
			prevTarget	= me->history.target,
			prevMutant	= me->history.mutant;
	tMutateFunc
			mutateFunc	= me->params.function.uniformFunc;
	
	do {
		double	curSource	= *iSource++,
				curTarget	= *iTarget++,
				omega		= iOmega ? *iOmega++ : me->params.omega,
				delta		= iDelta ? *iDelta++ : me->params.delta,
				mutantInt,
				curMutant;
		
		mutantInt = mutateFunc(curSource - prevSource, curTarget - prevTarget, omega);
		curMutant = (delta >= 0) 	// NB: I treat delta == 0.0 as positive here... this
									// is OK, (cf. comments to the Emphasize...() functions)
									// and special-casing this inside our calculation loop
									// would not buy us anything in terms of performance.
						? EmphasizePos(prevMutant, mutantInt, 1.0 - delta)
						: EmphasizeNeg(prevMutant, mutantInt, 1.0 + delta);
		
		prevSource = curSource;
		prevTarget = curTarget;
		*oMutant++ = prevMutant = curMutant;
		} while (--iVectorSize > 0);
	
	me->history.source	= prevSource;
	me->history.target	= prevTarget;
	me->history.mutant	= prevMutant;
	}
		
