/*
	File:		vera.c

	Contains:	Max external object generating sequences of populations from the classic
				(chaotic) population growth model: p' <- r·p·(1-p)

	Written by:	Peter Castine

	Copyright:	© 2002 Peter Castine

	Change History (most recent first):

         <6>      5–3–05    pc      Update for compatibility with CW 8 <cfloat> implementation. Also
                                    made updates to Target Names.
         <5>     11–1–04    pc      Update for Windows (keep file up to conventions while I have
                                    the changes in my head).
         <4>   24–8–2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <3>  30–12–2002    pc      Drop faux 'Vers' resource and related modifications. Add object
                                    version to DoInfo().
         <2>  28–11–2002    pc      Tidy up after initial check in.
         <1>  28–11–2002    pc      Initial check in.

		 2-Apr-2002:	Modified inlet order to match poppy~
		25-Mar-2002:	First implementation.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Identify Target

#if __ide_target("X-Verhulst/Ctrl (Classic)")				\
		|| __ide_target("X-Verhulst/Ctrl (Carbon)")			\
		|| __ide_target("X-Verhuslt/Ctrl (Windows)")
		
	#define __VERA_VAN__ 1

#elif __ide_target("X-Verh. May/Ctrl (Classic)")			\
		|| __ide_target("X-Verh. May/Ctrl (Carbon)")		\
		|| __ide_target("X-Verh. May/Ctrl (Windows)")		\

	#define __VERA_MAY__ 1

#elif __ide_target("X-Verh. +/Ctrl (Classic)")				\
		|| __ide_target("X-Verh. +/Ctrl (Carbon)")			\
		|| __ide_target("X-Verh. +/Ctrl (Windows)")
		
	#define __VERA_PLUS__ 1

#elif __ide_target("X-Verh. =/Ctrl (Classic)")				\
		|| __ide_target("X-Verh. =/Ctrl (Carbon)")			\
		|| __ide_target("X-Verh. =/Ctrl (Windows)")
		
	#define __VERA_IS__ 1

#elif __ide_target("X-Verh. L/Ctrl (Classic)")				\
		|| __ide_target("X-Verh. L/Ctrl (Carbon)")			\
		|| __ide_target("X-Verh. L/Ctrl (Windows)")
		
	#define __VERA_LAM__ 1

#elif __ide_target("X-Verh. S/Ctrl (Classic)")				\
		|| __ide_target("X-Verh. S/Ctrl (Carbon)")			\
		|| __ide_target("X-Verh. S/Ctrl (Windows)")
		
	#define __VERA_STEW__ 1

#else

	#error /* Undefined target */

#endif



#pragma mark • Include Files

#include "LitterLib.h"

#include <cfloat>			// For FLT_EPSILON
#include <math.h>			// For fabs(), log()

#pragma mark • Constants

	// These are common to all varieties

const int		kMinCycle	= 1,
				kMaxCycle	= 8095;

	// Indices for STR# resource
enum {
	strIndexInBang		= lpStrIndexLastStandard + 1,
#if __VERA_MAY__
	strIndexInBeta,
#endif
	strIndexInSeed,
	
	strIndexOutVera,
	
	strIndexInLeft		= strIndexInBang,
	strIndexOutLeft		= strIndexOutVera
	};

#if !__VERA_MAY__
		// Indices for functions we support
	enum Function {
		fnIdent			= 0,
		fnZero,
		fnOne,
		fnRecip,
		fnSinPi,
		fnCosPi,
		fnTanPi,
		fnCotPi,
		fnSinh,
		fnCosh,
		fnTanh,
		fnLog,
		fnExp,
		fnSqr,
		fnRoot,
		
		fnFirst		= fnIdent,
		fnLast		= fnRoot
		};

		// Function names
	const char*		kFuncNames[fnLast + 1]
							= {	"ident",
								"zero",
								"one",
								"recip",
								"sinpi",
								"cospi",
								"tanpi",
								"cotpi",
								"sinh",
								"cosh",
								"tanh",
								"log",
								"exp",
								"sqr",
								"sqrt"
								};
#endif

	// The following four macros used to be "properly" defined as const doubles, but with
	// changes to <cfloat> I can't figure out how to assign the new value of FLT_EPSILON
	// to a const double at compile time. So I've reverted to using preprocessor macros.
#define kMinSeed	FLT_EPSILON							// Yes, *FLT*_EPSILON
#define kMaxSeed	(1.0 - FLT_EPSILON)
#define kMinGrowth	(-FLT_MAX)
#define kMaxGrowth	FLT_MAX

const double	kDefGrowth	= 0.0,
				kDefSeed	= 2.0 / 3.0;				// Good seed for most growth rates

#if __VERA_VAN__
	const char	kClassName[]	= "lp.vera";			// Class name
	
	const enum Function kDefVFunc = fnSinPi;

#elif __VERA_PLUS__
	const char	kClassName[]	= "lp.vera+";			// Class name
	
	const enum Function kDefVFunc = fnSinPi;

#elif __VERA_IS__
	const char	kClassName[]	= "lp.vera=";			// Class name
	
	const enum Function kDefVFunc = fnSinPi;

#elif __VERA_LAM__
	const char	kClassName[]	= "lp.veral";			// Class name
	
	const enum Function kDefVFunc = fnSinPi;

#elif __VERA_STEW__
	const char	kClassName[]	= "lp.veras";			// Class name
	
	const enum Function kDefVFunc = fnSinPi;

#elif __VERA_MAY__
	const char	kClassName[]	= "lp.veram";			// Class name
	
	const double	kDefBeta	= 1.0;
#endif




#pragma mark • Type Definitions

#if !__VERA_MAY__
	typedef enum Function tFunction;
#endif

#pragma mark • Object Structure

typedef struct {
	Object			coreObject;
	
	double			seed,
					curPop,

#if __VERA_MAY__
					beta,
#endif

					growth;				// Used for single-value growth rate
					
	long			cycleLen,			// Cycle length
					allocation,			// Bytes allocated (may be more than needed)
					curElem;			// Index to current element in cycle
	
	float*			rates;				// Variable length array of growth rate values;
										// Used when cycleLen > 1
#if !__VERA_MAY__
	tFunction		func;
#endif
	} tVera;


#pragma mark • Global Variables



#pragma mark • Function Prototypes

	// Class message functions
void*		NewVera(Symbol*, short, Atom*);
void		FreeVera(tVera*);

	// Object message functions
static void DoBang(tVera*);
static void Do1Rate(tVera*, double);					// Rate can be a single float...
static void DoRates(tVera*, Symbol*, short, Atom*);	// ... or a list
static void DoSet(tVera*, Symbol*, short, Atom*);
static void DoSeed(tVera*, double);
static void DoReset(tVera*);

#if __VERA_MAY__
	static void DoBeta(tVera*, double);
#else
	static void	DoIdent(tVera*);
	static void	DoZero(tVera*);
	static void	DoOne(tVera*);
	static void	DoRecip(tVera*);
	static void	DoSinPi(tVera*);
	static void	DoCosPi(tVera*);
	static void	DoTanPi(tVera*);
	static void	DoCotPi(tVera*);
	static void	DoSinh(tVera*);
	static void	DoCosh(tVera*);
	static void	DoTanh(tVera*);
	static void	DoLog(tVera*);
	static void	DoExp(tVera*);
	static void	DoSqr(tVera*);
	static void DoRoot(tVera*);
#endif

static void DoTattle(tVera*);
static void	DoAssist(tVera*, void* , long , long , char*);
static void	DoInfo(tVera*);

	// Helper methods
static void Set1Rate(tVera*, double);
static Boolean SetRates(tVera*, short, Atom*);

#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark • Inline Functions

#if __VERA_VAN__
	static inline double NextPop(double iPop, double iFnPop, double iGrowth)
				{ return iPop + iGrowth * iFnPop * (1.0 - iFnPop); }

#elif __VERA_PLUS__
	static inline double NextPop(double iPop, double iFnPop, double iGrowth)
				{ return iPop + iGrowth * iFnPop; }

#elif __VERA_IS__
	static inline double NextPop(double , double iFnPop, double iGrowth)
				{ return iGrowth * iFnPop; }

#elif __VERA_LAM__
	static inline double NextPop(double, double iFnPop, double iGrowth)
				{ return iGrowth * iFnPop * (1.0 - iFnPop); }

#elif __VERA_STEW__
	static inline double NextPop(double, double iFnPop, double iGrowth)
				{ return iGrowth * iFnPop * iFnPop - 1.; }

#elif __VERA_MAY__
	static inline double NextPop(double iPop, double iFnPop, double iGrowth)
				{ return iGrowth * iPop / iFnPop; }

#endif


#pragma mark -

/******************************************************************************************
 *
 *	main()
 *	
 *	Standar Max External Object Entry Point Function
 *	
 ******************************************************************************************/

void
main(void*)				// Parameter is obsolete (68k legacy)
	
	{
	
	// Standard Max setup() call
	setup(	&gObjectClass,				// Pointer to our class definition
			(method) NewVera,			// Instance creation function
			(method) FreeVera,			// Custom deallocation function
			sizeof(tVera),				// Class object size
			NIL,						// No menu function
			A_GIMME,					// Arguments:	Seed population (mandatory),
			0);							//		followed by list of Growth Rates
		
	// Messages
	addbang	((method) DoBang);
	addfloat((method) Do1Rate);
	addmess	((method) DoRates,	"list",		A_GIMME, 0);
	addmess	((method) DoSet,	"set",		A_GIMME, 0);
	addmess	((method) DoReset,	"reset",	A_NOTHING);
	
#if __VERA_MAY__
	addftx	((method) DoBeta,	1);
	addftx	((method) DoSeed,	2);
#else
	addftx	((method) DoSeed,	1);
	addmess	((method) DoIdent,	"ident",	A_NOTHING);
	addmess	((method) DoZero,	"zero",		A_NOTHING);
	addmess	((method) DoOne,	"one",		A_NOTHING);
	addmess	((method) DoRecip,	"recip",	A_NOTHING);
	addmess	((method) DoSinPi,	"sinpi",	A_NOTHING);
	addmess	((method) DoCosPi,	"cospi",	A_NOTHING);
	addmess	((method) DoTanPi,	"tanpi",	A_NOTHING);
	addmess	((method) DoCotPi,	"cotpi",	A_NOTHING);
	addmess	((method) DoSinh,	"sinh",		A_NOTHING);
	addmess	((method) DoCosh,	"cosh",		A_NOTHING);
	addmess	((method) DoTanh,	"tanh",		A_NOTHING);
	addmess	((method) DoLog,	"log",		A_NOTHING);
	addmess	((method) DoExp,	"exp",		A_NOTHING);
	addmess	((method) DoSqr,	"sqr",		A_NOTHING);
	addmess	((method) DoRoot,	"sqrt",		A_NOTHING);
#endif
	
	addmess	((method) DoTattle,	"dblclick",	A_CANT, 0);
	addmess	((method) DoTattle,	"tattle",	A_NOTHING);
	addmess	((method) DoAssist,	"assist",	A_CANT, 0);
	addmess	((method) DoInfo,	"info",		A_CANT, 0);
	
	// Initialize Litter Library
	LitterInit(kClassName, 0);

	}

#pragma mark -
#pragma mark • Utility Functions

/******************************************************************************************
 *
 *	NewVera(iName, iArgC, iArgV)
 *	
 *	Arguments:
 *		iName is always "lp.poppy" and we do not use this paramter
 *		iArgC and iArgV specify a list of atoms, which will normally all be floats.
 *		The first item is a growth rate.
 *		If there is more than one item in the list, the last item is an initial seed.
 *		Any additional items before 
 *	
 ******************************************************************************************/

#if !__VERA_MAY__
	static short
	MapStrToFunction(
		const char* iStr)
		{
		short	result;
		
		for (result = fnLast; result >= 0; result -= 1) {
			if (strcmp(iStr, kFuncNames[result]) == 0) break;
			} 
		
		return result;
		}
#endif

#pragma mark -
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	NewVera(iName, iArgC, iArgV)
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
NewVera(
	Symbol*	,
	short	iArgC,
	Atom*	iArgV)
	
	{
	tVera*		me			= NIL;
	double		seed		= kDefSeed,
				growth		= kDefGrowth;
	long		cycleLen	= kMinCycle,
				allocation	= 0;
	float*		rates		= NIL;

#if __VERA_MAY__
	double		beta		= kDefBeta;
#else
	tFunction	func		= kDefVFunc;
#endif
	
	//
	// Let Max allocate us, our inlets, and outlets
	//
	me = (tVera*) newobject(gObjectClass);
		if (me == NIL) goto punt;
	
		// Extra inlet
#if __VERA_MAY__
	floatin(me, 2);		// veram has two inlets
#endif
	floatin(me, 1);		// Everyone else has one
	
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
	
#if __VERA_MAY__

	if (iArgC > 0) {
		// First argument is initial beta value
		Atom* betaAtom = iArgV++;
		
		if ( ParseAtom(betaAtom, false, true, 0, NIL, NIL) )
			beta = betaAtom->a_w.w_float;
			
		iArgC -= 1;
		}
		
	me->beta = beta;
	
#else

		// The argument list may, optionally, start with a symbol indicating the function
	if (iArgC > 0 && iArgV->a_type == A_SYM) {
		char*	funcName = (iArgV++)->a_w.w_sym->s_name;
		int		funcNum = MapStrToFunction(funcName);
		
		if (fnFirst <= funcNum && funcNum <= fnLast)
			func = (tFunction) funcNum;
		else error("%s: doesn't understand %s", kClassName, funcName);
		
		iArgC -= 1;
		}
		
	me->func = func;
	
#endif		
		
		// Check if there is more than one argument. In this case, the last argument is
		// a seed, which we chop from the list
	if (iArgC > 1) {
		Atom* seedAtom = iArgV + --iArgC;
		if ( ParseAtom(seedAtom, false, true, 0, NIL, NIL) ) {
			seed = seedAtom->a_w.w_float;
			}
		}
	
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

	DoSeed(me, seed);
	
	
	//
	// All done
	//
	
	return me;

punt:
	// Oops... cheesy exception handling
	error("Insufficient memory to create %s object.", kClassName);
	if (me) FreeVera(me);
	
	return NIL;
	}

/******************************************************************************************
 *
 *	FreeVera(me)
 *	FreeProxies(iProxyList);
 *	
 ******************************************************************************************/

void
FreeVera(
	tVera* me)
	
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

static void DoOut(tVera* me)
	{ outlet_float(me->coreObject.o_outlet, me->curPop); }

static void
DoNext(
	tVera* me)
	
	{
	double	p = me->curPop,
			fnp;
	
#if __VERA_MAY__
	fnp = pow(p + 1.0, me->beta);
#else
	switch (me->func) {
		case fnZero:	fnp = 0.0;				break;
		case fnOne:		fnp = 1.0;				break;
		case fnRecip:	fnp = 1.0 / p;			break;
		case fnSinPi:	fnp = sin(kPi * p);		break;
		case fnCosPi:	fnp = cos(kPi * p);		break;
		case fnTanPi:	fnp = tan(kPi * p);		break;
		case fnCotPi:	fnp = 1 / tan(kPi * p);	break;
		case fnSinh:	fnp = sinh(p);			break;
		case fnCosh:	fnp = cosh(p);			break;
		case fnTanh:	fnp = tanh(p);			break;
		case fnLog:		fnp = log(p);			break;
		case fnExp:		fnp = exp(p);			break;
		case fnSqr:		fnp = p * p;			break;
		case fnRoot:	fnp = sqrt(p);			break;
			// Only thing left is fnIdent
		default:		fnp = p;				break;
		}
#endif
	
	if (me->cycleLen == 1)
		p = NextPop(p, fnp, me->growth);
	else {
		p = NextPop(p, fnp, me->rates[me->curElem++]);
		
		if (me->curElem >= me->cycleLen)
			me->curElem = 0;
		}
	
	me->curPop = p;
	}
	
static void
DoBang(
	tVera* me)
	
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

	static void Set1Rate(tVera* me, double iGrowth)
		{
		if (iGrowth < kMinGrowth)		iGrowth = kMinGrowth;
		else if (iGrowth > kMaxGrowth)	iGrowth = kMaxGrowth;
		
		me->growth		= iGrowth;
		me->cycleLen	= 1;
		me->curElem		= 0;
		}
	
	static Boolean SetRates(tVera* me, short iArgC, Atom* iArgV)
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
	tVera*	me,
	double	iGrowth)
	
	{
	Set1Rate(me, iGrowth);
	DoBang(me);
	}

static void
DoRates(
	tVera*	me,
	Symbol*	,			// Will be either list or nil symbol, but we don't care.
	short	iArgC,
	Atom*	iArgV)
	
	{
	if ( SetRates(me, iArgC, iArgV) ) DoBang(me);
	}

static void
DoSet(
	tVera*	me,
	Symbol*	,			// Will be set symbol, but we don't care.
	short	iArgC,
	Atom*	iArgV)
	
	{
	
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
	tVera*	me,
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

static void DoReset(tVera* me)
	{ me->curPop = me->seed; }


/******************************************************************************************
 *
 *	Ident(me)
 *	Zero(me)
 *	One(me)
 *	Recip(me)
 *	SinPi(me)
 *	CosPi(me)
 *	TanPi(me)
 *	CotPi(me)
 *	Sinh(me)
 *	Cosh(me)
 *	Tanh(me)
 *	Log(me)
 *	Exp(me)
 *	Sqr(me)
 *	Root(me)
 *	
 ******************************************************************************************/

#if __VERA_MAY__
	static void DoBeta(tVera* me, double iBeta)	{ me->beta = iBeta; }
#else
	static void	DoIdent(tVera* me)				{ me->func = fnIdent; }
	static void	DoZero(tVera* me)				{ me->func = fnZero; }
	static void	DoOne(tVera* me)				{ me->func = fnOne; }
	static void	DoRecip(tVera* me)				{ me->func = fnRecip; }
	static void	DoSinPi(tVera* me)				{ me->func = fnSinPi; }
	static void	DoCosPi(tVera* me)				{ me->func = fnCosPi; }
	static void	DoTanPi(tVera* me)				{ me->func = fnTanPi; }
	static void	DoCotPi(tVera* me)				{ me->func = fnCotPi; }
	static void	DoSinh(tVera* me)				{ me->func = fnSinh; }
	static void	DoCosh(tVera* me)				{ me->func = fnCosh; }
	static void	DoTanh(tVera* me)				{ me->func = fnTanh; }
	static void	DoLog(tVera* me)				{ me->func = fnLog; }
	static void	DoExp(tVera* me)				{ me->func = fnExp; }
	static void	DoSqr(tVera* me)				{ me->func = fnSqr; }
	static void DoRoot(tVera* me)				{ me->func = fnRoot; }
#endif

/******************************************************************************************
 *
 *	DoTattle(me)
 *
 *	Post state information
 *
 ******************************************************************************************/

void
DoTattle(
	tVera* me)
	
	{
	int i;
	
	post("%s state", kClassName);
	post("  Current population is %lf (from initial pop. of %lf)",
			me->curPop, me->seed);

#if __VERA_MAY__
	post("  Current beta: %lf", me->beta);
#else
	post("  Current function: %ld, (%s)",
			(long) me->func, kFuncNames[me->func]);
#endif
	
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

void DoAssist(tVera*, void*, long iDir, long iArgNum, char* oCStr)
	{ LitterAssist(iDir, iArgNum, strIndexInLeft, strIndexOutLeft, oCStr); }

void DoInfo(tVera* me)
	{ LitterInfo(kClassName, &me->coreObject, (method) DoTattle); }


