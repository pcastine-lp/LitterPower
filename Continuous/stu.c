/*
	File:		stu.c

	Contains:	Max external object generating a Guassian distribution.

	Written by:	Peter Castine

	Copyright:	© 2001-2002 Peter Castine

	Change History (most recent first):

         <8>   30Ð3Ð2006    pc      Update for new LitterLib organization, implement more efficient
                                    generator.
         <7>   23Ð3Ð2006    pc      Add support for expect message.
         <6>     15Ð1Ð04    pc      Avoid possible memory leak if seed argument is used.
         <5>     11Ð1Ð04    pc      Update for Windows.
         <4>    7Ð7Ð2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <3>  30Ð12Ð2002    pc      Drop faux 'Vers' resource and related modifications. Add object
                                    version to StuInfo().
         <2>  28Ð11Ð2002    pc      Tidy up after initial check-in.
         <1>  28Ð11Ð2002    pc      Initial check in.
		30-Jun-2001:	Modified to use Taus88 instead of TT800 as default generator.
						Also update to use new ULong2Unitxx() functions.
		14-Apr-2001:	Update to LPP overhaul
		2-Apr-2001:		First implementation.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark ¥ Include Files

#include "LitterLib.h"
#include "TrialPeriodUtils.h"
#include "RNGCauchy.h"
#include "MiscUtils.h"
#include "MoreMath.h"							// Needed for digamma(), #includes <math.h>


#pragma mark ¥ Constants

const char	kClassName[]	= "lp.stu";			// Class name

	// Indices for STR# resource
enum {
	strIndexInBang		= lpStrIndexLastStandard + 1,
	strIndexInDoF,
	
	strIndexTheOutlet,
	
	strIndexLeftInlet	= strIndexInBang,
	strIndexLeftOutlet	= strIndexTheOutlet
	};


#pragma mark ¥ Object Structure

typedef struct {
	LITTER_CORE_OBJECT(Object, coreObject);
	
	tTaus88DataPtr	tausData;
	
	long			dof;			// Degrees of Freedom	
	} objStu;




#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark ¥ Object Message Handlers

/******************************************************************************************
 *
 *	StuBang(me)
 *
 *	Degrees of Freedom == 1		Equivalent to standard Cauchy distribution
 *	Degrees of Freedom == 2		Kinderman & Monahan
 *	Degrees of Freedom >= 3		Best's T3T algorithm, with modifications by Dagpunar
 *
 ******************************************************************************************/
	
	static inline double CalcStu1(tTaus88Data* iData)
		{ return GenCauchyStdTaus88(iData) ; }
	
	static inline double CalcStu2(tTaus88Data* iData)
		{
		const double	c = -0.75,				// -(n+1)/4
						a = 5.42161202166,		// 4 * (1.5)^(3/4)
// ?? Unused??			f = 2.95115178587,		// 16 / a
						g = 0.8773826753;		// 2 * 3^(-0.75)
		
		double stu;
		UInt32 s1, s2, s3;
		
		if (iData == NIL)
			iData = Taus88GetGlobals();
		Taus88Load(iData, &s1, &s2,&s3);
		
		do  {
			const double	u1 = ULong2Unit_zo( Taus88Process(&s1, &s2, &s3) ),
							u2 = ULong2Unit_zo( Taus88Process(&s1, &s2, &s3) );
			
			double thresh, stsq;
			
			// Evaluate candidate variate stu = g * (2 * u2 - 1) / u1
			stu  = u2 + u2;
			stu -= 1.0;
			stu *= g;
			stu /= u1;
			
			// We need stu^2 for all the tests
			stsq = stu * stu;
			
			// First acceptance condition
			thresh = 5.0 - a * u1;
			if (stsq <= thresh) break;
			
			// Second acceptance condtion
			thresh  = 0.5 * stsq;
			thresh += 1.0;
			thresh  = pow(thresh, c);
			if (u1 <= thresh) break;
			
			// Otherwise try again
			} while (true);
		
		
		Taus88Store(iData, s1, s2, s3);
		
		return stu;
		}
	
	static inline double CalcStu3(tTaus88Data* iData)
		{
		const double kD1 = 0.86602540378;			// sqrt(3) / 2
		
		double	stu, t;
		UInt32	s1, s2, s3;
		
		if (iData == NIL)
			iData = Taus88GetGlobals();
		Taus88Load(iData, &s1, &s2,&s3);
		
		do	{
			const double	x = ULong2Unit_zo( Taus88Process(&s1, &s2, &s3) ),
							y = 2.0 * ULong2Unit_zo( Taus88Process(&s1, &s2, &s3) ) - 1.0;
			
			stu = kD1 * y / x;
			
			// Evaluate test condtion
			t  = stu * stu;
			t += 3.0;
			t *= x;
			} while (t > 3.0);
			
		Taus88Store(iData, s1, s2, s3);
		
		return stu;
		}

	static inline double CalcStu3Plus(long iDoF, tTaus88Data* iData)
		{
		const double	kD1 = 0.86602540378,			// sqrt(3) / 2
						kD2 = 8.59111293933,			// 256 * e / 81
						kD3 = 0.92740571477;			// 9 * sqrt(e) / 16
		
		double	stu,
				stsq,			// square of candidate variate (stu ^ 2)
				dof1,			// 1 / (dof + 1)
				thresh;		
		UInt32	s1, s2, s3;
		
		if (iData == NIL)
			iData = Taus88GetGlobals();
		Taus88Load(iData, &s1, &s2,&s3);
		
		do  {
			double u, w;
			
			do	{
				const double	x = ULong2Unit_zo( Taus88Process(&s1, &s2, &s3) ),
								y = 2.0 * ULong2Unit_zo( Taus88Process(&s1, &s2, &s3) ) - 1.0;
				
				stu = kD1 * y / x;
				
				// Evaluate test condtion. Use variable u as temporary register.
				stsq = stu * stu;
				u = stsq + 3.0;		// Use variable u as temeporary register for the
				u *= x;				// expression (x * (3 + t))
				} while (u > 3.0);
			
			u = ULong2Unit_zo( Taus88Process(&s1, &s2, &s3) );
			w = u * u;
			
			// First, fastest acceptance condition
			thresh  = kD3 * kD3;
			thresh -= stsq / kD2;
			if (thresh >= w) break;
			
			// Second acceptance condition
			thresh  = stsq / 3.0;
			thresh += 1.0;
			thresh *= thresh;
			thresh *= kD3;
			thresh /= u;
			if (stsq <= thresh) break;
			
			// Final acceptance condition
			dof1 = 1.0 / ((double) iDoF + 1.0);
			thresh -= 1.0;
			thresh *= dof1;
			} while (log((stsq + (double) iDoF) * dof1) > thresh);
		
		Taus88Store(iData, s1, s2, s3);
		
		return stu;
		}
		
static void
StuBang(
	objStu* me)
	
	{
	long	dof = me->dof;
	double	stu;
	
	// ASSERT: (dof > 0)
	switch (dof) {
		case 1:		stu = CalcStu1(me->tausData);			break;
		case 2:		stu = CalcStu2(me->tausData);			break;
		case 3:		stu = CalcStu3(me->tausData);			break;
		default:	stu = CalcStu3Plus(dof, me->tausData);	break;
		}
	
	outlet_float(me->coreObject.o_outlet, stu);
	}


/******************************************************************************************
 *
 *	StuDoF(me, iDoF)
 *	StuSeed(me, iSeed)
 *	
 *	Set parameters, making sure nothing bad will happen.
 *	
 ******************************************************************************************/

static void StuDoF(objStu* me, long iDoF)
	{ me->dof = (iDoF > 0) ? iDoF : 1; }
	
static void StuSeed(objStu* me, long iSeed)
	
	{ Taus88Seed(me->tausData, (unsigned long) iSeed); }

#pragma mark -
#pragma mark ¥ Attribute/Information Functions

/******************************************************************************************
 *
 *	StuTattle(me)
 *	StuAssist
 *
 ******************************************************************************************/

static void StuAssist(objStu* me, void* iBox, long iDir, long iArgNum, char* oCStr)
	{
	#pragma unused(me, iBox)
	
	 LitterAssist(iDir, iArgNum, strIndexLeftInlet, strIndexLeftOutlet, oCStr);
	 }

static void
StuTattle(
	objStu* me)
	
	{
	
	post("%s state", kClassName);
	post("  Degrees of freedom: %ld", me->dof);
	
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
	objStu*			me,
	eExpectSelector iSel)
	
	{
	const double kLGammaOneHalf = 0.5723649429;		// aka ln(sqrt(pi))
	
	double	result	= 0.0 / 0.0,		// Initially undefined
			dof;						// Only get Degrees of Freedeom when we need it
	
	switch(iSel) {
	case expMean:
		if (me->dof == 1)				// Mean is undefined!
			break;
		// ... otherwise fall into the next cases
	case expMedian:
	case expMode:
		result = 0.0;
		break;
	case expVar:
	case expStdDev:
		if ((dof = me->dof) > 2.0) {
			result = dof / (dof - 2.0);
			if (iSel == expStdDev) result = sqrt(result);
			}
		break;
	case expSkew:
		if (me->dof > 2)
			result = 0.0;
		break;
	case expKurtosis:
		if ((dof = me->dof - 4) > 0.0)
			result = 6.0 / dof;
		break;
	case expMin:
		result = -1.0 / 0.0;
		break;
	case expMax:
		result = 1.0 / 0.0;
		break;
	case expEntropy:
		dof = 0.5 * (double) me->dof;		// Here we want dof/2
		
		result  = digamma(dof + 0.5) - digamma(dof);
		result *= dof + 0.5;
			// Faster to add two floats than to convert again from int:
		result += log(sqrt(dof + dof));
			// Instead of adding beta(dof, 0.5), call the relevant lgamma() functions
			// directly... beta() is just exp() wrapped around some lgamma() calls
		result += lgamma(dof);
		result += kLGammaOneHalf;
		result -= lgamma(dof + 0.5);
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

	static t_max_err StuGetMin(objStu* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMin), ioArgC, ioArgV); }
	static t_max_err StuGetMax(objStu* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMax), ioArgC, ioArgV); }
	static t_max_err StuGetMean(objStu* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMean), ioArgC, ioArgV); }
	static t_max_err StuGetMedian(objStu* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMedian), ioArgC, ioArgV); }
	static t_max_err StuGetMode(objStu* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMode), ioArgC, ioArgV); }
	static t_max_err StuGetVar(objStu* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expVar), ioArgC, ioArgV); }
	static t_max_err StuGetStdDev(objStu* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expStdDev), ioArgC, ioArgV); }
	static t_max_err StuGetSkew(objStu* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expSkew), ioArgC, ioArgV); }
	static t_max_err StuGetKurtosis(objStu* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expKurtosis), ioArgC, ioArgV); }
	static t_max_err StuGetEntropy(objStu* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expEntropy), ioArgC, ioArgV); }
	
	static t_max_err StuSetAttrDoF(objStu* me, void* iAttr, long iArgC, Atom iArgV[])
		{
		
		if (iArgC > 0 && iArgV != NIL)
			StuDoF(me, AtomGetLong(iArgV));
		
		return MAX_ERR_NONE;
		}
	
	static inline void
	AddInfo(void)
		{
		Object*	attr;
		Symbol*	symFloat64		= gensym("float64");
		
		// Read-Write Attributes
		attr = attr_offset_new(	"dof", gensym("long"), 0,
								NULL, (method) StuSetAttrDoF,
								calcoffset(objStu, dof));
		class_addattr(gObjectClass, attr);
		
		// Read-Only Attributes
		attr = attribute_new("min", symFloat64, kAttrFlagsReadOnly, (method) StuGetMin, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("max", symFloat64, kAttrFlagsReadOnly, (method) StuGetMax, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("mean", symFloat64, kAttrFlagsReadOnly, (method) StuGetMean, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("median", symFloat64, kAttrFlagsReadOnly, (method) StuGetMedian, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("mode", symFloat64, kAttrFlagsReadOnly, (method) StuGetMode, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("var", symFloat64, kAttrFlagsReadOnly, (method) StuGetVar, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("stddev", symFloat64, kAttrFlagsReadOnly, (method) StuGetStdDev, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("skew", symFloat64, kAttrFlagsReadOnly, (method) StuGetSkew, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("kurtosis", symFloat64, kAttrFlagsReadOnly, (method) StuGetKurtosis, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("entropy", symFloat64, kAttrFlagsReadOnly, (method) StuGetEntropy, NULL);
		class_addattr(gObjectClass, attr);
		}

static void
StuTell(
	objStu*	me,
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

static void StuInfo(objStu* me)
	{ LitterInfo(kClassName, &me->coreObject, (method) StuTattle); }

static inline void AddInfo(void)
	{ LitterAddCant((method) StuInfo, "info"); }

static void StuTell(objStu* me, Symbol* iTarget, Symbol* iMsg)
	{ LitterExpect((tExpectFunc) DoExpect, (Object*) me, iMsg, iTarget, TRUE); }

#endif


#pragma mark -
#pragma mark ¥ Class Message Handlers

/******************************************************************************************
 *
 *	StuFree(me)
 *	StuNew(iOracle)
 *
 ******************************************************************************************/

static void StuFree(objStu* me)
	{ Taus88Free(me->tausData); }							// Taus88Free is NIL-safe

static void*
StuNew(
	long	iDoF,
	long	iSeed)
	
	{
	const long	kDefDoF	= 1;
	
	objStu*			me			= NIL;
	tTaus88DataPtr	myTausStuff	= NIL;
	
	// Run through initialization parameters from right to left, handling defaults
	if (iSeed == 0) Taus88Init();
	else {
		myTausStuff = Taus88New(iSeed);
		goto noMoreDefaults;
		}
	
	if (iDoF == 0)
		iDoF = kDefDoF;
noMoreDefaults:
	// Finished checking intialization parameters

	// Let Max allocate us, our inlets, and outlets
	me = (objStu*) LitterAllocateObject();
	
	intin(me, 1);												// Degrees of Freedom inlet
	
	floatout(me);								// Access through me->coreObject.o_outlet
	
	// Store object components
	me->tausData = myTausStuff;
	StuDoF(me, iDoF);
	
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
						A_DEFLONG,		// Optional arguments:	1. Degrees of Freedom
						A_DEFLONG,		// 						2. seed
						A_NOTHING
						};
	
	LITTER_CHECKTIMEOUT(kClassName);
	
	// Standard Max setup() call
	LitterSetupClass(	kClassName,
						sizeof(objStu),
						LitterCalcOffset(objStu),	// Class object size
						(method) StuNew,			// Instance creation function
						(method) StuFree,			// Custom deallocation function
						NIL,						// No menu function
						myArgTypes);				
	

	// Messages
	LITTER_TIMEBOMB LitterAddBang((method) StuBang);
	LitterAddMess1	((method) StuDoF,		"in1",		A_LONG);
	LitterAddMess1 ((method) StuSeed,		"seed",		A_DEFLONG);
	LitterAddMess2 ((method) StuTell,		"tell", A_SYM, A_SYM);
	LitterAddCant	((method) StuTattle,	"dblclick");
	LitterAddMess0	((method) StuTattle,	"tattle");
	LitterAddCant	((method) StuAssist,	"assist");
	
	AddInfo();

	
	// Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();
	
	}

