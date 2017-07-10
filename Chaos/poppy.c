/*
	File:		poppy.c

	Contains:	Max external object generating sequences of populations from the classic
				(chaotic) population growth model: p' <- r·p·(1-p)

	Written by:	Peter Castine

	Copyright:	© 2002 Peter Castine

	Change History (most recent first):

         <6>    2–3–2005    pc      Update for CW 8 <cfloat> implementation.
         <5>     11–1–04    pc      Update for Windows.
         <4>    7–7–2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <3>  30–12–2002    pc      Drop faux 'Vers' resource and related modifications. Add object
                                    version to DoInfo().
         <2>  28–11–2002    pc      Tidy up after check in.
         <1>  28–11–2002    pc      Initial check in.
*/


/******************************************************************************************

Previous history:

		 2-Apr-2002:	Modified inlet order to match poppy~
		25-Mar-2002:	First implementation.
 
 ******************************************************************************************/


#pragma mark • Include Files

#include "LitterLib.h"
#include "TrialPeriodUtils.h"

#ifdef __MWERKS__
	// Used to need to include this for CodeWarrior
	// GCC doesn't know about it, and whatever's needed is #included anyway
	#include <cfloat>			// For FLT_EPSILON
#endif

#include <math.h>			// For fabs(), log()

#pragma mark • Constants

	// These are common to all varieties
const char	kClassName[]		= "lp.poppy";			// Class name

	// The following three macros used to be "properly" defined as const doubles, but with
	// changes to <cfloat> I can't figure out how to assign the new value of FLT_EPSILON
	// to a const double at compile time. So I've reverted to using preprocessor macros.
#define kMinSeed	FLT_EPSILON						// Yes, *FLT*_EPSILON
#define kMaxSeed	(1.0 - FLT_EPSILON)
#define kMinGrowth	FLT_EPSILON

const int		kMinCycle	= 1,
				kMaxCycle	= 8095;
const float		kDefSeed	= 2.0 / 3.0,			// Good seed for most growth rates
				kMaxGrowth	= 4.0,
				kDefGrowth	= 3.56994571869;		// Smallest  growth rate producing chaos



	// Indices for STR# resource
enum {
	strIndexInBang		= lpStrIndexLastStandard + 1,
	strIndexInSeed,
	
	strIndexOutPop,
	
	strIndexInLeft		= strIndexInBang,
	strIndexOutLeft		= strIndexOutPop
	};


#pragma mark • Type Definitions



#pragma mark • Object Structure

typedef struct {
	Object			coreObject;
	
	double			seed,
					curPop,
					growth;				// Used for single-value growth rate
					
	long			cycleLen,			// Cycle length
					allocation,			// Bytes allocated (may be more than needed)
					curElem;			// Index to current element in cycle
	
	float*			rates;				// Variable length array of growth rate values;
										// Used when cycleLen > 1
	} tPoppy;


#pragma mark • Global Variables

//t_messlist*	gPoppyClass		= NIL;


#pragma mark • Function Prototypes

	// Class message functions
void*		NewPoppy(Symbol*, short, Atom*);
void		FreePoppy(tPoppy*);

	// Object message functions
static void DoBang(tPoppy*);
static void Do1Rate(tPoppy*, double);					// Rate can be a single float...
static void DoRates(tPoppy*, Symbol*, short, Atom*);	// ... or a list
static void DoSet(tPoppy*, Symbol*, short, Atom*);
static void DoSeed(tPoppy*, double);
static void DoReset(tPoppy*);

static void DoTattle(tPoppy*);
static void	DoAssist(tPoppy*, void* , long , long , char*);
static void	DoInfo(tPoppy*);

	// Helper methods
static void Set1Rate(tPoppy*, double);
static Boolean SetRates(tPoppy*, short, Atom*);

#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark • Inline Functions

static inline double NextPop(double iPop, double iGrowth)
			{ return iGrowth * iPop * (1.0 - iPop); }

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
			(method) NewPoppy,			// Instance creation function
			(method) FreePoppy,			// Custom deallocation function
			sizeof(tPoppy),				// Class object size
			NIL,						// No menu function
			A_GIMME,					// Arguments:	Seed population (mandatory),
			0);							//		followed by list of Growth Rates
		
	// Messages
	LITTER_TIMEBOMB	addbang	((method) DoBang);
	LITTER_TIMEBOMB	addfloat((method) Do1Rate);
	LITTER_TIMEBOMB	addmess	((method) DoRates,	"list",		A_GIMME, 0);
	addmess	((method) DoSet,	"set",		A_GIMME, 0);
	addftx	((method) DoSeed,	1);
	addmess	((method) DoReset,	"reset",	A_NOTHING);
	
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
 *	NewPoppy(iName, iArgC, iArgV)
 *	
 *	Arguments:
 *		iName is always "lp.poppy" and we do not use this paramter
 *		iArgC and iArgV specify a list of atoms, which will normally all be floats.
 *		The first item is a growth rate.
 *		If there is more than one item in the list, the last item is an initial seed.
 *		Any additional items before 
 *	
 ******************************************************************************************/

void*
NewPoppy(
	Symbol*	sym,
	short	iArgC,
	Atom*	iArgV)
	
	{
	#pragma unused(sym)
	
	tPoppy*	me			= NIL;
	double	seed		= kDefSeed,
			growth		= kDefGrowth;
	long	cycleLen	= kMinCycle,
			allocation	= 0;
	float*	rates		= NIL;
	
	//
	// Let Max allocate us, our inlets, and outlets
	//
	me = (tPoppy*) newobject(gObjectClass);
		if (me == NIL) goto punt;
	
		// One extra inlet
	floatin(me, 1);
	
		// One outlet, accessed through me->coreObject.o_outlet
	floatout(me);
	
	//
	// Make sure pointers and counters are set to valid initial states
	//
	
	me->cycleLen	= 0;
	me->allocation	= 0;
	me->curElem		= 0;		// This is not really necessary
	me->rates		= NIL;		// This is a good idea.
	
	//
	// Parse and store arguments
	//
		// Check if there is more than one argument. In this case, the last argument is
		// a seed, which we chop from the list
	if (iArgC > 1) {
		Atom* seedAtom = iArgV + --iArgC;
		if ( ParseAtom(seedAtom, false, true, 0, NIL, NIL) ) {
			seed = seedAtom->a_w.w_float;
			}
		}
	DoSeed(me, seed);
	
	switch (iArgC) {
		case 1:
			if ( ParseAtom(iArgV, false, true, 0, NIL, NIL) )
				growth = iArgV->a_w.w_float;
			// fall into next case
		case 0:
			Set1Rate(me, growth);
			break;
		
		default:
			SetRates(me, iArgC, iArgV);
			break;	
		}
	
	
	//
	// All done
	//
	
	return me;

punt:
	// Oops... cheesy exception handling
	error("Insufficient memory to create %s object.", kClassName);
	if (me) FreePoppy(me);
	
	return NIL;
	}

/******************************************************************************************
 *
 *	FreePoppy(me)
 *	FreeProxies(iProxyList);
 *	
 ******************************************************************************************/

void
FreePoppy(
	tPoppy* me)
	
	{
	
	if (me == NIL)		// Sanity check
		return;		
	
	if (me->rates)
		freebytes(me->rates, me->allocation);
		
	}
	

#pragma mark -
#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	DoBang(me)
 *	DoOut(me)
 *	DoNext(me)
 *
 ******************************************************************************************/

static void DoOut(tPoppy* me)
	{ outlet_float(me->coreObject.o_outlet, me->curPop); }

static void
DoNext(
	tPoppy* me)
	
	{
	double	p = me->curPop;
	
	if (me->cycleLen == 1)
		p = NextPop(p, me->growth);
	else {
		p = NextPop(p, me->rates[me->curElem++]);
		
		if (me->curElem >= me->cycleLen)
			me->curElem = 0;
		}
	
	me->curPop = p;
	}
	
static void
DoBang(
	tPoppy* me)
	
	{
	DoNext(me);
	DoOut(me);
	}


/******************************************************************************************
 *
 *	Do1Rate(me, iGrowth)
 *	DoRates(me, iName, iArgC, iArgV)
 *	DoSet(me, iName, iArgC, iArgV)
 *
 ******************************************************************************************/

	static void Set1Rate(tPoppy* me, double iGrowth)
		{
		if (iGrowth < kMinGrowth)		iGrowth = kMinGrowth;
		else if (iGrowth > kMaxGrowth)	iGrowth = kMaxGrowth;
		
		me->growth		= iGrowth;
		me->cycleLen	= 1;
		me->curElem		= 0;
		}
	
	static Boolean SetRates(tPoppy* me, short iArgC, Atom* iArgV)
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
		
		// Reset counter; copy in new data
		me->curElem		= 0;
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
		
		return true;
		
	punt:
		// Cheesy exception handling
		error("%s: couldn't allocate memory for %ld growth rates", kClassName, (long) iArgC);
		return false;
		}

static void
Do1Rate(
	tPoppy*	me,
	double	iGrowth)
	
	{
	Set1Rate(me, iGrowth);
	DoBang(me);
	}

static void
DoRates(
	tPoppy*	me,
	Symbol*	sym,			// Will be either list or nil symbol, but we don't care.
	short	iArgC,
	Atom*	iArgV)
	
	{
	#pragma unused(sym)
	
	if ( SetRates(me, iArgC, iArgV) ) DoBang(me);
	
	}

static void
DoSet(
	tPoppy*	me,
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
 *	DoSeed(me, iSeed)
 *
 ******************************************************************************************/

static void
DoSeed(
	tPoppy*	me,
	double	iSeed)
	
	{
	
	if (iSeed < kMinSeed)		iSeed = kMinSeed;
	else if (iSeed > kMaxSeed)	iSeed = kMaxSeed;
	
	me->curPop = me->seed = iSeed;
	
	}	


/******************************************************************************************
 *
 *	DoReset(me)
 *	
 ******************************************************************************************/

static void DoReset(tPoppy* me)
	{ me->curPop = me->seed; }


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
	int i;
	
	post("%s state", kClassName);
	post("  Current population is %lf (from initial pop. of %lf)",
			me->curPop, me->seed);
	post("  Length of growth cycle is: %d, currently at element %d, with following rates:",
			me->cycleLen, me->curElem);
	if (me->cycleLen == 1)
		post("    %lf", me->growth);
	else for (i = 0; i < me->cycleLen; i += 1)
		post("    %d: %lf %c", i, me->rates[i], (i == me->curElem) ? '*' : ' ');
	
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
	{ LitterInfo(kClassName, &me->coreObject, (method) DoTattle); }


