/*
	File:		sinker.c

	Contains:	Max/MSP external object synchronizing float-to-signal conversion against a
				trigger signal.

	Written by:	Peter Castine

	Copyright:	© 2003-04 Peter Castine

	Change History (most recent first):

         <3>     11–1–04    pc      Update for modified LitterInit()
         <2>      8–1–04    pc      Some updates for potential Windows compatibility (if we ever
                                    impelement this item).
         <1>    6–7–2003    pc      Initial check-in.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "LitterLib.h"


#pragma mark • Constants

const char*	kClassName		= "lp.sinker~";			// Class name

	// Indices for STR# resource
enum {
	strIndexInTrigger		= lpStrIndexLastStandard + 1,
	strIndexInValue1,
	
	strIndexOutSig1
	};

	// Indices for Inlets and Outlets. Users count starting at 1, we count from 0
	// 
enum {
	inletTrigger		= 0,
	inletValue1,
	
	outletOut1
	};

enum trigMode {
	trigPosDelta		= -1,
	trigNegDelta,
	trigPosXZero,
	trigNegXZero,
	trigAnyXZero,
	
	trigDef				= trigNegDelta,
	trigMin				= trigPosDelta,
	trigMax				= trigAnyXZero
	};


#pragma mark • Type Definitions

typedef enum trigMode eTrigMode;

typedef struct {
		float	current,
				pending;
		} tCurPendPair;

#pragma mark • Object Structure

typedef struct {
	t_pxobject		coreObject;
	short			valCount;		// Number of outlets; one less than number of inlets
	eTrigMode		mode;
	float			lastSync;
	tCurPendPair*	vals;			// Array of values to use when triggered
	tSampleVector*	inSigs;			// Array of input signals (may be NIL)
	tSampleVector*	outSigs;		// Array of output signals
	} objSinker;


#pragma mark • Global Variables

#pragma mark • Function Prototypes

	// Class message functions
void*	SinkerNew(SymbolPtr, short, Atom[]);

	// Object message functions
static void SinkerInt(objSinker*, long);
static void SinkerFloat(objSinker*, double);
static void SinkerDSP(objSinker*, t_signal*[], short[]);
static int* SinkerPerform(int*);

static void SinkerTattle(objSinker*);
static void	SinkerAssist(objSinker*, void* , long , long , char[]);
static void	SinkerInfo(objSinker*);

#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark • Inline Functions

#pragma mark -

/******************************************************************************************
 *
 *	main()
 *	
 *	Standard Max/MSP External Object Entry Point Function
 *	
 ******************************************************************************************/

void
main(void)
	
	{
	LITTER_CHECKTIMEOUT(kClassName);
	
	// Standard Max setup() call
	setup(	&gObjectClass,					// Pointer to our class definition
			(method) SinkerNew,				// Instantiation method  
			(method) dsp_free,				// Default deallocation function
			(short) sizeof(objSinker),		// Class object size
			NIL,							// No menu function
			A_GIMME,						// List of arguments (one value per outlet)
			0);
	
	// Standard Max Messages
	addint	((method) SinkerInt);
	addfloat((method) SinkerFloat);
	
	// MSP Message
	LITTER_TIMEBOMB addmess	((method) SinkerDSP,	"dsp",			A_CANT, 0);
	
	// Info messages
	addmess	((method) SinkerAssist,	"assist",		A_CANT, 0);
	addmess	((method) SinkerInfo,	"info",			A_CANT, 0);
	addmess ((method) SinkerTattle,	"tattle",	 	A_NOTHING);
	addmess ((method) SinkerTattle,	"dblclick", 	A_CANT, 0);
	
	// Initialize Litter Library
	LitterInit(kClassName, 0);

	}

#pragma mark -
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	SinkerNew(iSym, iArgC, iArgV)
 *
 ******************************************************************************************/

void*
SinkerNew(
	SymbolPtr,
	short		iArgC,
	Atom		iArgV[])
	
	{
	const float kDefVal = 0.0;
	
	short			valCount = (iArgC > 0) ? iArgC : 1;
	objSinker*		me	= NIL;
	tCurPendPair*	vals = (tCurPendPair*) NewPtrClear(valCount * sizeof(tCurPendPair));
	tSampleVector*	inSigs = (tSampleVector*) NewPtrClear(valCount * sizeof(tSampleVector));
	tSampleVector*	outSigs = (tSampleVector*) NewPtrClear(valCount * sizeof(tSampleVector));
	
	if (vals == NIL || inSigs == NIL || outSigs == NIL) {
		// Quick punt before any damage can be done
		if (vals != NIL) DisposePtr((Ptr) vals);
		if (inSigs != NIL) DisposePtr((Ptr) inSigs);
		if (outSigs != NIL) DisposePtr((Ptr) outSigs);
		return NIL;	
		}					
	
	// Let Max/MSP allocate us and inlets
	me = (objSinker*) newobject(gObjectClass);
	
	// Add outlets, right to left, and initialize values
	if (iArgC == 0) {
		// Default to two inlets, one outlet
		dsp_setup(&me->coreObject, 2);
		outlet_new(me, "signal");
		// Don't actually need to initialize as long as kDefVal is zero
		// vals[0].current = vals[0].pending = kDefVal;		
		}
	else {
		dsp_setup(&me->coreObject, iArgC + 1);
		while (--iArgC >= 0) {
			outlet_new(me, "signal");
			vals[iArgC].current = vals[iArgC].pending = AtomGetFloat(&iArgV[iArgC]);
			}
		}
	
	// My own initialization. Do this now before we lose the value of iArgCount
	me->lastSync	= 0.0;
	me->valCount	= valCount;
	me->mode		= trigDef;
	me->vals		= vals;
	me->inSigs		= inSigs;
	me->outSigs		= outSigs;
	
punt:
	return me;
	}

#pragma mark -
#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	SinkerInt(me, iVal)
 *	SinkerFloat(me, iVal)
 *
 ******************************************************************************************/

void
SinkerInt(
	objSinker*	me,
	long		iVal)
	
	{
	
	if (me->coreObject.z_in == 0) {
		if (trigMin <= iVal && iVal <= trigMax)
			me->mode = (eTrigMode) iVal;
		else error("%s: invalid mode %ld", kClassName, iVal);
		}
	else SinkerFloat(me, (double) iVal);
	
	}

void
SinkerFloat(
	objSinker*	me,
	double		iVal)
	{
	long index = me->coreObject.z_in - 1;
	
	if (index >= 0)
		me->vals[index].pending = iVal;
	else error("%s: doesn't understand float in left inlet");
	
	}

/******************************************************************************************
 *
 *	SinkerTattle(me)
 *
 * 	Tell all we know.
 *
 ******************************************************************************************/

void
SinkerTattle(
	objSinker*	me)
	
	{
	
	// Object state
	post("%s status", kClassName);
	
	// ... and then stuff specific to Sinker 
	post("  %ld synchronized signals", (long) me->valCount);
	post("  mode: %ld", (long) me->mode);
	
	}

/******************************************************************************************
 *
 *	SinkerAssist
 *	SinkerInfo(me)
 *
 ******************************************************************************************/

void SinkerAssist(objSinker*, void*, long iDir, long iArgNum, char* oCStr)
	{
	iArgNum = (iDir == ASSIST_INLET)
				? 1 + (iArgNum > 1) 
				: 1;
	LitterAssist(iDir, iArgNum, strIndexInTrigger, strIndexOutSig1, oCStr);
	}

void SinkerInfo(objSinker* me)
	{ LitterInfo(kClassName, (tObjectPtr) me, (method) SinkerTattle); }

#pragma mark -
#pragma mark • DSP Methods

/******************************************************************************************
 *
 *	SinkerDSP(me, ioDSPVectors, iConnectCounts)
 *
 ******************************************************************************************/

void
SinkerDSP(
	objSinker*	me,
	t_signal*	ioDSPVectors[],
	short		iConnectCounts[])
	
	{
	long	valCount = me->valCount;
	int		index;
	
	dsp_add(
		SinkerPerform, 3,
		me,
		(long) ioDSPVectors[0]->s_n,
		*iConnectCounts++ ? ioDSPVectors++[0]->s_vec : NIL
		);

	for (index = 0; index < valCount; index += 1) {
		me->inSigs[index]	= iConnectCounts++[0] ? ioDSPVectors[0]->s_vec : NIL;
		me->outSigs[index]	= ioDSPVectors++[valCount]->s_vec;
		}
	
	}
	

/******************************************************************************************
 *
 *	PerformSinker(iParams)
 *
 *	Parameter block contains 4 values:
 *		- Address of this function
 *		- The performing Sinker object
 *		- Current signal vector size
 *		- Sync signal
 *
 ******************************************************************************************/

	inline static void UpdatePending(tCurPendPair iVals[], int iCount)
		{ do { iVals[0].current = iVals[0].pending; iVals += 1; } while (--iCount > 0); }
		
	inline static void
	CopyVals(
		tCurPendPair	iVals[],
		int				iCount,
		tSampleVector	iInSigs[],
		tSampleVector	iOutSigs[])
		
		{
		}

int*
SinkerPerform(
	int* iParams)
	
	{
	enum {
		paramFuncAddress	= 0,
		paramMe,
		paramVectorSize,
		paramSync,
		
		paramNextLink
		};
	
	objSinker*		me = (objSinker*) iParams[paramMe];
	
	long			vecCounter;
	tSampleVector	syncSig;
	float			prevSamp;
	int				valCount;
	tCurPendPair*	vals;
	tSampleVector*	inSigs;
	tSampleVector*	outSigs;
	
	if (me->coreObject.z_disabled) goto exit;
	
	vecCounter	= (long) iParams[paramVectorSize];
	syncSig		= (tSampleVector) iParams[paramSync];
	prevSamp	= me->lastSync;
	valCount	= me->valCount;
	vals		= me->vals;
	inSigs		= me->inSigs;
	outSigs		= me->outSigs;
	
	switch (me->mode) {
		float curSamp;
	case trigNegDelta:
		do {
			curSamp = *syncSig++;
			if (curSamp < prevSamp)
				UpdatePending(vals, valCount);
			CopyVals(vals, valCount, inSigs, outSigs);
			prevSamp = curSamp;
			} while (--vecCounter > 0);
		break;
		
	case trigPosXZero:
		do {
			curSamp = *syncSig++;
			if (curSamp > 0.0 && prevSamp <= 0.0)
				UpdatePending(vals, valCount);
			CopyVals(vals, valCount, inSigs, outSigs);
			prevSamp = curSamp;
			} while (--vecCounter > 0);
		break;
		
	case trigNegXZero:
		do {
			curSamp = *syncSig++;
			if (curSamp < 0.0 && prevSamp >= 0.0)
				UpdatePending(vals, valCount);
			CopyVals(vals, valCount, inSigs, outSigs);
			prevSamp = curSamp;
			} while (--vecCounter > 0);
		break;
		
	case trigAnyXZero:
		do {
			curSamp = *syncSig++;
			if (curSamp != 0.0 && curSamp * prevSamp <= 0.0)
				UpdatePending(vals, valCount);
			CopyVals(vals, valCount, inSigs, outSigs);
			prevSamp = curSamp;
			} while (--vecCounter > 0);
		break;
		
	default:
		// Must be trigPosDelta
		// Not really "default", but the only thing that's left
		do {
			curSamp = *syncSig++;
			if (curSamp > prevSamp)
				UpdatePending(vals, valCount);
			CopyVals(vals, valCount, inSigs, outSigs);
			prevSamp = curSamp;
			} while (--vecCounter > 0);
		break;
		}

	// Don't forget to cache final sync value for next run
	me->lastSync = prevSamp;
	
exit:
	return iParams + paramNextLink;
	}

