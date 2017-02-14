/*
	File:		bernie.c

	Contains:	Max external object generating a random values with Bernoulli distribution.

	Written by:	Peter Castine

	Copyright:	© 2001-2002 Peter Castine

	Change History (most recent first):

         <9>   26Ð4Ð2006    pc      Renamed LitterLib files for Binomial distribution
         <8>      4Ð3Ð06    pc      Add expect message. Massive performance improvements
         <7>     15Ð1Ð04    pc      Avoid possible memory leak if seed argument is used.
         <6>     14Ð1Ð04    pc      Try to grab Mac resources inside Windows DLL.
         <5>   8Ð16Ð2003    pc      Update for Windows.
         <4>    7Ð7Ð2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <3>  30Ð12Ð2002    pc      Drop faux 'Vers' resource and related modifications. Add object
                                    version to DoInfo().
         <2>  28Ð11Ð2002    pc      Tidy up after initial check in.
         <1>  28Ð11Ð2002    pc      Initial Check-In.
		30-Jun-2001:	Modified to use Taus88 instead of TT800 as default generator.
		12-Apr-2001:	Binary choice object integrated into this object
		4-Apr-2001:		First implementation.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark ¥ Include Files

#include "LitterLib.h"
#include "TrialPeriodUtils.h"
#include "Taus88.h"
#include "RNGBinomial.h"

#pragma mark ¥ Constants

const char*		kClassName		= "lp.bernie";		// Class name


	// Indices for STR# resource
enum {
	strIndexInBang		= lpStrIndexLastStandard + 1,
	strIndexInNTrials,
	strIndexInProb,
	
	strIndexOut0Trials,								// Use if nTrials is 0
	strIndexOut1Trial,								// Use if nTrials is 1
	strIndexOutNTrials								// Use in all other cases
	};

enum Generator {								// How to generate deviates
	genUndef			= -1,					// Haven't yet determined algorithm to use
	genAlwaysZero,								// n = 0 or p = 0
	genAlwaysN,									// p = 1
	genFiftyFifty,								// p = .5, n < ~768
	genBruteForce,								// n < ~ 15
	genBInversion,								// np < 20
	genBTPErej									// General-purpose
	};


#pragma mark ¥ Type Definitions

typedef enum Generator eGenerator;

typedef union genParams {
	UInt32		bfThresh;
	tBINVParams	binvStuff;
	tBTPEParams	btpeStuff;
	} uGenParams;

#pragma mark ¥ Object Structure

typedef struct {
	Object			coreObject;
	
	tTaus88DataPtr	tausData;
	
	double			prob;
	UInt32			nTrials;
	eGenerator		gen;
	uGenParams		params;
	} objBernie;


#pragma mark ¥ Global Variables



#pragma mark ¥ Function Prototypes

	// Class message functions
void*	BernieNew	(long, double, long);
void	BernieFree	(objBernie*);

	// Object message functions
static void BernieBang	(objBernie*);
static void BernieNTrials(objBernie*, long);
static void BernieProb	(objBernie*, double);
static void BernieExpect(objBernie*, Symbol*, long, Symbol*);
static void BernieSeed	(objBernie*, long);
static void BernieTattle(objBernie*);
static void	BernieAssist(objBernie*, void* , long , long , char*);
static void	BernieInfo	(objBernie*);

#ifndef __DEBUG__
	#define __DEBUG__ 0
#endif
#if __DEBUG__
	static void BernieGen	(objBernie*, long);
#endif

#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark ¥ Inline Functions



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
			(method) BernieNew,			// Instance creation function
			(method) BernieFree,		// Custom deallocation function
			(short) sizeof(objBernie),	// Class object size
			NIL,						// No menu function
			A_DEFLONG,					// Optional arguments:	1: Number of trials
			A_DEFFLOAT,					//						2: Probablility--p(0)
			A_DEFLONG,					// 						3: Seed
			0);		
	

	// Messages
	LITTER_TIMEBOMB addbang	((method) BernieBang);
	addmess	((method) BernieNTrials,	"in1",		A_LONG, 0);
	addmess	((method) BernieProb,		"ft2",		A_FLOAT, 0);
	addmess ((method) BernieSeed,		"seed",		A_DEFLONG, 0);
	addmess	((method) BernieExpect,		"expect",	A_SYM, A_DEFLONG, A_DEFSYM, 0);
	addmess	((method) BernieTattle,		"dblclick",	A_CANT, 0);
	addmess	((method) BernieTattle,		"tattle",	A_NOTHING);
	addmess	((method) BernieAssist,		"assist",	A_CANT, 0);
	addmess	((method) BernieInfo,		"info",		A_CANT, 0);
#if __DEBUG__
	addmess ((method) BernieGen,		"gen",		A_LONG, 0);
#endif
		
	//Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();
	
	}



#pragma mark -
#pragma mark ¥ Class Message Handlers

/******************************************************************************************
 *
 *	BernieNew(iOracle)
 *
 ******************************************************************************************/

void BernieFree(objBernie* me)
	{ Taus88Free(me->tausData); }							// Taus88Free is NIL-safe

void*
BernieNew(
	long	iNTrials,
	double	iProb,
	long	iSeed)
	
	{
	const long		kDefNTrials		= 1;
	const double	kDefProb		= 0.5;
	
	objBernie*	me = (objBernie*) newobject(gObjectClass);	// Default instantiation
	
	// Allocate inlets and outlets	
	floatin(me, 2);											// p(1)
	intin(me, 1);											// Number of Trials
	
	intout(me);					// Access main outlet through me->coreObject.o_outlet;
	
	// Set up our own members to defaults
	me->tausData	= NIL;
	me->prob		= kDefProb;
	me->nTrials		= kDefNTrials;
	me->gen			= genUndef;
		// Don't need to worry about genParams as long as the generator is undefined
	
	// Run through initialization parameters from right to left, checking for defaults
	if (iSeed == 0) Taus88Init();
	else me->tausData = Taus88New(iSeed);
	
	if (iProb == 0.0)	iProb = kDefProb;
	if (iNTrials == 0)	iNTrials = kDefNTrials;

	// Any changes from defaults?
	if (iNTrials != kDefNTrials)
		BernieNTrials(me, iNTrials);
	if (iProb != kDefProb)
		BernieProb(me, iProb);
	
	return me;
	}

#pragma mark -
#pragma mark ¥ Object Message Handlers

/******************************************************************************************
 *
 *	BernieBang(me)
 *
 ******************************************************************************************/

	static void SelectGenerator(objBernie* me)
		{
		const double	p = me->prob;
		const long		n = me->nTrials;
		
		if (n == 0 || p == 0.0)
			me->gen = genAlwaysZero;
		
		else if (p == 1.0)
			me->gen = genAlwaysN;
			
		else if (p == 0.5 && n <= 768)
			me->gen	= genFiftyFifty;
		
		else if (n <= 15) {
			me->gen = genBruteForce;
			me->params.bfThresh = CalcBFThreshhold(p);
			}
			
		else {
			// It'sgoing to be either Inversion or BTPE
			// For either we reflect distributions with p > 0.5
			const double effectiveP = (p > 0.5) ? 1.0 - p : p;
			
			if (effectiveP * n < 16) {
				me->gen = genBInversion;
				CalcBINVParams(&me->params.binvStuff, n, p);
				}
			
			else {
				me->gen = genBTPErej;
				CalcBTPEParams(&me->params.btpeStuff, n, p);
				}
			}
		
		}

void
BernieBang(
	objBernie* me)
	
	{
	long	b;
	
	if (me->gen == genUndef)
		SelectGenerator(me);
		
	switch (me->gen) {
	default:											// Must be genAlwaysZero
		b = 0;
		break;
	
	case genAlwaysN:
		b = me->nTrials;
		break;
	
	case genFiftyFifty:
		b = GenDevBinomialFCTaus88(me->nTrials, me->tausData);
		break;
	
	case genBruteForce:
		b = GenDevBinomialBFTaus88(me->nTrials, me->params.bfThresh, me->tausData);
		break;
	
	case genBInversion:
		b = GenBinomialBINV(&me->params.binvStuff, (tRandomFunc) Taus88, (void*) me->tausData);
		break;
	
	case genBTPErej:
		b = GenBinomialBTPE(&me->params.btpeStuff, (tRandomFunc) Taus88, (void*) me->tausData);
		break;
		}
	
	outlet_int(me->coreObject.o_outlet, b);
	}
	


/******************************************************************************************
 *
 *	BernieProb(me, iProb)
 *	BernieNTrials(me, iNTrials)
 *	BernieSeed(me, iSeed)
 *	
 *	Set parameters. Make sure nothing bad is happening.
 *	
 ******************************************************************************************/

void BernieNTrials(objBernie* me, long iNTrials)
	{ me->nTrials = (iNTrials >= 0) ? iNTrials : 0; me->gen = genUndef; }

void BernieProb(objBernie* me, double iProb)
	{
	if		(iProb < 0.0) iProb = 0.0;
	else if (iProb > 1.0) iProb = 1.0;
	
	me->prob	= iProb;
	me->gen 	= genUndef;
	}
	
void BernieSeed(objBernie* me, long iSeed)
	{ if (me->tausData) Taus88Seed(me->tausData, (UInt32) iSeed); }

/******************************************************************************************
 *
 *	BernieExpect(me, iSelector, iLabel, iRecip)
 *
 ******************************************************************************************/

	static double DoExpect(objBernie* me, eExpectSelector iSel)
		{
		double	p		= me->prob,
				np		= p * ((double) me->nTrials),
				q,
				result	= 0.0 / 0.0;
		
		switch(iSel) {
		case expMean:
			result = np;
			break;
		case expMedian:					// Not completely sure about median
		case expMode:
			result = floor(np + p);
			if (result == (np + p))		// In this case there are two modes
				result -= 1.0;			// Rather than returning NaN, return the lower mode
			break;
		case expVar:
			result = np * (1.0 - p);
			break;
		case expStdDev:
			result = sqrt(np * (1.0 - p));
			break;
		case expSkew:
			if (p == 0.5 || p == 0.0 || p == 1.0)
				result = 0.0;
			else {
				q		= 1.0 - p;
				result  = (q - p);
				result /= sqrt(np * q);
				}
			break;
		case expKurtosis:
			q		= 1.0 - p;
			result  = 1.0;
			result -= 6.0 * p * q;
			result /= np * q;
			break;
		case expMin:
			result = 0.0;
			break;
		case expMax:
			result = (me->gen == genAlwaysZero) ? 0.0 : me->nTrials;
			break;
		case expEntropy:
			// Handle extremes, then take a brute force approach up to some point, 
			// then approximate using formula for a Gaussian distribution
			result	= 0.0;
			q		= 1.0 - p;
			
			if (p > 0.0 && q > 0.0) {
				long n = me->nTrials;
				
				switch (n) {
				case 0:									// Entropy is zero and that's 
					break;								// what's in the result register
				
				case 1:									// Simple case
					result -= p * log2(p);
					result -= q * log2(q);
					break;
				
				default:
					if (n <= 24) {						// Pragmatic threshhold
						long	k,
								ncombk	= 1;
						double	pk		= 1.0,			// p^k, start from k = 0;
								qinv	= 1.0 / q,
								qnk 	= pow(q, n);	// q^(n-k), also start from k = 0;
						
						result = 0.0;
						for (k = 0; k <= n; /* increment inside loop */) {
							double term = ncombk * pk * qnk;
							
							result -= term * log2(term);
							
							// Increment 
							pk *= p;
							qnk *= qinv;
							ncombk *= n - k++;
							ncombk /= k;
							}
						}
					else {
						// This is lookin' pretty close to a Gauss distribution
						result 	= np * q;				// Variance
						result *= 2.0 * kPi * kNatLogBase;
						result  = log2( sqrt(result) );
						}
					break;
					}
				}
			break;
			
		default:
			break;
			}
			
		return result;
		}


void BernieExpect(objBernie* me, Symbol* iSelector, long iLabel, Symbol* iTarget)
	{ LitterExpect((tExpectFunc) DoExpect, (Object*) me, iSelector, iTarget, iLabel != 0); }

/******************************************************************************************
 *
 *	BernieTattle(me)
 *	BernieAssist
 *	BernieInfo(me)
 *
 ******************************************************************************************/

void
BernieTattle(
	objBernie* me)
	
	{
	
	post("%s state", kClassName);
	post("  number of trials is %lu", me->nTrials);
	post("  p is %lf", me->prob);
	post("  Using generator:");
	switch (me->gen) {
	case genAlwaysZero:
		post("    always zero");
		break;
	case genAlwaysN:
		post("    always n");
		break;
	case genFiftyFifty:
		post("    flip-coin algorithm");
		break;
	case genBruteForce:
		post("    brute force algorithm");
		post("    threshhold set to %ul", me->params.bfThresh);
		break;
	case genBInversion:
		post("    Binomial Inversion algorithm");
		if (me->params.binvStuff.mirror)
			post("    using mirrored distribution");
		post("    q^n = %lf", me->params.binvStuff.qPowN);
		post("    p/q = %lf", me->params.binvStuff.pOverQ);
		break;
	case genBTPErej:
		post("    Kachitvichyanukul and Schmeiser BTPE algorithm");
		if (me->params.btpeStuff.mirror)
			post("    using mirrored distribution");
		post("    q = %lf", me->params.btpeStuff.q);
		post("    n * p * q = %lf", me->params.btpeStuff.var);
		post("    cached mode = %lf", me->params.btpeStuff.mm);
		post("    cumulative areas p1-4 = %lf, %lf, %lf, %lf",
				me->params.btpeStuff.p1, me->params.btpeStuff.p2,
				me->params.btpeStuff.p3, me->params.btpeStuff.p4);
		post("    xm, xl, xr = %lf, %lf, %lf", me->params.btpeStuff.xm,
				me->params.btpeStuff.xl, me->params.btpeStuff.xr);
		post("    lambdas L/R = %lf, %lf",
				me->params.btpeStuff.lambdaL, me->params.btpeStuff.lambdaR);
		break;
	default:
		// Must be genUndef
		post("    generating algorithm currently undetermined");
		}
	
	}


void
BernieAssist(
	objBernie*	me,
	void*		box,
	long		iDir,
	long		iArgNum,
	char*		oCStr)
	
	{
	#pragma unused(box)
	
	UInt32	nTrials = me->nTrials;
	
	switch (nTrials) {
	case 0:
	 	LitterAssist(iDir, iArgNum, strIndexInBang, strIndexOut0Trials, oCStr);
	 	break;
	 
	 case 1:
		LitterAssistVA(	iDir, iArgNum, strIndexInBang, strIndexOut1Trial, oCStr,
						me->prob);
		break;
		
	default:
		LitterAssistVA(	iDir, iArgNum, strIndexInBang, strIndexOutNTrials, oCStr,
	 					nTrials);
	 	break;
		 }
	
	}

void BernieInfo(objBernie* me)
	{ LitterInfo(kClassName, &me->coreObject, (method) BernieTattle); }


#if __DEBUG__

/******************************************************************************************
 *
 *	BernieGen(me)
 *
 ******************************************************************************************/

	static void ClearParamsUnion(uGenParams* p)
		{
		int		bytesToClear = sizeof(uGenParams);
		long*	pp = (long*) p;
		
		while (bytesToClear > 0) {
			*pp++ = 0;
			bytesToClear -= 4;
			}
		
		}

void
BernieGen(
	objBernie*	me,
	long		iAlg)
	
	{
	
	switch (iAlg) {
	case genUndef:
	case genAlwaysZero:
	case genAlwaysN:
	case genFiftyFifty:
		ClearParamsUnion(&me->params);
		me->gen = iAlg;
		break;
	case genBruteForce:
		ClearParamsUnion(&me->params);
		me->gen = iAlg;
		me->params.bfThresh = CalcBFThreshhold(me->prob);
		break;
	case genBInversion:
		ClearParamsUnion(&me->params);
		me->gen = iAlg;
		CalcBINVParams(&me->params.binvStuff, me->nTrials, me->prob);
		break;
	case genBTPErej:
		ClearParamsUnion(&me->params);
		me->gen = iAlg;
		CalcBTPEParams(&me->params.btpeStuff, me->nTrials, me->prob);
		break;
	default:
		// Ignore invalid input
		error("%s: invalid parameter for gen: %ld", kClassName, iAlg);
		break;
		}
		
	}
		
#endif
