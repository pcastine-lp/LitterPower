/*
	File:		lili.c

	Contains:	Max external object implementing a parameterizable linear congruence 
				psuedo-random number generator.

	Written by:	Peter Castine

	Copyright:	© 2001-2002 Peter Castine

	Change History (most recent first):

         <6>     15–3–06    pc      Add expect message
         <5>     14–1–04    pc      Update for Windows.
         <4>    7–7–2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <3>  30–12–2002    pc      Drop faux 'Vers' resource and related modifications. Add object
                                    version to DoInfo().
         <2>  28–11–2002    pc      Tidy up after initial check in.
         <1>  28–11–2002    pc      Initial check in.
		27-Apr-2001:	First implementation.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "LitterLib.h"
#include "TrialPeriodUtils.h"
#include "MiscUtils.h"
#include "UniformExpectations.h"


#pragma mark • Constants

const char	kClassName[]		= "lp.lili";			// Class name


	// Indices for STR# resource
enum {
	strIndexSeed		= lpStrIndexLastStandard + 1,
	strIndexMul,
	strIndexAdd,
	strIndexMod,
	
	strIndexTheOutlet,
	
	strIndexLeftInlet	= strIndexSeed,
	strIndexLeftOutlet	= strIndexTheOutlet
	};


#pragma mark • Object Structure

typedef struct {
	LITTER_CORE_OBJECT(Object, coreObject);
	
	UInt32	seed,
			mul,
			add,
			mod;
	} objLili;


#pragma mark • Function Prototypes

static void LiliSet		(objLili*, UInt32);


#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	LiliNew(iMul, iAdd, iMod, iSeed)
 *
 ******************************************************************************************/

static void*
LiliNew(
	unsigned long	iMul,					// Note that Max is thinking signed; we think
	unsigned long	iAdd,					// unsigned. Hope no one gets too confused.
	unsigned long	iMod,
	unsigned long	iSeed)
	
	{
	const long	kDefMul		= 65539,
				kDefSeed	= 1;
	
	objLili*	me	= NIL;
	
	// Run through initialization parameters from right to left, handling defaults
	if (iSeed == 0)  
		iSeed = kDefSeed;
	else goto noMoreDefaults;
	
		// 0 is default of iMod and iAdd
	if (iMod != 0) goto noMoreDefaults;
	if (iAdd != 0) goto noMoreDefaults;
			
	if (iMul == 0)
		iMul = kDefMul;
noMoreDefaults:
	// Finished checking intialization parameters
	
	// Let Max allocate us, our inlets, and outlets
	me = (objLili*) LitterAllocateObject();

	intin(me, 3);										// Mod inliet
	intin(me, 2);										// Adder inlet
	intin(me, 1);										// Multiplier inlet
	
	intout(me);					// Access main outlet through me->coreObject.o_outlet;
	
	// Initialize and store object components
	me->mul		= iMul;
	me->add		= iAdd;
	me->mod		= iMod;
	me->seed	= iSeed;
	
	return me;
	}

#pragma mark -
#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	LiliBang(me)
 *	LiliSeed(me)
 *
 *	Grab a new random number from Lili.
 *
 ******************************************************************************************/

static void
LiliBang(
	objLili* me)
	
	{
	UInt32	l,
			mod	= me->mod;
	
	l = me->seed * me->mul + me->add;
	if (mod != 0)
		l %= mod;
	me->seed = l;
	
	outlet_int(me->coreObject.o_outlet, l);		// Implicitly typecasts l to signed long
	
	}

static void
LiliSeed(
	objLili*			me,
	unsigned long	iSeed)
	
	{
	LiliSet(me, iSeed);
	LiliBang(me);
	}

/******************************************************************************************
 *
 *	LiliSet(me, iSeed)
 *	LiliMul(me, iMul)
 *	LiliAdderl(me, iMul)
 *	LiliMod(me, iMul)
 *	
 *	Nothing reallly bad can happen... we simply treat everything as unsigned. A multiplier
 *	of 0 seems a little counter-productive, but even this is allowed.
 *
 ******************************************************************************************/

static void LiliSet		(objLili* me, UInt32 iSeed)	{ me->seed = iSeed; }
static void LiliMul		(objLili* me, UInt32 iMul)	{ me->mul = iMul; }
static void LiliAdder	(objLili* me, UInt32 iAdd)	{ me->add = iAdd; }
static void LiliMod		(objLili* me, UInt32 iMod)	{ me->mod = iMod; }


#pragma mark -
#pragma mark • Attribute/Information Functions

/******************************************************************************************
 *
 *	LiliAssist
 *	LiliTattle(me)
 *
 ******************************************************************************************/

static void LiliAssist(objLili* me, void* iBox, long iDir, long iArgNum, char* oCStr)
	{
	#pragma unused(me, iBox)
	
	 LitterAssist(iDir, iArgNum, strIndexSeed, strIndexTheOutlet, oCStr);
	 }

static void
LiliTattle(
	objLili* me)
	
	{
	
	post("%s state",
			kClassName);
	post("  Current seed: %lu", me->seed);
	post("  Current multiplier: %lu", me->mul);
	post("  Current adder: %lu", me->add);
	post("  Current modulo: %lu", me->mod);
	if (me->mod == 0)
		post("     ...which is treated as 4294967296");
	
	}

/******************************************************************************************
 *
 *	DoExpect(me, iSelector, iLabel, iTarget)
 *
 ******************************************************************************************/

static double
DoExpect(
	objLili*		me,
	eExpectSelector iSel)
	
	{
	long min, max;
	
	if (me->mod == 0 || me->mod > kLongMax) {
		// Calculated values will include results that wrap around to negative values
		min = kLongMin;
		max = kLongMax;
		}
	else {
		min = 0;
		max = me->mod - 1;
		}
	
	return UniformExpectationsDiscrete(min, max, iSel);
	}

/******************************************************************************************
 *
 *	Attribute Funtions
 *
 *	Functions for Inspector/Get Info... dialog
 *	
 ******************************************************************************************/

#if LITTER_USE_OBEX

	static t_max_err LiliGetMin(objLili* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expMin), ioArgC, ioArgV);
		}
	static t_max_err LiliGetMax(objLili* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expMax), ioArgC, ioArgV);
		}
	static t_max_err LiliGetMean(objLili* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expMean), ioArgC, ioArgV);
		}
	static t_max_err LiliGetMedian(objLili* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expMedian), ioArgC, ioArgV);
		}
	static t_max_err LiliGetMode(objLili* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expMode), ioArgC, ioArgV);
		}
	static t_max_err LiliGetVar(objLili* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expVar), ioArgC, ioArgV);
		}
	static t_max_err LiliGetStdDev(objLili* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expStdDev), ioArgC, ioArgV);
		}
	static t_max_err LiliGetSkew(objLili* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expSkew), ioArgC, ioArgV);
		}
	static t_max_err LiliGetKurtosis(objLili* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{ 
		#pragma unused(iAttr)
		
		return LitterGetAttrFloat(DoExpect(me, expKurtosis), ioArgC, ioArgV);
		}
	static t_max_err LiliGetEntropy(objLili* me, void* iAttr, long* ioArgC, Atom** ioArgV)
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
		attr = attr_offset_new("seed", symLong, 0, NULL, NULL, calcoffset(objLili, seed));
		class_addattr(gObjectClass, attr);
		attr = attr_offset_new("mul", symLong, 0, NULL, NULL, calcoffset(objLili, mul));
		class_addattr(gObjectClass, attr);
		attr = attr_offset_new("add", symLong, 0, NULL, NULL, calcoffset(objLili, add));
		class_addattr(gObjectClass, attr);
		attr = attr_offset_new("mod", symLong, 0, NULL, NULL, calcoffset(objLili, mod));
		class_addattr(gObjectClass, attr);
		
		// Read-Only Attributes
		attr = attribute_new("min", symLong, kAttrFlagsReadOnly, (method) LiliGetMin, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("max", symLong, kAttrFlagsReadOnly, (method) LiliGetMax, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("mean", symFloat64, kAttrFlagsReadOnly, (method) LiliGetMean, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("median", symFloat64, kAttrFlagsReadOnly, (method) LiliGetMedian, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("mode", symFloat64, kAttrFlagsReadOnly, (method) LiliGetMode, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("var", symFloat64, kAttrFlagsReadOnly, (method) LiliGetVar, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("stddev", symFloat64, kAttrFlagsReadOnly, (method) LiliGetStdDev, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("skew", symFloat64, kAttrFlagsReadOnly, (method) LiliGetSkew, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("kurtosis", symFloat64, kAttrFlagsReadOnly, (method) LiliGetKurtosis, NULL);
		class_addattr(gObjectClass, attr);
		attr = attribute_new("entropy", symFloat64, kAttrFlagsReadOnly, (method) LiliGetEntropy, NULL);
		class_addattr(gObjectClass, attr);
		}

static void
LiliTell(
	objLili*	me,
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

static void LiliInfo(objLili* me)
	{ LitterInfo(kClassName, &me->coreObject, (method) LiliTattle); }

static inline void AddInfo(void)
	{ LitterAddCant((method) LiliInfo, "info"); }
		
static void LiliTell(objLili* me, Symbol* iTarget, Symbol* iMsg)
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
						A_DEFLONG,					// Optional argument:	1. Multipilier
						A_DEFLONG,					//						2. Adder
						A_DEFLONG,					//						3. Modulo
						A_DEFLONG,					//						4. Seed
						A_NOTHING
						};
	
	LITTER_CHECKTIMEOUT(kClassName);
	
	LitterSetupClass(	kClassName,
						sizeof(objLili),			// Class object size
						LitterCalcOffset(objLili),	// Magic "Obex" Calculation
						(method) LiliNew,			// Instance creation function
						NIL,						// No custom deallocation function
						NIL,						// No UI function
						myArgTypes);				// See above

	// Messages
	LITTER_TIMEBOMB LitterAddBang	((method) LiliBang);
	LITTER_TIMEBOMB LitterAddInt	((method) LiliSeed);
	LitterAddMess1	((method) LiliSet,		"set",		A_LONG);
	LitterAddMess1	((method) LiliSet,		"seed",		A_LONG);
	LitterAddMess1	((method) LiliMul,		"in1",		A_LONG);
	LitterAddMess1	((method) LiliAdder,	"in2",		A_LONG);
	LitterAddMess1	((method) LiliMod,		"in3",		A_LONG);
	LitterAddMess2	((method) LiliTell,		"tell",		A_SYM, A_SYM);
	LitterAddMess0	((method) LiliTattle,	"tattle");
	LitterAddCant	((method) LiliTattle,	"dblclick");
	LitterAddCant	((method) LiliAssist,	"assist");
	
	AddInfo();
	
	//Initialize Litter Library
	LitterInit(kClassName, 0);
	
	}


