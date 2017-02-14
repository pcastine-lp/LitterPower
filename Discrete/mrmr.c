/*
	File:		mrmr.c

	Contains:	Max external object implementing the Mrmr Twister pseudo-random number
				generator.

	Written by:	Peter Castine

	Copyright:	© 2004-08 Peter Castine

	Change History (most recent first):

         <2>     15Ð3Ð06    pc      Add expect message
         <1>     13Ð8Ð04    pc      First implementation.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark ¥ Include Files

#include "LitterLib.h"
#include "TrialPeriodUtils.h"
#include "MiscUtils.h"
#include "UniformExpectations.h"


#pragma mark ¥ Constants

const char*	kClassName		= "lp.mrmr";			// Class name


	// Indices for STR# resource
enum {
	strIndexBang		= lpStrIndexLastStandard + 1,
	strIndexMin,
	strIndexMax,
	
	strIndexTheOutlet,
	
	strIndexFirstInlet	= strIndexBang,
	strIndexFirstOutlet	= strIndexTheOutlet
	};

	// Constants for Mrmr Twister algorighm
enum {
	kArraySize		= 624,
	kArrayBreakPt	= 397
	};

#pragma mark ¥ Type Definitions



#pragma mark ¥ Object Structure

typedef struct {
	LITTER_CORE_OBJECT(Object, coreObject);
	
	long			min,
					max;
	} objMersenne;


#pragma mark ¥ Global Variables

static UInt32	gMTData[kArraySize];		// Inited at run time
static int		gMTIndex = 0;


#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark ¥ Class Message Handlers

/******************************************************************************************
 *
 *	MrmrInit(iSeed)
 *
 ******************************************************************************************/

static void
MrmrInit(
	UInt32 iSeed)

	{
	const UInt32 kKnuthMult	= 1812433253UL;
	
	UInt32	prevSeed;								// Cache this in a register
	int		i;
   
	prevSeed = gMTData[0] = iSeed;
	
	for (i = 1; i < kArraySize; i += 1)
		prevSeed = gMTData[i] = (kKnuthMult * (prevSeed ^ (prevSeed >> 30)) + i); 
	
	}

/******************************************************************************************
 *
 *	MrmrRand()
 *
 ******************************************************************************************/

static UInt32
MrmrRand(void)

	{
	const UInt32 kMerMagic[2] = {0, 0x9908b0df};
	
	UInt32	result;
	
	if (gMTIndex >= kArraySize) {
		// Generate new set of seeds
		const UInt32	kHighBit = 0x80000000,
						kLowBits = 0x7fffffff,
						kZeroBit = 0x00000001;
		
		UInt32	y;
		int 	i;
		
		for (i = 0; i < kArraySize - kArrayBreakPt; i += 1) {
			y = (gMTData[i] & kHighBit) | (gMTData[i + 1] & kLowBits);
			gMTData[i] = gMTData[i + kArrayBreakPt] ^ (y >> 1) ^ kMerMagic[y & kZeroBit];
			}
		for ( ; i < kArraySize - 1; i += 1) {
			y = (gMTData[i] & kHighBit) | (gMTData[i + 1] & kLowBits);
			gMTData[i] = gMTData[i + kArrayBreakPt - kArraySize] ^ (y >> 1) ^ kMerMagic[y & kZeroBit];
			}
		y = (gMTData[kArraySize - 1] & kHighBit) | (gMTData[0] & kLowBits);
		gMTData[kArraySize - 1] = gMTData[kArrayBreakPt - 1] ^ (y >> 1) ^ kMerMagic[y & kZeroBit];
		
		gMTIndex = 0;
		}
	
	result = gMTData[gMTIndex++];
	
	// "Temper" the result
    result ^= (result >> 11);
    result ^= (result << 7) & 0x9d2c5680UL;
    result ^= (result << 15) & 0xefc60000UL;
    result ^= (result >> 18);

    return result;
	}

/******************************************************************************************
 *
 *	MrmrNew(iMin, iMax, iSeed)
 *
 ******************************************************************************************/

static void*
MrmrNew(
	long	iMin,
	long	iMax,
	long	iSeed)
	
	{
	objMersenne*		me			= NIL;
	
	// Run through initialization parameters from right to left, handling defaults
	if (iSeed != 0) {
		MrmrInit(iSeed);
		goto noMoreDefaults;
		}
	
	if (iMax == 0) {
		if (iMin > 0) {
			iMax = iMin;
			iMin = 0;
			}
		else if (iMin == 0) {
			iMin = kLongMin;
			iMax = kLongMax;
			}
		}
noMoreDefaults:
	// Finished checking intialization parameters
	
	// Let Max allocate us, our inlets, and outlets
	me = (objMersenne*) LitterAllocateObject();

	
	intin(me, 2);										// Max inlet
	intin(me, 1);										// Min inlet
	
	intout(me);					// Access main outlet through me->coreObject.o_outlet;
	
	// Initialize and store object components
	me->min	= iMin;
	me->max	= iMax;
	
	return me;
	}

#pragma mark -
#pragma mark ¥ Object Message Handlers

/******************************************************************************************
 *
 *	MrmrBang(me)
 *
 *	Grab a new random number from Mrmr. Scale if necessary.
 *
 ******************************************************************************************/

static void
MrmrBang(
	objMersenne* me)
	
	{
	long	mt,
			min		= me->min,
			max		= me->max;
	
	if (min < max) {
		double	scale = (double) max - (double) min + 1.0;
		
		mt = MrmrRand();
		mt = scale * ULong2Unit_Zo(mt) + min;
		}
	else mt = (min == max) ? min : 0;
	
	outlet_int(me->coreObject.o_outlet, mt);
	
	}


/******************************************************************************************
 *
 *	MrmrMin(me, iMin)
 *	MrmrMax(me, iMax)
 *	MrmrSeed(me, iSeed)
 *
 ******************************************************************************************/

static void MrmrMin(objMersenne* me, long iMin)	{ me->min = iMin; }
static void MrmrMax(objMersenne* me, long iMax)	{ me->max = iMax; }

static void MrmrSeed(objMersenne* me, long iSeed)
	{
	#pragma unused(me)
	
	MrmrInit((UInt32) iSeed);
	}


#pragma mark -
#pragma mark ¥ Attribute/Information Functions

/******************************************************************************************
 *
 *	MrmrAssist
 *	MrmrTattle(me)
 *
 ******************************************************************************************/

static void
MrmrAssist(
	objMersenne*	me,
	void*			iBox,
	long			iDir,
	long			iArgNum,
	char*			oCStr)
	
	{
	#pragma unused(iBox)
	
	long	min = me->min,
			max = me->max;
	
	if (min >= max) {
		min = kLongMin;
		max = kLongMax;
		}
	
	LitterAssistVA(iDir, iArgNum, strIndexFirstInlet, strIndexFirstOutlet, oCStr, min, max);
	
	}

static void
MrmrTattle(
	objMersenne* me)
	
	{
	
	post("%s state",
			kClassName);
	post("  Range: [%ld .. %ld]",
			me->min, me->max);
	if (me->min > me->max)
		post("  NB: Ignoring range while min > max");
	
	}


/******************************************************************************************
 *
 *	DoExpect()
 *
 *	Helper function for calculating expected values (mean, standard deviation, etc.) for
 *	Mrmr
 *	
 ******************************************************************************************/

static double
DoExpect(
	objMersenne*	me,
	eExpectSelector iSel)
	{
	return UniformExpectationsDiscrete(me->min, me->max, iSel);
	}
	

/******************************************************************************************
 *
 *	Attribute Funtions
 *
 *	Functions for Inspector/Get Info... dialog
 *	
 ******************************************************************************************/

#if LITTER_USE_OBEX

	static t_max_err MrmrGetMean(objMersenne* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expMean), ioArgC, ioArgV);
		}
	static t_max_err MrmrGetMedian(objMersenne* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expMedian), ioArgC, ioArgV);
		}
	static t_max_err MrmrGetMode(objMersenne* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expMode), ioArgC, ioArgV);
		}
	static t_max_err MrmrGetVar(objMersenne* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expVar), ioArgC, ioArgV);
		}
	static t_max_err MrmrGetStdDev(objMersenne* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expStdDev), ioArgC, ioArgV);
		}
	static t_max_err MrmrGetSkew(objMersenne* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expSkew), ioArgC, ioArgV);
		}
	static t_max_err MrmrGetKurtosis(objMersenne* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expKurtosis), ioArgC, ioArgV);
		}
	static t_max_err MrmrGetEntropy(objMersenne* me, void* iAttr, long* ioArgC, Atom** ioArgV)
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
		attr = attr_offset_new("min", symLong, 0, NULL, NULL, calcoffset(objMersenne, min));
		class_addattr(gObjectClass, attr);
		attr = attr_offset_new("max", symLong, 0, NULL, NULL, calcoffset(objMersenne, max));
		class_addattr(gObjectClass, attr);
		
		// Read-Only Attributes
		attr = attribute_new("mean", symFloat64, kAttrFlagsReadOnly, (method) MrmrGetMean, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("median", symFloat64, kAttrFlagsReadOnly, (method) MrmrGetMedian, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("mode", symFloat64, kAttrFlagsReadOnly, (method) MrmrGetMode, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("var", symFloat64, kAttrFlagsReadOnly, (method) MrmrGetVar, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("stddev", symFloat64, kAttrFlagsReadOnly, (method) MrmrGetStdDev, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("skew", symFloat64, kAttrFlagsReadOnly, (method) MrmrGetSkew, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("kurtosis", symFloat64, kAttrFlagsReadOnly, (method) MrmrGetKurtosis, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("entropy", symFloat64, kAttrFlagsReadOnly, (method) MrmrGetEntropy, NULL);
		class_addattr(gObjectClass, attr);
		}

static void
MrmrTell(
	objMersenne*	me,
	Symbol*			iTarget,
	Symbol*			iAttrName)
	
	{
	long	argC = 0;
	Atom*	argV = NIL;
		
	if (object_attr_getvalueof(me, iAttrName, &argC, &argV) == MAX_ERR_NONE) {
		ForwardAnything(iTarget, iAttrName, argC, argV);
		freebytes(argV, argC * sizeof(Atom));	// ASSERT (argC > 0 && argV != NIL)
		}
	}

#else

static void MrmrInfo(objMersenne* me)
	{ LitterInfo(kClassName, &me->coreObject, (method) MrmrTattle); }

static inline void AddInfo(void)
	{ LitterAddCant((method) MrmrInfo, "info"); }
		
static void MrmrTell(objMersenne* me, Symbol* iTarget, Symbol* iAttrName)
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
						A_DEFLONG,		// Minimum	(Default: kMinLong)
						A_DEFLONG,		// Maximum	(Default: kMaxLong)
						A_DEFLONG,		// Seed		(Default: Zero -- autoseed)
						A_NOTHING
						};
	
	LITTER_CHECKTIMEOUT(kClassName);
	
	// Standard Max setup() call
	LitterSetupClass(	kClassName,
						sizeof(objMersenne),			// Class object size
						LitterCalcOffset(objMersenne),	// Magic "Obex" Calculation
						(method) MrmrNew,				// Instance creation function
						NIL,							// No custom deallocation function
						NIL,							// No menu function
						myArgTypes);					// See above
	

	// Messages
	LITTER_TIMEBOMB LitterAddBang	((method) MrmrBang);
	LitterAddMess1	((method) MrmrMin,		"in1",		A_LONG);
	LitterAddMess1	((method) MrmrMax,		"in2",		A_LONG);
	LitterAddMess1	((method) MrmrSeed,		"seed",		A_DEFLONG);
	LitterAddMess2	((method) MrmrTell,		"tell",		A_SYM, A_SYM);
	LitterAddMess0	((method) MrmrTattle,	"tattle");
	LitterAddCant	((method) MrmrTattle,	"dblclick");
	LitterAddCant	((method) MrmrAssist,	"assist");
	
	AddInfo();
	
	//Initialize Litter Library
	LitterInit(kClassName, 0);
	MrmrInit( MachineKharma() );
	
	}

