/*
	File:		vera~.c

	Contains:	Max external object generating signals using Fractint extensions to the
				classic (chaotic) population growth model: p' <- r·p·(1-p)

	Written by:	Peter Castine

	Copyright:	© 2002 Peter Castine

	Change History (most recent first):

         <6>      5–3–05    pc      Update for compatibility with CW 8 <cfloat> implementation. Also
                                    made updates to Target Names.
         <5>     11–1–04    pc      Update for Windows (keep file up to conventions while I have
                                    the changes in my head).
         <4>   24–8–2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <3>    6–3–2003    pc      Drop faux 'Vers' resource and related modifications. Add object
                                    version to DoInfo().
         <2>  28–11–2002    pc      Tidy up after check in.
         <1>  28–11–2002    pc      Initial check in.

		14-Apr-2002:	First implementation.
*/

/******************************************************************************************
 ******************************************************************************************/


#pragma mark • Identify Target

#if __ide_target("X-Verhulst/Sig (Classic)")				\
		|| __ide_target("X-Verhulst/Sig (Carbon)")			\
		|| __ide_target("X-Verhuslt/Sig (Windows)")
		
	#define __VERA_VAN__ 1

#elif __ide_target("X-Verh. May/Sig (Classic)")				\
		|| __ide_target("X-Verh. May/Sig (Carbon)")			\
		|| __ide_target("X-Verh. May/Sig (Windows)")		\

	#define __VERA_MAY__ 1

#elif __ide_target("X-Verh. +/Sig (Classic)")				\
		|| __ide_target("X-Verh. +/Sig (Carbon)")			\
		|| __ide_target("X-Verh. +/Sig (Windows)")
		
	#define __VERA_PLUS__ 1

#elif __ide_target("X-Verh. =/Sig (Classic)")				\
		|| __ide_target("X-Verh. =/Sig (Carbon)")			\
		|| __ide_target("X-Verh. =/Sig (Windows)")
		
	#define __VERA_IS__ 1

#elif __ide_target("X-Verh. L/Sig (Classic)")				\
		|| __ide_target("X-Verh. L/Sig (Carbon)")			\
		|| __ide_target("X-Verh. L/Sig (Windows)")
		
	#define __VERA_LAM__ 1

#elif __ide_target("X-Verh. S/Sig (Classic)")				\
		|| __ide_target("X-Verh. S/Sig (Carbon)")			\
		|| __ide_target("X-Verh. S/Sig (Windows)")
		
	#define __VERA_STEW__ 1

#else

	#error /* Undefined target */

#endif



#pragma mark • Include Files

#include "LitterLib.h"
#include "z_dsp.h"

#include <cfloat>										// For FLT_EPSILON

#pragma mark • Constants

	// These are common to all varieties

	// Indices for STR# resource
enum {
	strIndexInGrowth		= lpStrIndexLastStandard + 1,
#if __VERA_MAY__
	strIndexInBeta,
#endif
	strIndexInSeed,
	strIndexFreq,
	strIndexInInterp,
	
	strIndexOutVera,
	
	strIndexInLeft		= strIndexInGrowth,
	strIndexOutLeft		= strIndexOutVera
	};

	// Indices for MSP Inlets and Outlets.
enum {
	inletGrowth			= 0,
	
	outletVera
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
	const char	kClassName[]	= "lp.vera~";			// Class name
	
	const enum Function kDefVFunc = fnSinPi;

#elif __VERA_PLUS__
	const char	kClassName[]	= "lp.vera+~";			// Class name
	
	const enum Function kDefVFunc = fnSinPi;

#elif __VERA_IS__
	const char	kClassName[]	= "lp.vera=~";			// Class name
	
	const enum Function kDefVFunc = fnSinPi;

#elif __VERA_LAM__
	const char	kClassName[]	= "lp.veral~";			// Class name
	
	const enum Function kDefVFunc = fnSinPi;

#elif __VERA_STEW__
	const char	kClassName[]	= "lp.veras~";			// Class name
	
	const enum Function kDefVFunc = fnSinPi;

#elif __VERA_MAY__
	const char	kClassName[]	= "lp.veram~";			// Class name
	
	const double	kDefBeta	= 1.0;

#endif




#pragma mark • Type Definitions

typedef enum Interpolation tInterp;

#if !__VERA_MAY__
	typedef enum Function tFunction;
#endif


#pragma mark • Object Structure

typedef struct {
	t_pxobject			coreObject;
	
	double			seed,
					curPop,

#if __VERA_MAY__
					beta,
#endif

					growth,			// Nominal growth rate per cycle
					goal,			// Next population
					slope,			// Used for interpolation
					curve,			// Used for quadratic interpolation
					userFreq,		// As specified by user, in Hz
					curSR;			// Used for single-value growth rate
					
#if !__VERA_MAY__
	tFunction		func;
#endif
	
	unsigned long	spc,			// Samples per growth cycle; exact.
					sampsToGo;		// Keep track of this between Perform calls
	
	tInterp			interp;			// None, linear, quadratic, or geometric
	
	Boolean			seedPending;	// Flag if seed message arrives mid-cycle.

	} tVera;


#pragma mark • Global Variables


#pragma mark • Function Prototypes

	// Class message functions
void*		NewVera(Symbol*, short, Atom*);

	// Object message functions
static void DoGrowth(tVera*, double);
static void DoSeed(tVera*, double);
static void DoBaseFreq(tVera*, double);
static void DoInterp(tVera*, long);
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

	// MSP Messages
static void	DoDSP(tVera*, t_signal**, short*);
static int*	PerformPoppyDynamic(int*);
static int*	PerformPoppyStatic(int*);


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
 *	Standar Max External Object Entry Point Function
 *	
 ******************************************************************************************/

void
main(void*)				// Parameter is obsolete (68k legacy)
	
	{
	
	// Standard Max setup() call
	setup(	&gObjectClass,				// Pointer to our class definition
			(method) NewVera,			// Instance creation function
			(method) dsp_free,			// Std. MSP deallocation function
			sizeof(tVera),				// Class object size
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
	
#if __VERA_MAY__
	addftx	((method) DoBeta,		1);
	addftx	((method) DoSeed,		2);
	addftx	((method) DoBaseFreq,	3);
	addinx	((method) DoInterp,		4);
#else
	addftx	((method) DoSeed,		1);
	addftx	((method) DoBaseFreq,	2);
	addinx	((method) DoInterp,		3);
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
	
	// MSP messages
	addmess	((method) DoDSP,	"dsp",	A_CANT, 0);

	// Initialize Litter Library
	LitterInit(kClassName, 0);

	}

#pragma mark -
#pragma mark • Utility Functions & Methods

/******************************************************************************************
 *
 *	MapStrToFunction(iStr)
 *	
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


	// Utility to calculate interpolation parameters
static inline void RecalcInterpParams(tVera* me)
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
static inline void UpdateBaseFreq(tVera* me)
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
	const double	kDefBF		= 0.0;				// Base Frequency; zero indicates sample rate
	const tInterp	kDefInterp	= interpGeo;

	tVera*		me			= NIL;
	double		seed		= kDefSeed,
				growth		= kDefGrowth,
				baseFreq	= kDefBF;
	tInterp		interp		= kDefInterp;

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
	dsp_setup(&(me->coreObject), 1);			// Rate inlet is the only one accepting
												// signal vectors
	
		// Extra inlets
#if __VERA_MAY__
	intin(me, 4);		// veram~ has four inlets
	floatin(me, 3);
	floatin(me, 2);
	floatin(me, 1);
#else
	intin(me, 3);		// veram Everyone else has three
	floatin(me, 2);
	floatin(me, 1);
#endif
	
		// One outlet
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

	DoInterp(me, interp);						// Sets me->interp
	DoBaseFreq(me, baseFreq);					// Sets me->userFreq, me->spc
	DoSeed(me, seed);							// Sets me->seed, me->curPop, me->seedPending
	DoGrowth(me, growth);						// Sets me->growth
	
	
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
DoGrowth(tVera* me, double iGrowth)
	{ me->growth = iGrowth; }


static void
DoSeed(
	tVera*	me,
	double	iSeed)
	
	{
	
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
	tVera*	me,
	double	iFreq)
	
	{
	
	if (iFreq <= 0.0)
		iFreq = me->curSR;
	
	me->userFreq = iFreq;
	UpdateBaseFreq(me);
	
	}
	
	
static void
DoInterp(
	tVera* me,
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
 ******************************************************************************************/

static void
DoReset(
	tVera* me)
	
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
	tVera*		me,
	t_signal**	iDSPVectors,
	short*		iConnectCounts)
	
	{
	
	if (iConnectCounts[outletVera] == 0)
		return;
	
	me->curSR = iDSPVectors[outletVera]->s_sr;
	UpdateBaseFreq(me);
	
	if ( iConnectCounts[inletGrowth] > 0 ) {
	
		dsp_add(
			PerformPoppyDynamic, 4, me,
			(long) iDSPVectors[outletVera]->s_n,
			iDSPVectors[inletGrowth]->s_vec,
			iDSPVectors[outletVera]->s_vec
			);
		
		}
	
	else {
		
		dsp_add(
			PerformPoppyStatic, 3, me, 
			(long) iDSPVectors[outletVera]->s_n,
			iDSPVectors[outletVera]->s_vec
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

	static void PoppyStat1Step(tVera* me, long iVecSize, tSampleVector oPoppy)
		{
		double	curPop	= me->curPop,
				growth	= me->growth;
		
		do {
			*oPoppy++ = curPop = NextPop(curPop, growth);
			} while (--iVecSize > 0);
		
		me->curPop = curPop;
		}
	
	static void PoppyStat2Step(tVera* me, long iVecSize, tInterp iInterp, tSampleVector oPoppy)
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
	
	static void PoppyStatNoInterp(tVera* me, long iVecSize, unsigned long iSPC, tSampleVector oPoppy)
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
	
	static void PoppyStatLin(tVera* me, long iVecSize, unsigned long iSPC, tSampleVector oPoppy)
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
	
	static void PoppyStatQuad(tVera* me, long iVecSize, long iSPC, tSampleVector oPoppy)
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
	
	static void PoppyStatGeo(tVera* me, long iVecSize, long iSPC, tSampleVector oPoppy)
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
	tVera*			me = (tVera*) iParams[paramMe];
	
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
		tVera*			me,
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
		tVera*			me,
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
			tVera*			me,
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
			tVera*			me,
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
			tVera*			me,
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
			tVera*			me,
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
	tVera*			me = (tVera*) iParams[paramMe];
	
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
