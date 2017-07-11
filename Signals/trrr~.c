/*
	File:		trrr~.c

	Contains:	Triangular and linear noise. Has unique NN factor.

	Written by:	Peter Castine

	Copyright:	© 2002 Peter Castine

	Change History (most recent first):

         <6>   26–4–2006    pc      Update for new LitterLib organization.
         <5>     14–1–04    pc      Update for Windows.
         <4>    6–7–2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <3>  30–12–2002    pc      Drop faux 'Vers' resource and related modifications. Add object
                                    version to TrrrInfo().
         <2>  28–11–2002    pc      Tidy up after initial check in.
         <1>  28–11–2002    pc      Initial check in.
		30-Jan-2002:	First implementation.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "LitterLib.h"	// Also #includes MaxUtils.h, ext.h
#include "TrialPeriodUtils.h"
#include "MiscUtils.h"
#include "Taus88.h"


#pragma mark • Constants

const char*	kClassName		= "lp.trrr~";			// Class name

const int	kMaxNN			= 31;

	// Indices for STR# resource
enum {
	strIndexInNN		= lpStrIndexLastStandard + 1,
	strIndexOutTri,
	
	strIndexInLeft		= strIndexInNN,
	strIndexOutLeft		= strIndexOutTri
	};

	// Indices for MSP Inlets and Outlets.
enum {
	inletTriNoise			= 0,		// For begin~
	
	outletTriNoise
	};

#pragma mark • Type Definitions



#pragma mark • Object Structure

typedef struct {
	t_pxobject		coreObject;
	
	int				nn;					// Number of bits to mask out
	unsigned long	mask,				// Values depend on nn
					offset;
	} tTriNoise;


#pragma mark • Global Variables


#pragma mark • Function Prototypes

	// Class message functions
void*	TrrrNew(long);

	// Object message functions
static void TrrrNN(tTriNoise*, long);
static void TrrrTattle(tTriNoise*);
static void	TrrrAssist(tTriNoise*, void* , long , long , char*);
static void	TrrrInfo(tTriNoise*);

	// MSP Messages
static void	TrrrDSP(tTriNoise*, t_signal**, short*);
static int*	TrrrPerform(int*);


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
			(method) TrrrNew,			// Instance creation function
			(method) dsp_free,				// Default deallocation function
			sizeof(tTriNoise),				// Class object size
			NIL,							// No menu function
			A_DEFLONG,						// Optional arguments:	1. NN Factor
			0);	
	
	dsp_initclass();

	// Messages
	addint	((method) TrrrNN);
	addmess	((method) TrrrTattle,	"dblclick",	A_CANT, 0);
	addmess	((method) TrrrTattle,	"tattle",	A_NOTHING);
	addmess	((method) TrrrAssist,	"assist",	A_CANT, 0);
	addmess	((method) TrrrInfo,		"info",		A_CANT, 0);
	
	// MSP-Level messages
	LITTER_TIMEBOMB addmess((method) TrrrDSP, "dsp", A_CANT, 0);

	//Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();
	
	}



#pragma mark -
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	TrrrNew(iNN)
 *	
 ******************************************************************************************/

void*
TrrrNew(
	long	iNN)
	
	{
	tTriNoise*		me	= NIL;
	
	// Take intialization parameters as they come.

	// Let Max/MSP allocate us, our inlets, and outlets.
	me = (tTriNoise*) newobject(gObjectClass);
	dsp_setup(&(me->coreObject), 1);				// Signal inlet for benefit of begin~
													// Otherwise left inlet does "NN" and
													// symmetry
	outlet_new(me, "signal");
	
	// Set up object components
	TrrrNN(me, iNN);
		// The following would be nicer with a switch, but C can't switch against
		// values not known at compile time

	return me;
	}

#pragma mark -
#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	TrrrNN(me, iNN)
 *	
 *	Set parameters, making sure nothing bad happens.
 *
 ******************************************************************************************/

void TrrrNN(
	tTriNoise* me,
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
 *	TrrrTattle(me)
 *
 *	Post state information
 *
 ******************************************************************************************/

void
TrrrTattle(
	tTriNoise* me)
	
	{
	
	post("%s state",
			kClassName);
	post("  NN factor is: %d (mask = %lx)",
			me->nn,
			me->mask);
	
	}


/******************************************************************************************
 *
 *	TrrrAssist()
 *	TrrrInfo()
 *
 *	Fairly generic Assist/Info methods
 *
 ******************************************************************************************/

void TrrrAssist(tTriNoise* me, void* box, long iDir, long iArgNum, char* oCStr)
	{
	#pragma unused(me, box)
	
	LitterAssist(iDir, iArgNum, strIndexInLeft, strIndexOutLeft, oCStr);
	}

void TrrrInfo(tTriNoise* me)
	{ LitterInfo(kClassName, &me->coreObject.z_ob, (method) TrrrTattle); }


#pragma mark -
#pragma mark • DSP Methods

/******************************************************************************************
 *
 *	TrrrDSP(me, ioDSPVectors, iConnectCounts)
 *
 ******************************************************************************************/

void
TrrrDSP(
	tTriNoise*	me,
	t_signal**	ioDSPVectors,
	short*		connectCounts)
	
	{
	#pragma unused(connectCounts)
	
	dsp_add(
		TrrrPerform, 3,
		me, (long) ioDSPVectors[outletTriNoise]->s_n, ioDSPVectors[outletTriNoise]->s_vec
		);
	
	}
	

/******************************************************************************************
 *
 *	TrrrPerform(iParams)
 *
 *	Parameter block for PerformSync contains 4 values:
 *		- Address of this function
 *		- The performing trrr~ object
 *		- Vector size
 *		- output signal
 *
 ******************************************************************************************/

int*
TrrrPerform(
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
	tTriNoise*		me = (tTriNoise*) iParams[paramMe];
	
	if (me->coreObject.z_disabled) goto exit;
	
	// Copy parameters into registers
	vecCounter	= (long) iParams[paramVectorSize];
	outNoise	= (tSampleVector) iParams[paramOut];
	
	// Do our stuff
	if (me->nn > 0) {
		unsigned long	mask	= me->mask,
						offset	= me->offset;
		do {
			unsigned long t = (Taus88(NULL) >> 1) + (Taus88(NULL) >> 1);
			*outNoise++ = ULong2Signal((t & mask) + offset);
			} while (--vecCounter > 0);
		}
	
	else do {
		*outNoise++ = Taus88TriSig();
		} while (--vecCounter > 0);

exit:
	return iParams + paramNextLink;
	}
