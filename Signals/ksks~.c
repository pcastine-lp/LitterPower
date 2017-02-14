/*
	File:		ksks~.c

	Contains:	Max/MSP external object creating "plucked noise".

	Written by:	Peter Castine

	Copyright:	© 2004 Peter Castine

	Change History (most recent first):

         <2>   26Ð4Ð2006    pc      Update for new LitterLib organization.
         <1>     5Ð11Ð04    pc      first checked in.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark ¥ Include Files

#include "LitterLib.h"	// Also #includes MaxUtils.h, ext.h
#include "TrialPeriodUtils.h"
#include "Taus88.h"


#pragma mark ¥ Constants

const char*	kClassName		= "lp.ksks~";		// Class name

#ifdef __GNUC__
	// Lame
	#define		kBufSize	512
#else
	const int	kBufSize	= 512;				// About the smallest size that still
												// produces decent low-frequency pitches
												// Also, we have high-quality decay time
												// estimates for this value
#endif

	// Indices for STR# resource
enum {
	strIndexInTrig		= lpStrIndexLastStandard + 1,
	strIndexInFreq,
	strIndexInDecay,
	strIndexInBlend,
	
	strIndexOutPluck,
	
	strIndexInLeft		= strIndexInTrig,
	strIndexOutLeft		= strIndexOutPluck
	};

	// Proxy/Inlet IDs
enum {
	inletTrigger		= 0,
	inletFreq,
	inletDecay,
	inletBlend,
	
	outletPluck,
	
	kInletCount			= 4
	};


enum eFlags {
	flagStochastic		= 1,
	
	flagsNone			= 0
	};


#pragma mark ¥ Type Definitions



#pragma mark ¥ Object Structure

typedef struct {
	t_pxobject	coreObject;
	
	Boolean		trigger;
	
	long		phiStep,	// f(freq, sample rate)	
				curPhi,		// Index to next sample in the playback buffer
				flags;		// Sum of bits defined in enum eFlags
							// (currently only one flag defined)
							
	unsigned long tau,		// Threshold value used for positive decay-modification exponents
				  blendTau;
	
	double		freq,		// Nominal frequency
				decay,		// Decay-modification exponent
				rho,		// Decay-stretch/compression factor derived from above
				blend,		// Blend factor
				sr,			// Current sample rate
				phiFrac,	// fraction of "real" phi (integer portion stored in phiStep)	
				cumPhiErr;	
				
	t_sample	buf[kBufSize+1];	// Playback buffer. It's convenient to duplicate first
	} objPluck;						// element at end of buffer.


#pragma mark ¥ Global Variables



#pragma mark ¥ Function Prototypes

	// Class message functions
void*	PluckNew(double, double, double);

	// Object message functions
static void PluckTrigger(objPluck*);
static void PluckInt(objPluck*, long);
static void PluckFloat(objPluck*, double);
static void PluckReset(objPluck*);
static void PluckTattle(objPluck*);
static void	PluckAssist(objPluck*, void* , long , long , char*);
static void	PluckInfo(objPluck*);

	// MSP Messages
static void	PluckDSP(objPluck*, t_signal**, short*);
static int*	PluckPerform(int*);


#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark ¥ Inline Functions



#pragma mark -

/******************************************************************************************
 *
 *	main()
 *	
 *	Standard Max/MSP External Object Entry Point Function
 *	
 ******************************************************************************************/

void
main(void)
	
	{
	LITTER_CHECKTIMEOUT(kClassName);
	
	// Standard Max/MSP initialization mantra
	setup(	&gObjectClass,				// Pointer to our class definition
			(method) PluckNew,		// Instance creation function
			(method) dsp_free,			// Default deallocation function
			sizeof(objPluck),			// Class object size
			NIL,						// No menu function
										// Optional arguments:
			A_DEFFLOAT,					//	1) Initial Frequency [Default: 440 Hz]
			A_DEFFLOAT,					//	2) Initial Decay Time [100.0 ms]
			A_DEFFLOAT,					//	3) Initial Blend Factor [1.0]
			0);		
	
	dsp_initclass();

	// Messages
	LITTER_TIMEBOMB addbang	((method) PluckTrigger);
	LITTER_TIMEBOMB addint	((method) PluckInt); // Because Max won't typecast for us :-(
	LITTER_TIMEBOMB addfloat((method) PluckFloat);
	addmess	((method) PluckTattle,	"dblclick",	A_CANT, 0);
	addmess	((method) PluckTattle,	"tattle",	A_NOTHING);
	addmess	((method) PluckAssist,	"assist",	A_CANT, 0);
	addmess	((method) PluckInfo,	"info",		A_CANT, 0);
	
	// MSP-Level messages
	addmess	((method) PluckDSP,		"dsp",		A_CANT, 0);

	//Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();
	
	}

#pragma mark -
#pragma mark ¥ Utility functions

/******************************************************************************************
 *
 *	ClearBuf(me)
 *	GenBuf(me)
 *	
 ******************************************************************************************/

static void
ClearBuf(
	objPluck* me)
	
	{
	t_sample*	curSamp = me->buf;
	long		i		= kBufSize;
	
	while (i-- >= 0)
		*curSamp++ = 0.0;
	
	}
	
	
static void
GenBuf(
	objPluck* me)
	
	{
	
	Taus88SigVector(me->buf, kBufSize);
	me->buf[kBufSize] = me->buf[0];
	
	me->trigger = false;
	
	}

/******************************************************************************************
 *
 *	SetFreq(me, iFreq)
 *	SetDecay(me, iDecay)
 *	SetBlend(me, iBlend)
 *	
 ******************************************************************************************/

static void
SetFreq(
	objPluck*	me,
	double		iFreq)
	
	{
	const double kChiFactor = 0.25 / (double) kBufSize;
	
	double	phi,
			intPhi;
	
	if (iFreq < 0.0)
		iFreq = - iFreq;

	phi		= (double) kBufSize * iFreq / me->sr;			// Requires valid sample rate
	intPhi	= floor(phi);

	me->freq	= iFreq;
	me->phiFrac	= phi - intPhi;								// 0 <= me->phiFrac < 1
	me->phiStep	= (long) phi % kBufSize;					// 0 <= me->phiStep < kBufSize
	
	}

static void
SetDecay(
	objPluck*	me,
	double		iDecay)
	
	{
	const double kMagicFactor = 0.015625;					// 1/64
	
	me->decay	= iDecay;
	me->tau		= kULongMax;
	
	iDecay *= kMagicFactor;
	if (iDecay >= 0.0)
		 iDecay += iDecay;
	else iDecay *= 0.5;
	
	me->rho = pow(2.0, iDecay);
	
	if (iDecay > 0.0)
		me->tau /= me->rho;
	
	}

static void
SetBlend(
	objPluck*	me,
	double		iBlend)
	
	{
	const double	kMinBlend = -1.0,
					kMaxBlend =  1.0;
	
	Boolean	inputError = false;
	
	if (iBlend < kMinBlend) {
		iBlend = kMinBlend;
		inputError = true;
		}
	else if (iBlend > kMaxBlend) {
		iBlend = kMaxBlend;
		inputError = true;
		}
	if (inputError)
		error("%s: blend must be between -1 and 1", kClassName);
	
	me->blend = iBlend;
	me->blendTau = ((double) kULongMax) * iBlend;
	
	}

#pragma mark -
#pragma mark ¥ Class Message Handlers

/******************************************************************************************
 *
 *	PluckNew(iNN)
 *	
 ******************************************************************************************/

void*
PluckNew(
	double	iFreq,
	double	iDecay,
	double	iBlend)
	
	{
	const double	kDefFreq	= 440.0,
				//	kDefDecay	=   0.0,
					kDefBlend	=   0.0;
	
	objPluck*		me	= NIL;
	
	// Let Max/MSP allocate us, our inlets, and outlets.
	me = (objPluck*) newobject(gObjectClass);
	dsp_setup(&(me->coreObject), kInletCount);

	outlet_new(me, "signal");
	
	// Set up object to default state
	me->trigger		= false;
	me->freq		= 0.0;
	me->sr			= sys_getsr();			// Provisional, recheck at DSP time
	me->cumPhiErr	= 0.0;
	me->phiStep		= 0;
	me->curPhi		= 0;
	me->flags		= flagsNone;
	ClearBuf(me);							// Generate a zeroed buffer to start off with


	// Finally, set up initialization values
	SetFreq(me, iFreq == 0.0 ? kDefFreq : iFreq);
									// Sets freq, phiStep, phiFrac
	SetDecay(me, iDecay);			// Sets decay, rho, tau
	SetBlend(me, iBlend);			// Checks for valid input, sets blend & blendTau
	
	return me;
	}

#pragma mark -
#pragma mark ¥ Object Message Handlers



/******************************************************************************************
 *
 *	PluckTrigger(me)
 *	PluckFloat(me, iVal)
 *
 ******************************************************************************************/
 
void PluckTrigger(objPluck* me)
	{ me->trigger = true; }

	// Have to include this because Max is too braindead to typecast for us
void PluckInt(objPluck* me, long iVal)
	{ PluckFloat(me, (double) iVal); }

void
PluckFloat(
	objPluck*	me,
	double		iVal)
	
	{
	
	switch ( ObjectGetInlet((Object*) me, me->coreObject.z_in) ) {
		case inletFreq:		SetFreq(me, iVal);					break;
		case inletDecay:	SetDecay(me, iVal);					break;
		case inletBlend:	SetBlend(me, iVal);					break;
		default:			if (iVal != 0.0) PluckTrigger(me);	break;
		}
	
	}
	
 
/******************************************************************************************
 *
 *	PluckReset(me)
 *
 *	Clear cur buffer, generate new goal and interp buffers, reset related counters
 *
 ******************************************************************************************/
 
void
PluckReset(
	objPluck*	me)
	
	{
	
	// Generate new buffer
	ClearBuf(me);
	
	}
 

/******************************************************************************************
 *
 *	PluckTattle(me)
 *
 *	Post state information
 *
 ******************************************************************************************/

void
PluckTattle(
	objPluck* me)
	
	{
	
	post("%s state", kClassName);
	post("  frequency is: %lf (phi = %lf at sample rate %lf)",
			me->freq, (double) me->phiStep + me->phiFrac, me->sr);
	post("	Current phi is at sample %ld, cumulative error %lf",
			me->curPhi, me->cumPhiErr);
	post("  Decay exponent is %lf (rho = %lf)",
			me->decay, me->rho);
	if (me->rho > 1.0)
		post("    tau (smoothing threshhold) is Taus88() <= %lu", me->tau);
	post("  Blend factor is %lf, with threshhold Taus88() < %lu", me->blend, me->blendTau);
	
	}


/******************************************************************************************
 *
 *	PluckAssist()
 *	PluckInfo()
 *
 *	Fairly generic Assist/Info methods
 *
 ******************************************************************************************/

void PluckAssist(objPluck* me, void* box, long iDir, long iArgNum, char* oCStr)
	{
	#pragma unused(me, box)
	
	LitterAssist(iDir, iArgNum, strIndexInLeft, strIndexOutLeft, oCStr);
	}

void PluckInfo(objPluck* me)
	{ LitterInfo(kClassName, &me->coreObject.z_ob, (method) PluckTattle); }



#pragma mark -
#pragma mark ¥ DSP Methods

/******************************************************************************************
 *
 *	PluckDSP(me, ioDSPVectors, iConnectCounts)
 *
 ******************************************************************************************/

void
PluckDSP(
	objPluck*	me,
	t_signal*	ioDSPVectors[],
	short		connectCounts[])
	
	{
	#pragma unused(connectCounts)
	
	double sr = ioDSPVectors[0]->s_sr;
	
	if (me->sr != sr) {
		me->sr = sr;
		me->phiStep	= kBufSize * (me->freq / sr);			// Recalculate phiStep
		}
	
	dsp_add(
		PluckPerform, 7,
		me, (long) ioDSPVectors[outletPluck]->s_n,
		ioDSPVectors[inletTrigger]->s_vec,
		ioDSPVectors[inletFreq]->s_vec,
		ioDSPVectors[inletDecay]->s_vec,
		ioDSPVectors[inletBlend]->s_vec,
		ioDSPVectors[outletPluck]->s_vec
		);
	
	}
	

/******************************************************************************************
 *
 *	PluckPerform(iParams)
 *
 *	Parameter block for PerformSync contains 8 values:
 *		- Address of this function
 *		- The performing pluck~ object
 *		- Vector size
 *		- Trigger signal
 *		- Frequency [Hz]
 *		- Decay time [ms]
 *		- Blend factor [-1 .. 1]
 *		- Output signal
 *
 *	Note that the PluckPerform() uses a truncating wavetable lookup. Not only is this more
 *	efficient than interpolating lookup, it sounds better at lower frequencies.
 *
 ******************************************************************************************/

	static inline void SmoothBuf(tSampleVector ioBuf, long iStart, long iEnd, double iRho)
		{
		long	span = iEnd - iStart;		// Actually, always one less than we really mean
		float	mean = (ioBuf[iStart] + ioBuf[iEnd]) * iRho;
			
		if (span < 0) {
			// The samples to process wrap around from the end of the wavetable back to
			// the beginning.
			// First handle the samples at the end of the buffer.
			span = kBufSize - iStart;
			do { ioBuf[iStart++] = mean; } while (--span >= 0);
			
			// Now set up parameters to handle the samples at the beginning of the buffer
			span = iEnd;
			}
		else ioBuf += iStart;
		
		do { *ioBuf++ = mean; } while (--span >= 0);
		
		}
	
	static inline long IncrPhi(long iPhi, long iIncr, double* ioCumErr, double iFrac)
		{
		double newErr = *ioCumErr + iFrac;
		
		iPhi += iIncr;
		
		if (newErr >= 1.0) {
			newErr	-= 1.0;
			iPhi	+= 1;
			}
		if (iPhi >= kBufSize)
			iPhi -= kBufSize;
			
		*ioCumErr = newErr;
		return iPhi;
		}
	
int*
PluckPerform(
	int* iParams)
	
	{
	enum {
		paramFuncAddress	= 0,
		paramMe,
		paramVectorSize,
		paramTrigger,
		paramFreq,
		paramDecay,
		paramBlend,
		paramOut,
		
		paramNextLink
		};
	
	const double kMaxRho = 0.499999;					// Just under 0.5 to reduce DC
	
	long			n;									// Count samples in sample vector
	tSampleVector	trig,								// Signal vectors
					freq,
					decay,
					blend,
					out;
	double			phiFrac,
					cumPhiErr,
					rho;
	long			phiStep,
					curPhi,
					prevPhi;
	unsigned long	blendTau;
	tSampleVector	buf;
	
	objPluck*		me = (objPluck*) iParams[paramMe];
	
	if (me->coreObject.z_disabled) goto punt;			// Nothing to do, we're out of here
	
	if (me->trigger)
		GenBuf(me);
	
	// Copy parameters into registers
	n			= (long) iParams[paramVectorSize];
	trig		= (tSampleVector) iParams[paramTrigger];
	freq		= (tSampleVector) iParams[paramFreq];
	decay		= (tSampleVector) iParams[paramDecay];
	blend		= (tSampleVector) iParams[paramBlend];
	out			= (tSampleVector) iParams[paramOut];
	// Copy object members into registers
	phiFrac		= me->phiFrac;
	cumPhiErr	= me->cumPhiErr;
	rho			= me->rho;
	phiStep		= me->phiStep;
	curPhi		= me->curPhi;
	buf			= me->buf;
	blendTau	= me->blendTau;
	
	if (rho <= 1.0) {
		// Modified Karplus-Strong w/Decay Compression
		rho *= kMaxRho;
		
		if (blendTau > 0) do {
			// Do magic blend sign switch?
			if (Taus88(NIL) < blendTau)
				buf[curPhi] *= -1.0;
			
			// Calculate output value
			*out++ = buf[curPhi];
			
			// Calculate phi for next sample
			prevPhi = curPhi;
			curPhi = IncrPhi(curPhi, phiStep, &cumPhiErr, phiFrac);
			
			// Tick low-pass filter
			if (curPhi != prevPhi)
				SmoothBuf(buf, prevPhi, curPhi, rho);
			} while (--n > 0);
		
		else do {												// blendTau == 0
			// Calculate output value
			*out++ = buf[curPhi];
			
			// Calculate phi for next sample
			prevPhi = curPhi;
			curPhi = IncrPhi(curPhi, phiStep, &cumPhiErr, phiFrac);
			
			// Tick low-pass filter
			if (curPhi != prevPhi)
				SmoothBuf(buf, prevPhi, curPhi, rho);
			} while (--n > 0);
		}
	
	else {														// rho > 1.0
		// Karplus-Strong w/Decay Stretching
		unsigned long tau = me->tau;
		
		if (blendTau > 0) do {
			// Do magic blend sign switch?
			if (Taus88(NIL) < blendTau)
				buf[curPhi] *= -1.0;
			
			// Calculate output value
			*out++ = buf[curPhi];
			
			// Calculate phi for next sample
			prevPhi = curPhi;
			curPhi = IncrPhi(curPhi, phiStep, &cumPhiErr, phiFrac);
			
			// Tick stochastically-delayed low-pass filter
			if (curPhi != prevPhi && Taus88(NIL) <= tau)
				SmoothBuf(buf, prevPhi, curPhi, kMaxRho);
			
			} while (--n > 0);
		
		else do {												// blendTau == 0
			// Calculate output value
			*out++ = buf[curPhi];
			
			// Calculate phi for next sample
			prevPhi = curPhi;
			curPhi = IncrPhi(curPhi, phiStep, &cumPhiErr, phiFrac);
			
			// Tick stochastically-delayed low-pass filter
			if (curPhi != prevPhi && Taus88(NIL) <= tau)
				SmoothBuf(buf, prevPhi, curPhi, kMaxRho);
			
			} while (--n > 0);
		}
		
	// Update member data
	me->cumPhiErr	= cumPhiErr;
	me->curPhi		= curPhi;
		
punt:
	return iParams + paramNextLink;
	}
