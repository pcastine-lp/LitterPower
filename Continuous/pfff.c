/*
	File:		pfff.c

	Contains:	Max external object generating evenly distributed random values in the
				range [0 .. 1] with 1/f^2 distribution: "Brown noise".

	Written by:	Peter Castine

	Copyright:	© 2001-2002 Peter Castine

	Change History (most recent first):

         <9>   26–4–2006    pc      Update for new LitterLib organization.
         <8>   24–6–2005    pc      Correcteded order of argXXX enumeration, putting Hurst factor
                                    before NN.
         <7>     15–1–04    pc      Avoid possible memory leak if seed argument is used.
         <6>     14–1–04    pc      Update for Windows.
         <5>    7–7–2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <4>  30–12–2002    pc      Drop faux 'Vers' resource and related modifications. Add object
                                    version to DoInfo().
         <3>  29–12–2002    pc      Update parameters to __ide_target() to match new Classic/Carbon
                                    target naming conventions
         <2>  28–11–2002    pc      Tidy up after initial check-in.
         <1>  28–11–2002    pc      Initial check in.
		 9-Feb-2000:	Added support for variable Hurst factor (lp.pvvv)
		30-Jun-2001:	Modified to use Taus88 instead of TT800 as default generator.
						Also update to use new ULong2Unitxx() functions.
		15-Apr-2000:	First implementation.
*/


/******************************************************************************************
 ******************************************************************************************/


#pragma mark • Identify Target

#ifdef __MWERKS__
	// With Metrowerks CodeWarrior make use of the __ide_target() preprocessor function
	// to identify which variant of the scamp family we're generating.
	// On other platforms (like GCC) we have to use a command-line flag or something
	// when building the individual targets.
	#if __ide_target("Uniform: Brown (Classic)") 			\
			|| __ide_target("Uniform: Brown (Carbon)")		\
			|| __ide_target("Uniform: Brown (Windows)")
		#define BROWN 1
	#elif __ide_target("Uniform: Black (Classic)") 			\
			|| __ide_target("Uniform: Black (Carbon)")		\
			|| __ide_target("Uniform: Black (Windows)")
		#define BLACK 1
	#elif __ide_target("Uni.: Multicolor (Classic)") 			\
			|| __ide_target("Uni.: Multicolor (Carbon)")	\
			|| __ide_target("Uni.: Multicolor (Windows)")
		#define VARICOLOR 1
	#else
		#error "Undefined target"
	#endif
#endif

#pragma mark • Include Files

#include "LitterLib.h"
#include "TrialPeriodUtils.h"
#include "MiscUtils.h"
#include "Taus88.h"
#include "RNGGauss.h"


#pragma mark • Constants

#if		defined(BROWN)
	const char*	kClassName		= "lp.pfff";			// Class name
#elif	defined(BLACK)
	const char*	kClassName		= "lp.phhh";			// Class name
#elif	defined(VARICOLOR)
	const char*	kClassName		= "lp.pvvv";			// Class name
#else
	#error "Unknown project"
#endif


const int	kMaxNN			= 31;


	// Indices for STR# resource
enum {
	strIndexInBang		= lpStrIndexLastStandard + 1,
#ifdef VARICOLOR
	strIndexInHurst,
#endif
	strIndexInNN,
	strIndexOutBrown,
	
	strIndexInLeft		= strIndexInBang,
	strIndexOutLeft		= strIndexOutBrown
	};


#pragma mark • Type Definitions



#pragma mark • Object Structure

typedef struct {
	Object			coreObject;
	
	tTaus88DataPtr	tausData;
	
	float*			theBuffer;			// Allocated at object creation
	int				nn;					// Number of bits to mask out
	
#ifdef VARICOLOR
	double			hurstExp,
					hurstFac;
#endif
	
	unsigned long	cycle,
					curPos,
					nnMask,				// Values depends on nn
					nnOffset;
	} tBrown;


#pragma mark • Global Variables



#pragma mark • Function Prototypes

	// Class message functions
static void*	PvvvNew(Symbol*, short, Atom[]);
static void		PvvvFree(tBrown*);

	// Object message functions
static void PvvvBang(tBrown*);
#ifdef VARICOLOR
	static void PvvvHurst(tBrown*, double);
#endif
static void PvvvNN(tBrown*, long);
static void PvvvSeed(tBrown*, long);
static void PvvvTattle(tBrown*);
static void	PvvvAssist(tBrown*, void* , long , long , char*);
static void	PvvvInfo(tBrown*);


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
	setup(	&gObjectClass,				// Pointer to our class definition
			(method) PvvvNew,			// Instance creation function
			(method) PvvvFree,			// Custom deallocation function
			sizeof(tBrown),				// Class object size
			NIL,						// No menu function
			A_GIMME,					// Optional arguments:	1. Cycle (must be 2^n)
										//						2. Hurst Factor (0 ≤ h ≤ 1)
										//						3. NN Factor
										// 						4. Seed value
			0);							// Unfortunately, Max won't parse this argument list
										// for me, so I have to use A_GIMME
		

	// Messages
	LITTER_TIMEBOMB addbang	((method) PvvvBang);
#ifdef VARICOLOR
	addmess	((method) PvvvHurst,	"ft1",		A_FLOAT, 0);
	addmess	((method) PvvvNN,		"in2",		A_LONG, 0);
#else
	addmess	((method) PvvvNN,		"in1",		A_LONG,	0);
#endif
	addmess ((method) PvvvSeed,		"seed",		A_DEFLONG, 0);
	addmess	((method) PvvvTattle,	"dblclick",	A_CANT, 0);
	addmess	((method) PvvvTattle,	"tattle",	A_NOTHING);
	addmess	((method) PvvvAssist,	"assist",	A_CANT, 0);
	addmess	((method) PvvvInfo,		"info",		A_CANT, 0);
	
	//Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();
	
	}



#pragma mark -
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	PvvvNew(iNN, iSeed)
 *	
 *	Arguments: nn factor, seed.
 *	
 *	If no seed is specified, use Taus88's own data pool.
 *
 ******************************************************************************************/

	static long NudgeCycle(long iCycle)
		{
		unsigned long	nudge	= iCycle,
						mask	= kLongMax - 1;		// NOT kULongMask!
		
		while (CountBits(nudge) > 1) {
			nudge &= mask;
			mask <<= 1;
			}
		
		return nudge;
		}

	static void NonComprendo(Atom* iBarf)
		{ error("%s doesn't understand", kClassName); postatom(iBarf); }

void*
PvvvNew(
	Symbol*	sym,			// unused, must == gensym(kClassName)
	short	iArgC,
	Atom	iArgV[])
	
	{
	#pragma unused(sym)
	
	enum UserArgs {
		argNone		= -1,
		argCycle,
#ifdef VARICOLOR
		argHurst,
#endif
		argNN,
		argSeed,
		
		argMaxArgs
		};
	
	const long		kDefCycle	= 512,
					kDefNN		= 0,
					kDefSeed	= 0;
#ifdef VARICOLOR
	const double	kDefHurst	= 0.0;			// Pink noise
												// This is also the default
												// for lp. pvvv~
#endif
	
	tBrown*			me			= NIL;
	tTaus88DataPtr	myTausStuff	= NIL;
	float*			theBuffer	= NIL;
	long			cycle,
					nn;
#ifdef VARICOLOR
	double			hurstExp;
#endif

	
	//
	// Parse initialization arguments
	//
	
		// a: Set up defaults for numeric values
	cycle		= kDefCycle;
	nn			= kDefNN;
#ifdef VARICOLOR
	hurstExp	= kDefHurst;
#endif
	
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
#ifdef VARICOLOR
		case argHurst:
			if (ParseAtom(iArgV + argHurst, FALSE, TRUE, 0, NIL, kClassName) == A_FLOAT)
				hurstExp = iArgV[argHurst].a_w.w_float;
			else NonComprendo(iArgV + argHurst);
			// Fall into next case
#endif
		case argCycle:
			if (ParseAtom(iArgV + argCycle, TRUE, FALSE, 0, NIL, kClassName) == A_LONG) {
				cycle = NudgeCycle(iArgV[argCycle].a_w.w_long);
				if (cycle <= 0) cycle = kDefCycle;
				}
			else NonComprendo(iArgV + argCycle);
			// Fall into final case
		case argNone:
			// No (more) arguments
			break;
		}
	

	//
	// Let Max try to allocate us
	//
	
	me = (tBrown*) newobject(gObjectClass);
		if (me == NIL) goto punt;
	
	// Inlets, from right to left
#ifdef VARICOLOR
	intin	(me, 2);
	floatin	(me, 1);
#else
	intin	(me, 1);											// NN inlet
#endif
	
	// Only one outlet, access through me->coreObject.o_outlet
	floatout(me);
	
	// Allocate data buffer of size cycle + 1. Note that cycle must be power of two.
	theBuffer = (float*) NewPtr((cycle + 1) * sizeof(float));
		if (theBuffer == NIL) goto punt;
	
	//
	// Store initial values
	//
	me->tausData	= myTausStuff;
	me->theBuffer	= theBuffer;
	me->cycle		= cycle;
	me->curPos		= cycle;			// This triggers GenerateNewBuffer on first bang
	theBuffer[cycle] = NormalKRTaus88(myTausStuff);
	PvvvNN(me, nn);
#ifdef VARICOLOR
	PvvvHurst(me, hurstExp);
#endif
	
	
	//
	// All done
	//
	
	return me;

punt:
	// Oops... cheesy exception handling
	error("Insufficient memory to create %s object.", kClassName);
	if (me != NIL) freeobject((Object*) me);
	return NIL;
	}


/******************************************************************************************
 *
 *	PvvvFree(me)
 *
 ******************************************************************************************/

static void
PvvvFree(
	tBrown* me)
	
	{
	
	Taus88Free(me->tausData);							// Taus88Free is NIL-safe
	
	if (me->theBuffer) DisposePtr((Ptr) me->theBuffer);
	
	}


#pragma mark -
#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	PvvvBang(me)
 *
 ******************************************************************************************/

	static void GenerateNewBuffer(tBrown* me)
		{
#if		defined(BROWN)
		const double hurstFac	= sqrt(0.5);			// Equiv. to pow(0.5, hurstExp) with
														// hurstExp = 0.5 (ie, brown noise).
#elif	defined(BLACK)
		const double hurstFac	= 0.5;					// Equiv. to pow(0.5, hurstExp) with
														// hurstExp = 1.0 (ie, black noise).
#elif	defined(VARICOLOR)
		const double hurstFac	= me->hurstFac;
#endif
		
		double			stdDev		= 1.0;
		float*			buf			= me->theBuffer;
		long			cycle		= me->cycle,
						stride		= cycle >> 1,
						offset		= stride >> 1;
		tTaus88DataPtr	tausStuff	= me->tausData;
		
		//
		// Voss random addition algorithm
		//
		
		// pfff takes buf[0] as it stands, initialize midpoint and endpoint
		// (Inheriting buf[0] from the previous run and leaving it unmodified is the main
		// modification to the Voss algorithm made in this implementation.)
		//
		buf[stride]	= NormalKRTaus88(tausStuff) * stdDev;		// ASSERT: stride = cycle/2
		buf[cycle]	= NormalKRTaus88(tausStuff) * stdDev;
		
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
				buf[i]			+= stdDev * NormalKRTaus88(tausStuff);
				buf[i+offset]	+= stdDev * NormalKRTaus88(tausStuff);
				} 
			
			// Next generation: halve stride and offset
			stride = offset; 
			offset >>= 1;
		    }
		}
	
void
PvvvBang(
	tBrown* me)
	
	{
	long	pos	= me->curPos;
	float*	buf	= me->theBuffer;
	double	sample;
	
	if (pos == me->cycle) {
		buf[0] = buf[pos];
		GenerateNewBuffer(me);
		pos = 0;
		}
	
	sample = buf[pos] / 12.0 + 0.5;
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
	
	if (me->nn != 0) {
		// Convert to int, mask, convert back to FP
		// ASSERT: 0.0 <= sample <= 1.0
		unsigned long lSamp = kULongMax * sample + 0.5;
		lSamp &= me->nnMask;
		lSamp += me->nnOffset;
		sample = lSamp * (1.0 / (double) kULongMax);
		}
	
	outlet_float(me->coreObject.o_outlet, sample);
	
	}

/******************************************************************************************
 *
 *	PvvvNN(me, iNN)
 *	PvvvHurst(me, iHurst)
 *	
 *	Set parameters, making sure nothing bad happens.
 *
 ******************************************************************************************/

void PvvvNN(
	tBrown* me,
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


#ifdef VARICOLOR
	void PvvvHurst(
		tBrown* me,
		double	iHurst)
		
		{
		
		me->hurstExp	= iHurst;
		me->hurstFac	= pow(0.5, iHurst);
		
		}
#endif

/******************************************************************************************
 *
 *	PvvvSeed(me, iSeed)
 *
 ******************************************************************************************/

void PvvvSeed(tBrown* me,long iSeed)
	{ Taus88Seed(me->tausData, (unsigned long) iSeed); }


/******************************************************************************************
 *
 *	PvvvTattle(me)
 *
 *	Post state information
 *
 ******************************************************************************************/

void
PvvvTattle(
	tBrown* me)
	
	{
	
	post("%s state", kClassName);
	post("  Cycle length is: %ld",
			me->cycle);
	post("  Buffer starts at: %lx",
			me->theBuffer);
	post("  Current position is: %ld",
			me->curPos);
	post("  Current buffer value is: %lf",
			me->theBuffer[me->curPos]);
#ifdef VARICOLOR
	post("	Hurst exponent is: %f (Hurst factor: %f)",
			me->hurstExp, me->hurstFac);
#endif
	post("  NN factor is: %d (mask = 0x%lx, offset = 0x%lx)",
			me->nn, me->nnMask, me->nnOffset);
	
	}


/******************************************************************************************
 *
 *	PvvvAssist
 *
 *	Fairly generic Assist Method
 *
 ******************************************************************************************/

void
PvvvAssist(
	tBrown*	me,
	void*	box,
	long	iDir,
	long	iArgNum,
	char*	oCStr)
	
	{
	#pragma unused(me, box)
	
	LitterAssist(iDir, iArgNum, strIndexInLeft, strIndexOutLeft, oCStr);
	
	}

/******************************************************************************************
 *
 *	PvvvInfo(me)
 *
 *	Fairly basic Info Method
 *
 ******************************************************************************************/

void
PvvvInfo(
	tBrown* me)
	
	{
	
	LitterInfo(kClassName, &me->coreObject, (method) PvvvTattle);
	
	}

