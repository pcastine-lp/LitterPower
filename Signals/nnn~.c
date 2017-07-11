/*
	File:		nnn~.c

	Contains:	"Browner" noise. Uses Taus88 algorithm for random values, also has
				unique NN factor.

	Written by:	Peter Castine

	Copyright:	© 2001-2002 Peter Castine

	Change History (most recent first):

         <2>  28–11–2002    pc      Tidy up after initial check in.
         <1>  28–11–2002    pc      Initial check in.
		14-Apr-2000:	First implementation.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include <stdlib.h>		// For rand(), RAND_MAX

#include "LitterLib.h"	// Also #includes MaxUtils.h, ext.h

#include "z_dsp.h"


#pragma mark • Constants

const char*	kClassName		= "lp.nnn~";			// Class name
const char*	kPackageName	= "lp.nnn~ Package";	// Package name for 'vers'(2) resource

const char* kCategory		= "MSP";

const int	kMaxNN			= 31;

	// Indices for STR# resource
enum {
	strIndexInNN		= 1,
	
	strIndexOutBrowner
	};

	// Indices for MSP Inlets and Outlets.
enum {
	inletBrowner			= 0,		// For begin~
	
	outletBrowner
	};

#pragma mark • Type Definitions



#pragma mark • Object Structure

typedef struct {
	t_pxobject		coreObject;
	
	int				nn;					// Number of bits to mask out
	unsigned long	mask,				// Value depends on nn
					offset;				// ditto
	double			nextSeam;			// Use for continuity between I/O vector calls
	} tBrowner;


#pragma mark • Global Variables

t_messlist*	gBrownerClass		= NIL;
short		gStrResID		= 0,
			gVersID			= 0,
			gPackVersID		= 0;


#pragma mark • Function Prototypes

	// Class message functions
void*	NewBrowner(long);

	// Object message functions
static void DoNN(tBrowner*, long);
static void DoTattle(tBrowner*);
static void	DoAssist(tBrowner*, void* , long , long , char*);
static void	DoInfo(tBrowner*);

	// MSP Messages
static void	BuildDSPChain(tBrowner*, t_signal**, short*);
static int*	PerformBrowner(int*);


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
	setup(	&gBrownerClass,				// Pointer to our class definition
			(method) NewBrowner,			// Instance creation function
			(method) dsp_free,			// Default deallocation function
			sizeof(tBrowner),				// Class object size
			NIL,						// No menu function
			A_DEFLONG,					// Optional arguments:	1. NN Factor
			0);		
	
	dsp_initclass();

	// Messages
	addint	((method) DoNN);
	addmess	((method) DoTattle,		"dblclick",	A_CANT, 0);
	addmess	((method) DoAssist,		"assist",	A_CANT, 0);
	addmess	((method) DoInfo,		"info",		A_CANT, 0);
	
	// MSP-Level messages
	LITTER_TIMEBOMB addmess	((method) BuildDSPChain, "dsp",		A_CANT, 0);

	// Initialize Litter Library
	LitterInit(kClassName, 0);
	
	}



#pragma mark -
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	NewBrowner(iNN)
 *	
 ******************************************************************************************/

void*
NewBrowner(
	long	iNN)
	
	{
	tBrowner*		me	= NIL;
	
	// Default NN value doesn't need massaging

	// Let Max/MSP allocate us, our inlets, and outlets.
	me = (tBrowner*) newobject(gBrownerClass);
	dsp_setup(&(me->coreObject), 1);				// Signal inlet for benefit of begin~
													// Otherwise left inlet does "NN" only
	outlet_new(me, "signal");
	
	// Set up object components
	DoNN(me, iNN);
	me->nextSeam = 0.0;

	return me;
	}

#pragma mark -
#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	DoNN(me, iNN)
 *	
 *	Set parameters, making sure nothing bad happens.
 *
 ******************************************************************************************/

void DoNN(
	tBrowner* me,
	long	iNN)
	
	{
	
	if (iNN <= 0) {
		me->nn		= 0;
		me->mask	= kULongMax;
		me->offset	= 0;
		}
	else {
		if (iNN > kMaxNN)
			iNN = kMaxNN;
		me->nn		= iNN;
		me->mask	= kULongMax << iNN;
		me->offset	= (~me->mask) >> 1;
		}
	
	}


/******************************************************************************************
 *
 *	DoTattle(me)
 *
 *	Post state information
 *
 ******************************************************************************************/

void
DoTattle(
	tBrowner* me)
	
	{
	
	post(	"%s state",
			kClassName);
	post(	"  NN factor is: %d (mask = %lx)",
			me->nn,
			me->mask);
	
	}


/******************************************************************************************
 *
 *	DoAssist
 *
 *	Fairly generic Assist Method
 *
 ******************************************************************************************/

void
DoAssist(
	tBrowner*,
	void*,					// We don't use this parameter
	long	iDir,
	long	iArgNum,
	char*	oCStr)
	
	{
	
	assist_string(	gStrResID, iDir, iArgNum, strIndexInNN, strIndexOutBrowner, oCStr);
	
	}

/******************************************************************************************
 *
 *	DoInfo(me)
 *
 *	Fairly basic Info Method
 *
 ******************************************************************************************/

void
DoInfo(
	tBrowner* me)
	
	{
	char	packMessage[sizeof(Str255)];
	
	
	GetVersStrings(gPackVersID, NIL, packMessage);
	
	ouchstring("%s object.\r%s.", kClassName, packMessage);
	DoTattle(me);
	
	}

#pragma mark -
#pragma mark • DSP Methods

/******************************************************************************************
 *
 *	BuildDSPChain(me, ioDSPVectors, iConnectCounts)
 *
 ******************************************************************************************/

void
BuildDSPChain(
	tBrowner*		me,
	t_signal**	ioDSPVectors,
	short*		/* iConnectCounts */)
	
	{
	
	dsp_add(
		PerformBrowner, 3,
		me, (long) ioDSPVectors[outletBrowner]->s_n, ioDSPVectors[outletBrowner]->s_vec
		);
	
	}
	

/******************************************************************************************
 *
 *	PerformBrowner(iParams)
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
PerformBrowner(
	int* iParams)
	
	{
	enum {
		paramFuncAddress	= 0,
		paramMe,
		paramVectorSize,
		paramOut,
		
		paramNextLink
		};
	
	const unsigned long		kSignBit	= 0x80000000;
	const double			kFactor		= 0.7071067812;
	
	long			vecCounter;
	unsigned long	mask;
	double			nextSeam,
					cachedGauss;
	Boolean			useCache;
	tSampleVector	outNoise;
	tBrowner*			me = (tBrowner*) iParams[paramMe];
	
	if (me->coreObject.z_disabled) goto exit;
	
	// Copy parameters into registers
	vecCounter	= (long) iParams[paramVectorSize];
	mask		= me->mask;
	nextSeam		= me->nextSeam;
	outNoise	= (tSampleVector) iParams[paramOut];
	useCache	= false;
	
		//
		// Voss random addition algorithm
		//
		
		// knnn takes buf[0] as it stands, initialize midpoint and endpoint
		outNoise[0]			= me->nextSeam;
		outNoise[stride]	= Long2Signal((long) Taus88(tausStuff));	// ASSERT: stride = cycle/2
		nextSeam			= Long2Signal((long) Taus88(tausStuff));
		
		// Recursive interpolation
		// Initial state: stride = cycle/2, offset = stride/2
		while (offset > 0) {
			int i;
			
			// Interpolate initial values at midpoints between values
			// calculated so far
			for (i = offset; i < cycle - offset; i += stride) 
			    outNoise[i] = 0.5 * (outNoise[i-offset] + outNoise[i+offset]);
			outnoise[i] = 0.5 * (outNoise[i-offset] + nextSeam);
			
			// Add noise with reduced variance
			scale *= hurstFac;
			for (i = offset; i < cycle; i += offset) {
				outnoise[i]	+= scale * ULong2Singal ( Taus88(tausStuff) );
				} 
			nextSeam += scale * ULong2Singal ( Taus88(tausStuff) );
			
			// Next generation: halve stride and offset
			stride = offset; 
			offset >>= 1;
		    }
		}
	
	
exit:
	return iParams + paramNextLink;
	}
