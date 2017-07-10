/*
	File:		gammer.c

	Contains:	Max external object generating a Guassian distribution.

	Written by:	Peter Castine

	Copyright:	© 2001-2002 Peter Castine

	Change History (most recent first):

         <9>   30–3–2006    pc      Update for new LitterLib organization
         <8>     18–2–06    pc      Add support for expect message.
         <7>     10–1–06    pc      Updated to use LitterAssistResFrag()
         <6>     14–1–04    pc      Seems to be a bug in the algorithm...
         <5>     11–1–04    pc      Update for Windows.
         <4>    7–7–2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <3>  30–12–2002    pc      Drop faux 'Vers' resource and related modifications. Add object
                                    version to GammerInfo().
         <2>  28–11–2002    pc      Tidy up after initial check-in.
         <1>  28–11–2002    pc      Initial check in.
		30-Jun-2001:	Modified to use Taus88 instead of TT800 as default generator.
						Also update to use new ULong2Unitxx() functions.
		21-May-2001:	Added more efficient method for large values of the alpha
						parameter. Renamed object to gammer in recognition of the fact
						that the Gamma distribution is a special case of the Gamma
						distribution with integer values for the alpha parameter. 
		26-May-2001:	Add support for non-integral alpha (i.e., we've now got the full
						Gamma distribution and not just the Erlang distribution). 
		14-Apr-2001:	Overhaul for LPP
		2-Apr-2001:		First implementation (as gammy.c).
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "LitterLib.h"
#include "TrialPeriodUtils.h"
#include "Taus88.h"
#include "RNGGauss.h"
#include "RNGGamma.h"
#include "MiscUtils.h"
#include "MoreMath.h"


#pragma mark • Constants

const char	kClassName[]		= "lp.gammer";			// Class name


	// Indices for STR# resource
enum {
	strIndexInBang		= lpStrIndexLastStandard + 1,
	strIndexInOrder,
	strIndexInLocation,
	
	strIndexTheOutlet,
	
	strIndexFragErlang,
	strIndexFragGamma,
	
	strIndexLeftInlet	= strIndexInBang,
	strIndexLeftOutlet	= strIndexTheOutlet
	};

#pragma mark • Type Definitions

	// These could have been defined in RNGGamma.h
	// But currently they aren't
	// It may, in the course of time, prove better to move them there
typedef struct {
			long	alpha;
			double	beta;
			} tErlDirParams;

typedef struct {
			long	alpha;
			double	beta,
					gamma;
			} tErlRejParams;

typedef struct {
			double	alpha,
					beta,
					gamma;
			} tGammaGSParams;

	// This is may be better off here in the lp.gammer object code
typedef union {
		tErlDirParams	ed;
		tErlRejParams	er;
		tGammaGSParams	gs;
		tGammaGDParams	gd;
		} uGammerParams;

		
#pragma mark • Object Structure

typedef struct {
	LITTER_CORE_OBJECT(Object, coreObject);
	
	tTaus88DataPtr	theData;
	
	eGammaAlg		alg;
	uGammerParams	params;
	} objGammer;




#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark • Utility Functions

static double
GammerGetAlpha(
	objGammer* me)
	
	{
	double alpha;
	
	switch(me->alg) {
		case algUndef:		// fall into next case...
		case algErlDir:	alpha = me->params.ed.alpha;	break;
		case algErlRej:	alpha = me->params.er.alpha;	break;
		case algGS:		alpha = me->params.gs.alpha;	break;
		case algGD:		alpha = me->params.gd.alpha;	break;
		default:		alpha = 0.0;					break;	/* This can't happen */
		}
	
	return alpha;
	}

static double
GammerGetBeta(
	objGammer* me)
	
	{
	double beta;
	
	switch(me->alg) {
		case algUndef:		// fall into next case...
		case algErlDir:	beta = me->params.ed.beta;		break;
		case algErlRej:	beta = me->params.er.beta;		break;
		case algGS:		beta = me->params.gs.beta;		break;
		case algGD:		beta = me->params.gd.beta;		break;
		default:		beta = 0.0;						break;	/* This can't happen */
		}
	
	return beta;
	}



#pragma mark -
#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	GammerBang(me)
 *
 ******************************************************************************************/		

static void
GammerBang(
	objGammer* me)
	
	{
	double	g;
	
	switch (me->alg) {
	case algErlDir:
		g = GerErlangDirTaus88(	me->params.ed.alpha,
								me->params.ed.beta,
								me->theData);
		break;
	
	case algErlRej:
		g = GenErlangRejTaus88(	me->params.er.alpha,
								me->params.er.beta,
								me->params.er.gamma,
								me->theData);
		break;
	
	case algGS:
		g = GenGammaGSTaus88(	me->params.gs.alpha,
								me->params.gs.beta,
								me->params.gs.gamma,
								me->theData);
		break;
	
	case algGD:
		g = GenGammaGDTaus88(&me->params.gd, me->theData);
		break;
	
	default:
		// Must be algUndef. Degenerate case
		g = 0.0;
		break;
		}
		
	outlet_float(me->coreObject.o_outlet, g);
	}


/******************************************************************************************
 *
 *	GammerOrder(me, iOrder)
 *	GammerLoc(me, iLoc)
 *	
 *	Set parameters, making sure nothing bad happens.
 *	
 ******************************************************************************************/

static void
GammerOrder(
	objGammer*	me,
	double		iOrder)
	
	{
	// Grab current beta, in case the new order/alpha parameter causes us to change
	// algorithm and, with it, the parameter structure that we use
	double	beta = GammerGetBeta(me);
	
	switch ( me->alg = RecommendGammaAlg(iOrder) ) {
	case algUndef:
	case algErlDir:
		me->params.ed.alpha = iOrder,
		me->params.ed.beta	= beta;
		break;
	
	case algErlRej:
		me->params.er.alpha = iOrder,
		me->params.er.beta	= beta;
		me->params.er.gamma	= CalcErlangRejGamma(me->params.er.alpha);
				// The above assumes that it's cheaper to read from memory than
				// to perform double-to-int conversion twice. I might be wrong.
post("GammerOrder setting gamma to %lf", CalcErlangRejGamma(me->params.er.alpha));
		break;
	
	case algGS:
		me->params.gs.alpha = iOrder,
		me->params.gs.beta	= beta;
		me->params.gs.gamma	= CalcGSGamma(iOrder);
		break;
	
	case algGD:
		CalcGDParams(&me->params.gd, iOrder, beta);
		break;
	default:
		// This can't happen
		break;
		}
	
	}

static void GammerBeta(objGammer* me, double iBeta)
	{
	
	switch(me->alg) {
		case algUndef:		// fall into next case...
		case algErlDir:	me->params.ed.beta = iBeta;		break;
		case algErlRej:	me->params.er.beta = iBeta;		break;
		case algGS:		me->params.gs.beta = iBeta;		break;
		case algGD:		me->params.gd.beta = iBeta;		break;
		default:			/* no other cases */		break;
		}
	
	}

static void GammerLoc(objGammer* me, double iLoc)
	{
	if (iLoc != 0.0)
		GammerBeta(me, 1.0 / iLoc);
	
	else error("%s: location parameter must be non-zero", kClassName);
	}
	

/******************************************************************************************
 *
 *	GammerSeed(me, iSeed)
 *
 ******************************************************************************************/

static void GammerSeed(objGammer* me,long iSeed)
	{ Taus88Seed(me->theData, (unsigned long) iSeed); }


#pragma mark -
#pragma mark • Attribute/Information Functions

/******************************************************************************************
 *
 *	GammerAssist
 *	GammerTattle(me)
 *
 ******************************************************************************************/

static void
GammerAssist(
	objGammer*	me,
	void*		box,
	long		iDir,
	long		iArgNum,
	char*		oCStr)
	
	{
	#pragma unused(box)
	
	if (iDir == ASSIST_INLET)
		LitterAssist(iDir, iArgNum, strIndexLeftInlet, 0, oCStr);
	else {
		short fragIndex = (me->alg == algErlDir || me->alg == algErlRej)
									? strIndexFragErlang
									: strIndexFragGamma;
		
		LitterAssistResFrag(iDir, iArgNum, 0, strIndexLeftOutlet, oCStr, fragIndex);
		}	
	}

static void
GammerTattle(
	objGammer* me)
	
	{
	
	post("%s state", kClassName);
	
	switch (me->alg) {
	case algErlDir:
		post("  Erlang distribution for small integral values of alpha (order)");
		post("  alpha = %ld", me->params.ed.alpha);
		post("  beta = %lf", me->params.ed.beta);
		break;
	
	case algErlRej:
		post("  Erlang distribution for large integral values of alpha (order)");
		post("  alpha = %ld", me->params.er.alpha);
		post("  beta = %lf", me->params.er.beta);
		post("  gamma = %lf", me->params.er.gamma);
		break;
	
	case algGS:
		post("  Gamma distribution; using GS algorithm for small values of alpha");
		post("  alpha = %lg", me->params.gs.alpha);
		post("  beta = %lf", me->params.gs.beta);
		post("  gamma = %lf", me->params.gs.gamma);
		break;
	
	case algGD:
		post("  Gamma distribution; using GD algorithm for large values of alpha");
		post("  alpha  = %lf", me->params.gd.alpha);
		post("  beta   = %lf", me->params.gd.beta);
		post("  gamma  = %lf", me->params.gd.gamma);
		post("  sigma  = %lf", me->params.gd.sigma);
		post("  sigma2 = %lf", me->params.gd.sigma2);
		post("  delta  = %lf", me->params.gd.delta);
		post("  rho    = %lf", me->params.gd.rho);
		post("  si     = %lf", me->params.gd.si);
		post("  q0     = %lf", me->params.gd.q0);
		post("  c      = %lf", me->params.gd.c);
		break;
	
	default:
		// Must be algNone (i.e., bad alpha parameter)
		post("   Invalid data entered for alpha; degenerate distribution");
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

static double
DoExpect(
	objGammer*		me,
	eExpectSelector iSel)
	
	{
	double	result	= 0.0 / 0.0,					// Initially undefined
			alpha	= GammerGetAlpha(me),
			beta	= GammerGetBeta(me);
			
	switch(iSel) {
	case expMean:
		result = alpha * beta;
		break;
	case expMode:
		result = (alpha >= 1.0) ? (alpha - 1.0) * beta : 0.0;
		break;
	case expVar:
		result = alpha * beta * beta;
		break;
	case expStdDev:
		result = sqrt(alpha) * fabs(beta);
		break;
	case expSkew:
		result  = 1.0 / sqrt(alpha);
		result += result;
		break;
	case expKurtosis:
		result = 6.0 / alpha;
		break;
	case expEntropy:
		if (alpha > 0.0) {
			if (beta < 0.0) beta = - beta;				// Take |beta|
			
			result  = alpha * beta;
			result += (alpha - 1.0) * log(beta);
			result += lgamma(alpha);
			result += (1.0 - alpha) * digamma(alpha);
			}
		 else result = -1.0 / 0.0;						// -inf
		break;
	default:
		// Only thing left is median, which is undefined
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

	static t_max_err GammerGetMin(objGammer* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMin), ioArgC, ioArgV); }
	static t_max_err GammerGetMax(objGammer* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMax), ioArgC, ioArgV); }
	static t_max_err GammerGetMean(objGammer* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMean), ioArgC, ioArgV); }
	static t_max_err GammerGetMode(objGammer* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMode), ioArgC, ioArgV); }
	static t_max_err GammerGetVar(objGammer* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expVar), ioArgC, ioArgV); }
	static t_max_err GammerGetStdDev(objGammer* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expStdDev), ioArgC, ioArgV); }
	static t_max_err GammerGetSkew(objGammer* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expSkew), ioArgC, ioArgV); }
	static t_max_err GammerGetKurtosis(objGammer* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expKurtosis), ioArgC, ioArgV); }
	static t_max_err GammerGetEntropy(objGammer* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expEntropy), ioArgC, ioArgV); }
	
	static t_max_err GammerSetAttrOrder(objGammer* me, void* iAttr, long iArgC, Atom iArgV[])
		{
		
		if (iArgC > 0 && iArgV != NIL)
			GammerOrder(me, AtomGetFloat(iArgV));
		
		return MAX_ERR_NONE;
		}
	
	static t_max_err GammerGetAttrOrder(objGammer* me, void* iAttr, long* ioArgC, Atom** iArgV)
		{ return LitterGetAttrFloat(GammerGetAlpha(me), ioArgC, iArgV); }
	
	static t_max_err GammerSetAttrLoc(objGammer* me, void* iAttr, long iArgC, Atom iArgV[])
		{
		
		if (iArgC > 0 && iArgV != NIL)
			GammerLoc(me, AtomGetFloat(iArgV));
		
		return MAX_ERR_NONE;
		}
	
	static t_max_err GammerGetAttrLoc(objGammer* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(GammerGetBeta(me), ioArgC, ioArgV); }
	
	static inline void
	AddInfo(void)
		{
		Object*	attr;
		Symbol*	symFloat64		= gensym("float64");
		
		// Read-Write Attributes
		attr = attr_offset_new(	"order", symFloat64, 0,
								(method) GammerGetAttrOrder, (method) GammerSetAttrOrder, 0);
		class_addattr(gObjectClass, attr);
		attr = attr_offset_new(	"beta", symFloat64, 0,
								(method) GammerGetAttrLoc, (method) GammerSetAttrLoc, 0);
		class_addattr(gObjectClass, attr);
		
		// Read-Only Attributes
		attr = attribute_new("min", symFloat64, kAttrFlagsReadOnly, (method) GammerGetMin, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("max", symFloat64, kAttrFlagsReadOnly, (method) GammerGetMax, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("mean", symFloat64, kAttrFlagsReadOnly, (method) GammerGetMean, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("mode", symFloat64, kAttrFlagsReadOnly, (method) GammerGetMode, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("var", symFloat64, kAttrFlagsReadOnly, (method) GammerGetVar, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("stddev", symFloat64, kAttrFlagsReadOnly, (method) GammerGetStdDev, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("skew", symFloat64, kAttrFlagsReadOnly, (method) GammerGetSkew, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("kurtosis", symFloat64, kAttrFlagsReadOnly, (method) GammerGetKurtosis, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("entropy", symFloat64, kAttrFlagsReadOnly, (method) GammerGetEntropy, NULL);
		class_addattr(gObjectClass, attr);
		}

static void
GammerTell(
	objGammer*	me,
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

static void GammerInfo(objGammer* me)
	{ LitterInfo(kClassName, &me->coreObject, (method) GammerTattle); }

static inline void AddInfo(void)
	{ LitterAddCant((method) GammerInfo, "info"); }
		
void GammerTell(objGammer* me, Symbol* iTarget, Symbol* iAttrName)
	{ LitterExpect((tExpectFunc) DoExpect, (Object*) me, iAttrName, iTarget, TRUE); }

#endif


#pragma mark -
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	GammerNew(iOracle)
 *
 ******************************************************************************************/

static void
GammerFree(objGammer* me)
	{ Taus88Free(me->theData); }						// Taus88Free() is NIL-pointer safe

static void*
GammerNew(
	double	iOrder,
	double	iLoc,
	long	iSeed)
	
	{
	const double	kDefOrder	= 1.0;
	const double	kDefLoc		= 1.0;
	
	objGammer*		me			= NIL;
	tTaus88DataPtr	myTausStuff	= NIL;
	
	// Run through initialization parameters from right to left, handling defaults
	if (iSeed != 0) {
		myTausStuff = Taus88New(iSeed);
		// Even if seed is specified, continue checking parameters because a location
		// of zero would be invalid
		}
	
	if (iLoc == 0.0)
		iLoc = kDefLoc;
		// Even if location is specified, continue checking parameters because a
		// non-positive order would be invalid
	
	if (iOrder <= 0.0)
		iOrder = kDefOrder;
	// Finished checking intialization parameters
	// ----------------------------------------------------
	
	// Let Max allocate us, our inlets (from right to left), and outlets
	me = (objGammer*) LitterAllocateObject();
	
	floatin(me, 2);
	floatin(me, 1);
	
	floatout(me);								// Access through me->coreObject.o_outlet
	
	// Initialize object components
	me->theData = myTausStuff;
	GammerOrder(me, iOrder);
	GammerLoc(me, iLoc);
	
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
						A_DEFFLOAT,					// Optional arguments:	1: Order
						A_DEFFLOAT,					//						2: Location
						A_DEFLONG,					// 						3: seed
						A_NOTHING
						};
	
	LITTER_CHECKTIMEOUT(kClassName);
	
	LitterSetupClass(	kClassName,
						sizeof(objGammer),
						LitterCalcOffset(objGammer),	// Class object size
						(method) GammerNew,				// Instance creation function
						(method) GammerFree,			// Custom deallocation function
						NIL,							// No menu function
						myArgTypes);				
	
	// Messages
	LITTER_TIMEBOMB LitterAddBang((method) GammerBang);
	LitterAddMess1	((method) GammerOrder,	"ft1",	A_FLOAT);
	LitterAddMess1	((method) GammerBeta,	"ft2",	A_FLOAT);
	LitterAddMess1	((method) GammerLoc,	"loc",	A_FLOAT);
	LitterAddMess1	((method) GammerSeed,	"seed",	A_DEFLONG);
	LitterAddMess2	((method) GammerTell,	"tell", A_SYM, A_SYM);
	LitterAddCant	((method) GammerTattle,	"dblclick");
	LitterAddCant	((method) GammerAssist,	"assist");
	
	AddInfo();

	// Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();
	
	}

