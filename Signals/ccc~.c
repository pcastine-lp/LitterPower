/*
	File:		ccc~.c

	Contains:	1/f noise, using the Schuster/Procaccia deterministic (chaotic) algorithm.

	Written by:	Peter Castine

	Copyright:	© 2001-2003 Peter Castine

	Change History (most recent first):

         <5>     11–1–04    pc      Update for Windows.
         <4>    7–7–2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <3>    6–3–2003    pc      Drop faux 'Vers' resource and related modifications. Add object
                                    version to DoInfo().
         <2>  28–11–2002    pc      Tidy up after initial check in.
         <1>  28–11–2002    pc      Initial Check-In.
		28-Apr-2002:		First implementation.
*/


/******************************************************************************************
	1/f noise can be created using random noise generators but it can also be producted
	using deterministic functions. One such method is a finite difference equation
	proposed by I. Procaccia and H. Schuster. It is simply

		x' = (x + x^2) mod 1
	
	References

	Schuster, H.G.
	Deterministic Chaos - An Introduction
	Physik verlag, Weinheim, 1984 

	Procaccia, I. and Schuster, H.G.
	Functional renormalisation group theory of universal 1/f noise in dynamical systems.
	Phys Rev 28 A, 1210-12 (1983) 
 ******************************************************************************************/

#pragma mark • Include Files

#include "LitterLib.h"

#include <float.h>


#pragma mark • Constants

const char*			kClassName		= "lp.ccc~";			// Class name


	// Indices for STR# resource
enum {
	strIndexInEnable		= lpStrIndexLastStandard + 1,
	
	strIndexOutPink
	};


#pragma mark • Type Definitions



#pragma mark • Object Structure

typedef struct {
	t_pxobject		coreObject;
	
	double			curVal;
	} tPink;


#pragma mark • Global Variables



#pragma mark • Function Prototypes

	// Class message functions
void*	NewPink(void);

	// Object message functions
static void DoTattle(tPink*);
static void	DoAssist(tPink*, void* , long , long , char*);
static void	DoInfo(tPink*);

	// MSP Messages
static void	BuildDSPChain(tPink*, t_signal**, short*);
static int*	PerformPink(int*);


#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark • Inline Functions



#pragma mark -

/******************************************************************************************
 *
 *	main()
 *	
 *	Standar Max/MSP External Object Entry Point Function
 *	
 ******************************************************************************************/

void
main(void)
	
	{
	LITTER_CHECKTIMEOUT(kClassName);
	
	// Standard Max/MSP initialization mantra
	setup(	&gObjectClass,				// Pointer to our class definition
			(method) NewPink,			// Instance creation function
			(method) dsp_free,			// Default deallocation function
			sizeof(tPink),				// Class object size
			NIL,						// No menu function
			A_NOTHING,					// No arguments
			0);		
	
	dsp_initclass();
	
	// Messages
	addmess	((method) DoTattle,		"dblclick",	A_CANT, 0);
	addmess	((method) DoTattle,		"tattle",	A_NOTHING);
	addmess	((method) DoAssist,		"assist",	A_CANT, 0);
	addmess	((method) DoInfo,		"info",		A_CANT, 0);
	
	// MSP-Level messages
	LITTER_TIMEBOMB addmess	((method) BuildDSPChain, "dsp",		A_CANT, 0);

	// Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();

	
	}



#pragma mark -
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	NewPink(iNN)
 *	
 *	Arguments: nn factor.
 *
 ******************************************************************************************/

void*
NewPink(void)
	
	{
	tPink*	me	= NIL;
	
	// Let Max/MSP allocate us, our inlets, and outlets.
	me = (tPink*) newobject(gObjectClass);
	dsp_setup(&(me->coreObject), 1);				// Signal inlet for benefit of begin~
	outlet_new(me, "signal");
	
	// Set up object components
	me->curVal = ULong2Unit_zo(MachineKharma());

	return me;
	}

#pragma mark -
#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	DoTattle(me)
 *
 *	Post state information
 *
 ******************************************************************************************/

void
DoTattle(
	tPink* me)
	
	{
	post("%s state", kClassName);
	post("  CurVal: %lf", me->curVal);
	}


/******************************************************************************************
 *
 *	DoAssist()
 *	DoInfo()
 *
 *	Generic Assist/Info methods
 *	
 *	Many parameters are not used.
 *
 ******************************************************************************************/

void DoAssist(tPink*, void*, long iDir, long iArgNum, char* oCStr)
	{ LitterAssist(iDir, iArgNum, strIndexInEnable, strIndexOutPink, oCStr); }

void DoInfo(tPink* me)
	{ LitterInfo(kClassName, &me->coreObject.z_ob, (method) DoTattle); }


#pragma mark -
#pragma mark • DSP Methods

/******************************************************************************************
 *
 *	BuildDSPChain(me, ioDSPVectors, iConnectCounts)
 *
 ******************************************************************************************/

void
BuildDSPChain(
	tPink*		me,
	t_signal**	ioDSPVectors,
	short*		/* iConnectCounts */)
	
	{
	enum {
		inletEnable	= 0,
		outletPink
		};
	
	dsp_add(
		PerformPink, 3,
		me, (long) ioDSPVectors[outletPink]->s_n, ioDSPVectors[outletPink]->s_vec
		);
	
	}
	

/******************************************************************************************
 *
 *	PerformPink(iParams)
 *
 *	Parameter block for PerformSync contains 8 values:
 *		- Address of this function
 *		- The performing object
 *		- Vector size
 *		- output signal
 *		- Imaginary output signal
 *
 ******************************************************************************************/

int*
PerformPink(
	int* iParams)
	
	{
	const double kMinPink = 1.0 / 525288.0;
	enum {
		paramFuncAddress	= 0,
		paramMe,
		paramVectorSize,
		paramOut,
		
		paramNextLink
		};
	
	long			vecCounter;
	tSampleVector	outNoise;
	tPink*			me = (tPink*) iParams[paramMe];
	double			curVal;
	
	if (me->coreObject.z_disabled) goto exit;
	
	// Copy parameters into registers
	vecCounter	= (long) iParams[paramVectorSize];
	outNoise	= (tSampleVector) iParams[paramOut];
	curVal		= me->curVal;
	
	// Sanity check... due to limitations in accuracy, we can die at very small values.
	// Only check once per PerformPink(), we can wait that long.
	// Also, we prefer to only "nudge" the value towards chaos... 
	if (curVal <= kMinPink) {
		if (curVal == 0.0)	curVal  = kMinPink;
		else				curVal += curVal;
		}
	
	// Do our stuff
	do {
		curVal = curVal * curVal + curVal;
		if (curVal >= 1.0)					// Cheaper than fmod(), and works quite nicely
			curVal -= 1.0;					// in the range of values that can occur.
		
		*outNoise++ = curVal;
		} while (--vecCounter > 0);
	
	me->curVal	= curVal;
	
exit:
	return iParams + paramNextLink;
	}
