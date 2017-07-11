/*
	File:		grrr~.c

	Contains:	"Gray" noise. Based on Taus88 algorithm, also has unique NN factor.

	Written by:	Peter Castine

	Copyright:	© 2001-2002 Peter Castine

	Change History (most recent first):

         <7>   24–3–2006    pc      Update #includes for new LitterLib organization.
         <6>     14–1–04    pc      Update for Windows.
         <5>    6–7–2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <4>    1–1–2003    pc      Correct minor problem with "tattle" strings.
         <4>    1–1–2003    pc      Correct minor problem with "tattle" strings.
         <3>  30–12–2002    pc      Drop faux 'Vers' resource and related modifications. Add object
                                    version to GrrrInfo().
         <2>  28–11–2002    pc      Tidy up after initial check in.
         <1>  28–11–2002    pc      Initial check in.
		14-Apr-2000:	First implementation.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "LitterLib.h"	// Also #includes MaxUtils.h, ext.h
#include "TrialPeriodUtils.h"
#include "Taus88.h"
#include "MiscUtils.h"



#pragma mark • Constants

const char*	kClassName		= "lp.grrr~";			// Class name

const int	kMaxNN			= 31;

	// Indices for STR# resource
enum {
	strIndexInNN		= lpStrIndexLastStandard + 1,
	
	strIndexOutGray
	};

	// Indices for MSP Inlets and Outlets.
enum {
	inletGray			= 0,		// For begin~
	
	outletGray
	};

#pragma mark • Type Definitions



#pragma mark • Object Structure

typedef struct {
	t_pxobject		coreObject;
	
	unsigned long	prev;
	} tGray;


#pragma mark • Global Variables



#pragma mark • Function Prototypes

	// Class message functions
void*	GrrrNew(void);

	// Object message functions
static void GrrrTattle(tGray*);
static void	GrrrAssist(tGray*, void* , long , long , char*);
static void	GrrrInfo(tGray*);

	// MSP Messages
static void	GrrrDSP(tGray*, t_signal**, short*);
static int*	GrrrPerform(int*);


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
			(method) GrrrNew,			// Instance creation function
			(method) dsp_free,			// Default deallocation function
			sizeof(tGray),				// Class object size
			NIL,						// No menu function
			A_NOTHING);					// Not even any arguments.
	
	dsp_initclass();

	// Messages
	addmess	((method) GrrrTattle,		"dblclick",	A_CANT, 0);
	addmess	((method) GrrrTattle,		"tattle",	A_NOTHING);
	addmess	((method) GrrrAssist,		"assist",	A_CANT, 0);
	addmess	((method) GrrrInfo,		"info",		A_CANT, 0);
	
	// MSP-Level messages
	LITTER_TIMEBOMB addmess	((method) GrrrDSP, "dsp",		A_CANT, 0);

	//Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();
	
	}



#pragma mark -
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	GrrrNew()
 *	
 ******************************************************************************************/

void*
GrrrNew(void)
	
	{
	tGray*		me			= NIL;
	
	// Let Max/MSP allocate us, our inlets, and outlets.
	me = (tGray*) newobject(gObjectClass);
	dsp_setup(&(me->coreObject), 1);				// Signal inlet for benefit of begin~
													// Otherwise left inlet does "NN" only
	outlet_new(me, "signal");
	
	// Set up object components
	me->prev = 0;									// Could actually be anything, but this
													// way the signal shouldn't click on start.
	
	return me;
	}

#pragma mark -
#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	GrrrTattle(me)
 *
 *	Post state information
 *
 ******************************************************************************************/

void
GrrrTattle(
	tGray* me)
	
	{
	
	post("%s state", kClassName);
	post("   last value: %lu", me->prev);
	
	}


/******************************************************************************************
 *
 *	GrrrAssist()
 *	GrrrInfo()
 *
 *	Fairly generic Assist/Info methods
 *
 ******************************************************************************************/

void GrrrAssist(tGray* me, void* box, long iDir, long iArgNum, char* oCStr)
	{
	#pragma unused(me, box)
	
	LitterAssist(iDir, iArgNum, strIndexInNN, strIndexOutGray, oCStr);
	}

void GrrrInfo(tGray* me)
	{ LitterInfo(kClassName, &me->coreObject.z_ob, (method) GrrrTattle); }



#pragma mark -
#pragma mark • DSP Methods

/******************************************************************************************
 *
 *	GrrrDSP(me, ioDSPVectors, iConnectCounts)
 *
 ******************************************************************************************/

void
GrrrDSP(
	tGray*		me,
	t_signal**	ioDSPVectors,
	short		connectCounts[])
	
	{
	#pragma unused(connectCounts)
	
	dsp_add(
		GrrrPerform, 3,
		me, (long) ioDSPVectors[outletGray]->s_n, ioDSPVectors[outletGray]->s_vec
		);
	
	}
	

/******************************************************************************************
 *
 *	GrrrPerform(iParams)
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
GrrrPerform(
	int* iParams)
	
	{
	enum {
		paramFuncAddress	= 0,
		paramMe,
		paramVectorSize,
		paramOut,
		
		paramNextLink
		};
	
	unsigned long	vecCounter,
					prev;	
	tSampleVector	outNoise;
	tGray*			me = (tGray*) iParams[paramMe];
	
	if (me->coreObject.z_disabled) goto exit;
	
	// Copy parameters into registers
	vecCounter	= (long) iParams[paramVectorSize];
	outNoise	= (tSampleVector) iParams[paramOut];
	prev		= me->prev;
	
	// Do our stuff
	do {
		prev ^= Taus88(NULL);
		*outNoise++ = ULong2Signal(prev);
		} while (--vecCounter > 0);
	
exit:
	return iParams + paramNextLink;
	}
