/*
	File:		schhh~.c

	Contains:	"White" noise. Based on Taus88 algorithm, also has unique NN factor.

	Written by:	Peter Castine

	Copyright:	© 2001-2002 Peter Castine

	Change History (most recent first):

         <6>   24–3–2006    pc      Update #includes for new LitterLib organization.
         <5>     14–1–04    pc      Update for Windows.
         <4>    6–7–2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <3>  30–12–2002    pc      Drop faux 'Vers' resource and related modifications. Add object
                                    version to ShhhInfo().
         <2>  28–11–2002    pc      Tidy up after initial check in.
         <1>  28–11–2002    pc      Initial check in.
		1-Jun-2001:		Use Taus88Vector and Taus88FPVector to speed things up a bit.
		14-Apr-2001:	First implementation.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "LitterLib.h"	// Also #includes MaxUtils.h, ext.h
#include "TrialPeriodUtils.h"
#include "Taus88.h"


#pragma mark • Constants

const char*	kClassName		= "lp.shhh~";			// Class name

const int	kMaxNN			= 31;

	// Indices for STR# resource
enum {
	strIndexInNN		= lpStrIndexLastStandard + 1,
	
	strIndexOutWhite
	};

	// Indices for MSP Inlets and Outlets.
enum {
	inletWhite			= 0,		// For begin~
	
	outletWhite
	};

#pragma mark • Type Definitions



#pragma mark • Object Structure

typedef struct {
	t_pxobject		coreObject;
	
	int				nn;					// Number of bits to mask out
	unsigned long	mask,				// Values depend on nn
					offset;
	} tWhite;


#pragma mark • Global Variables



#pragma mark • Function Prototypes

	// Class message functions
void*	ShhhNew(long);

	// Object message functions
static void ShhhNN(tWhite*, long);
static void ShhhTattle(tWhite*);
static void	ShhhAssist(tWhite*, void* , long , long , char*);
static void	ShhhInfo(tWhite*);

	// MSP Messages
static void	ShhhDSP(tWhite*, t_signal**, short*);
static int*	ShhhPerform(int*);


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
			(method) ShhhNew,			// Instance creation function
			(method) dsp_free,			// Default deallocation function
			sizeof(tWhite),				// Class object size
			NIL,						// No menu function
			A_DEFLONG,					// Optional arguments:	1. NN Factor
			0);		
	
	dsp_initclass();

	// Messages
	addint	((method) ShhhNN);
	addmess	((method) ShhhTattle,	"dblclick",	A_CANT, 0);
	addmess	((method) ShhhTattle,	"tattle",	A_NOTHING);
	addmess	((method) ShhhAssist,	"assist",	A_CANT, 0);
	addmess	((method) ShhhInfo,		"info",		A_CANT, 0);
	
	// MSP-Level messages
	LITTER_TIMEBOMB addmess	((method) ShhhDSP,		"dsp",		A_CANT, 0);

	//Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();
	
	}



#pragma mark -
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	ShhhNew(iNN)
 *	
 ******************************************************************************************/

void*
ShhhNew(
	long	iNN)
	
	{
	tWhite*		me	= NIL;
	
	// Default NN value doesn't need massaging

	// Let Max/MSP allocate us, our inlets, and outlets.
	me = (tWhite*) newobject(gObjectClass);
	dsp_setup(&(me->coreObject), 1);				// Signal inlet for benefit of begin~
													// Otherwise left inlet does "NN" only
	outlet_new(me, "signal");
	
	// Set up object components
	ShhhNN(me, iNN);

	return me;
	}

#pragma mark -
#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	ShhhNN(me, iNN)
 *	
 *	Set parameters, making sure nothing bad happens.
 *
 ******************************************************************************************/

void ShhhNN(
	tWhite* me,
	long	iNN)
	
	{
	
	if (iNN <= 0) {
		me->nn		= 0;
		me->mask	= kULongMax;
		me->offset	= 0;
		}
	else {
		if (iNN > kMaxNN)
			iNN = kMaxNN;
		me->nn		= iNN;
		me->mask	= kULongMax << iNN;
		me->offset	= (~me->mask) >> 1;
		}
	
	}


/******************************************************************************************
 *
 *	ShhhTattle(me)
 *
 *	Post state information
 *
 ******************************************************************************************/

void
ShhhTattle(
	tWhite* me)
	
	{
	
	post(	"%s state",
			kClassName);
	post(	"  NN factor is: %d (mask = %lx)",
			me->nn,
			me->mask);
	
	}


/******************************************************************************************
 *
 *	ShhhAssist
 *	ShhhInfo(me)
 *
 *	Fairly generic Assist/Info methods
 *
 ******************************************************************************************/

void ShhhAssist(tWhite* me, void* box, long iDir, long iArgNum, char* oCStr)
	{
	#pragma unused(me, box)
	
	LitterAssist(iDir, iArgNum, strIndexInNN, strIndexOutWhite, oCStr);
	}

void ShhhInfo(tWhite* me)
	{
	#pragma unused(me)
	
	LitterInfo(kClassName, NIL, NIL);
	}

#pragma mark -
#pragma mark • DSP Methods

/******************************************************************************************
 *
 *	ShhhDSP(me, ioDSPVectors, iConnectCounts)
 *
 ******************************************************************************************/

void
ShhhDSP(
	tWhite*		me,
	t_signal**	ioDSPVectors,
	short*		connectCounts)
	
	{
	#pragma unused(connectCounts)
	
	dsp_add(
		ShhhPerform, 3,
		me, (long) ioDSPVectors[outletWhite]->s_n, ioDSPVectors[outletWhite]->s_vec
		);
	
	}
	

/******************************************************************************************
 *
 *	ShhhPerform(iParams)
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
ShhhPerform(
	int iParams[])
	
	{
	enum {
		paramFuncAddress	= 0,
		paramMe,
		paramVectorSize,
		paramOut,
		
		paramNextLink
		};
	
	long			vecCounter;
	tSampleVector	outNoise;
	tWhite*			me = (tWhite*) iParams[paramMe];
	
	if (me->coreObject.z_disabled) goto exit;
	
	// Copy parameters into registers
	vecCounter	= (long) iParams[paramVectorSize];
	outNoise	= (tSampleVector) iParams[paramOut];
	
	// Do our stuff
	if (me->nn == 0)
			Taus88SigVector(outNoise, vecCounter);
	else	Taus88SigVectorMasked(outNoise, vecCounter, me->mask, me->offset);
	
exit:
	return iParams + paramNextLink;
	}
