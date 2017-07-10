/*
	File:		zzz.c

	Contains:	1/f distribution generator (“Pink noise”) using James McCartney's
				improved version of the Voss/Gardner algorithm

	Written by:	Peter Castine

	Copyright:	© 2001-2002 Peter Castine

	Change History (most recent first):

         <7>   26–4–2006    pc      Update for new LitterLib organization.
         <6>     15–1–04    pc      Avoid possible memory leak if seed argument is used.
         <5>     14–1–04    pc      Update for Windows.
         <4>    7–7–2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <3>  30–12–2002    pc      Drop faux 'Vers' resource and related modifications. Add object
                                    version to ZzzInfo().
         <2>  28–11–2002    pc      Tidy up after initial check-in.
         <1>  28–11–2002    pc      Initial check in.
		30-Jun-2001:	Modified to use Taus88 instead of TT800 as default generator.
						Also update to use new ULong2Unitxx() functions.
		14-Apr-2001:	First implementation.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "LitterLib.h"
#include "TrialPeriodUtils.h"
#include "MiscUtils.h"
#include "Taus88.h"


#pragma mark • Constants

const char*			kClassName		= "lp.zzz";			// Class name

#ifdef __GNUC__
	// Lame
	#define		kArraySize	15
	#define		kArrayMask	0x7fff
	#define		kMaxNN		31
#else
	const int	kArraySize		= 15,
				kArrayMask		= 0x7fff,
				kMaxNN			= 31;
#endif

	// Indices for STR# resource
enum {
	strIndexInBang		= lpStrIndexLastStandard + 1,
	strIndexInNN,
	
	strIndexOutPink
	};

	// Indices for Inlets and Outlets. Users count starting at 1, we count from 0
	// 
enum {
	inletBang		= 0,
	inletNN,
	
	outletPink
	};

#pragma mark • Type Definitions



#pragma mark • Object Structure

typedef struct {
	Object			coreObject;
	
	tTaus88DataPtr	tausData;
	
	int				nn;					// Number of bits to mask out
	unsigned long	counter,
					nnMask,				// Values depend on nn
					nnOffset;
	unsigned long	sum,
					dice[kArraySize];
	} tPink;


#pragma mark • Global Variables


#pragma mark • Function Prototypes

	// Class message functions
void*	ZzzNew(long, long);
void	ZzzFree	(tPink*);

	// Object message functions
static void ZzzBang(tPink*);
static void ZzzNN(tPink*, long);
static void ZzzSeed(tPink*, long);
static void ZzzTattle(tPink*);
static void	ZzzAssist(tPink*, void* , long , long , char*);
static void	ZzzInfo(tPink*);


#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark • Inline Functions



#pragma mark -

/******************************************************************************************
 *
 *	main()
 *	
 *	Standard Max External Object Entry Point Function
 *	
 ******************************************************************************************/

void
main(void)
	
	{
	LITTER_CHECKTIMEOUT(kClassName);
	
	// Standard Max setup() call
	setup(	&gObjectClass,			// Pointer to our class definition
			(method) ZzzNew,		// Instance creation function
			(method) ZzzFree,		// Custom deallocation function
			sizeof(tPink),			// Class object size
			NIL,					// No menu function
			A_DEFLONG,				// Optional arguments:	1. NN Factor
			A_DEFLONG,				// 						3. Seed value
			0);		
	

	// Messages
	LITTER_TIMEBOMB addbang	((method) ZzzBang);
	addmess	((method) ZzzNN,		"in1",		A_LONG);
	addmess ((method) ZzzSeed,		"seed",		A_DEFLONG, 0);
	addmess	((method) ZzzTattle,	"dblclick",	A_CANT, 0);
	addmess	((method) ZzzTattle,	"tattle",	A_NOTHING);
	addmess	((method) ZzzAssist,	"assist",	A_CANT, 0);
	addmess	((method) ZzzInfo,		"info",		A_CANT, 0);
	
	//Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();
	
	}



#pragma mark -
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	ZzzFree(me)
 *	ZzzNew(iNN, iSeed)
 *	
 *	Arguments: nn factor, seed.
 *	
 *	If no seed is specified, use Taus88's own data pool. If max is 0 and min is positive, 
 *	set range to [0..min]. If both max and min are 0, set range to [0..1]. If min>max, no
 *	random numbers will be generated (output 0s instead).
 *
 ******************************************************************************************/

void ZzzFree(tPink* me)
	{ Taus88Free(me->tausData); }							// Taus88Free is NIL-safe

	static void InitPinkDice(tPink* me) 
		{ 
		tTaus88DataPtr	tausData = me->tausData;
		unsigned long*	curDie	= me->dice;
		long			sum 	= 0;
		int				i		= kArraySize;
		
		do { sum += *curDie++ = Taus88(tausData) >> 4; }
			while (--i > 0);
			
		me->counter = 0;
		me->sum		= sum;
		} 
 
void*
ZzzNew(
	long	iNN,
	long	iSeed)
	
	{
	tPink*			me			= NIL;
	tTaus88DataPtr	myTausStuff	= NIL;	
	
	
	// Run through initialization parameters from right to left, handling defaults
	if (iSeed != 0) myTausStuff = Taus88New(iSeed);
	
	// Default NN value doesn't need massaging
	// Finished checking intialization parameters

	// Let Max allocate us, our inlets, and outlets.
	me = (tPink*) newobject(gObjectClass);
	
	intin(me, 1);								// NN inlet
	
	floatout(me);								// Access through me->coreObject.o_outlet
	
	// Set up object components
	me->tausData = myTausStuff;
	ZzzNN(me, iNN);								// Order is important: NN must be inited
	InitPinkDice(me);							// before calculating the row.

	return me;
	}

#pragma mark -
#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	ZzzBang(me)
 *
 ******************************************************************************************/

void
ZzzBang(
	tPink* me)
	
	{
	tTaus88DataPtr	tausData = me->tausData;
	unsigned long	c		= me->counter;
	long			sum 	= me->sum;
	
	if (c != 0) {
		// Need to count the number of clear LSBs to work out which die to update
		unsigned int	rightZeroes	= 0;
		unsigned long	testBit		= 0x01;
		unsigned long*	curDie;
		
		// ASSERT: c != 0
		// (Otherwise the following will never terminate!)
		while ((c & testBit) == 0) {
			testBit		<<= 1;
			rightZeroes	+= 1;
			}
		
		// rightZeroes is the index into the array of "dice" of the element that gets modified
		// this time around. Calculate the address once
		curDie = me->dice + rightZeroes;
		sum -= *curDie;
		sum += *curDie = Taus88(tausData) >> 4;
		}
	// If c == 0, nothing gets changed (aside from the hi-freq. white noise component)
	
	me->counter = (c + 1) & kArrayMask;
	me->sum		= sum;
	
	// Add "white noise" component, then NN-ize final result 
	sum	+= Taus88(tausData) >> 4;
	if (me->nn != 0) {
		sum &= me->nnMask;
		sum += me->nnOffset;
		}
	
	outlet_float(me->coreObject.o_outlet, ULong2Unit_ZO(sum));
	
	}

/******************************************************************************************
 *
 *	ZzzNN(me, iNN)
 *	
 *	Set parameters, making sure nothing bad happens.
 *
 ******************************************************************************************/

void ZzzNN(
	tPink* me,
	long	iNN)
	
	{
	
	if (iNN <= 0) {
		me->nn			= 0;
		me->nnMask		= kULongMax;
		me->nnOffset	= 0;
		}
	else {
		if (iNN > kMaxNN)
			iNN = kMaxNN;
		me->nn			= iNN;
		me->nnMask		= kULongMax << iNN;
		me->nnOffset	= (~me->nnMask) >> 1;
		}
	
	}


/******************************************************************************************
 *
 *	ZzzSeed(me, iSeed)
 *
 ******************************************************************************************/

void
ZzzSeed(
	tPink*	me,
	long	iSeed)		// Gets treated as unsigned, even if negative was specified
	
	{
	
	Taus88Seed(me->tausData, (unsigned long) iSeed);
	
	}


/******************************************************************************************
 *
 *	ZzzTattle(me)
 *
 *	Post state information
 *
 ******************************************************************************************/

void
ZzzTattle(
	tPink* me)
	
	{
	
	post("%s state",
			kClassName);
	post("  NN factor is: %d (mask = 0x%lx, offset = 0x%lx)",
			me->nn, me->nnMask, me->nnOffset);
	post("  Current sum is: %lu",
			me->sum);
	
	}


/******************************************************************************************
 *
 *	ZzzAssist
 *
 *	Fairly generic Assist Method
 *
 ******************************************************************************************/

void
ZzzAssist(
	tPink*		me,
	void*		box,
	long		iDir,
	long		iArgNum,
	char*		oCStr)
	
	{
	#pragma unused(me, box)
	
	LitterAssist(iDir, iArgNum, strIndexInBang, strIndexOutPink, oCStr);

	}

/******************************************************************************************
 *
 *	ZzzInfo(me)
 *
 *	Fairly basic Info Method
 *
 ******************************************************************************************/

void
ZzzInfo(
	tPink* me)
	
	{
	
	LitterInfo(kClassName, &me->coreObject, (method) ZzzTattle);
	
	}

