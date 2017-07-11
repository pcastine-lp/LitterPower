/*
	File:		p2c~.c

	Contains:	Convert polar to Cartesian representation of signals.

	Written by:	Peter Castine

	Copyright:	© 2000-01 Peter Castine

	Change History (most recent first):

         <7>     11–1–04    pc      Update for modified LitterInit()
         <6>      8–1–04    pc      Update for Windows.
         <5>    6–7–2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <4>  30–12–2002    pc      Add object version to P2CInfo()
         <3>  30–12–2002    pc      Use 'STR#' resource instead of faux 'Vers' resource for storing
                                    version information used at run-time.
         <2>  28–11–2002    pc      Tidy up after initial check in.
         <1>  28–11–2002    pc      Initial check in.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include <stdlib.h>		// For rand(), RAND_MAX
#include <math.h>		// For pow()

#include "LitterLib.h"
#include "TrialPeriodUtils.h"


#pragma mark • Global Constants

const char*	kClassName		= "lp.p2c~";				// Class name

	// Indices for STR# resource
enum {
	strIndexInAmp		= lpStrIndexLastStandard + 1,
	strIndexInPhase,
	
	strIndexOutReal,
	strIndexOutImag
	};

	// Indices for Inlets and Outlets. Users count starting at 1, we count from 0
	// 
enum {
	inletAmp		= 0,
	inletPhase,
	
	outletReal,
	outletImag
	};


#pragma mark • Type Definitions

#pragma mark • Object Structure

typedef struct {
	t_pxobject	coreObject;
	} tConverter;


#pragma mark • Global Variables

#pragma mark • Function Prototypes

static void*	P2CNew(void);
static void	P2CDSP(tConverter*, t_signal**, short*);
static int*	P2CPerform(int*);

	// Various Max messages
static void	P2CAssist(tConverter*, void* , long , long , char*);
static void	P2CInfo(tConverter*);


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
	setup(	&gObjectClass,					// Pointer to our class definition
			(method) P2CNew,				// Instance creation function
			(method) dsp_free,				// Default deallocation function
			(short) sizeof(tConverter),		// Class object size
			NIL,							// No menu function
			A_NOTHING);						// No arguments
	
	// Max/MSP class initialization mantra, including registering all aliases
	dsp_initclass();
	
	// Fairly standard Max messages
	addmess	((method) P2CAssist,	"assist",	A_CANT, 0);
	addmess	((method) P2CInfo,		"info",		A_CANT, 0);
	
	// MSP-Level messages
	LITTER_TIMEBOMB addmess((method) P2CDSP, "dsp", A_CANT, 0);
	
	// Initialize Litter Library
	LitterInit(kClassName, 0);
	
	}

#pragma mark -
#pragma mark • Methods

/******************************************************************************************
 *
 *	P2CNew()
 *
 ******************************************************************************************/

void*
P2CNew(void)
	
	{
	tConverter*	me	= NIL;
	
	// Let Max/MSP allocate us and give us inlets;
	me = (tConverter*) newobject(gObjectClass);
	dsp_setup(&(me->coreObject), 2);
	
	// And two outlets, set these up right-to-left
	outlet_new(me, "signal");			
	outlet_new(me, "signal");			
	
	return me;
	}

#pragma mark -
#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	P2CAssist
 *	P2CInfo(me)
 *
 ******************************************************************************************/

void P2CAssist(tConverter* me, void* box, long iDir, long iArgNum, char oCDestStr[])
	{
	#pragma unused(me, box)
	
	LitterAssist(iDir, iArgNum, strIndexInAmp, strIndexOutReal, oCDestStr);
	}

void P2CInfo(tConverter* me)
	{
	#pragma unused(me)
	
	LitterInfo(kClassName, NIL, NIL);
	}

	

#pragma mark -
#pragma mark • DSP Methods

/******************************************************************************************
 *
 *	P2CDSP(me, ioDSPVectors, iConnectCounts)
 *
 ******************************************************************************************/

void
P2CDSP(
	tConverter*	me,
	t_signal**	ioDSPVectors,
	short*		iConnectCounts)
	
	{
	
	dsp_add(
		P2CPerform, 6,
		me,
		(long) ioDSPVectors[inletAmp]->s_n,
		iConnectCounts[inletAmp] ? ioDSPVectors[inletAmp]->s_vec : NIL,
		iConnectCounts[inletPhase] ? ioDSPVectors[inletPhase]->s_vec : NIL,
		iConnectCounts[outletReal] ? ioDSPVectors[outletReal]->s_vec : NIL,
		iConnectCounts[outletImag] ? ioDSPVectors[outletImag]->s_vec : NIL
		);

	}
	

/******************************************************************************************
 *
 *	P2CPerform(iParams)
 *
 *	Parameter block contains 7 values:
 *		- Address of this function
 *		- The performing Converter object
 *		- Current signal vector size
 *		- Real signal
 *		- Imaginary signal
 *		- Amplitude signal (out)
 *		- Phase signal (out)
 *
 ******************************************************************************************/

int*
P2CPerform(
	int* iParams)
	
	{
	enum {
		paramFuncAddress	= 0,
		paramMe,
		paramVectorSize,
		paramAmp,
		paramPhase,
		paramImag,
		paramReal,
		
		paramNextLink
		};
	
	long			vecCounter;
	tSampleVector	ampSig,
					phaseSig,
					realSig,
					imagSig;
	tConverter*		me = (tConverter*) iParams[paramMe];
	
	if (me->coreObject.z_disabled) goto exit;
	
	vecCounter	= (long) iParams[paramVectorSize];
	ampSig		= (tSampleVector) iParams[paramAmp];
	phaseSig	= (tSampleVector) iParams[paramPhase];
	realSig		= (tSampleVector) iParams[paramReal];
	imagSig		= (tSampleVector) iParams[paramImag];
	
	do {
		double	amp		= ampSig	? *ampSig++		: 0.0,
				phase	= phaseSig	? *phaseSig++	: 0.0;
				
		if (realSig) *realSig++	= amp * sin(phase);
		if (imagSig) *imagSig++	= amp * cos(phase);
		} while (--vecCounter > 0);
		
exit:
	return iParams + paramNextLink;
	}

