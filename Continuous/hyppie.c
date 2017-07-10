/*
	File:		hyppie.c

	Contains:	Max external object generating a hyperbolic cosine distribution.

	Written by:	Peter Castine

	Copyright:	© 2001-2002 Peter Castine

	Change History (most recent first):

         <8>   30–3–2006    pc      Update for new LitterLib organization
         <7>     18–2–06    pc      Add support for expect message and hyperbolic secant
                                    distribution.
         <6>     15–1–04    pc      Avoid possible memory leak if seed argument is used.
         <5>     11–1–04    pc      Update for Windows.
         <4>    7–7–2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <3>  30–12–2002    pc      Drop faux 'Vers' resource and related modifications. Add object
                                    version to HyppieInfo().
         <2>  28–11–2002    pc      Tidy up after initial check-in.
         <1>  28–11–2002    pc      Initial check in.
		30-Jun-2001:	Modified to use Taus88 instead of TT800 as default generator.
						Also update to use new ULong2Unitxx() functions.
		13-Apr-2001:	General LPP overhaul, including removal of scaling parameter tau
						(Its inclusion seems to have been an idiosyncracy, and if we're
						expecting people to pass norm's output through scampi to get a 
						general Gauss function, then it seems fair enough to do the same
						here.)
		2-Apr-2001:		First implementation.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "LitterLib.h"
#include "TrialPeriodUtils.h"
#include "Taus88.h"
#include "MiscUtils.h"


#pragma mark • Constants

const char	kClassName[]		= "lp.hyppie";			// Class name

	// Indices for STR# resource
enum {
	strIndexInBang		= lpStrIndexLastStandard + 1,
	strIndexTheOutlet,
	
	strIndexLeftInlet	= strIndexInBang,
	strIndexLeftOutlet	= strIndexTheOutlet
	};

enum hypVar {
	hypCos	= 0,
	hypSec
	};



#pragma mark • Type Definitions

typedef enum hypVar eHypVar;


#pragma mark • Object Structure

typedef struct {
	LITTER_CORE_OBJECT(Object, coreObject);
	
	tTaus88DataPtr	tausData;
	
	eHypVar			variant;
	} objHyperan;



#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	HyppieBang(me)
 *	HyppieFloat(me, iVal)
 *
 ******************************************************************************************/

	static double CalcHypercos(double x)
		{ return log(tan(kHalfPi * x)); }
	
	static double CalcHypersec(double x)
		{ return (-2.0 / kPi) * asinh(1.0 / tan(kPi * x)); }

static void
HyppieBang(
	objHyperan* me)
	
	{
	double h = ULong2Unit_zo(Taus88(me->tausData));
	
	h = (me->variant == hypCos) ? CalcHypercos(h) : CalcHypersec(h);
	
	outlet_float(me->coreObject.o_outlet, h);
	}

static void
HyppieFloat(
	objHyperan*	me,
	double		iVal)
	
	{
	
	if (0.0 < iVal && iVal < 1.0) {
		iVal = (me->variant == hypCos) ? CalcHypercos(iVal) : CalcHypersec(iVal);
		
		outlet_float(me->coreObject.o_outlet, iVal);
		}
	}

/******************************************************************************************
 *
 *	HyppieCos(me)
 *	HyppieSec(me)
 *
 ******************************************************************************************/

static void HyppieCos(objHyperan* me)		{ me->variant = hypCos; }
static void HyppieSec(objHyperan* me)		{ me->variant = hypSec; }

/******************************************************************************************
 *
 *	HyppieSeed(me, iSeed)
 *
 ******************************************************************************************/

static void HyppieSeed(objHyperan* me, long iSeed)
	{ Taus88Seed(me->tausData, (unsigned long) iSeed); }


#pragma mark -
#pragma mark • Attribute/Information Functions

/******************************************************************************************
 *
 *	HyppieAssist(me, iBox, iDir, iArgNum, oCStr)
 *	HyppieTattle(me)
 *
 ******************************************************************************************/

static void HyppieAssist(objHyperan* me, void* box, long iDir, long iArgNum, char*oCStr)
	{
	#pragma unused(me, box)
	
	LitterAssist(iDir, iArgNum, strIndexLeftInlet, strIndexLeftOutlet, oCStr);
	}

static void
HyppieTattle(
	objHyperan* me)
	
	{
	static unsigned sTattleCount = 0;
	
	char* varStr = (me->variant == hypCos)
						? "cosine"
						: "secant";
	
	// Listen closely, we are only going to say this one time.
	if (sTattleCount++ == 0) {
		post((char*) kClassName);
		if (sTattleCount & Taus88(me->tausData))
				post("  Hey man, like, no parameters, just hyperbolic %s.", varStr);
		else	post("  Can you dig it? No parameters. Just groove with %s.", varStr);
		}
	else switch (sTattleCount & Taus88(me->tausData)) {
		case 1: post("%s: Groovy, man", kClassName); break;
		case 2: post("%s: Far out, dude", kClassName); break;
		case 4: post("outasight"); break;
		case 8: post("True insight will come with lp.ginger"); break;
		case 16:
			{
			int c = sTattleCount;
			do { post(""); } while (--c > 16);
			post("   urp");
			}
			break;
		case 32:
			post("ZAP!");
			do { post("    %c", (char) (Taus88(me->tausData) & 0x7f)); }
				while (sTattleCount-- > 0);
			post("UNZAP!");
			break;
		default:
			post("Like hyperbolic %s, man", varStr);
			break;
		}
	
	}


/******************************************************************************************
 *
 *	DoExpect()
 *
 *	Helper function for calculating expected values (mean, standard deviation, etc.)
 *	
 ******************************************************************************************/

static double DoExpect(objHyperan* me, eExpectSelector iSel)
	{
	#pragma unused(me)
	
	const double 	kCatalan		= 0.9159655942,
					kHypSecEntropy	= 4.0 * kCatalan / kPi;
	
	double	result	= 0.0 / 0.0;					// Initially undefined
	
	switch(iSel) {
	case expMedian:
	case expMode:
		result = 0.0;
		break;
	default:
		if (me->variant == hypSec) switch (iSel) {
			case expMean:									// Fall into next case...
			case expSkew:		result = 0.0;				break;
			case expVar:									// Fall into next case...
			case expStdDev:		result = 1.0;				break;
			case expKurtosis:	result = 2.0;				break;
			case expEntropy:	result = kHypSecEntropy;	break;
			default:										break;
			}
		// All this stuff is undefined for hyperbolic cosine
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

	static t_max_err HyppieGetMin(objHyperan* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMin), ioArgC, ioArgV); }
	static t_max_err HyppieGetMax(objHyperan* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMax), ioArgC, ioArgV); }
	static t_max_err HyppieGetMean(objHyperan* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMean), ioArgC, ioArgV); }
	static t_max_err HyppieGetMedian(objHyperan* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMedian), ioArgC, ioArgV); }
	static t_max_err HyppieGetMode(objHyperan* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMode), ioArgC, ioArgV); }
	static t_max_err HyppieGetVar(objHyperan* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expVar), ioArgC, ioArgV); }
	static t_max_err HyppieGetStdDev(objHyperan* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expStdDev), ioArgC, ioArgV); }
	static t_max_err HyppieGetSkew(objHyperan* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expSkew), ioArgC, ioArgV); }
	static t_max_err HyppieGetKurtosis(objHyperan* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expKurtosis), ioArgC, ioArgV); }
	static t_max_err HyppieGetEntropy(objHyperan* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expEntropy), ioArgC, ioArgV); }
	
	static inline void
	AddInfo(void)
		{
		Object*	attr;
		Symbol*	symFloat64		= gensym("float64");
		
		// Read-Only Attributes
		attr = attribute_new("min", symFloat64, kAttrFlagsReadOnly, (method) HyppieGetMin, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("max", symFloat64, kAttrFlagsReadOnly, (method) HyppieGetMax, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("mean", symFloat64, kAttrFlagsReadOnly, (method) HyppieGetMean, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("median", symFloat64, kAttrFlagsReadOnly, (method) HyppieGetMedian, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("mode", symFloat64, kAttrFlagsReadOnly, (method) HyppieGetMode, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("var", symFloat64, kAttrFlagsReadOnly, (method) HyppieGetVar, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("stddev", symFloat64, kAttrFlagsReadOnly, (method) HyppieGetStdDev, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("skew", symFloat64, kAttrFlagsReadOnly, (method) HyppieGetSkew, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("kurtosis", symFloat64, kAttrFlagsReadOnly, (method) HyppieGetKurtosis, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("entropy", symFloat64, kAttrFlagsReadOnly, (method) HyppieGetEntropy, NULL);
		class_addattr(gObjectClass, attr);
		
		}

static void
HyppieTell(
	objHyperan*	me,
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

static void HyppieInfo(objHyperan* me)
	{ LitterInfo(kClassName, &me->coreObject, (method) HyppieTattle); }

static inline void AddInfo(void)
	{ LitterAddCant((method) HyppieInfo, "info"); }
		
static void HyppieTell(objHyperan* me, Symbol* iTarget, Symbol* iAttrName)
	{ LitterExpect((tExpectFunc) DoExpect, (Object*) me, iAttrName, iTarget, TRUE); }

#endif



#pragma mark -
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	HyppieFree(me)
 *	HyppieNew(iOracle)
 *
 ******************************************************************************************/

static void HyppieFree(objHyperan* me)
	{ Taus88Free(me->tausData); }							// Taus88Free is NIL-safe

static void*
HyppieNew(
	long	iSeed)
	
	{
	objHyperan*		me			= NIL;
	tTaus88DataPtr	myTausStuff	= NIL;
	
	// Run through initialization parameters from right to left, handling defaults
	if (iSeed != 0) {
		myTausStuff = Taus88New(iSeed);
		}
noMoreDefaults:
	// Finished checking intialization parameters
	
	// Let Max try to allocate us, and our outlet (no extra parameter inlets on this one!)
	me = (objHyperan*) LitterAllocateObject();
	
	floatout(me);								// Access through me->coreObject.o_outlet
	
	// Store object components
	me->tausData	= myTausStuff;
	me->variant		= hypCos;
	
	return me;
	}

#pragma mark -
/******************************************************************************************
 *
 *	HyppieTattle(me)
 *
 *	Post state information
 *
 ******************************************************************************************/


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
						A_DEFLONG,					// Optional arguments:	1. seed
						A_NOTHING
						};
	
	// Standard Max setup() call
	LitterSetupClass(	kClassName,
						sizeof(objHyperan),
						LitterCalcOffset(objHyperan),			// Class object size
						(method) HyppieNew,			// Instance creation function
						(method) HyppieFree,		// Custom deallocation function
						NIL,						// No menu function
						myArgTypes);				
	
	// Messages
	LITTER_TIMEBOMB LitterAddBang	((method) HyppieBang);
	LITTER_TIMEBOMB LitterAddMess1	((method) HyppieFloat, "float", A_FLOAT);
	LitterAddMess0	((method) HyppieCos,	"cos");
	LitterAddMess0	((method) HyppieSec,	"sec");
	LitterAddMess1	((method) HyppieSeed,	"seed",	A_DEFLONG);
	LitterAddMess2	((method) HyppieTell,	"tell", A_SYM, A_SYM);
	LitterAddCant	((method) HyppieTattle,	"dblclick");
	LitterAddMess0	((method) HyppieTattle,	"tattle");
	LitterAddCant	((method) HyppieAssist,	"assist");
	
	AddInfo();
	
	// Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();
	
	}

