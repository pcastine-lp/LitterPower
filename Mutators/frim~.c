/*
	File:		frim~.c

	Contains:	Frequency-domain mutation. In particular, maintains parallel handling of amplitude
				and phase signals in irregular mutations.

	Written by:	Peter Castine

	Copyright:	© 2000-2003 Peter Castine. All rights reserved.

	Change History (most recent first):

         <6>     10–1–04    pc      Update for Windows, using new LitterLib calls.
         <5>    7–7–2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <4>   22–3–2003    pc      Move dsp_free to beginning of free routine (new recommendation
                                    from C74).
         <3>   22–3–2003    pc      Fix Kern Protection Fault on OS X. Not sure why this never burnt
                                    in Boron days.
         <2>  30–12–2002    pc      Add object version to DoInfo()
         <1>  30–12–2002    pc      Initial Check-In
*/


/******************************************************************************************
	Change History (most recent first):
		17-Nov-2000:	First implementation (based on previous Mutator~.c)

 ******************************************************************************************/

#pragma mark • Include Files

#include "imLib.h"
#include "TrialPeriodUtils.h"

#pragma mark • Constants

const char*	kClassName		= "lp.frim~";			// Class name


#pragma mark • Function Prototypes

void*	NewMutator(Symbol*, short, Atom*);
void	FreeMutator(tMutator*);

	// Various Max messages
void	DoAssist(tMutator*, void* , long , long , char*);
void	DoInfo(tMutator*);
void	DoTattle(tMutator*);

void	DoOBands(tMutator*, long);

	// MSP messages
void	BuildDSPChain(tMutator*, t_signal**, short*);
int*	PerformMutator(int*);
	// The following routines are where the actual work is done.
	// The first parameters are always: Mutator object, vector size, Source signal, Target
	// signal, and Mutant signal. The three signals are guaranteed never to be NIL.
	// These parameters are all followed by omega. If the mutation is irregular, the 
	// parameter pi follows. And, finally, if the mutation is not using absolute values
	// (i.e., it history >= 1) there is a delta parameter.
	// With control-rate mutations, omega, delta and pi are floats (taken from the first
	// sample of the signal if non-NIL, otherwise the current values from the Mutator object
	// are taken).
void	CalcAbsIrreg(	tMutator*, long,
						t_sample*, t_sample*, t_sample*, t_sample*, t_sample*, t_sample*,
						t_sample, t_sample);
void	CalcAbsUniform(tMutator*, long,
						t_sample*, t_sample*, t_sample*, t_sample*, t_sample*, t_sample*,
						t_sample);
void	CalcRelIrreg(	tMutator*, long,
						t_sample*, t_sample*, t_sample*, t_sample*, t_sample*, t_sample*,
						t_sample, t_sample, t_sample);
void	CalcRelUniform(tMutator*, long,
						t_sample*, t_sample*, t_sample*, t_sample*, t_sample*, t_sample*,
						t_sample, t_sample);


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
			(method) FreeMutator,		// Custom deallocation function
			(short) sizeof(tMutator),	// Class object size
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
	addmess ((method) LitterVers,	"vers",		A_DEFLONG, A_DEFLONG, 0);
	
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
	
	addmess ((method) DoOBands,		"obands",	A_DEFLONG, 0);

	// Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();

	}

#pragma mark -
#pragma mark • Helper functions

/******************************************************************************************
 *
 *	UpdateFrameParams(me)
 *	RecalcBandLeaders(me, iBpO)
 *	HasHistory(me)
 *
 ******************************************************************************************/
	
	static inline Boolean HasHistory(tMutator* me)
		{ return me->history.bufSize > 0; }
	
	static void RecalcBandLeaders(
		tMutator*	me,
		long		iBpO)		// Bands per Octave
		
		{
		long	nyquist		= me->history.nyquist;
		BytePtr	stateBuf;
		
		if ( !HasHistory(me) )
			return;
		
		stateBuf =	me->history.stateBuf + 3;		// Skip DC component and first two
													// frequency bins; they are  **always**
													// "Band Leaders"
		
		if (iBpO > 0) {
			// We flag the first bin of every frequency band as a "Band Leader"
			// This algorithm may look as if it could easily be optimized, but the 
			// ecology is very fragile. Be extremely cautious with any attempt at
			// modification.
			long	curBin			= 2,
					nextOctave		= 4,
					bandIndex		= 0;
			double	bandRatio		= pow(2.0, 1.0 / (double) iBpO),
					nextLeadBinFP	= bandRatio + bandRatio;	// == curBin * bandRatio
			long	nextLeadBinL	= nextLeadBinFP + 0.5;
					// Maintain nextLeadBin in floating point format for accuracy and in
					// integer format for efficiency during comparisons
			do	{
				if (++curBin < nextLeadBinL)
					*stateBuf++ |= stateFollowLeadBit;
				else {
					// ASSERT: curBin == nextLeadBinL
					// In other words, this is the next "Band Leader"
					*stateBuf++ &= ~stateFollowLeadBit;
					if (++bandIndex <= iBpO) {
						nextLeadBinFP	*= bandRatio;
						nextLeadBinL	= nextLeadBinFP + 0.5;
						if (nextLeadBinL <= curBin) nextLeadBinL += 1;
						}
					else nextLeadBinFP = nextOctave;
					
					if (nextLeadBinL > nextOctave) {
						nextLeadBinFP = nextLeadBinL = nextOctave;
						bandIndex = 0;
						}
					
					if (curBin == nextOctave) nextOctave += nextOctave;
					}
				} while (curBin < nyquist);
			
			// We're now at the nyquist bin, which is a special case; it is always
			// a band follower (this is sort of cheesy, but there are no bins to
			// follow it and there is normally not enough energy up around here
			// to make it worth worrying much about).
			*stateBuf |= stateFollowLeadBit;
			}
		
		else do {
			// No grouping of bins into frequency bands; every bin is a "Band Leader"
			*stateBuf++ &= ~stateFollowLeadBit;
			} while (--nyquist >= 0);
		
		}

	void UpdateFrameParams(tMutator* me)
		{

		if (me->params.bandsPerOctave != me->frameParams.bandsPerOctave) {
			RecalcBandLeaders(me, me->params.bandsPerOctave);
			}
		
		if (me->params.irregular && !me->frameParams.irregular) {
			// Starting an irregular mutation, so set state to stateIndeterminate,
			// leaving the stateFollowLeadBit intact
			long	bufCounter	= me->history.nyquist;
			BytePtr	stateBuf	= me->history.stateBuf;
			
			do { *stateBuf++ &= stateFollowLeadBit; }
				while (--bufCounter >= 0);
			}
		
		me->frameParams	= me->params;
		
		me->paramUpdatePending = false;
		}


#pragma mark -
#pragma mark • Class Message Handlers

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
	Boolean			gotInitType		= false,		// The gotInitXXX variables are for 
					gotHistSize		= false,		// tracking which arguments were typed
					gotInitOmega	= false,		// into our object box.
					gotInitPi		= false,
					gotInitDelta	= false;
	short			initType		= imDefault;	// Values to use if nothing specified
	long			histSize		= 0,			// in the object box.
					nyquist			= 0;
	double			initOmega		= 0.0,
					initPi			= 0.0,
					initDelta		= 0.0;
	tMutationPtr	muBuffer		= NIL;
	BytePtr			stateBuffer		= NIL;
	
	tMutator*		me				= NIL;
	
	// Parse arguments
	while (iArgCount-- > 0) {
		switch (iArgVector->a_type) {
			case A_LONG:
				if (gotHistSize) goto punt_argument;			// spurious argument
				
				gotHistSize = true;
				histSize = iArgVector->a_w.w_long;
				if (histSize < 0) 					histSize = 0;
				else if (histSize > kMaxHistory)	histSize = kMaxHistory;
				
				nyquist = histSize / 2;						// Compiler optimizes to right-shift
				break;
			
			case A_FLOAT:
				if (!gotInitOmega) {
					gotInitOmega = true;
					initOmega = iArgVector->a_w.w_float;	// Validated in Initialize()
					}
				else if (!gotInitPi) {
					gotInitPi = true;
					initPi = iArgVector->a_w.w_float;		// Validated in Initialize()
					}
				else if (!gotInitDelta) {
					gotInitDelta = true;
					initDelta = iArgVector->a_w.w_float;	// Validated in Initialize()
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
	
	if (histSize > 0) {
		// This is the critical point in initialization; allocating memory
		// for storing Source, Target, and Mutant signal values for the length
		// specified by histSize.
		muBuffer = (tMutationPtr) NewPtrClear(histSize * sizeof(tMutation));
			if (muBuffer == NIL) goto punt;
		stateBuffer = (BytePtr) NewPtrClear((nyquist + 1) * sizeof(Byte));
			if (stateBuffer == NIL) goto punt;
		}
	
	// Let Max/MSP allocate us and give us inlets;
	// number of inlets depends upon which variant we are
	me = (tMutator*) newobject(gObjectClass);
	dsp_setup(&(me->coreObject), histSize > 0 ? 7 : 6);
	
	// And three outlets, set these up right-to-left
	// 4-Jan-2002: now only two outlets
	outlet_new(me, "signal");
	outlet_new(me, "signal");
	
	// Set up default/initial values
	// We need to set up history values before calling the std. Initialize()
	me->history.bufSize		= histSize;
	me->history.bufIndex	= 0;
	me->history.nyquist		= nyquist;
	me->history.muBuffer	= me->history.curMu
							= muBuffer;
	me->history.stateBuf	= me->history.curState
							= stateBuffer;
	
	me->params.omega		= ValidOmega(initOmega);
	me->params.pi			= ValidPi(initPi);
	me->params.delta		= ValidDelta(initDelta);
	me->params.clumpLen		= 0;
	me->params.bandsPerOctave = 0;
	me->params.relInterval	= gotInitDelta;
	
	DoOBands(me, 0);					// Calls RecalcBandLeaders()
	SetMutatorType(me, initType);		// Calls UpdateFrameParams()
						
	return me;

//
//	Exception handling
//	
punt:
	if (muBuffer != NIL)	DisposePtr((Ptr) muBuffer);
	if (stateBuffer != NIL)	DisposePtr((Ptr) stateBuffer);
		
	return NIL;
	}

/******************************************************************************************
 *
 *	FreeMutator(me)
 *
 ******************************************************************************************/

void
FreeMutator(
	tMutator* me)
	
	{
	
	dsp_free(&(me->coreObject));
	
	if ( HasHistory(me) ) {
		DisposePtr((Ptr) me->history.muBuffer);
		DisposePtr((Ptr) me->history.stateBuf);
		}
	
	}

#pragma mark -
#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	DoAssist
 *	DoInfo(me)
 *	DoTattle(me)
 *
 *	Fairly generic implementations
 *
 ******************************************************************************************/

void DoAssist(tMutator* me, void* box, long iDir, long iArgNum, char* oCStr)
	{
	#pragma unused(me, box)
	
	LitterAssist(iDir, iArgNum, strIndexInSource1, strIndexOutMutant1, oCStr);
	}

void DoInfo(tMutator* me)
	{ LitterInfo(kClassName, &me->coreObject.z_ob, (method) DoTattle); }

void
DoTattle(
	tMutator* me)
	
	{
	
	post("%s state", kClassName);
	post("  Mutation type: %s", kLongNames[CalcMutationIndex(me)]);
	
	post("  Mutation index (if no signal): %lf", me->params.omega);
	
	if (me->params.irregular) {
		post("  Clumping factor (if no signal): %lf", me->params.pi);
		// Are we simulating fixed-Q filter banks?
		if (me->params.bandsPerOctave > 0)
				post("  Simulating filter bank with %d bands per octave", me->params.bandsPerOctave);	
		else	post("  Each frequency bin mutated individually (obands 0)");
		}
		
	if (me->params.relInterval) {
		post("  Using interval distance of %d", me->history.bufSize);
		post("    Delta Emphasis (if no signal): %lf", me->params.delta);
		}
	else post("  Using Absolute Intervals");

	post("  history buffer at item %ld", (long) me->history.bufIndex);
	post("  Frame Params: omega/pi/delta %lf, %lf, %lf",
			me->frameParams.omega,
			me->frameParams.pi,
			me->frameParams.delta);
	post("    ireg/mag/sign/rel//bpo %ld/%ld/%ld/%ld//%ld",
			(long) me->frameParams.irregular,
			(long) me->frameParams.magOnly,
			(long) me->frameParams.signOnly,
			(long) me->frameParams.relInterval,
			(long) me->frameParams.bandsPerOctave);
	post("  User Params: omega/pi/delta %lf, %lf, %lf",
			me->params.omega,
			me->params.pi,
			me->params.delta);
	post("    ireg/mag/sign/rel//bpo %ld/%ld/%ld/%ld//%ld",
			(long) me->params.irregular,
			(long) me->params.magOnly,
			(long) me->params.signOnly,
			(long) me->params.relInterval,
			(long) me->params.bandsPerOctave);
	}


/******************************************************************************************
 *
 *	DoOBands(me, iBpO)
 *
 *	Updates Octave Bands setting
 *
 ******************************************************************************************/

void
DoOBands(
	tMutator*	me,
	long		iBpO)
	
	{
	// Check range
	if		(iBpO < 0)		iBpO = 0;
	else if (iBpO > 15)		iBpO = 15;
	
	me->params.bandsPerOctave = iBpO;
	
	if (me->history.bufIndex == 0) {
		me->frameParams.bandsPerOctave = iBpO;
		RecalcBandLeaders(me, iBpO);
		}
	}

#pragma mark -
#pragma mark • DSP Utilities

/******************************************************************************************
 *
 *	HasHistory(me)
 *	GetState(me)
 *	GetHistory(me, oSource1, oSource2, oTarget1, oTarget2, oMutant1, oMutant2)
 *	PutState(me, iState)
 *	PutHistory(me, iSource1, iSource2, iTarget1, iTarget2, iMutant1, iMutant2)
 *	WrapUpMutation(	me, oMutant1Sig, oMutant2Sig,
 *					iSource1, iSource2, iTarget1, iTarget2,
 *					iMutant1, iMutant2)
 *
 *	PutState() and PutHistory() have the side effect of updating the relavent pointers to
 *	the next item in the ring buffers.
 *
 ******************************************************************************************/

	static inline Byte GetState(tMutator* me)
		{ 
		Byte result = *(me->history.curState);
		
		if (me->history.bufIndex > me->history.nyquist)
			result |= stateFollowLeadBit;
		
		return result;
		}
	
	static inline void GetHistory(
		tMutator*	me,
		double*		oSource1,
		double*		oSource2,
		double*		oTarget1,
		double*		oTarget2,
		double*		oMutant1,
		double*		oMutant2)
		
		{
		tMutationPtr	muPtr = me->history.curMu;
		
		*oSource1 = (double) muPtr->source1;
		*oSource2 = (double) muPtr->source2;
		*oTarget1 = (double) muPtr->target1;
		*oTarget2 = (double) muPtr->target2;
		*oMutant1 = (double) muPtr->mutant1;
		*oMutant2 = (double) muPtr->mutant2;
		}
	
	// NB: Always call PutState() before PutHistory()/WrapUpMutation()!!!
	static inline void PutState(tMutator* me, Byte iState)
		{
		BytePtr curStatePtr	= me->history.curState;
		long	nyqOffset	= me->history.nyquist - me->history.bufIndex;
				
		if (nyqOffset > 0) {
			// ASSERT: bufIndex in [0..nyquist-1]
			// So: check if the current bin is a "Band Leader" and, if so, update
			// it *and* all the dependent "Band Followers".
			if ((*curStatePtr & stateFollowLeadBit) == 0) {
				*curStatePtr++ = iState;
				iState += stateFollowLeadBit;
				
				while (--nyqOffset >= 0 && (*curStatePtr & stateFollowLeadBit)) {
					*curStatePtr++ = iState;
					}
				}
			
			me->history.curState += 1;
			}
		
		else {
			// The Nyquist bin may have a "mind of its own", so we have to 
			if (nyqOffset == 0 && (*curStatePtr & stateFollowLeadBit) == 0)
				*curStatePtr = iState;
			
			me->history.curState -= 1;
			}
		
		}
	
	static void PutHistory(
		tMutator*	me,
		double		iSource1,
		double		iSource2,
		double		iTarget1,
		double		iTarget2,
		double		iMutant1,
		double		iMutant2)
		{
		tMutationPtr	muPtr = me->history.curMu;
		
		muPtr->source1 = (tInterval) iSource1;
		muPtr->source2 = (tInterval) iSource2;
		muPtr->target1 = (tInterval) iTarget1;
		muPtr->target2 = (tInterval) iTarget2;
		muPtr->mutant1 = (tInterval) iMutant1;
		muPtr->mutant2 = (tInterval) iMutant2;
		
		// Update pointers in ring buffer
		if (++(me->history.bufIndex) == me->history.bufSize) {
			// Hit end of ring buffer; wrap around to beginning & update frame parameters
			me->history.bufIndex	= 0;
			me->history.curMu		= me->history.muBuffer;
			if (me->paramUpdatePending) UpdateFrameParams(me);
			}
		else me->history.curMu += 1;	// Just nudge history up to next bin
		}
	
	static void WrapUpRegMutation(
		tMutator*	me,
		double		iSource1,
		double		iSource2,
		double		iTarget1,
		double		iTarget2,
		double		iMutant1,
		double		iMutant2)
		{
		
		if ( HasHistory(me) ) {
			// Even though we may currently be using absolute intervals, the user may want
			// to switch over to relative intervals at any time, so we need to maintain
			// history...
			PutHistory(	me, iSource1, iSource2, iTarget1, iTarget2, iMutant1, iMutant2);
			}
		
		}
		
	static void WrapUpIrregMutation(
		tMutator*	me,
		double		iSource1,
		double		iSource2,
		double		iTarget1,
		double		iTarget2,
		double		iMutant1,
		double		iMutant2,
		Byte		iState)
		{
		
		if ( HasHistory(me) ) {
			// Even though we may currently be using absolute intervals, the user may want
			// to switch over to relative intervals at any time, so we need to maintain
			// history...
			PutState(me, iState);
			PutHistory(me, iSource1, iSource2, iTarget1, iTarget2, iMutant1, iMutant2);
			}
		
		}
		
	
	
#pragma mark -
#pragma mark • DSP Methods

/******************************************************************************************
 *
 *	BuildDSPChain(iAtom, iParam, iMaxParamLen)
 *
 ******************************************************************************************/

void
BuildDSPChain(
	tMutator*	me,
	t_signal**	ioDSPVectors,
	short*		iConnectCounts)
	
	{
	// We may need to compensate for objects with no delta emphasis inlet
	Boolean	hasDelta		= HasHistory(me);
	int		outlet1Index	= hasDelta ? outletMutant1 : (outletMutant1 - 1);
	
	dsp_add(
		PerformMutator, 11,						// Preform method and # parameters to pass
		me,											//	1: The performing Mutator
		ioDSPVectors[inletSource1]->s_n,			//	2: Signal vector size
		ioDSPVectors[inletSource1]->s_vec,			//	3: Source signal (real or amplitude)
		ioDSPVectors[inletSource2]->s_vec,			//	4: Source signal (imaginary or phase)
		ioDSPVectors[inletTarget1]->s_vec,			//	5: Target signal (real/amplitude)
		ioDSPVectors[inletTarget2]->s_vec,			//	6: Target signal (imaginary/phase)
		
		iConnectCounts[inletOmega] > 0				//	7: Mutation Index signal
			? ioDSPVectors[inletOmega]->s_vec
			: NIL,
		iConnectCounts[inletPi] > 0					//	8: Clumping Factor signal
			? ioDSPVectors[inletPi]->s_vec
			: NIL,
		(hasDelta && iConnectCounts[inletDelta] > 0) //	9: Delta Emphasis signal
			? ioDSPVectors[inletDelta]->s_vec
			: NIL,
		
		ioDSPVectors[outlet1Index]->s_vec,			//	10: Mutant (out, real/amp)
		ioDSPVectors[outlet1Index + 1]->s_vec		//	11: Mutant (out, imag/phase)
		);
	
	}
	

/******************************************************************************************
 *
 *	PerformMutator(iParams)
 *
 *	Parameter block contains 12 values:
 *		- Address of this function (MSP convention)
 *		- The performing Mutator object
 *		- Current signal vector size
 *		- Source signals
 *		- Target signals
 *		- Mutation Index signal (may be NIL)
 *		- Clumping signal		(may be NIL)
 *		- Delta Emphasis signal (may be NIL)
 *		- Mutant signals (out)
 *
 *	The perform method routes to the specific mutation performance routine appropriate
 *	for the current value of mutationType, etc.
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
		paramSource1,
		paramSource2,
		paramTarget1,
		paramTarget2,
		paramOmega,
		paramPi,
		paramDelta,
		paramMutant1,
		paramMutant2,
		
		paramNextLink
		};
	
	tMutator*		me	= (tMutator*) iParams[paramMe];
	tMutationParams	curParams;
	
	if (me->coreObject.z_disabled) goto exit;
	
	// Copy mutation parameters; we use them a lot
	curParams = ( HasHistory(me) )
					? me->frameParams
					: me->params;
	
	// Parameters for the vector calculation method depend upon whether the mutation
	// is irregular or uniform; if absolute or relative intervals are used; and if
	// there are any signals that need to be sampled at audio rate.
	
	if (curParams.irregular) {
		// Irregular Mutations
		if (curParams.relInterval) {
			// Irregular w/relative intervals
			CalcRelIrreg(
				me, (long) iParams[paramVectorSize],
				(t_sample*) iParams[paramSource1], (t_sample*) iParams[paramSource2],
				(t_sample*) iParams[paramTarget1], (t_sample*) iParams[paramTarget2],
				(t_sample*) iParams[paramMutant1], (t_sample*) iParams[paramMutant2],
				iParams[paramOmega]
					? *((t_sample*) iParams[paramOmega])
					: (t_sample) curParams.omega,
				iParams[paramPi]
					? *((t_sample*) iParams[paramPi])
					: (t_sample) curParams.pi,
				iParams[paramDelta]
					? *((t_sample*) iParams[paramDelta])
					: (t_sample) curParams.delta);
			}
		else {
			// Irregular w/Absolute Intervals
			CalcAbsIrreg(
				me, (long) iParams[paramVectorSize],
				(t_sample*) iParams[paramSource1], (t_sample*) iParams[paramSource2],
				(t_sample*) iParams[paramTarget1], (t_sample*) iParams[paramTarget2],
				(t_sample*) iParams[paramMutant1], (t_sample*) iParams[paramMutant2],
				iParams[paramOmega]
					? *((t_sample*) iParams[paramOmega])
					: (t_sample) curParams.omega,
				iParams[paramPi]
					? *((t_sample*) iParams[paramPi])
					: (t_sample) curParams.pi);
			}
		}
		
	else {
		// Uniform Mutations
		if (curParams.relInterval) {
			// Uniform w/Relative intervals
			CalcRelUniform(
				me, (long) iParams[paramVectorSize],
				(t_sample*) iParams[paramSource1], (t_sample*) iParams[paramSource2],
				(t_sample*) iParams[paramTarget1], (t_sample*) iParams[paramTarget2],
				(t_sample*) iParams[paramMutant1], (t_sample*) iParams[paramMutant2],
				iParams[paramOmega]
					? *((t_sample*) iParams[paramOmega])
					: (t_sample) curParams.omega,
				iParams[paramDelta]
					? *((t_sample*) iParams[paramDelta])
					: (t_sample) curParams.delta);
			}
		else {
			// Uniform w/Absolute Intervals
			CalcAbsUniform(
				me, (long) iParams[paramVectorSize],
				(t_sample*) iParams[paramSource1], (t_sample*) iParams[paramSource2],
				(t_sample*) iParams[paramTarget1], (t_sample*) iParams[paramTarget2],
				(t_sample*) iParams[paramMutant1], (t_sample*) iParams[paramMutant2],
				iParams[paramOmega]
					? *((t_sample*) iParams[paramOmega])
					: (t_sample) curParams.omega);
			}
		}
	
	 

exit:
	return iParams + paramNextLink;
	}

/******************************************************************************************
 *
 *	CalcAbsIrreg(me, iVectorSize, iSource1, iSource2, iTarget1, iTarget2,
 *					oMutant1, oMutant2, iOmega, iPi)
 *
 ******************************************************************************************/

void
CalcAbsIrreg(
	tMutator*	me,
	long		iVectorSize,
	t_sample*	iSource1,
	t_sample*	iSource2,
	t_sample*	iTarget1,
	t_sample*	iTarget2,
	t_sample*	oMutant1,
	t_sample*	oMutant2,
	t_sample	iOmega,
	t_sample	iPi)
	
	{
	double		curSource1,
				curSource2,
				curTarget1,
				curTarget2,
				curMutant1,
				curMutant2;
	Boolean		hasHist		= HasHistory(me);
	tMutate1Func mutateFunc	= hasHist
								? me->frameParams.function.irregularFunc
								: me->params.function.irregularFunc;
	
	if (iOmega <= 0.0) do {
		// With Ω == 0, the mutant is simply the source. Note, however, that the mutant
		// values may need to be corrected to current range settings whereas the source
		// values (used in history maintenance) are not range-checked.
		curMutant1 = curSource1 = *iSource1++,
		curMutant2 = curSource2 = *iSource2++;
		
		WrapUpIrregMutation(me, curSource1, curSource2, *iTarget1++, *iTarget2++,
							curMutant1, curMutant2, stateSource);
		
		*oMutant1++ = curMutant1;
		*oMutant2++ = curMutant2;
		} while (--iVectorSize > 0);
	
	else if (1.0 <= iOmega) do {
		curSource1 = *iSource1++;
		curSource2 = *iSource2++;
		curTarget1 = *iTarget1++;
		curTarget2 = *iTarget2++;
		curMutant1 = mutateFunc(curSource1, curTarget1),
		curMutant2 = mutateFunc(curSource2, curTarget2);
		
		WrapUpIrregMutation(me, curSource1, curSource2, curTarget1, curTarget2,
							curMutant1, curMutant2, stateTarget);
		
		*oMutant1++ = curMutant1;
		*oMutant2++ = curMutant2;
		} while (--iVectorSize > 0);
	
	else {
		// iOmega in (0.0 .. 1.0)
		Byte			curState	= stateIndeterminate;
		unsigned long	s2mThresh,
						m2mThresh,
						i2mThresh	= CalcMutantThreshholds(iOmega, iPi, &s2mThresh, &m2mThresh);
		
		do {
			unsigned long curThresh;
			curSource1 = *iSource1++,
			curSource2 = *iSource2++,
			curTarget1 = *iTarget1++,
			curTarget2 = *iTarget2++;
			
			if (hasHist) curState	= GetState(me);
			
			if (curState & stateFollowLeadBit) {
				if (curState == stateFollowLeadBit + stateTarget) {
					curMutant1	= mutateFunc(curSource1, curTarget1);
					curMutant2	= mutateFunc(curSource2, curTarget2);
					}
				else {
					curMutant1	= curSource1;
					curMutant2	= curSource2;
					}
				}
			
			else {
				switch (curState) {
					case stateSource:
						curThresh = s2mThresh;
						break;
					case stateTarget:
						curThresh = m2mThresh;
						break;
					default:
						// ASSERT: (curState == stateIndeterminate)
						curThresh = i2mThresh;
						break;
					}
			
				if ( WannaMutateStrict(curThresh) ) {
					curState	= stateTarget;
					curMutant1	= mutateFunc(curSource1, curTarget1);
					curMutant2	= mutateFunc(curSource2, curTarget2);
					}
				else {
					curState	= stateSource;
					curMutant1	= curSource1;
					curMutant2	= curSource2;
					}
				
				}
			
			WrapUpIrregMutation(me, curSource1, curSource2, curTarget1, curTarget2,
								curMutant1, curMutant2, curState);
		
			*oMutant1++ = curMutant1;
			*oMutant2++ = curMutant2;
			} while (--iVectorSize > 0);
		}
	
	}

/******************************************************************************************
 *
 *	CalcAbsUniform(me, iVectorSize, iSource, iTarget, oMutant, iOmega)
 *
 ******************************************************************************************/

void
CalcAbsUniform(
	tMutator*	me,
	long		iVectorSize,
	t_sample*	iSource1,
	t_sample*	iSource2,
	t_sample*	iTarget1,
	t_sample*	iTarget2,
	t_sample*	oMutant1,
	t_sample*	oMutant2,
	t_sample	iOmega)
	
	{
	double		curSource1,
				curSource2,
				curTarget1,
				curTarget2,
				curMutant1,
				curMutant2;
	tMutateFunc mutateFunc	= HasHistory(me)
								? me->frameParams.function.uniformFunc
								: me->params.function.uniformFunc;
	
	if (iOmega <= 0.0) do {
		curMutant1 = curSource1 = *iSource1++,
		curMutant2 = curSource2 = *iSource2++;
		
		WrapUpRegMutation(	me,  curSource1, curSource2, *iTarget1++, *iTarget2++,
							curMutant1, curMutant2);
		
		*oMutant1++ = curMutant1;
		*oMutant2++ = curMutant2;
		} while (--iVectorSize > 0);
	
	else do {
		curSource1 = *iSource1++;
		curSource2 = *iSource2++;
		curTarget1 = *iTarget1++;
		curTarget2 = *iTarget2++;
		curMutant1 = mutateFunc(curSource1, curTarget1, iOmega),
		curMutant2 = mutateFunc(curSource2, curTarget2, iOmega);
		
		WrapUpRegMutation(	me, curSource1, curSource2, curTarget1, curTarget2,
							curMutant1, curMutant2);
		
		*oMutant1++ = curMutant1;
		*oMutant2++ = curMutant2;
		} while (--iVectorSize > 0);
	
	}
		

/******************************************************************************************
 *
 *	CalcRelIrreg(me, iVectorSize, iSource, iTarget, oMutant, iOmega, iPi, iDelta)
 *
 ******************************************************************************************/

void
CalcRelIrreg(
	tMutator*	me,
	long		iVectorSize,
	t_sample*	iSource1,
	t_sample*	iSource2,
	t_sample*	iTarget1,
	t_sample*	iTarget2,
	t_sample*	oMutant1,
	t_sample*	oMutant2,
	t_sample	iOmega,
	t_sample	iPi,
	t_sample	iDelta)
	
	{
	double	prevSource1,
			prevSource2,
			prevTarget1,
			prevTarget2,
			prevMutant1,
			prevMutant2,
			curSource1,
			curSource2,
			curTarget1,
			curTarget2,
			curMutant1,
			curMutant2;
	
	tEmphasisFunction
				emphFunc	=  (iDelta == 0.0)
								? EmphasizeZero 
								: (iDelta > 0.0) ? EmphasizePos : EmphasizeNeg;
	double		effectDelta = EffectiveDeltaValue(iDelta);
	tMutate1Func mutateFunc	= me->frameParams.function.irregularFunc;
	
	if (iOmega <= 0.0) do {
		GetHistory(	me,
					&prevSource1, &prevSource2,
					&prevTarget1, &prevTarget2,
					&prevMutant1, &prevMutant2);
		
		curSource1 = *iSource1++,
		curSource2 = *iSource2++;
		
		curMutant1 = emphFunc(prevMutant1, curSource1 - prevSource1, effectDelta);
		curMutant2 = emphFunc(prevMutant2, curSource2 - prevSource2, effectDelta);
		
		WrapUpIrregMutation(me,  curSource1, curSource2, *iTarget1++, *iTarget2++,
							curMutant1, curMutant2, stateSource);
		
		*oMutant1++ = curMutant1;
		*oMutant2++ = curMutant2;
		} while (--iVectorSize > 0);
	
	else if (1.0 <= iOmega) do {
		GetHistory(	me,
					&prevSource1, &prevSource2,
					&prevTarget1, &prevTarget2,
					&prevMutant1, &prevMutant2);
		
		curSource1 = *iSource1++,
		curSource2 = *iSource2++,
		curTarget1 = *iTarget1++,
		curTarget2 = *iTarget2++;
		
		curMutant1 = emphFunc(
						prevMutant1,
						mutateFunc(curSource1 - prevSource1, curTarget1 - prevTarget1),
						effectDelta);
		curMutant2 = emphFunc(
						prevMutant2,
						mutateFunc(curSource2 - prevSource2, curTarget2 - prevTarget2),
						effectDelta);
		
		WrapUpIrregMutation(me,  curSource1, curSource2, curTarget1, curTarget2,
							curMutant1, curMutant2, stateTarget);
		
		*oMutant1++ = curMutant1;
		*oMutant2++ = curMutant2;
		} while (--iVectorSize > 0);
	
	else {
		// iOmega in (0.0 .. 1.0)
		unsigned long	s2mThresh,
						m2mThresh,
						i2mThresh	= CalcMutantThreshholds(iOmega, iPi, &s2mThresh, &m2mThresh);
			
		do	{
			Byte			curState = GetState(me);
			unsigned long	curThresh;
			
			GetHistory(	me,
						&prevSource1, &prevSource2, &prevTarget1, &prevTarget2,
						&prevMutant1, &prevMutant2);
			
			curSource1	= *iSource1++,
			curSource2	= *iSource2++,
			curTarget1	= *iTarget1++,
			curTarget2	= *iTarget2++;
			
			if (curState & stateFollowLeadBit) {
				if (curState == stateFollowLeadBit + stateTarget) {
					curMutant1 = mutateFunc(curSource1 - prevSource1,
											curTarget1 - prevTarget1);
					curMutant2 = mutateFunc(curSource2 - prevSource2,
											curTarget2 - prevTarget2);
					}
				else {
					curMutant1 = curSource2 - prevSource2;
					curMutant2 = curSource2 - prevSource2;
					}
				}
			
			else {
				switch (curState) {
					case stateSource:
						curThresh = s2mThresh;
						break;
					case stateTarget:
						curThresh = m2mThresh;
						break;
					default:
						curThresh = i2mThresh;
						break;
					}
			
				if ( WannaMutateStrict(curThresh) ) {
					curState = stateTarget;
					curMutant1 = mutateFunc(curSource1 - prevSource1,
											curTarget1 - prevTarget1);
					curMutant2 = mutateFunc(curSource2 - prevSource2,
											curTarget2 - prevTarget2);
					}
				else {
					curState = stateSource;
					curMutant1 = curSource2 - prevSource2;
					curMutant2 = curSource2 - prevSource2;
					}
				}
				
			curMutant1 = emphFunc(prevMutant1, curMutant1, effectDelta);
			curMutant2 = emphFunc(prevMutant2, curMutant2, effectDelta);
			
			WrapUpIrregMutation(me,  curSource1, curSource2, curTarget1, curTarget2,
								curMutant1, curMutant2, curState);
		
			*oMutant1++ = curMutant1;
			*oMutant2++ = curMutant2;
			
			} while (--iVectorSize > 0);
		
		}
	
	}
		

/******************************************************************************************
 *
 *	CalcRelUniform(me, iVectorSize, iSource, iTarget, oMutant, iOmega, iDelta)
 *
 ******************************************************************************************/

void
CalcRelUniform(
	tMutator*	me,
	long		iVectorSize,
	t_sample*	iSource1,
	t_sample*	iSource2,
	t_sample*	iTarget1,
	t_sample*	iTarget2,
	t_sample*	oMutant1,
	t_sample*	oMutant2,
	t_sample	iOmega,
	t_sample	iDelta)
	
	{
	double	prevSource1,
			prevSource2,
			prevTarget1,
			prevTarget2,
			prevMutant1,
			prevMutant2,
			curSource1,
			curSource2,
			curTarget1,
			curTarget2,
			curMutant1,
			curMutant2;
	
	tEmphasisFunction
				emphFunc	=  (iDelta == 0.0)
									? EmphasizeZero 
									: (iDelta > 0.0) ? EmphasizePos : EmphasizeNeg;
	double		effectDelta = EffectiveDeltaValue(iDelta);
	
	if (iOmega == 0.0) do {
		GetHistory(	me,
					&prevSource1, &prevSource2,
					&prevTarget1, &prevTarget2,
					&prevMutant1, &prevMutant2);
		
		curSource1 = *iSource1++;
		curSource2 = *iSource2++;	
		
		curMutant1 = emphFunc(prevMutant1, curSource1 - prevSource1, effectDelta);
		curMutant2 = emphFunc(prevMutant2, curSource2 - prevSource2, effectDelta);
		
		WrapUpRegMutation(	me,  curSource1, curSource2, *iTarget1++, *iTarget2++,
							curMutant1, curMutant2);
		
		*oMutant1++ = curMutant1;
		*oMutant2++ = curMutant2;
		} while (--iVectorSize > 0);
	
	else {
		tMutateFunc mutateFunc	= me->frameParams.function.uniformFunc;
		 do {
			GetHistory(	me,
						&prevSource1, &prevSource2,
						&prevTarget1, &prevTarget2,
						&prevMutant1, &prevMutant2);
			
			curSource1 = *iSource1++;
			curSource2 = *iSource2++;
			curTarget1 = *iTarget1++;
			curTarget2 = *iTarget2++;	
			
			curMutant1 = emphFunc(
							prevMutant1,
							mutateFunc(curSource1 - prevSource1, curTarget1 - prevTarget2, iOmega),
							effectDelta);
			curMutant2 = emphFunc(
							prevMutant2,
							mutateFunc(curSource2 - prevSource2, curTarget2 - prevTarget2, iOmega),
							effectDelta);
			
			WrapUpRegMutation(	me,  curSource1, curSource2, curTarget1, curTarget2,
								curMutant1, curMutant2);
		
			*oMutant1++ = curMutant1;
			*oMutant2++ = curMutant2;
			} while (--iVectorSize > 0);
		}
	
	}
