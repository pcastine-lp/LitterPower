/*
	File:		noisify~.c

	Contains:	Vocoder-based object to modulate an audio signal to noise.

	Written by:	Peter Castine

	Copyright:	© 2004 Peter Castine

	Change History (most recent first):

         <8>   26–4–2006    pc      Update for new LitterLib organization.
         <7>   23–3–2006    pc      Modify filter bank to use Bark freqeuncy bands
         <6>     9–12–04    pc      #include ext_critical.h for Carbon and Windows
         <5>     18–3–04    pc      Fix problem with output fading to zero for max omega. Have also
                                    tweaked mapping of omega to filterbank Q and made tracking of
                                    mean amp more robust (costs CPU, alas).
         <4>     11–3–04    pc      Inverted nesting of loops in perform method (for-each-filter as
                                    outer loop, for-each-sample as inner loop). Also inlined Taus88
                                    code. Improved performance, but sound output fades to zero when
                                    omega is close to one. Not good.
         <3>      9–3–04    pc      Continue work... memory management turns out to be ok, finite
                                    precision is the problem. Now getting CPU utilization in the
                                    range 28.6 - 30.7% (mean 29.7%). Inner loop is running through
                                    filters in the bank.
         <2>      6–3–04    pc      First implementation. There are still problems with memory
                                    allocation and maintaining running averages for the source
                                    energy.
         <1>     28–2–04    pc      first checked in.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Profiler Switch

	// Turn profiler on or off
#define __PROFILER_ONFLAG__ 0


#pragma mark -
#pragma mark • Include Files

#include "LitterLib.h"			// Also #includes MaxUtils.h, ext.h
#include "TrialPeriodUtils.h"
#include "Taus88.h"

#include <float.h>				// For FLT_EPSILON


#if defined(MAC_VERSION) || defined(WIN_VERSION)
	#define __HAS_CRITICAL_REGIONS__ 1
#else
	#define __HAS_CRITICAL_REGIONS__ 0
#endif

#if __HAS_CRITICAL_REGIONS__
	#include "ext_critical.h"	// For critical_enter() and critical_exit()
								// Should this be in MaxUtils.h or something?
#endif


#if __PROFILER_ONFLAG__
	#include <profiler.h>
#endif



#pragma mark • Constants

const char	kClassName[]	= "lp.emeric~";					// Class name

#ifdef __MWERKS__
	const int	kFBankSize		= 24;
#else
	// This is for the benefit of lame GCC
	#define kFBankSize				24
#endif

const float kFBankCenters[]	= {	50.,	150.,	250.,
								350.,	450.,	570.,
								700.,	840.,	1000.,
								1170.,	1370.,	1600.,
								1850.,	2150.,	2500.,
								2900.,	3400.,	4000.,
								4800.,	5800.,	7000.,
								8500.,	10500,	13500
								};

	// Indices for STR# resource
enum {
	strIndexInSrc		= lpStrIndexLastStandard + 1,
	strIndexInTgt,
	strIndexInIndex,
	
	strIndexOutSig,
	
	strIndexInLeft		= strIndexInSrc,
	strIndexOutLeft		= strIndexOutSig
	};

#pragma mark • Type Definitions

typedef struct resonParams {
			double	a0, b1, b2;			// Following Dodge & Jerse
			} tResonParams;

typedef struct resonBuf	{
			double y1, y2;				// Ditto: D&J var names
			} tResonBuf;


#pragma mark • Object Structure

typedef struct {
	t_pxobject		coreObject;
	
	float			omega;				// In range 0 <= omega <= 1
	unsigned long	noiseThresh;		// Depends on omega; threshold used to decide
										// whether to use source or target sample as
										// excitation source
										
	double			srcXFade,			// Amount of "dry" source to mix into output
					vocXFade;			// Amount of "wet" vocoder output to mix into output
										// These values also depend on omega;
					
	float			curSR;
	
	tResonParams	rParams[kFBankSize];	// These also depend on omega as well as the
	tResonBuf		srcRBuf[kFBankSize],	// hard-wired center frequencies and sample
					excRBuf[kFBankSize];	// rate
	
	tSampleVector	exciteBuf;				// Need a private buffer of samples for the
	unsigned long	exciteBufSize;			// excitation buffer
	
	double			ampSums[kFBankSize];	// Cache sum of (absolute) amplitudes of the
											// ring buffer below
	int				sumToUpdate;			// Update one running sum per sample vector
	tSampleVector	ringBuf,				// Allocate at dsp_add time; size depends on SR
					ringBufCurPos;			// Points to vector of 21 amps inside the ring buffer
	unsigned long	ringBufMaxSize,			// Size in # of floats stored
					ringBufCurSize;
	} objEmeric;


#pragma mark • Global Variables

/*	// Private copy of an initialized Taus88 data structure
	// This is hardwired into emeric for speed.
	// The static initialization below should be overwritten by the calling functions,
	// but I'm wary of leaving this stuff initialized to 0s.
static tTaus88Data
				sTausData		= {0x4a1fcf79, 0xb86271cc, 0x6c986d11};
*/

#pragma mark • Function Prototypes

	// Class message functions
void*	EmericNew(double);
void	EmericFree(objEmeric*);

	// Object message functions
static void EmericOmega(objEmeric*, double);
static void EmericFloat(objEmeric*, double);
static void EmericTattle(objEmeric*);
static void	EmericAssist(objEmeric*, void*, long, long, char*);
static void	EmericInfo(objEmeric*);


	// MSP Messages
static void	EmericDSP(objEmeric*, t_signal**, short*);
static int*	EmericPerform6(int*);
static int*	EmericPerform5(int*);


#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

/******************************************************************************************
 *
 *	main()
 *	
 *	Standard Max/MSP External Object Entry Point Function
 *	
 ******************************************************************************************/
#if __PROFILER_ONFLAG__
	static Boolean	sProfilerOK = false,
					sProfilerOn = false;
#endif

void
main(void)
	
	{
	LITTER_CHECKTIMEOUT(kClassName);
	
#if __PROFILER_ONFLAG__
	if (ProfilerInit(collectDetailed, bestTimeBase, 1000, 20) == noErr) {
		sProfilerOK = true;
		post("profiler initialized successfully");
		sProfilerOn = true;
		}
	else post("couldn't initialize profiler");
#endif

	// Standard Max/MSP initialization mantra
	setup(	&gObjectClass,				// Pointer to our class definition
			(method) EmericNew,			// Instance creation function
			(method) EmericFree,			// Default deallocation function
			sizeof(objEmeric),			// Class object size
			NIL,						// No menu function
			A_DEFFLOAT,					// Optional arguments:	1. Initial omega
			0);	
	
	dsp_initclass();
	
	// Messages
	addfloat((method) EmericFloat);
	addmess	((method) EmericTattle,	"dblclick",	A_CANT, 0);
	addmess	((method) EmericTattle,	"tattle",	A_NOTHING);
	addmess	((method) EmericAssist,	"assist",	A_CANT, 0);
	addmess	((method) EmericInfo,	"info",		A_CANT, 0);
	
	// MSP-Level messages
	LITTER_TIMEBOMB addmess	((method) EmericDSP, "dsp", A_CANT, 0);

	// Initialize Litter Library
	LitterInit(kClassName, 0);
	
/*	// Instead of calling Taus88Init(), as we usually do, we're calling Taus88Seed() directly.
	// This allows us to hardwire the Taus88 algorithm into code used in the Perform method, for
	// better performance. We will regret this if we ever have to make changes to the code.
	Taus88Seed(&sTausData, 0);
*/
	Taus88Init();
	
	}

#pragma mark -
#pragma mark • Utility Functions

/******************************************************************************************
 *
 *	Omega2Q()
 *	Omega2BufSize()
 *	
 ******************************************************************************************/

static double
Omega2Q(
	double iOmega)
	
	{
	const double	kMaxQ		= 18.0,
					kMinQ		= 2.0 / 3.0,
					kMaxPoint	= 0.1;			// Max Q is when omega reaches this point
	
	double q = (iOmega <= kMaxPoint)
					? kMaxQ * (iOmega / kMaxPoint)
					: kMaxQ * (1.0 - iOmega) / (1.0 - kMaxPoint);
	
	return q + kMinQ;
	}

static unsigned long
Omega2BufSize(
	double iOmega,
	double iSR)
	
	{
	const double	kMaxBufDur = 0.1,			// Buffer needs to store up to 100 ms.
					kMinBufDur = 0.01;			// Minimum buffer is 10 ms
	
	double			bufDur = kMinBufDur + iOmega * (kMaxBufDur - kMinBufDur);
	
	return kFBankSize * (unsigned long) ceil(bufDur * iSR);
	}


/******************************************************************************************
 *
 *	CalcRunningSum(me, iFiltNum)
 *	
 ******************************************************************************************/

static inline void
CalcRunningSum(
	objEmeric*	me,
	int			iFiltNum)
	
	{
	double			sum	= 0.0;
	tSampleVector	s	= me->ringBuf + iFiltNum;		// Current sample
	int				i	= me->ringBufCurSize;			// Size of buffer
	
	do	{
		sum += *s;
		
		s += kFBankSize;
		i -= kFBankSize;
		} while (i > 0);
	
	me->ampSums[iFiltNum] = sum;
	}


/******************************************************************************************
 *
 *	CalcFiltParams()
 *	
 ******************************************************************************************/

	// ??
	// ??	The gain calculation should probably be adjusted when Q is very small. In these 
	//		cases, with bandwidth greater than Nyquist (and sometimes even greater than the 
	//		sampling rate!), we should probably adjust gain to take into account that energy 
	//		from neighboring filters is overlapping and the sum output from the filterbank
	//		is far greater than the input amplitude, often causing extreme clipping artefacts.
	//		OTOH, Kasper likes the clipping.
	// ??
	// ??
static void
CalcFiltParams(
	double			q,
	double			sr,
	tResonParams	oRParams[])
	
	{
	// The filter bank sums the output of all 21 filters, so reduce amplitude such that
	// the sum will have the same power as the input signal
	const double kFBankAmpFactor	= 0.2182178902;				// = sqrt(1/kFBankSize)

	int i;
	
	for (i = 0; i < kFBankSize; i += 1) {
		double	f0 = kFBankCenters[i],
				a0 = kFBankAmpFactor,
				b1 = 0.0,
				b2 = 0.0;
		
		// Cf. Dodge & Jerse or other std. text on two-pole IIR filters
		// We skip the grunt arithmetic when the bandwidth is greater than the current
		// sample rate and just let the coefficients degenerate to the null filter.
		// This should also avoid problems with denormalized values.
		// The condition is equivalent to (bw <= sr), with bw = f0/q
		// As a corollary, this is a safe way of assuring that q > 0
		if (f0 <= sr * q) {
			double	b2plus1,
					a0sq;
			
			// Express center frequency as fraction of sample rate
			// ASSERT: sr > 0
			f0 /= sr;
			
			// ASSERT (cf. above): (q > 0)
			b2		= exp(-k2pi * f0 / q);						// b2 > 0, ergo...
			b2plus1	= b2 + 1.0;									// ... b2plus1 > 1 > 0
			b1		= (-4.0 * b2) / b2plus1 * cos(k2pi * f0);	// ergo division is safe
			
			a0sq  = b2plus1 * b2plus1;
			a0sq -= b1 * b1;
			a0sq *= (1.0 - b2);
			a0sq /= b2plus1;
			a0	 *= sqrt(a0sq);
			}
		
		oRParams[i].a0 = a0;
		oRParams[i].b1 = b1;
		oRParams[i].b2 = b2;
		}
	
	}

/******************************************************************************************
 *
 *	InitVocBuffers(me)
 *	SetVocBufSize(me, iNewSize)
 *	
 ******************************************************************************************/

static void
InitVocBuffers(
	objEmeric*	me)
	
	{
	int i;
	
	for (i = 0; i < kFBankSize; i += 1) {
		me->ampSums[i]		= 0.0;
		me->srcRBuf[i].y1	= 0.0;
		me->srcRBuf[i].y2	= 0.0;
		me->excRBuf[i].y1	= 0.0;
		me->excRBuf[i].y2	= 0.0;
		}
	
	me->ringBuf			= NIL;
	me->ringBufCurPos	= NIL;
	me->ringBufMaxSize	= 0;
	me->ringBufCurSize	= 0;
	me->sumToUpdate		= 0;
	}

static void
SetVocBufSize(
	objEmeric*		me,
	unsigned long	iNewSize)
	
	{
	// Pre-conditions:	me->ringBuf != NIL
	//					(ie, it exists and everything's initialized)
	//					iNewSize <= me->ringBufMaxSize
	//					(ie, if sample rate has changed, the larger buffer has
	//					already been allocated)
	
	// Copy base address of vectors from the object into registers
	double* const	kAmpSumBase = me->ampSums;
	floatPtr const	kRingBufBase = me->ringBuf,
					kRingBufStop = kRingBufBase + me->ringBufCurSize;
	const long		kRBCurSize	= me->ringBufCurSize;
	
	unsigned long	i, j, k;						// General-purpose counters
	floatPtr		p;								// General-purpose pointers
	double*			s;
	
#if __HAS_CRITICAL_REGIONS__
	critical_enter(0);
#else
	short prevLock = lockout_set(1);
#endif

	// Two basic alternatives
	if (iNewSize < 	me->ringBufCurSize) {
		// Need to shrink the ring buffer
		i = me->ringBufCurSize - iNewSize;			// How much history to trash
		p = me->ringBufCurPos;						// Current pointer position
		j = kRingBufStop - p;						// How far to the end of the buffer
		
		// Will we have to wrap around (or reach end of buffer)?
		if (j <= i) {
			// Yes. Do from current pos to end of buffer first...
			i -= j;
			do	{
				
				s = kAmpSumBase;
				j -= k = kFBankSize;
				do { *s++ -= *p++; } while (--k > 0);
				
				} while (j > 0);
			
			// ...and wrap around to beginning of buffer
			p = kRingBufBase;
			}
		
		// Finish remaining samples
		while (i > 0) {
			i -= k = kFBankSize;
			s = kAmpSumBase;
			do { *s++ -= *p++; } while (--k > 0);
			}
		
		if (p >= kRingBufStop)
			p = kRingBufBase;
		
		// Finally, move history around to reflect new buffer size
		if (p < me->ringBufCurPos) {
			i = me->ringBufCurPos - p;
			BlockMoveData((Ptr) p, (Ptr) kRingBufBase, i * sizeof(float));
			me->ringBufCurPos = kRingBufBase;
			}
		else {
			i = kRingBufStop - p;
			BlockMoveData((Ptr) p, (Ptr) me->ringBufCurPos, i * sizeof(float));
			}
		
		}
	
	else if (iNewSize > me->ringBufCurSize) {
		// Need to expand the ring buffer
		// This entails filling the new history with the current mean values to ensure a
		// smooth transition.
		const double kMeanFactor = ((double) kFBankSize) / ((double) me->ringBufCurSize);
		const long	 kNewSamples = (iNewSize - me->ringBufCurSize) / kFBankSize;
		
		float	curMeans[kFBankSize];
		
		s = kAmpSumBase;
		p = &curMeans[0];
		i = kFBankSize;
		do	{
			double m = *s * kMeanFactor;			// Get current mean for this filter band
			
			*s++ += m * kNewSamples;				// Adjust running sums-of-amplitudes
			*p++ = (m > FLT_EPSILON) ? m : 0.0;		// Convert mean to single-precision.
			} while (--i > 0);
			
		i = iNewSize - me->ringBufCurSize;
		p = me->ringBufCurPos;
		j = kRingBufStop - p;
		BlockMoveData((Ptr) p, (Ptr) (p + i), j * sizeof(float));
		
		do	{
			BlockMoveData((Ptr) curMeans, (Ptr) p, kFBankSize);
			
			p += kFBankSize;
			i -= kFBankSize;
			} while (i > 0);
		}
	// ... otherwise no change
	
	// Don't forget to update cur size component
	me->ringBufCurSize = iNewSize;

#if __HAS_CRITICAL_REGIONS__
	critical_exit(0);
#else
	lockout_set(prevLock);
#endif
	}

/******************************************************************************************
 *
 *	SetSR(me, iSR)
 *	
 ******************************************************************************************/

static OSErr
SetSR(
	objEmeric* me,
	double iSR)
	
	{
	// This function assumes that iSR is always a valid sample rate
	// (at the very least, a positive value)
	//
	// It also relies on omega having been initialized
	//
	// And, of course, not being called inside an interrupt!
	//
	// Caller should check if this is a real change of sample rate.
	
	const double kMaxOmega = 1.0;				// Value of Omega for which maximum
												// buffer is needed
	
	OSErr			myErr			= noErr;
	unsigned long	ringBufMaxSize	= Omega2BufSize(kMaxOmega, iSR),
					ringBufCurSize	= Omega2BufSize(me->omega, iSR);
	
	me->curSR = iSR;
	
	if (me->ringBufMaxSize > ringBufMaxSize) {
		// Safe as tofu...
		if (ringBufCurSize != me->ringBufCurSize)
			SetVocBufSize(me, ringBufCurSize);
		
		SetPtrSize((Ptr) me->ringBuf, ringBufMaxSize * sizeof(float));
		me->ringBufMaxSize = ringBufMaxSize;
		}
	
	else if (me->ringBufMaxSize > 0) {
		// This means a ring buffer exists, but we need a larger one
		// First try to resize the existing buffer
		SetPtrSize((Ptr) me->ringBuf, ringBufMaxSize * sizeof(float));
		if (MemError() == noErr) {
			// Excellent! The rest is easy
			me->ringBufMaxSize = ringBufMaxSize;
			if (ringBufCurSize != me->ringBufCurSize)
				SetVocBufSize(me, ringBufCurSize);
			}
		
		else {
			// OK, try to allocate a new block and copy data
			floatPtr newBuf = (floatPtr) NewPtrClear(ringBufMaxSize * sizeof(float));
			if (newBuf != NIL) {
				unsigned long curPosOffset = me->ringBufCurPos - me->ringBuf;
				
				BlockMoveData(me->ringBuf, newBuf, me->ringBufCurSize * sizeof(float));
				DisposePtr((Ptr) me->ringBuf);
				
				me->ringBuf			= newBuf;
				me->ringBufCurPos	= newBuf + curPosOffset;
				me->ringBufMaxSize	= ringBufMaxSize;
				if (ringBufCurSize != me->ringBufCurSize)
					SetVocBufSize(me, ringBufCurSize);
				}
			
			else {
				// Memory's tight. Try trashing the old buffer, then allocate anew
				// (below, at outermost level of this function)
				DisposePtr((Ptr) me->ringBuf);
				InitVocBuffers(me);					// Important: sets me->ringBuf to NIL
				}
			}
		}
	
	if (me->ringBuf == NIL) {						// NOTA BENE: *Not* else if...
		floatPtr newBuf = (floatPtr) NewPtrClear(ringBufMaxSize * sizeof(float));
		if (newBuf != NIL) {
			me->ringBuf			= newBuf;
			me->ringBufCurPos	= newBuf;
			me->ringBufCurSize	= ringBufCurSize;
			me->ringBufMaxSize	= ringBufMaxSize;
			}
		else myErr = MemError();
		}
	
	if (myErr == noErr)	
		CalcFiltParams(Omega2Q(me->omega), iSR, me->rParams);
	
	return myErr;
	}


#pragma mark -
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	EmericNew(iOmega)
 *	
 ******************************************************************************************/

void*
EmericNew(
	double	iOmega)
	
	{
	const double kDefOmega = 0;
	
	objEmeric*	me	= NIL;
	
	// Take intialization parameters as they come

	// Let Max/MSP allocate us, our inlets, and outlets.
	me = (objEmeric*) newobject(gObjectClass);
		if (me == NIL) goto punt;
		
	dsp_setup(&(me->coreObject), 3);		// Signal inlets: Source, Target, Omega
											// Omega is updated at most once per sig vector
	outlet_new(me, "signal");
	
	// Customize flags for core object
	me->coreObject.z_misc = Z_NO_INPLACE;
	
	// Initialize members
	me->omega			= kDefOmega;		// Default value for omega/noise thresh
	me->noiseThresh		= 0;
	me->srcXFade		= 0.0;
	me->vocXFade		= 0.0;
	me->exciteBuf		= NIL;
	me->exciteBufSize	= 0;
	
	InitVocBuffers(me);
	
	SetSR(me, sys_getsr());		// This will be recalculated in the DSP method
								// With omega == 0, this call simply clears all rParams
								// Ignore return value at this time.
	
	if (iOmega != kDefOmega)				// Now, finally, try the user argument
		EmericOmega(me, iOmega);

punt:
	return me;
	}

void
EmericFree(
	objEmeric*	me)
	
	{
#if __PROFILER_ONFLAG__
	if (sProfilerOn) {
		ProfilerDump("\pemeric.dump");
		ProfilerTerm();
		sProfilerOn = false;
		post("end of profiling");
		}
#endif
	// Do this first, in case DSP is still running or something
	dsp_free(&(me->coreObject));

	if (me->ringBuf != NIL)
		DisposePtr((Ptr) me->ringBuf);
	if (me->exciteBuf != NIL)
		DisposePtr((Ptr) me->exciteBuf);
	
	}

#pragma mark -
#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	EmericOmega(me, iOmega)
 *	
 *	Set parameters, making sure nothing bad happens.
 *
 ******************************************************************************************/

void EmericOmega(
	objEmeric*	me,
	double		iOmega)
	
	{
	const double	kXFadeCutoff = 0.05;	// Point where we no longer cross-fade between
											// source and vocoder output
	
	if (iOmega < 0.0 || 1.0 < iOmega) {
		error("%s: invalid omega %ld", kClassName, iOmega);
		return;
		}
	
	me->omega		= iOmega;
	me->noiseThresh = ((double) kULongMax) * iOmega * iOmega + 0.5;
	
	if (iOmega < kXFadeCutoff) {
		const double kOmega2Phi = 31.4159265358979323846;		// (pi/2) / kXFadeCutoff)
		
		double phi = kOmega2Phi * iOmega;
		
		me->srcXFade = cos(phi);
		me->vocXFade = sin(phi);
		}
	else {
		me->srcXFade = 0.0;
		me->vocXFade = 0.0;
		}
	
	CalcFiltParams(Omega2Q(iOmega), me->curSR, me->rParams);
	
	if (me->ringBuf != NIL) {
		unsigned long newBufSize = Omega2BufSize(iOmega, me->curSR);
		if (newBufSize != me->ringBufCurSize) SetVocBufSize(me, newBufSize);
		}
	
	}

void EmericFloat(
	objEmeric*	me,
	double		iVal)
	
	{
	enum {
		inletSrcIn	= 0,
		inletTgtIn,
		inletOmega
		};
	
	long proxyNum = ObjectGetInlet(&me->coreObject.z_ob, me->coreObject.z_in);
	
	if (proxyNum == inletOmega)
		 EmericOmega(me, iVal);
	else error("%s: doesn't accept floats in inlet %ld", proxyNum);
	
	}


/******************************************************************************************
 *
 *	EmericTattle(me)
 *
 *	Post state information
 *
 ******************************************************************************************/

void
EmericTattle(
	objEmeric* me)
	
	{
	const double	kMeanFactor = ((double) kFBankSize) / ((double) me->ringBufCurSize),
					kLatency	= 1000.0 / (kMeanFactor * me->curSR);
	
	int		i;
	double	q = Omega2Q(me->omega),
			energy;
	
	post("%s state",
			kClassName);
	post("  Calculations based on sample rate of %lf", me->curSR);
	post("  Omega: %lf (Q = %lf, noiseThresh = %lu, energy latency %lf ms)",
			me->omega, q, me->noiseThresh, kLatency);
	if (me->srcXFade > 0.0) {
		post("Crossfading wet/dry: %lf source, %lf mutant", me->srcXFade, me->vocXFade);
		}
	
	if (q > 0.0) {
		post("  Filterbank state:");
		for (i = 0; i < kFBankSize; i += 1) {
			double			f0		= kFBankCenters[i],
							halfBW	= 0.5 * f0 / q;
			tResonParams*	params	= &me->rParams[i];
			tResonBuf*		srcBuf	= &me->srcRBuf[i];
			tResonBuf*		excBuf	= &me->excRBuf[i];
			
			if (f0 + halfBW < 0.5 * me->curSR)
				 post("    Center freq %lf (3dB BW: %lf - %lf)",
				 		f0, (f0 > halfBW) ? f0 - halfBW : (double) 0.0, f0 + halfBW);
			else post("    Center freq %lf (BW > Nyquist)", f0);
				
			post("      a0: %lf, b1: %lf, b2: %lf", params->a0, params->b1, params->b2);
			post("      srcRBuf: %lf, %lf", srcBuf->y1, srcBuf->y2);
			post("      excRBuf: %lf, %lf", excBuf->y1, excBuf->y2);
			}
		}
	else post("    -- infinite bandwidth --");
	
	post("  Amplitude averaging state:");
	post("  %ld values stored at 0x%p, ",
			me->ringBufMaxSize, me->ringBuf);
	post("  currently using %ld (%ld samples)",
			me->ringBufCurSize, me->ringBufCurSize / kFBankSize);
	
	energy = 0.0;
	for (i = 0; i < kFBankSize; i += 1) {
		double bandEnergy = me->ampSums[i] * kMeanFactor;
		post("    band %ld sum of absolute amplitudes: %lf (mean %lf)",
			 i, me->ampSums[i], bandEnergy);
		
		energy += bandEnergy;
		}
	post("  total energy: %lf", energy);
	
	}


/******************************************************************************************
 *
 *	EmericAssist
 *	EmericInfo(me)
 *
 *	Fairly generic Assist/Info methods.
 *	
 *	We don't use a lot of the parameters.
 *
 ******************************************************************************************/

void EmericAssist(objEmeric* me, void* box, long iDir, long iArgNum, char* oCStr)
	{
	#pragma unused(me, box)
	
	LitterAssist(iDir, iArgNum, strIndexInLeft, strIndexOutLeft, oCStr);
	}

void EmericInfo(objEmeric* me)
	{ LitterInfo(kClassName, &me->coreObject.z_ob, (method) EmericTattle); }

	
#pragma mark -
#pragma mark • DSP Methods

/******************************************************************************************
 *
 *	EmericDSP(me, ioDSPVectors, iConnectCounts)
 *
 ******************************************************************************************/

	static OSErr SetExciteBufSize(objEmeric* me, long iVecSize) 
		{
		OSErr	myErr = noErr;
		
		if (me->exciteBufSize > iVecSize) {
			SetPtrSize((Ptr) me->exciteBuf, iVecSize * sizeof(t_sample));
			me->exciteBufSize = iVecSize;
			}
		
		else if (me->exciteBufSize > 0) {
			// This means an excitation buffer exists, but we need a larger one
			// First try to resize the existing buffer
			SetPtrSize((Ptr) me->exciteBuf, iVecSize * sizeof(t_sample));
			if (MemError() == noErr) 
				me->exciteBufSize = iVecSize;
			else {
				// OK, Trash old buffer and try again below
				DisposePtr((Ptr) me->exciteBuf);
				me->exciteBuf		= NIL;
				me->exciteBufSize	= 0;
				}
			}
		
		if (me->exciteBuf == NIL) {						// NOTA BENE: *Not* else if...
			t_sample* newBuf = (t_sample*) NewPtr(iVecSize * sizeof(t_sample));
			if (newBuf != NIL) {
				me->exciteBuf		= newBuf;
				me->exciteBufSize	= iVecSize;
				}
			else myErr = MemError();
			}
		
		return myErr;
		}

void
EmericDSP(
	objEmeric*	me,
	t_signal**	iDSPVectors,
	short*		iConnectCounts)
	
	{
	enum {
		inletSrcIn			= 0,		// For input signal, begin~
		inletTgtIn,
		inletOmega,
		
		outletMut
		};
	
	OSErr	myErr	= noErr;
	double	curSR	= iDSPVectors[inletSrcIn]->s_sr;
	long	vecSize	= iDSPVectors[inletSrcIn]->s_n;
	
	// Update buffers to match current sample rate and vector size
	if (me->curSR != curSR) {
		myErr = SetSR(me, curSR);
		if (myErr != noErr) goto punt;
		}
	if (me->exciteBufSize != vecSize) {
		myErr = SetExciteBufSize(me, vecSize);
		if (myErr != noErr) goto punt;
		}

	// Always recalculate omega-related parameters!
	// This is necessary in case this object had a signal in the omega inlet the last time the DSP chain
	// was built, and the user has since deleted the signal.
	EmericOmega(me, me->omega);

	if (iConnectCounts[inletOmega] > 0)
		dsp_add(
			EmericPerform6, 6,
			me, (long) iDSPVectors[inletSrcIn]->s_n,
			iDSPVectors[inletSrcIn]->s_vec,
			iDSPVectors[inletTgtIn]->s_vec,
			iDSPVectors[inletOmega]->s_vec,
			iDSPVectors[outletMut]->s_vec
			);
	else dsp_add(
			EmericPerform5, 5,
			me, (long) iDSPVectors[inletSrcIn]->s_n,
			iDSPVectors[inletSrcIn]->s_vec,
			iDSPVectors[inletTgtIn]->s_vec,
			iDSPVectors[outletMut]->s_vec
			);
	
	return;
	// End of normal processing
	// ------------------------

	// Poor man's exception handling
punt:
	error("%s: can't allocate memory for buffers; object disabled.");

	}
	

/******************************************************************************************
 *
 *	PerformNull(me, iVecSize, iInput, iOutput)
 *
 *	Stripped down perform method when omega == 0.0
 *
 ******************************************************************************************/

static void
PerformNull(
	objEmeric*	me,
	long		iVecSize,
	t_sample*	iInput,
	t_sample*	iOutput)
	
	{
	// ASSERT: a0 member is the same for all filters in filterbank, b1 and b2 are all zero
	const double weight = me->rParams[0].a0;
	
	unsigned long	i = kFBankSize;
	tResonBuf*		s = me->srcRBuf;
	tResonBuf*		x = me->excRBuf;
	
	// Maintain state of filter buffers.
	// Need to special-case for the obnoxious situation of (iVecSize == 1)
	if (iVecSize == 1) {
		const double samp = iInput[0] * weight;
		do {
			s->y2 = s->y1;
			s++->y1 = samp;
			x->y2 = x->y1;
			x++->y1 = samp;
			} while (--i > 0);
		}
	else {
		// ASSERT: iVecSize >= 2
		const double	samp1 = iInput[iVecSize - 1] * weight,
						samp2 = iInput[iVecSize - 2] * weight;
		do {
			x->y2	= s->y2		= samp2;
			x++->y1 = s++->y1	= samp1;
			} while (--i > 0);
		}
	
	// ?? Should I update running sum buffer as well??
	
	// Copy input to output
	if (iInput != iOutput)
		do { *iOutput++ = *iInput++; } while (--iVecSize > 0);
	}

/******************************************************************************************
 *
 *	Perform(me, iInput, iOutput)
 *
 *	Core perform method
 *
 ******************************************************************************************/

	static inline void ClearOutputBuffer(long iBufSize, t_sample* iBuf)
		{ do {*iBuf++ = 0.0; } while (--iBufSize > 0); }
	
	// Have now inlined the Taus88 code so we can keep stuff in registers
	static inline void
	MungeExcitationBuffer(
		long			iBufSize,
		long			iNoiseThresh,
		const t_sample*	iSrc,
		const t_sample*	iTgt,
		t_sample*		oExc)
		
		{
		UInt32	s1, s2, s3;
			
		Taus88LoadGlobal(&s1, &s2, &s3);
		
		do {
			*oExc++ = (Taus88Process(&s1, &s2, &s3) > iNoiseThresh) ? *iSrc : *iTgt;
			iTgt++;
			iSrc++;
			} while (--iBufSize > 0);
		
		Taus88StoreGlobal(s1, s2, s3);
		
		}
	
	static inline double
	CalcFilterOutput(double x, double y1, double y2, double a0, double b1, double b2)
		{
		// Use first parameter as accumulator
		// This would even be OK if a function call weren't call-by-value
		x *= a0;
		x -= b1 * y1;
		x -= b2 * y2;
		
		return x;
		}

	static inline double
	UpdateRunningSum(double iNewVal, double iGate, double* ioSum, float* ioCache)
		{
		double	result = *ioSum;
		
		// Work with absolute values of amplitudes
		if (iNewVal < 0.0)
			iNewVal = -iNewVal;
		
		// Drop old sample, compensating for cumulative rounding errors
		result -= *ioCache;
		if (result < 0.0)
			result = 0.0;
		
		// Add new sample (if larger than gate value)
		if (iNewVal >= iGate) {
			result += iNewVal;
			*ioCache = iNewVal;
			}
		else *ioCache = 0.0;
		
		// Update cached sum and return value
		return (*ioSum = result);
		}


static void
Perform(
	objEmeric*	me,
	long		iVecSize,
	t_sample*	iSrc,
	t_sample*	iTgt,
	t_sample*	oOut)
	
	{
	const double kGate	= 9.5367431641e-7;  		// -120 dB gate for the filters.
	
	// Cache values from object into registers (read-only)
	const long			kRBCurSize	= me->ringBufCurSize;
	const double		kMeanFactor	= ((double) kFBankSize) / ((double) kRBCurSize);
	tSampleVector const	kRingBufBase = &me->ringBuf[0],
						kRingBufStop = kRingBufBase + kRBCurSize;		// tSampleVector is just a pointer to t_sample
	
	// Other stuff to get into registers;
	tResonParams*	curParam	= me->rParams;
	tResonBuf*		curSrcBuf	= me->srcRBuf;
	tResonBuf*		curExcBuf	= me->excRBuf;
	double*			curAmpSum;
	floatPtr		rbCurPos	= me->ringBufCurPos;
	long			i;
	
	// 1) Clear output buffer
	ClearOutputBuffer(iVecSize, oOut);
	
	// 2) Set up "real" excitation buffer
	MungeExcitationBuffer(iVecSize, me->noiseThresh, iSrc, iTgt, me->exciteBuf);

	// 3) Loop through filters in filterbank
	curAmpSum = me->ampSums;
	i = kFBankSize;
	do {
		// Get filter parameters into registers (read-only)
		const double	a0	= curParam->a0,
						b1	= curParam->b1,
						b2	= curParam++->b2;
		
		// Copy buffers into registers
		double	srcY1	= curSrcBuf->y1,
				srcY2	= curSrcBuf->y2,
				excY1	= curExcBuf->y1,
				excY2	= curExcBuf->y2;
		
		// Everything else we need...
		floatPtr	rBufPos = rbCurPos++; 
		t_sample*	s		= iSrc;
		t_sample*	t		= iTgt;
		t_sample*	o		= oOut;
		long		j		= iVecSize;
		
		do {
			double	srcY0, excY0;
			
			// Calculate current source amplitude and update filter buffer values
			srcY0 = CalcFilterOutput(*s++, srcY1, srcY2, a0, b1, b2);
			srcY2 = srcY1; srcY1 = srcY0;
			
			// Calculate current excitation amplitude and update filter buffer values
			excY0 = CalcFilterOutput(*t++, excY1, excY2, a0, b1, b2);
			excY2 = excY1; excY1 = excY0;
			
			// Convert source amplitude to analysis amplitude
			// (ie, scaled as component in filterbank)
			// and update ring buffer pointer
			srcY0  = UpdateRunningSum(srcY0, kGate, curAmpSum, rBufPos) * kMeanFactor;
			rBufPos += kFBankSize;
			if (rBufPos >= kRingBufStop)
				rBufPos -= kRBCurSize;
			
			// Update output value
			*o++ += srcY0 * excY0;
			} while (--j > 0);
		
		// Save buffer state for next run
		curSrcBuf->y1	= srcY1;
		curSrcBuf++->y2	= srcY2;
		curExcBuf->y1	= excY1;
		curExcBuf++->y2	= excY2;
		curAmpSum += 1;
		} while (--i > 0);
	
	
	// Work out "real" current position in ring buffer and store for next call to this function
	// Note that rbCurPos has already been incremented kFBankSize times
	rbCurPos += ((iVecSize - 1) * kFBankSize) % kRBCurSize;
	if (rbCurPos >= kRingBufStop)
		rbCurPos -= kRBCurSize;
	
	me->ringBufCurPos = rbCurPos;
	
	// Finally, recalculate one of the running sum buffers from scratch,
	// rotating through the filters, one per vector
	if (me->sumToUpdate == 0)
		me->sumToUpdate = kFBankSize;
	CalcRunningSum(me, --(me->sumToUpdate));
	}
	
/******************************************************************************************
 *
 *	PerformXFade(iParams)
 *
 ******************************************************************************************/
	
static void
PerformXFade(
	double		iSWeight,
	double		iVWeight,
	long		iVecSize,
	t_sample*	iSrc,
	t_sample*	iVoc)
	
	{
	do {
		double mix = iVWeight * *iVoc;
		
		mix += iSWeight * *iSrc++;
		*iVoc++ = mix;
		} while (--iVecSize > 0);
	}

	
/******************************************************************************************
 *
 *	EmericPerform6(iParams)
 *	EmericPerform5(iParams)
 *
 *	The parameter block for EmericPerform6() contains 7 values:
 *		- Address of this function
 *		- The performing object
 *		- Vector size
 *		- Vector of Source samples
 *		- Vector of Target samples
 *		- Vector of Omega values (we only sample the first one)
 *		- Vector for the output signal
 *
 *	The parameter block for EmericPerform5() is identical, except that there is no vector of
 *	omega values.
 *
 *	The actual work is done by the functions PerformNull(), PerformXFade() and Perform().
 *	PerformNull() is called when the current omega value is zero, and simply
 *	copies the input signal to the output signal (with some bookkeeping). The Perform()
 *	function is the "real thing".
 *
 ******************************************************************************************/
	
int*
EmericPerform6(
	int iParams[])
	
	{
	enum {
		paramFuncAddress	= 0,
		paramMe,
		paramVectorSize,
		paramSrc,
		paramTgt,
		paramOmega,
		paramOut,
		
		paramNextLink
		};
	
	double		omega;
	objEmeric*	me = (objEmeric*) iParams[paramMe];
	
	if (me->coreObject.z_disabled || me->ringBuf == NIL) goto exit;
	
	omega = ((tSampleVector) iParams[paramOmega])[0];
	
	if (omega == 0.0)
		PerformNull(me, iParams[paramVectorSize],
					(tSampleVector) iParams[paramSrc],
					(tSampleVector) iParams[paramOut]);
	
	else {
		float saveOmega	= me->omega;
		
		EmericOmega(me, omega);
		
		Perform(me, iParams[paramVectorSize],
				 (tSampleVector) iParams[paramSrc],
				 (tSampleVector) iParams[paramTgt],
				 (tSampleVector) iParams[paramOut]);
		
		if (me->srcXFade > 0.0) {
			PerformXFade(me->srcXFade, me->vocXFade, iParams[paramVectorSize],
						 (tSampleVector) iParams[paramSrc],
						 (tSampleVector) iParams[paramOut]);
			}
		
		me->omega = saveOmega;
		}
	
exit:
	return iParams + paramNextLink;
	}


int*
EmericPerform5(
	int iParams[])
	
	{
	enum {
		paramFuncAddress	= 0,
		paramMe,
		paramVectorSize,
		paramSrc,
		paramTgt,
		paramOut,
		
		paramNextLink
		};
	
	objEmeric*	me = (objEmeric*) iParams[paramMe];
	
	if (me->coreObject.z_disabled || me->ringBuf == NIL) goto exit;
	
	
	if (me->omega == 0.0)
		PerformNull(me, iParams[paramVectorSize],
					(tSampleVector) iParams[paramSrc],
					(tSampleVector) iParams[paramOut]);
	
	else {
		Perform(me, iParams[paramVectorSize],
				 (tSampleVector) iParams[paramSrc],
				 (tSampleVector) iParams[paramTgt],
				 (tSampleVector) iParams[paramOut]);
		
		if (me->srcXFade > 0.0) {
			PerformXFade(me->srcXFade, me->vocXFade, iParams[paramVectorSize],
						 (tSampleVector) iParams[paramSrc],
						 (tSampleVector) iParams[paramOut]);
			}
		}
	
exit:
	return iParams + paramNextLink;
	}
