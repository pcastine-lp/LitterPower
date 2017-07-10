/*
	File:		1bit~.c

	Contains:	One-bit noise

	Written by:	Peter Castine

	Copyright:	© 2004 Peter Castine

	Change History (most recent first):

         <4>   26–4–2006    pc      Update for new LitterLib organization.
         <3>     7–11–04    pc      Support integer input (I forgot about Max' braindead handling of
                                    integer input to float inlets).
         <2>     5–11–04    pc      Change default amplitude to 1/sqrt(3), to produce the same power
                                    as a plain-vanilla white noise object.
         <1>    14–10–04    pc      first checked in.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "LitterLib.h"									// Also #includes MaxUtils.h, ext.h
#include "TrialPeriodUtils.h"
#include "Taus88.h"


#pragma mark • Constants

const char*		kClassName	= "lp.feta~";		// Class name

const double	kDefAmp		= 0.5773502692;		// 1/sqrt(3)
												// Produces equal power as plain-vanilla
												// white noise

	// Indices for STR# resource
enum {
		// Inlets
	strIndexInAmp		= lpStrIndexLastStandard + 1,
	
		// Outlets
	strIndexOutOneBit,
	
		// Offsets for assiststring()
	strIndexInLeft		= strIndexInAmp,
	strIndexOutLeft		= strIndexOutOneBit
	};

#pragma mark • Type Definitions



#pragma mark • Object Structure

typedef struct {
	t_pxobject		coreObject;
	
	float			amp;
	} tOneBit;


#pragma mark • Global Variables



#pragma mark • Function Prototypes

	// Class message functions
void*	FetaNew(double);

	// Object message functions
static void FetaInt(tOneBit*, long);
static void FetaAmp(tOneBit*, double);
static void FetaDefAmp(tOneBit*);
static void FetaTattle(tOneBit*);
static void	FetaAssist(tOneBit*, void* , long , long , char*);
static void	FetaInfo(tOneBit*);

	// MSP Messages
static void	FetaDSP(tOneBit*, t_signal**, short*);
static int*	FetaPerformSimp(int*);
static int*	FetaPerformMod(int*);


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
			(method) FetaNew,						// Instance creation function
			(method) dsp_free,						// Default deallocation function
			sizeof(tOneBit),						// Class object size
			NIL,									// No menu function
			A_DEFFLOAT,								// Optional argument: Amplitude
			0);
			
	dsp_initclass();

	// Messages
	addint	((method) FetaInt);
	addfloat((method) FetaAmp);
	addmess	((method) FetaDefAmp,	"defamp",	A_NOTHING);
	addmess	((method) FetaTattle,	"dblclick",	A_CANT, 0);
	addmess	((method) FetaTattle,	"tattle",	A_NOTHING);
	addmess	((method) FetaAssist,	"assist",	A_CANT, 0);
	addmess	((method) FetaInfo,		"info",		A_CANT, 0);
	
	// MSP-Level messages
	LITTER_TIMEBOMB addmess((method) FetaDSP, "dsp", A_CANT, 0);

	//Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();
	
	}



#pragma mark -
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	FetaNew(iAmp)
 *	
 ******************************************************************************************/

void*
FetaNew(
	double iAmp)
	
	{
	
	tOneBit* me	= NIL;
	
	// Cheesy test for default amplitude
	if (iAmp == 0.0)
		iAmp = kDefAmp;
	
	// Let Max/MSP allocate us, our inlets, and outlets.
	me = (tOneBit*) newobject(gObjectClass);
	dsp_setup(&(me->coreObject), 1);				// Signal inlet for benefit of begin~
	
	outlet_new(me, "signal");
	
	me->amp = iAmp;
	
	return me;
	}

#pragma mark -
#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	FetaAmp(me, iAmp)
 *	FetaDefAmp(me)
 *
 *	FetaInt(me, iAmp)
 *
 ******************************************************************************************/

void FetaAmp(tOneBit* me, double iAmp)
	{ me->amp = iAmp; }

void FetaDefAmp(tOneBit* me)
	{ FetaAmp(me, kDefAmp); }

void FetaInt(tOneBit* me, long iAmp)
	{ FetaAmp(me, (double) iAmp); }

/******************************************************************************************
 *
 *	FetaTattle(me)
 *
 *	Fairly generic Assist/Info methods
 *
 ******************************************************************************************/

void
FetaTattle(
	tOneBit* me)
	
	{
	
	post("%s state:", kClassName);
	post("  amplitude %lf", me->amp);
	
	}


/******************************************************************************************
 *
 *	FetaAssist
 *	FetaInfo(me)
 *
 *	Fairly generic Assist/Info methods
 *
 ******************************************************************************************/

void FetaAssist(tOneBit* me, void* box, long iDir, long iArgNum, char* oCStr)
	{
	#pragma unused(me, box)
	
	LitterAssist(iDir, iArgNum, strIndexInLeft, strIndexOutLeft, oCStr);
	}

void FetaInfo(tOneBit* me)
	{
	#pragma unused(me)
	
	LitterInfo(kClassName, NIL, NIL);
	}

#pragma mark -
#pragma mark • DSP Methods

/******************************************************************************************
 *
 *	FetaDSP(me, ioDSPVectors, iConnectCounts)
 *
 ******************************************************************************************/

void
FetaDSP(
	tOneBit*	me,
	t_signal**	ioDSPVectors,
	short*		iConnectCounts)
	
	{
	// Indices for signal inlets & outlets
	enum {
		inletAmp		= 0,
		outletNoise
		};
	
	if (iConnectCounts[inletAmp] > 0)
		dsp_add(
			FetaPerformMod, 4,
			me, (long) ioDSPVectors[inletAmp]->s_n,
			ioDSPVectors[inletAmp]->s_vec,
			ioDSPVectors[outletNoise]->s_vec
			);
	
	else dsp_add(
			FetaPerformSimp, 3,
			me, (long) ioDSPVectors[outletNoise]->s_n, ioDSPVectors[outletNoise]->s_vec
			);
	
	}
	

/******************************************************************************************
 *
 *	FetaPerformMod(iParams)
 *	FetaPerformMod(iParams)
 *
 *	Parameter block for FetaPerformMod contains 5 values:
 *		- Address of this function
 *		- The performing feta~ object
 *		- Vector size
 *		- Input signal
 *		- Output signal
 *
 *	Parameter block for FetaPerformSimp contains 4 values:
 *		- Address of this function
 *		- The performing feta~ object
 *		- Vector size
 *		- Output signal
 *
 ******************************************************************************************/

int*
FetaPerformMod(
	int iParams[])
	
	{
	enum {
		paramFuncAddress	= 0,
		paramMe,
		paramVectorSize,
		paramAmp,
		paramOut,
		
		paramNextLink
		};
		
	const int kRandBitCount = 32;
	
	long			vecCounter,
					randBits;
	tSampleVector	inAmp,
					outNoise;
	tOneBit*		me = (tOneBit*) iParams[paramMe];
	
	if (me->coreObject.z_disabled) goto exit;
	
	// Copy parameters into registers
	vecCounter	= (long) iParams[paramVectorSize];
	inAmp		= (tSampleVector) iParams[paramAmp];
	outNoise	= (tSampleVector) iParams[paramOut];
	
	// Do our stuff
	// We rely on vecCounter being a power of 2, which means that either
	// a single 32-bit random value will sort us out or we will use some
	// exact integral multiple of 32-bits
	if (vecCounter < kRandBitCount) {
		randBits = Taus88(NIL);
		
		do {
			*outNoise++ = (randBits < 0) ? -(*inAmp++) : *inAmp++;
			randBits <<= 1;
			} while (vecCounter-- > 0);
		}
	
	else while (vecCounter > 0) {
		int i = kRandBitCount;
		
		randBits = Taus88(NIL);
		do {
			*outNoise++ = (randBits < 0) ? -(*inAmp++) : *inAmp++;
			randBits <<= 1;
			} while (--i > 0);
		
		vecCounter -= kRandBitCount;
		}
	
exit:
	return iParams + paramNextLink;
	}


int*
FetaPerformSimp(
	int iParams[])
	
	{
	enum {
		paramFuncAddress	= 0,
		paramMe,
		paramVectorSize,
		paramOut,
		
		paramNextLink
		};
		
	const int kRandBitCount = 32;
	
	long			vecCounter,
					randBits;
	float			amp, negAmp;
	tSampleVector	outNoise;
	tOneBit*		me = (tOneBit*) iParams[paramMe];
	
	if (me->coreObject.z_disabled) goto exit;
	
	// Copy parameters into registers
	vecCounter	= (long) iParams[paramVectorSize];
	outNoise	= (tSampleVector) iParams[paramOut];
	amp			= me->amp;
	negAmp		= -amp;
	
	// Do our stuff
	// We rely on vecCounter being a power of 2, which means that either
	// a single 32-bit random value will sort us out or we will use some
	// exact integral multiple of 32-bits
	if (vecCounter < kRandBitCount) {
		randBits = Taus88(NIL);
		
		do {
			*outNoise++ = (randBits < 0) ? negAmp : amp;
			randBits <<= 1;
			} while (vecCounter-- > 0);
		}
	
	else while (vecCounter > 0) {
		int i = kRandBitCount;
		
		randBits = Taus88(NIL);
		do {
			*outNoise++ = (randBits < 0) ? negAmp : amp;
			randBits <<= 1;
			} while (--i > 0);
		
		vecCounter -= kRandBitCount;
		}
	
exit:
	return iParams + paramNextLink;
	}
