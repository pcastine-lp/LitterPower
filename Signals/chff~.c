/*
	File:		chff~.c

	Contains:	Pulse train with random amplitudes.

	Written by:	Peter Castine

	Copyright:	© 2003 Peter Castine

	Change History (most recent first):

		10-Sep-2003:	First implementation.
         <2>   26–4–2006    pc      General maintenance to keep up with project state, in case I
                                    decide to really follow up with this object some day.
         <1>   23–3–2006    pc      first checked in.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "LitterLib.h"	// Also #includes MaxUtils.h, ext.h
#include "Taus88.h"

#pragma mark • Constants

const char*	kClassName		= "lp.chff~";			// Class name


	// Indices for STR# resource
enum {
	strIndexInFreq		= lpStrIndexLastStandard + 1,
	strIndexInWidth,
	strIndexInPhase,
	strIndexInThresh,
	
	strIndexOutTrain,
	strIndexOutBang,
	
	strIndexInLeft		= strIndexInFreq,
	strIndexOutLeft		= strIndexOutTrain
	};

enum freqMode {
	freqMS,
	freqHz,
	freqSamps
	};

enum dutyState {
	dutyOff	= false,
	dutyOn	= true
	};

#pragma mark • Type Definitions

typedef enum freqMode	eFreqMode;
typedef enum dutyState	eDutyState;

#pragma mark • Object Structure

typedef struct {
	t_pxobject	coreObject;
	
	tOutletPtr	bangOut;
	
	double		curSR,
				freqMS,					// Pulse freq in ms
				freqHz,					// Pulse freq in Hz
				freqSamps,				// Pulse freq in number of samples (incl. fraction)
				dutyPercent,
				dutySamps,
				offsetPercent,
				offsetSamps,
				thresh,
				curAmp;					// Recalc once per cycle
	
	eFreqMode	freqMode;
	eDutyState	dState;					// "true" means Duty cycle on
	long		sampsToGo;
	} objRandTrain;


#pragma mark • Global Variables



#pragma mark • Function Prototypes

	// Class message functions
void*	ChffNew(double, double, double, double);

	// Object message functions
static void ChffFloat(objRandTrain*, double);
static void ChffFreqMS(objRandTrain*, double);
static void ChffFreqHz(objRandTrain*, double);
static void ChffFreqSamps(objRandTrain*, double);
static void ChffWidth(objRandTrain*, double);
static void ChffPhase(objRandTrain*, double);
static void ChffThresh(objRandTrain*, double);
static void ChffTattle(objRandTrain*);
static void	ChffAssist(objRandTrain*, void* , long , long , char*);
static void	ChffInfo(objRandTrain*);

	// MSP Messages
static void	ChffDSP(objRandTrain*, t_signal**, short*);
static int*	ChffPerformStat(int*);
static int*	ChffPerformDyn(int*);


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
			(method) ChffNew,			// Instance creation function
			(method) dsp_free,			// Default deallocation function
			sizeof(objRandTrain),		// Class object size
			NIL,						// No menu function
			A_DEFFLOAT,					// Optional arguments:	1. Pulse Frequency
			A_DEFFLOAT,					//						2. Pulse Width
			A_DEFFLOAT,					//						3. Phase Offset
			A_DEFFLOAT,					//						4. Bang threshhold
			0);		
	
	dsp_initclass();

	// Messages
	
	addfloat((method) ChffFloat);
	addmess	((method) ChffFreqMS,	"ms",		A_FLOAT, 0);	// Redundant since ms is default
																// unit for frequency
	addmess	((method) ChffFreqHz,	"hz",		A_FLOAT, 0);
	addmess	((method) ChffFreqSamps,"samps",	A_FLOAT, 0);
	addmess	((method) ChffTattle,	"dblclick",	A_CANT, 0);
	addmess	((method) ChffTattle,	"tattle",	A_NOTHING);
	addmess	((method) ChffAssist,	"assist",	A_CANT, 0);
	addmess	((method) ChffInfo,		"info",		A_CANT, 0);
	
	// MSP-Level messages
	LITTER_TIMEBOMB addmess	((method) ChffDSP,		 "dsp",		A_CANT, 0);
		
	// Initialize Litter Lib
	LitterInit(kClassName, 0);
	Taus88Init();
	
	}


#pragma mark -
#pragma mark • Utility Functions

/******************************************************************************************
 *
 *	UpdateSR(me, iSR)
 *	
 ******************************************************************************************/

static void
UpdateSR(
	objRandTrain*	me,
	double			iSR)
	
	{
	
	if (me->curSR == iSR)
		return;
	
	// ?? what do we need to do ??
	
	}



#pragma mark -
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	ChffNew(iMul, iAdd, iMod, iSeed)
 *	
 ******************************************************************************************/

void*
ChffNew(
	double	iFreq,
	double	iDuty,
	double	iPhase,
	double	iThresh)
	
	{
	const double	kDefFreq	= 100.0,
					kDefDuty	= 0.5,
					kDefThresh	= 0.5;
					// Default phase is 0.0, so we don't need to define 
	
	objRandTrain*	me	= NIL;
	
	// Run through initialization parameters from right to left, handling defaults
	if (iThresh == 0.0)  
		iThresh = kDefThresh;
	else goto noMoreDefaults;
	
		// 0 is default for iPhase
	if (iPhase != 0) goto noMoreDefaults;
	
	if (iDuty == 0.0)  
		iDuty = kDefDuty;
	else goto noMoreDefaults;
			
	if (iFreq == 0)
		iFreq = kDefFreq;
noMoreDefaults:
	// Finished checking intialization parameters

	// Let Max/MSP allocate us, our inlets (from right to left), and outlets.
	me = (objRandTrain*) newobject(gObjectClass);
	dsp_setup(&(me->coreObject), 4);				// Three inlets:
													// 		Pulse interval
													//		Pulse width
													//		Phase offset
													//		Bang threshhold
	
	
	me->bangOut = outlet_bang(me);
	outlet_new(me, "signal");						// Access via me->coreObject->o_outlet
	
	// Initialize and store object components
	me->freqMode	= freqMS;						// Set this to prevent ChffFreqMS() from
	me->curSR		= sys_getsr();					// Must check this in DSP method
	ChffFreqMS(me, iFreq);							// calculating duty/offset with bogus
													// initial values
	ChffWidth(me, iDuty);
	ChffPhase(me, iPhase);
	me->thresh		= iThresh;
	me->dState		= dutyOff;
	me->sampsToGo	= 0;

	return me;
	}

#pragma mark -
#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	ChffSet(me, iSeed)
 *	ChffMul(me, iMul)
 *	ChffAdderl(me, iMul)
 *	ChffMod(me, iMul)
 *	
 *	Nothing reallly bad can happen... we simply treat everything as unsigned. A multiplier
 *	of 0 seems a little counter-productive, but even this is allowed.
 *
 ******************************************************************************************/

void ChffFreqMS	(
	objRandTrain*	me,
	double			iFreq)
	{
	
	if (iFreq <= 0.0)
		return;
	
	me->freqMode	= freqMS;	
	me->freqMS		= iFreq;
	
	}

void ChffFreqHz	(
	objRandTrain*	me,
	double			iFreq)
	{
	
	if (iFreq <= 0.0)
		return;
	
	me->freqMode	= freqHz;	
	me->freqHz		= iFreq;
	
	}

void ChffFreqSamps(
	objRandTrain*	me,
	double			iFreq)
	{
	
	if (iFreq <= 0.0)
		return;
	
	me->freqMode	= freqSamps;	
	me->freqSamps	= iFreq;
	
	}




/******************************************************************************************
 *
 *	ChffTattle(me)
 *
 *	Post state information
 *
 ******************************************************************************************/

void
ChffTattle(
	objRandTrain* me)
	
	{
	
	post("%s state",
			kClassName);
	post("  Current sample rate: %lf", me->curSR);
	
	}


/******************************************************************************************
 *
 *	ChffAssist()
 *	ChffInfo()
 *
 *	Fairly generic Assist/Info methods
 *
 ******************************************************************************************/

void ChffAssist(objRandTrain*, void*, long iDir, long iArgNum, char* oCStr)
	{ LitterAssist(iDir, iArgNum, strIndexInLeft, strIndexOutLeft, oCStr); }

void ChffInfo(objRandTrain* me)
	{ LitterInfo(kClassName, &me->coreObject.z_ob, (method) ChffTattle); }


#pragma mark -
#pragma mark • DSP Methods

/******************************************************************************************
 *
 *	ChffDSP(me, ioDSPVectors, iConnectCounts)
 *
 ******************************************************************************************/

void
ChffDSP(
	objRandTrain*	me,
	t_signal**		ioDSPVectors,
	short*			iConnectCounts)
	
	{
	enum {
		inletFreq,
		inletWidth,
		inletPhase,
		inletThresh,
		outletTrain
		};
	
	UpdateSR(me, ioDSPVectors[outletTrain]->s_sr);
	
	if (iConnectCounts[inletFreq] == 0
			&& iConnectCounts[inletWidth] == 0
			&& iConnectCounts[inletPhase] == 0
			&& iConnectCounts[inletThresh] == 0) {
		
		dsp_add(
			ChffPerformStat, 3,
			me, (long) ioDSPVectors[outletTrain]->s_n,
			ioDSPVectors[outletTrain]->s_vec);
		}
	
	else {
		dsp_add(
			ChffPerformDyn, 7,
			me, (long) ioDSPVectors[outletTrain]->s_n,
			(iConnectCounts[inletFreq] == 0) ? NIL : ioDSPVectors[inletFreq]->s_vec,
			(iConnectCounts[inletWidth] == 0) ? NIL : ioDSPVectors[inletWidth]->s_vec,
			(iConnectCounts[inletPhase] == 0) ? NIL : ioDSPVectors[inletPhase]->s_vec,
			(iConnectCounts[inletThresh] == 0) ? NIL : ioDSPVectors[inletThresh]->s_vec,
			ioDSPVectors[outletTrain]->s_vec
			);
		
		}
	
	}
	

/******************************************************************************************
 *
 *	ChffPerform(iParams)
 *
 *	Parameter block for PerformSync contains 8 values:
 *		- Address of this function
 *		- The performing schhh~ object
 *		- Vector size
 *		- output signal
 *		- Imaginary output signal
 *
 ******************************************************************************************/

int*
ChffPerformStat(
	int* iParams)
	
	{
	enum {
		paramFuncAddress	= 0,
		paramMe,
		paramVectorSize,
		paramOut,
		
		paramNextLink
		};
	
	long			vecCounter, 
					sampsToGo;
	tSampleVector	outNoise;
	objRandTrain*	me = (objRandTrain*) iParams[paramMe];
	double			freqSamps,
					dutySamps,
					offsetSamps,
					thresh,
					curAmp;
	eDutyState		dState;
	
	if (me->coreObject.z_disabled) goto exit;
	
	// Copy parameters into registers
	vecCounter	= (UInt32) iParams[paramVectorSize];
	outNoise	= (tSampleVector) iParams[paramOut];
	freqSamps	= me->freqSamps;
	dutySamps	= me->dutySamps;
	offsetSamps	= me->offsetSamps;
	thresh		= me->thresh;
	curAmp		= me->curAmp;
	
	// Do our stuff
	do {
		
		} while (vecCounter > 0);
	
	
exit:
	return iParams + paramNextLink;
	}
