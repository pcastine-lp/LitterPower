/*
	File:		mama.c

	Contains:	Max external object implementing the Marsaliga "Mama of All Random Number
				Generators."

	Written by:	Peter Castine

	Copyright:	© 2006 Peter Castine

	Change History (most recent first):

         <5>     15Ð3Ð06    pc      Add expect message
         <4>     10Ð2Ð06    pc      Fix bug in MamaRand() that is causing sequence to converge.
         <3>   10Ð2Ð2006    pc      Fix braindead intialization error in MamaNew(). Good thing we
                                    tested this on Max 4.1.
         <2>     10Ð2Ð06    pc      Shaved a few more cycles off the MamaRand() function.
                                    Incidentally, this might be a candidate for vectorizing.
         <1>     10Ð2Ð06    pc      first checked in.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark ¥ Include Files

#include "LitterLib.h"
#include "TrialPeriodUtils.h"
#include "MiscUtils.h"
#include "UniformExpectations.h"


#pragma mark ¥ Constants

const char*	kClassName		= "lp.mama";			// Class name


	// Indices for STR# resource
enum {
	strIndexBang		= lpStrIndexLastStandard + 1,
	strIndexMin,
	strIndexMax,
	
	strIndexTheOutlet,
	
	strIndexFirstInlet	= strIndexBang,
	strIndexFirstOutlet	= strIndexTheOutlet
	};
	

	// Use enum rather than const int for compatibility across compilers
enum {
	kArraySize = 10
	};



#pragma mark ¥ Object Structure

typedef struct {
	LITTER_CORE_OBJECT(Object, coreObject);
	
	UInt16*			pools;
	long			min,
					max;
	} objMama;


#pragma mark ¥ Global Variables

static UInt16	gPool1[kArraySize],		// Inited at run time
				gPool2[kArraySize];


#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark ¥ Helper Functions

/******************************************************************************************
 *
 *	MamaInit(iSeed)
 *
 ******************************************************************************************/

static void
MamaInit(
	UInt32	iSeed,
	UInt16	oPool1[],
	UInt16	oPool2[])

	{
	const	UInt32 kGMMagic = 30903;	// George Marsaglia's magic number
	
	UInt32	lowBits,					// All the machines we're building for have 32 bit registers,
			number;						// so no point in asking for anything less
	int		i;
			
	if (iSeed == 0)
		iSeed = MachineKharma();
	
	lowBits = iSeed & 0x0000ffff;					// Only want 16 bits
	number	= iSeed & 0x7fffffff;					// Only want 31 bits

	// Initialize seed pools with a stripped-down version of the carry-and-multiply approach
	// Note that at this point we do *not* initialize the last element in the array because
	// it will be overwritten before we use it.
	for (i = 0; i < kArraySize - 1; i += 1) {
		number		= kGMMagic * lowBits + (number >> 16);
		lowBits		= number & 0x0000ffff;
		oPool1[i]	= lowBits;
		}
	for (i = 0; i < kArraySize - 1; i += 1) {
		number		= kGMMagic * lowBits + (number >> 16);
		lowBits		= number & 0x0000ffff;
		oPool2[i]	= lowBits;
		}
	
	// Strip first to entries in the seed pools to just the low-order 15 bits
	oPool1[0] &= 0x00007fff;
	oPool2[0] &= 0x00007fff;
	
	}

/******************************************************************************************
 *
 *	MamaRand()
 *
 ******************************************************************************************/

	static inline UInt16 FastHiWord(UInt32 x) { return x >> 16; }
	static inline UInt16 FastLoWord(UInt32 x) { return x; }
	
	// This is faster than using BlockMoveData() or memmove()
	// Funnily enough, after unwrapping the loop, I tried several variants of the following
	// using more "typically C" constructs with pointer arithmetic, but the CodeWarrior
	// compiler always ended up generating (at best) the same code as it does for the 
	// more "amateurish" (Pascal-like?) code that follows. Go figure.
	static inline void UpdatePool(UInt16 ioPool[], UInt32 iCarry)
		{
		ioPool[8] = ioPool[7];
		ioPool[7] = ioPool[6];
		ioPool[6] = ioPool[5];
		ioPool[5] = ioPool[4];
		ioPool[4] = ioPool[3];
		ioPool[3] = ioPool[2];
		ioPool[2] = ioPool[1];
		ioPool[1] = FastLoWord(iCarry);
		ioPool[0] = FastHiWord(iCarry);
		}

static UInt32
MamaRand(
	objMama* me)

	{
	UInt32	carry1,
			carry2;
	UInt16	*pool1,
			*pool2;
	
	if (me->pools == NULL) {
		pool1 = gPool1;
		pool2 = gPool2;
		}
	else {
		pool1 = me->pools;
		pool2 = me->pools + kArraySize;
		}

	
	// Munge bits around, update pool sets
	carry1 = pool1[0] + 1949 * pool1[1]			// Marsaglia used:	1941
					  + 1861 * pool1[2]			//					1860
					  + 1831 * pool1[3]			//					1812
					  + 1777 * pool1[4]			//					1776
					  + 1493 * pool1[5]			//					1492
					  + 1217 * pool1[6]			//					1215
					  + 1069 * pool1[7]			//					1066
					  + 1021 * pool1[8];		//					12013
	UpdatePool(pool1, carry1);
	carry2 = pool2[0] + 1117 * pool2[1]			//					1111
					  + 2237 * pool2[2]			//					2222
					  + 3343 * pool2[3]			//					etc.
					  +  351 * pool2[4]
					  + 1461 * pool2[5]
					  + 2577 * pool2[6]
					  + 3693 * pool2[7]			//					7777
					  + 1085 * pool2[8];		//					9272
	UpdatePool(pool2, carry2);

	// Combine the two 16 bit random numbers into one 32 bit word
	// Don't worry about the fact that we appear to be taking the low word from the
	// carry2 twice. FastLoWord(0 is basically just syntactic sugar.
	return (carry1 << 16) + FastLoWord(carry2);
	}


#pragma mark -
#pragma mark ¥ Class Message Handlers

/******************************************************************************************
 *
 *	MamaNew(iMin, iMax, iSeed)
 *
 ******************************************************************************************/

static void*
MamaNew(
	long	iMin,
	long	iMax,
	long	iSeed)
	
	{
	objMama*	me		= NIL;
	UInt16*		myPool	= NIL;
	
	// Run through initialization parameters from right to left, handling defaults
	if (iSeed != 0) {
		myPool = (UInt16*) NewPtr(2 * kArraySize * sizeof(UInt16));
		if (myPool != NIL)
			 MamaInit(iSeed, myPool, myPool + kArraySize);
		else error("%s: could not allocate memory for private seed pool", kClassName);
			
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
	me = (objMama*) LitterAllocateObject();
	
	intin(me, 2);										// Max inlet
	intin(me, 1);										// Min inlet
	
	intout(me);					// Access main outlet through me->coreObject.o_outlet;
	
	// Store object components
	me->pools	= myPool;
	me->min		= iMin;
	me->max		= iMax;
	
	return me;
	}

#pragma mark -
#pragma mark ¥ Object Message Handlers

/******************************************************************************************
 *
 *	MamaBang(me)
 *
 *	Grab a new random number from Taus88. Scale if necessary.
 *
 ******************************************************************************************/

static void
MamaBang(
	objMama* me)
	
	{
	long	mama,
			min		= me->min,
			max		= me->max;
	
	if (min < max) {
		double	scale = (double) max - (double) min + 1.0;
		
		mama = MamaRand(me);
		mama = scale * ULong2Unit_Zo(mama) + min;
		}
	else mama = (min == max) ? min : 0;
	
	outlet_int(me->coreObject.o_outlet, mama);
	
	}


/******************************************************************************************
 *
 *	MamaMin(me, iMin)
 *	MamaMax(me, iMax)
 *	MamaSeed(me, iSeed)
 *
 ******************************************************************************************/

static void MamaMin(objMama* me, long iMin)	{ me->min = iMin; }
static void MamaMax(objMama* me, long iMax)	{ me->max = iMax; }

static void MamaSeed(objMama* me, long iSeed)
	{
	if (me->pools != NIL)	MamaInit(iSeed, me->pools, me->pools + kArraySize);
	else					MamaInit(iSeed, gPool1, gPool2);
	}


#pragma mark -
#pragma mark ¥ Attribute/Information Functions

/******************************************************************************************
 *
 *	MamaAssist
 *	MamaTattle(me)
 *
 ******************************************************************************************/

static void
MamaAssist(
	objMama*	me,
	void*		iBox,
	long		iDir,
	long		iArgNum,
	char*		oCStr)
	
	{
	#pragma unused (iBox)
	
	long	min = me->min,
			max = me->max;
	
	if (min >= max) {
		min = kLongMin;
		max = kLongMax;
		}
	
	LitterAssistVA(iDir, iArgNum, strIndexFirstInlet, strIndexFirstOutlet, oCStr, min, max);
	
	}

static void
MamaTattle(
	objMama* me)
	
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
 *	Helper function for calculating expected values (mean, standard deviation, etc.)
 *	
 ******************************************************************************************/

static double DoExpect(objMama* me, eExpectSelector iSel)
	{ return UniformExpectationsDiscrete(me->min, me->max, iSel); }


/******************************************************************************************
 *
 *	Attribute Funtions
 *
 *	Functions for Inspector/Get Info... dialog
 *	
 ******************************************************************************************/

#if LITTER_USE_OBEX

	static t_max_err MamaGetMean(objMama* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expMean), ioArgC, ioArgV);
		}
	static t_max_err MamaGetMedian(objMama* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expMedian), ioArgC, ioArgV);
		}
	static t_max_err MamaGetMode(objMama* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expMode), ioArgC, ioArgV);
		}
	static t_max_err MamaGetVar(objMama* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expVar), ioArgC, ioArgV);
		}
	static t_max_err MamaGetStdDev(objMama* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expStdDev), ioArgC, ioArgV);
		}
	static t_max_err MamaGetSkew(objMama* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expSkew), ioArgC, ioArgV);
		}
	static t_max_err MamaGetKurtosis(objMama* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expKurtosis), ioArgC, ioArgV);
		}
	static t_max_err MamaGetEntropy(objMama* me, void* iAttr, long* ioArgC, Atom** ioArgV)
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
		attr = attr_offset_new("min", symLong, 0, NULL, NULL, calcoffset(objMama, min));
		class_addattr(gObjectClass, attr);
		attr = attr_offset_new("max", symLong, 0, NULL, NULL, calcoffset(objMama, max));
		class_addattr(gObjectClass, attr);
		
		// Read-Only Attributes
		attr = attribute_new("mean", symFloat64, kAttrFlagsReadOnly, (method) MamaGetMean, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("median", symFloat64, kAttrFlagsReadOnly, (method) MamaGetMedian, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("mode", symFloat64, kAttrFlagsReadOnly, (method) MamaGetMode, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("var", symFloat64, kAttrFlagsReadOnly, (method) MamaGetVar, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("stddev", symFloat64, kAttrFlagsReadOnly, (method) MamaGetStdDev, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("skew", symFloat64, kAttrFlagsReadOnly, (method) MamaGetSkew, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("kurtosis", symFloat64, kAttrFlagsReadOnly, (method) MamaGetKurtosis, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("entropy", symFloat64, kAttrFlagsReadOnly, (method) MamaGetEntropy, NULL);
		class_addattr(gObjectClass, attr);
		}

static void
MamaTell(
	objMama*	me,
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

static void MamaInfo(objMama* me)
	{ LitterInfo(kClassName, &me->coreObject, (method) MamaTattle); }

static inline void AddInfo(void)
	{ LitterAddCant((method) MamaInfo, "info"); }
		
static void MamaTell(objMama* me, Symbol* iTarget, Symbol* iMsg)
	{ LitterExpect((tExpectFunc) DoExpect, (Object*) me, iMsg, iTarget, TRUE); }


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
	
	LitterSetupClass(	kClassName,
						sizeof(objMama),			// Class object size
						LitterCalcOffset(objMama),	// Magic Obex offset calculation
						(method) MamaNew,			// Instance creation function
						NIL,						// No custom deallocation function
						NIL,						// No menu function
						myArgTypes);		
	
	// Messages
	LITTER_TIMEBOMB LitterAddBang	((method) MamaBang);
	LitterAddMess1	((method) MamaMin,		"in1",		A_LONG);
	LitterAddMess1	((method) MamaMax,		"in2",		A_LONG);
	LitterAddMess1	((method) MamaSeed,		"seed",		A_DEFLONG);
	LitterAddMess2	((method) MamaTell,		"tell",		A_SYM, A_SYM);
	LitterAddMess0	((method) MamaTattle,	"tattle");
	LitterAddCant	((method) MamaTattle,	"dblclick");
	LitterAddCant	((method) MamaAssist,	"assist");
	
	AddInfo();
	
	//Initialize Litter Library
	LitterInit(kClassName, 0);
	MamaInit(0, gPool1, gPool2);
	
	}


