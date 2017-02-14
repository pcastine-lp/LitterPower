/*
	File:		norm.c

	Contains:	Max external object generating a Guassian distribution.

	Written by:	Peter Castine

	Copyright:	© 2001-2002 Peter Castine

	Change History (most recent first):

         <8>   24Ð3Ð2006    pc      Further updates for new LitterLib organization.
         <7>   23Ð3Ð2006    pc      Add support for expect message. Added faster RNG algorithms.
         <6>     15Ð1Ð04    pc      Avoid possible memory leak if seed argument is used.
         <5>     11Ð1Ð04    pc      Update for Windows.
         <4>    7Ð7Ð2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <3>  30Ð12Ð2002    pc      Drop faux 'Vers' resource and related modifications. Add object
                                    version to DoInfo().
         <2>  28Ð11Ð2002    pc      Tidy up after initial check-in.
         <1>  28Ð11Ð2002    pc      Initial check in.
		1-Jul-2001:		Modified to use Taus88 instead of TT800 as default generator.
		14-Apr-2001:	Overhaul for LPP
		2-Apr-2001:		First implementation.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark ¥ Include Files

#include "LitterLib.h"
#include "TrialPeriodUtils.h"
#include "Taus88.h"
#include "RNGGauss.h"
#include "MiscUtils.h"


#pragma mark ¥ Constants

const char	kClassName[]		= "lp.norm";			// Class name


	// Indices for STR# resource
enum {
	strIndexInBang		= lpStrIndexLastStandard + 1,
	strIndexInMean,
	strIndexInStdDev,
	
	strIndexTheOutlet,
	
	strIndexLeftInlet	= strIndexInBang,
	strIndexLeftOutlet	= strIndexTheOutlet
	};


#pragma mark ¥ Object Structure

typedef struct {
	LITTER_CORE_OBJECT(Object, coreObject);
	
	tTaus88DataPtr	tausData;
	
	double			mean,
					stdDev;
	
	} objGauss;



#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark ¥ Object Message Handlers

/******************************************************************************************
 *
 *	NormBang(me)
 *
 ******************************************************************************************/

static void
NormBang(
	objGauss* me)
	
	{
	double g = NormalKRTaus88(me->tausData);
	
	g *= me->stdDev;
	g += me->mean;
	
	outlet_float(me->coreObject.o_outlet, g);
	}


/******************************************************************************************
 *
 *	NormMean(me, iOrder)
 *	NormStdDev(me, iStdDev)
 *	NormSeed(me, iSeed)
 *	
 *	Set parameter. Nothing really bad can happen.
 *	
 ******************************************************************************************/

static void NormMean(objGauss* me, double iMean)
	{ me->mean = iMean; }
	
static void NormStdDev(objGauss* me, double iStdDev)
	{
	
	me->stdDev = iStdDev;
	
	if (iStdDev < 0.0) {
		const	UInt32	kOneMinute	= 3600;				// Time in ticks
		static	UInt32	sLastTime	= 0;
		
		UInt32 now = TickCount();
		
		if (sLastTime == 0 || now > sLastTime + kOneMinute) {
			post("¥ warning: %s: negative standard deviation is deprecated", kClassName);
			sLastTime = now;
			}
		}
	
	}
	
static void NormSeed(objGauss* me, long iSeed)
	{ Taus88Seed(me->tausData, (unsigned long) iSeed); }
	

#pragma mark -
#pragma mark ¥ Attribute/Information Functions

/******************************************************************************************
 *
 *	NormAssist(me, iBox, iDir, iArgNum, oCStr)
 *	NormTattle(me)
 *
 ******************************************************************************************/

static void NormAssist(objGauss* me, void* box, long iDir, long iArgNum, char* oCStr)
	{
	#pragma unused(me, box)
	
	LitterAssist(iDir, iArgNum, strIndexLeftInlet, strIndexLeftOutlet, oCStr);
	}

static void
NormTattle(
	objGauss* me)
	
	{
	
	post("%s state", kClassName);
	post("  mu (mean): %lf", me->mean);
	post("  sigma (std. dev.): %lf", me->stdDev);
	
	}


/******************************************************************************************
 *
 *	DoExpect()
 *
 *	Helper function for calculating expected values (mean, standard deviation, etc.)
 *	
 ******************************************************************************************/

static double
DoExpect(objGauss* me, eExpectSelector iSel)
		{
		const double kRoot2PiE	= 4.1327313541;
		
		double	result	= 0.0 / 0.0;
		
		switch(iSel) {
		case expMean:
		case expMedian:
		case expMode:
			result = me->mean;
			break;
		case expVar:
			result  = me->stdDev;
			result *= result;
			break;
		case expStdDev:
			result = me->stdDev;
			break;
		case expSkew:
		case expKurtosis:
			result = 0.0;
			break;
		case expMin:
			result = -1.0 / 0.0;
			break;
		case expMax:
			result = 1.0 / 0.0;
			break;
		case expEntropy:
			result = log(me->stdDev * kRoot2PiE);
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

	static t_max_err NormGetMin(objGauss* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMin), ioArgC, ioArgV); }
	static t_max_err NormGetMax(objGauss* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMax), ioArgC, ioArgV); }
	static t_max_err NormGetMedian(objGauss* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMedian), ioArgC, ioArgV); }
	static t_max_err NormGetMode(objGauss* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expMode), ioArgC, ioArgV); }
	static t_max_err NormGetVar(objGauss* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expVar), ioArgC, ioArgV); }
	static t_max_err NormGetSkew(objGauss* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expSkew), ioArgC, ioArgV); }
	static t_max_err NormGetKurtosis(objGauss* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expKurtosis), ioArgC, ioArgV); }
	static t_max_err NormGetEntropy(objGauss* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ return LitterGetAttrFloat(DoExpect(me, expEntropy), ioArgC, ioArgV); }
	
	static inline void
	AddInfo(void)
		{
		Object*	attr;
		Symbol*	symFloat64		= gensym("float64");
		
		// Read-Write Attributes
		attr = attr_offset_new(	"mean", symFloat64, 0,
								NULL, NULL, calcoffset(objGauss, mean));
		class_addattr(gObjectClass, attr);
		attr = attr_offset_new(	"stddev", symFloat64, 0,
								NULL, NULL, calcoffset(objGauss, stdDev));
		class_addattr(gObjectClass, attr);
		
		// Read-Only Attributes
		attr = attribute_new("min", symFloat64, kAttrFlagsReadOnly, (method) NormGetMin, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("max", symFloat64, kAttrFlagsReadOnly, (method) NormGetMax, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("median", symFloat64, kAttrFlagsReadOnly, (method) NormGetMedian, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("mode", symFloat64, kAttrFlagsReadOnly, (method) NormGetMode, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("var", symFloat64, kAttrFlagsReadOnly, (method) NormGetVar, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("skew", symFloat64, kAttrFlagsReadOnly, (method) NormGetSkew, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("kurtosis", symFloat64, kAttrFlagsReadOnly, (method) NormGetKurtosis, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("entropy", symFloat64, kAttrFlagsReadOnly, (method) NormGetEntropy, NULL);
		class_addattr(gObjectClass, attr);
		}

static void
NormTell(
	objGauss*	me,
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

static void NormInfo(objGauss* me)
	{ LitterInfo(kClassName, &me->coreObject, (method) NormTattle); }

static inline void AddInfo(void)
	{ LitterAddCant((method) NormInfo, "info"); }
		
static void NormTell(objGauss* me, Symbol* iTarget, Symbol* iAttrName)
	{ LitterExpect((tExpectFunc) DoExpect, (Object*) me, iAttrName, iTarget, TRUE); }

#endif


#pragma mark -
#pragma mark ¥ Class Message Handlers

/******************************************************************************************
 *
 *	NormFree(me)
 *	NormNew(iOracle)
 *
 ******************************************************************************************/

static void NormFree(objGauss* me)
	{ Taus88Free(me->tausData); }							// Taus88Free is NIL-safe

static void*
NormNew(
	Symbol*	iClassName,
	short	iArgCount,
	Atom	iArgVec[])
	
	{
	#pragma unused(iClassName)
	
	objGauss*		me			= NIL;
	
	// Let Max allocate us, our inlets (from right to left), and outlets
	me = (objGauss*) LitterAllocateObject();
	
	floatin(me, 2);
	floatin(me, 1);
	
	floatout(me);								// Access through me->coreObject.o_outlet
	
	// Set up defaults
	me->tausData	= NIL;
	me->mean		= 0.0;
	me->stdDev		= 1.0;

	// Run through initialization parameters from right to left
	switch (iArgCount) {
	default:
		// Too many arguments
		error("%s: spurious arguments ignored", kClassName);
		// fall into next case...
	case 3:
		me->tausData = Taus88New( AtomGetLong(&iArgVec[2]) );
		if (me->tausData == NIL) {
			// Highly unlikely
			error("%s: insufficient memory for separate seed pool, using global seeds",
					kClassName);
			}
		// fall into next case...
	case 2:
		if (iArgVec[1].a_type != A_SYM)
			NormStdDev(me, AtomGetFloat(&iArgVec[1]));
		// fall into next case...
	case 1:
		NormMean(me, AtomGetFloat(&iArgVec[0]));
		// fall into next case...
	case 0:
		// Nothing to do
		break;
		}
	
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
						A_GIMME,				// Parse arguments ourself
						A_NOTHING
						};
	
	LITTER_CHECKTIMEOUT(kClassName);
	
	// Standard Max setup() call
	LitterSetupClass(	kClassName,
						sizeof(objGauss),						// Class object size
						LitterCalcOffset(objGauss),
						(method) NormNew,		// Instance creation function
						(method) NormFree,		// Custom deallocation function
						NIL,					// No menu function
						myArgTypes);				
	
	// Messages
	LITTER_TIMEBOMB LitterAddBang((method) NormBang);
	LitterAddMess1	((method) NormMean,		"ft1",	A_LONG);
	LitterAddMess1	((method) NormStdDev,	"ft2",	A_LONG);
	LitterAddMess1	((method) NormSeed,		"seed",	A_DEFLONG);
	LitterAddMess2	((method) NormTell,		"tell", A_SYM, A_SYM);
	LitterAddMess0	((method) NormTattle,	"tattle");
	LitterAddCant	((method) NormTattle,	"dblclick");
	LitterAddCant	((method) NormAssist,	"assist");
	
	AddInfo();
	
	// Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();
	
	}



