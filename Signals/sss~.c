/*
	File:		sss~.c

	Contains:	"Pink" noise using the Voss/Gardner (simplified) algorithm.

	Written by:	Peter Castine

	Copyright:	© 2001-2002 Peter Castine

	Change History (most recent first):

         <6>   24–3–2006    pc      Update #includes for new LitterLib organization.
         <5>     14–1–04    pc      Update for Windows.
         <4>    6–7–2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <3>  30–12–2002    pc      Drop faux 'Vers' resource and related modifications. Add object
                                    version to SssInfo().
         <2>  28–11–2002    pc      Tidy up after initial check in.
         <1>  28–11–2002    pc      Initial check in.
		14-Apr-2001:		First implementation.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "LitterLib.h"
#include "TrialPeriodUtils.h"
#include "Taus88.h"
#include "MiscUtils.h"


#pragma mark • Constants

const char*			kClassName		= "lp.sss~";			// Class name

#ifdef __MWERKS__
	const int			kArraySize		= 32,
						kMaxNN			= 31;
#else
	// This is for the benefit of lame GCC that can't handle
	// const ints as array size specifiers
	#define kArraySize	32
	#define kMaxNN		31
#endif

	// Indices for STR# resource
enum {
	strIndexInNN		= lpStrIndexLastStandard + 1,
	
	strIndexOutPink
	};

	// Indices for MSP Inlets and Outlets
	// 
enum {
	inletPink			= 0,		// For begin~
	
	outletPink
	};

#pragma mark • Type Definitions



#pragma mark • Object Structure

typedef struct {
	t_pxobject		coreObject;
	
	unsigned short	counter;
	int				nn;					// Number of bits to mask out
	unsigned long	mask,				// Value depends on nn
					offset,				// ditto
					sum,
					dice[kArraySize];
	} tPink;


#pragma mark • Global Variables



#pragma mark • Function Prototypes

	// Class message functions
void*	SssNew(long);

	// Object message functions
static void SssNN(tPink*, long);
static void SssTattle(tPink*);
static void	SssAssist(tPink*, void* , long , long , char*);
static void	SssInfo(tPink*);

	// MSP Messages
static void	SssDSP(tPink*, t_signal**, short*);
static int*	SssPerform(int*);


#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark • Inline Functions



#pragma mark -

/******************************************************************************************
 *
 *	main()
 *	
 *	Standar Max/MSP External Object Entry Point Function
 *	
 ******************************************************************************************/

void
main(void)
	
	{
	LITTER_CHECKTIMEOUT(kClassName);
	
	// Standard Max/MSP initialization mantra
	setup(	&gObjectClass,				// Pointer to our class definition
			(method) SssNew,			// Instance creation function
			(method) dsp_free,			// Default deallocation function
			sizeof(tPink),				// Class object size
			NIL,						// No menu function
			A_DEFLONG,					// Optional arguments:	1. NN Factor
			0);		
	
	dsp_initclass();

	// Messages
	addint	((method) SssNN);
	addmess	((method) SssTattle,	"dblclick",	A_CANT, 0);
	addmess	((method) SssTattle,	"tattle",	A_NOTHING);
	addmess	((method) SssAssist,	"assist",	A_CANT, 0);
	addmess	((method) SssInfo,		"info",		A_CANT, 0);
	
	// MSP-Level messages
	LITTER_TIMEBOMB addmess	((method) SssDSP,		"dsp",		A_CANT, 0);

	//Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();
	
	}



#pragma mark -
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	SssNew(iNN)
 *	
 *	Arguments: nn factor.
 *
 ******************************************************************************************/

	static void InitDice(tPink* me)
		{
		int				i		= kArraySize;
		unsigned long	sum		= 0;
		unsigned long*	curDie	= me->dice;
		
		do { sum += *curDie++ = Taus88(NULL) / kArraySize; }
			while (--i > 0);
		
		me->sum = sum;
		
		}

void*
SssNew(
	long	iNN)
	
	{
	tPink*	me	= NIL;
	
	// Default NN value doesn't need massaging

	// Let Max/MSP allocate us, our inlets, and outlets.
	me = (tPink*) newobject(gObjectClass);
	dsp_setup(&(me->coreObject), 1);				// Signal inlet for benefit of begin~
													// Otherwise left inlet does "NN" only
	outlet_new(me, "signal");
	
	// Set up object components
	SssNN(me, iNN);
	me->counter = 0;
	InitDice(me);

	return me;
	}

#pragma mark -
#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	SssNN(me, iNN)
 *	
 *	Set parameters, making sure nothing bad happens.
 *
 ******************************************************************************************/

void SssNN(
	tPink* me,
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
 *	SssTattle(me)
 *
 *	Post state information
 *
 ******************************************************************************************/

void
SssTattle(
	tPink* me)
	
	{
	post("%s state",
			kClassName);
	post("  NN factor is: %d (mask = 0x%lx)",
			me->nn,
			me->mask);
	post("  Current sum is: %lu",
			me->sum);
	}


/******************************************************************************************
 *
 *	SssAssist()
 *	SssInfo()
 *
 *	Fairly generic Assist/Info methods
 *
 ******************************************************************************************/

void SssAssist(tPink* me, void* box, long iDir, long iArgNum, char* oCStr)
	{
	#pragma unused(me, box)
	
	LitterAssist(iDir, iArgNum, strIndexInNN, strIndexOutPink, oCStr);
	}

void SssInfo(tPink* me)
	{ LitterInfo(kClassName, &me->coreObject.z_ob, (method) SssTattle); }

#pragma mark -
#pragma mark • DSP Methods

/******************************************************************************************
 *
 *	SssDSP(me, ioDSPVectors, iConnectCounts)
 *
 ******************************************************************************************/

void
SssDSP(
	tPink*		me,
	t_signal**	ioDSPVectors,
	short*		connectCounts)
	
	{
	#pragma unused(connectCounts)
	
	dsp_add(
		SssPerform, 3,
		me, (long) ioDSPVectors[outletPink]->s_n, ioDSPVectors[outletPink]->s_vec
		);
	
	}
	

/******************************************************************************************
 *
 *	PerformWhite(iParams)
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
SssPerform(
	int* iParams)
	
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
	tPink*			me = (tPink*) iParams[paramMe];
	unsigned long*	firstDie;
	unsigned long	sum,
					mask,
					offset;
	unsigned short	counter;				// Must be 16-bit value to match "dice" array
	
	if (me->coreObject.z_disabled) goto exit;
	
	// Copy parameters into registers
	vecCounter	= (long) iParams[paramVectorSize];
	outNoise	= (tSampleVector) iParams[paramOut];
	sum			= me->sum;
	firstDie	= me->dice;
	mask		= me->mask;
	offset		= me->offset;
	counter		= me->counter;
	
	// Do our stuff
	do {
		unsigned long*		curDie		= firstDie;
		int				 	bitCount	= CountBits(counter);
		unsigned			bit			= 0x01;
		
		switch (bitCount) {
			case 0:
				InitDice(me);
				sum = me->sum;
				break;
			
			case 1:
				// Just update one die
				while ((bit & counter) == 0)
					{
					bit		*= 2;			// Let compiler choose fastest way to do this
					curDie	+= 1;
					}
				sum -= *curDie;
				sum += *curDie = Taus88(NULL) / kArraySize;
				break;
			
			default:
				if (bitCount < kArraySize / 2) {
					// Still faster to calculate differences
					int i = kArraySize;
					do {
						if (bit & counter) {
							sum -= *curDie;
							sum += *curDie = Taus88(NULL) / kArraySize;
							}
						bit		*= 2;
						curDie	+= 1;
						} while (--i > 0);
					}
				else {
					// Faster just to update and recalculate sum from scratch
					int i	= kArraySize;
					sum	= 0;
					do {
						if (bit & counter)
							*curDie = Taus88(NULL) / kArraySize;
						sum += *curDie++;
						bit *= 2;
						} while (--i > 0);
					}
				break;
			}
		
		*outNoise++	= ULong2Signal((sum & mask) + offset);
		counter++;
		}
		while (--vecCounter > 0);
	
	me->sum		= sum;
	me->counter	= counter;
	
exit:
	return iParams + paramNextLink;
	}
