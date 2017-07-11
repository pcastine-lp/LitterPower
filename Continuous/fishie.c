/*
	File:		fishie.c

	Contains:	Max external object generating a Fisher distribution.

	Written by:	Peter Castine

	Copyright:	© 2001-2002 Peter Castine

	Change History (most recent first):

         <8>   30–3–2006    pc      Update for new LitterLib organization
         <7>     18–2–06    pc      Add support for expect message.
         <6>     15–1–04    pc      Avoid possible memory leak if seed argument is used.
         <5>     11–1–04    pc      Update for Windows.
         <4>    7–7–2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <3>  30–12–2002    pc      Drop faux 'Vers' resource and related modifications. Add object
                                    version to FishieInfo().
         <2>  28–11–2002    pc      Tidy up after initial check-in.
         <1>  28–11–2002    pc      Initial check in.
		1-Jul-2001:		Modified to use Taus88 instead of TT800 as default generator.
		13-Apr-2001:	Updated name and added std. features for Litter Power Package.
		2-Apr-2001:		First implementation.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "LitterLib.h"
#include "TrialPeriodUtils.h"
#include "Taus88.h"
#include "RNGChi2.h"


#pragma mark • Constants

const char	kClassName[]	= "lp.fishie";			// Class name


	// Indices for STR# resource
enum {
	strIndexInBang		= lpStrIndexLastStandard + 1,
	strIndexInFishieF1,
	strIndexInFishieF2,
	
	strIndexTheOutlet
	};


#pragma mark • Type Definitions



#pragma mark • Object Structure

typedef struct {
	LITTER_CORE_OBJECT(Object, coreObject);
	
	tTaus88DataPtr	tausData;
	
	UInt32			dof1,
					dof2;
	double			dofInv1,
					dofInv2,
					gamma1,				// Gammas are an auxiliary value needed for large
					gamma2;				// values for Degrees of Freedom
	
	eChi2Alg		alg1,
					alg2;	
	} objFisher;


#pragma mark -

/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	FishieBang(me)
 *
 ******************************************************************************************/

static void
FishieBang(
	objFisher* me)
	
	{
	tTaus88DataPtr	tausData = me->tausData;
	double			fish1	= (me->alg1 == algChi2Dir)
									? GenChi2DirTaus88(me->dof1, tausData)
									: GenChi2RejTaus88(me->dof1, me->gamma1, tausData),
					fish2	= (me->alg2 == algChi2Dir)
									? GenChi2DirTaus88(me->dof2, tausData)
									: GenChi2RejTaus88(me->dof2, me->gamma2, tausData);
	
	fish1 *= me->dofInv1;
	fish2 *= me->dofInv2;
	
	outlet_float(me->coreObject.o_outlet, fish1 / fish2);
	}


/******************************************************************************************
 *
 *	FishieF1(me, iFishieF)
 *	FishieF2(me, iFishieF)
 *	FishieSeed(me, iSeed)
 *	
 *	Set parameters/seed, make sure nothing bad happens.
 *	
 *	NB: We allow the user to set F1 to 0 here (as opposed to initialization arguments,
 *		where 0 sigma defaults to 1). F2 cannot be 0, otherwise we get divide by 0 errors.
 *
 ******************************************************************************************/

	static inline void
	UpdateDoF(long iDoF, UInt32* oStore, eChi2Alg* oAlg, double* oGamma, double* oInv)
		{
		eChi2Alg	alg;
		
		// Sanity check
		if (iDoF <= 0)	iDoF = 1;
		
		alg = Chi2RecommendAlg(iDoF);
		
		*oStore	= iDoF;
		*oAlg	= alg;
		*oGamma	= (alg == algChi2Rej) ? CalcChi2RejGamma(iDoF) : 0.0;
		*oInv	= 1.0 / (double) iDoF;
		
		}

static void FishieF1(objFisher* me, long iF1)
	{ UpdateDoF(iF1, &me->dof1, &me->alg1, &me->gamma1, &me->dofInv1); }
	
static void FishieF2(objFisher* me, long iF2)
	{ UpdateDoF(iF2, &me->dof2, &me->alg2, &me->gamma2, &me->dofInv2); }
	

static void FishieSeed	(objFisher*	me, long iSeed)
	{ Taus88Seed(me->tausData, (unsigned long) iSeed); }


#pragma mark -
#pragma mark • Attribute/Information Functions

/******************************************************************************************
 *
 *	FishieTattle(me)
 *	FishieAssist(me, iBox, iDir, iArgNum, oCStr)
 *
 ******************************************************************************************/

static void
FishieTattle(
	objFisher* me)
	
	{
	post("%s state", kClassName);
	post("  Degrees of Freedom: %ld, %ld", me->dof1, me->dof2);
	post("  Inverses: % lf, %lf", me->dofInv1, me->dofInv2);
	post("  Algorithms: %s and %s",
			(me->alg1 == algChi2Dir) ? "direct" : "rejection",
			(me->alg2 == algChi2Dir) ? "direct" : "rejection");
	post("  Gamma values (if applicable): % lf, %lf", me->gamma1, me->gamma2);
	}

static void FishieAssist(objFisher* me, void* box, long iDir, long iArgNum, char* oCStr)
	{
	#pragma unused(me, box)
	
	LitterAssist(iDir, iArgNum, strIndexInBang, strIndexTheOutlet, oCStr);
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
	objFisher*		me,
	eExpectSelector iSel)
	
	{
	double	result = 0.0 / 0.0,				// Initialize as undefined
			f1, f2;							// Copy members to these register if needed
	
	switch (iSel) {
	case expMean:
		if ((f2 = me->dof2) > 2.0) result = f2 / (f2 - 2.0);
		break;
	case expMode:
		if ((f1 = me->dof1) > 2.0) {
			f2 = me->dof2;
			result  = (f1 - 2.0) / f1;
			result *= f2 / (f2 + 2.0);
			}
		break;
	case expVar:
	case expStdDev:
		if ((f2 = me->dof2) > 4.0) {
			f1 = me->dof1;
			
			result  = f2 / (f2 - 2.0);
			result *= result;
			result *= f1 + f2 - 2.0;
			result /= f1 * (f2 - 4.0);
			result += result;
			
			if (iSel == expStdDev) result = sqrt(result);
			}
		break;
	case expSkew:
		if ((f2 = me->dof2) > 6.0) {
			f1 = me->dof1;
			
			result  = 8.0 * (f2 - 4.0);
			result /= f1 * (f1 + f2 + 2.0);
			result  = sqrt(result);
			result *= f1 + f1 + f2 - 2.0;
			result /= f2 - 6.0;
			}
		break;
	case expMin:
		result = 0.0;
		break;
	case expMax:
		result = 1.0 / 0.0;
		break;
	default:
		// everything else is undefined
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

	static t_max_err FishieGetMin(objFisher* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMin), ioArgC, ioArgV); }
	static t_max_err FishieGetMax(objFisher* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMax), ioArgC, ioArgV); }
	static t_max_err FishieGetMean(objFisher* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMean), ioArgC, ioArgV); }
	static t_max_err FishieGetMode(objFisher* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMode), ioArgC, ioArgV); }
	static t_max_err FishieGetVar(objFisher* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expVar), ioArgC, ioArgV); }
	static t_max_err FishieGetStdDev(objFisher* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expStdDev), ioArgC, ioArgV); }
	static t_max_err FishieGetSkew(objFisher* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expSkew), ioArgC, ioArgV); }
	static t_max_err FishieGetKurtosis(objFisher* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expKurtosis), ioArgC, ioArgV); }
	
	static t_max_err FishieSetAttrDoF1(objFisher* me, void* iAttr, long iArgC, Atom iArgV[])
		{
		
		if (iArgC > 0 && iArgV != NIL)
			FishieF1(me, AtomGetFloat(iArgV));
		
		return MAX_ERR_NONE;
		}
	
	static t_max_err FishieSetAttrDoF2(objFisher* me, void* iAttr, long iArgC, Atom iArgV[])
		{
		
		if (iArgC > 0 && iArgV != NIL)
			FishieF2(me, AtomGetFloat(iArgV));
		
		return MAX_ERR_NONE;
		}
	
	static inline void
	AddInfo(void)
		{
		Object*	attr;
		Symbol*	symFloat64		= gensym("float64");
		
		// Read-Write Attributes
		attr = attr_offset_new(	"dof1", symFloat64, 0,
								NULL, (method) FishieSetAttrDoF1,
								calcoffset(objFisher, dof1));
		class_addattr(gObjectClass, attr);
		attr = attr_offset_new(	"dof2", symFloat64, 0,
								NULL, (method) FishieSetAttrDoF2,
								calcoffset(objFisher, dof1));
		class_addattr(gObjectClass, attr);
		
		// Read-Only Attributes
		attr = attribute_new("min", symFloat64, kAttrFlagsReadOnly, (method) FishieGetMin, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("max", symFloat64, kAttrFlagsReadOnly, (method) FishieGetMax, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("mean", symFloat64, kAttrFlagsReadOnly, (method) FishieGetMean, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("mode", symFloat64, kAttrFlagsReadOnly, (method) FishieGetMode, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("var", symFloat64, kAttrFlagsReadOnly, (method) FishieGetVar, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("stddev", symFloat64, kAttrFlagsReadOnly, (method) FishieGetStdDev, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("skew", symFloat64, kAttrFlagsReadOnly, (method) FishieGetSkew, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("kurtosis", symFloat64, kAttrFlagsReadOnly, (method) FishieGetKurtosis, NULL);
		class_addattr(gObjectClass, attr);
		}

static void
FishieTell(
	objFisher*	me,
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

static void FishieInfo(objFisher* me)
	{ LitterInfo(kClassName, &me->coreObject, (method) FishieTattle); }

static inline void AddInfo(void)
	{ LitterAddCant((method) FishieInfo, "info"); }
		
void FishieTell(objFisher* me, Symbol* iTarget, Symbol* iAttrName)
	{ LitterExpect((tExpectFunc) DoExpect, (Object*) me, iAttrName, iTarget, TRUE); }

#endif

#pragma mark -
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	FishieFree(me)
 *	FishieNew(iFishieF1, iFishieF2, iSeed)
 *
 ******************************************************************************************/

static void FishieFree(objFisher* me)
	{ Taus88Free(me->tausData); }							// Taus88Free is NIL-safe

static void*
FishieNew(
	long	iDoF1,
	long	iDoF2,
	long	iSeed)
	
	{
	const long	kDefDoF			= 1;					// Same for both parameters

	objFisher*		me			= NIL;
	tTaus88DataPtr	myTausStuff	= NIL;
	
	// Run through initialization parameters from right to left, handling defaults
	if (iSeed != 0) {
		myTausStuff = Taus88New(iSeed);
		goto noMoreDefaults;
		}
	
	if (iDoF2 == 0)
		iDoF2 = kDefDoF;
	else goto noMoreDefaults;
	
	if (iDoF1 == 0)
		iDoF1 = kDefDoF;
noMoreDefaults:
	// Finished checking intialization parameters

	// Let Max allocate us, our inlets, and outlets
	me = (objFisher*) LitterAllocateObject();
	
	intin(me, 2);												// F2 inlet
	intin(me, 1);												// F1 inlet
	
	floatout(me);
	
	// Store object components
	me->tausData	= myTausStuff;
	FishieF1(me, iDoF1);
	FishieF2(me, iDoF2);
	
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
						A_DEFLONG,					// Optional arguments:	1: F1
						A_DEFLONG,					//						2: F2
						A_DEFLONG,					// 						3: seed
						};
	
	LITTER_CHECKTIMEOUT(kClassName);
	
	// Standard Max setup() call
	LitterSetupClass(	kClassName,
						sizeof(objFisher),
						LitterCalcOffset(objFisher),		// Class object size
						(method) FishieNew,					// Instance creation function
						(method) FishieFree,				// Custom deallocation function
						NIL,								// No menu function
						myArgTypes);				
	

	// Messages
	LITTER_TIMEBOMB LitterAddBang((method) FishieBang);
	LitterAddMess1	((method) FishieF1,		"in1",		A_LONG);
	LitterAddMess1	((method) FishieF2,		"in2",		A_LONG);
	LitterAddMess1 ((method) FishieSeed,	"seed",		A_DEFLONG);
	LitterAddMess2 ((method) FishieTell,	"tell", A_SYM, A_SYM);
	LitterAddMess0	((method) FishieTattle,	"tattle");
	LitterAddCant	((method) FishieTattle,	"dblclick");
	LitterAddCant	((method) FishieAssist,	"assist");
	
	AddInfo();
	
	// Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();
	
	}

