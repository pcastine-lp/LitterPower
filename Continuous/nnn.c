/*
	File:		nnn.c

	Contains:	Max external object generating evenly distributed random values in the
				range [0 .. 1] with 1/f^2 distribution: "Browner noise".

	Written by:	Peter Castine

	Copyright:	© 2001-2002 Peter Castine

	Change History (most recent first):

         <2>  28–11–2002    pc      Tidy up after initial check-in.
         <1>  28–11–2002    pc      Initial check in.
		30-Jun-2001:	Modified to use Taus88 instead of TT800 as default generator.
						Also update to use new ULong2Unitxx() functions.
		15-Apr-2000:	First implementation.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include <stdlib.h>		// For rand(), RAND_MAX

#include "LitterLib.h"

#ifndef __SETUPA4__
	#include <SetUpA4.h> 
#endif

#ifndef __A4STUFF__
	#include <A4Stuff.h>		// #includes Devices.h
#endif


#pragma mark • Constants

const char*	kClassName		= "lp.knnn";			// Class name
const char*	kPackageName	= "lp.knnn Package";	// Package name for 'vers'(2) resource

const char* kCategory		= "Arith/Logic/Bitwise";

const int	kMaxNN			= 31,
			kSignBit		= 0x80000000,
			kStepMask		= 0x07ffffff;			// ¿¿¿ Should this be user configurable ???


	// Indices for STR# resource
enum {
	strIndexInBang		= 1,
	strIndexInHurst,
	strIndexInNN,
	
	strIndexOutBrowner
	};


#pragma mark • Type Definitions



#pragma mark • Object Structure

typedef struct {
	Object			coreObject;
	voidPtr			theOutlet;
	
	tTaus88DataPtr	theData;
	
	double			hurst;
	float*			theBuffer;			// Allocated at object creation
	unsigned long	cycle,
					curPos,
					nn,					// Number of bits to mask out
					nnMask,				// Values depends on nn
					nnOffset;
	} tBrowner;


#pragma mark • Global Variables

fptr*		FNS				= NIL;
t_messlist*	gBrownerClass		= NIL;
short		gStrResID		= 0,
			gVersID			= 0,
			gPackVersID		= 0;


#pragma mark • Function Prototypes

	// Class message functions
void*	NewBrowner(Symbol*, short, Atom[]);
void	FreeBrowner(tBrowner*);

	// Object message functions
static void DoBang(tBrowner*);
static void DoHurst(tBrowner*, double);
static void DoNN(tBrowner*, long);
static void DoSeed(tBrowner*, long);
static void DoTattle(tBrowner*);
static void	DoAssist(tBrowner*, void* , long , long , char*);
static void	DoInfo(tBrowner*);


#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark • Inline Functions



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
	setup(	&gBrownerClass,				// Pointer to our class definition
			(method) NewBrowner,			// Instance creation function
			(method) FreeBrowner,			// Custom deallocation function
			sizeof(tBrowner),				// Class object size
			NIL,						// No menu function
			A_GIMME,					// Optional arguments:	1. Cycle (must be 2^n)
										//						2. Hurst Factor (0 ≤ h ≤ 1)
										//						3. NN Factor
										// 						4. Seed value
			0);							// Unfortunately, Max won't parse this argument list
										// for me, so I have to use A_GIMME
		
	// Messages
	LITTER_TIMEBOMB addbang	((method) DoBang);
	addmess	((method) DoHurst,		"ft1",		A_FLOAT);
	addmess	((method) DoNN,			"in2",		A_LONG);
	addmess ((method) DoSeed,		"seed",		A_DEFLONG, 0);
	addmess	((method) DoTattle,		"dblclick",	A_CANT, 0);
	addmess	((method) DoAssist,		"assist",	A_CANT, 0);
	addmess	((method) DoInfo,		"info",		A_CANT, 0);
	
	// Initialize Litter Library
	LitterInit();
	
	}



#pragma mark -
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	NewBrowner(iNN, iSeed)
 *	
 *	Arguments: nn factor, seed.
 *	
 *	If no seed is specified, use Taus88's own data pool.
 *
 ******************************************************************************************/

	static long NudgeCycle(long iCycle)
		{
		unsigned long	nudge	= iCycle,
						mask	= kLongMax;		// NOT kULongMask!
		
		while (CountBits(nudge) > 1) {
			nudge &= mask;
			mask <<= 1;
			}
		
		return nudge;
		}

	static void NonComprendo(Atom* iBarf)
		{ error("%s doesn't understand", kClassName); postatom(iBarf); }

void*
NewBrowner(
	Symbol*,
	short	iArgC,
	Atom	iArgV[])
	
	{
	enum UserArgs {
		argCycle	= 0,
		argHurst,
		argNN,
		argSeed,
		
		argMaxArgs
		};
	
	const double	kDefHurst	= 0.5;
	const long		kDefCycle	= 512,
					kDefNN		= 0,
					kDefSeed	= 0;
	
	tBrowner*			me			= NIL;
	tTaus88DataPtr	myTausStuff	= NIL;
	float*			theBuffer	= NIL;
	double			hurst;
	long			cycle,
					nn;
						
	//
	// Parse initialization arguments
	//
	
		// a: Set up defaults for numeric values
	cycle	= kDefCycle;
	hurst	= kDefHurst;
	nn		= kDefNN;
	
		// b: Check to see what the user has specified.
	switch (iArgC - 1) {
		default:
			error("%s received too many arguments", kClassName);
			// Fall into next case
		case argSeed:
			if (ParseAtom(iArgV + argSeed, TRUE, FALSE, 0, NIL, kClassName) == A_LONG)
				myTausStuff = Taus88New(iArgV[argSeed].a_w.w_long);
			else NonComprendo(iArgV + argSeed);
			// Fall into next case
		case argNN:
			if (ParseAtom(iArgV + argNN, TRUE, FALSE, 0, NIL, kClassName) == A_LONG)
				nn = iArgV[argNN].a_w.w_long;
			else NonComprendo(iArgV + argNN);
			// Fall into next case
		case argHurst:
			// Watch out! Here comes our floating point value!
			if (ParseAtom(iArgV + argHurst, FALSE, TRUE, 0, NIL, kClassName) == A_FLOAT)
				hurst = iArgV[argHurst].a_w.w_float;
			else NonComprendo(iArgV + argHurst);
			// Fall into next case
		case argCycle:
			if (ParseAtom(iArgV + argCycle, TRUE, FALSE, 0, NIL, kClassName) == A_LONG)
				cycle = NudgeCycle(iArgV[argCycle].a_w.w_long);
			else NonComprendo(iArgV + argCycle);
			break;
		}
	

	//
	// Let Max try to allocate us
	//
	
	me = (tBrowner*) newobject(gBrownerClass);
		if (me == NIL) goto punt;
	
	// Inlets, from right to left
	intin	(me, 2);											// NN inlet
	floatin	(me, 1);											// Hurst factor
	
	// Only one outlet
	me->theOutlet	= floatout(me);
		if (me->theOutlet == NIL) goto punt;
	
	// Allocate data buffer of size cycle + 1. Note that cycle + 1 must be power of two.
	theBuffer = (float*) NewPtr((cycle + 1) * sizeof(float));
		if (theBuffer == NIL) goto punt;
	
	//
	// Store initial values
	//
	me->theData		= myTausStuff;
	me->theBuffer	= theBuffer;
	me->cycle		= cycle;
	me->curPos		= cycle;			// This triggers GenerateNewBuffer on first bang
	theBuffer[cycle] = ULong2Unit_zo(Taus88(myTausStuff));
	DoHurst(me, hurst);
	DoNN(me, nn);
	
	
	//
	// All done
	//
	return me;

punt:
	// Oops... cheesy exception handling
	error("Insufficient memory to create %s object.", kClassName);
	if (me) freeobject((Object*) me);
	return NIL;
	}


/******************************************************************************************
 *
 *	FreeBrowner(me)
 *
 ******************************************************************************************/

static void
FreeBrowner(tBrowner* me)
	{ if (me->theBuffer) DisposePtr((Ptr) me->theBuffer); }


#pragma mark -
#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	DoBang(me)
 *
 ******************************************************************************************/

	static void GenerateNewBuffer(tBrowner* me)
		{
		const double	kInitStdDev	= 0.3333333333;		// Experimentally derived
		
		double			u1, u2,
						stdDev		= kInitStdDev,
						hurstFac	= pow(0.5, me->hurst);
		float*			buf			= me->theBuffer;
		long			cycle		= me->cycle,
						stride		= cycle >> 1,
						offset		= stride >> 1;
		tTaus88DataPtr	tausStuff	= me->theData;
		
		//
		// Voss random addition algorithm
		//
		
		// knnn takes buf[0] as it stands, initialize midpoint and endpoint
		u1 = LPNormal(&u2, (tRandomFunc) Taus88, tausStuff);
		buf[stride]	= 0.5 + u1 * stdDev;		// ASSERT: stride = cycle/2
		buf[cycle]	= 0.5 + u2 * stdDev;
		
		// Recursive interpolation
		// Initial state: stride = cycle/2, offset = stride/2
		while (offset > 0) {
			int i;
			
			// Interpolate initial values at midpoints between values
			// calculated so far
			for (i = offset; i <= cycle - offset; i += stride) 
			    buf[i] = 0.5 * (buf[i-offset] + buf[i+offset]);	
			
			// Add noise with reduced variance
			stdDev *= hurstFac;
			for (i = offset; i < cycle; i += stride) {
				u1 = LPNormal(&u2, (tRandomFunc) Taus88, tausStuff);
				buf[i]			+= stdDev * u1;
				buf[i+offset]	+= stdDev * u2;
				} 
			
			// Next generation: halve stride and offset
			stride = offset; 
			offset >>= 1;
		    }
		}
	
void
DoBang(
	tBrowner* me)
	
	{
	long	pos	= me->curPos;
	float*	buf	= me->theBuffer;
	double	sample;
	
	if (pos == me->cycle) {
		buf[0] = buf[pos];
		GenerateNewBuffer(me);
		pos = 0;
		}
	
	sample = buf[pos];
	me->curPos = pos + 1;
	
	if (sample < 0.0 || 1.0 < sample) {
		// Reflect into unit range
		sample = fmod(sample, 2);
		if (sample < 0)						// Compensate for C's lousy 
			sample += 2;					// modulo arithmetic
											// ASSERT: 0 <= outVal < 2
		if (sample > 1)
			sample = 2 - sample;			// ASSERT: 0 <= outVal < 1
		}
	
	outlet_float(me->theOutlet, sample);
	
	}

/******************************************************************************************
 *
 *	DoHurst(me, iHurst)
 *	DoNN(me, iNN)
 *	
 *	Set parameters, making sure nothing bad happens.
 *
 ******************************************************************************************/

void DoHurst(
	tBrowner* me,
	double	iHurst)
	
	{
	
	if (iHurst < 0.0)
		iHurst = 0.0;
	else if (iHurst > 1.0)
		iHurst = 1.0;
	
	me->hurst = iHurst;
	
	}

void DoNN(
	tBrowner* me,
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
 *	DoSeed(me, iSeed)
 *
 ******************************************************************************************/

void
DoSeed(
	tBrowner*	me,
	long		iSeed)		// Gets treated as unsigned, even if negative was specified
	
	{
	
	Taus88Seed(me->theData, (unsigned long) iSeed);
	
	}


/******************************************************************************************
 *
 *	DoTattle(me)
 *
 *	Post state information
 *
 ******************************************************************************************/

void
DoTattle(
	tBrowner* me)
	
	{
	
	post("%s state", kClassName);
	post("  Cycle length is: %ld",
			me->cycle);
	post("  Hurst factor is: %lf",
			me->hurst);
	post("  Buffer starts at: %lx",
			me->theBuffer);
	post("  Current position is: %ld",
			me->curPos);
	post("  Current buffer value is: %lf",
			me->theBuffer[me->curPos]);
	post("  NN factor is: %d (mask = 0x%lx, offset = 0x%lx)",
			me->nn, me->nnMask, me->nnOffset);
	
	}


/******************************************************************************************
 *
 *	DoAssist
 *
 *	Fairly generic Assist Method
 *
 ******************************************************************************************/

void
DoAssist(
	tBrowner*	me,
	void*		iBox,
	long		iDir,
	long		iArgNum,
	char		oCStr[])
	
	{
	#pragma unused(me, iBox)
	
	assist_string(	gStrResID, iDir, iArgNum, strIndexInBang, strIndexOutBrowner, oCStr);
	
	}

/******************************************************************************************
 *
 *	DoInfo(me)
 *
 *	Fairly basic Info Method
 *
 ******************************************************************************************/

void
DoInfo(
	tBrowner* me)
	
	{
	char	packMessage[sizeof(Str255)]
	
	GetVersStrings(gPackVersID, NIL, packMessage);
	
	ouchstring("%s object.\r%s.", kClassName, packMessage);
	DoTattle(me);
	
	}

