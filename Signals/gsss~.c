/*
	File:		gsss~.c

	Contains:	Gaussian noise.

	Written by:	Peter Castine

	Copyright:	© 2002 Peter Castine

	Change History (most recent first):

         <8>   26–4–2006    pc      Update for new LitterLib organization.
         <7>   23–3–2006    pc      Use Marsaglia-Bray Gaussian generator
         <6>     9–12–04    pc      Update to use inlet_getproxy()
         <5>     14–1–04    pc      Update for Windows.
         <4>    6–7–2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <3>  30–12–2002    pc      Drop faux 'Vers' resource and related modifications. Add object
                                    version to GsssInfo().
         <2>  28–11–2002    pc      Tidy up after initial check in.
         <1>  28–11–2002    pc      Initial check in.
		30-Jan-2002:	First implementation.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "LitterLib.h"	// Also #includes MaxUtils.h, ext.h
#include "TrialPeriodUtils.h"
#include "RNGGauss.h"
#include "Taus88.h"


#pragma mark • Constants

const char*	kClassName		= "lp.gsss~";			// Class name

const int	kMaxNN			= 31;

	// Indices for STR# resource
enum {
	strIndexInMu		= lpStrIndexLastStandard + 1,
	strIndexInStdDev,
	strIndexOutGauss,
	
	strIndexInLeft		= strIndexInMu,
	strIndexOutLeft		= strIndexOutGauss
	};

	// Inlets/Outlet MSP knows about
enum {
	inletMu			= 0,
	inletStdDev,
	
	outletNoise
	};

#pragma mark • Type Definitions



#pragma mark • Object Structure

typedef struct {
	t_pxobject		coreObject;
	
	double			mu,
					stdDev;
	} objGaussNoise;


#pragma mark • Global Variables



#pragma mark • Function Prototypes

	// Class message functions
void*	GsssNew(double, double);

	// Object message functions
static void GsssFloat(objGaussNoise*, double);
//static void GsssInt(objGaussNoise*, long);
static void GsssMu(objGaussNoise*, double);
static void GsssStdDev(objGaussNoise*, double);
static void GsssTattle(objGaussNoise*);
static void	GsssAssist(objGaussNoise*, void* , long , long , char*);
static void	GsssInfo(objGaussNoise*);

	// MSP Messages
static void	GsssDSP(objGaussNoise*, t_signal**, short*);
static int*	GsssPerformStat(int*);
static int*	GsssPerformDyn(int*);


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
			(method) GsssNew,			// Instance creation function
			(method) dsp_free,			// Default deallocation function
			sizeof(objGaussNoise),		// Class object size
			NIL,						// No menu function
			A_DEFFLOAT,					// Optional arguments:	1. Mu
			A_DEFFLOAT,					//						2. Std Var.
			0);	
	
	dsp_initclass();

	// Messages
	addfloat((method) GsssFloat);
	addmess	((method) GsssTattle,	"dblclick",	A_CANT, 0);
	addmess	((method) GsssTattle,	"tattle",	A_NOTHING);
	addmess	((method) GsssAssist,	"assist",	A_CANT, 0);
	addmess	((method) GsssInfo,		"info",		A_CANT, 0);
	
	// MSP-Level messages
	LITTER_TIMEBOMB addmess	((method) GsssDSP, "dsp", A_CANT, 0);

	//Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();
	
	}



#pragma mark -
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	GsssNew(iMu, iStdDev, iNN)
 *	
 ******************************************************************************************/

void*
GsssNew(
	double	iMu,
	double	iStdDev)
	
	{
	objGaussNoise*		me	= NIL;
	
	// Take intialization parameters as they come. An undefined StdDev is tweaked in the
	// GsssStdDev method

	// Let Max/MSP allocate us, our inlets, and outlets.
	me = (objGaussNoise*) newobject(gObjectClass);
		if (me == NIL) goto punt;
		
	dsp_setup(&(me->coreObject), 2);
	
	outlet_new(me, "signal");
	
	// Set up object components
	GsssMu(me, iMu);
	GsssStdDev(me, iStdDev);

	// All done
punt:
	return me;
	}

#pragma mark -
#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	GsssNN(me, iNN)
 *	
 *	Set parameters, making sure nothing bad happens.
 *
 ******************************************************************************************/

	void GsssMu(objGaussNoise* me, double iMu)
		{ me->mu = iMu; }
		
	void GsssStdDev(objGaussNoise* me, double iStdDev)
		{
		const double	kDefStdDev = 0.4082482905;		// sqrt(1/6)
		me->stdDev = (iStdDev == 0.0) ? kDefStdDev : fabs(iStdDev);
		}
	

void GsssFloat(
	objGaussNoise*	me,
	double			iVal)
	
	{
	
	switch ( ObjectGetInlet((Object*) me, me->coreObject.z_in) ) {
		case inletMu:
			GsssMu(me, iVal);
			break;
		case inletStdDev:
			GsssStdDev(me, iVal);
			break;
		default:
			// It can't happen here...
			break;
		}
		
	}


/******************************************************************************************
 *
 *	GsssTattle(me)
 *
 *	Post state information
 *
 ******************************************************************************************/

void
GsssTattle(
	objGaussNoise* me)
	
	{
	
	post("%s state",
			kClassName);
	post("  Mu (DC Offset) is: %f; Standard Variation is %f",
			me->mu,
			me->stdDev);
	
	}


/******************************************************************************************
 *
 *	GsssAssist()
 *	GsssInfo()
 *
 *	Fairly generic Assist/Info methods
 *
 ******************************************************************************************/

void GsssAssist(objGaussNoise* me, void* box, long iDir, long iArgNum, char* oCStr)
	{
	#pragma unused(me, box)
	
	LitterAssist(iDir, iArgNum, strIndexInLeft, strIndexOutLeft, oCStr);
	}

void GsssInfo(objGaussNoise* me)
	{ LitterInfo(kClassName, &me->coreObject.z_ob, (method) GsssTattle); }



#pragma mark -
#pragma mark • DSP Methods

/******************************************************************************************
 *
 *	GsssDSP(me, ioDSPVectors, iConnectCounts)
 *
 ******************************************************************************************/

void
GsssDSP(
	objGaussNoise*	me,
	t_signal**		ioDSPVectors,
	short*			iConnectCounts)
	
	{
	
	// Can a signal vector have an odd size?
	// Not very likely. Since the creation of MSP (and presumably all of its forerunners),
	// Vector size has always been a positive power of two. All FFT objects will rely on
	// this, and David Zicarelli has indicated that an odd vector size is extremely unlikely.
	// So, I won't bloat the code with a check here. But we may regret this some day.
	
	if (iConnectCounts[inletMu] == 0 && iConnectCounts[inletStdDev] == 0)
			dsp_add(
				GsssPerformStat, 3,
				me, (long) ioDSPVectors[outletNoise]->s_n,
				ioDSPVectors[outletNoise]->s_vec
				);
		
	else	dsp_add(
				GsssPerformDyn, 5,
				me, (long) ioDSPVectors[inletMu]->s_n,
				iConnectCounts[inletMu] > 0 ? ioDSPVectors[inletMu]->s_vec : NIL,
				iConnectCounts[inletStdDev] > 0 ? ioDSPVectors[inletStdDev]->s_vec : NIL,
				ioDSPVectors[outletNoise]->s_vec
				);
	
	}
	

/******************************************************************************************
 *
 *	GsssPerformStat(iParams)
 *	GsssPerformDyn(iParams)
 *
 *	GsssPerformStat is used when neither Gauss parameter has a signal connected.
 *	GsssPerformDyn is used if either parameters is controlled by a signal.
 *	
 *	Parameter block for GsssPerformStat contains 4 values:
 *		- Address of this function
 *		- The performing lp.gsss~ object
 *		- Vector size
 *		- output signal
 *
 *	Parameter block for GsssPerformStat contains 6 values:
 *		- Address of this function
 *		- The performing lp.gsss~ object
 *		- Vector size
 *		- Signal vector for mu (may be NIL)
 *		- Signal vector for StdDev (may be NIL)
 *		- output signal
 *
 ******************************************************************************************/

int*
GsssPerformStat(
	int iParams[])
	
	{
	enum {
		paramFuncAddress	= 0,
		paramMe,
		paramVectorSize,
		paramOut,
		
		paramNextLink
		};
	
	double			mu,
					stdDev;
	long			vecCounter;
	tSampleVector	outNoise;
	objGaussNoise*	me = (objGaussNoise*) iParams[paramMe];
	
	if (me->coreObject.z_disabled) goto exit;
	
	// Copy parameters into registers
	mu			= me->mu;
	stdDev		= me->stdDev;
	vecCounter	= (long) iParams[paramVectorSize];
	outNoise	= (tSampleVector) iParams[paramOut];
	
	// Do our stuff
	if (mu == 0.0 && stdDev == 1.0)
		 do { *outNoise++ = NormalKRTaus88(NIL); } while (--vecCounter > 0);
	else do { *outNoise++ = stdDev * NormalKRTaus88(NIL) + mu; } while (--vecCounter > 0);
		
exit:
	return iParams + paramNextLink;
	}

int*
GsssPerformDyn(
	int iParams[])
	
	{
	enum {
		paramFuncAddress	= 0,
		paramMe,
		paramVectorSize,
		paramInMu,
		paramInStdDev,
		paramOut,
		
		paramNextLink
		};
		
	long			vecCounter;
	tSampleVector	muSig,
					sdSig,
					outNoise;
	objGaussNoise*	me = (objGaussNoise*) iParams[paramMe];
	
	if (me->coreObject.z_disabled) goto exit;
	
	// Copy parameters into registers
	vecCounter	= (long) iParams[paramVectorSize];
	muSig		= (tSampleVector) iParams[paramInMu];
	sdSig		= (tSampleVector) iParams[paramInStdDev];
	outNoise	= (tSampleVector) iParams[paramOut];
	
	// ASSERT: vecCount is even.
	if (muSig && sdSig) do {
		*outNoise++ = *sdSig++ * NormalKRTaus88(NIL) + *muSig++;
		} while (--vecCounter > 0);
		
	else if (muSig) {
		// No signal for Std. Dev.
		double stdDev = me->stdDev;
		do	{
			*outNoise++ = stdDev * NormalKRTaus88(NIL) + *muSig++;
			} while (--vecCounter > 0);
		}
	else {
		// ASSERT: no signal for µ, but there must be one for Std.Dev.
		double mu = me->mu;
		do	{
			*outNoise++ = *sdSig++ * NormalKRTaus88(NIL) + mu;
			} while (--vecCounter > 0);
		}
	
exit:
	return iParams + paramNextLink;
	
	}