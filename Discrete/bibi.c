/*
	File:		bibi.c

	Contains:	Max external object generating a random values with Beta Binomial distribution.

	Written by:	Peter Castine

	Copyright:	© 2006 Peter Castine

	Change History (most recent first):

		15-Dec-2006:		First implementation.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "LitterLib.h"
#include "TrialPeriodUtils.h"
#include "Taus88.h"
#include "RNGBinomial.h"
#include "RNGDistBeta.h"

#pragma mark • Constants

const char*		kClassName		= "lp.bibi";		// Class name


	// Indices for STR# resource
enum {
	strIndexInBang		= lpStrIndexLastStandard + 1,
	strIndexInNTrials,
	strIndexInA,
	strIndexInB,
	
	strIndexOut,
	
	strIndexInLeft		= strIndexInBang,
	strIndexOutLeft		= strIndexOut
	};

enum binomGen {							// How to generate deviates
	binalgAlwaysZero,							// n = 0 or p = 0
	binalgAlwaysN,								// p = 1
	binalgFiftyFifty,							// p = .5, n < ~768
	binalgBruteForce,							// n < ~ 15
	binalgBInversion,							// np < 20
	binalgBTPErej								// General-purpose
	};




#pragma mark • Type Definitions

typedef enum binomGen eBinomAlg;

typedef union betaParams {
			tBCParams	bc;						// Put bc first so we can initialize the 
			tBBParams	bb;						// entire union to zeros
			tJKParams	jk;
			} uBetaParams;


#pragma mark • Object Structure

typedef struct {
	LITTER_CORE_OBJECT(Object, coreObject);
	
	tTaus88DataPtr	tausData;
	
	UInt32			nTrials;					// Binomial parameter
	double			alpha,							// Beta parameters
					beta;
	eBetaAlg		betaAlg;
	uBetaParams		betaParams;
	} objBibi;



#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	BibiBang(me)
 *
 ******************************************************************************************/

	typedef union {
		tBTPEParams	btpeStuff;
		tBINVParams	binvStuff;
		UInt32		bfThresh;
		} uBinomParams;
	
	static eBinomAlg SelectBinomAlg(long iNTrials, double iProb, uBinomParams* oParams)
		{
		const uBinomParams kDefParams = {{	false,
											0.0, 0.0, 0.0, 0.0, 0.0,
											0.0, 0.0, 0.0, 0.0, 0.0,
											0.0, 0.0, 0.0, 0.0, 0.0
											}};
		
		eBinomAlg result;
		
		*oParams = kDefParams;					
		
		if (iNTrials == 0 || iProb == 0.0)
			result = binalgAlwaysZero;
		
		else if (iProb == 1.0)
			result = binalgAlwaysN;
			
		else if (iProb == 0.5 && iNTrials <= 768)
			result	= binalgFiftyFifty;
		
		else if (iNTrials <= 15) {
			result = binalgBruteForce;
			oParams->bfThresh = CalcBFThreshhold(iProb);
			}
			
		else {
			// It'sgoing to be either Inversion or BTPE
			// For either we reflect distributions with iProb > 0.5
			const double effectiveP = (iProb > 0.5) ? 1.0 - iProb : iProb;
			
			if (effectiveP * iNTrials < 16) {
				result = binalgBInversion;
				CalcBINVParams((tBINVParams*) oParams, iNTrials, iProb);
				}
			
			else {
				result = binalgBTPErej;
				CalcBTPEParams((tBTPEParams*) oParams, iNTrials, iProb);
				}
			}
		
		return result;
		}
	
	static eBetaAlg SelectBetaAlg(double iAlpha, double iBeta, uBetaParams* oParams)
		{
		eBetaAlg result = RecommendBetaAlg(iAlpha, iBeta);
		
		switch (result) {
		case algJoehnk:
			CalcJKParams((tJKParams*) oParams, iAlpha, iBeta);
			break;
		case algChengBB:
			CalcBBParams((tBBParams*) oParams, iAlpha, iBeta);
			break;
		case algChengBC:
			CalcBCParams((tBCParams*) oParams, iAlpha, iBeta);
			break;
		default:
			// To be squeaky clean we should nuke oParams.
			// But we're not. Yet.
			break;
			}
		
		return result;
		}

static void
BibiBang(
	objBibi* me)
	
	{
	long			b,
					n = me->nTrials;
	double			p;
	uBinomParams	binomParams;
	
	//
	//	First generate a beta-distributed value for p
	//
	
	if (me->betaAlg == algUndef)
		me->betaAlg = SelectBetaAlg(me->alpha, me->beta, &me->betaParams);
	
	// ASSERT: me->betaAlg != algUndef
	switch (me->betaAlg) {
		default:							// Must be algIndeterm
			p = 0.0;
			if (((long) Taus88(me->tausData)) < 0) break;
			// otherwise, fall into next case
		case algConstOne:
			p = 1.0;
			break;
		
		case algConstZero:
			p = 0.0;
			break;
		
		case algUniform:
			// a == b == 1: Uniform distribution
			p = ULong2Unit_ZO( Taus88(me->tausData) );
			break;
		
		case algArcSine:
			// a == b == 0.5: Arc Sine distribution
			p = GenArcsineTaus88(me->tausData);
			break;
		
		case algJoehnk:
			p = GenBetaJKTaus88(&me->betaParams.jk, me->tausData);
			break;
		
		case algChengBB:
			p = GenBetaBBTaus88(&me->betaParams.bb, me->tausData);
			break;
		
		case algChengBC:
			p = GenBetaBCTaus88(&me->betaParams.bc, me->tausData);
			break;
		}
	
	//
	//	Now use that to generate a (n, p)-distributed binomial variate
	//
	
	switch ( SelectBinomAlg(n, p, &binomParams) ) {
	default:											// Must be genAlwaysZero
		b = 0;
		break;
	
	case binalgAlwaysN:
		b = me->nTrials;
		break;
	
	case binalgFiftyFifty:
		b = GenDevBinomialFCTaus88(n, me->tausData);
		break;
	
	case binalgBruteForce:
		b = GenDevBinomialBFTaus88(n, binomParams.bfThresh, me->tausData);
		break;
	
	case binalgBInversion:
		b = GenBinomialBINV(&binomParams.binvStuff, (tRandomFunc) Taus88, (void*) me->tausData);
		break;
	
	case binalgBTPErej:
		b = GenBinomialBTPE(&binomParams.btpeStuff, (tRandomFunc) Taus88, (void*) me->tausData);
		break;
		}
	
	outlet_int(me->coreObject.o_outlet, b);
	}
	


/******************************************************************************************
 *
 *	BibiProb(me, iProb)
 *	BibiNTrials(me, iNTrials)
 *	BibiSeed(me, iSeed)
 *	
 *	Set parameters. Make sure nothing bad is happening.
 *	
 ******************************************************************************************/

static void BibiNTrials(objBibi* me, long iNTrials)
	{ me->nTrials = (iNTrials >= 0) ? iNTrials : 0; }

static void
BibiAlpha(
	objBibi*	me,
	double		iAlpha)
	
	{
	if (iAlpha < 0.0)
		iAlpha = 0.0;
	
	me->alpha	= iAlpha;
	me->betaAlg	= algUndef;
	}

static void
BibiBeta(
	objBibi*	me,
	double		iBeta)
	
	{
	if (iBeta < 0.0)
		iBeta = 0.0;
	
	me->beta	= iBeta;
	me->betaAlg	= algUndef;
	}
	
static void BibiSeed(objBibi* me, long iSeed)
	{ if (me->tausData) Taus88Seed(me->tausData, (UInt32) iSeed); }

#pragma mark -
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	BibiNew(iSym, iArgC, iArgV)
 *	BibiFree(me)
 *
 ******************************************************************************************/

static void*
BibiNew(
	Symbol*	iSym,
	short	iArgC,
	Atom	iArgV[])
	
	{
	#pragma unused(iSym)
	
	const long			kDefNTrials		= 1;
	const double		kDefAlpha		= 0.5,
						kDefBeta		= 0.5;
	const uBetaParams	kDefParams		= {{false,0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}};
										// In the default case the uBetaParams isn't used
										// So we just make sure it's nicely nuked
										
	objBibi* me = (objBibi*) LitterAllocateObject();
	
	// Allocate inlets and outlets	
	floatin(me, 3);											// Beta
	floatin(me, 2);											// Alpha
	intin(me, 1);											// Number of Trials
	
	intout(me);					// Access main outlet through me->coreObject.o_outlet;
	
	// Set up our own members to defaults
	me->tausData	= NIL;
	me->nTrials		= kDefNTrials;
	me->alpha		= kDefAlpha;
	me->beta		= kDefBeta;
	me->betaAlg		= algUndef;
	me->betaParams	= kDefParams;
	
	// Run through initialization parameters from right to left, checking for defaults
	switch (iArgC) {
	default:
		error("%s: ignoring extra arguments", kClassName);
		// fall into next case...
	case 4:
		// Seed, generate our own seed pool
		me->tausData	= Taus88New( AtomGetLong(&iArgV[3]) );
		if (me->tausData == NIL) {
			// We'll just work with the global seed pool
			// But let the user know
			error("%s: couldn't allocate seed pool", kClassName);
			}
		// fall into next case...
	case 3:
		BibiBeta(me, AtomGetFloat(&iArgV[2]));
		// fall into next case...
	case 2:
		BibiAlpha(me, AtomGetFloat(&iArgV[1]));
		// fall into next case...
	case 1:
		BibiNTrials(me, AtomGetLong(&iArgV[0]));
		// we're actually all done, but zero arguments is also a valid option so,
		// fall into next case...
	case 0:
		// Nothing else to do.
		break;
		}
	
	return me;
	}

static void BibiFree(objBibi* me)
	{ Taus88Free(me->tausData); }							// Taus88Free is NIL-safe


#pragma mark -
#pragma mark • Attribute/Information Functions

/******************************************************************************************
 *
 *	BibiTattle(me)
 *	BibiAssist
 *
 ******************************************************************************************/

static void
BibiTattle(
	objBibi* me)
	
	{
	
	post("%s state", kClassName);
	post("  number of trials is %lu", me->nTrials);
	post("  beta distribution parameters are %lf, %lf", me->alpha, me->beta);
	post("  Using beta generator:");
	switch (me->betaAlg) {
	case algConstZero:
		post("    always 0");
		break;
	case algConstOne:
		post("    always &lu", me->nTrials);
		break;
	case algIndeterm:
		post("    intederminate (select 0 and %lu randomly", me->nTrials);
		break;
	case algUniform:
		post("    'uniform'");
		break;
	case algArcSine:
		post("    'arc sine'");
		break;
	case algJoehnk:
		post("    using Jöhnk algorithm");
		break;
	case algChengBB:
		post("    using Cheng BB algorithm");
		break;
	case algChengBC:
		post("   using Cheng BC algorithm");
		break;
	default:
		// Must be genUndef
		post("    generating algorithm currently undetermined");
		}
	
	}


static void
BibiAssist(
	objBibi*	me,
	void*		box,
	long		iDir,
	long		iArgNum,
	char*		oCStr)
	
	{
	#pragma unused(box)
	
	UInt32	nTrials = me->nTrials;
	
	LitterAssistVA(iDir, iArgNum, strIndexInLeft, strIndexOutLeft, oCStr, nTrials);
	
	}


/******************************************************************************************
 *
 *	DoExpect()
 *
 *	Helper function for calculating expected values (mean, standard deviation, etc.)
 *	
 ******************************************************************************************/

// Currently only expected values for min/max.
// The math for everything else is extremely funky
static double DoExpect(objBibi* me, eExpectSelector iSel)
	{
	double	result	= 0.0 / 0.0;
	
	switch(iSel) {
	case expMean:
		break;
	case expMedian:
		break;
	case expMode:
		break;
	case expVar:
		break;
	case expStdDev:
		break;
	case expSkew:
		break;
	case expKurtosis:
		break;
	case expMin:
		result = 0.0;
		break;
	case expMax:
		result = me->nTrials;
		break;
	case expEntropy:
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

	static t_max_err BibiGetMin(objBibi* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expMin), ioArgC, ioArgV);
		}
	static t_max_err BibiGetMax(objBibi* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expMax), ioArgC, ioArgV);
		}
	static t_max_err BibiGetMean(objBibi* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expMean), ioArgC, ioArgV);
		}
	static t_max_err BibiGetMedian(objBibi* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expMedian), ioArgC, ioArgV);
		}
	static t_max_err BibiGetMode(objBibi* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expMode), ioArgC, ioArgV);
		}
	static t_max_err BibiGetVar(objBibi* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expVar), ioArgC, ioArgV);
		}
	static t_max_err BibiGetStdDev(objBibi* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expStdDev), ioArgC, ioArgV);
		}
	static t_max_err BibiGetSkew(objBibi* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expSkew), ioArgC, ioArgV);
		}
	static t_max_err BibiGetKurtosis(objBibi* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expKurtosis), ioArgC, ioArgV);
		}
	static t_max_err BibiGetEntropy(objBibi* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expEntropy), ioArgC, ioArgV);
		}
	
	static inline void
	AddInfo(void)
		{
		Object*	attr;
		Symbol*	symFloat64		= gensym("float64");
		Symbol*	symLong			= gensym("long");
		
		// Read-Write Attributes
		attr = attr_offset_new("n", symLong, 0, NULL, NULL, calcoffset(objBibi, nTrials));
		class_addattr(gObjectClass, attr);
		attr = attr_offset_new("alpha", symFloat64, 0, NULL, NULL, calcoffset(objBibi, alpha));
		class_addattr(gObjectClass, attr);
		attr = attr_offset_new("beta", symFloat64, 0, NULL, NULL, calcoffset(objBibi, beta));
		class_addattr(gObjectClass, attr);
		
		// Read-Only Attributes
		attr = attribute_new("min", symLong, kAttrFlagsReadOnly, (method) BibiGetMin, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("max", symLong, kAttrFlagsReadOnly, (method) BibiGetMax, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("mean", symFloat64, kAttrFlagsReadOnly, (method) BibiGetMean, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("median", symFloat64, kAttrFlagsReadOnly, (method) BibiGetMedian, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("mode", symFloat64, kAttrFlagsReadOnly, (method) BibiGetMode, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("var", symFloat64, kAttrFlagsReadOnly, (method) BibiGetVar, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("stddev", symFloat64, kAttrFlagsReadOnly, (method) BibiGetStdDev, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("skew", symFloat64, kAttrFlagsReadOnly, (method) BibiGetSkew, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("kurtosis", symFloat64, kAttrFlagsReadOnly, (method) BibiGetKurtosis, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("entropy", symFloat64, kAttrFlagsReadOnly, (method) BibiGetEntropy, NULL);
		class_addattr(gObjectClass, attr);
		}

static void
BibiTell(
	objBibi*	me,
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

static void BibiInfo(objBibi* me)
	{ LitterInfo(kClassName, &me->coreObject, (method) BibiTattle); }

static inline void AddInfo(void)
	{ LitterAddCant((method) BibiInfo, "info"); }

static void BibiTell(objBibi* me, Symbol* iTarget, Symbol* iMsg)
	{ LitterExpect((tExpectFunc) DoExpect, (Object*) me, iMsg, iTarget, TRUE); }

#endif

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
	LITTER_CHECKTIMEOUT(kClassName);
	
	// Standard Max setup() call
	LitterSetupClassGimme(	kClassName,
							sizeof(objBibi),			// Class object size
							LitterCalcOffset(objBibi),	// Magic "Obex" Calculation
							(method) BibiNew,			// Instance creation function
							(method) BibiFree,			// Custom deallocation function
							NIL);						// No menu function
	// Messages
	LITTER_TIMEBOMB LitterAddBang	((method) BibiBang);
	LitterAddMess1	((method) BibiNTrials,	"in1",		A_LONG);
	LitterAddMess1	((method) BibiAlpha,	"ft2",		A_FLOAT);
	LitterAddMess1	((method) BibiBeta,		"ft3",		A_FLOAT);
	LitterAddMess1 ((method) BibiSeed,		"seed",		A_DEFLONG);
	LitterAddMess2	((method) BibiTell,		"tell",		A_SYM, A_SYM);
	LitterAddMess0	((method) BibiTattle,	"tattle");
	LitterAddCant	((method) BibiTattle,	"dblclick");
	LitterAddCant	((method) BibiAssist,	"assist");
		
	AddInfo();
	
	//Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();
	
	}




