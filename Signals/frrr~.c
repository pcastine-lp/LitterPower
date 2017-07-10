/*
	File:		frrr~.c

	Contains:	Linear congruence "noise".

	Written by:	Peter Castine

	Copyright:	© 2001-2002 Peter Castine

	Change History (most recent first):

         <7>   26–4–2006    pc      Update for new LitterLib organization.
         <6>   12–3–2005    pc      Handle int messages coming in the left inlet. Tip o' the hat to
                                    the persons responsible for changing Max behavior and not
                                    documenting that this sort of thing is necessary.
         <5>     14–1–04    pc      Update for Windows.
         <4>    6–7–2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <3>  30–12–2002    pc      Drop faux 'Vers' resource and related modifications. Add object
                                    version to FrrrInfo().
         <2>  28–11–2002    pc      Tidy up after initial check in.
         <1>  28–11–2002    pc      Initial check in.
		 9-Feb-2002:	Tested Global Optimization Level 4. Surprisingly, the result seems,
		 				if anything, a little slower than Level 0.
		14-Apr-2000:	First implementation.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files


#include "LitterLib.h"	// Also #includes MaxUtils.h, ext.h
#include "TrialPeriodUtils.h"
#include "MiscUtils.h"
#include "Taus88.h"



#pragma mark • Constants

const char*	kClassName		= "lp.frrr~";			// Class name


	// Indices for STR# resource
enum {
	strIndexInFreq		= lpStrIndexLastStandard + 1,
	strIndexInInterp,
	
	strIndexOutLoFreq
	};

	// Indices for MSP Inlets and Outlets.
enum {
	inletLoFreq			= 0,		// For begin~
	
	outletLoFreq
	};

	// Symbolic constants for interpolation
enum Interpolation {
	interpNone			= 0,
	interpLin,
	interpQuad,
	
	interpMin			= interpNone,
	interpMax			= interpQuad
	};

	

#pragma mark • Type Definitions

typedef enum Interpolation	tInterp;

#pragma mark • Object Structure

typedef struct {
	t_pxobject		coreObject;
	
	double			curVal,
					goal,
					slope,					// Used for linear & quadratic interpolation
					curve,					// Used for quadratic interpolation
					approxBaseFreq,			// As specified by user, in Hz
					curSR;					// Cache sample rate here (global value may not apply)
	
	unsigned long	baseFreqSamps,			// Exact, in samples; recalculated in dsp Method
					sampsToGo;				// Keep track of this between Perform calls
	
	tInterp			interp;					// None, linear, or quadratic
	} objLoFreq;


#pragma mark • Global Variables



#pragma mark • Function Prototypes

	// Class message functions
void*	FrrrNew(double, long);

	// Object message functions
static void FrrrBaseFreq(objLoFreq*, double);
static void FrrrBaseFreqInt(objLoFreq*, long);
static void FrrrInterp(objLoFreq*, long);
static void FrrrTattle(objLoFreq*);
static void	FrrrAssist(objLoFreq*, void* , long , long , char*);
static void	FrrrInfo(objLoFreq*);

	// MSP Messages
static void	FrrrDSP(objLoFreq*, t_signal**, short*);
static int*	FrrrPerform(int*);


#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark • Inline Functions

static inline double ValidateFreq(double iFreq)
	{ return iFreq < kFloatEpsilon ? kFloatEpsilon :  iFreq; }
	
static inline long CalcFreqSamps(double iSR, double iFreq)
	{
	double freqSamps = iSR / iFreq + 0.5;
	
		// Add test for exceeding kMaxLong because otherwise on Intel the double-to-long
		// conversion results in a negative value
	return (freqSamps > (double) kLongMax)
			? kLongMax
			: (freqSamps < 1.0) ? 1 : (long) freqSamps;
			// Damn. When we added 0.5 to the intialization of freqSamps at (to better
			// handle the Intel treatment of overflow when converting double to int), we
			// still left another 0.5 increment in the final calculation of effective
			// frequency. This means that our UBs were incorrectly calculating the value
			// from mid-2006 to early March-08. There will still be more patches built
			// on the correct calculation (2001-2006), so it seems the less painful solution
			// is to revert to the correct calculation.
			// Moral: don't make mistakes. 
	}

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
			(method) FrrrNew,			// Instance creation function
			(method) dsp_free,			// Default deallocation function
			sizeof(objLoFreq),			// Class object size
			NIL,						// No menu function
			A_DEFFLOAT,					// Optional arguments:	1. Base Frequency
			A_DEFLONG,					//						2. Interpolation (0, 1, or 2)
			0);		
	
	dsp_initclass();

	// Messages
	addfloat((method) FrrrBaseFreq);
	addint	((method) FrrrBaseFreqInt);
	addmess	((method) FrrrInterp,	"in1",		A_LONG, 0);
	addmess	((method) FrrrTattle,	"dblclick",	A_CANT, 0);
	addmess	((method) FrrrTattle,	"tattle",	A_NOTHING);
	addmess	((method) FrrrAssist,	"assist",	A_CANT, 0);
	addmess	((method) FrrrInfo,		"info",		A_CANT, 0);
	
	// MSP-Level messages
	LITTER_TIMEBOMB addmess	((method) FrrrDSP,		"dsp",		A_CANT, 0);

	//Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();
	
	}


#pragma mark -
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	FrrrNew(iNN)
 *	
 *	Arguments: Base frequency, interpolation.
 *	
 ******************************************************************************************/

void*
FrrrNew(
	double	iBaseFreq,
	long	iInterp)
	
	{
	const double	kDefBaseFreq	= 100.0;
	
	objLoFreq*	me	= NIL;
	double		curSR = sys_getsr();			// Just for starters, correct in DSP method.
	
	// Run through initialization parameters from right to left, handling defaults
	if (iInterp != 0) goto noMoreDefaults;		// Don't need to massage the default value
	
	iBaseFreq = (iBaseFreq == 0.0)
		 			? kDefBaseFreq
					: ValidateFreq(iBaseFreq);
noMoreDefaults:
	// Finished checking intialization parameters

	// Let Max/MSP allocate us, our inlets (from right to left), and outlets.
	me = (objLoFreq*) newobject(gObjectClass);
	dsp_setup(&(me->coreObject), 1);			// Signal inlet for benefit of begin~
												// Otherwise left inlet does Base 
												// Frequency only
	
	intin(me, 1);								// Add inlet for Interpolation
	
	outlet_new(me, "signal");
	
	// Initialize and store object components
	me->curVal			= 0.0;
	me->goal			= 0.0;
	me->slope			= 0.0;
	me->curve			= 0.0;
	me->approxBaseFreq	= iBaseFreq;
	me->curSR			= curSR;
	me->baseFreqSamps	= CalcFreqSamps(curSR, iBaseFreq);
	me->sampsToGo		= 0;
	FrrrInterp(me, iInterp);
	
	return me;
	}

#pragma mark -
#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	FrrrBaseFreq(me, iFreq);
 *	FrrrInterp(me, long);
 *	
 *	Update parameters, make sure nothing bad happens.
 *
 ******************************************************************************************/

		// Utility to calculate the interpolation parameters
	static inline void RecalcInterpParams(objLoFreq* me)
		{
		double	diff,
				durInv,
				durInv2;
		
		switch (me->interp) {
			case interpQuad:
				diff		= me->goal - me->curVal;
				durInv		= 0.5 / me->sampsToGo;		// Take parabolic curve over twice
				durInv2		= durInv * durInv;			// the distance between random samples
				me->slope	= 4.0 * diff * (durInv - durInv2);
				me->curve	= -8.0 * diff * durInv2;
				break;
				
			case interpLin:
				diff		= me->goal - me->curVal;
				me->slope	= diff / me->sampsToGo;
				break;
			
			default:
				// Must be interpNone. Nothing to do
				break;
			}
		
		}
	
	
		// Utility to convert the (approximate) user-specified Base Frequency to
		// the equivalent number of samples at the current sampling rate
	static inline void UpdateBaseFreq(objLoFreq* me)
		{
		unsigned long	newBaseFreq,
						newSampsToGo;
		
		newBaseFreq = CalcFreqSamps(me->curSR, me->approxBaseFreq);
		
		if (newBaseFreq == me->baseFreqSamps) // Nothing else needs doing...
			return;
		
		newSampsToGo = me->sampsToGo;
		if (newSampsToGo > kUShortMax) {
			// To avoid overflow, divide first and then multiply.
			newSampsToGo *= newBaseFreq / (double) me->baseFreqSamps;
			}
		else {
			// We can avoid the floating point division
			newSampsToGo *= newBaseFreq;
			newSampsToGo /= me->baseFreqSamps;
			}
		
		me->baseFreqSamps	= newBaseFreq;
		me->sampsToGo		= newSampsToGo;
			
		if (newSampsToGo > 0)				// If sampsToGo is 0, the parameters
			RecalcInterpParams(me);			// will be calculated in the next Perform
			
		}

void
FrrrBaseFreq(
	objLoFreq*	me,
	double		iFreq)
	
	{
	
	me->approxBaseFreq = ValidateFreq(iFreq);
	UpdateBaseFreq(me);
	
	}
	
void FrrrBaseFreqInt(objLoFreq* me, long iFreq)
	{ FrrrBaseFreq(me, (double) iFreq); }
	
void
FrrrInterp(
	objLoFreq*	me,
	long		iInterp)
	
	{
	Boolean	badParam = false;
	
	if (iInterp < interpMin) {
		badParam = true;
		me->interp = interpMin;
		}
	else if (iInterp > interpMax) {
		badParam = true;
		me->interp = interpMax;
		}
	else me->interp = (tInterp) iInterp;
	
	if (badParam)
		error("%s cannot perform interpolation of order %ld", kClassName, iInterp);
	
	}


/******************************************************************************************
 *
 *	FrrrTattle(me)
 *
 *	Post state information
 *
 ******************************************************************************************/

void
FrrrTattle(
	objLoFreq* me)
	
	{
	
	post("%s state",
			kClassName);
	post("  Base frequency:");
	post("    you specified %lf Hz,", me->approxBaseFreq);
	post("    The closest we come is %lu samples (that should be %f Hz)",
			me->baseFreqSamps, sys_getsr() / me->baseFreqSamps);
	post("    next update due in %lu samples", me->sampsToGo);
	post("  Interpolation parameters:");
	post("    Current value: %lf", me->curVal);
	post("    Goal value: %lf", me->goal);
	switch (me->interp) {
	case interpQuad:
		post("    Quadratic interpolation, slope is currently %lf, curvature is %lf",
			me->slope, me->curve);
		break;
	case interpLin:
		post("    Linear interpolation, slope is %lf", me->slope);
		break;
	default:
		// Must be interpNone
		post("    No interpolation");
		break;
		}
	
	}


/******************************************************************************************
 *
 *	FrrrAssist()
 *	FrrrInfo()
 *
 *	Fairly generic Assist/Info methods
 *
 ******************************************************************************************/

void FrrrAssist(objLoFreq* me, void* box, long iDir, long iArgNum, char* oCStr)
	{
	#pragma unused(me, box)
	
	LitterAssist(iDir, iArgNum, strIndexInFreq, strIndexOutLoFreq, oCStr);
	}

void FrrrInfo(objLoFreq* me)
	{ LitterInfo(kClassName, &me->coreObject.z_ob, (method) FrrrTattle); }



#pragma mark -
#pragma mark • DSP Methods

/******************************************************************************************
 *
 *	FrrrDSP(me, ioDSPVectors, iConnectCounts)
 *
 ******************************************************************************************/

void
FrrrDSP(
	objLoFreq*	me,
	t_signal**	ioDSPVectors,
	short*		connectCounts)
	
	{
	#pragma unused(connectCounts)
	
	// Make sure base frequency is correct!
	me->curSR = ioDSPVectors[outletLoFreq]->s_sr;
	UpdateBaseFreq(me);

	dsp_add(
		FrrrPerform, 3,
		me, (long) ioDSPVectors[outletLoFreq]->s_n, ioDSPVectors[outletLoFreq]->s_vec
		);
	
	}
	

/******************************************************************************************
 *
 *	FrrrPerform(iParams)
 *
 *	Parameter block for PerformSync contains 8 values:
 *		- Address of this function
 *		- The performing schhh~ object
 *		- Vector size
 *		- output signal
 *		- Imaginary output signal
 *
 ******************************************************************************************/

	static void PerformNoInterp(objLoFreq* me, long iVecSize, tSampleVector oNoise)
		{
		double			curVal			= me->curVal,
						goal			= me->goal;
		unsigned long	baseFreqSamps	= me->baseFreqSamps,
						sampsToGo		= me->sampsToGo;
		
		do {
			unsigned long sampsThisTime;
			
			if (sampsToGo == 0) {
				curVal		= goal;
				goal		= Long2Signal((long) Taus88(NULL));
				sampsToGo	= baseFreqSamps;
				// Note that we don't have to worry about interpolation parameters because
				// me->interp == interpNone;
				}
			
			sampsThisTime = sampsToGo;
			if (sampsThisTime > iVecSize)
				sampsThisTime = iVecSize;
			
				// ASSERT:	sampsThisTime ≤ iVecSize
				// 			sampsThisTime ≤ sampsToGo
			iVecSize -= sampsThisTime;
			sampsToGo -= sampsThisTime;
			
			do { *oNoise++ = curVal; } while (--sampsThisTime > 0);
				
			} while (iVecSize > 0);
		
		// The first two values may not need updating, but copying two registers back
		// into memory is pretty cheap; I don't think we would save enough to justify
		// the effort of flagging the appropriate condition and branching. Even if I'm
		// wrong, this can only be a few cycles here.
		me->curVal		= curVal;
		me->goal		= goal;
		me->sampsToGo	= sampsToGo;				// This always needs updating
		
		}
	
	static void PerformLinear(objLoFreq* me, long iVecSize, tSampleVector oNoise)
		{
		double			curVal			= me->curVal,
						goal			= me->goal,
						slope			= me->slope;
		unsigned long	baseFreqSamps	= me->baseFreqSamps,
						sampsToGo		= me->sampsToGo;
		
		do {
			unsigned long sampsThisTime;
			
			if (sampsToGo == 0) {
				// ASSERT:	curVal == goal
				// Leastaways close enough so as not to make any difference. So no need to update!
				goal		= Long2Signal((long) Taus88(NULL));
				sampsToGo	= baseFreqSamps;
				slope		= (goal - curVal) / baseFreqSamps;
				}
			
			sampsThisTime = sampsToGo;
			if (sampsThisTime > iVecSize)
				sampsThisTime = iVecSize;
			
				// ASSERT:	sampsThisTime ≤ iVecSize
				// 			sampsThisTime ≤ sampsToGo
			iVecSize -= sampsThisTime;
			sampsToGo -= sampsThisTime;
			
			do { *oNoise++ = curVal += slope; } while (--sampsThisTime > 0);
				
			} while (iVecSize > 0);
		
		// The first three values may not need updating, but copying a few registers back
		// into memory is pretty cheap; but I don't think we would save enough to justify
		// the effort of flagging the appropriate condition and branching. Even if I'm
		// wrong, this can only be a few cycles here.
		me->curVal		= curVal;
		me->goal		= goal;
		me->slope		= slope;
		me->sampsToGo	= sampsToGo;				// This always needs updating
		
		}

	static void PerformQuad(objLoFreq* me, long iVecSize, tSampleVector oNoise)
		{
		double			curVal			= me->curVal,
						goal			= me->goal,
						slope			= me->slope,
						curve			= me->curve;
		unsigned long	baseFreqSamps	= me->baseFreqSamps,
						sampsToGo		= me->sampsToGo;
		
		do {
			unsigned long sampsThisTime;
			
			if (sampsToGo == 0) {
				double	diff,
						durInv,
						durInv2;
				// ASSERT:	curVal == goal
				// Leastaways close enough so as not to make any difference. So no need to update!
				goal		= Long2Signal((long) Taus88(NULL));
				sampsToGo	= baseFreqSamps;
				diff		= goal - curVal;
				durInv		= 0.5 / sampsToGo;			// Take parabolic curve over twice
				durInv2		= durInv * durInv;			// the distance between random samples
				slope		= 4.0 * diff * (durInv - durInv2);
				curve		= -8.0 * diff * durInv2;
				}
			
			sampsThisTime = sampsToGo;
			if (sampsThisTime > iVecSize)
				sampsThisTime = iVecSize;
			
				// ASSERT:	sampsThisTime ≤ iVecSize
				// 			sampsThisTime ≤ sampsToGo
			iVecSize -= sampsThisTime;
			sampsToGo -= sampsThisTime;
			
			do {
				curVal	+= slope;
				slope	+= curve;
				*oNoise++ = curVal;
				} while (--sampsThisTime > 0);
				
			} while (iVecSize > 0);
		
		// The first three values may not need updating, but copying a few registers back
		// into memory is pretty cheap; but I don't think we would save enough to justify
		// the effort of flagging the appropriate condition and branching. Even if I'm
		// wrong, this can only be a few cycles here.
		me->curVal		= curVal;
		me->goal		= goal;
		me->slope		= slope;
		me->curve		= curve;
		me->sampsToGo	= sampsToGo;				// This always needs updating
		
		}

int*
FrrrPerform(
	int* iParams)
	
	{
	enum {
		paramFuncAddress	= 0,
		paramMe,
		paramVectorSize,
		paramOut,
		
		paramNextLink
		};
	
	objLoFreq*		me = (objLoFreq*) iParams[paramMe];
	
	if (me->coreObject.z_disabled) goto exit;
	
	// Which version to perform?
	switch (me->interp) {
			case interpQuad:
				PerformQuad(me, (long) iParams[paramVectorSize], (tSampleVector) iParams[paramOut]);
				break;
				
			case interpLin:
				PerformLinear(me, (long) iParams[paramVectorSize], (tSampleVector) iParams[paramOut]);
				break;
			
			default:
				// Must be interpNone.
				PerformNoInterp(me, (long) iParams[paramVectorSize], (tSampleVector) iParams[paramOut]);
				break;
		}
	
exit:
	return iParams + paramNextLink;
	}
