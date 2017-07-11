/*
	File:		coshy.c

	Contains:	Max external object generating a random values with Cauchy distribution.

	Written by:	Peter Castine

	Copyright:	© 2001-2002 Peter Castine

	Change History (most recent first):

        <10>   30–3–2006    pc      Update for new LitterLib organization, implement more efficient
                                    generator.
         <9>   24–3–2006    pc      Update for new LitterLib organization
         <8>     18–2–06    pc      Add support for expect message.
         <7>     10–1–06    pc      Updated to use LitterAssistResFrag()
         <6>     15–1–04    pc      Avoid possible memory leak if seed argument is used.
         <5>     11–1–04    pc      Update for Windows.
         <4>    7–7–2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <3>  30–12–2002    pc      Drop faux 'Vers' resource and related modifications. Add object
                                    version to CoshyInfo().
         <2>  28–11–2002    pc      Tidy up after initial check-in.
         <1>  28–11–2002    pc      Initial check in.
		4-Apr-2001:		First implementation.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "LitterLib.h"
#include "TrialPeriodUtils.h"
#include "RNGCauchy.h"
#include "MiscUtils.h"


#pragma mark • Constants

const char		kClassName[]		= "lp.coshy";			// Class name



	// Indices for STR# resource
enum {
	strIndexInBang		= lpStrIndexLastStandard + 1,
	strIndexInTau,
	strIndexInLoc,
	
	strIndexTheOutlet,
	
	strIndexFragNeg,
	strIndexFragSym,
	strIndexFragPos,
	
	strIndexLeftInlet	= strIndexInBang,
	strIndexLeftOutlet	= strIndexTheOutlet
	};


#pragma mark • Object Structure

typedef struct {
	LITTER_CORE_OBJECT(Object, coreObject);
	
	tTaus88DataPtr	tausData;
	
	double			tau,
					loc;
	eSymmetry		sym;		
	} objCauchy;


#pragma mark • Global Variables

SymbolPtr	gSymSymbol	= NIL,
			gNegSymbol	= NIL,
			gPosSymbol	= NIL;


#pragma mark • Function Prototypes
/*
	// Class message functions
static void*	CoshyNew	(double, Symbol*, long);
static void	CoshyFree	(objCauchy*);

	// Object message functions
static void CoshyBang	(objCauchy*);
static void CoshyTau	(objCauchy*, double);
static void CoshyLoc	(objCauchy*, double);
static void CoshySeed	(objCauchy*, long);
static void CoshySym	(objCauchy*);
static void CoshyPos	(objCauchy*);
static void CoshyNeg	(objCauchy*);
static void	CoshyTell	(objCauchy*, Symbol*, Symbol*);
//static void	CoshyExpect	(objCauchy*, Symbol*, long, Symbol*);
static void CoshyTattle	(objCauchy*);
static void	CoshyAssist	(objCauchy*, void* , long , long , char*);
static void	CoshyInfo	(objCauchy*);
*/

#pragma mark -

/*****************************  I M P L E M E N T A T I O N  ******************************/


#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	CoshyBang(me)
 *
 ******************************************************************************************/

static void
CoshyBang(
	objCauchy* me)
	
	{
	double	cauchy,
			scale	= me->tau,
			loc		= me->loc;
	
	switch (me->sym) {
	case symNeg:
		scale *= -1.0;
		// fall into next case...
	case symPos:
		cauchy = GenCauchyPosTaus88(me->tausData);
		break;
	
	default:
		// must be symSym...
		cauchy = GenCauchyStdTaus88(me->tausData);
		break;
		}
	
	cauchy *= scale;
	cauchy += loc;
	outlet_float(me->coreObject.o_outlet, cauchy);
	}


/******************************************************************************************
 *
 *	CoshyTau(me, iTau)
 *	CoshySym(me)
 *	CoshyPos(me)
 *	CoshyNeg(me)
 *	
 *	Set parameters.
 *	
 *	Allow negative values for tau (Cauchy is only defined for positive tau, but the
 *	arithmetic works fine with non-positive values).
 *	
 ******************************************************************************************/

static void CoshyTau(objCauchy* me, double iTau)	{ me->tau = iTau; }
static void CoshyLoc(objCauchy* me, double iLoc)	{ me->loc = iLoc; }

static void CoshySym(objCauchy* me)		{ me->sym = symSym; }
static void CoshyPos(objCauchy* me)		{ me->sym = symPos; }
static void CoshyNeg(objCauchy* me)		{ me->sym = symNeg; }
	

/******************************************************************************************
 *
 *	CoshySeed(me, iSeed)
 *
 ******************************************************************************************/

static void CoshySeed(objCauchy* me, long iSeed)
	{ Taus88Seed(me->tausData, (unsigned long) iSeed); }


#pragma mark -
#pragma mark • Attribute/Information Functions

/******************************************************************************************
 *
 *	CoshyTattle(me)
 *
 *	Post state information
 *
 ******************************************************************************************/

	static short inline SymmetryResStrIndex(eSymmetry iSym)
		// Relies on the order of values in the tSymmetry enum matching the order of
		// Indices to the relevant strings
		{ return strIndexFragSym + iSym; }
	
	static void inline SymmetryName(eSymmetry iSym, char oNameStr[])
		{
		
		LitterGetIndString(SymmetryResStrIndex(iSym), oNameStr);
		
		if (oNameStr[0] == '\0') {
			// We're probably running inside a collective, with no support for resources
			// Emergency procedure:
			sprintf(oNameStr, "%ld (resource for symmetry name not available)", (long) iSym);
			}
		
		}

static void
CoshyTattle(
	objCauchy* me)
	
	{
	char	symString[kMaxResourceStrSize];
	double	tau = me->tau;
	
	SymmetryName(me->sym, symString);
	
	post("%s state",
			kClassName);
	post(	"  --> Calculating %s Cauchy distribution", symString);
	post("  tau = %lf", tau);
	if (tau <= 0.0)
		post("   Note: the Cauchy distribution is defined only for positive tau, but we'll do what we can.");
	post("  location = %lf", me->loc);
	
	}


/******************************************************************************************
 *
 *	CoshyAssist
 *
 ******************************************************************************************/

static void
CoshyAssist(
	objCauchy*	me,
	void*		sym,					// We don't use this parameter
	long		iDir,
	long		iArgNum,
	char*		oCStr)
	
	{
	#pragma unused(sym)
	
	LitterAssistResFrag(iDir, iArgNum, strIndexLeftInlet, strIndexLeftOutlet,
						oCStr, SymmetryResStrIndex(me->sym));
	}



/******************************************************************************************
 *
 *	DoExpect()
 *
 *	Helper function for calculating expected values (mean, standard deviation, etc.)
 *	
 ******************************************************************************************/

static double 
DoExpect(
	objCauchy*		me,
	eExpectSelector iSel)
	
	{
	const double k4pi = 12.5663706144;
	
	double result = 0.0 / 0.0;							// Initially undefined
	
	switch (iSel) {
	case expMedian:
		if (me->sym == symSym) result = me->loc;
		break;
	case expMode:
		result = me->loc;
		break;
	case expEntropy:
		result = log(k4pi * me->tau);
		if (me->sym != symSym) result -= 1.0;			// ?? Not completely sure of this
		break;
	default:
		// Everything else is undefined
		break;
		}
	
	return result;
	}

/******************************************************************************************
 *
 *	Attribute Funtions
 *
 *	Functions for Inspector/Get Info... dialog
 *	
 ******************************************************************************************/

#if LITTER_USE_OBEX

//	static t_max_err CoshyGetMin(objCauchy* me, void* iAttr, long* ioArgC, Atom** ioArgV)
//		{ return LitterGetAttrFloat(DoExpect(me, expMin), ioArgC, ioArgV); }
//	static t_max_err CoshyGetMax(objCauchy* me, void* iAttr, long* ioArgC, Atom** ioArgV)
//		{ return LitterGetAttrFloat(DoExpect(me, expMax), ioArgC, ioArgV); }
//	static t_max_err CoshyGetMean(objCauchy* me, void* iAttr, long* ioArgC, Atom** ioArgV)
//		{ return LitterGetAttrFloat(DoExpect(me, expMean), ioArgC, ioArgV); }
//	static t_max_err CoshyGetMedian(objCauchy* me, void* iAttr, long* ioArgC, Atom** ioArgV)
//		{ return LitterGetAttrFloat(DoExpect(me, expMedian), ioArgC, ioArgV); }
	static t_max_err CoshyGetMode(objCauchy* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMode), ioArgC, ioArgV); }
//	static t_max_err CoshyGetVar(objCauchy* me, void* iAttr, long* ioArgC, Atom** ioArgV)
//		{ return LitterGetAttrFloat(DoExpect(me, expVar), ioArgC, ioArgV); }
//	static t_max_err CoshyGetStdDev(objCauchy* me, void* iAttr, long* ioArgC, Atom** ioArgV)
//		{ return LitterGetAttrFloat(DoExpect(me, expStdDev), ioArgC, ioArgV); }
//	static t_max_err CoshyGetSkew(objCauchy* me, void* iAttr, long* ioArgC, Atom** ioArgV)
//		{ return LitterGetAttrFloat(DoExpect(me, expSkew), ioArgC, ioArgV); }
//	static t_max_err CoshyGetKurtosis(objCauchy* me, void* iAttr, long* ioArgC, Atom** ioArgV)
//		{ return LitterGetAttrFloat(DoExpect(me, expKurtosis), ioArgC, ioArgV); }
	static t_max_err CoshyGetEntropy(objCauchy* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expEntropy), ioArgC, ioArgV); }
	
	
	static inline void
	AddInfo(void)
		{
		Object*	attr;
		Symbol*	symFloat64		= gensym("float64");
		
		// Read-Write Attributes
		attr = attr_offset_new("tau", symFloat64, 0, NIL, NIL, calcoffset(objCauchy, tau));
		class_addattr(gObjectClass, attr);
		attr = attr_offset_new("loc", symFloat64, 0, NIL, NIL, calcoffset(objCauchy, loc));
		class_addattr(gObjectClass, attr);
		
		// Read-Only Attributes
//		attr = attribute_new("min", symFloat64, kAttrFlagsReadOnly, (method) CoshyGetMin, NULL);
//		class_addattr(gObjectClass, attr);
//		attr = attribute_new("max", symFloat64, kAttrFlagsReadOnly, (method) CoshyGetMax, NULL);
//		class_addattr(gObjectClass, attr);
//		attr = attribute_new("mean", symFloat64, kAttrFlagsReadOnly, (method) CoshyGetMean, NULL);
//		class_addattr(gObjectClass, attr);
//		attr = attribute_new("median", symFloat64, kAttrFlagsReadOnly, (method) CoshyGetMedian, NULL);
//		class_addattr(gObjectClass, attr);
		attr = attribute_new("mode", symFloat64, kAttrFlagsReadOnly, (method) CoshyGetMode, NULL);
		class_addattr(gObjectClass, attr);
//		attr = attribute_new("var", symFloat64, kAttrFlagsReadOnly, (method) CoshyGetVar, NULL);
//		class_addattr(gObjectClass, attr);
//		attr = attribute_new("stddev", symFloat64, kAttrFlagsReadOnly, (method) CoshyGetStdDev, NULL);
//		class_addattr(gObjectClass, attr);
//		attr = attribute_new("skew", symFloat64, kAttrFlagsReadOnly, (method) CoshyGetSkew, NULL);
//		class_addattr(gObjectClass, attr);
//		attr = attribute_new("kurtosis", symFloat64, kAttrFlagsReadOnly, (method) CoshyGetKurtosis, NULL);
//		class_addattr(gObjectClass, attr);
		attr = attribute_new("entropy", symFloat64, kAttrFlagsReadOnly, (method) CoshyGetEntropy, NULL);
		class_addattr(gObjectClass, attr);
		}

static void
CoshyTell(
	objCauchy*	me,
	Symbol*		iTarget,
	Symbol*		iAttrName)
	
	{
	long	argC = 0;
	Atom*	argV = NIL;
		
	if (object_attr_getvalueof(me, iAttrName, &argC, &argV) == MAX_ERR_NONE) {
		ForwardAnything(iTarget, iAttrName, argC, argV);
		freebytes(argV, argC * sizeof(Atom));	// ASSERT (argC > 0 && argV != NIL)
		}
	}

#else

static void CoshyInfo(objCauchy* me)
	{ LitterInfo(kClassName, &me->coreObject, (method) CoshyTattle); }

static inline void AddInfo(void)
	{ LitterAddCant((method) CoshyInfo, "info"); }
		
static void CoshyTell(objCauchy* me, Symbol* iTarget, Symbol* iAttrName)
	{ LitterExpect((tExpectFunc) DoExpect, (Object*) me, iAttrName, iTarget, TRUE); }

#endif

#pragma mark -
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	CoshyFree()
 *	CoshyNew()
 *
 ******************************************************************************************/

static void CoshyFree(objCauchy* me)
	{ Taus88Free(me->tausData); }							// Taus88Free is NIL-safe

static void*
CoshyNew(
	double	iTau,
	Symbol*	iSymmetry,
	long	iSeed)
	
	{
	const double	kDefTau		= 1.0,
					kDefLoc		= 0.0;
	
	objCauchy*		me			= NIL;
	tTaus88DataPtr	myTTStuff	= NIL;
	
	// Run through initialization parameters from right to left, handling defaults
	if (iSeed != 0) {
		myTTStuff = Taus88New(iSeed);
		goto noMoreDefaults;
		}
	
	if (iSymmetry->s_name[0] == '\0')						// Quick test for null symbol
		iSymmetry = gSymSymbol;
	else goto noMoreDefaults;
	
	if (iTau == 0.0)
		iTau = kDefTau;
noMoreDefaults:
	// Finished checking intialization parameters

	// Let Max allocate us, our inlets, and outlets
	me = (objCauchy*) LitterAllocateObject();
	
	floatin(me, 2);											// loc inlet
	floatin(me, 1);											// tau inlet
	
	floatout(me);
	
	// Store object components
	me->tausData	= myTTStuff;
	me->tau			= iTau;
	me->loc			= kDefLoc;
	
		// The following would be sort of nicer with a switch, but C can't switch against
		// values not known at compile time
	me->sym = symSym;
	if (iSymmetry == gPosSymbol)
		me->sym = symPos;
	else if (iSymmetry == gNegSymbol)
		me->sym = symNeg;
	else if (iSymmetry != gSymSymbol)
		error("%s doesn't understand %s", kClassName, iSymmetry->s_name);
	
	return me;
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
main(void)
	
	{
	const tTypeList myArgTypes = {
						A_DEFFLOAT,				// Optional arguments:	1. Tau
						A_DEFSYM,				//						2. sym, pos, or neg
						A_DEFLONG,				// 						3. seed
						A_NOTHING
						};
	
	LITTER_CHECKTIMEOUT(kClassName);
	
	// Standard Max setup() call
	LitterSetupClass(	kClassName,
						sizeof(objCauchy),
						LitterCalcOffset(objCauchy),			// Class object size
						(method) CoshyNew,			// Instance creation function
						(method) CoshyFree,			// Custom deallocation function
						NIL,						// No menu function
						myArgTypes);				
	

	// Messages
	LITTER_TIMEBOMB LitterAddBang((method) CoshyBang);
	LitterAddMess1	((method) CoshyTau,	"ft1",		A_FLOAT);
	LitterAddMess1	((method) CoshyLoc,	"ft2",		A_FLOAT);
	LitterAddMess1	((method) CoshySeed,"seed",		A_DEFLONG);
	LitterAddMess0	((method) CoshySym,	"sym");
	LitterAddMess0	((method) CoshyPos,	"pos");
	LitterAddMess0	((method) CoshyNeg,	"neg");
	
	LitterAddMess2	((method) CoshyTell,"tell", A_SYM, A_SYM);
	LitterAddMess0	((method) CoshyTattle,	"tattle");
	LitterAddCant	((method) CoshyTattle,	"dblclick");
	LitterAddCant	((method) CoshyAssist,	"assist");
	
	AddInfo();
	
	// Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();
	
	// Stash pointers to commonly used symbols
	gSymSymbol	= gensym("sym");
	gPosSymbol	= gensym("pos");
	gNegSymbol	= gensym("neg");
	
	}


