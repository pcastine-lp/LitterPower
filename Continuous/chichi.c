/*
	File:		chichi.c

	Contains:	Max external object generating a Chi-squared distribution.

	Written by:	Peter Castine

	Copyright:	© 2001-2002 Peter Castine

	Change History (most recent first):

         <9>   30Ð3Ð2006    pc      Update for new LitterLib organization
          <>     16Ð2Ð06    pc      We had previously thought that entropy calculations returning
                                    negative values were wrong. It turns out the continuous
                                    distributions (for which "entropy" means differential entropy)
                                    can very well generate negaitve entropy. So let the negative
                                    results roll.
         <7>     15Ð2Ð06    pc      Add support for Chi, Inverse-Chi, and Scale-inverse
                                    distributions. Also added expect message. Still need to support
                                    distributions and scale as add'l initialization arguments.
         <6>     15Ð1Ð04    pc      Avoid possible memory leak if seed argument is used.
         <5>     11Ð1Ð04    pc      Update for Windows.
         <4>    7Ð7Ð2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <3>  30Ð12Ð2002    pc      Drop faux 'Vers' resource and related modifications. Add object
                                    version to DoInfo().
         <2>  28Ð11Ð2002    pc      Tidy up after initial check-in.
         <1>  28Ð11Ð2002    pc      Initial check in.
		30-Jun-2001:	Modified to use Taus88 instead of TT800 as default generator.
						Also update to use new ULong2Unitxx() functions.
		13-Apr-2001:	Renamed for Puerto Rican golf great Chi Chi Rodriguez. Part of the
						general renaming overhaul. Added ChichiTattle, etc.
		2-Apr-2001:		First implementation.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark ¥ Include Files

#include "LitterLib.h"
#include "TrialPeriodUtils.h"
#include "RNGChi2.h"
#include "MoreMath.h"



#pragma mark ¥ Constants

const char	kClassName[]		= "lp.chichi";			// Class name



	// Indices for STR# resource
enum {
	strIndexInBang		= lpStrIndexLastStandard + 1,
	strIndexInChichiF,
	strIndexTheOutlet,
	
	strIndexDistChi2,
	strIndexDistChi,
	strIndexDistInv,
	strIndexDistScaleInv,
	
	strIndexInLeft		= strIndexInBang,
	strIndexOutLeft		= strIndexTheOutlet
	};

	// Variations on Chi-Square distribution
enum distVariant {
	distChiSquare	= 0,			// What we've always supported
	distChi,						// Chi distribution
	distInv,						// Chi-inverse, first definition (1/X)
	distScaleInv,					// Scale-inverse chi-squared (sigma^2*nu/X)
									// Note that this defaults to 2nd definition
									// of Chi-inverse when sigma2 is one
	
	distCount
	};	

	// Names used for symbols
char*	kDistSymNames[distCount] = {"chi2", "chi", "inv", "scaleinv"};
	
#pragma mark ¥ Macros

#define	GetDistStrIndex(X)	(strIndexDistChi2 + X)

#pragma mark ¥ Type Definitions

typedef enum distVariant eDistVar;

#pragma mark ¥ Object Structure

typedef struct {
	LITTER_CORE_OBJECT(Object, coreObject);
	
	tTaus88DataPtr	tausData;
	
	UInt32			dof;			// Degrees of Freedom (nu)
	double			scale,			// Normally unit except for scale-inverse
									// but we don't need to be fussy
					gamma;			// Aux parameter used when using Rejection algorith
									
	Symbol*			varSym;			// Fast way to get name of variant
	eDistVar		variant;		// More convenient for switch statements
	eChi2Alg		alg;
	} objChiSquare;



#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark ¥ Object Message Handlers

/******************************************************************************************
 *
 *	ChichiBang(me)
 *
 ******************************************************************************************/

static void
ChichiBang(
	objChiSquare* me)
	
	{
	double	result;
	
	// Start off with Chi-Squared generation
	switch (me->alg) {
	case algChi2Dir:
		result = GenChi2DirTaus88(me->dof, me->tausData);
		break;
	case algChi2Rej:
		result = GenChi2RejTaus88(me->dof, me->gamma, me->tausData);
		break;
	default:
		// Must be algChi2Const0
		result = 0.0;
		break;
		}
	
	// We could shave off a few bytes of object code by restructuring the following as a 
	// series of if/else statements, but the switch seems much easier to maintain.
	switch (me->variant) {
	default:
		// Must be chi-square (chi2). Nothing more to do.
		break;
		
	case distChi:
		result = sqrt(result);
		// ?? There is supposed to be a more efficient, direct algorithm for calculating
		//		chi variates in John F Monahan (1987), "An algorithm for generating chi
		//		random variables," ACM Transactions on Mathematical Software 13, pp168-171
		//		(Corrections 1988, ibid., 14 p.111)
		break;
	
	case distInv:
		result = 1.0 / result;
		break;
	
	case distScaleInv:
		result = ((double) me->dof) / result;
		break;
		}
	
	result *= me->scale;
	
	outlet_float(me->coreObject.o_outlet, result);
	
	}


/******************************************************************************************
 *
 *	ChichiDoF(me, iChichiF)
 *	ChichiScale(me, iScale)
 *	ChiChiVariant(me, iVarSym)
 *	
 *	Set Degrees of Freedom parameter. Make sure nothing bad is happening.
 *	
 ******************************************************************************************/

static void
ChichiDoF(
	objChiSquare*	me,
	long		iDoF)
	
	{
	if (iDoF > 0) {
		 me->dof	= iDoF;
		 me->alg	= Chi2RecommendAlg(iDoF);
		 me->gamma	= (me->alg == algChi2Rej)
		 				? CalcChi2RejGamma(iDoF)
		 				: 0.0;
		 }
	else error("%s: degrees of freedom must be positive", kClassName);
	}

static void
ChichiScale(
	objChiSquare*	me,
	double		iScale)
	
	{
	if (iScale > 0.0)
		 me->scale = iScale;
	else error("%s: scale must be positive", kClassName);
	}

static void
ChichiVariant(
	objChiSquare*	me,
	Symbol*		iVarSym)
	
	{
	int	i;
	
	for (i = 0; i < distCount; i += 1)
		if ( iVarSym == gensym(kDistSymNames[i]) ) break;
	
	if (i < distCount) {
		me->variant	= i;
		me->varSym	= iVarSym;
		}
	else error("%s: unrecognized distribution variant %s", kClassName, iVarSym->s_name);
	}
	


/******************************************************************************************
 *
 *	ChichiSeed(me, iSeed)
 *
 *	iSeed is treated as if it were unsigned, even if the user entered a negative value
 *
 ******************************************************************************************/

static void ChichiSeed(objChiSquare*	me, long iSeed)
	{ Taus88Seed(me->tausData, (unsigned long) iSeed); }
	
	
#pragma mark -
#pragma mark ¥ Class Message Handlers

/******************************************************************************************
 *
 *	ChichiNew(iOracle)
 *
 ******************************************************************************************/

static void ChichiFree(objChiSquare* me)
	{ Taus88Free(me->tausData); }							// Taus88Free is NIL-safe

static void*
ChichiNew(
	long	iDoF,				// Degrees of freedom
	long	iSeed)
	
	{
	const long		kDefDoF		= 1,
					kDefVariant	= distChiSquare;
	const double	kDefScale	= 1.0;
	
	objChiSquare*	me			= NIL;
	tTaus88DataPtr	myTausStuff	= NIL;
	
	// Run through initialization parameters from right to left, handling defaults
	if (iSeed == 0) Taus88Init();
	else {
		myTausStuff = Taus88New(iSeed);
		goto noMoreDefaults;
		}
	
	if (iDoF == 0) iDoF = kDefDoF;
noMoreDefaults:
	
	// Let Max allocate us, our inlets, and outlets
	me = (objChiSquare*) LitterAllocateObject();
	
	intin(me, 1);												// Degrees of Freedom inlet
	
	floatout(me);
	
	// Store object components, starting off with defaults (which are all valid)
	me->tausData	= myTausStuff;
	me->dof			= kDefDoF;
	me->scale		= kDefScale;
	me->gamma		= 0.0;
	me->variant		= kDefVariant;
	me->varSym		= gensym(kDistSymNames[kDefVariant]);
	
	// Has the user overridden any of the defaults?
	if (iDoF != kDefDoF)	ChichiDoF(me, iDoF);
	// ?? Need to allow user to override initial variant & scale
	
	return me;
	}

#pragma mark -
#pragma mark ¥ Attribute/Information Functions

/******************************************************************************************
 *
 *	ChichiTattle(me)
 *	ChichiAssist()
 *
 ******************************************************************************************/

static void
ChichiTattle(
	objChiSquare* me)
	
	{
	char	distName[sizeof(Str255)];
	
	LitterGetIndString(GetDistStrIndex(me->variant), distName);
	
	post("%s state", kClassName);
	post("  Degrees of freedom: %ld", me->dof);
	post("  Variant %ld ('%s', %s)", (long) me->variant, me->varSym->s_name, distName);
	post("  Scale: %lf", me->scale);
	if (me->scale != 1.0 && me->variant != distScaleInv)
		post("    %s normally has unit scale", distName);

	}


static void ChichiAssist(objChiSquare* me, void* box, long iDir, long iArgNum, char* oCStr)
	{
	#pragma unused(box)
	
	LitterAssistVA(iDir, iArgNum, strIndexInLeft, strIndexOutLeft, oCStr, me->dof);
	}


/******************************************************************************************
 *
 *	DoExpect()
 *
 *	Helper function for calculating expected values (mean, standard deviation, etc.) for
 *	Taus88
 *	
 ******************************************************************************************/

static double
DoExpect(
	objChiSquare*	me,
	eExpectSelector	iSel)
	
	{
	const double	kSqrt2	= 1.4142135624,
					kLn2	= 0.6931471806;
	
	double	result	= 0.0 / 0.0;					// Initialize to NaN
	long	dof		= me->dof;						// Need these for all calculations
	double	ddof	= (double) dof,
			scale	= me->scale;
	
	switch (me->variant) {
	default:										// Chi-Square Distribution
		switch(iSel) {
		case expMean:
			result = scale * ddof;
			break;
		case expMedian:
			result = ddof - (2.0/3.0);				// Approximation
			result *= scale;
			break;
		case expMode:
			if (dof >= 2)
				result = scale * (ddof - 2.0);
			break;
		case expVar:
		case expStdDev:								// Take square root at end of function
			result = ddof + ddof;
			result *= scale * scale;
			break;
		case expSkew:
			result = sqrt(8.0/ddof);
			break;
		case expKurtosis:
			result = 12.0 / ddof;
			break;
		case expEntropy:
			if (dof >= 2 && scale == 1.0) {
				double ddof2 = ddof * 0.5;
				long	dof2 = dof >> 1;			// Deliberately drop any remainder
					
				if (dof & 0x01) {
					// Odd value, dof == 2 * (dof2 + 1/2)
					result  = ddof2;
					result += lgamma(ddof2) + kLn2;	// Addend == log(2*gamma(ddof2))
					result += (1.0 - ddof2) * DigammaIntPlusHalf(dof2);
					}
				else {
					// Even value, dof/2 is an integer
					result  = ddof2;
					result += LogFactorial(dof2 - 1) + kLn2;
					result += (1.0 - ddof2) * DigammaInt(dof2);
					}
				}
			break;
		default:
			break;
			}
		
		break;
		// End of Chi-Square block
	
	case distChi:									// Chi Distribution	
		switch(iSel) {
		case expMean:
			result  = 0.5 * ddof;					// Intermediary result
			result  = gamma(result + 0.5) / gamma(result);
			result *= kSqrt2;
			result *= scale;
			break;
		case expMedian:
			// Undefined
			break;
		case expMode:
			result = scale * sqrt(ddof - 1.0);
			break;
		case expVar:
		case expStdDev:
			result  = DoExpect(me, expMean);		// Intermediary result
			result *= result;
			result  = ddof - result;
			result *= scale * scale;
			break;
		case expSkew:
			result  = DoExpect(me, expStdDev);		// Intermediary result
			result  = (1.0 - 2.0 * result * result) / (result * result * result);
			result *= DoExpect(me, expMean);
			break;
		case expKurtosis:
			{
			const double	mean 	= DoExpect(me, expMean),
							var		= DoExpect(me, expVar),
							skew	= DoExpect(me, expSkew);
			
			result  = 1.0 - mean * sqrt(var) * skew - var;
			result /= var;
			result += result;
			}
			break;
		case expEntropy:
			if (scale == 1.0) {
				result  = 0.5 * ddof;
				result += lgamma(result);
				result -= 0.5 * kLn2;
				result -= (ddof - 1.0) * ( (dof & 0x01)
												? DigammaIntPlusHalf(dof >> 1)
												: DigammaInt(dof >> 1) );
				
				// Sometimes we get bad results, reset to undefined
				if (result <= 0.0) result = 0.0 / 0.0;
				}
			break;
		default:
			break;
			}
		break;
		// End of distChi
	
	case distInv:						// Chi-inverse, first definition (1/X)
		switch(iSel) {
		case expMean:
			if (dof > 2)
				result = scale / (ddof - 2.0);
			break;
		case expMedian:
			// Undefined
			break;
		case expMode:
			result = scale / (ddof + 2.0);
			break;
		case expVar:
		case expStdDev:
			if (dof > 4) {
				result  = ddof / (ddof - 2.0);
				result *= result;
				result += result;
				result /= (ddof - 4.0);
				result *= scale * scale;
				}
			break;
		case expSkew:
			if (dof > 6) {
				result  = ddof - 4.0;
				result += result;
				result  = sqrt(result);
				result += result;
				result += result;
				result /= (ddof - 6.0);
				}
			break;
		case expKurtosis:
			if (dof > 8) {
				result  = 60.0 * ddof;
				result -= 264.0;
				result /= ((ddof - 6.0) * (ddof - 8.0));
				}
			break;
		case expEntropy:
			{
			double ddof2 = 0.5 * ddof;
			
			result  = ddof2;
			result += lgamma(ddof2);
			result += log(0.5 * scale);
			result -= (1.0 + ddof2) * ( (dof & 0x01)
											? DigammaIntPlusHalf(dof >> 1)
											: DigammaInt(dof >> 1) );
			
			// Sometimes we get bad results, reset to undefined
			if (result <= 0.0) result = 0.0 / 0.0;
			}
			break;
		default:
			break;
			}
		break;
		// End of distInv

	case distScaleInv:
		switch(iSel) {
		case expMean:
			if (dof > 2)
				result = scale * ddof / (ddof - 2.0);
			break;
		case expMedian:
			// Undefined
			break;
		case expMode:
			result = scale * ddof / (ddof + 2.0);
			break;
		case expVar:
		case expStdDev:
			if (dof > 4) {
				result  = ddof / (ddof - 2.0);
				result *= result;
				result *= scale * scale;
				result /= (ddof - 4.0);
				result += result;
				}
			break;
		case expSkew:
			if (dof > 6) {
				result  = ddof - 4.0;
				result += result;
				result  = sqrt(result);
				result += result;
				result += result;
				result /= (ddof - 6.0);
				}
			break;
		case expKurtosis:
			if (dof > 8) {
				result  = 60.0 * ddof;
				result -= 264.0;
				result /= ((ddof - 6.0) * (ddof - 8.0));
				}
			break;
		case expEntropy:
			{
			double ddof2 = 0.5 * ddof;
			
			result  = ddof2;
			result += lgamma(ddof2);
			result += log(scale * ddof2);
			result -= (1.0 + ddof2) * ( (dof & 0x01)
											? DigammaIntPlusHalf(dof >> 1)
											: DigammaInt(dof >> 1) );
			
			// Sometimes we get bad results, reset to undefined
			if (result <= 0.0) result = 0.0 / 0.0;
			}
			break;
		default:
			break;
			}
		break;
		// End of distScaleInv
	
		}
	
	if (iSel == expStdDev)
		result = sqrt(result);
		
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

	static t_max_err ChichiGetMin(objChiSquare* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMin), ioArgC, ioArgV); }
	static t_max_err ChichiGetMax(objChiSquare* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMax), ioArgC, ioArgV); }
	static t_max_err ChichiGetMean(objChiSquare* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMean), ioArgC, ioArgV); }
	static t_max_err ChichiGetMedian(objChiSquare* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMedian), ioArgC, ioArgV); }
	static t_max_err ChichiGetMode(objChiSquare* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMode), ioArgC, ioArgV); }
	static t_max_err ChichiGetVar(objChiSquare* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expVar), ioArgC, ioArgV); }
	static t_max_err ChichiGetStdDev(objChiSquare* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expStdDev), ioArgC, ioArgV); }
	static t_max_err ChichiGetSkew(objChiSquare* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expSkew), ioArgC, ioArgV); }
	static t_max_err ChichiGetKurtosis(objChiSquare* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expKurtosis), ioArgC, ioArgV); }
	static t_max_err ChichiGetEntropy(objChiSquare* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expEntropy), ioArgC, ioArgV); }
	
	static inline void
	AddInfo(void)
		{
		Object*	attr;
		Symbol*	symFloat64		= gensym("float64");
		Symbol*	symLong			= gensym("long");
		
		// Read-Write Attributes
		// !! TO DO: make dof, scale, variant settable attributes
		
		// Read-Only Attributes
		attr = attribute_new("min", symFloat64, kAttrFlagsReadOnly, (method) ChichiGetMin, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("max", symFloat64, kAttrFlagsReadOnly, (method) ChichiGetMax, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("mean", symFloat64, kAttrFlagsReadOnly, (method) ChichiGetMean, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("median", symFloat64, kAttrFlagsReadOnly, (method) ChichiGetMedian, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("mode", symFloat64, kAttrFlagsReadOnly, (method) ChichiGetMode, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("var", symFloat64, kAttrFlagsReadOnly, (method) ChichiGetVar, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("stddev", symFloat64, kAttrFlagsReadOnly, (method) ChichiGetStdDev, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("skew", symFloat64, kAttrFlagsReadOnly, (method) ChichiGetSkew, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("kurtosis", symFloat64, kAttrFlagsReadOnly, (method) ChichiGetKurtosis, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("entropy", symFloat64, kAttrFlagsReadOnly, (method) ChichiGetEntropy, NULL);
		class_addattr(gObjectClass, attr);
		}

static void
ChichiTell(
	objChiSquare*	me,
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

static void ChichiInfo(objChiSquare* me)
	{ LitterInfo(kClassName, &me->coreObject, (method) ChichiTattle); }

static inline void AddInfo(void)
	{ LitterAddCant((method) ChichiInfo, "info"); }
		
static void ChichiTell(objChiSquare* me, Symbol* iTarget, Symbol* iAttrName)
	{ LitterExpect((tExpectFunc) DoExpect, (Object*) me, iAttrName, iTarget, TRUE); }

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
main(void)
	
	{
	const tTypeList myArgTypes = {
						A_DEFLONG,			// Optional arguments:	1. degrees of freedom
						A_DEFLONG,			// 						2. seed
						A_NOTHING
						};
	
	LITTER_CHECKTIMEOUT(kClassName);
	
	// Standard Max setup() call
	LitterSetupClass(	kClassName,
						sizeof(objChiSquare),			// Class object size
						LitterCalcOffset(objChiSquare),
						(method) ChichiNew,				// Instance creation function
						(method) ChichiFree,			// Custom deallocation function
						NIL,							// No menu function
						myArgTypes);	
	

	// Messages
	LITTER_TIMEBOMB LitterAddBang((method) ChichiBang);
	LitterAddMess1	((method) ChichiDoF,	"in1",	A_FLOAT);
	LitterAddMess1	((method) ChichiScale,	"scale",A_FLOAT);
	LitterAddMess1	((method) ChichiVariant,"var",	A_SYM);
	LitterAddMess1	((method) ChichiSeed,	"seed",	A_DEFLONG);
	LitterAddMess2	((method) ChichiTell,	"tell", A_SYM, A_SYM);
	LitterAddMess0	((method) ChichiTattle,	"tattle");
	LitterAddCant	((method) ChichiTattle,	"dblclick");
	LitterAddCant	((method) ChichiAssist,	"assist");
	
	AddInfo();

	
	// Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();
	
	}

