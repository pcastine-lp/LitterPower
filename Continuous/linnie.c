/*
	File:		linnie.c

	Contains:	Max external object generating a Guassian distribution.

	Written by:	Peter Castine

	Copyright:	© 2001-2002 Peter Castine

	Change History (most recent first):

         <8>   23–3–2006    pc      Support arbitrary triangular distributions.
         <7>      4–3–06    pc      Add support for expect message.
         <6>     15–1–04    pc      Avoid possible memory leak if seed argument is used.
         <5>     11–1–04    pc      Update for Windows.
         <4>    7–7–2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <3>  30–12–2002    pc      Drop faux 'Vers' resource and related modifications. Add object
                                    version to LinnieInfo().
         <2>  28–11–2002    pc      Tidy up after initial check-in.
         <1>  28–11–2002    pc      Initial check in.
		30-Jun-2001:	Modified to use Taus88 instead of TT800 as default generator.
						Also update to use new ULong2Unitxx() functions.
		14-Apr-2001:	General overhaul for LPP
		2-Apr-2001:		First implementation.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "LitterLib.h"
#include "TrialPeriodUtils.h"
#include "Taus88.h"
#include "MiscUtils.h"

#include <math.h>


#pragma mark • Constants

const char	kClassName[]		= "lp.linnie";			// Class name


	// Indices for STR# resource
enum {
	strIndexInBang		= lpStrIndexLastStandard + 1,
	strIndexTheOutlet,
	
	strIndexInLeft		= strIndexInBang,
	strIndexOutLeft		= strIndexTheOutlet
	};


#pragma mark • Object Structure

typedef struct {
	LITTER_CORE_OBJECT(Object, coreObject);
	
	tTaus88DataPtr	tausData;
	
	double			a, b, c,			// (User) coordinates of start, end, apex
					scale,				// b - a
					scaleLeft,			// c - a
					scaleRight,			// b - c
					cc;					// Apex normalized inside unit range: (c-a)/(b-a)
	UInt32			lrThresh;
		
	} objLinran;


#pragma mark • Global Variables

SymbolPtr	gSymSymbol	= NIL,
			gNegSymbol	= NIL,
			gPosSymbol	= NIL,
			gNullSymbol	= NIL;



#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	LinnieBang(me)
 *	LinnieFloat(me, float)
 *	
 *	There are two methods in the literature for generating a linear distribution: 
 *	
 *		1. Take square root of a uniformly distributed random number
 *		2. Take the smaller/larger/mean of two independent uniformly distributed random
 *			numbers
 *	
 *	The second method is used here on the assumption that it is computationally cheaper.
 *	Taus88Float is basically some bit fiddling and one floating point multiply; two of those
 *	will be cheaper that a call to sqrt(), and Taus88's cycle is long enough that we can
 *	afford two calls. This method is also easier to extend to the symmetrical variant.
 *	
 *	The mapping function provided by LinnieFloat() uses the first variant. 
 *
 ******************************************************************************************/

static void
LinnieBang(
	objLinran* me)
	
	{
	tTaus88DataPtr	tausData = me->tausData;
	UInt32			lrThresh = me->lrThresh,
					s1, s2, s3;
	double			lin, u1, u2;
	
	if (tausData == NIL)
		tausData = Taus88GetGlobals();
	Taus88Load(tausData, &s1, &s2, &s3);
	
	// We need two unit uniform random variates no matter what
	u1 = ULong2Unit_ZO( Taus88Process(&s1, &s2, &s3) ),
	u2 = ULong2Unit_ZO( Taus88Process(&s1, &s2, &s3) );
	
	if (lrThresh == 0x80000000UL) {
		// Special case for isoceles coordinates
		lin  = u1 + u2;
		lin *= me->scaleLeft;
		lin += me->a;
		}
	else {
		// If the core RNG were expensive, we could consider getting by with the two
		// uniform deviates already calculated. Taken together, we have 64 random bits,
		// and we basically need three single-precision float values, so there are enough
		// random bits to go around for "pretty good" randomness. But inline Taus88 is so
		// fast, let's just grab another variate.
		// We don't even have to convert this one to float.
		if (Taus88Process(&s1, &s2, &s3) <= lrThresh) {
			lin  = (u1 > u2) ? u1 : u2;
			lin *= me->scaleLeft;
			lin += me->a;
			}
		else {
			lin = (u1 < u2) ? u1 : u2;
			lin *= me->scaleRight;
			lin += me->c;
			}
		}
	
	Taus88Store(tausData, s1, s2, s3);
	
	outlet_float(me->coreObject.o_outlet, lin);
	}

static void
LinnieFloat(
	objLinran*	me,
	double		iVal)
	
	{
	
	if (0.0 <= iVal && iVal <= 1.0) {
		double	a = me->a,
				b = me->b,
				c = me->c;
		
		if (a == c) {
			// Simple negative slope case
			iVal  = 1.0 - sqrt(1.0 - iVal);
			iVal *= me->scaleRight;
			iVal += a;
			}
		else if (b == c) {
			// Simple positive slope case
			iVal = sqrt(iVal);
			iVal *= me->scaleLeft;
			iVal += a;
			}
		else {
			// Which part of the triangle are we in?
			double cc = me->cc;
			
			if (iVal <= cc) {
				// Left half
				iVal  = sqrt(iVal/cc);
				iVal *= me->scaleLeft;
				iVal += a;
				}
			else {
				// Right half
				iVal  = 1.0 - sqrt((1.0-iVal) / (1.0-cc));
				iVal *= me->scaleRight;
				iVal += c;
				
				}
			}
		
		outlet_float(me->coreObject.o_outlet, iVal);
		}
	
	}


/******************************************************************************************
 *
 *	void LinnieSym(me)
 *	void LinniePos(me)
 *	void LinnieNeg(me)
 *	
 *	Set which variant to use. Nothing bad can happen. 
 *	
 ******************************************************************************************/

static void
LinnieList(
	objLinran*	me,
	Symbol*		iListSym,
	short		iArgCount,
	Atom		iArgVec[])
	
	{
	#pragma unused (iListSym)
	
	Boolean goodParams = false;
	
	if ( iArgCount == 3
			&& AtomIsNumeric(iArgVec)
			&& AtomIsNumeric(iArgVec + 1)
			&& AtomIsNumeric(iArgVec + 2) ) {
		double	a = AtomGetFloat(iArgVec),
				c = AtomGetFloat(iArgVec + 1),
				b = AtomGetFloat(iArgVec + 2);
		
		if (a <= c && c <= b) {
			double	ca = c - a,
					bc = b - c;
			
			goodParams = true;
			
			me->a			= a;
			me->b			= b;
			me->c			= c;
			me->scaleLeft	= ca;
			me->scaleRight	= bc;
			me->lrThresh	= (ca / (b - a)) * ((double) kULongMax) + 0.5;
			// ASSERT: (ca == bc) <==> (lrThresh == 0x80000000UL)
			}
		}
	
	if (!goodParams)
		error("%s: list values invalid", kClassName);
	
	}

static void
LinnieSym(
	objLinran* me)
	
	{
	me->c = 0.5 * (me->a + me->b);
	me->scaleLeft = me->scaleRight = 0.5 * (me->b - me->a);
	me->lrThresh = 0x80000000UL;
	me->cc = 0.5;
	}
	
static void
LinniePos(
	objLinran* me)
	
	{
	me->c = me->b;
	me->scaleLeft = me->b - me->a;
	me->scaleRight = 0.0;
	me->lrThresh = kULongMax;
	me->cc = 1.0;
	}

static void
LinnieNeg(
	objLinran* me)
	
	{
	me->c = me->a;
	me->scaleLeft = 0.0;
	me->scaleRight = me->b - me->a;
	me->lrThresh = 0;
	me->cc = 0.0;
	}
	
static void LinnieSeed(objLinran* me, long iSeed)
	{ Taus88Seed(me->tausData, (unsigned long) iSeed); }
#pragma mark -
#pragma mark • Attribute/Information Functions

/******************************************************************************************
 *
 *	LinnieAssist(me, iBox, iDir, iArgNum, oCStr)
 *	LinnieTattle(me)
 *
 ******************************************************************************************/

/******************************************************************************************
 *
 *	LinnieAssist
 *	LinnieTattle(me)
 *
 ******************************************************************************************/

static void LinnieAssist(objLinran* me, void* box, long iDir, long iArgNum, char* oCStr)
	{
	#pragma unused (me, box)
	
	 LitterAssist(iDir, iArgNum, strIndexInLeft, strIndexOutLeft, oCStr);
	 }

static void
LinnieTattle(
	objLinran* me)
	
	{
	
	post("%s state", kClassName);
	post("  Parameters: %lf, %lf, %lf", me->a, me->c, me->b);
	post("  Scale: %lf (left %lf, right %lf)", me->scale, me->scaleLeft, me->scaleRight);
	post("  Normalized apex: %lf (UInt32 threshhold: 0x%.8lx)", me->cc, me->lrThresh);
	
	}


/******************************************************************************************
 *
 *	DoExpect()
 *
 *	Helper function for calculating expected values (mean, standard deviation, etc.)
 *	
 ******************************************************************************************/

static double DoExpect(objLinran* me, eExpectSelector iSel)
	{
	const double kSqrt2	= 1.41421356237309504880168872420969808;
	
	double	a		= me->a,
			b		= me->b,
			c		= me->c,
			result	= 0.0 / 0.0;					// Initially undefined
	
	switch(iSel) {
	case expMean:
		result = (a + b + c) / 3.0;
		break;
		
	case expMedian:
		result = 0.5 * (b - a);
		if (c >= result) {
			result *= c - a;
			result  = sqrt(result);
			result += a;
			}
		else {
			result *= b - c;
			result  = b - sqrt(result);
			}
		break;
		
	case expMode:
		result = c;
		break;
		
	case expVar:
	case expStdDev:
		result  = a*a + b*b + c*c;
		result -= a*b + a*c + b*c;
		result *= 1.0 / 18.0;
		if (iSel == expStdDev)
			result = sqrt(result);
		break;
	
	case expSkew:
		result  = 18.0 * DoExpect(me, expVar);
		result  = 5.0 * pow(result, 1.5);
		result  = 1.0 / result;
		result *= kSqrt2;
		result *= a + b - c - c;
		result *= a + a - b - c;
		result *= a - b - b + c;
		break;
	case expKurtosis:
		result = 2.4;
		break;
	case expMin:
		result = a;
		break;
	case expMax:
		result = b;
		break;
	case expEntropy:
		result = 0.5 + log2(0.5 * (b - a));
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

	static t_max_err LinnieGetMean(objLinran* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMean), ioArgC, ioArgV); }
	static t_max_err LinnieGetMedian(objLinran* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMedian), ioArgC, ioArgV); }
	static t_max_err LinnieGetMode(objLinran* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMode), ioArgC, ioArgV); }
	static t_max_err LinnieGetVar(objLinran* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expVar), ioArgC, ioArgV); }
	static t_max_err LinnieGetStdDev(objLinran* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expStdDev), ioArgC, ioArgV); }
	static t_max_err LinnieGetSkew(objLinran* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expSkew), ioArgC, ioArgV); }
	static t_max_err LinnieGetKurtosis(objLinran* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expKurtosis), ioArgC, ioArgV); }
	static t_max_err LinnieGetEntropy(objLinran* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expEntropy), ioArgC, ioArgV); }
	
	static t_max_err LinnieSetAttrMin(objLinran* me, void* iAttr, long iArgC, Atom iArgV[])
		{
		
		if (iArgC > 0 && iArgV != NIL) {
			Atom	params[3];
			double	min = AtomGetFloat(iArgV);
			
			if (min > me->c) min = me->c;
			
			AtomSetFloat(&params[0], min);
			AtomSetFloat(&params[1], me->c);
			AtomSetFloat(&params[2], me->b);
			
			LinnieList(me, NIL, 3, params);
			}
		
		return MAX_ERR_NONE;
		}
	
	static t_max_err LinnieSetAttrMax(objLinran* me, void* iAttr, long iArgC, Atom iArgV[])
		{
		
		if (iArgC > 0 && iArgV != NIL) {
			Atom	params[3];
			double	max = AtomGetFloat(iArgV);
			
			if (max < me->c) max = me->c;
			
			AtomSetFloat(&params[0], me->a);
			AtomSetFloat(&params[1], me->c);
			AtomSetFloat(&params[2], max);
			
			LinnieList(me, NIL, 3, params);
			}
		
		return MAX_ERR_NONE;
		}
	
	static t_max_err LinnieSetAttrMode(objLinran* me, void* iAttr, long iArgC, Atom iArgV[])
		{
		
		if (iArgC > 0 && iArgV != NIL) {
			Atom	params[3];
			double	mode = AtomGetFloat(iArgV);
			
			if		(mode < me->a) mode = me->a;
			else if (mode > me->b) mode = me->b;
			
			AtomSetFloat(&params[0], me->a);
			AtomSetFloat(&params[1], mode);
			AtomSetFloat(&params[2], me->b);
			
			LinnieList(me, NIL, 3, params);
			}
		
		return MAX_ERR_NONE;
		}
	
	static inline void
	AddInfo(void)
		{
		Object*	attr;
		Symbol*	symFloat64		= gensym("float64");
		
		// Read-Write Attributes
		attr = attr_offset_new(	"min", symFloat64, 0,
								NULL, (method) LinnieSetAttrMin,
								calcoffset(objLinran, a));
		class_addattr(gObjectClass, attr);
		attr = attr_offset_new(	"max", symFloat64, 0,
								NULL, (method) LinnieSetAttrMax,
								calcoffset(objLinran, b));
		class_addattr(gObjectClass, attr);
		attr = attr_offset_new(	"mode", symFloat64, 0,
								NULL, (method) LinnieSetAttrMode,
								calcoffset(objLinran, c));
		class_addattr(gObjectClass, attr);
		
		// Read-Only Attributes
		attr = attribute_new("mean", symFloat64, kAttrFlagsReadOnly, (method) LinnieGetMean, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("median", symFloat64, kAttrFlagsReadOnly, (method) LinnieGetMedian, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("var", symFloat64, kAttrFlagsReadOnly, (method) LinnieGetVar, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("stddev", symFloat64, kAttrFlagsReadOnly, (method) LinnieGetStdDev, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("skew", symFloat64, kAttrFlagsReadOnly, (method) LinnieGetSkew, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("kurtosis", symFloat64, kAttrFlagsReadOnly, (method) LinnieGetKurtosis, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("entropy", symFloat64, kAttrFlagsReadOnly, (method) LinnieGetEntropy, NULL);
		class_addattr(gObjectClass, attr);
		}

static void
LinnieTell(
	objLinran*	me,
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

static void LinnieInfo(objLinran* me)
	{ LitterInfo(kClassName, &me->coreObject, (method) LinnieTattle); }

static inline void AddInfo(void)
	{ LitterAddCant((method) LinnieInfo, "info"); }
		
static void LinnieTell(objLinran* me, Symbol* iTarget, Symbol* iMsg)
	{ LitterExpect((tExpectFunc) DoExpect, (Object*) me, iMsg, iTarget, TRUE); }


#endif


#pragma mark -
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	LinnieFree(me)
 *	LinnieNew(iOracle)
 *	
 ******************************************************************************************/

static void LinnieFree(objLinran* me)
	{ Taus88Free(me->tausData); }							// Taus88Free is NIL-safe

static void*
LinnieNew(
	Symbol*	iSymmetry,
	long	iSeed)
	
	{
	objLinran*		me			= NIL;
	tTaus88DataPtr	myTausStuff	= NIL;
	
	// Run through initialization parameters from right to left, handling defaults
	if (iSeed != 0) {
		myTausStuff = Taus88New(iSeed);
		goto noMoreDefaults;
		}
	
	if (iSymmetry == gNullSymbol)
		iSymmetry = gNegSymbol;
noMoreDefaults:
	// Finished checking intialization parameters

	// Let Max allocate us and our outlet. Only the default inlet.
	me = (objLinran*) LitterAllocateObject();
	
	floatout(me);								// Access through me->coreObject.o_outlet
	
	// Store object components
	me->tausData	= myTausStuff;
	me->a			= 0.0;
	me->b			= 1.0;
	me->c			= 0.5;
	me->scaleLeft	= 0.5;
	me->scaleRight 	= 0.5;
	me->lrThresh	= 0x80000000UL;

	return me;
	}




#pragma mark -

/******************************************************************************************
 *
 *	main()
 *	
 *	Standard Max External Object Entry Point Function
 *	
 ******************************************************************************************/

void
main(void)
	
	{
	const tTypeList myArgTypes = {
						A_DEFSYM,			// Optional arguments:	1. sym, pos, or neg
						A_DEFLONG,			// 						2. seed
						A_NOTHING
						};
	
	
	// Standard Max setup() call
	LitterSetupClass(	kClassName,
						sizeof(objLinran),
						LitterCalcOffset(objLinran),	// Class object size
						(method) LinnieNew,				// Instance creation function
						(method) LinnieFree,			// Custom deallocation function
						NIL,							// No menu function
						myArgTypes);				
	

	// Messages
	LITTER_TIMEBOMB LitterAddBang((method) LinnieBang);
	LITTER_TIMEBOMB addfloat((method) LinnieFloat);
	LITTER_TIMEBOMB LitterAddMess1	((method) LinnieList, "list", A_GIMME);
	LitterAddMess1 ((method) LinnieSeed,		"seed",		A_DEFLONG);
	LitterAddMess0	((method) LinniePos,		"pos");
	LitterAddMess0	((method) LinnieSym,		"sym");
	LitterAddMess0	((method) LinnieNeg,		"neg");
	LitterAddMess2 ((method) LinnieTell,		"tell", A_SYM, A_SYM);
	LitterAddCant	((method) LinnieTattle,		"dblclick");
	LitterAddCant	((method) LinnieAssist,		"assist");
	
	AddInfo();
	
	// Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();
	
	// Stash pointers to commonly used symbols
	gSymSymbol	= gensym("sym");
	gPosSymbol	= gensym("pos");
	gNegSymbol	= gensym("neg");
	gNullSymbol	= gensym("");
	
	}


