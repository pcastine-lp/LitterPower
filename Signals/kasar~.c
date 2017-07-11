/*
	File:		kasar~.c

	Contains:	Cymbal noise (sum of large number of sinusoids)

	Written by:	Peter Castine

	Copyright:	© 2005 Peter Castine

	Change History (most recent first):

         <2>   26–4–2006    pc      Update for new LitterLib organization.
         <1>   23–3–2006    pc      first checked in. (Experimental)
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "LitterLib.h"									// Also #includes MaxUtils.h, ext.h
#include "TrialPeriodUtils.h"
#include "Taus88.h"

#pragma mark • Constants

const char		kClassName[]	= "lp.kasar~";		// Class name

const unsigned long	kTableLen		= 256,
				kTableLenMod	= 0x000001ff,
				kMaxUnitThresh	= 0x07ffffff;		// kMaxUInt / kTableLen

	// Indices for STR# resource
enum {
		// Inlets
	strIndexInNSines	= lpStrIndexLastStandard + 1,
	
		// Outlets
	strIndexOutCymbal,
	
		// Offsets for assiststring()
	strIndexInLeft		= strIndexInNSines,
	strIndexOutLeft		= strIndexOutCymbal
	};

#pragma mark • Type Definitions



#pragma mark • Object Structure

typedef struct {
	t_pxobject		coreObject;
	
	SymbolPtr		bufSym;
	tBufferPtr		buf;
	unsigned long	bufOffset;			// User-specified offset into buffer table
	
	long			maxN,
					curN;
	
	double			sr,
					*step;				// 
	} tCymbal;


#pragma mark • Global Variables

t_sample	gCosine[kTableLen];



#pragma mark • Function Prototypes

	// Class message functions
void*	CymbalNew(double);

	// Object message functions
static void CymbalInt(tCymbal*, long);
static void CymbalAmp(tCymbal*, double);
static void CymbalDefAmp(tCymbal*);
static void CymbalTattle(tCymbal*);
static void	CymbalAssist(tCymbal*, void* , long , long , char*);
static void	CymbalInfo(tCymbal*);

	// MSP Messages
static void	CymbalDSP(tCymbal*, t_signal**, short*);
static int*	CymbalPerformSimp(int*);
static int*	CymbalPerformMod(int*);


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

	static void InitCosineTable()
		{
		const double kThetaIncr = k2pi / (double) kTableLen;
		
		int		i;
		double	theta;
		
		for (	i = 0, theta = 0.0;
				i < kTableLen;
				i += 1, theta += kThetaIncr) {
			gCosine[i] = cos(theta);
			}
		}

void
main(void)
	
	{
	LITTER_CHECKTIMEOUT(kClassName);
	
	// Get this out of the way
	InitCosineTable();
	
	// Standard Max/MSP initialization mantra
	setup(	&gObjectClass,							// Pointer to our class definition
			(method) CymbalNew,						// Instance creation function
			(method) dsp_free,						// Default deallocation function
			sizeof(tCymbal),						// Class object size
			NIL,									// No menu function
			A_DEFFLOAT,								// Optional argument: Amplitude
			0);
			
	dsp_initclass();

	// Messages
	addint	((method) CymbalInt);
	addfloat((method) CymbalAmp);
	addmess	((method) CymbalDefAmp,	"defamp",	A_NOTHING);
	addmess	((method) CymbalTattle,	"dblclick",	A_CANT, 0);
	addmess	((method) CymbalTattle,	"tattle",	A_NOTHING);
	addmess	((method) CymbalAssist,	"assist",	A_CANT, 0);
	addmess	((method) CymbalInfo,	"info",		A_CANT, 0);
	
	// MSP-Level messages
	LITTER_TIMEBOMB addmess	((method) CymbalDSP,	"dsp",		A_CANT, 0);

	//Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();
	
	}



#pragma mark -
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	CymbalNew(iAmp)
 *	
 ******************************************************************************************/

void*
CymbalNew(
	double iAmp)
	
	{
	const double kDefAmp = 1.0;						// ?? 
	
	tCymbal* me	= NIL;
	
	// Cheesy test for default amplitude
	if (iAmp == 0.0)
		iAmp = kDefAmp;
	
	// Let Max/MSP allocate us, our inlets, and outlets.
	me = (tCymbal*) newobject(gObjectClass);
	dsp_setup(&(me->coreObject), 1);				// Signal inlet for benefit of begin~
	
	outlet_new(me, "signal");
	
//	me->amp = iAmp;
	
	return me;
	}

#pragma mark -
#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	CymbalAmp(me, iAmp)
 *	CymbalDefAmp(me)
 *
 *	CymbalInt(me, iAmp)
 *
 ******************************************************************************************/

/*void CymbalAmp(tCymbal* me, double iAmp)
	{ me->amp = iAmp; }

void CymbalDefAmp(tCymbal* me)
	{ CymbalAmp(me, kDefAmp); }

void CymbalInt(tCymbal* me, long iAmp)
	{ CymbalAmp(me, (double) iAmp); }
*/
/******************************************************************************************
 *
 *	CymbalTattle(me)
 *
 *	Fairly generic Assist/Info methods
 *
 ******************************************************************************************/

void
CymbalTattle(
	tCymbal* me)
	
	{
	
	post("%s state:", kClassName);
	
	}


/******************************************************************************************
 *
 *	CymbalAssist
 *	CymbalInfo(me)
 *
 *	Fairly generic Assist/Info methods
 *
 ******************************************************************************************/

void CymbalAssist(tCymbal*, void*, long iDir, long iArgNum, char* oCStr)
	{ LitterAssist(iDir, iArgNum, strIndexInLeft, strIndexOutLeft, oCStr); }

void CymbalInfo(tCymbal*)
	{ LitterInfo(kClassName, NIL, NIL); }

#pragma mark -
#pragma mark • DSP Methods

/******************************************************************************************
 *
 *	CymbalDSP(me, ioDSPVectors, iConnectCounts)
 *
 ******************************************************************************************/

void
CymbalDSP(
	tCymbal*	me,
	t_signal**	ioDSPVectors,
	short*		iConnectCounts)
	
	{
	// Indices for signal inlets & outlets
	enum {
		inletAmp		= 0,
		outletNoise
		};
	
	if (iConnectCounts[inletAmp] > 0)
		dsp_add(
			CymbalPerformMod, 4,
			me, (long) ioDSPVectors[inletAmp]->s_n,
			ioDSPVectors[inletAmp]->s_vec,
			ioDSPVectors[outletNoise]->s_vec
			);
	
	else dsp_add(
			CymbalPerformSimp, 3,
			me, (long) ioDSPVectors[outletNoise]->s_n, ioDSPVectors[outletNoise]->s_vec
			);
	
	}
	

/******************************************************************************************
 *
 *	CymbalPerformMod(iParams)
 *	CymbalPerformMod(iParams)
 *
 *	Parameter block for CymbalPerformMod contains 5 values:
 *		- Address of this function
 *		- The performing kashar~ object
 *		- Vector size
 *		- Input signal
 *		- Output signal
 *
 *	Parameter block for CymbalPerformSimp contains 4 values:
 *		- Address of this function
 *		- The performing kashar~ object
 *		- Vector size
 *		- Output signal
 *
 ******************************************************************************************/

int*
CymbalPerformMod(
	int iParams[])
	
	{
	enum {
		paramFuncAddress	= 0,
		paramMe,
		paramVectorSize,
		paramAmp,
		paramOut,
		
		paramNextLink
		};
		
	const int kRandBitCount = 32;
	
	long			vecCounter,
					randBits;
	tSampleVector	inAmp,
					outNoise;
	tCymbal*		me = (tCymbal*) iParams[paramMe];
	
	if (me->coreObject.z_disabled) goto exit;
	
	// Copy parameters into registers
	vecCounter	= (long) iParams[paramVectorSize];
	inAmp		= (tSampleVector) iParams[paramAmp];
	outNoise	= (tSampleVector) iParams[paramOut];
	
	// Do our stuff
	// We rely on vecCounter being a power of 2, which means that either
	// a single 32-bit random value will sort us out or we will use some
	// exact integral multiple of 32-bits
	if (vecCounter < kRandBitCount) {
		randBits = Taus88(NIL);
		
		do {
			*outNoise++ = (randBits < 0) ? -(*inAmp++) : *inAmp++;
			randBits <<= 1;
			} while (vecCounter-- > 0);
		}
	
	else while (vecCounter > 0) {
		int i = kRandBitCount;
		
		randBits = Taus88(NIL);
		do {
			*outNoise++ = (randBits < 0) ? -(*inAmp++) : *inAmp++;
			randBits <<= 1;
			} while (--i > 0);
		
		vecCounter -= kRandBitCount;
		}
	
exit:
	return iParams + paramNextLink;
	}


int*
CymbalPerformSimp(
	int iParams[])
	
	{
	enum {
		paramFuncAddress	= 0,
		paramMe,
		paramVectorSize,
		paramOut,
		
		paramNextLink
		};
		
	const int kRandBitCount = 32;
	
	long			vecCounter,
					randBits;
	float			amp, negAmp;
	tSampleVector	outNoise;
	tCymbal*		me = (tCymbal*) iParams[paramMe];
	
	if (me->coreObject.z_disabled) goto exit;
	
	// Copy parameters into registers
	vecCounter	= (long) iParams[paramVectorSize];
	outNoise	= (tSampleVector) iParams[paramOut];
//	amp			= me->amp;
	negAmp		= -amp;
	
	// Do our stuff
	// We rely on vecCounter being a power of 2, which means that either
	// a single 32-bit random value will sort us out or we will use some
	// exact integral multiple of 32-bits
	if (vecCounter < kRandBitCount) {
		randBits = Taus88(NIL);
		
		do {
			*outNoise++ = (randBits < 0) ? negAmp : amp;
			randBits <<= 1;
			} while (vecCounter-- > 0);
		}
	
	else while (vecCounter > 0) {
		int i = kRandBitCount;
		
		randBits = Taus88(NIL);
		do {
			*outNoise++ = (randBits < 0) ? negAmp : amp;
			randBits <<= 1;
			} while (--i > 0);
		
		vecCounter -= kRandBitCount;
		}
	
exit:
	return iParams + paramNextLink;
	}
