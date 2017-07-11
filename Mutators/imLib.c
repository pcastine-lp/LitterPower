/*
	File:		imLib.c

	Contains:	Common code used by all interval mutation objects.

	Written by:	Peter Castine

	Copyright:	© 2000-04 Peter Castine. All rights reserved.

	Change History (most recent first):

         <5>    8–5–2006    pc      Update to support Jitter matrices. More work may still need to
                                    be done.
         <4>     9–12–04    pc      Update to use inlet_getproxy()
         <3>     10–1–04    pc      Update for Windows, using new LitterLib calls.
         <2>    7–7–2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <1>  30–12–2002    pc      Initial Check-In
*/


/******************************************************************************************

	Previous changes (most recent first):
		15-Nov-2000:	First implementation (based on previous Mutator~.c)

 ******************************************************************************************/

#pragma mark • Include Files

#include "imLib.h"

#ifdef __MWERKS__
	#include <string.h>		// For strcmp(), we seem to need this in CodeWarrior
#endif

#pragma mark • Constants

const double	kMaxPi			= 65535.0 / 65536.0;
const long		kMaxHistory 	= 2048;


const char*		kShortNames[imLast + 1]
						= {"usim", "isim", "uuim", "iuim", "wcm", "lcm"};
const char*		kLongNames[imLast + 1]
						= {	"Uniform Signed Interval Mutation",
							"Irregular Signed Interval Mutation",
							"Uniform Unsigned Interval Mutation",
							"Irregular Unsigned Interval Mutation",
							"Weighted Contour Mutation",
							"Linear Contour Mutation"
							};


/******************************************************************************************
 *
 *	MapStrToMutationType(iStr)
 *
 ******************************************************************************************/

short
MapStrToMutationType(
	const char* iStr)
	{
	short	result;
	
	for (result = imLast; result >= 0; result -= 1) {
		if (strcmp(iStr, kShortNames[result]) == 0) break;
		} 
	
	return result;
	}

/******************************************************************************************
 *
 *	CalcMutationIndex(me)
 *
 ******************************************************************************************/

int
CalcMutationIndex(
	const tMutator* me)
	{
	short result = me->params.irregular;		// Abuse C's Boolean conventions as a
												// cheap typecast
	
	if (me->params.magOnly) {
		result += imUUIM;
		}
	else if (me->params.signOnly) {
		result += imWCM;
		}
	
	return result;
	}

	
#pragma mark -
#pragma mark • Mutation Calculations

/******************************************************************************************
 *
 *	Calc__Mutant(sourceInt, targetInt, omega)
 *
 *	Bread and butter... calculate the Mutant Interval based on source, target,
 *	and Omega for Signed Intervals (SI), Unsigned Intervals (UI), and Linear
 *	Contour (LC)
 *
 *	Includes simplified variants for omega = 1.0 and 0.0. These are helpful with
 *	irregular mutations.
 *
 ******************************************************************************************/

double CalcSIMutant(tInterval iSource, tInterval iTarget, double iOmega)
	{ return iSource + iOmega * (iTarget - iSource); }

tInterval CalcSIMutant1(tInterval source, tInterval iTarget)
	{
	#pragma unused(source)
	
	return iTarget;
	}

double CalcUIMutant(tInterval iSource, tInterval iTarget, double iOmega)
	// Oh dear. The published formula for this mutation is:
	//
	//		 Sign(sourceInt) * (|sourceInt| + omega * ||targetInt| - |sourceInt||)
	//
	// but it doesn't reduce to CalcUIMutant1 for omega = 1.0 if |sourceInt| > |targetInt|
	// and it doesn't fulfill the informally stated goal.
	// Instead, we use
	//
	//		Sign(sourceInt) * (|sourceInt| + omega * (|targetInt| - |sourceInt|))
	//
	// which reduces to:
	{
	if (iSource > 0)		return iSource + iOmega * (Abs(iTarget) - iSource);
	else if (iSource < 0)	return -( iSource + iOmega * (Abs(iTarget) - iSource) );
	else					return 0.0;
	}

tInterval CalcUIMutant1(tInterval sourceInt, tInterval targetInt)
	{ return Sign(sourceInt) * Abs(targetInt); }

	// A "weighted" linear interval is not part of the canonical mutations, but
	// has a certain symmetrical logic...
double CalcWCMutant(tInterval iSource, tInterval iTarget, double iOmega)
	// The forumla for this mutation is
	//
	//		|sourceInt| * (Sign(sourceInt) + omega * (Sign(targetInt) - Sign(sourceInt)))
	//
	// which reduces to
	{
	if (iSource > 0)		return iSource * (1 + iOmega * (Sign(iTarget) - 1));
	else if (iSource < 0)	return -iSource * (-1 + iOmega * (Sign(iTarget) + 1));
	else					return 0.0;
	}

tInterval CalcLCMutant1(tInterval sourceInt, tInterval targetInt)
	{ return Abs(sourceInt) * Sign(targetInt); }

/******************************************************************************************
 *
 *	WannaMutate(iWishMutants, iSamplesLeft, iThresh)
 *
 *	Used by Irregular mutations to calculate if the next interval is to be mutated or not.
 *
 *	We handle clumping as a second-order, two-state Markov Chain.
 *	For this we use two variables, indicating the probability of generating a mutant
 *	depemding upon (a) if the previous output interval was simply the source interval
 *	or (b) if the previous output interval was a mutant. For efficiency, these probabilites
 *	are represented as a threshhold value in [0..kULongMax]. CalcMutantThreshholds() also
 *	returns a value for an initial probability of mutating, derived simply from the mutation
 *	index.
 *
 ******************************************************************************************/

double CalcOmegaPrime(double iOmega, double iPi)
	{ return (iOmega <= 0.0) ? 0.0 : CalcOmegaPrimeCore(iOmega, iPi); }

unsigned long CalcInitThresh(double iOmega)
	{ return (iOmega <= 0.0) ? 0 : CalcInitThreshCore(iOmega); }

unsigned long CalcMutantToMutantThresh(double iOmegaPrime)
	{ return (iOmegaPrime <= 0.0) ? 0 : CalcMutantToMutantThreshCore(iOmegaPrime); }

unsigned long
CalcSourceToMutantThresh(
		double	iOmega,
		double	iOmegaPrime)
	{
	return (iOmega <= 0.0)
				? -1
				: (iOmega >= 1.0)
					? kULongMax
					: CalcSourceToMutantThreshCore(iOmega, iOmegaPrime);
	}

unsigned long
CalcMutantThreshholds(
	double			iOmega,
	double			iPi,
	unsigned long*	oSourceToMutant,
	unsigned long*	oMutantToMutant)
	
	{
	
	if (iOmega <= 0.0) {
		return *oSourceToMutant = *oMutantToMutant = -1;
		}
	else if (iOmega >= 1.0) {
		return *oSourceToMutant = *oMutantToMutant = kULongMax;
		}
	else {
		// Note that the equations for omegaPrime and oSourceToMutant rely on
		// iOmega being in (0.0 .. 1.0) and that iPi must be strictly less than
		// 1.0.
		double omegaPrime	= CalcOmegaPrimeCore(iOmega, iPi);
		*oSourceToMutant	= CalcSourceToMutantThreshCore(iOmega, omegaPrime);
		*oMutantToMutant	= CalcMutantToMutantThreshCore(omegaPrime);
		
		return CalcInitThreshCore(iOmega);
		}
	}


/******************************************************************************************
 *
 *	EmphasizePos(iPrevMutant, iPureInterval, iEffectiveDelta)
 *	EmphasizeNeg(iPrevMutant, iPureInterval, iEffectiveDelta)
 *	EmphasizeZero(iPrevMutant, iPureInterval, iEffectiveDelta)
 *
 *	EffectiveDeltaValue(iDelta)
 *
 *	"Filtering" calculations used with all mutations over relative intervals. 
 *	They calculate the "effective" value for the next Mutant, based on the previous
 *	mutant value (iPrevMutant), the "pure" interval calcluated by the current
 *	mutation algorithm, and an effective delta value.
 *
 *	The "Delta Emphasis" parameter ranges from [-1.0 .. 1.0], but the calculations
 *	are rather different, depending upon whether delta is positive or negative.
 *	The behavior for delta == 0.0 is identical with either variant (one can speak
 *	of a "degenerate positive" or "degenerate negative" case). Due to the structure
 *	of the main calcualtion loops, we can have a small performance improvement by
 *	including a special case EmphasizeZero() version of the emphasis calculations.
 *
 *	To allow the mutation procedures to run more quickly, it is more efficacious
 *	to have them calculate an "effective" delta (1.0 - abs(delta)) outside the
 *	main processing loop.
 *
 ******************************************************************************************/

double EmphasizePos(double iPrevMutant, double iRawInterval, double iEmphFactor)
	{ return iPrevMutant + (iEmphFactor * iRawInterval); }

double EmphasizeNeg(double iPrevMutant, double iRawInterval, double iEmphFactor)
	{ return (iEmphFactor * iPrevMutant) + iRawInterval; }

double EmphasizeZero(double iPrevMutant, double iRawInterval, double iEmphFactor)
	{
	#pragma unused(iEmphFactor)
	
	return iPrevMutant + iRawInterval;
	}


	
#pragma mark -
#pragma mark • Generic Mutator Functions

/******************************************************************************************
 *
 *	Initialize(me, iType, iOmega, iPi, iDelta, iRelInterval)
 *
 ******************************************************************************************/

void
Initialize(
	tMutator*	me,
	int			iType,
	double		iOmega,
	double		iPi,
	double		iDelta,
	Boolean		iRelInterval
	)
	
	{
	
	me->params.omega	= ValidOmega(iOmega);
	me->params.pi		= ValidPi(iPi);
	me->params.delta	= ValidDelta(iDelta);
	
	SetMutatorType(me, iType);
	
	me->params.clumpLen			= 0;			// Currently hard-wired
	me->params.relInterval		= iRelInterval;
	
	#if __IM_MSP__
		#if !__IM_DOUBLEBARREL__
			me->params.omegaSR	= false;
			me->params.piSR		= false;
			me->params.deltaSR	= false;
		#endif
	#endif
	
	DoClear(me);
	}
	
/******************************************************************************************
 *
 *	SetMutatorType(me, iType)
 *
 ******************************************************************************************/

void
SetMutatorType(tMutator* me, short iType)

	{
	
	switch (iType) {
		default:
			// We should never get an undefined iType, but if we do, we just
			// quietly change it to the default (imUSIM) and fall into the 
			// following case. Aren't we glad that the C specification allows
			// the default label to occur anywhere within a switch block?!
		case imUSIM:
			me->params.function.uniformFunc = CalcSIMutant;
			me->params.irregular	= false;
			me->params.magOnly		= false;
			me->params.signOnly		= false;
			break;
		case imISIM:
			me->params.function.irregularFunc = CalcSIMutant1;
			me->params.irregular	= true;
			me->params.magOnly		= false;
			me->params.signOnly		= false;
			break;
		case imUUIM:
			me->params.function.uniformFunc = CalcUIMutant;
			me->params.irregular	= false;
			me->params.magOnly		= true;
			me->params.signOnly		= false;
			break;
		case imIUIM:
			me->params.function.irregularFunc = CalcUIMutant1;
			me->params.irregular	= true;
			me->params.magOnly		= true;
			me->params.signOnly		= false;
			break;
		case imWCM:
			me->params.function.uniformFunc = CalcWCMutant;
			me->params.irregular	= false;
			me->params.magOnly		= false;
			me->params.signOnly		= true;
			break;
		case imLCM:
			me->params.function.irregularFunc = CalcLCMutant1;
			me->params.irregular	= true;
			me->params.magOnly		= false;
			me->params.signOnly		= true;
			break;
		}
	
#if __IM_HISTORY__
	if (me->history.bufSize > 0) {
		if (me->history.bufIndex == 0)
				UpdateFrameParams(me);
		else	me->paramUpdatePending = true;	
		}
#endif
	
	}


#pragma mark -
#pragma mark • Generic Mutation Object Methods

/******************************************************************************************
 *
 *	DoBang(me)
 *
 ******************************************************************************************/

#if !__IM_MSP__ && !__IM_JITTER__

	void DoBang(tMutator* me)
		{ outlet_float(me->coreObject.o_outlet, me->history.mutant); }
	
#endif


/******************************************************************************************
 *
 *	DoOmega(me, iOmega)
 *	DoDelta(me, iDelta)
 *	DoPi(me, iPi)
 *	DoFloat(me, iValue)
 *
 *	DoFloat() is only used in the MSP version (to allow Omega, Delta, and Pi) to be accessed
 *	either by signals or floats.
 *
 ******************************************************************************************/

void DoOmega(tMutator* me, double iOmega)
	{ me->params.omega = ValidOmega(iOmega); }

void DoPi(tMutator* me, double iPi)
	{ me->params.pi = ValidPi(iPi); }

void DoDelta(tMutator* me, double iDelta)
	{ me->params.delta = ValidDelta(iDelta); }


#if __IM_MSP__

	void
	DoFloat(
		tMutator*	me,
		double		iValue)
		
		{
		const long	kInlet = ObjectGetInlet((Object*) me, me->coreObject.z_in);
		
		switch (kInlet) {
			case inletOmega:
				DoOmega(me, iValue);
				break;
			case inletDelta:
				DoDelta(me, iValue);
				break;
			case inletPi:
				DoPi(me, iValue);
				break;
			default:
				error("%s doesn't handle floats in inlet %ld", kClassName, kInlet + 1);
				break;
			}
		
		#if __IM_HISTORY__
			if (me->history.bufSize > 0) {
				if (me->history.bufIndex == 0)
						UpdateFrameParams(me);
				else	me->paramUpdatePending = true;	
				}
		#endif
	
		}
		
#endif


/******************************************************************************************
 *
 *	DoClear(me)
 *
 ******************************************************************************************/

void
DoClear(
	tMutator*	me)
	
	{
#if __IM_HISTORY__

	long			index;
	tMutationPtr	mPtr;
	BytePtr			sPtr;
	
	index	= me->history.bufSize;
	mPtr	= me->history.muBuffer + index;
	do {
		(--mPtr)->source1	= 0;
		mPtr->source2		= 0;
		mPtr->target1		= 0;
		mPtr->target2		= 0;
		mPtr->mutant1		= 0;
		mPtr->mutant2		= 0;
		
		} while (--index > 0);
		
	index	= me->history.nyquist;
	sPtr	= me->history.stateBuf + index;
	do { *(sPtr--)	&= stateFollowLeadBit; }
		while (--index >= 0);
	
#else

	me->history.source	= 0;
	me->history.target	= 0;
	me->history.mutant	= 0;
	me->history.state	= stateIndeterminate;

#endif	
	}


/******************************************************************************************
 *
 *	DoLCM(me)
 *	DoUSIM(me)
 *	DoISIM(me)
 *	DoUUIM(me)
 *	DoIUIM(me)
 *
 ******************************************************************************************/

void DoUSIM(tMutator* me)	{ SetMutatorType(me, imUSIM); }
void DoISIM(tMutator* me)	{ SetMutatorType(me,imISIM); }
void DoUUIM(tMutator* me)	{ SetMutatorType(me, imUUIM); }
void DoIUIM(tMutator* me)	{ SetMutatorType(me, imIUIM); }
void DoWCM(tMutator* me)	{ SetMutatorType(me, imWCM); }
void DoLCM(tMutator* me)	{ SetMutatorType(me, imLCM); }


/********************************************************************************
 *
 *	DoAbsInt(me)
 *	DoRelInt(me, iDelta)
 *
 ********************************************************************************/

void
DoAbsInt(
	tMutator*	me)
	
	{
	
	me->params.relInterval = false;
	
#if __IM_HISTORY__
	if (me->history.bufSize > 0) {
		if (me->history.bufIndex == 0)
				UpdateFrameParams(me);
		else	me->paramUpdatePending = true;	
		}
#endif
	}

void
DoRelInt(
	tMutator*	me,
	float		iDelta)
	
	{
	
#if __IM_HISTORY__
	if (me->history.bufSize == 0) {
		error("Can't perform relative intervals on this %s", kClassName);
		return;
		}
#endif

	me->params.relInterval = true;
	
//	DoClear(me);
	DoDelta(me, iDelta);
	
#if __IM_HISTORY__
	// ASSERT: (me->history.bufSize > 0)
	if (me->history.bufIndex == 0)
			UpdateFrameParams(me);
	else	me->paramUpdatePending = true;	
#endif
	}

/******************************************************************************************
 *
 *	DoControlRate(me)
 *	DoSampleRate(me)
 *
 ******************************************************************************************/

#if __IM_MSP__ && !__IM_DOUBLEBARREL__

void
DoControlRate(
	tMutator*	me)
	
	{
	
	switch ( ObjectGetInlet((Object*) me, me->coreObject.z_in) ) {
		case inletOmega:
			// Clear Omega Sample Rate bit to indicate use of control rate
			me->params.omegaSR	= false;
			break;
		case inletPi:
			// Clear Clumping Factor bit
			me->params.piSR		= false;
			break;
		case inletDelta:
			// Clear Delta Sample Rate bit
			me->params.deltaSR	= false;
			break;
		default:
			// Clear all three Sample Rate bits
			me->params.omegaSR	= false;
			me->params.piSR		= false;
			me->params.deltaSR	= false;
			break;
		}
	
	}

void
DoSampleRate(
	tMutator*	me)
	
	{
	
	switch ( ObjectGetInlet((Object*) me, me->coreObject.z_in) ) {
		case inletOmega:
			// Set Omega Sample Rate bit to indicate use of sample rate
			me->params.omegaSR	= true;
			break;
		case inletPi:
			// Set Clumping Factor bit
			me->params.piSR		= true;
			break;
		case inletDelta:
			// Set Delta Sample Rate bit
			me->params.deltaSR	= true;
			break;
		default:
			// Set all three Sample Rate bits
			me->params.omegaSR	= true;
			me->params.piSR		= true;
			me->params.deltaSR	= true;
			break;
		}
	
	}
#endif
