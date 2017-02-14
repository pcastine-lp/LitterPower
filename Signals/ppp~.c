/*
	File:		ppp~.c

	Contains:	"Popcorn" or "Dust" noise. Width of the pops are configurable, and can be
				Poisson-distributed.

	Written by:	Peter Castine

	Copyright:	© 2001-2002 Peter Castine

	Change History (most recent first):

         <6>   26Ð4Ð2006    pc      Update for new LitterLib organization.
         <5>     14Ð1Ð04    pc      Update for Windows.
         <4>    6Ð7Ð2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <3>  30Ð12Ð2002    pc      Drop faux 'Vers' resource and related modifications. Add object
                                    version to PppInfo().
         <2>  28Ð11Ð2002    pc      Tidy up after initial check in.
         <1>  28Ð11Ð2002    pc      Initial check in.
		 9-Feb-2002:	Tested this with different optimization levels in MW C.
		 				Global Optimization Level 1 was marginally faster than Level 0
		 				Level 2 appeared to be slower (!). Level 3 causes lp.ppp~ to freeze
		 				after several seconds. Be warned.
		14-Apr-2000:	First implementation.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark ¥ Include Files

#include "LitterLib.h"	// Also #includes MaxUtils.h, ext.h
#include "TrialPeriodUtils.h"
#include "MiscUtils.h"
#include "Taus88.h"


#pragma mark ¥ Constants

const char*	kClassName		= "lp.ppp~";			// Class name


	// Indices for STR# resource
enum {
	strIndexInDensity		= lpStrIndexLastStandard + 1,
	strIndexInPopWidth,
	
	strIndexOutPop
	};

	// Indices for MSP Inlets and Outlets.
enum {
	inletPop			= 0,		// For begin~
	
	outletPop
	};

#pragma mark ¥ Type Definitions



#pragma mark ¥ Object Structure

typedef struct {
	t_pxobject		coreObject;
	
	double			curSR,
					curLevel,		// This is 0.0 a lot of the time...
					slope,			// Used while processing a pop
					nextLevel,
					minusTau,		// (-1.0 / density)
					threshhold;		// Used for Poisson-distributed (negative) pop width
	
	long			sampsToNext,	// Recalc in DSP method to match sample rate
					popWidth;
	
	eSymmetry		sym;
	} tPop;


#pragma mark ¥ Global Variables


SymbolPtr	gSymSymbol	= NIL,
			gNegSymbol	= NIL,
			gPosSymbol	= NIL,
			gNullSymbol	= NIL;

#pragma mark ¥ Function Prototypes

	// Class message functions
void*	PppNew(double, long, Symbol*);

	// Object message functions
static void PppDensity(tPop*, double);
static void	PppConstWidth(tPop*, long);
static void	PppPoisWidth(tPop*, double);
static void PppPos(tPop*);
static void PppNeg(tPop*);
static void PppSym(tPop*);
static void PppTattle(tPop*);
static void	PppAssist(tPop*, void* , long , long , char*);
static void	PppInfo(tPop*);

	// MSP Messages
static void	PppDSP(tPop*, t_signal**, short*);
static int*	PppPerform(int*);

static void PppCalcNext(tPop*);

#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark ¥ Inline Functions



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
			(method) PppNew,		// Instance creation function
			(method) dsp_free,		// Default deallocation function
			sizeof(tPop),			// Class object size
			NIL,					// No menu function
			A_DEFFLOAT,				// Optional arguments:	1. Density
			A_DEFLONG,				//						2. Pop Width
			A_DEFSYM,				//						3. Symmetry
			0);		
	
	dsp_initclass();

	// Messages
	addfloat((method) PppDensity);
	addmess	((method) PppConstWidth,	"in1",		A_LONG, 0);
//	addmess	((method) PppPoisWidth,	"fl1",		A_FLOAT, 0);
	addmess	((method) PppSym,		"sym",		A_NOTHING);
	addmess	((method) PppPos,		"pos",		A_NOTHING);
	addmess	((method) PppNeg,		"neg",		A_NOTHING);
	addmess	((method) PppTattle,		"dblclick",	A_CANT, 0);
	addmess	((method) PppTattle,		"tattle",	A_NOTHING);
	addmess	((method) PppAssist,		"assist",	A_CANT, 0);
	addmess	((method) PppInfo,		"info",		A_CANT, 0);
	
	// MSP-Level messages
	LITTER_TIMEBOMB addmess	((method) PppDSP, "dsp",		A_CANT, 0);

	// Stash pointers to commonly used symbols
	gSymSymbol	= gensym("sym");
	gPosSymbol	= gensym("pos");
	gNegSymbol	= gensym("neg");
	gNullSymbol	= gensym("");
	
	//Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();
	
	}



#pragma mark -
#pragma mark ¥ Class Message Handlers

/******************************************************************************************
 *
 *	PppNew(iNN, iSeed)
 *	
 *	!!! NOT YET HANDLING FLOATING POINT POPWIDTH ARGUMENTS !!!
 *
 ******************************************************************************************/

void*
PppNew(
	double	iDensity,
	long	iPopWidth,
	Symbol*	iSymmetry)
	
	{
	const double	kDefDensity		= 10.0;
	const long		kDefPopWidth	= 1;
	
	tPop*		me	= NIL;
	
	// Run through initialization parameters from right to left, handling defaults
	if (iSymmetry == gNullSymbol)
		iSymmetry = gPosSymbol;
	else goto noMoreDefaults;
	
	if (iPopWidth == 0)
		iPopWidth = kDefPopWidth;
	else goto noMoreDefaults;
	
	if (iDensity == 0.0)
		iDensity = kDefDensity;
noMoreDefaults:
	// Finished checking intialization parameters

	// Let Max/MSP allocate us, our inlets, and outlets.
	me = (tPop*) newobject(gObjectClass);
	dsp_setup(&(me->coreObject), 1);				// Signal inlet for benefit of begin~
													// Otherwise left inlet does "NN" only
	
	intin(me, 1);									// Add inlet for pop width
	
	outlet_new(me, "signal");
	
	// Set up object components
		// The following would be sort of nicer with a switch, but C can't switch against
		// values not known at compile time
	if (iSymmetry == gSymSymbol)
		me->sym = symSym;
	else if (iSymmetry == gNegSymbol)
		me->sym = symNeg;
	else {
		me->sym = symPos;
		if (iSymmetry != gPosSymbol)
			error("%s doesn't understand %s", kClassName, iSymmetry->s_name);
		}
	me->curSR		= sys_getsr();				// This will be set properly in the DSP method
	me->curLevel	= 0.0;
	me->slope		= 0.0;
	me->nextLevel	= 0.0;
	me->sampsToNext	= -1;						// Negative value flags recalc in DSP method
	PppDensity(me, iDensity);
	PppConstWidth(me, iPopWidth);

	return me;
	}

#pragma mark -
#pragma mark ¥ Object Message Handlers

/******************************************************************************************
 *
 *	PppDensity(me, iLambda)
 *	PppConstWidth(me, iWidth)
 *	PppPoisWidth(me, iLambda)
 *	PppPos(me)
 *	PppNeg(me)
 *	PppSym(me)
 *	
 *	Set parameters, making sure nothing bad happens.
 *
 ******************************************************************************************/

static void
PppDensity(
	tPop*	me,
	double iLambda)
	
	{
	me->minusTau = (iLambda > 0.0) ? (-1.0 / iLambda) : 0.0;
	}

static void
PppConstWidth(
	tPop* me,
	long iWidth)
	
	{ 
	me->popWidth	= (iWidth >= 0) ? iWidth : 0;
	me->threshhold	= 0.0;
	}

static void
PppPoisWidth(
	tPop*	me,
	double	iLambda)
	
	{
	me->popWidth	= - 1;
	me->threshhold	= (iLambda > 0.0) ? exp(-iLambda) : 1.0;
	}

void PppSym(tPop* me)		{ me->sym = symSym; }
void PppPos(tPop* me)		{ me->sym = symPos; }
void PppNeg(tPop* me)		{ me->sym = symNeg; }

/******************************************************************************************
 *
 *	PppTattle(me)
 *
 *	Post state information
 *
 ******************************************************************************************/

void
PppTattle(
	tPop* me)
	
	{
	double	minusTau = me->minusTau;
	
	post(	"%s state, object located at %lx",
			kClassName, me);
	switch (me->sym) {
		case symSym:
			post("  Symmetrical dust");
			break;
		case symPos:
			post("  Positive dust");
			break;
		case symNeg:
			post("  Negative dust");
			break;
		default:
			post("  Unkown dust value: %d", (int) me->sym);
		}
	
	post("  tau = %lf (i.e., density = %lf pops/second)",
			-minusTau,
			(minusTau != 0.0) ? (-1.0 / minusTau) : 0.0);
	post("  current level is %lf", me->curLevel);
	if (me->sampsToNext >= 0)
			post("  samples to next pop: %ld", me->sampsToNext);
	else	post("  samples to next pop: not yet defined");
	post("  pop width set to %lu samples", me->popWidth);
	post("  next pop will peak/trough at %lf", me->nextLevel);
	
	}


/******************************************************************************************
 *
 *	PppAssist()
 *	PppInfo()
 *
 *	Fairly generic Assist/Info methods
 *
 ******************************************************************************************/

void PppAssist(tPop* me, void* box, long iDir, long iArgNum, char* oCStr)
	{
	#pragma unused(me, box)
	
	LitterAssist(iDir, iArgNum, strIndexInDensity, strIndexOutPop, oCStr);
	}

void PppInfo(tPop* me)
	{ LitterInfo(kClassName, &me->coreObject.z_ob, (method) PppTattle); }


#pragma mark -
#pragma mark ¥ DSP Methods

/******************************************************************************************
 *
 *	PppDSP(me, ioDSPVectors, iConnectCounts)
 *
 ******************************************************************************************/

	static inline double CalcExponential()		// Exponential with lamda = 1
		{
		return log( ULong2Unit_zO(Taus88(NULL)) );
		}
	
	void PppCalcNext(tPop* me)
		{
		double			x,
						t;
		long			pw,
						s2n;
		
		t = me->threshhold;
		if (t > 0.0) {
			// Poisson-distributed pulse width
			pw	= -1;
			x	= 1.0;
			do {
				pw	+= 1;
				x	*= ULong2Unit_Zo(Taus88(NULL));
				} while (x > t);
			me->popWidth = pw;
			}
		else pw = me->popWidth;
		
		x = me->minusTau;
		if (x < 0.0) {
			x *= CalcExponential();
			}
		s2n = x * me->curSR + 0.5;
		s2n -= pw;
		me->sampsToNext = (s2n > 0) ? s2n : 0;
		
		x = ULong2Unit_zO(Taus88(NULL));		// x in range (0 .. 1]
		switch (me->sym) {
			case symNeg:
				x = -x;								// x in range [-1 .. 0)
				break;
			case symSym:
				x += x;								// x in range (0 .. 2]
				x -= 1.0;							// x in range (-1 .. 1]
				if (x == 0.0)						// Unlikely, but now:
					x = -1.0;						// x either in [-1 .. 0) or (0 .. 1]
				break;
			default:
				// Must be symPos; nothing to do
				break;
			}
		me->nextLevel = x;
		
		}

void
PppDSP(
	tPop*		me,
	t_signal**	ioDSPVectors,
	short		connectCounts[])
	
	{
	#pragma unused(connectCounts)
	
	me->curSR = ioDSPVectors[outletPop]->s_sr;
	
	if (me->sampsToNext < 0)
		PppCalcNext(me);
	
	dsp_add(
		PppPerform, 3,
		me, (long) ioDSPVectors[outletPop]->s_n, ioDSPVectors[outletPop]->s_vec
		);
	
	}
	

/******************************************************************************************
 *
 *	PppPerform(iParams)
 *
 *	Parameter block for PerformSync contains 8 values:
 *		- Address of this function
 *		- The performing schhh~ object
 *		- Vector size
 *		- output signal
 *		- Imaginary output signal
 *
 ******************************************************************************************/

	static long PopCorn(tPop* me, long vecCounter, tSampleVector oNoise)
		{
		double	curLevel	= me->curLevel,
				slope		= me->slope,
				goal		= me->nextLevel;
		long	sampsAtStart = vecCounter,
				popWidth	= me->popWidth,
				sampsThisTime;
		
		if (curLevel == 0.0) {
			// We're starting a brand new pop. Weee!
			slope = goal / popWidth;
			sampsThisTime = popWidth;
			if (sampsThisTime > vecCounter) {
				// Won't make it all the way up
				while (vecCounter-- > 0)
					*oNoise++ = curLevel += slope;
				goto exit;
				}
			// At this point we know that we can at least go as far as the peak
			vecCounter -= sampsThisTime;
			while (sampsThisTime-- > 0)
				*oNoise++ = curLevel += slope;
			// We're at the top (or bottom). Reverse slope and continue back to 0
			slope = -slope;
			sampsThisTime = popWidth;
			if (sampsThisTime > vecCounter)
				while (vecCounter-- > 0) *oNoise++ = curLevel += slope;
			else {
				vecCounter -= sampsThisTime;
				while (sampsThisTime-- > 0)
					*oNoise++ = curLevel += slope;
				slope	= 0.0;
				// curLevel should now be 0.
				// It's certainly very close, but we require exactitude
				curLevel = 0.0;
				}
			}
		
		else if ((goal > 0.0 && slope > 0.0)
					|| (goal < 0.0 && slope < 0.0)) {
			// We are picking up in the first half of a pop.
			sampsThisTime = (goal - curLevel) / slope + 0.5;
			if (sampsThisTime > vecCounter) {
				// We still won't make it all the way to the peak. Strange, but possible
				while (vecCounter-- > 0)
					*oNoise++ = curLevel += slope;
				goto exit;
				}
			// At this point we know that we can at least go as far as the peak
			vecCounter -= sampsThisTime;
			while (sampsThisTime-- > 0)
				*oNoise++ = curLevel += slope;
			// We're at the top (or bottom). Reverse slope and continue back to 0
			slope = - slope;
			sampsThisTime = popWidth;
			if (sampsThisTime > vecCounter)
				while (vecCounter-- > 0) *oNoise++ = curLevel += slope;
			else {
				vecCounter -= sampsThisTime;
				while (sampsThisTime-- > 0)
					*oNoise++ = curLevel += slope;
				slope = 0.0;
				// curLevel should now be 0.
				// It's certainly very close, but we require exactitude
				curLevel = 0.0;
				}
			}
		
		else {
			// Aha. We are picking up on the return half of a pop.
			sampsThisTime = -curLevel / slope + 0.5;
			if (sampsThisTime > vecCounter)
				while (vecCounter-- > 0) *oNoise++ = curLevel += slope;
			else {
				vecCounter -= sampsThisTime;
				while (sampsThisTime-- > 0)
					*oNoise++ = curLevel += slope;
				slope = 0.0;
				// curLevel should now be 0.
				// It's certainly very close, but we require exactitude
				curLevel = 0.0;
				}
			}
			
		if (curLevel == 0.0) {
			// Made it to end of pop.
			// That was fun. When can we do it again?
			PppCalcNext(me);
			}
	exit:
		// Don't forget to update state.
		me->slope		= slope;
		me->curLevel	= curLevel;
		
		return sampsAtStart - vecCounter;
		}

int*
PppPerform(
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
	tPop*			me = (tPop*) iParams[paramMe];
	long			sampsToNext;
	
	if (me->coreObject.z_disabled) goto exit;
	
	// Copy parameters into registers
	vecCounter	= (long) iParams[paramVectorSize];
	outNoise	= (tSampleVector) iParams[paramOut];
	sampsToNext	= (me->curLevel == 0.0) ? me->sampsToNext : 0;
	
	do {
		
		if (sampsToNext > 0) {
			long sampsThisTime = sampsToNext;
			if (sampsThisTime > vecCounter)
				sampsThisTime = vecCounter;
			
				// ASSERT:	0 < sampsThisTime ² vecCounter
				// 			0 < sampsThisTime ² sampsToNext
			vecCounter	-= sampsThisTime;
			sampsToNext	-= sampsThisTime;
			
			while (sampsThisTime-- > 0) *outNoise++ = 0.0;
			}
		
		else {
			long sampsDone = PopCorn(me, vecCounter, outNoise);
			
			vecCounter	-= sampsDone;
			outNoise	+= sampsDone;
			
			// This component may have been updated in PopCorn()
			sampsToNext	= me->sampsToNext;
			}
		
		} while (vecCounter > 0);
	
	me->sampsToNext = sampsToNext;
	
exit:
	return iParams + paramNextLink;
	}
