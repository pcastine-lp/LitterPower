/*
	File:		netochka~.c

	Contains:	General-purpose signal quality degradation.

	Written by:	Peter Castine

	Copyright:	© 2002 Peter Castine

	Change History (most recent first):

         <8>   26–4–2006    pc      Update for new LitterLib organization.
         <7>     11–1–04    pc      Update for modified LitterInit()
         <6>      8–1–04    pc      Update for Windows.
         <5>    6–7–2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <4>  30–12–2002    pc      Add object version to NetochkaInfo()
         <3>  30–12–2002    pc      Use 'STR#' resource instead of faux 'Vers' resource for storing
                                    version information used at run-time.
         <2>  28–11–2002    pc      Tidy up after initial check in.
         <1>  28–11–2002    pc      Initial check in.
		20-Feb-2002:	Implement floating-point NN factor (i.e., fractional bit resolution)
		 3-Feb-2002:	First implementation.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "LitterLib.h"	// Also #includes MaxUtils.h, ext.h
#include "MiscUtils.h"
#include "TrialPeriodUtils.h"
#include "Taus88.h"


#pragma mark • Constants

const char*	kClassName		= "lp.nn~";			// Class name

const int	kMaxNN			= 31,
			kMinNN			= -31;

	// Indices for STR# resource
enum {
	strIndexInSig		= lpStrIndexLastStandard + 1,
	strIndexInNN,
	strIndexInSR,
	
	strIndexOutSig,
	
	strIndexInLeft		= strIndexInSig,
	strIndexOutLeft		= strIndexOutSig
	};

	// Indices for MSP Inlets and Outlets.
enum {
	inletSigIn			= 0,		// For input signal, begin~
	
	outletSigOut
	};

#pragma mark • Type Definitions

#pragma mark • Object Structure

typedef struct {
	t_pxobject		coreObject;
	
	int				nn;					// Number of bits to mask out or dither
	double			factor,				// Values depend on nn.
					offset,
					nnFrac;
	
	double			curSR,				// Current MSP sample rate.
					effSR,				// Effective Sample Rate (as entered by user)
					skipSamps,			// Number of samples to skip when degrading
					curSkipFrac;		// Use this to maintain accuracy of stepping through
										// the signal in integral steps even when skipSamps
										// has a fractional component.
	float			curSamp;			// Cache downsampled value between calls to Perform()
	} tDegrade;


#pragma mark • Global Variables


#pragma mark • Function Prototypes

	// Class message functions
void*	NetochkaNew(double, double);

	// Object message functions
static void NetochkaNN(tDegrade*, double);
static void NetochkaESR(tDegrade*, double);
static void NetochkaTattle(tDegrade*);
static void	NetochkaAssist(tDegrade*, void* , long , long , char*);
static void	NetochkaInfo(tDegrade*);
//static void	NetochkaVers(tDegrade*, long, Symbol*);


	// MSP Messages
static void	NetochkaDSP(tDegrade*, t_signal**, short*);
static int*	NetochkaPerform(int*);


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
	setup(	&gObjectClass,				// Pointer to our class definition
			(method) NetochkaNew,			// Instance creation function
			(method) dsp_free,			// Default deallocation function
			sizeof(tDegrade),			// Class object size
			NIL,						// No menu function
			A_DEFFLOAT,					// Optional arguments:	1. NN Factor
			A_DEFFLOAT,					// 						2. Effective Sample Rate
			0);	
	
	dsp_initclass();
	
	// Messages
	addftx	((method) NetochkaNN, 1);
	addftx	((method) NetochkaESR, 2);
	addmess	((method) NetochkaTattle,	"dblclick",	A_CANT, 0);
	addmess	((method) NetochkaTattle,	"tattle",	A_NOTHING);
	addmess	((method) NetochkaAssist,	"assist",	A_CANT, 0);
	addmess	((method) NetochkaInfo,		"info",		A_CANT, 0);
	
	// MSP-Level messages
	LITTER_TIMEBOMB addmess	((method) NetochkaDSP,		"dsp",		A_CANT, 0);

	// Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();

	}

#pragma mark -
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	NetochkaNew(iNN)
 *	
 ******************************************************************************************/

void*
NetochkaNew(
//	long	iNN,
	double	iNN,
	double	iESR)
	
	{
	tDegrade*		me	= NIL;
	
	// Take intialization parameters as they come

	// Let Max/MSP allocate us, our inlets, and outlets.
	me = (tDegrade*) newobject(gObjectClass);
		if (me == NIL) goto punt;
		
	dsp_setup(&(me->coreObject), 1);			// Signal inlet
	outlet_new(me, "signal");
	
		// Additional inlets, right-to-left
	floatin(me, 2);								// Effective Sample Rate
	floatin(me, 1);								// NN factor
	
	// Set up object components
	me->curSR		= sys_getsr();			// Just for starters; double-check in NetochkaDSP().
	me->curSamp		= 0.0;
	me->curSkipFrac	= 0.0;
	
	NetochkaNN(me, iNN);
	NetochkaESR(me, iESR);

punt:
	return me;
	}

#pragma mark -
#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	NetochkaNN(me, iNN)
 *	NetochkaESR(me, iESR)
 *	
 *	Set parameters, making sure nothing bad happens.
 *
 ******************************************************************************************/

void NetochkaNN(
	tDegrade* me,
	double	iNN)
	
	{
//	int	intNN	= (iNN >= 0.0) ? floor(iNN) : ceil(iNN);
	int	intNN	= iNN;											// Truncate to integer
	
	if (intNN == 0) {
		me->nn			= 0;
		me->nnFrac		= 0.0;
		me->factor		= 0.0;
		me->offset		= 0.0;
		}
	
	else if (intNN > 0) {
		if (iNN > kMaxNN) iNN = intNN = kMaxNN;
		
		me->nn		= intNN;
		me->nnFrac	= iNN - intNN;
		me->factor	= 1 << (31 - intNN);
		me->offset	= 0.5 / me->factor;
		}
	
	else {
		if (iNN < kMinNN) iNN = intNN = kMinNN;
		
		me->nn		= intNN;
		me->nnFrac	= intNN - iNN;
		me->factor	= 1 << (31 + intNN);
		me->offset	= 0.5 / me->factor;
		}
	
	}

void NetochkaESR(
	tDegrade*	me,
	double		iESR)
	
	{
	if (iESR <= 0.0) {
		me->effSR		= 0.0;
		me->skipSamps	= 0.0;
		}
	else {
		me->effSR		= iESR;
		me->skipSamps	= (iESR >= me->curSR)
							? 0.0
							: me->curSR / iESR - 1.0;
		}
	
	}

/******************************************************************************************
 *
 *	NetochkaTattle(me)
 *
 *	Post state information
 *
 ******************************************************************************************/

void
NetochkaTattle(
	tDegrade* me)
	
	{
	
	post("%s state",
			kClassName);
	post("  NN factor is: %d + %f (factor = %f, offset = %f)",
			me->nn,
			me->nnFrac,
			me->factor,
			me->offset);
	post("  Effective SR is: %f (repeat each sample %f times)",
			me->effSR,
			me->skipSamps + 1);
	post("  Currrent step fraction: %f",
			me->curSkipFrac);
	
	}


/******************************************************************************************
 *
 *	NetochkaAssist
 *	NetochkaInfo(me)
 *
 *	Fairly generic Assist/Info methods.
 *	
 *	We don't use a lot of the parameters.
 *
 ******************************************************************************************/

void NetochkaAssist(tDegrade* me, void* box, long iDir, long iArgNum, char* oCStr)
	{
	#pragma unused(me, box)
	
	LitterAssist(iDir, iArgNum, strIndexInLeft, strIndexOutLeft, oCStr);
	}

void NetochkaInfo(tDegrade* me)
	{ LitterInfo(kClassName, &me->coreObject.z_ob, (method) NetochkaTattle); }

	
#pragma mark -
#pragma mark • DSP Methods

/******************************************************************************************
 *
 *	NetochkaDSP(me, ioDSPVectors, iConnectCounts)
 *
 ******************************************************************************************/

void
NetochkaDSP(
	tDegrade*	me,
	t_signal**	ioDSPVectors,
	short*		connectCounts)
	
	{
	double	curSR = ioDSPVectors[inletSigIn]->s_sr;
	
	if (me->curSR != curSR) {
		me->curSR = curSR;
		NetochkaESR(me, me->effSR);		// Safest way to make sure everything is recalculated
		}
	
	dsp_add(
		NetochkaPerform, 4,
		me, (long) ioDSPVectors[inletSigIn]->s_n,
		ioDSPVectors[inletSigIn]->s_vec,
		ioDSPVectors[outletSigOut]->s_vec
		);
	
	}
	

/******************************************************************************************
 *
 *	NetochkaPerform(iParams)
 *
 *	Parameter block for PerformSync contains 4 values:
 *		- Address of this function
 *		- The performing nn~ object
 *		- Vector size
 *		- output signal
 *
 ******************************************************************************************/

	static /*inline*/ float
	Mask(double iSamp, double iFactor, double iOffset, long iShift)
		{
		iSamp += 1.0;
		
		if (iSamp < 2.0) {
			unsigned long intSamp = iSamp * iFactor;
			return ULong2Signal(intSamp << iShift) + iOffset;
			}
		else return 1.0 - iOffset;
		}
	
	static /*inline*/ float
	MaskAndNudge(double iSamp, double iFactor, double iOffset, double iNudge, long iShift)
		{
		iSamp += 1.0;
		
		if (iSamp < 2.0) {
			unsigned long	intSamp = iSamp * iFactor;
			
			if (intSamp & 0x01)
					iNudge *= -iOffset;
			else	iNudge *= iOffset;
			
			return ULong2Signal(intSamp << iShift) + iOffset + iNudge;
			}
		else {
			iNudge += 1.0;
			
			return 1.0 - iNudge * iOffset;
			}
		}
		
	static /*inline*/ float
	Dither(double iSamp, double iFactor, double iOffset, long iShift)
		{
		iSamp += 1.0;
		
		if (iSamp < 2.0) {
			unsigned long	intSamp = iSamp * iFactor;
			double			dither = Taus88TriSig() + 1.0;
			
			return ULong2Signal(intSamp << iShift) + iOffset * dither;
			}
		else {
			double dither = Taus88TriSig() + 1.0;
			
			return 1.0 - iOffset * dither;
			}
		}

	static /*inline*/ float
	DitherAndNudge(double iSamp, double iFactor, double iOffset, double iNudge, long iShift)
		{
		iSamp += 1.0;
		
		if (iSamp < 2.0) {
			unsigned long	intSamp = iSamp * iFactor;
			double			dither = (1.0 + iNudge) * Taus88TriSig() + 1.0;
			
			if (intSamp & 0x01)
					iNudge *= -iOffset;
			else	iNudge *= iOffset;
			
			return ULong2Signal(intSamp << iShift) + iOffset * dither + iNudge;
			}
		else {
			double dither = Taus88TriSig() + 1.0;
			iNudge += 1.0;
			
			return 1.0 - iNudge * iOffset * dither;
			}
		}
	
int*
NetochkaPerform(
	int iParams[])
	
	{
	enum {
		paramFuncAddress	= 0,
		paramMe,
		paramVectorSize,
		paramIn,
		paramOut,
		
		paramNextLink
		};
	
	long			vecCounter;
	tSampleVector	inSig,
					outSig;
	int				nn;
	double			skipSamps;
	tDegrade*		me = (tDegrade*) iParams[paramMe];
	
	if (me->coreObject.z_disabled) goto exit;
	
	// Copy parameters into registers
	vecCounter	= (long) iParams[paramVectorSize];
	inSig		= (tSampleVector) iParams[paramIn];
	outSig		= (tSampleVector) iParams[paramOut];
	
	// Do our stuff
	nn			= me->nn;
	skipSamps	= me->skipSamps;
	
	if (nn == 0) {
		// No nn factor
		if (skipSamps == 0.0) {
			// Not much to do...
			// ...and even then, only if input and output are not sharing the same
			// vector.
			if (inSig != outSig)
				do { *outSig++ = *inSig++; } while (--vecCounter > 0);
			}
		
		else {
			// Downsample
			float	curSamp		= me->curSamp;
			double	curSkipFrac	= me->curSkipFrac;
			long	extraSteps	= curSkipFrac;					// Truncate float-to-integer
			
			if (extraSteps >= vecCounter) {
				// Fill vector with current sample; update cached counter.
				me->curSkipFrac -= vecCounter;
				do { *outSig++ = curSamp; } while (--vecCounter > 0);
				}
			
			else {
				// Finish off last run of the current sample...
				inSig		+= extraSteps;
				vecCounter	-= extraSteps;
				curSkipFrac -= extraSteps;
				
				while (extraSteps-- > 0)
					*outSig++ = curSamp;
				
				// ... and run through the rest of the vector
				while (vecCounter > 0) {
					curSamp = *outSig++ = *inSig++;
					vecCounter -= 1;
					
					extraSteps	= skipSamps + curSkipFrac;
					if (extraSteps > vecCounter)
						extraSteps = vecCounter;
					inSig		+= extraSteps;
					vecCounter	-= extraSteps;
					curSkipFrac	+= skipSamps - extraSteps;
					
					while (extraSteps-- > 0) 
						*outSig++ = curSamp;
					}
				
				me->curSamp		= curSamp;
				me->curSkipFrac	= curSkipFrac;
				}
			}
		}														// END IF (nn == 0)
	
	else {
		double	nnFrac	= me->nnFrac,
				offset	= me->offset,
				factor	= me->factor;
		
		if (nnFrac == 0.0) {
			// Integral NN/Dither
			float (*IntNNFunc)(double, double, double, long);
			
			if (nn < 0) {
				IntNNFunc = Dither;
				nn = -nn;
				}
			else IntNNFunc = Mask;
			
			if (skipSamps == 0.0) do {
				// Just mask/dither
				*outSig++ = IntNNFunc(*inSig++, factor, offset, nn);
				} while (--vecCounter > 0);
			
			else {
				// Mask/dither and downsample
				float	curSamp		= me->curSamp;
				double	curSkipFrac	= me->curSkipFrac;
				long	extraSteps	= curSkipFrac;				// Truncate float-to-integer
				
				if (extraSteps >= vecCounter) {
					// Fill vector with current sample; update cached counter.
					me->curSkipFrac -= vecCounter;
					do { *outSig++ = curSamp; } while (--vecCounter > 0);
					}
				
				else {
					// Finish off last run of the current sample...
					inSig		+= extraSteps;
					vecCounter	-= extraSteps;
					curSkipFrac -= extraSteps;
					
					while (extraSteps-- > 0)
						*outSig++ = curSamp;
					
					// ... and run through the rest of the vector
					while (vecCounter > 0) {
						curSamp = *outSig++ = IntNNFunc(*inSig++, factor, offset, nn);
						vecCounter -= 1;
						
						extraSteps	= skipSamps + curSkipFrac;
						if (extraSteps > vecCounter)
							extraSteps = vecCounter;
						inSig		+= extraSteps;
						vecCounter	-= extraSteps;
						curSkipFrac	+= skipSamps - extraSteps;
						
						while (extraSteps-- > 0) 
							*outSig++ = curSamp;
						}
					
					me->curSamp		= curSamp;
					me->curSkipFrac	= curSkipFrac;
					}											// END (extraSteps < vecCounter)
				}												// END (skipSamps != 0.0)
			}													// END IF (nnFrac == 0.0)
		
		else {
			// Fractional NN/Dither
			float (*FloatNNFunc)(double, double, double, double, long);
			
			if (nn < 0) {
				FloatNNFunc = DitherAndNudge;
				nn = -nn;
				}
			else FloatNNFunc = MaskAndNudge;
			
			if (skipSamps == 0.0) do {
				// Just mask/dither
				*outSig++ = FloatNNFunc(*inSig++, factor, offset, nnFrac, nn);
				} while (--vecCounter > 0);
			
			else {
				// Mask/dither and downsample
				float	curSamp		= me->curSamp;
				double	curSkipFrac	= me->curSkipFrac;
				long	extraSteps	= curSkipFrac;				// Truncate float-to-integer
				
				if (extraSteps >= vecCounter) {
					// Fill vector with current sample; update cached counter.
					me->curSkipFrac -= vecCounter;
					do { *outSig++ = curSamp; } while (--vecCounter > 0);
					}
				
				else {
					// Finish off last run of the current sample...
					inSig		+= extraSteps;
					vecCounter	-= extraSteps;
					curSkipFrac -= extraSteps;
					
					while (extraSteps-- > 0)
						*outSig++ = curSamp;
					
					// ... and run through the rest of the vector
					while (vecCounter > 0) {
						curSamp = *outSig++ = FloatNNFunc(*inSig++, factor, offset, nnFrac, nn);
						vecCounter -= 1;
						
						extraSteps	= skipSamps + curSkipFrac;
						if (extraSteps > vecCounter)
							extraSteps = vecCounter;
						inSig		+= extraSteps;
						vecCounter	-= extraSteps;
						curSkipFrac	+= skipSamps - extraSteps;
						
						while (extraSteps-- > 0) 
							*outSig++ = curSamp;
						}
					
					me->curSamp		= curSamp;
					me->curSkipFrac	= curSkipFrac;
					}											// END (extraSteps < vecCounter)
				}												// END (skipSamps != 0.0)
			}													// END (nnFrac != 0.0)
		}														// END (nn != 0)
	
exit:
	return iParams + paramNextLink;
	}
