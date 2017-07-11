/*
	File:		shhh.c

	Contains:	Max external object generating evenly distributed random values in the
				range [0 .. 1]. "White noise".

	Written by:	Peter Castine

	Copyright:	© 2001-2002 Peter Castine

	Change History (most recent first):

         <7>   26–4–2006    pc      Update for new LitterLib organization.
         <6>     15–1–04    pc      Avoid possible memory leak if seed argument is used.
         <5>     14–1–04    pc      Update for Windows.
         <4>    7–7–2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <3>  30–12–2002    pc      Drop faux 'Vers' resource and related modifications. Add object
                                    version to ShhhInfo().
         <2>  28–11–2002    pc      Tidy up after initial check-in.
         <1>  28–11–2002    pc      Initial check in.
		30-Jun-2001:	Modified to use Taus88 instead of TT800 as default generator.
						Also update to use new ULong2Unitxx() functions.
		14-Apr-2000:	Overhaul for LPP.
		2-Apr-2001:		First implementation.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "LitterLib.h"
#include "TrialPeriodUtils.h"
#include "MiscUtils.h"
#include "Taus88.h"
#include "UniformExpectations.h"

#pragma mark • Constants

const char*	kClassName		= "lp.shhh";			// Class name


const int	kMaxNN			= 31;

	// Indices for STR# resource
enum {
	strIndexInBang		= lpStrIndexLastStandard + 1,
	strIndexInNN,
	
	strIndexOutWhite
	};


#pragma mark • Type Definitions



#pragma mark • Object Structure

typedef struct {
	Object			coreObject;
	
	tTaus88DataPtr	tausData;
	
	int				nn;					// Number of bits to mask out
	unsigned long	nnMask,				// Values depends on nn
					nnOffset;
	} objWhite;


#pragma mark • Global Variables



#pragma mark • Function Prototypes

	// Class message functions
void*	ShhhNew(long, long);
void	ShhhFree	(objWhite*);

	// Object message functions
static void ShhhBang(objWhite*);
static void ShhhNN(objWhite*, long);
static void ShhhSeed(objWhite*, long);
static void ShhhTell(objWhite*, Symbol*, Symbol*);
static void ShhhTattle(objWhite*);
static void	ShhhAssist(objWhite*, void* , long , long , char*);
static void	ShhhInfo(objWhite*);


#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark • Inline Functions



#pragma mark -

/******************************************************************************************
 *
 *	main()
 *	
 *	Standard Max External Object Entry Point Function
 *	
 ******************************************************************************************/

void
main(void)
	
	{
	LITTER_CHECKTIMEOUT(kClassName);
	
	// Standard Max setup() call
	setup(	&gObjectClass,			// Pointer to our class definition
			(method) ShhhNew,		// Instance creation function
			(method) ShhhFree,		// Custom deallocation function
			sizeof(objWhite),			// Class object size
			NIL,					// No menu function
			A_DEFLONG,				// Optional arguments:	1. NN Factor
			A_DEFLONG,				// 						3. Seed value
			0);		
	

	// Messages
	LITTER_TIMEBOMB addbang	((method) ShhhBang);
	addmess	((method) ShhhNN,		"in1",		A_LONG);
	addmess ((method) ShhhSeed,		"seed",		A_DEFLONG, 0);
	addmess ((method) ShhhTell,		"tell",		A_SYM, A_SYM, 0);
	addmess	((method) ShhhTattle,	"dblclick",	A_CANT, 0);
	addmess	((method) ShhhTattle,	"tattle",	A_NOTHING);
	addmess	((method) ShhhAssist,	"assist",	A_CANT, 0);
	addmess	((method) ShhhInfo,		"info",		A_CANT, 0);
	
	// Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();
	
	}



#pragma mark -
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	ShhhFree(me)
 *	ShhhNew(iNN, iSeed)
 *	
 *	Arguments: nn factor, seed.
 *	
 *	If no seed is specified, use Taus88's own data pool. If max is 0 and min is positive, 
 *	set range to [0..min]. If both max and min are 0, set range to [0..1]. If min>max, no
 *	random numbers will be generated (output 0s instead).
 *
 ******************************************************************************************/

void ShhhFree(objWhite* me)
	{ Taus88Free(me->tausData); }							// Taus88Free is NIL-safe

void*
ShhhNew(
	long	iNN,
	long	iSeed)
	
	{
	objWhite*		me			= NIL;
	tTaus88DataPtr	myTausStuff	= NIL;	
	
	
	// Run through initialization parameters from right to left, handling defaults
	if (iSeed != 0) myTausStuff = Taus88New(iSeed);

	// Default NN value doesn't need massaging
	// Finished checking intialization parameters

	// Let Max allocate us, our inlets, and outlets.
	me = (objWhite*) newobject(gObjectClass);
	
	intin(me, 1);												// NN inlet
	
	floatout(me);								// Access through me->coreObject.o_outlet
	
	// Set up object components
	me->tausData = myTausStuff;
	ShhhNN(me, iNN);

	return me;
	}

#pragma mark -
#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	ShhhBang(me)
 *
 ******************************************************************************************/

void
ShhhBang(
	objWhite* me)
	
	{
	unsigned long white;
	
	white = Taus88(me->tausData);
	if (me->nn != 0) {
		white &= me->nnMask;
		white += me->nnOffset;
		}
	
	outlet_float(me->coreObject.o_outlet, ULong2Unit_ZO(white));
	
	}

/******************************************************************************************
 *
 *	ShhhNN(me, iNN)
 *	
 *	Set parameters, making sure nothing bad happens.
 *
 ******************************************************************************************/

void ShhhNN(
	objWhite* me,
	long	iNN)
	
	{
	
	if (iNN <= 0) {
		me->nn			= 0;
		me->nnMask		= kULongMax;
		me->nnOffset	= 0;
		}
	else {
		if (iNN > kMaxNN)
			iNN = kMaxNN;
		me->nn			= iNN;
		me->nnMask		= kULongMax << iNN;
		me->nnOffset	= (~me->nnMask) >> 1;
		}
	
	}


/******************************************************************************************
 *
 *	ShhhSeed(me, iSeed)
 *
 ******************************************************************************************/

void
ShhhSeed(
	objWhite*	me,
	long		iSeed)		// Gets treated as unsigned, even if negative was specified
	
	{
	
	Taus88Seed(me->tausData, (unsigned long) iSeed);
	
	}


/******************************************************************************************
 *
 *	ShhhTell(me, iTarget, iMsg)
 *
 ******************************************************************************************/

	static double DoExpect(objWhite* me, eExpectSelector iSel)
		{
		#pragma unused(me)
		
		return UniformExpectationsContinuous(0.0, 1.0, iSel);
		}

void ShhhTell(objWhite* me, Symbol* iTarget, Symbol* iMsg)
	{ LitterExpect((tExpectFunc) DoExpect, (Object*) me, iMsg, iTarget, TRUE); }


/******************************************************************************************
 *
 *	ShhhTattle(me)
 *
 *	Post state information
 *
 ******************************************************************************************/

void
ShhhTattle(
	objWhite* me)
	
	{
	
	post("%s state",
			kClassName);
	post("  NN factor is: %d (mask = 0x%lx, offset = 0x%lx)",
			me->nn, me->nnMask, me->nnOffset);
	
	}


/******************************************************************************************
 *
 *	ShhhAssist
 *
 *	Fairly generic Assist Method
 *
 ******************************************************************************************/

void
ShhhAssist(
	objWhite* me,
	void*	box,
	long	iDir,
	long	iArgNum,
	char*	oCStr)
	
	{
	#pragma unused(me, box)
	
	LitterAssist(iDir, iArgNum, strIndexInBang, strIndexOutWhite, oCStr);
	
	}

/******************************************************************************************
 *
 *	ShhhInfo(me)
 *
 *	Fairly basic Info Method
 *
 ******************************************************************************************/

void
ShhhInfo(
	objWhite* me)
	
	{
	
	LitterInfo(kClassName, &me->coreObject, (method) ShhhTattle);
	
	}

