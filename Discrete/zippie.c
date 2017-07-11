/*
	File:		zippie.c

	Contains:	Max external object generating a Zipf (zeta) distribution.

	Written by:	Peter Castine

	Copyright:	© 2006 Peter Castine

	Change History (most recent first):

         <2>     16–3–06    pc      Wikipedia defines the distirubtion differently from Dagnupar. We
                                    used the former for DoExpect() but the latter for generating
                                    values. We now follow Dagnupar's definition, which seems to have
                                    higher acceptance, throughout.
         <1>     15–3–06    pc      first checked in.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "LitterLib.h"
#include "TrialPeriodUtils.h"
#include "Taus88.h"
#include "MiscUtils.h"
#include "MoreMath.h"

//#include <math.h>


#pragma mark • Constants

const char*		kClassName		= "lp.zippie";			// Class name


	// Indices for STR# resource
enum {
	strIndexInBang		= lpStrIndexLastStandard + 1,
	strIndexInZeta,
	strIndexInN,
	
	strIndexTheOutlet,
	
	strIndexInLeft		= strIndexInBang,
	strIndexOutLeft		= strIndexTheOutlet
	};


#pragma mark • Object Structure

typedef struct {
	LITTER_CORE_OBJECT(Object, coreObject);
	
	tTaus88DataPtr	tausData;
	
	double			rho,
					delta,				// rho - 0.5
					logos;				// (rho + 1) * ln(1 / (rho + 1))
	} objZipf;



#pragma mark -

/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	ZippieBang(me)
 *
 ******************************************************************************************/

static void
ZippieBang(
	objZipf* me)
	
	{
	double			rho	= me->rho;
	long			zeta;
	UInt32			s1, s2, s3;
	tTaus88Data*	td = me->tausData;
	
	if (td == NIL)
		td = Taus88GetGlobals();
	Taus88Load(td, &s1, &s2, &s3);
	
	if (rho > 0.0) {
		double			rho		= me->rho,
						delta	= me->delta,
						logos	= me->logos,
						n, e, thresh;
		do {
			double	u1 = ULong2Unit_zo( Taus88Process(&s1, &s2, &s3) ),
					u2 = ULong2Unit_zo( Taus88Process(&s1, &s2, &s3) ),
					x  = rho * pow(u1, -1.0 / rho) - delta;
			
			n = floor(x + 0.5);
			e = -log(u2);
			
			thresh = (rho + 1.0) * log(n / (x + delta)) - logos;
			} while (e <= thresh);
		
		zeta = (long) n;
		}
	else do { zeta = Taus88Process(&s1, &s2, &s3) & 0x7fffffff; } while (zeta == 0);
		// Slightly cheesy uniform distribution of positive integers
	
	Taus88Store(td, s1, s2, s3);
	
	outlet_int(me->coreObject.o_outlet, zeta);
	}


/******************************************************************************************
 *
 *	ZippieLambda(me, iLambda)
 *	ZippieSeed(me, iSeed)
 *	
 *	Set parameter. Make sure nothing bad is happening.
 *	
 ******************************************************************************************/

static void
ZippieRho(
	objZipf*	me,
	double		iRho)
	
	{
//	const double kMinRho = 0.00000011920929;		// Arbitrary, smaller than anything
													// we are likely to be sent in an Atom
	// Sanity check
	if (iRho > 0.0) {
		me->rho		= iRho;
		me->delta	= iRho - 0.5;
		
		// Increment rho once, use the incremented value twice
		iRho	 += 1.0;
		me->logos = iRho * log(1.0 / iRho);
		}
	else {
		me->rho		= 0.0;
		me->delta	= 0.0;
		me->logos	= 0.0;
		}
	
	}
	

static void ZippieSeed(objZipf* me, long iSeed)
	{ Taus88Seed(me->tausData, (unsigned long) iSeed); }


#pragma mark -
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	ZippieNew(iRho, iSeed)
 *
 ******************************************************************************************/

static void*
ZippieNew(
	double	iRho,
	long	iSeed)
	
	{
	const double	kDefRho	= 1.0;
	
	objZipf*		me			= NIL;
	tTaus88DataPtr	myTausData	= NIL;
	
	// Run through initialization parameters from right to left, handling defaults
	if (iSeed == 0) Taus88Init();						// Use Taus88's data pool
	else {
		myTausData = Taus88New(iSeed);
		goto noMoreDefaults;
		}
	
	if (iRho == 0.0)
		iRho = kDefRho;
noMoreDefaults:
	// Finished checking intialization parameters
	
	
	// Let Max allocate us, our inlets, and outlets
	me = (objZipf*) LitterAllocateObject();

	floatin(me, 1);												// lambda inlet
	
	intout(me);					// Access main outlet through me->coreObject.o_outlet;
	
	// Store object components
	me->tausData = myTausData;
	ZippieRho(me, iRho);
	
	return me;
	}

static void ZippieFree(objZipf* me)
	{ Taus88Free(me->tausData); }							// Taus88Free is NIL-safe

#pragma mark -
#pragma mark • Attribute/Information Functions

/******************************************************************************************
 *
 *	ZippieTattle(me)
 *	ZippieAssist
 *
 ******************************************************************************************/

static void
ZippieTattle(
	objZipf* me)
	
	{
	
	post("%s state",
			kClassName);
	post("  zeta exponent: %lf (delta %lf, logos %lf)", me->rho, me->delta, me->logos);
	
	}

static void ZippieAssist(objZipf* me, void* iBox, long iDir, long iArgNum, char* oCStr)
	
	{
	#pragma unused (me, iBox)
	
	LitterAssist(iDir, iArgNum, strIndexInLeft, strIndexOutLeft, oCStr);
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
	objZipf*		me,
	eExpectSelector iSel)
	
	{
	double	result	= 0.0 / 0.0,
			rho;
	
	switch(iSel) {
	case expMean:
		rho = me->rho;
		if (rho > 1.0)
			result = RiemannZeta(rho) / RiemannZeta(rho + 1.0);
		break;
	case expVar:
	case expStdDev:
		rho = me->rho;
		if (rho > 2.0) {
			double  zetaRho1 = RiemannZeta(rho + 1.0);
			
			result  = RiemannZeta(rho) / zetaRho1;
			result *= result;
			result  = (RiemannZeta(rho - 1.0) / zetaRho1) - result;
			
			if (iSel == expStdDev) result = sqrt(result);
			}
		break;
	case expMode:
	case expMin:
		result = 1.0;
		break;
	case expMax:
		result = 1.0 / 0.0;
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

	static t_max_err ZippieGetMean(objZipf* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expMean), ioArgC, ioArgV);
		}
	static t_max_err ZippieGetMedian(objZipf* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expMedian), ioArgC, ioArgV);
		}
	static t_max_err ZippieGetMode(objZipf* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expMode), ioArgC, ioArgV);
		}
	static t_max_err ZippieGetVar(objZipf* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expVar), ioArgC, ioArgV);
		}
	static t_max_err ZippieGetStdDev(objZipf* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expStdDev), ioArgC, ioArgV);
		}
	static t_max_err ZippieGetSkew(objZipf* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expSkew), ioArgC, ioArgV);
		}
	static t_max_err ZippieGetKurtosis(objZipf* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expKurtosis), ioArgC, ioArgV);
		}
	static t_max_err ZippieGetEntropy(objZipf* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expEntropy), ioArgC, ioArgV);
		}
	
	static t_max_err ZippieSetAttrRho(objZipf* me, void* iAttr, long* iArgC, Atom iArgV[])
		{
		#pragma unused(iAttr)
		
		if (*iArgC > 0 && iArgV != NIL)
			ZippieRho(me, AtomGetFloat(iArgV));
		
		return MAX_ERR_NONE;
		}
	
	static inline void
	AddInfo(void)
		{
		Object*	attr;
		Symbol*	symFloat64		= gensym("float64");
//		Symbol*	symLong			= gensym("long");
		
		// Read-Write Attributes
		attr = attr_offset_new(	"rho", symFloat64, 0,
								NULL, (method) ZippieSetAttrRho,
								calcoffset(objZipf, rho));
		class_addattr(gObjectClass, attr);
//		attr = attr_offset_new("max", symLong, 0, NULL, NULL, calcoffset(objZipf, max));
//		class_addattr(gObjectClass, attr);
		
		// Read-Only Attributes
		attr = attribute_new("mean", symFloat64, kAttrFlagsReadOnly, (method) ZippieGetMean, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("median", symFloat64, kAttrFlagsReadOnly, (method) ZippieGetMedian, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("mode", symFloat64, kAttrFlagsReadOnly, (method) ZippieGetMode, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("var", symFloat64, kAttrFlagsReadOnly, (method) ZippieGetVar, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("stddev", symFloat64, kAttrFlagsReadOnly, (method) ZippieGetStdDev, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("skew", symFloat64, kAttrFlagsReadOnly, (method) ZippieGetSkew, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("kurtosis", symFloat64, kAttrFlagsReadOnly, (method) ZippieGetKurtosis, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("entropy", symFloat64, kAttrFlagsReadOnly, (method) ZippieGetEntropy, NULL);
		class_addattr(gObjectClass, attr);
		}

static void
ZippieTell(
	objZipf*	me,
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

static void ZippieInfo(objZipf* me)
	{ LitterInfo(kClassName, &me->coreObject, (method) ZippieTattle); }

static inline void AddInfo(void)
	{ LitterAddCant((method) ZippieInfo, "info"); }
		
void ZippieTell(objZipf* me, Symbol* iTarget, Symbol* iAttrName)
	{ LitterExpect((tExpectFunc) DoExpect, (Object*) me, iAttrName, iTarget, TRUE); }

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
	const tTypeList myArgTypes = {
						A_DEFFLOAT,			// Optional arguments:	1. rho
						A_DEFLONG,			// 						2. seed
											// If no seed specified, use global Taus88 data
						A_NOTHING
						};
	
	LITTER_CHECKTIMEOUT(kClassName);
	
	LitterSetupClass(	kClassName,
						sizeof(objZipf),			// Class object size
						LitterCalcOffset(objZipf),
						(method) ZippieNew,			// Instance creation function
						(method) ZippieFree,		// Custom deallocation function
						NIL,
						myArgTypes);	

	// Messages
	LITTER_TIMEBOMB LitterAddBang	((method) ZippieBang);
	LitterAddMess1	((method) ZippieRho,	"ft1",		A_FLOAT);
	LitterAddMess1	((method) ZippieSeed,	"seed",		A_DEFLONG);
	LitterAddMess2	((method) ZippieTell,	"tell"	,	A_SYM, A_SYM);
	LitterAddMess0	((method) ZippieTattle,	"tattle");
	LitterAddCant	((method) ZippieTattle,	"dblclick");
	LitterAddCant	((method) ZippieAssist,	"assist");
	
	AddInfo();
	
	//Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();
	
	}
