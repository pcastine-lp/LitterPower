/*
	File:		epoisse~.c

	Contains:	Max/MSP external object creating "pitched noise".

	Written by:	Peter Castine

	Copyright:	© 2001-2002 Peter Castine

	Change History (most recent first):

         <5>   26Ð4Ð2006    pc      Update for new LitterLib organization.
         <4>   23Ð3Ð2006    pc      Minor updates
         <3>    14Ð10Ð04    pc      Update to new object name
         <2>     20Ð3Ð04    pc      Work on first implementation. All v1 features
									implemented except signal input for frequency. And maybe
									a phase inlet.
         <1>     18Ð3Ð04    pc      first checked in.
*/


/*****************************************************************************************
 *****************************************************************************************/

#pragma mark ¥ Include Files

#include "LitterLib.h"	// Also #includes MaxUtils.h, ext.h
#include "TrialPeriodUtils.h"
#include "MiscUtils.h"
#include "Taus88.h"


#pragma mark ¥ Constants

const char*	kClassName		= "lp.epoisse~";		// Class name

enum {												// Have to use enum for GNU C
	kMaxBuf	= 512									// rather than const int
	};
const int	kMaxBufMask		= 0x01ff,				// Bit mask for % kMaxBuf
			kIterations		= 9;					// ld(kMaxBuf)

	// Indices for STR# resource
enum {
	strIndexInFreq		= lpStrIndexLastStandard + 1,
	strIndexInNu,
	strIndexInNN,
	
	strIndexOutEpoisse,
	
	strIndexInLeft		= strIndexInFreq,
	strIndexOutLeft		= strIndexOutEpoisse
	};

enum eFlags {
	flagStochastic		= 1,
	
	flagsNone			= 0
	};


#pragma mark ¥ Type Definitions



#pragma mark ¥ Object Structure

typedef struct {
	t_pxobject	coreObject;
	
	float		freq,			// Center frequency
				pi,				// Pitchiness factor (0 = noise, larger values are more stable)
				hurstExp,		// Hurst exponent (aka 'rho'), determines "color" of base noise
				hurstFac,		// Hurst factor, precalced from pow(0.5, hurstExp)
				hurstGain,		// Adjust initial "scale" factor in GenBuf to maintain constant
								// gain across different values of rho.
				sRate,			// Current sample rate
				sDur;			// Cache 1/sRate here... speeds up some important calculations
	double		phiFrac,		// fraction of "real" phi (integer portion stored in phiStep)	
				cumPhiErr;	
				
	long		phiStep,		// f(freq, sample rate)	
				curPhi,			// Index to next sample in the playback buffer
				countDown,		// Update main playback buffer every time this counter hits zero
				interpSamp,		// Counter for interpolating between cur and goal buffers
				flags;			// Sum of bits defined in enum eFlags
								// (currently only one flag defined)
	
	t_sample	cur[kMaxBuf + 1],		// Playback buffer. Duplicating zeroth sample at end
				goal[kMaxBuf + 1],		// of buffer makes the GenBuf() function easier to
				interp[kMaxBuf + 1];	// implement.
	} objEpoisse;


#pragma mark ¥ Global Variables



#pragma mark ¥ Function Prototypes

#if 0
	// Class message functions
void*	EpoisseNew(double, double, double);

	// Object message functions
static void EpoisseReset(objEpoisse*);
static void EpoisseFreq(objEpoisse*, double);
static void EpoissePi(objEpoisse*, double);
static void EpoisseRho(objEpoisse*, double);
static void EpoisseTattle(objEpoisse*);
static void	EpoisseAssist(objEpoisse*, void* , long , long , char*);
static void	EpoisseInfo(objEpoisse*);

	// MSP Messages
static void	EpoisseDSP(objEpoisse*, t_signal**, short*);
static int*	EpoissePerform(int*);
#endif

#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark ¥ Utility functions

/*****************************************************************************************
 *
 *	GenBuf(me)
 *	
 *****************************************************************************************/

static void
GenBuf(
	objEpoisse* me)
	
	{
	const float		kHurstFac	= me->hurstFac,
					kEnergy		= 0.25;						// ?? Need to tweak this ??
	
	tSampleVector const	goal	= me->goal,
						cur		= me->cur,
						interp	= me->interp;
	
	float	scale		= me->hurstGain;
	long	stride		= kMaxBuf / 2,
			offset		= kMaxBuf / 4;
	
	//
	// Voss random addition algorithm
	//
	
	// Set buf[0] and buf[kMaxBuf] to zero, choose a random value for the midpoint
	goal[0]			= 0.0;
	goal[stride]	= kEnergy * ULong2Signal(Taus88(NIL));	// ASSERT: stride = kMaxBuf/2
	goal[kMaxBuf]	= 0.0;
	
	// Recursive interpolation
	// Initial state: stride = kMaxBuf/2, offset = stride/2
	while (offset > 0) {
		int i;
		
		// Interpolate initial values at midpoints between values
		// calculated so far
		for (i = offset; i <= kMaxBuf - offset; i += stride) 
		    goal[i] = 0.5 * (goal[i-offset] + goal[i+offset]);
		
		// Add noise with reduced variance
		scale *= kHurstFac;
		for (i = offset; i < kMaxBuf; i += offset) {
			goal[i] += ULong2Signal(Taus88(NIL)) * scale;
			} 
		
		// Next generation: halve stride and offset
		stride = offset;										// ASSERT: offset == stride/2
		offset /= 2;											// Again, let compiler optimize.
		}
	
	// Calculate new interpolation values
	// Reuse the register variables scale and offset
	scale = 1.0 / me->pi;
	for (offset = 0; offset <= kMaxBuf; offset += 1)
		interp[offset] = scale * (goal[offset] - cur[offset]);
	 
	me->countDown = kMaxBuf * me->pi + 0.5;
	}


#pragma mark -
#pragma mark ¥ Object Message Handlers

/*****************************************************************************************
 *
 *	EpoisseFreq(me, iFreq)
 *	EpoissePi(me, iPi)
 *	EpoisseRho(me, iRho)
 *	
 *	Set parameters, update dependent members, make sure nothing bad happens.
 *
 *****************************************************************************************/

void
EpoisseFreq(
	objEpoisse*	me,
	double		iFreq)
	
	{
	double	phi,
			intPhi;
	
	if (iFreq < 0.0)		// Actually, a negative frequency could make a subtle difference
		iFreq *= -1.0;		// to the sound, but the perform method is more efficient if
							// we don't have to check for negative increments.
							// And most people won't notice the difference.

	phi		= (double) kMaxBuf * iFreq * me->sDur;			// Requires valid sample rate
	intPhi	= floor(phi);

	me->freq	= iFreq;
	me->phiFrac	= phi - intPhi;								// 0 <= me->phiFrac < 1
	me->phiStep	= ((long) phi) & kMaxBufMask;				// 0 <= me->phiStep < kMaxBuf
	
	}

void
EpoissePi(
	objEpoisse*	me,
	double		iPi)
	
	{
	double	scale;
	int		i;

	if (iPi < 0.0) {
		iPi *= -1.0;
		me->flags |= flagStochastic;						// set stochastic bit
		}
	else me->flags &= ~flagStochastic;						// clear stochastic bit
	
	iPi += 1.0;			// User counts from zero up, we count from one up
	
	// Adjust countdown and interpolation values
	scale	= me->pi / iPi;
	me->countDown = (double) me->countDown / scale + 0.5;
	for (i = 0; i <= kMaxBuf; i += 1)
		me->interp[i] *= scale;
	
	me->pi	= iPi;
	
	}

void
EpoisseRho(
	objEpoisse*	me,
	double		iRho)			// 'rho' is another name for the Hurst Exponent
	
	{
	int		i;					// counter
	double	hf,					// Hurst factor
			xn,					// Hurst factor taken to the nth power
			gain;				// Expected gain
	
	// Calculate expected gain
	hf = pow(0.5, iRho);		// Caluclate Hurst factor from Hurst exponent
	gain = xn = hf;				// Initialize gain to hf^1; handling the first loop iteration 
	i	= kIterations - 1;
	do {
		xn	 *= hf;
		gain += xn;
		} while (--i > 0);
	
	me->hurstExp	= iRho;
	me->hurstFac	= hf;
	me->hurstGain	= 1.0 / gain;			// Compensate for Hurst Factor gain
		
	}


/*****************************************************************************************
 *
 *	EpoisseReset(me)
 *
 *	Clear cur buffer, generate new goal and interp buffers, reset related counters
 *
 *****************************************************************************************/
 
 void
 EpoisseReset(
 	objEpoisse*	me)
 	
 	{
 	int i;
 	
 	// Clear cur buffer
 	for (i = 0; i <= kMaxBuf; i += 1)
 		me->cur[i] = 0.0;
 	
 	// Generate new goal and interpolation buffers
 	GenBuf(me);
 	
 	// Reset related counters
 	me->countDown = kMaxBuf * me->pi;
 	
 	}
 

/*****************************************************************************************
 *
 *	EpoisseTattle(me)
 *
 *	Post state information
 *
 *****************************************************************************************/

void
EpoisseTattle(
	objEpoisse* me)
	
	{
	
	post("%s state", kClassName);
	post("  frequency is: %lf (phiStep = %lf at sample rate %lf)",
			me->freq, (double) me->phiStep + me->phiFrac, me->sRate);
	post("	Current phi is at sample %ld, error %lf",
			me->curPhi, me->cumPhiErr);
	post("  Pitchiness factor is: %lf",
			me->pi - 1.0);
	post("	Hurst exponent (rho) is: %lf (Hurst factor: %lf, gain %lf)",
			me->hurstExp, me->hurstFac, me->hurstGain);
	
	}


/*****************************************************************************************
 *
 *	EpoisseAssist()
 *	EpoisseInfo()
 *
 *	Fairly generic Assist/Info methods
 *
 *****************************************************************************************/

void EpoisseAssist(objEpoisse* me, void* box, long iDir, long iArgNum, char* oCStr)
	{
	#pragma unused(me, box)
	
	LitterAssist(iDir, iArgNum, strIndexInLeft, strIndexOutLeft, oCStr);
	}

void EpoisseInfo(objEpoisse* me)
	{ LitterInfo(kClassName, &me->coreObject.z_ob, (method) EpoisseTattle); }



#pragma mark -
#pragma mark ¥ DSP Methods

/*****************************************************************************************
 *
 *	EpoissePerform(iParams)
 *
 *	Parameter block for PerformSync contains 8 values:
 *		- Address of this function
 *		- The performing schhh~ object
 *		- Vector size
 *		- output signal
 *		- Imaginary output signal
 *
 *****************************************************************************************/
	
int*
EpoissePerform(
	int* iParams)
	
	{
	enum {
		paramFuncAddress	= 0,
		paramMe,
		paramVectorSize,
		paramOut,
		
		paramNextLink
		};
	
	long			n;									// Count samples in sample vector
	tSampleVector	out;								// Signal outlet
	double			phiFrac,
					cumPhiErr;
	long			phiStep,
					curPhi,
					countDown,
					interpSamp;
	tSampleVector	cur,
					interp;
	Boolean			stochInterp;
	
	objEpoisse*		me = (objEpoisse*) iParams[paramMe];
	
	if (me->coreObject.z_disabled) goto exit;			// Quick punt
	
	// Copy parameters and object members into registers
	n			= (long) iParams[paramVectorSize];
	out			= (tSampleVector) iParams[paramOut];
	phiFrac		= me->phiFrac;
	cumPhiErr	= me->cumPhiErr;
	phiStep		= me->phiStep;
	curPhi		= me->curPhi;
	countDown	= me->countDown;
	interpSamp	= me->interpSamp;
	cur			= me->cur;
	interp		= me->interp;
	stochInterp	= me->flags & flagStochastic;			// Relies on flagStochastic <= 255	
	
	do {
		// Interpolate output value
		*out++ = (1.0 - cumPhiErr) * cur[curPhi] + cumPhiErr * cur[curPhi + 1];
		
		// Calculate phi for next sample
		curPhi		+= phiStep;
		cumPhiErr	+= phiFrac;
		if (cumPhiErr >= 1.0) {
			cumPhiErr	-= 1.0;
			curPhi		+= 1;
			}
		if (curPhi >= kMaxBuf)
			curPhi -= kMaxBuf;
		
		// Interpolate one sample from cur towards goal. Time to generate new wave table?
		if (countDown-- <= 0) {
			GenBuf(me);
			countDown = kMaxBuf * me->pi + 0.5;
			}
		else {
			cur[interpSamp] += stochInterp
								? interp[interpSamp] * (ULong2Signal(Taus88(NIL)) + 1.0)
								: interp[interpSamp];
			// Move incrementer index to next sample; wrap around at end of 
			interpSamp += 1;
			interpSamp &= kMaxBufMask;			
			}
		} while (--n > 0);
		
	// Save state
	me->cumPhiErr	= cumPhiErr;
	me->curPhi		= curPhi;
	me->countDown	= countDown;
	me->interpSamp	= interpSamp;
	
exit:
	return iParams + paramNextLink;
	}
	

/*****************************************************************************************
 *
 *	EpoisseDSP(me, ioDSPVectors, iConnectCounts)
 *
 *****************************************************************************************/

void
EpoisseDSP(
	objEpoisse*	me,
	t_signal*	ioDSPVectors[],
	short		connectCounts[])
	
	{
	#pragma unused(connectCounts)
	
	enum {
		outletEpoisse = 0
		};

	float sr = ioDSPVectors[0]->s_sr;				// Get real Sample Rate used by object
													// It may be different from global SR
	
	if (me->sRate != sr) {
		me->sRate	= sr;
		EpoisseFreq(me, me->freq);					// This deals with all dependent members
		}
	
	dsp_add(
		EpoissePerform, 3,
		me, (long) ioDSPVectors[outletEpoisse]->s_n, ioDSPVectors[outletEpoisse]->s_vec
		);
	
	}
	

#pragma mark -
#pragma mark ¥ Class Message Handlers

/*****************************************************************************************
 *
 *	EpoisseNew(iNN)
 *	
 *****************************************************************************************/

void*
EpoisseNew(
	double	iFreq,
	double	iPi,
	double	iRho)
	
	{
	objEpoisse*		me	= NIL;
	
	// Take all initialization values as give to us by Max/MSP

	// Let Max/MSP allocate us, our inlets, and outlets.
	me = (objEpoisse*) newobject(gObjectClass);
	dsp_setup(&(me->coreObject), 0);
	floatin(me, 2);							// Extra inlet for rho
	floatin(me, 1);							// Extra inlet for pi

	outlet_new(me, "signal");
	
	// Set up object to default state
	me->freq		= 0.0f;
	me->pi			= 1.0f;					// cf. EpoissePi()
	me->hurstExp	= 0.0f;
	me->hurstFac	= 1.0f;					// = pow(2.0, kDefHurstExp)
	me->hurstGain	= 1.0f / kIterations;
	me->sRate		= sys_getsr();			// Provisional, recheck at DSP time
	me->sDur		= 1.0f / me->sRate;
	me->phiFrac		= 0.0;
	me->cumPhiErr	= 0.0;
	me->phiStep		= 0;
	me->curPhi		= 0;
	me->countDown	= kMaxBuf;
	me->interpSamp	= 0;
	me->flags		= flagsNone;
	// Generate a zeroed buffer to start off with, and a random buffer as a goal
	EpoisseReset(me);

	// Finally, set up initialization values
	if (iFreq != 0.0)
		EpoisseFreq(me, iFreq);				// Sets freq, phiStep, phiFrac
	if (iPi != 0.0)	
		EpoissePi(me, iPi);					// Sets pi, interpSteps, resets curStep
	if (iRho != 0.0)
		EpoisseRho(me, iRho);				// Checks for valid rho,
											// sets hurstExp, hurstFac, hurstGain
	return me;
	}

#pragma mark -

/*****************************************************************************************
 *
 *	main()
 *	
 *	Standard Max/MSP External Object Entry Point Function
 *	
 *****************************************************************************************/

void
main(void)
	
	{
	LITTER_CHECKTIMEOUT(kClassName);
	
	// Standard Max/MSP initialization mantra
	setup(	&gObjectClass,				// Pointer to our class definition
			(method) EpoisseNew,		// Instance creation function
			(method) dsp_free,			// Default deallocation function
			sizeof(objEpoisse),			// Class object size
			NIL,						// No menu function
										// Optional arguments:
			A_DEFFLOAT,					//	1) Initial Frequency [Default: 440 Hz]
			A_DEFFLOAT,					//	2) Initial "Pitchiness" Factor [0.0]
			A_DEFFLOAT,					//	3) Initial Hurst Factor [0.0]
			0);		
	
	dsp_initclass();

	// Messages
	addbang	((method) EpoisseReset);
	addfloat((method) EpoisseFreq);
	addftx	((method) EpoissePi, 1);
	addftx	((method) EpoisseRho, 2);
	addmess	((method) EpoisseReset,	"reset",	A_NOTHING);
	addmess	((method) EpoisseTattle,"dblclick",	A_CANT, 0);
	addmess	((method) EpoisseTattle,"tattle",	A_NOTHING);
	addmess	((method) EpoisseAssist,"assist",	A_CANT, 0);
	addmess	((method) EpoisseInfo,	"info",		A_CANT, 0);
	
	// MSP-Level messages
	LITTER_TIMEBOMB addmess	((method) EpoisseDSP,	"dsp",		A_CANT, 0);

	//Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();
	
	}


