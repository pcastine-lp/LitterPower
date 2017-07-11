/*
	File:		lll~.c

	Contains:	Linear congruence "noise".

	Written by:	Peter Castine

	Copyright:	© 2001-2002 Peter Castine

	Change History (most recent first):

         <6>   26–4–2006    pc      Update for new LitterLib organization.
         <5>     14–1–04    pc      Update for Windows.
         <4>    6–7–2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <3>  30–12–2002    pc      Drop faux 'Vers' resource and related modifications. Add object
                                    version to LllInfo().
         <2>  28–11–2002    pc      Tidy up after initial check in.
         <1>  28–11–2002    pc      Initial check in.
		14-Apr-2000:	First implementation.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "LitterLib.h"	// Also #includes MaxUtils.h, ext.h
#include "TrialPeriodUtils.h"
#include "MiscUtils.h"

#pragma mark • Constants

const char*	kClassName		= "lp.lll~";			// Class name

	// Indices for STR# resource
enum {
	strIndexInSeed		= lpStrIndexLastStandard + 1,
	strIndexInMul,
	strIndexInAdd,
	strIndexInMod,
	
	strIndexOutLCN,
	
	strIndexInLeft		= strIndexInSeed,
	strIndexOutLeft		= strIndexOutLCN
	};
	
#pragma mark • Repertoire Type & Constants

#if 0
	// Repertoire of historical LC RNGs
enum {
	repCSound		= 0,
	repCSound15,
	repParkMiller,
	repMatLab,
	repBSD,
	repIBM,
	repFrei,
	
	repCount
	};

	// Developing repertoire of historical RNGs
typedef struct {
	const char		name[];
	UInt32			mul,
					add,
					mod,
					seed;
	Boolean			genSeed;
	} tRepertoire;

const tRepertoire	kRepertoire[] = {
						// legacy rand16 used in CSound:
					{ "csound",		15625,		1,		65536,		1000,	false},
						// legacy rand15 used in CSound:
					{ "csound15",	15625,		1,		32768,		1000,	false},
						// Park-Miller "Minimum Standard"
					{ "pm",			16807,		0,		2147483647,	0,		true},
						// rand31 from CSound, optional but more modern RNG.
						// But note the modification to the modulo
					{ "csound31",			16807,		0,		0,	0,		true},
						// Matlab's rng. Note that it uses a real modulo. 
						// Initial seed must be non-zero
					{ "matlab",		16807,		0,		2147483647,	0,		true},
						// RNG from BSD Unix. Now deprecated.
					{ "bsd",		1103515245,	12345,	2147483648, 0,		true},
						// IBM's RANDU
					{ "ibm",		65539,		0,		2147483648,	1,		false},
						// Beat Frei's suggested noise generator
					{ "frei",		69069,		1,		0,			0,		false}
				};
#endif

#pragma mark • Object Structure

typedef struct {
	t_pxobject		coreObject;
	
	unsigned long	mul,
					add,
					mod,
					modMask,
					seed;
	} tLCN;


#pragma mark • Global Variables



#pragma mark • Function Prototypes

	// Class message functions
void*	LllNew(long, long, long, long);

	// Object message functions
static void LllMul(tLCN*, unsigned long);
static void LllAdd(tLCN*, unsigned long);
static void LllMod(tLCN*, unsigned long);
static void LllSeed(tLCN*, unsigned long);
static void LllTattle(tLCN*);
static void	LllAssist(tLCN*, void* , long , long , char*);
static void	LllInfo(tLCN*);

	// MSP Messages
static void	LllDSP(tLCN*, t_signal**, short*);
static int*	LllPerform(int*);


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
	
	// Standard Max/MSP initialization mantra
	setup(	&gObjectClass,					// Pointer to our class definition
			(method) LllNew,			// Instance creation function
			(method) dsp_free,			// Default deallocation function
			sizeof(tLCN),				// Class object size
			NIL,						// No menu function
			A_DEFLONG,					// Optional arguments:	1. Multiplier
			A_DEFLONG,					//						2. Adder
			A_DEFLONG,					//						3. Modulo
			A_DEFLONG,					//						4. Seed
			0);		
	
	dsp_initclass();

	// Messages
	addint	((method) LllSeed);
	addmess	((method) LllMul,		"in1",		A_LONG, 0);
	addmess	((method) LllAdd,		"in2",		A_LONG,	0);
	addmess	((method) LllMod,		"in3",		A_LONG, 0);
	addmess	((method) LllTattle,	"dblclick",	A_CANT, 0);
	addmess	((method) LllTattle,	"tattle",	A_NOTHING);
	addmess	((method) LllAssist,	"assist",	A_CANT, 0);
	addmess	((method) LllInfo,		"info",		A_CANT, 0);
	
	// MSP-Level messages
	LITTER_TIMEBOMB addmess	((method) LllDSP,		"dsp",		A_CANT, 0);

	//Initialize Litter Library
	LitterInit(kClassName, 0);
	
	}



#pragma mark -
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	LllNew(iMul, iAdd, iMod, iSeed)
 *	
 ******************************************************************************************/

void*
LllNew(
	long	iMul,
	long	iAdd,
	long	iMod,
	long	iSeed)
	
	{
	const long	kDefMul		= 65539,
				kDefAdd		= 0,
				kDefMod		= 0,			// This is interpreted as 2^32
				kDefSeed	= 1;
	
	tLCN*		me	= NIL;
	
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

	// Let Max/MSP allocate us, our inlets (from right to left), and outlets.
	me = (tLCN*) newobject(gObjectClass);
	dsp_setup(&(me->coreObject), 1);				// Signal inlet for benefit of begin~
													// Otherwise left inlet does multiplier
													// only
	
	intin(me, 3);									// Seed inliet
	intin(me, 2);									// Modulo inlet
	intin(me, 1);									// Add inlet
	
	outlet_new(me, "signal");
	
	// Initialize and store object components
	me->mul		= iMul;
	me->add		= iAdd;
	LllMod(me, iMod);								// This is the only parameter with
													// some added initialization work.
	me->seed	= iSeed;

	return me;
	}

#pragma mark -
#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	LllSet(me, iSeed)
 *	LllMul(me, iMul)
 *	LllAdderl(me, iMul)
 *	LllMod(me, iMul)
 *	
 *	Nothing reallly bad can happen... we simply treat everything as unsigned. A multiplier
 *	of 0 seems a little counter-productive, but even this is allowed.
 *
 ******************************************************************************************/

void LllMul	(tLCN* me, unsigned long iMul)		{ me->mul = iMul; }
void LllAdd	(tLCN* me, unsigned long iAdd)		{ me->add = iAdd; }
void LllSeed	(tLCN* me, unsigned long iSeed)		{ me->seed = iSeed; }

void LllMod	(tLCN* me, unsigned long iMod)
	{
	unsigned bitCount = CountBits(iMod);
	
	me->mod		= iMod;
	me->modMask = bitCount == 1
					? iMod - 1
					: 0;
	
	}


/******************************************************************************************
 *
 *	LllTattle(me)
 *
 *	Post state information
 *
 ******************************************************************************************/

void
LllTattle(
	tLCN* me)
	
	{
	
	post("%s state",
			kClassName);
	post("  Current multiplier: %lu", me->mul);
	post("  Current adder: %lu", me->add);
	post("  Current modulo: %lu", me->mod);
	if (me->mod == 0)
		post("     ...which is treated as 4294967296");
	if (me->modMask != 0)
		post("     ...which we perform by masking with %lx", me->modMask);
	post("  Current seed: %lu", me->seed);
	
	}


/******************************************************************************************
 *
 *	LllAssist()
 *	LllInfo()
 *
 *	Fairly generic Assist/Info methods
 *
 ******************************************************************************************/

void LllAssist(tLCN* me, void* box, long iDir, long iArgNum, char* oCStr)
	{
	#pragma unused(me, box)
	
	LitterAssist(iDir, iArgNum, strIndexInLeft, strIndexOutLeft, oCStr);
	}

void LllInfo(tLCN* me)
	{ LitterInfo(kClassName, &me->coreObject.z_ob, (method) LllTattle); }


#pragma mark -
#pragma mark • DSP Methods

/******************************************************************************************
 *
 *	LllDSP(me, ioDSPVectors, iConnectCounts)
 *
 ******************************************************************************************/

void
LllDSP(
	tLCN*		me,
	t_signal**	ioDSPVectors,
	short		connectCounts[])
	
	{
	#pragma unused(connectCounts)
	
	enum {
		inletLCN	= 0,
		outletLCN
		};
	
	dsp_add(
		LllPerform, 3,
		me, (long) ioDSPVectors[outletLCN]->s_n, ioDSPVectors[outletLCN]->s_vec
		);
	
	}
	

/******************************************************************************************
 *
 *	LllPerform(iParams)
 *
 *	Parameter block for PerformSync contains 8 values:
 *		- Address of this function
 *		- The performing schhh~ object
 *		- Vector size
 *		- output signal
 *		- Imaginary output signal
 *
 ******************************************************************************************/

int*
LllPerform(
	int* iParams)
	
	{
	enum {
		paramFuncAddress	= 0,
		paramMe,
		paramVectorSize,
		paramOut,
		
		paramNextLink
		};
	
	long			vecCounter;
	tSampleVector	outNoise;
	tLCN*			me = (tLCN*) iParams[paramMe];
	unsigned long	mul,
					add,
					mod,
					seed;
	
	if (me->coreObject.z_disabled) goto exit;
	
	// Copy parameters into registers
	vecCounter	= (long) iParams[paramVectorSize];
	outNoise	= (tSampleVector) iParams[paramOut];
	mul			= me->mul;
	add			= me->add;
	mod			= me->mod;
	seed		= me->seed;
	
	// Do our stuff
	if (mod == 0) do {
		// Treat modulo as 2^32, which means we just let the processor truncate arithmetic overflow
		seed *= mul;
		seed += add;
		*outNoise++ = ULong2Signal(seed);
		} while (--vecCounter > 0);
		
	else {
		unsigned long	modMask = me->modMask;
		double			scale	= (double) kULongMax / (double) mod;
		
		if (modMask != 0)  do {
			seed *= mul;
			seed += add;
			seed &= modMask;
			*outNoise++ = scale * Long2Signal((long) seed) - 1.0;
			} while (--vecCounter > 0);
		
		else  do {
			seed *= mul;
			seed += add;
			seed %= mod;
			*outNoise++ = scale * Long2Signal((long) seed) - 1.0;
			} while (--vecCounter > 0);
		}
	
	me->seed = seed;
	
exit:
	return iParams + paramNextLink;
	}
