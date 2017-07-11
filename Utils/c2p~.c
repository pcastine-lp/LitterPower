/*
	File:		c2p~.c

	Contains:	Max external object implementing Larry Polansky's Interval Mutation
				algorithms.

	Written by:	Peter Castine

	Copyright:	© 2000-01 Peter Castine

	Change History (most recent first):

         <7>     11–1–04    pc      Update for modified LitterInit()
         <6>      8–1–04    pc      Update for Windows.
         <5>    6–7–2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <4>  30–12–2002    pc      Add object version to C2PInfo()
         <3>  30–12–2002    pc      Use 'STR#' resource instead of faux 'Vers' resource for storing
                                    version information used at run-time.
         <2>  28–11–2002    pc      Tidy up after initial check in.
         <1>  28–11–2002    pc      Initial Check-In.
		24-Jun-2001:	Adapted for inclusion in Litter Power Package
		15-Nov-2000:	First implementation (based on previous Mutator~.c)
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include <stdlib.h>		// For rand(), RAND_MAX
#include <math.h>		// For pow()

#include "LitterLib.h"
#include "TrialPeriodUtils.h"


#pragma mark • Global Constants

const char*	kClassName		= "lp.c2p~";				// Class name

	// Indices for STR# resource
enum {
	strIndexInReal		= lpStrIndexLastStandard + 1,
	strIndexInImag,
	
	strIndexOutAmp,
	strIndexOutPhase
	};

	// Indices for Inlets and Outlets. Users count starting at 1, we count from 0
	// 
enum {
	inletReal		= 0,
	inletImag,
	
	outletAmp,
	outletPhase
	};


#pragma mark • Type Definitions

#pragma mark • Object Structure

typedef struct {
	t_pxobject	coreObject;
	} tConverter;


#pragma mark • Global Variables

#pragma mark • Function Prototypes

static void*	C2PNew(void);
static void		C2PDSP(tConverter*, t_signal**, short*);
static int*		C2PPerform(int*);

	// Various Max messages
static void		C2PAssist(tConverter*, void* , long , long , char*);
static void		C2PInfo(tConverter*);


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
			(method) C2PNew,				// Instance creation function
			(method) dsp_free,					// Default deallocation function
			(short) sizeof(tConverter),			// Class object size
			NIL,								// No menu function
			A_NOTHING);							// No arguments
	
	// Max/MSP class initialization mantra, including registering all aliases
	dsp_initclass();
	
	// Fairly standard Max messages
	addmess	((method) C2PAssist,	"assist",	A_CANT, 0);
	addmess	((method) C2PInfo,		"info",		A_CANT, 0);
	
	// MSP-Level messages
	LITTER_TIMEBOMB addmess	((method) C2PDSP, "dsp", A_CANT, 0);

	// Initialize Litter Library
	LitterInit(kClassName, 0);

	}

#pragma mark -
#pragma mark • Methods

/******************************************************************************************
 *
 *	C2PNew()
 *
 ******************************************************************************************/

void*
C2PNew(void)
	
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
 *	C2PAssist
 *	C2PInfo(me)
 *
 *	Fairly generic implementations
 *
 ******************************************************************************************/

void C2PAssist(tConverter* me, void* box, long iDir, long iArgNum, char* oCStr)
	{
	#pragma unused(me, box)
	
	LitterAssist(iDir, iArgNum, strIndexInReal, strIndexOutAmp, oCStr);
	}

void C2PInfo(tConverter* me)
	{
	#pragma unused(me)
	
	LitterInfo(kClassName, NIL, NIL);
	}

	

#pragma mark -
#pragma mark • DSP Methods

/******************************************************************************************
 *
 *	C2PDSP(me, ioDSPVectors, iConnectCounts)
 *
 ******************************************************************************************/

void
C2PDSP(
	tConverter*	me,
	t_signal**	ioDSPVectors,
	short*		iConnectCounts)
	
	{
	
	dsp_add(
		C2PPerform, 6,
		me,
		(long) ioDSPVectors[inletReal]->s_n,
		iConnectCounts[inletReal] ? ioDSPVectors[inletReal]->s_vec : NIL,
		iConnectCounts[inletImag] ? ioDSPVectors[inletImag]->s_vec : NIL,
		iConnectCounts[outletAmp] ? ioDSPVectors[outletAmp]->s_vec : NIL,
		iConnectCounts[outletPhase] ? ioDSPVectors[outletPhase]->s_vec : NIL
		);

	}
	

/******************************************************************************************
 *
 *	C2PPerform(iParams)
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
C2PPerform(
	int* iParams)
	
	{
	enum {
		paramFuncAddress	= 0,
		paramMe,
		paramVectorSize,
		paramReal,
		paramImag,
		paramAmp,
		paramPhase,
		
		paramNextLink
		};
	
	long			vecCounter;
	tSampleVector	realSig,
					imagSig,
					ampSig,
					phaseSig;
	tConverter*		me = (tConverter*) iParams[paramMe];
	
	if (me->coreObject.z_disabled) goto exit;
	
	vecCounter	= (long) iParams[paramVectorSize];
	realSig		= (tSampleVector) iParams[paramReal];
	imagSig		= (tSampleVector) iParams[paramImag];
	ampSig		= (tSampleVector) iParams[paramAmp];
	phaseSig	= (tSampleVector) iParams[paramPhase];
	
	do {
		double	real	= realSig ? *realSig++ : 0.0,
				imag	= imagSig ? *imagSig++ : 0.0,
				amp		= sqrt(real * real + imag * imag);
				
		if (ampSig)		*ampSig++	= amp;
		if (phaseSig)	*phaseSig++	= (amp > 0.0)
										? atan2(imag, real)
										: 0.0;				// Very arbitrary value
		} while (--vecCounter > 0);
		
exit:
	return iParams + paramNextLink;
	}

