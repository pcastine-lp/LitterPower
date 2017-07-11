/*
	File:		poppy~.c

	Contains:	MSP external object generating sequences of populations from the classic
				(chaotic) population growth model: p' <- r·p·(1-p).
				This object is a hybrid combining lp.poppy and lp.frrr~

	Written by:	Peter Castine

	Copyright:	© 2002 Peter Castine

	Change History (most recent first):

         <6>      5–3–05    pc      Update for compatibility with CW 8 <cfloat> implementation. This
                                    was easier with this file, possibly because FLT_EPSILON was only
                                    assigned to a const double inside static functions.
         <5>     11–1–04    pc      Update for Windows.
         <4>    7–7–2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <3>  30–12–2002    pc      Drop faux 'Vers' resource and related modifications. Add object
                                    version to DoInfo().
         <2>  28–11–2002    pc      Tidy up after check in.
         <1>  28–11–2002    pc      Initial check in.
		31-Mar-2002:	Modified to make more like lp.frrr~
		30-Mar-2002:	First implementation.
*/


/******************************************************************************************
 ******************************************************************************************/


#pragma mark • Identify Target

#define __MAX_MSP_OBJECT__ 1


#pragma mark • Include Files

#include "LitterLib.h"
#include "TrialPeriodUtils.h"

#ifdef __MWERKS__
	#include <cfloat>									// For FLT_EPSILON
#endif													// Don't seem to need for GCC


#pragma mark • Constants

const char	kClassName[]	= "lp.poppy~";			// Class name



	// Indices for STR# resource
enum {
	strIndexInGrowth		= lpStrIndexLastStandard + 1,
	strIndexInSeed,
	strIndexInFreq,
	strIndexInInterp,
	
	strIndexOutPoppy,
	
	strIndexInLeft		= strIndexInGrowth,
	strIndexOutLeft		= strIndexOutPoppy
	};

	// Symbolic constants for interpolation
enum Interpolation {
	interpGeo			= -1,
	interpNone,
	interpLin,
	interpQuad,
	
	interpMin			= interpGeo,
	interpMax			= interpQuad
	};


#pragma mark • Type Definitions

typedef enum Interpolation tInterp;


#pragma mark • Object Structure

typedef struct {
	t_pxobject	coreObject;
	
	double			seed,
					curPop,
					growth,			// Nominal growth rate per cycle
					goal,			// Next population
					slope,			// Used for interpolation
					curve,			// Used for quadratic interpolation
					userFreq,		// As specified by user, in Hz
					curSR;			// Cache sample rate here (global value may not apply)
					
	unsigned long	spc,			// Samples per growth cycle; exact.
					sampsToGo;		// Keep track of this between Perform calls
	
	tInterp			interp;			// None, linear, quadratic, or geometric
	
	Boolean			seedPending;	// Flag if seed message arrives mid-cycle.
	} tPoppy;


#pragma mark • Global Variables



#pragma mark • Function Prototypes

	// Class message functions
void*		NewPoppy(Symbol*, short, Atom*);

	// Object message functions
static void DoGrowth(tPoppy*, double);
static void DoSeed(tPoppy*, double);
static void DoBaseFreq(tPoppy*, double);
static void DoInterp(tPoppy*, long);
static void DoReset(tPoppy*);

static void DoTattle(tPoppy*);
static void	DoAssist(tPoppy*, void* , long , long , char*);
static void	DoInfo(tPoppy*);

	// MSP Messages
static void	DoDSP(tPoppy*, t_signal**, short*);
static int*	PerformPoppyDynamic(int*);
static int*	PerformPoppyStatic(int*);


/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark -
#pragma mark • Inline Functions

static inline double NextPop(double iPop, double iGrowth)
			{ return iGrowth * iPop * (1.0 - iPop); }

static inline double GeoSlope(double iNow, double iGoal, unsigned long iSteps)
			{ return pow(iGoal/iNow, 1.0/(double)iSteps); }

static inline double LinSlope(double iNow, double iGoal, unsigned long iSteps)
			{ return (iGoal - iNow) / (double) iSteps; }

static inline double QuadSlope(double iNow, double iGoal, unsigned long iSteps, double* oCurve)
			{
			double	diff	= iGoal - iNow,
					durInv	= 0.5 / iSteps,
					durInv2	= durInv * durInv;
			
			*oCurve = -8.0 * diff * durInv2;
			return 4.0 * diff * (durInv - durInv2);
			}

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
	
	// Standard Max setup() call
	setup(	&gObjectClass,				// Pointer to our class definition
			(method) NewPoppy,			// Instance creation function
			(method) dsp_free,			// Std. MSP deallocation function
			sizeof(tPoppy),				// Class object size
			NIL,						// No menu function
			A_GIMME,					// Arguments:	1. Growth rate
										//				2. Seed population
										//				3. Samples per growth cycle
			0);							// Max could actually handle this as a type-checked
										// list, but the original design required A_GIMME
										// handling, so we'll stick to this approach.
										 
	dsp_initclass();
	
	// Messages
	addfloat((method) DoGrowth);
	addftx	((method) DoSeed,		1);
	addftx	((method) DoBaseFreq,	2);
	addinx	((method) DoInterp,		3);
	
	addmess	((method) DoReset,	"reset",	A_NOTHING);
	addmess	((method) DoTattle,	"dblclick",	A_CANT, 0);
	addmess	((method) DoTattle,	"tattle",	A_NOTHING);
	addmess	((method) DoAssist,	"assist",	A_CANT, 0);
	addmess	((method) DoInfo,	"info",		A_CANT, 0);
	
	// MSP messages
	LITTER_TIMEBOMB addmess	((method) DoDSP,	"dsp",	A_CANT, 0);

	// Initialize Litter Library
	LitterInit(kClassName, 0);

	}

#pragma mark -
#pragma mark • Utility Methods

	// Utility to calculate interpolation parameters
static inline void RecalcInterpParams(tPoppy* me)
	{
	const unsigned long	sampsToGo	= me->sampsToGo,
						interp		= me->interp;
	
	if (sampsToGo == 0 || interp == interpNone || me->spc <= 2) {
		me->slope	= 0.0;
		me->curve	= 0.0;
		}
	
	else if (interp == interpQuad) {
		me->slope	= QuadSlope(me->curPop, me->goal, me->sampsToGo, &(me->curve));
		}
	
	else {
		me->slope	= (interp == interpLin)
						? LinSlope(me->curPop, me->goal, me->sampsToGo)
						: GeoSlope(me->curPop, me->goal, me->sampsToGo);
		me->curve	= 0.0;
		}
	
	}

	// Utility to convert the (approximate) user-specified Base Frequency to
	// the equivalent number of samples at the current sampling rate
static inline void UpdateBaseFreq(tPoppy* me)
	{
	unsigned long	spc;				// Samples per cycle
	
	spc = me->curSR / me->userFreq + 0.5;
	if (spc == 0)
		spc = 1;
	
	me->spc = spc;
	
	if (me->sampsToGo > spc) {
		me->sampsToGo = spc;
		RecalcInterpParams(me);
		}
	
	}


#pragma mark -
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	NewPoppy(iName, iArgC, iArgV)
 *	
 *	Arguments: vector of up to three values (growth rate, seed value, base frequency)
 *	
 ******************************************************************************************/

void*
NewPoppy(
	Symbol*	sym,
	short	iArgC,
	Atom*	iArgV)
	
	{
	#pragma unused(sym)
	
#ifdef __MWERKS__
	const double	kDefGrowth	= 3.56994571869D,	// Smallest growth rate producing chaos
#else
	const double	kDefGrowth	= 3.56994571869,	// ditto, but GCC doesn't like the "D" suffix
#endif
					kDefSeed	= 0.0,				// This forces a very small positive value
													// for the seed population
					kDefBF		= 0.0;				// Base Frequency; zero indicates sample rate
	const tInterp	kDefInterp	= interpGeo;
					
	tPoppy*		me			= NIL;
	double		growth		= kDefGrowth,
				seed		= kDefSeed,
				baseFreq	= kDefBF;
	tInterp		interp		= kDefInterp;
	
	// Run through initialization parameters from right to left
	switch (iArgC) {
		default:
			error("%s: ignoring spurious arguments", kClassName);
			// fall into next case...
		case 4:
			if ( ParseAtom(iArgV + 3, true, false, 0, NIL, kClassName) )
				interp = iArgV[3].a_w.w_long;
		case 3:
			if ( ParseAtom(iArgV + 2, false, true, 0, NIL, kClassName) )
				baseFreq = iArgV[2].a_w.w_float;
			// fall into next case...
		case 2:
			if ( ParseAtom(iArgV + 1, false, true, 0, NIL, kClassName) )
				seed = iArgV[1].a_w.w_float;
			// fall into next case...
		case 1:
			if ( ParseAtom(iArgV, false, true, 0, NIL, kClassName) )
				growth = iArgV[0].a_w.w_float;
			// fall into next case...
		case 0:
			break;
		}


	//
	// Let Max allocate us, our inlets, and outlets
	//
	
	me = (tPoppy*) newobject(gObjectClass);
		if (me == NIL) goto punt;
	dsp_setup(&(me->coreObject), 1);			// Rate inlet is the only one accepting
												// signal vectors
	intin(me, 3);
	floatin(me, 2);
	floatin(me, 1);
	
	outlet_new(me, "signal");
	
	//
	// Store initial values
	//
	
	me->curSR		= sys_getsr();				// Just for starters, correct in DSP method.
	me->goal		= 0.0;
	me->slope		= 0.0;
	me->curve		= 0.0;
	me->sampsToGo	= 0;
	me->spc			= 1;						// Preset this so DoBaseFreq() is well behaved.
	
	DoInterp(me, interp);						// Sets me->interp
	DoBaseFreq(me, baseFreq);					// Sets me->userFreq, me->spc
	DoSeed(me, seed);							// Sets me->seed, me->curPop, me->seedPending
	DoGrowth(me, growth);						// Sets me->growth
	
	//
	// All done
	//
	
punt:
	return me;
	}


#pragma mark -
#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	DoGrowth(me, iGrowth)
 *	DoSeed(me, iSeed)
 *	DoBaseFreq(me, iUserFreq)
 *	DoInterp(me, iInterp)
 *
 ******************************************************************************************/

static void
DoGrowth(
	tPoppy* me,
	double iGrowth)
	
	{
	const double	kMinGrowth	= FLT_EPSILON,
					kMaxGrowth	= 4.0;
		// See DoSeed() about why FLT_EPSILON
	
	if (iGrowth < kMinGrowth)		iGrowth = kMinGrowth;
	else if (iGrowth > kMaxGrowth)	iGrowth = kMaxGrowth;
	
	me->growth = iGrowth;
	}


static void
DoSeed(
	tPoppy*	me,
	double	iSeed)
	
	{
	const double	kMinSeed	= FLT_EPSILON,
					kMaxSeed	= 1.0 - FLT_EPSILON;
		// FLT_EPSILON instead of DBL_EPSILON for somewhat dubious reasons:
		// 1) Although arithmetic is done in double precision, signals are single-precision.
		// 2) DBL_EPSILON requires linking in MSL C.PPC.lib, which generates bunches of
		//	spurious double-defined warnings from the linker, which tends to cover up real
		//	warnings that I want to know about. It's possible to create a local version of
		//	__dbl_epsilon, but that's deprecated in some circles.
	
	if (iSeed < kMinSeed)		iSeed = kMinSeed;
	else if (iSeed > kMaxSeed)	iSeed = kMaxSeed;
	
	me->seed = iSeed;
	
	if (me->sampsToGo > 0)
		me->seedPending = true;
	else {
		// ASSERT: me->sampsToGo == 0
		// Update curPop now, in case the special handling of me->spc <= 2 is in effect.
		// This is harmless for larger values of spc.
		me->curPop = me->goal = iSeed;
		me->seedPending = false;
		}
	
	}

static void
DoBaseFreq(
	tPoppy*	me,
	double	iFreq)
	
	{
	
	if (iFreq <= 0.0)
		iFreq = me->curSR;
	
	me->userFreq = iFreq;
	UpdateBaseFreq(me);
	
	}
	
	
static void
DoInterp(
	tPoppy* me,
	long iInterp)
	
	{
	
	if (iInterp < interpMin)		iInterp = interpMin;
	else if (iInterp > interpMax)	iInterp = interpMax;
	
	if (me->interp != iInterp) {
		// Don't want to call RecalcInterpParams() unnecessarily
		me->interp = iInterp;
		RecalcInterpParams(me);
		}
	
	}

/******************************************************************************************
 *
 *	DoReset(me)
 *
 *	Post state information
 *
 ******************************************************************************************/

static void
DoReset(
	tPoppy* me)
	
	{
	
	if (me->sampsToGo > 0)
		me->seedPending = true;
	else {
		me->curPop = me->goal = me->seed;
		me->seedPending = false;
		}
	
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
	tPoppy* me)
	
	{
	
	post("%s state", kClassName);
	post("  Current population is %lf (goal is %lf, from seed %lf)",
			me->curPop, me->goal, me->seed);
	post("  (Static) growth rate per cycle is %lf", me->growth);
	post("  Base frequency of growth cycle:");
	post("    you specified %lf Hz,", me->userFreq);
	post("    last time we looked, the sampling rate was %lf Hz", me->curSR);
	post("    so we reckon %lu samples per growth cycle (that should be %lf Hz)",
			me->spc, me->curSR / me->spc);
	
	if (me->spc > 2)
		post("  %ld samples left in current cycle", me->sampsToGo);
	
	switch (me->interp) {
		case interpQuad:
			post("    Quadratic interpolation, slope is currently %lf, curvature is %lf",
				me->slope, me->curve);
			break;
		case interpLin:
			post("    Linear interpolation, slope is %lf", me->slope);
			break;
		case interpNone:
			post("    No interpolation");
			break;
		default:
			// Must be Geometric interpolation
			post("  Geometric interpolation: growth per sample is: %lf", me->slope);
			break;
		}
	
	if (me->seedPending)
		post("  Next seed population is: %lf", me->seed);
	else post("  Initial population was: %lf", me->seed);
	
	}


/******************************************************************************************
 *
 *	DoAssist()
 *	DoInfo()
 *
 *	Generic Assist/Info methods
 *	
 *	Many parameters are not used.
 *
 ******************************************************************************************/

void DoAssist(tPoppy* me, void* box, long iDir, long iArgNum, char* oCStr)
	{
	#pragma unused(me, box)
	
	LitterAssist(iDir, iArgNum, strIndexInLeft, strIndexOutLeft, oCStr);
	}

void DoInfo(tPoppy* me)
	{ LitterInfo(kClassName, &me->coreObject.z_ob, (method) DoTattle); }

#pragma mark -
#pragma mark • DSP Methods

/******************************************************************************************
 *
 *	DoDSP(me, ioDSPVectors, iConnectCounts)
 *
 ******************************************************************************************/

void
DoDSP(
	tPoppy*		me,
	t_signal**	iDSPVectors,
	short*		iConnectCounts)
	
	{
	enum {
		inletGrowth			= 0,
		outletPoppy
		};

	
	if (iConnectCounts[outletPoppy] == 0)
		return;
	
	me->curSR = iDSPVectors[outletPoppy]->s_sr;
	UpdateBaseFreq(me);
	
	if ( iConnectCounts[inletGrowth] > 0 ) {
	
		dsp_add(
			PerformPoppyDynamic, 4, me,
			(long) iDSPVectors[outletPoppy]->s_n,
			iDSPVectors[inletGrowth]->s_vec,
			iDSPVectors[outletPoppy]->s_vec
			);
		
		}
	
	else {
		
		dsp_add(
			PerformPoppyStatic, 3, me, 
			(long) iDSPVectors[outletPoppy]->s_n,
			iDSPVectors[outletPoppy]->s_vec
			);
		
		}
	
	}
	

/******************************************************************************************
 *
 *	PerformPoppyStatic(iParams)
 *
 *	Parameter block contains the following values values:
 *		- Address of the function
 *		- The performing poppy~ object
 *		- Vector size
 *		- output signal
 *		- Address of the next link in the parameter chain
 *
 ******************************************************************************************/

	static void PoppyStat1Step(tPoppy* me, long iVecSize, tSampleVector oPoppy)
		{
		double	curPop	= me->curPop,
				growth	= me->growth;
		
		do {
			*oPoppy++ = curPop = NextPop(curPop, growth);
			} while (--iVecSize > 0);
		
		me->curPop = curPop;
		}
	
	static void PoppyStat2Step(tPoppy* me, long iVecSize, tInterp iInterp, tSampleVector oPoppy)
		{
		// ASSERT: me->sampsToGo == 0
		// As long as vector sizes are guaranteed to be powers of 2, the assertion holds
		// NB:	Brief testing indicates that the above special-case is, indeed, a
		//		performance optimization
		const double	growth		= me->growth;
		
		double	curPop,
				goal	= me->goal;
		
		switch (iInterp) {
			case interpNone:
				do {
					curPop		= goal;
					goal		= NextPop(curPop, growth);
					*oPoppy++	= curPop;
					*oPoppy++	= curPop;
					} while ((iVecSize -= 2) > 0);
				break;
			
			case interpLin:
				do {
					curPop		= goal;
					goal		= NextPop(curPop, growth);
					*oPoppy++	= curPop;
					*oPoppy++	= (curPop + goal) * 0.5;
					} while ((iVecSize -= 2) > 0);
					
				curPop = oPoppy[-1];		// Store last sample in case base freq changes.
				break;
			
			default:
				// Both quadratic and geometric interpolation behave identically at
				// two samples/cycle
				do {
					curPop		= goal;
					goal		= NextPop(curPop, growth);
					*oPoppy++	= curPop;
					*oPoppy++	= curPop = sqrt(curPop * goal);
					} while ((iVecSize -= 2) > 0);
				
				curPop = oPoppy[-1];		// Store last sample in case base freq changes.
				break;
			}
		
		me->curPop	= curPop;
		me->goal	= goal;
		}
	
	static void PoppyStatNoInterp(tPoppy* me, long iVecSize, unsigned long iSPC, tSampleVector oPoppy)
		{
		const double	growth		= me->growth;
		
		double			curPop		= me->curPop,
						goal		= me->goal;
		unsigned long	sampsToGo	= me->sampsToGo;
		Boolean			seedPending	= me->seedPending;
						
		do {
			unsigned long sampsThisTime;
			
			if (sampsToGo == 0) {
				if (seedPending) {
					goal = me->seed;
					me->seedPending = seedPending = false;
					}
				curPop 		= goal;
				goal		= NextPop(curPop, growth);
				sampsToGo	= iSPC;
				}
			
			sampsThisTime = sampsToGo;
			if (sampsThisTime > iVecSize)
				sampsThisTime = iVecSize;
				
				// ASSERT:	sampsThisTime ≤ iVecSize
				// 			sampsThisTime ≤ sampsToGo
			iVecSize -= sampsThisTime;
			sampsToGo -= sampsThisTime;
			
			do { *oPoppy++ = curPop; } while (--sampsThisTime > 0);
				
			} while (iVecSize > 0);
		
		me->curPop		= curPop;
		me->goal		= goal;
		me->sampsToGo	= sampsToGo;
		me->seedPending	= seedPending;
		}
	
	static void PoppyStatLin(tPoppy* me, long iVecSize, unsigned long iSPC, tSampleVector oPoppy)
		{
		const double	growth		= me->growth;
		
		double			curPop		= me->curPop,
						goal		= me->goal,
						slope		= me->slope;
		unsigned long	sampsToGo	= me->sampsToGo;
						
		do {
			unsigned long sampsThisTime;
			
			if (sampsToGo == 0) {
				curPop		= goal;
				goal		= NextPop(curPop, growth);
				slope		= LinSlope(curPop, goal, iSPC);
				sampsToGo	= iSPC;
				}
			
			sampsThisTime = sampsToGo;
			if (sampsThisTime > iVecSize)
				sampsThisTime = iVecSize;
				
				// ASSERT:	sampsThisTime ≤ iVecSize
				// 			sampsThisTime ≤ sampsToGo
			iVecSize -= sampsThisTime;
			sampsToGo -= sampsThisTime;
			
			do { *oPoppy++ = curPop += slope; } while (--sampsThisTime > 0);
				
			} while (iVecSize > 0);
		
		me->curPop		= curPop;
		me->goal		= goal;
		me->sampsToGo	= sampsToGo;
		me->slope		= slope;
		}
	
	static void PoppyStatQuad(tPoppy* me, long iVecSize, long iSPC, tSampleVector oPoppy)
		{
		const double	growth		= me->growth;
		
		double			curPop		= me->curPop,
						goal		= me->goal,
						slope		= me->slope,
						curve		= me->curve;
		unsigned long	sampsToGo	= me->sampsToGo;
		
		do {
			unsigned long sampsThisTime;
			
			if (sampsToGo == 0) {
//				double	diff, durInv, durInv2;
				curPop		= goal;
				goal		= NextPop(curPop, growth);
				slope		= QuadSlope(curPop, goal, iSPC, &curve);
				sampsToGo	= iSPC;
				
//				diff		= goal - curPop;
//				durInv		= 0.5 / sampsToGo;			// Take parabolic curve over twice
//				durInv2		= durInv * durInv;			// the distance between random samples
				
//				slope		= 4.0 * diff * (durInv - durInv2);
//				curve		= -8.0 * diff * durInv2;
				}
			
			sampsThisTime = sampsToGo;
			if (sampsThisTime > iVecSize)
				sampsThisTime = iVecSize;
			
				// ASSERT:	sampsThisTime ≤ iVecSize
				// 			sampsThisTime ≤ sampsToGo
			iVecSize -= sampsThisTime;
			sampsToGo -= sampsThisTime;
			
			do {
				*oPoppy++ = curPop += slope;
				slope += curve;
				} while (--sampsThisTime > 0);
				
			} while (iVecSize > 0);
		
		me->curPop		= curPop;
		me->goal		= goal;
		me->slope		= slope;
		me->curve		= curve;
		me->sampsToGo	= sampsToGo;
		}
	
	static void PoppyStatGeo(tPoppy* me, long iVecSize, long iSPC, tSampleVector oPoppy)
		{
		const double	growth		= me->growth;
		
		double			curPop		= me->curPop,
						goal		= me->goal,
						slope		= me->slope;
		unsigned long	sampsToGo	= me->sampsToGo;
						
		do {
			unsigned long sampsThisTime;
			
			if (sampsToGo == 0) {
				curPop		= goal;
				goal		= NextPop(curPop, growth);
				slope		= GeoSlope(curPop, goal, iSPC);
				sampsToGo	= iSPC;
				}
			
			sampsThisTime = sampsToGo;
			if (sampsThisTime > iVecSize)
				sampsThisTime = iVecSize;
				
				// ASSERT:	sampsThisTime ≤ iVecSize
				// 			sampsThisTime ≤ sampsToGo
			iVecSize -= sampsThisTime;
			sampsToGo -= sampsThisTime;
			
			do { *oPoppy++ = curPop *= slope; } while (--sampsThisTime > 0);
				
			} while (iVecSize > 0);
		
		me->curPop		= curPop;
		me->goal		= goal;
		me->sampsToGo	= sampsToGo;
		me->slope		= slope;
		}

int*
PerformPoppyStatic(
	int* iParams)
	
	{
	enum {
		paramFuncAddress	= 0,
		paramMe,
		paramVectorSize,
		paramOut,
		
		paramNextLink
		};
	
	unsigned long	spc;
	tInterp			interp;
	tPoppy*			me = (tPoppy*) iParams[paramMe];
	
	if (me->coreObject.z_disabled) goto exit;
	
	// Copy parameters into registers
	spc			= me->spc;
	interp		= me->interp;
	
	switch (spc) {
		case 1:
			// We don't need to worry about interpolation
			PoppyStat1Step(
				me, (long) iParams[paramVectorSize], (tSampleVector) iParams[paramOut]);
			break;
		
		case 2:
			// We can optimize interpolation
			PoppyStat2Step(
				me, (long) iParams[paramVectorSize], interp, (tSampleVector) iParams[paramOut]);
			break;
		
		default:
			switch (me->interp) {
				case interpNone:
					PoppyStatNoInterp(
						me, (long) iParams[paramVectorSize], spc, (tSampleVector) iParams[paramOut]);
					break;
				
				case interpQuad:
					PoppyStatQuad(
						me, (long) iParams[paramVectorSize], spc, (tSampleVector) iParams[paramOut]);
					break;
				
				case interpLin:
					PoppyStatLin
						(me, (long) iParams[paramVectorSize], spc, (tSampleVector) iParams[paramOut]);
					break;
				
				default:
					// Must be interpGeo
					PoppyStatGeo(
						me, (long) iParams[paramVectorSize], spc, (tSampleVector) iParams[paramOut]);
					break;
				}
			break;
		}
			
exit:
	return iParams + paramNextLink;
	}


/******************************************************************************************
 *
 *	PerformPoppyDynamic(iParams)
 *
 *	Parameter block contains the following values values:
 *		- Address of the function
 *		- The performing poppy~ object
 *		- Vector size
 *		- Input growth rate signal
 *		- output signal
 *		- Address of the next link in the parameter chain
 *
 ******************************************************************************************/

	static void
	PoppyDyn1Step(
		tPoppy*			me,
		long			iVecSize,
		tSampleVector	iGrowth,
		tSampleVector	oPoppy)
		{
		double	curPop	= me->curPop;
		
		do {
			*oPoppy++ = curPop = NextPop(curPop, *iGrowth++);
			} while (--iVecSize > 0);
		
		me->curPop = curPop;
		}

	static void
	PoppyDyn2Step(
		tPoppy*			me,
		long			iVecSize,
		tInterp			iInterp,
		tSampleVector	iGrowth,
		tSampleVector	oPoppy)
		{
		// ASSERT: me->sampsToGo == 0
		// As long as vector sizes are guaranteed to be powers of 2, the assertion holds
		double	curPop,
				goal	= me->goal;
		
		switch (iInterp) {
			case interpNone:
				do {
					curPop		= goal;
					goal		= NextPop(curPop, *iGrowth);
					iGrowth		+= 2;
					*oPoppy++	= curPop;
					*oPoppy++	= curPop;
					} while ((iVecSize -= 2) > 0);
				break;
			
			case interpLin:
				do {
					curPop		= goal;
					goal		= NextPop(curPop, *iGrowth);
					iGrowth		+= 2;
					*oPoppy++	= curPop;
					*oPoppy++	= (curPop + goal) * 0.5;
					} while ((iVecSize -= 2) > 0);
					
				curPop = oPoppy[-1];		// Store last sample in case base freq changes.
				break;
			
			default:
				// Both quadratic and geometric interpolation behave identically at
				// two samples/cycle
				do {
					curPop		= goal;
					goal		= NextPop(curPop, *iGrowth);
					iGrowth		+= 2;
					*oPoppy++	= curPop;
					*oPoppy++	= curPop = sqrt(curPop * goal);
					} while ((iVecSize -= 2) > 0);
				
				curPop = oPoppy[-1];		// Store last sample in case base freq changes.
				break;
			}
		
		me->curPop	= curPop;
		me->goal	= goal;
		}
	
	static void PoppyDynNoInterp(
			tPoppy*			me,
			long			iVecSize,
			unsigned long	iSPC,
			tSampleVector	iGrowth,
			tSampleVector	oPoppy)
		
		{
		double			curPop		= me->curPop,
						goal		= me->goal;
		unsigned long	sampsToGo	= me->sampsToGo;
						
		do {
			unsigned long sampsThisTime;
			
			if (sampsToGo == 0) {
				curPop		= goal;
				goal		= NextPop(curPop, *iGrowth);
				sampsToGo	= iSPC;
				}
			
			sampsThisTime = sampsToGo;
			if (sampsThisTime > iVecSize)
				sampsThisTime = iVecSize;
				
				// ASSERT:	sampsThisTime ≤ iVecSize
				// 			sampsThisTime ≤ sampsToGo
			iGrowth		+= sampsThisTime;
			iVecSize	-= sampsThisTime;
			sampsToGo	-= sampsThisTime;
			
			do { *oPoppy++ = curPop; } while (--sampsThisTime > 0);
				
			} while (iVecSize > 0);
		
		me->curPop		= curPop;
		me->goal		= goal;
		me->sampsToGo	= sampsToGo;
		}
	
	static void PoppyDynLin(
			tPoppy*			me,
			long			iVecSize,
			unsigned long	iSPC,
			tSampleVector	iGrowth,
			tSampleVector	oPoppy)
		
		{
		double			curPop		= me->curPop,
						goal		= me->goal,
						slope		= me->slope;
		unsigned long	sampsToGo	= me->sampsToGo;
						
		do {
			unsigned long sampsThisTime;
			
			if (sampsToGo == 0) {
				curPop		= goal;
				goal		= NextPop(curPop, *iGrowth);
				slope		= LinSlope(curPop, goal, iSPC);
				sampsToGo	= iSPC;
				}
			
			sampsThisTime = sampsToGo;
			if (sampsThisTime > iVecSize)
				sampsThisTime = iVecSize;
				
				// ASSERT:	sampsThisTime ≤ iVecSize
				// 			sampsThisTime ≤ sampsToGo
			iVecSize	-= sampsThisTime;
			sampsToGo	-= sampsThisTime;
			iGrowth		+= sampsThisTime;
			
			do { *oPoppy++ = curPop += slope; } while (--sampsThisTime > 0);
				
			} while (iVecSize > 0);
		
		me->curPop		= curPop;
		me->goal		= goal;
		me->sampsToGo	= sampsToGo;
		me->slope		= slope;
		}

	static void PoppyDynQuad(
			tPoppy*			me,
			long			iVecSize,
			unsigned long	iSPC,
			tSampleVector	iGrowth,
			tSampleVector	oPoppy)
		
		{
		double			curPop		= me->curPop,
						goal		= me->goal,
						slope		= me->slope,
						curve		= me->curve;
		unsigned long	sampsToGo	= me->sampsToGo;
		
		do {
			unsigned long sampsThisTime;
			
			if (sampsToGo == 0) {
				curPop		= goal;
				goal		= NextPop(curPop, *iGrowth);
				slope		= QuadSlope(curPop, goal, iSPC, &curve);
				sampsToGo	= iSPC;
				}
			
			sampsThisTime = sampsToGo;
			if (sampsThisTime > iVecSize)
				sampsThisTime = iVecSize;
			
				// ASSERT:	sampsThisTime ≤ iVecSize
				// 			sampsThisTime ≤ sampsToGo
			iVecSize	-= sampsThisTime;
			sampsToGo	-= sampsThisTime;
			iGrowth		+= sampsThisTime;
			
			do {
				*oPoppy++ = curPop += slope;
				slope += curve;
				} while (--sampsThisTime > 0);
				
			} while (iVecSize > 0);
		
		me->curPop		= curPop;
		me->goal		= goal;
		me->slope		= slope;
		me->curve		= curve;
		me->sampsToGo	= sampsToGo;
		}
	
	static void PoppyDynGeo(
			tPoppy*			me,
			long			iVecSize,
			unsigned long	iSPC,
			tSampleVector	iGrowth,
			tSampleVector	oPoppy)
		{
		double			curPop		= me->curPop,
						goal		= me->goal,
						slope		= me->slope;
		unsigned long	sampsToGo	= me->sampsToGo;
						
		do {
			unsigned long sampsThisTime;
			
			if (sampsToGo == 0) {
				curPop		= goal;
				goal		= NextPop(curPop, *iGrowth);
				slope		= GeoSlope(curPop, goal, iSPC);
				sampsToGo	= iSPC;
				}
			
			sampsThisTime = sampsToGo;
			if (sampsThisTime > iVecSize)
				sampsThisTime = iVecSize;
				
				// ASSERT:	sampsThisTime ≤ iVecSize
				// 			sampsThisTime ≤ sampsToGo
			iVecSize	-= sampsThisTime;
			sampsToGo	-= sampsThisTime;
			iGrowth		+= sampsThisTime;
			
			do { *oPoppy++ = curPop *= slope; } while (--sampsThisTime > 0);
				
			} while (iVecSize > 0);
		
		me->curPop		= curPop;
		me->goal		= goal;
		me->sampsToGo	= sampsToGo;
		me->slope		= slope;
		}


int*
PerformPoppyDynamic(
	int* iParams)
	
	{
	enum {
		paramFuncAddress	= 0,
		paramMe,
		paramVectorSize,
		paramGrowth,
		paramOut,
		
		paramNextLink
		};
	
	unsigned long	spc;
	tInterp			interp;
	tPoppy*			me = (tPoppy*) iParams[paramMe];
	
	if (me->coreObject.z_disabled) goto exit;
	
	// Copy parameters into registers
	spc			= me->spc;
	interp		= me->interp;
	
	switch (spc) {
		case 1:
			// We don't need to worry about interpolation
			PoppyDyn1Step(	me,
							(long) iParams[paramVectorSize],
							(tSampleVector) iParams[paramGrowth],
							(tSampleVector) iParams[paramOut] );
			break;
		
		case 2:
			// We can optimize interpolation
			PoppyDyn2Step(	me,
							(long) iParams[paramVectorSize],
							interp,
							(tSampleVector) iParams[paramGrowth],
							(tSampleVector) iParams[paramOut] );
			break;
		
		default:
			switch (me->interp) {
				case interpNone:
					PoppyDynNoInterp(
									me,
									(long) iParams[paramVectorSize],
									spc,
									(tSampleVector) iParams[paramGrowth],
									(tSampleVector) iParams[paramOut]);
					break;
				
				case interpLin:
					PoppyDynLin(	me,
									(long) iParams[paramVectorSize],
									spc,
									(tSampleVector) iParams[paramGrowth],
									(tSampleVector) iParams[paramOut]);
					break;
				
				case interpQuad:
					PoppyDynQuad(	me,
									(long) iParams[paramVectorSize],
									spc,
									(tSampleVector) iParams[paramGrowth],
									(tSampleVector) iParams[paramOut]);
					break;
				
				default:
					// Must be interpGeo
					PoppyDynGeo(	me,
									(long) iParams[paramVectorSize],
									spc,
									(tSampleVector) iParams[paramGrowth],
									(tSampleVector) iParams[paramOut]);
					break;
				}
			break;
		}
	
exit:
	return iParams + paramNextLink;
	}
