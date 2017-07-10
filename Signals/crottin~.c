/*
	File:		crottin~.c

	Contains:	Pitch/noise using pulses of varying width/amplitude

	Written by:	Peter Castine

	Copyright:	© 2004 Peter Castine

	Change History (most recent first):

         <2>   26–4–2006    pc      Update for new LitterLib organization.
         <1>     9–12–04    pc      first checked in.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "LitterLib.h"									// Also #includes MaxUtils.h, ext.h
#include "TrialPeriodUtils.h"
#include "MiscUtils.h"
#include "Taus88.h"


#pragma mark • Constants

const char*		kClassName	= "lp.crottin~";			// Class name

	// Indices for STR# resource
enum {
		// Inlets
	strIndexInFreq			= lpStrIndexLastStandard + 1,
	strIndexInAmpModRate,
	strIndexInWidthModRate,
	
		// Outlets
	strIndexOutSegmentNoise,
	
		// Offsets for assiststring()
	strIndexInLeft			= strIndexInFreq,
	strIndexOutLeft			= strIndexOutSegmentNoise
	};
	
	// Indices for tracking our proxies
enum {
	proxyAmpRate			= 0,
	proxyWidthRate,
	
	proxyCount
	};

	// ...and for inlets/outlets
enum {
	inletFreq				= 0,
	inletAmpRate,
	inletWidthRate,
	
	outletNoise
	};
	
	
#pragma mark • Type Definitions

typedef struct segment {
	double	curAmp,
			curWidth,
			deltaAmp,
			deltaWidth,
			goalAmp,
			goalWidth;
	} tSegment;
	
typedef tSegment*	tSegmentPtr;
typedef tSegment	tSegmentVec[];

#pragma mark • Object Structure

typedef struct {
	t_pxobject		coreObject;
	
	Boolean			interpAmp,
					interpWidth,
					bang;						// Generate new segments on next call to
												// CrottinPerform()
	short			segCount,
					curSeg;
					
	long			sampsLeft,					// Samples left for current segment
					ampRateCycles,
					ampCtr,
					widthRateCycles,
					widthCtr;
	
	double			freq,
					sr,
					waveLen,					// sr/freq
					ampRate,
					widthRate,
					cumSampErr;
					
	tSegmentPtr		segments;
	
	} objCrottin;


#pragma mark • Global Variables



#pragma mark • Function Prototypes

	// Class message functions
void*	CrottinNew(SymbolPtr, short, Atom[]);
void	CrottinFree(objCrottin*);

	// Object message functions
static void CrottinBang(objCrottin*);
static void CrottinInt(objCrottin*, long);
static void CrottinFloat(objCrottin*, double);
static void	CrottinInterp(objCrottin*, long);
static void CrottinTattle(objCrottin*);
static void	CrottinAssist(objCrottin*, void* , long , long , char*);
static void	CrottinInfo(objCrottin*);

	// MSP Messages
static void	CrottinDSP(objCrottin*, t_signal**, short*);
static int*	CrottinPerform(int*);


#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark • Inline Functions



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
	setup(	&gObjectClass,							// Pointer to our class definition
			(method) CrottinNew,					// Instance creation function
			(method) CrottinFree,					// Default deallocation function
			sizeof(objCrottin),						// Class object size
			NIL,									// No menu function
			A_GIMME,								// Arguments too complex for Max arg parser
			0);
			
	dsp_initclass();

	// Messages
	addbang	((method) CrottinBang);
	addint	((method) CrottinInt);
	addfloat((method) CrottinFloat);
	addmess	((method) CrottinInterp,	"interp",	A_LONG, 0);
	addmess	((method) CrottinTattle,	"dblclick",	A_CANT, 0);
	addmess	((method) CrottinTattle,	"tattle",	A_NOTHING);
	addmess	((method) CrottinAssist,	"assist",	A_CANT, 0);
	addmess	((method) CrottinInfo,		"info",		A_CANT, 0);
	
	// MSP-Level messages
	LITTER_TIMEBOMB addmess	((method) CrottinDSP,		"dsp",		A_CANT, 0);

	//Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();
	
	}



#pragma mark -
#pragma mark • Utilities

/******************************************************************************************
 *
 *	SetSR(me, sr)
 *	SetFreq(me, freq)
 *	
 *	SetSR() recalculates all relevant components (waveLen, segment widths, etc.) from
 *	scratch.
 *
 *	SetFreq(), which can be called from the perform method, does a quicker recalculation,
 *	just scaling segment widths.
 *	
 ******************************************************************************************/

static void
SetWaveLen(
	objCrottin*	me,
	double		iWaveLen)
	{
	int			i		= me->segCount;
	tSegmentPtr curSeg	= me->segments;
	double		factor	= iWaveLen / me->waveLen;
	
	// ASSERT: i > 0
	do  {
		curSeg->curWidth	*= factor;
		curSeg->deltaWidth	*= factor;
		curSeg->goalWidth	*= factor;
		curSeg += 1;
		} while (--i > 0);
	
	me->sampsLeft *= factor;
	me->cumSampErr *= factor;
	if (me->cumSampErr > 1.0) {
		// When stretching the effective wavelength (due to an extreme lowering of
		// sample rate) it may be necessary to adjust the sampsLeft member
		me->sampsLeft += fmod(me->cumSampErr, 1.0);
		me->cumSampErr = floor(me->cumSampErr);
		}
	
	me->waveLen = iWaveLen;
	
	}

static void
SetAmpRate(
	objCrottin*	me,
	double		iRate)
	
	{
	int			i;
	double		cFac;
	tSegmentPtr	sp;
	
	if (iRate < 0.0)
		iRate = -iRate;
	
	me->ampRate = iRate;
	me->ampRateCycles = me->freq / iRate + 0.5;
	
	cFac = 1.0 / (double) me->ampRateCycles;
	for (i = me->segCount, sp = me->segments; i > 0; i -= 1, sp += 1) {
		sp->deltaAmp = (sp->goalAmp - sp->curAmp) * cFac;
		}
	
	}

static void
SetWidthRate(
	objCrottin*	me,
	double		iRate)
	
	{
	int			i;
	double		cFac;
	tSegmentPtr	sp;
	
	if (iRate < 0.0)
		iRate = -iRate;
	
	me->widthRate = iRate;
	me->widthRateCycles = me->freq / iRate + 0.5;
	
	cFac = 1.0 / (double) me->widthRateCycles;
	for (i = me->segCount, sp = me->segments; i > 0; i -= 1, sp += 1) {
		sp->deltaWidth = (sp->goalWidth - sp->curWidth) * cFac;
		}
	
	}

static void
SetSR(
	objCrottin*	me,
	double		iSR)
	
	{
	
	me->sr = iSR;
	SetWaveLen(me, iSR / me->freq);
	
		// Update amp and width rate parameters
	SetAmpRate(me, me->ampRate);
	SetWidthRate(me, me->widthRate);
	
	}


static void
SetFreq(
	objCrottin*	me,
	double		iFreq)
	
	{
	
	if (iFreq < 0.0)		// Actually, a negative frequency might make a subtle difference
		iFreq = - iFreq;	// to the sound, but the perform method is more efficient if
							// we don't have to check for negative increments.
							// And most people won't notice the difference.

	me->freq = iFreq;
	SetWaveLen(me, me->sr / iFreq);
	
	}


/******************************************************************************************
 *
 *	GenNewAmpGoals(me)
 *	GenNewWidthGoals(me)
 *	GenNewGoals(me)
 *	
 ******************************************************************************************/

static void
GenNewAmpGoals(
	objCrottin* me)
	
	{
	int			i  = me->segCount;
	tSegmentPtr	sp = me->segments;
	double		f  = 1.0 / me->ampRateCycles;
	
	do  {
		sp->goalAmp = ULong2Signal(Taus88(NIL));
		sp->deltaAmp = f * (sp->goalAmp - sp->curAmp);
		sp += 1;
		} while (--i > 0);
	
	me->ampCtr = me->ampRateCycles;
	
	}

static void
GenNewWidthGoals(
	objCrottin* me)
	
	{
	int			i  = me->segCount;
	tSegmentPtr	sp = me->segments;
	double		f  = 1.0 / me->widthRateCycles,
				l  = 0.0;
	
	do { l += sp++->goalWidth = ULong2Unit_zO(Taus88(NIL)); } while (--i > 0);
	
	l  = me->waveLen / l;
	i  = me->segCount;
	sp = me->segments;
	do  {
		sp->goalWidth *= l;
		sp->deltaWidth = f * (sp->goalWidth - sp->curWidth);
		sp += 1;
		} while (--i > 0);
	
	me->widthCtr = me->widthRateCycles;
	
	}

static void
GenNewGoals(
	objCrottin* me)
	
	{
	GenNewAmpGoals(me);
	GenNewWidthGoals(me);
	}

/******************************************************************************************
 *
 *	RollOverAmpGoals(me)
 *	RollOverWidthGoals(me)
 *	RollOverGoals(me)
 *	
 ******************************************************************************************/

static void
RollOverAmpGoals(
	objCrottin* me)
	
	{
	int			i = me->segCount;
	tSegmentPtr	sp = me->segments;
	
	do  {
		sp->curAmp = sp->goalAmp;
		sp += 1;
		} while (--i > 0);
	
	GenNewAmpGoals(me);
	
	}

static void
RollOverWidthGoals(
	objCrottin* me)
	
	{
	int			i = me->segCount;
	tSegmentPtr	sp = me->segments;
	
	do  {
		sp->curWidth = sp->goalWidth;
		sp += 1;
		} while (--i > 0);
	
	GenNewWidthGoals(me);
	
	}

static void
RollOverGoals(
	objCrottin* me)
	
	{
	int			i = me->segCount;
	tSegmentPtr	sp = me->segments;
	
	do  {
		sp->curAmp		= sp->goalAmp;
		sp->curWidth	= sp->goalWidth;
		sp += 1;
		} while (--i > 0);
	
	GenNewGoals(me);
	
	}

#pragma mark -
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	CrottinNew(iClassName, iArgC, iArgV)
 *	CrottinFree(me)
 *
 *	CrottinNew() expects the arguments (in left-to-right order, all optional):
 *	
 *		- segment count							[Default: 2]
 *		- frequency								[Default: 440 Hz]
 *		- rate of pulse amplitude change		[Default: 0]
 *		- rate of pulse segment width change	[Default: 0]
 *
 *	If the rate of pulse/amplitude change is left at zero, interpolation will be off by
 *	default
 *	
 ******************************************************************************************/

	static void InitSegments(tSegment ioSegs[], const short segCount)
		{
		double		widthSum = 0.0;
		int			i;
		tSegmentPtr	seg;
		
		for (i = segCount, seg = ioSegs; i > 0; i -= 1, seg++) {
			seg->curAmp		= 0.0;
			seg->curWidth 	= 0.0;
			seg->deltaAmp	= 0.0;
			seg->deltaWidth	= 0.0;
			seg->goalAmp 	= ULong2Signal(Taus88(NIL));
			
			widthSum += seg->goalWidth = ULong2Unit_zO(Taus88(NIL));
			}
		
		// Normalize widths; take inverse of widthSum so we can multiply instead of divide
		widthSum = 1.0 / widthSum;
		for (i = segCount, seg = ioSegs; i > 0; i -= 1, seg++)
			seg->goalAmp *= widthSum;

		}

void*
CrottinNew(
	SymbolPtr	iName,									// Must be 'lp.crottin~'
	short		iArgC,
	Atom		iArgV[])
	
	{
	#pragma unused(iName)
	
	enum {
		argSegCount		= 0,
		argFreq,
		argAmpRate,
		argWidthRate
		};
	
	const long kMaxSegCount = 1000;				// Ridiculously large, arbitrary value
												// Must be within range of 16-bit integers
	
	long	segCount	=   2;					// Arguments
	double	freq		= 440.0,				// Initialize to reasonable default values
			ampRate		=   0.0,
			widthRate	=   0.0;
	
	objCrottin* me	= NIL;
	
	// Parse arguments
	// Note that iArgC is one higher than the index of the last argument specified
	switch (iArgC) {
		default:
			// Spurious arguments
			error("%s: ignoring spurious arguments", kClassName);
			// ...fall into next case...
		case argWidthRate + 1:
			if ( ParseAtom(&iArgV[argWidthRate], false, true, 0, NIL, kClassName) )
				widthRate = iArgV[argWidthRate].a_w.w_float;
			// ...fall into next case...
		case argAmpRate + 1:
			if ( ParseAtom(&iArgV[argAmpRate], false, true, 0, NIL, kClassName) )
				ampRate = iArgV[argAmpRate].a_w.w_float;
			// ...fall into next case...
		case argFreq + 1:
			if ( ParseAtom(&iArgV[argFreq], false, true, 0, NIL, kClassName) )
				freq = iArgV[argFreq].a_w.w_float;
			// ...fall into next case...
		case argSegCount + 1:
			// In this case we generate our own error messages, so NIL for final argument
			if ( ParseAtom(&iArgV[argSegCount], true, false, 0, NIL, NIL)
					&& 0 < iArgV[argSegCount].a_w.w_long
					&& iArgV[argSegCount].a_w.w_long <= kMaxSegCount )
				segCount = iArgV[argSegCount].a_w.w_long;
			
			else {
				error("%s: ignoring invalid segment count parameter", kClassName);
				postatom(&iArgV[argSegCount]);
				}
			
			break;
			
		case 0:
			// No arguments
			break;
		}
	
	// Let Max/MSP allocate us, our inlets, and outlets.
	me = (objCrottin*) newobject(gObjectClass);
	dsp_setup(&(me->coreObject), 3);		// Signal inlets for frequency, etc.
	
	outlet_new(me, "signal");				// Outlet
	
	me->interpAmp		= (ampRate != 0.0);
	me->interpWidth		= (widthRate != 0.0);
	me->bang			= false;
	me->segCount		= segCount;
	me->curSeg			= -1;				// Temp values
	me->sampsLeft		= 0;
	me->ampRateCycles	= freq / ampRate + 0.5;
		if (me->ampRateCycles == 0) me->ampRateCycles = 1;
	me->ampCtr			= 0;
	me->widthRateCycles	= freq / widthRate + 0.5;
		if (me->widthRateCycles == 0) me->widthRateCycles = 1;
	me->widthCtr		= 0;
	me->freq			= freq;
	me->sr				= sys_getsr();		// Get real sample rate in "dsp" method
	me->waveLen			= me->sr / freq;
	me->ampRate			= ampRate;
	me->widthRate		= widthRate;
	me->cumSampErr		= 0.0;
	
	me->segments = (tSegmentPtr) NewPtr(segCount * sizeof(tSegment));
		if (me->segments == NIL) { me->segCount = 0; goto punt; }		// Oops!
	
	GenNewGoals(me);
	RollOverGoals(me);
	
	me->curSeg			= 0;				// Real values
	me->sampsLeft		= me->segments[0].curWidth;
	
	return me;
	// ----------------------------------------------------
	// End of normal processing
	
	// Poor man's exception handling
	// ----------------------------------------------------
punt:
	error("%s: can't create new object", kClassName);
	if (me != NIL) freeobject((Object*) me);
	return NIL;
	}

/******************************************************************************************
 *
 *	CrottinFree(me)
 *
 ******************************************************************************************/

void
CrottinFree(
	objCrottin* me)
	
	{
	
	dsp_free((t_pxobject*) me);					// Call me first.
	
	if (me->segments != NIL)
		DisposePtr((Ptr) me->segments);
	
	}

#pragma mark -
#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	CrottinBang(me)
 *
 ******************************************************************************************/

void CrottinBang(objCrottin* me)
	{ me->bang = true; }

/******************************************************************************************
 *
 *	SetFreq(me, iVal)
 *	SetAmpRate(me, iVal)
 *	SetWidthRate(me, iVal)
 *
 *	CrottinFloat(me, iVal)
 *	CrottinInt(me, iVal)
 *
 ******************************************************************************************/

void CrottinFloat(
	objCrottin*	me,
	double		iVal)
	
	{
	
	switch ( ObjectGetInlet((Object*) me, me->coreObject.z_in) ) {
		case proxyAmpRate + 1:		SetAmpRate(me, iVal);	break;
		case proxyWidthRate + 1:	SetWidthRate(me, iVal);	break;
		default:					SetFreq(me, iVal);		break;
		}
	
	}

void CrottinInt(objCrottin* me, long iVal)
	{ CrottinFloat(me, (double) iVal); }


/******************************************************************************************
 *
 *	CrottinInterp(me)
 *
 ******************************************************************************************/

void CrottinInterp(
	objCrottin*	me,
	long		iState)
	
	{
	iState = (iState != 0);						// Convert to canonical boolean value
	
	switch ( ObjectGetInlet((Object*) me, me->coreObject.z_in) ) {
		default:					me->interpAmp = iState;
			// Having set one flag, now fall into next case...
		case proxyWidthRate + 1:	me->interpWidth = iState;	break;
		case proxyAmpRate + 1:		me->interpAmp = iState;		break;
		}
	
	}


/******************************************************************************************
 *
 *	CrottinTattle(me)
 *
 *	Fairly generic Assist/Info methods
 *
 ******************************************************************************************/

void
CrottinTattle(
	objCrottin* me)
	
	{
	const char	kOnStr[] = "on",
				kOffStr[] = "off";
	
	post("%s state:", kClassName);
	post("  Interpolation");
	post("    Segment Amplitudes: %s", me->interpAmp ? kOnStr : kOffStr);
	post("    Segment Widths:     %s", me->interpWidth ? kOnStr : kOffStr);
	post("  %sbang pending", me->bang ? "" : "no ");
	post("  %ld segments, currently at segment %ld", me->segCount, me->curSeg);
	post("  frequency is: %lf (wavelen %lf at sample rate %lf)",
			me->freq, me->waveLen, me->sr);
	if (me->ampRate > 0.0) {
		post("  Modulating segment amplitudes at %lf Hz", me->ampRate);
		post("    (every %ld cycles)", me->ampRateCycles);
		}
	if (me->ampRate > 0.0) {
		post("  Modulating segment widths at %lf Hz", me->widthRate);
		post("    (every %ld cycles)", me->widthRateCycles);
		}
	
	}


/******************************************************************************************
 *
 *	CrottinAssist
 *	CrottinInfo(me)
 *
 *	Fairly generic Assist/Info methods
 *
 ******************************************************************************************/

void CrottinAssist(objCrottin* me, void* iBox, long iDir, long iArgNum, char* oCStr)
	{
	#pragma unused(me, iBox)
	
	LitterAssist(iDir, iArgNum, strIndexInLeft, strIndexOutLeft, oCStr);
	}

void CrottinInfo(objCrottin* me)
	{ LitterInfo(kClassName, (tObjectPtr) me, (method) CrottinTattle); }

#pragma mark -
#pragma mark • DSP Methods

/******************************************************************************************
 *
 *	CrottinDSP(me, ioDSPVectors, iConnectCounts)
 *
 ******************************************************************************************/

void
CrottinDSP(
	objCrottin*	me,
	t_signal**	ioDSPVectors,
	short*		iConnectCounts)
	
	{
	double sr = ioDSPVectors[0]->s_sr;
	
	if (me->sr != sr)
		SetSR(me, sr);								// Recalculate segment lengths, etc.
	
	dsp_add(
		CrottinPerform, 6,
		me, (long) ioDSPVectors[inletFreq]->s_n,
		(iConnectCounts[inletFreq] > 0)		? ioDSPVectors[inletFreq]->s_vec : NIL,
		(iConnectCounts[inletAmpRate] > 0)	? ioDSPVectors[inletAmpRate]->s_vec : NIL,
		(iConnectCounts[inletWidthRate] > 0) ? ioDSPVectors[inletWidthRate]->s_vec : NIL,
		ioDSPVectors[outletNoise]->s_vec
		);
	
	}
	

/******************************************************************************************
 *
 *	CrottinPerform(iParams)
 *
 *	Parameter block for CrottinPerform contains 7 values:
 *		- Address of this function
 *		- The performing crottin~ object
 *		- Vector size
 *		- Frequency signal
 *		- AmpRate signal
 *		- WidthRate signal
 *		- Output signal
 *
 ******************************************************************************************/

int*
CrottinPerform(
	int iParams[])
	
	{
	enum {
		paramFuncAddress	= 0,
		paramMe,
		paramVectorSize,
		paramFreq,
		paramAmpRate,
		paramWidthRate,
		paramOut,
		
		paramNextLink
		};
		
	long			i,
					segCount,
					seg,
					sampsLeft;
	double			x,							// Multi-purpose floating point register
					cumErr;
	tSegmentPtr		sp;
	tSampleVector	out;
	objCrottin*		me = (objCrottin*) iParams[paramMe];
	
	if (me->coreObject.z_disabled) goto exit;
	
	if (me->bang) {
		// DoTrigger(me);
		RollOverGoals(me);
		me->bang = false;
		}
	
	// Copy parameters into registers
	i			= (long) iParams[paramVectorSize];
	out			= (tSampleVector) iParams[paramOut];
	segCount	= me->segCount;
	seg			= me->curSeg;
	sp			= me->segments;
	sampsLeft	= me->sampsLeft;
	cumErr		= me->cumSampErr;
	
	// Need to update parameters?
	if ((float*) iParams[paramFreq] != NIL) {
		x = *((float*) iParams[paramFreq]);
		if (x != me->freq) SetFreq(me, x);
		}
	if ((float*) iParams[paramAmpRate] != NIL) {
		x = *((float*) iParams[paramAmpRate]);
		if (x != me->ampRate) SetAmpRate(me, x);
		}
	if ((float*) iParams[paramWidthRate] != NIL) {
		x = *((float*) iParams[paramWidthRate]);
		if (x != me->freq) SetWidthRate(me, x);
		}
	
	// Do our stuff
	do {
		double	amp = sp[seg].curAmp;
		long	j	= sampsLeft;				// Samples we will write in following loop
		
		// Check if we need to adjust for cumulative error from truncating floating point
		// segment lengths to integral numbers of samples
		if (cumErr >= 0.5) {
			j += 1;
			cumErr -= 1.0;
			}
		
		if (j > i) {							// Limit to available buffer size, or...
			j = i;
			sampsLeft -= j;
			}
		else {									// ...prepare next segment
			cumErr += fmod(sp[seg].curWidth, 1.0);
			
			if (me->interpAmp)
				sp[seg].curAmp += sp[seg].deltaAmp;
			if (me->interpWidth)
				sp[seg].curWidth += sp[seg].deltaWidth;
			
			seg += 1;
			
			// Time to cycle round?
			if (seg >= segCount) {
				if (me->interpAmp && --me->ampCtr <= 0)
					RollOverAmpGoals(me);
				if (me->interpWidth && --me->widthCtr <= 0)
					RollOverWidthGoals(me);
					
				seg = 0;
				}
			
			sampsLeft = sp[seg].curWidth;
			}
		
		// Decrement main counter
		i -= j;
		
		// Write samples
		while (j-- > 0)
			*out++ = amp;
		
		} while (i > 0);
	
	// Update object components
	me->curSeg		= seg;
	me->sampsLeft	= sampsLeft;
	me->cumSampErr	= cumErr;
	
	
exit:
	return iParams + paramNextLink;
	}
