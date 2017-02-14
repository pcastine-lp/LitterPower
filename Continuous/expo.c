/*
	File:		expo.c

	Contains:	Max external object generating an exponential distribution.

	Written by:	Peter Castine

	Copyright:	© 2001-2002 Peter Castine

	Change History (most recent first):

         <9>   29Ð3Ð2006    pc      Update for new LitterLib organization
         <8>     18Ð2Ð06    pc      Add support for expect message.
         <7>     10Ð1Ð06    pc      Updated to use LitterAssistResFrag()
         <6>     15Ð1Ð04    pc      Avoid possible memory leak if seed argument is used.
         <5>     11Ð1Ð04    pc      Update for Windows.
         <4>    7Ð7Ð2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <3>  30Ð12Ð2002    pc      Drop faux 'Vers' resource and related modifications. Add object
                                    version to ExpoInfo().
         <2>  28Ð11Ð2002    pc      Tidy up after initial check-in.
         <1>  28Ð11Ð2002    pc      Initial check in.
		13-Apr-2001:	Merged bilateral (Laplace) distribution into this object.
						Renamed to follow new LPP conventions, added ExpoTattle()
		2-Apr-2001:		First implementation.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark ¥ Include Files

#include "LitterLib.h"
#include "TrialPeriodUtils.h"
#include "Taus88.h"
#include "MiscUtils.h"

#include <math.h>


#pragma mark ¥ Constants

const char		kClassName[]		= "lp.expo";			// Class name


	
	// Indices for STR# resource
enum {
	strIndexInBang		= lpStrIndexLastStandard + 1,
	strIndexInScale,
	strIndexInLoc,
	
	strIndexTheOutlet,
	
	strIndexFragNexExp,
	strIndexFragLaplace,
	strIndexFragPosExp
	};


#pragma mark ¥ Object Structure

typedef struct {
	LITTER_CORE_OBJECT(Object, coreObject);
	
	tTaus88DataPtr	tausData;
	
	double			tau,					// = 1/lambda	
					loc;					
	eSymmetry		sym;
	} objExpran;


#pragma mark ¥ Global Variables

SymbolPtr	gSymSymbol	= NIL,
			gNegSymbol	= NIL,
			gPosSymbol	= NIL;



#pragma mark -

/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark ¥ Object Message Handlers

/******************************************************************************************
 *
 *	ExpoBang(me)
 *	ExpoFloat(me)
 *
 ******************************************************************************************/

static void
ExpoBang(
	objExpran* me)
	
	{
	double	x = me->tau;
	
	if (x > 0.0) {
		eSymmetry sym = me->sym;
		if (sym == symSym) {
			x *= Unit2Laplace( ULong2Unit_zO(Taus88(me->tausData)) );	
			}
		else {
			// Either (standard) exponential or negative variant
			x *= Unit2Exponential( ULong2Unit_zO(Taus88(me->tausData)) );
			x *= sym;
			}
		}
	
	x += me->loc;
	
	outlet_float(me->coreObject.o_outlet, x);
	
	}

static void
ExpoFloat(
	objExpran*	me,
	double		iVal)
	
	{
	
	if (0.0 < iVal && iVal <= 1.0) {
		double	x = me->tau;
		if (x > 0.0) {
			eSymmetry sym = me->sym;
			if (sym == symSym) {
				x *= Unit2Laplace( ULong2Unit_zO(Taus88(me->tausData)) );	
				}
			else {
				// Either (standard) exponential or negative variant
				x *= Unit2Exponential( ULong2Unit_zO(Taus88(me->tausData)) );
				x *= sym;
				}
			}
		
		outlet_float(me->coreObject.o_outlet, x);
		}
	
	}
	
	
/******************************************************************************************
 *
 *	ExpoLamda(me, iLamda)
 *	ExpoTau(me, iTau)
 *	ExpoSym(me)
 *	ExpoPos(me)
 *	ExpoNeg(me)
 *	ExpoSeed(me, iSeed)
 *	
 *	Set parameters, making sure nothing bad happens.
 *	
 ******************************************************************************************/

static void ExpoLamda(objExpran* me, double iLamda)
	{ me->tau = (iLamda > 0.0) ? (1.0 / iLamda) : 0.0; }
	
static void ExpoTau(objExpran* me, double iTau)
	{ me->tau = (iTau > 0.0) ? iTau : 0.0; }

static void ExpoLocation(objExpran* me, double iLoc)
	{ me->loc = iLoc; }
	
static void ExpoSym(objExpran* me)		{ me->sym = symSym; }
static void ExpoPos(objExpran* me)		{ me->sym = symPos; }
static void ExpoNeg(objExpran* me)		{ me->sym = symNeg; }

static void ExpoSeed(objExpran* me, long iSeed)
	{ Taus88Seed(me->tausData, (unsigned long) iSeed); }


#pragma mark ¥ Class Message Handlers

/******************************************************************************************
 *
 *	ExpoFree(me)
 *	ExpoNew(iLambda, iSymmetry, iSeed)
 *
 ******************************************************************************************/

static void ExpoFree(objExpran* me)
	{ Taus88Free(me->tausData); }							// Taus88Free is NIL-safe

static void*
ExpoNew(
	double	iLambda,
	Symbol*	iSymmetry,
	long	iSeed)
	
	{
	const double	kDefLambda	= 1.0;
	
	objExpran*		me			= NIL;
	tTaus88DataPtr	myTausStuff	= NIL;
	
	// Run through initialization parameters from right to left, handling defaults
	if (iSeed != 0) {
		myTausStuff = Taus88New(iSeed);
		goto noMoreDefaults;
		}
	
	if (iSymmetry->s_name[0] == '\0')						// Quick test for empty symbol
		iSymmetry = gPosSymbol;
	else goto noMoreDefaults;
	
	if (iLambda == 0.0)
		iLambda = kDefLambda;
noMoreDefaults:
	// Finished checking intialization parameters
	
	// Let Max allocate us, our inlets, and outlets
	me = (objExpran*) LitterAllocateObject();
	
	floatin(me, 2);											// location inlet
	floatin(me, 1);											// lamda inlet
	
	floatout(me);
	
	// Store object components
	me->tausData	= myTausStuff;
	ExpoLamda(me, iLambda);				// Lamda defaults to 1.0
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
	
	return me;
	}

#pragma mark -
#pragma mark ¥ Attribute/Information Functions

/******************************************************************************************
 *
 *	ExpoTattle(me)
 *
 *	Post state information
 *
 ******************************************************************************************/

	static short inline SymmetryResStrIndex(eSymmetry iSym)
		// Relies on the order of values in the eSymmetry enum matching the order of
		// Indices to the relevant strings
		{ return strIndexFragLaplace + iSym; }
	
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
ExpoTattle(
	objExpran* me)
	
	{
	char	symString[kMaxResourceStrSize];
	double	tau = me->tau;
	
	SymmetryName(me->sym, symString);
	
	post("%s state", kClassName);
	post("  --> Calculating %s distribution", symString);
	post("  tau = %lf (i.e., mean = %lf)",
			tau,
			(tau != 0.0) ? (1.0 / tau) : 0.0);
	if (tau == 0.0)
		post("   Note: The distribution is undefined for this value of lambda/tau");
	
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
	objExpran*		me,
	eExpectSelector	iSel)
	
	{
	const double	kSqrt2	= 1.4142135624,
					kLn2	= 0.6931471806;
	
	double	result	= me->loc,						// Good a place as any to start
			tau		= me->tau;						// tau = lambda = 1/scale
	
	if (me->sym == symSym) switch(iSel) {
		default:									// This covers mean, mode, median
			break;									// Nothing to do, result == me->loc
		case expVar:
			result	= tau * tau;
			result += result;
			break;
		case expStdDev:
			result = kSqrt2 * tau;
			break;
		case expSkew:
			result = 0.0;
			break;
		case expKurtosis:
			result = 3.0;
			break;
		case expMin:
			result = -1.0 / 0.0;
			break;
		case expMax:
			result = 1.0 / 0.0;
			break;
		case expEntropy:
			result = (tau > 0.0) ? 1.0 - log(tau + tau) : 0.0;
			break;
		}
	
		// Otherwise either symPos (== 1) or symNeg (== -1)
	else switch(iSel) {
		case expMean:
			result += (((me->sym) > 0) ? 1.0 : -1.0) * tau;
			break;
		case expMedian:
			result += (((me->sym) > 0) ? 1.0 : -1.0) * kLn2 * tau;
			break;
		case expMode:
			// Nothing to do
			break;
		case expVar:
			result  = tau * tau;
			break;
		case expStdDev:
			result = tau;
			break;
		case expSkew:
			result = ((me->sym) > 0) ? 2.0 : -2.0;
			break;
		case expKurtosis:
			result = 6.0;
			break;
		case expMin:
			result = ((me->sym) > 0) ? me->loc : 1.0 / 0.0;
			break;
		case expMax:
			result = ((me->sym) < 0) ? me->loc : -1.0 / 0.0;
			break;
		case expEntropy:
			result = (tau > 0.0) ? 1.0 - log(tau) : 0.0;
			break;
		default:
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

	static t_max_err ExpoGetMin(objExpran* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMin), ioArgC, ioArgV); }
	static t_max_err ExpoGetMax(objExpran* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMax), ioArgC, ioArgV); }
	static t_max_err ExpoGetMean(objExpran* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMean), ioArgC, ioArgV); }
	static t_max_err ExpoGetMedian(objExpran* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMedian), ioArgC, ioArgV); }
	static t_max_err ExpoGetMode(objExpran* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMode), ioArgC, ioArgV); }
	static t_max_err ExpoGetVar(objExpran* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expVar), ioArgC, ioArgV); }
	static t_max_err ExpoGetStdDev(objExpran* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expStdDev), ioArgC, ioArgV); }
	static t_max_err ExpoGetSkew(objExpran* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expSkew), ioArgC, ioArgV); }
	static t_max_err ExpoGetKurtosis(objExpran* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expKurtosis), ioArgC, ioArgV); }
	static t_max_err ExpoGetEntropy(objExpran* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expEntropy), ioArgC, ioArgV); }
	
	static t_max_err ExpoSetAttrTau(objExpran* me, void* iAttr, long iArgC, Atom iArgV[])
		{
		
		if (iArgC > 0 && iArgV != NIL)
			ExpoTau(me, AtomGetFloat(iArgV));
		
		return MAX_ERR_NONE;
		}
	
	static inline void
	AddInfo(void)
		{
		Object*	attr;
		Symbol*	symFloat64		= gensym("float64");
		Symbol* symLong			= gensym("long");
		
		// Read-Write Attributes
		attr = attr_offset_new("loc", symLong, 0, NIL, NIL, calcoffset(objExpran, loc));
		class_addattr(gObjectClass, attr);
		attr = attr_offset_new("sym", symLong, 0, NIL, NIL, calcoffset(objExpran, sym));
		attr_addfilter_clip(attr, -1.0, 1.0, true, true);
		class_addattr(gObjectClass, attr);
		attr = attr_offset_new(	"tau", symFloat64, 0,
								NIL, (method) ExpoSetAttrTau,
								calcoffset(objExpran, tau));
		class_addattr(gObjectClass, attr);
		
		// Read-Only Attributes
		attr = attribute_new("min", symFloat64, kAttrFlagsReadOnly, (method) ExpoGetMin, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("max", symFloat64, kAttrFlagsReadOnly, (method) ExpoGetMax, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("mean", symFloat64, kAttrFlagsReadOnly, (method) ExpoGetMean, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("median", symFloat64, kAttrFlagsReadOnly, (method) ExpoGetMedian, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("mode", symFloat64, kAttrFlagsReadOnly, (method) ExpoGetMode, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("var", symFloat64, kAttrFlagsReadOnly, (method) ExpoGetVar, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("stddev", symFloat64, kAttrFlagsReadOnly, (method) ExpoGetStdDev, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("skew", symFloat64, kAttrFlagsReadOnly, (method) ExpoGetSkew, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("kurtosis", symFloat64, kAttrFlagsReadOnly, (method) ExpoGetKurtosis, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("entropy", symFloat64, kAttrFlagsReadOnly, (method) ExpoGetEntropy, NULL);
		class_addattr(gObjectClass, attr);
		}

static void
ExpoTell(
	objExpran*	me,
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

static void ExpoInfo(objExpran* me)
	{ LitterInfo(kClassName, &me->coreObject, (method) ExpoTattle); }

static inline void AddInfo(void)
	{ LitterAddCant((method) ExpoInfo, "info"); }
		
void ExpoTell(objExpran* me, Symbol* iTarget, Symbol* iAttrName)
	{ LitterExpect((tExpectFunc) DoExpect, (Object*) me, iAttrName, iTarget, TRUE); }

#endif


/******************************************************************************************
 *
 *	ExpoAssist
 *	ExpoInfo(me)
 *
 ******************************************************************************************/

static void
ExpoAssist(objExpran* me, void* box, long iDir, long iArgNum, char* oCStr)
	
	{
	#pragma unused(box)
	
	LitterAssistResFrag(iDir, iArgNum, strIndexInBang, strIndexTheOutlet,
						oCStr, SymmetryResStrIndex(me->sym));
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
						A_DEFFLOAT,				// Optional arguments:	1. lamda
						A_DEFSYM,				//						2. sym, pos, or neg
						A_DEFLONG,				// 						3. seed
						A_NOTHING
						};
	
	LITTER_CHECKTIMEOUT(kClassName);
	
	// Standard Max setup() call
	LitterSetupClass(	kClassName,
						sizeof(objExpran),
						LitterCalcOffset(objExpran),		// Class object size
						(method) ExpoNew,					// Instance creation function
						(method) ExpoFree,					// Custom deallocation function
						NIL,
						myArgTypes);				
	

	// Messages
	LITTER_TIMEBOMB LitterAddBang((method) ExpoBang);
	LITTER_TIMEBOMB addfloat((method) ExpoFloat);
	LitterAddMess1	((method) ExpoLamda,	"ft1",	A_FLOAT);
	LitterAddMess1	((method) ExpoLocation,	"ft2",	A_FLOAT);
	LitterAddMess1	((method) ExpoTau,		"tau",	A_FLOAT);		// tau = 1/lambda
	LitterAddMess0	((method) ExpoSym,		"sym");
	LitterAddMess0	((method) ExpoPos,		"pos");
	LitterAddMess0	((method) ExpoNeg,		"neg");
	LitterAddMess1	((method) ExpoSeed,		"seed",	A_DEFLONG);
	LitterAddMess2	((method) ExpoTell,		"tell", A_SYM, A_SYM);
	LitterAddCant	((method) ExpoTattle,	"dblclick");
	LitterAddMess0	((method) ExpoTattle,	"tattle");
	LitterAddCant	((method) ExpoAssist,	"assist");
	
	AddInfo();

	// Stash pointers to commonly used symbols
	gSymSymbol	= gensym("sym");
	gPosSymbol	= gensym("pos");
	gNegSymbol	= gensym("neg");
	
	// Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();
	
	}
