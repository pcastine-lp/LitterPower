/*
	File:		sss.c

	Contains:	Max external object generating evenly distributed random values in the
				range [0 .. 1]. Based on M. Matsumoto's Taus88 generator. Minimum
				and maximum are user-configurable at runtime.

	Written by:	Peter Castine

	Copyright:	© 2001-2002 Peter Castine

	Change History (most recent first):

         <7>   26Ð4Ð2006    pc      Update for new LitterLib organization.
         <6>     15Ð1Ð04    pc      Avoid possible memory leak if seed argument is used.
         <5>     14Ð1Ð04    pc      Update for Windows.
         <4>    7Ð7Ð2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <3>  30Ð12Ð2002    pc      Drop faux 'Vers' resource and related modifications. Add object
                                    version to SssInfo().
         <2>  28Ð11Ð2002    pc      Tidy up after initial check-in.
         <1>  28Ð11Ð2002    pc      Initial check in.
		30-Jun-2001:	Modified to use Taus88 instead of TT800 as default generator.
						Also update to use new ULong2Unitxx() functions.
		14-Apr-2001:	First implementation.
									
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark ¥ Include Files

#include "LitterLib.h"
#include "TrialPeriodUtils.h"
#include "MiscUtils.h"
#include "Taus88.h"


#pragma mark ¥ Constants

const char*			kClassName		= "lp.sss";			// Class name


#ifdef __MWERKS__
	// presumably CodeWarrior or other compiler that allows you to define arrays
	// using const ints
	const int			kArraySize		= 16,
						kMaxNN			= 31;
#else
	// Talk about lame. GCC won't compile unless I use macros
	// Haven't tested this with other compilers, will need to see if it is necessary
	// for further checks
	#define kArraySize		16
	#define kMaxNN			31
#endif

const unsigned long	kDiceBits		= 0x0fffffff;

	// Indices for STR# resource
enum {
	strIndexInBang		= lpStrIndexLastStandard + 1,
	strIndexInNN,
	
	strIndexOutPink
	};


#pragma mark ¥ Type Definitions



#pragma mark ¥ Object Structure

typedef struct {
	Object			coreObject;
	
	tTaus88DataPtr	tausData;
	
	unsigned short	counter;
	int				nn;					// Number of bits to mask out
	unsigned long	nnMask,				// Value depends on nn
					nnOffset,
					sum,
					dice[kArraySize];
	} tPink;


#pragma mark ¥ Global Variables



#pragma mark ¥ Function Prototypes

	// Class message functions
void*	SssNew(long, long);
void	SssFree	(tPink*);

	// Object message functions
static void SssBang(tPink*);
static void SssNN(tPink*, long);
static void SssSeed(tPink*, long);
static void SssTattle(tPink*);
static void	SssAssist(tPink*, void* , long , long , char*);
static void	SssInfo(tPink*);


#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark ¥ Inline Functions



#pragma mark -

/******************************************************************************************
 *
 *	main()
 *	
 *	Standar Max External Object Entry Point Function
 *	
 ******************************************************************************************/

void
main(void)
	
	{
	LITTER_CHECKTIMEOUT(kClassName);
	
	// Standard Max setup() call
	setup(	&gObjectClass,			// Pointer to our class definition
			(method) SssNew,		// Instance creation function
			(method) SssFree,		// Custom deallocation function
			sizeof(tPink),			// Class object size
			NIL,					// No menu function
			A_DEFLONG,				// Optional arguments:	1. NN Factor
			A_DEFLONG,				// 						3. Seed value
			0);		
	

	// Messages
	LITTER_TIMEBOMB addbang	((method) SssBang);
	addmess	((method) SssNN,		"in1",		A_LONG);
	addmess ((method) SssSeed,		"seed",		A_DEFLONG, 0);
	addmess	((method) SssTattle,	"dblclick",	A_CANT, 0);
	addmess	((method) SssTattle,	"tattle",	A_NOTHING);
	addmess	((method) SssAssist,	"assist",	A_CANT, 0);
	addmess	((method) SssInfo,		"info",		A_CANT, 0);
	
	//Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();
	
	}



#pragma mark -
#pragma mark ¥ Class Message Handlers

/******************************************************************************************
 *
 *	SssFree(me)
 *	SssNew(iNN, iSeed)
 *	
 *	Arguments: nn factor, seed.
 *	
 *	If no seed is specified, use Taus88's own data pool. If max is 0 and min is positive, 
 *	set range to [0..min]. If both max and min are 0, set range to [0..1]. If min>max, no
 *	random numbers will be generated (output 0s instead).
 *
 ******************************************************************************************/

void SssFree(tPink* me)
	{ Taus88Free(me->tausData); }							// Taus88Free is NIL-safe

void*
SssNew(
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
	SssNN(me, iNN);
	me->counter = 0;							// This will cause the dice array to be
												// initialized at the first bang.

	return me;
	}

#pragma mark -
#pragma mark ¥ Object Message Handlers

/******************************************************************************************
 *
 *	SssBang(me)
 *
 ******************************************************************************************/

void
SssBang(
	tPink* me)
	
	{
	tTaus88DataPtr	tausData	= me->tausData;
	unsigned short	c			= me->counter++;	// We rely on the increment wrapping
													// every 2^16 iterations (16-bit integer
													// representation).
	unsigned long*	curDie	= me->dice;
	unsigned long	sum 	= me->sum;
	int				i;
	
	if (c == 0) {
		// First time through... or wrap around!
		i	= kArraySize;
		sum = 0;
		do { sum += *curDie++ = Taus88(tausData) & kDiceBits; }
			while (--i > 0);
		}
	
	else {
		unsigned long	bitCount	= CountBits(c);
		unsigned 		bit			= 0x01;
		
		if (bitCount == 1) {
			// Just update one die
			while ((bit & c) == 0)
				{
				bit		+= bit;						// Marginally faster than bit <<= 1
				curDie	+= 1;
				}
			sum -= *curDie;
			sum += *curDie = Taus88(tausData) & kDiceBits;
			}
		else if (bitCount < kArraySize / 2) {
			// Still faster to calculate differences
			i = kArraySize;
			do {
				if (bit & c) {
					sum -= *curDie;
					sum += *curDie = Taus88(tausData) & kDiceBits;
					}
				bit		+= bit;						// Marginally faster than bit <<= 1
				curDie	+= 1;
				} while (--i > 0);
			}
		else {
			// Faster just to update and recalculate sum from scratch
			i	= kArraySize;
			sum	= 0;
			do {
				if (bit & c) *curDie = Taus88(tausData) & kDiceBits;
				sum += *curDie++;
				bit += bit;
				} while (--i > 0);
			}
		
		}
	
	me->sum = sum;
	
	if (me->nn != 0) {
		// Adjust output value for NN factor
		sum &= me->nnMask;
		sum += me->nnOffset;
		}
	outlet_float(me->coreObject.o_outlet, ULong2Unit_ZO(sum));
	
	}

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
 *	SssSeed(me, iSeed)
 *
 ******************************************************************************************/

void
SssSeed(
	tPink*	me,
	long	iSeed)		// Gets treated as unsigned, even if negative was specified
	
	{
	
	Taus88Seed(me->tausData, (unsigned long) iSeed);
	
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
	post("  NN factor is: %d (mask = 0x%lx, offset = 0x%lx)",
			me->nn, me->nnMask, me->nnOffset);
	post("  Current sum is: %ld",
			me->sum);
	
	}


/******************************************************************************************
 *
 *	SssAssist
 *
 *	Fairly generic Assist Method
 *
 ******************************************************************************************/

void
SssAssist(
	tPink*	me,
	void*	sym,
	long	iDir,
	long	iArgNum,
	char*	oCStr)
	
	{
	#pragma unused(me, sym)
	
	LitterAssist(iDir, iArgNum, strIndexInBang, strIndexOutPink, oCStr);
	
	}

/******************************************************************************************
 *
 *	SssInfo(me)
 *
 *	Fairly basic Info Method
 *
 ******************************************************************************************/

void
SssInfo(
	tPink* me)
	
	{
	
	LitterInfo(kClassName, &me->coreObject, (method) SssTattle);
	
	}

