/*
	File:		lya.c

	Contains:	Max external object generating sequences of populations from the classic
				(chaotic) population growth model: p' <- r·p·(1-p)

	Written by:	Peter Castine

	Copyright:	© 2002 Peter Castine

	Change History (most recent first):

         <6>      5–3–05    pc      Update for compatibility with CW 8 <cfloat> implementation.
         <5>     11–1–04    pc      Update for Windows.
         <4>    7–7–2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <3>    6–3–2003    pc      Drop faux 'Vers' resource and related modifications. Add object
                                    version to DoInfo().
         <2>  28–11–2002    pc      Tidy up after check in.
         <1>  28–11–2002    pc      Initial check in.
		 2-Apr-2002:	Modified inlet order to match poppy~
		25-Mar-2002:	First implementation.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "LitterLib.h"
#include "TrialPeriodUtils.h"

#ifdef __MWERKS__
	#include <cfloat>		// For FLT_EPSILON
#endif

#include <math.h>			// For fabs(), log()

#pragma mark • Constants

const char	kClassName[]		= "lp.lya";			// Class name


	// The following macro used to be "properly" defined as const doubles, but with
	// changes to <cfloat> I can't figure out how to assign the new value of FLT_EPSILON
	// to a const double at compile time. So I've reverted to using preprocessor macros.
#define kMinGrowth	FLT_EPSILON

const int		kMinCycle	= 1,
				kMaxCycle	= 8095;
const double	k1OverLog2	= 1.4426950409,			// 1/log(2), needed for Lyapunov calculation
				kLog2		= 0.6931471806,			// Also needed for the optimization strategy
				kDefSeed	= 2.0 / 3.0,			// Good seed for most growth rates
				kMaxGrowth	= 4.0,
				kDefGrowth	= 3.56994571869;		// Smallest  growth rate producing chaos

	// Empirically determined, seem to work well
const long		kDefSkip		= 32,		// Skip this number of initial iterations
				kDefIter		= 1000;		// Use this number of samples to estimate mean
	


	// Indices for STR# resource
enum {
	strIndexInRates		= lpStrIndexLastStandard + 1,
	strIndexPrecision,
	
	strIndexOutLya,
	
	strIndexInLeft		= strIndexInRates,
	strIndexOutLeft		= strIndexOutLya
	};


#pragma mark • Type Definitions



#pragma mark • Object Structure

typedef struct {
	Object			coreObject;
	
	double			growth,				// Used for single-value growth rate
					lya;				// Lyapunov exponent
					
	long			iter,				// Controls how many iterations we use
										// when empirically estimating the value
					cycleLen,			// Cycle length
					allocation;			// Bytes allocated (may be more than needed)
	
	float*			rates;				// Variable length array of growth rate values;
										// Used when cycleLen > 1
	} tLya;


#pragma mark • Global Variables



#pragma mark • Function Prototypes

	// Class message functions
void*		NewLya(long);
void		FreeLya(tLya*);

	// Object message functions
static void DoBang(tLya*);
static void Do1Rate(tLya*, double);					// Rate can be a single float...
static void DoRates(tLya*, Symbol*, short, Atom*);	// ... or a list
static void DoSet(tLya*, Symbol*, short, Atom*);
static void DoPrecision(tLya*, long);

static void DoTattle(tLya*);
static void	DoAssist(tLya*, void* , long , long , char*);
static void	DoInfo(tLya*);

	// Helper methods
static void		Set1Rate(tLya*, double);
static Boolean	SetRates(tLya*, short, Atom*);

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
main(void)				// Parameter is obsolete (68k legacy)
	
	{
	LITTER_CHECKTIMEOUT(kClassName);
		
	// Standard Max setup() call
	setup(	&gObjectClass,				// Pointer to our class definition
			(method) NewLya,			// Instance creation function
			(method) FreeLya,			// Custom deallocation function
			sizeof(tLya),				// Class object size
			NIL,						// No menu function
			A_DEFLONG,					// Arguments:	Precision (optional)
			0);
		
	// Messages
	LITTER_TIMEBOMB addbang	((method) DoBang);
	LITTER_TIMEBOMB addfloat((method) Do1Rate);
	LITTER_TIMEBOMB addmess	((method) DoRates,	"list",		A_GIMME, 0);
	addmess	((method) DoSet,	"set",		A_GIMME, 0);
	addinx	((method) DoPrecision,	1);
	
	addmess	((method) DoTattle,	"dblclick",	A_CANT, 0);
	addmess	((method) DoTattle,	"tattle",	A_NOTHING);
	addmess	((method) DoAssist,	"assist",	A_CANT, 0);
	addmess	((method) DoInfo,	"info",		A_CANT, 0);
	
	// Initialize Litter Library
	LitterInit(kClassName, 0);

	}

#pragma mark -
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	NewLya(iName, iArgC, iArgV)
 *	
 *	Arguments:
 *		iName is always "lp.poppy" and we do not use this paramter
 *		iArgC and iArgV specify a list of atoms, which will normally all be floats.
 *		If the last item is an integer, it is the precision
 *	
 ******************************************************************************************/

void*
NewLya(
	long	iPrecision)
	
	{
	tLya*		me			= NIL;
	
	//
	// Let Max allocate us, our inlets, and outlets
	//
	me = (tLya*) newobject(gObjectClass);
		if (me == NIL) goto punt;
	
		// One extra inlet
	intin(me, 1);
	
		// One outlet, accessed through me->coreObject.o_outlet
	floatout(me);
	
	//
	// Initialize components
	//
	
	me->growth		= 0.0;
	me->lya			= 0.0;
	me->cycleLen	= 0;
	me->allocation	= 0;
	me->rates		= NIL;
	
	DoPrecision(me, iPrecision);
	Do1Rate(me, 0.0);
	
	//
	// All done
	//
	
	return me;

punt:
	// Oops... cheesy exception handling
	error("Insufficient memory to create %s object.", kClassName);
	if (me) FreeLya(me);
	
	return NIL;
	}

/******************************************************************************************
 *
 *	FreeLya(me)
 *	FreeProxies(iProxyList);
 *	
 ******************************************************************************************/

void
FreeLya(
	tLya* me)
	
	{
	
	if (me == NIL)		// Sanity check
		return;		
	
	if (me->rates)
		freebytes(me->rates, me->allocation);
		
	}
	

#pragma mark -
#pragma mark • Helper Functions

/******************************************************************************************
 *
 *	CalcLya1Rate
 *	CalcLyaRates
 *
 ******************************************************************************************/

static double CalcLya1Rate(double r, long iter)
	{
	double	l,			// Lyapunov exponent is calculated here
			p;			// Current population
	long	i;			// Loop index
	
	if (r <= 1.0) {
		// The population goes assymptotically to zero, which means that the Lyapunov
		// exponent goes to the limit log2(r)
		
		return k1OverLog2 * log(r);
		}
	
	else if (r <= 3.0) {
		// The population stabilizes to p = rp(1-p), which, after applying the binomial
		// formula, solves to p = |r-1|/r (the complementary solution is zero).
		// Inserting into the formula for the Lyapunov exponent, this reduces to
		// log2(|2.0 - r|).
		// 
		// Note that the Lyapunov exponent for r = 2 is -infinity.
		// Don't blame me, it wasn't my idea.
		
		return k1OverLog2 * log(fabs(2.0 - r));
		}
	
	//
	//	OK, r > 3, so brute force this...
	//

	// Skip a few initial values to settle down to whatever we're settling down to
	for (i = kDefSkip * iter, p = kDefSeed; i-- > 0; )
		p *= r * (1.0 - p);

	// Calculate Lyapunov coefficient
	//
	//		l	= 1/N * sum(log2|r-2rp|)
	//			= 1/N * log2(product(|r-2rp|)
	//			= 1/N * log2(product(|2rp-r|)
	//			= 1/N * log2(|product(2rp-r)|
	//
	// and, furthermore (using PIN to represent the product over N iterations):
	//
	//		PIN(2rp-r)	= PIN(r(2p-1))
	//					= PIN(r) * PIN(2p-1)
	//					= r^N * PIN(2p-1)
	//
	// which allows the following substitution for the second term above
	// (and noting that r is always positive):
	//
	//		l	= 1/N * log2(|r^N * PIN(2p-1)|)
	//			= 1/N * (log2(r^N) + log2(|PIN(2p-1)|)
	//			= 1/N * (N*log2(r) + log2(|PIN(2p-1)|)
	//			= log2(r) + log2(|PIN(2p-1)|)/N
	//
	// Unfortunately, the mathematics break on the limitations of 64-bit floating
	// point representation. For any reasonable number of iterations, the cumulative
	// product is likely to either overflow or underflow. Currently, the only optimization
	// I'm using is to convert from base-e to base-2 logarithms outside the main loop.
	// I could take the multiplication with r outside the main loop, but I don't think
	// the difference in processing cost is significant, given that there's a log() inside
	// the loop. Another possibility, if we need to safe cycles, would be to take some
	// number of iterations (say 16 or 32) with the product formula, and then log. In
	// trials so far, even the cumulative product of 32 iterations can underflow. Although
	// there are strategies to get around this, I have shelved further attempts at
	// optimization until there is a need for audio-rate generation of this function.
	
	iter *= kDefIter;
	for (i = iter, l = 0.0; i-- > 0; ) {
		p *= r * (1.0 - p);
		l += log(fabs(r * (p + p - 1.0)));
		}
	l /= (double) iter;
	l *= k1OverLog2;
	
	return l;
	}

static double CalcLyaRates(long c, float* r, long iter)
	{
	double	p,
			l;
	float*	rr;
	long	i, j;
	
	// Skip enough initial values to settle down to whatever we're settling down to
	for (p = kDefSeed, i = kDefSkip * c; i-- > 0; )
		for (rr = r, j = c; j-- > 0; ) p *= *rr++ * (1.0 - p);
	
	// Calculate Lyapunov coefficient
	l = 0.0;
	iter *= kDefIter;
	iter /= c;
	iter += 1;								// Round up rather than down
	for (i = iter, l = 0.0; i-- > 0; ) {
		for (rr = r, j = c; j-- > 0; ) {
			double r = *rr++;				// This hides the function parameter r,
											// which is just fine and dandy.
			p *= r * (1.0 - p);
			l += log(fabs(r * (p + p - 1.0)));
			}
		}
	l /= (double) (iter * c);
	l *= k1OverLog2;
	
	return l;
	}

/******************************************************************************************
 *
 *	Set1Rate(me, iGrowth)
 *	SetRates(me, iName, iArgC, iArgV)
 *
 ******************************************************************************************/

static void Set1Rate(tLya* me, double iGrowth)
	{
	if (iGrowth < kMinGrowth)		iGrowth = kMinGrowth;
	else if (iGrowth > kMaxGrowth)	iGrowth = kMaxGrowth;
	
	me->growth		= iGrowth;
	me->cycleLen	= 1;
	
	me->lya = CalcLya1Rate(iGrowth, me->iter);
	}

static Boolean SetRates(tLya* me, short iArgC, Atom* iArgV)
	{
	long	allocation = iArgC * sizeof(float);
			// We could check for (iArgC < 8096), but that seems unnecessarily
			// paranoid. If someone actually manages to construct a list of that size,
			// our call to getbytes() will fail, and that we need to handle anyway.
	
	// Only allocate new memory if we don't already have enough
	if (allocation > me->allocation) {
		// Make sure we can allocate before dropping any memory we have.
		float*	rates = (float*) getbytes(allocation);
				if (rates == NIL) goto punt;
		
		if (me->rates != NIL)
			freebytes(me->rates, me->allocation);
		
		me->rates		= rates;
		me->allocation	= allocation;
		}
	
	// Copy in new data
	me->cycleLen	= iArgC;
	{
	float* curRate = me->rates;
	
	while (iArgC-- > 0) {
		if ( ParseAtom(iArgV, false, true, 0, NIL, NIL) ) {
			float growth = iArgV->a_w.w_float;
			
			if (growth < kMinGrowth)		growth = kMinGrowth;
			else if (growth > kMaxGrowth)	growth = kMaxGrowth;
			
			*curRate++ = growth;
			}
		else *curRate++ = kDefGrowth;
		
		iArgV += 1;
		}
	}
	
	me->lya = CalcLyaRates(me->cycleLen, me->rates, me->iter);
	
	return true;
	
punt:
	// Cheesy exception handling
	error("%s: couldn't allocate memory for %ld growth rates", kClassName, (long) iArgC);
	return false;
	}

#pragma mark -
#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	DoBang(me)
 *	Do1Rate(me, iGrowth)
 *	DoRates(me, iName, iArgC, iArgV)
 *	DoSet(me, iName, iArgC, iArgV)
 *
 ******************************************************************************************/

static void DoBang(tLya* me)
	{ outlet_float(me->coreObject.o_outlet, me->lya); }

static void Do1Rate(tLya* me, double iGrowth)
	{ Set1Rate(me, iGrowth); DoBang(me); }

static void DoRates(
				tLya*	me,
				Symbol*	sym,			// Will be either list or nil symbol, but we don't care.
				short	iArgC,
				Atom*	iArgV)
	
	{
	#pragma unused(sym)
	
	if ( SetRates(me, iArgC, iArgV) ) DoBang(me);
	}

static void DoSet(
			tLya*	me,
			Symbol*	sym,			// Will be set symbol, but we don't care.
			short	iArgC,
			Atom*	iArgV)
	
	{
	#pragma unused(sym)
	
	if (iArgC > 1)
		SetRates(me, iArgC, iArgV);
	else {
		double growth = (iArgC > 0 && ParseAtom(iArgV, false, true, 0, NIL, NIL))
							? iArgV->a_w.w_float
							: kDefGrowth;
		Set1Rate(me, growth);
		}
	
	}
	
	
/******************************************************************************************
 *
 *	DoTattle(me)
 *
 *	Post state information
 *
 ******************************************************************************************/

static void DoPrecision(tLya* me, long iPrecision)
	{ me->iter = (iPrecision > 0) ? iPrecision : 1; }

/******************************************************************************************
 *
 *	DoTattle(me)
 *
 *	Post state information
 *
 ******************************************************************************************/

void
DoTattle(
	tLya* me)
	
	{
	int i;
	
	post("%s state", kClassName);
	post("  Last exponent calculated: %lf, using precision %ld", me->lya, me->iter);
	post("  Length of growth cycle is: %d, with following rate[s]:", me->cycleLen);
	if (me->cycleLen == 1)
		post("    %lf", me->growth);
	else for (i = 0; i < me->cycleLen; i += 1)
		post("    %d: %lf", i, me->rates[i]);
	
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

void DoAssist(tLya* me, void* box, long iDir, long iArgNum, char* oCStr)
	{
	#pragma unused(me,  box)
	
	LitterAssist(iDir, iArgNum, strIndexInLeft, strIndexOutLeft, oCStr);
	}

void DoInfo(tLya* me)
	{ LitterInfo(kClassName, &me->coreObject, (method) DoTattle); }

