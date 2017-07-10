/*
	File:		grrr.c

	Contains:	Max external object generating evenly distributed random values in the
				range [0 .. 1] with "Gray noise" distribution.

	Written by:	Peter Castine

	Copyright:	© 2001-2002 Peter Castine

	Change History (most recent first):

         <7>   26–4–2006    pc      Update for new LitterLib organization.
         <6>     15–1–04    pc      Avoid possible memory leak if seed argument is used.
         <5>     14–1–04    pc      Update for Windows.
         <4>    7–7–2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <3>  30–12–2002    pc      Drop faux 'Vers' resource and related modifications. Add object
                                    version to DoInfo().
         <2>  28–11–2002    pc      Tidy up after initial check-in.
         <1>  28–11–2002    pc      Initial check in.
		30-Jun-2001:	Modified to use Taus88 instead of TT800 as default generator.
						Also update to use new ULong2Unitxx() functions.
		15-Apr-2000:	First implementation.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "LitterLib.h"
#include "TrialPeriodUtils.h"
#include "MiscUtils.h"
#include "Taus88.h"


#pragma mark • Constants

const char*	kClassName		= "lp.grrr";			// Class name



	// Indices for STR# resource
enum {
	strIndexInBang		= lpStrIndexLastStandard + 1,
	
	strIndexOutGray
	};


#pragma mark • Type Definitions



#pragma mark • Object Structure

typedef struct {
	Object			coreObject;
	
	tTaus88DataPtr	tausData;
	
	unsigned long	prev;				// Value depends on nn
	} tGray;


#pragma mark • Global Variables



#pragma mark • Function Prototypes

	// Class message functions
void*	GrrrNew	(long);
void	GrrrFree(tGray*);

	// Object message functions
static void GrrrBang	(tGray*);
static void GrrrSeed	(tGray*, long);
static void GrrrTattle	(tGray*);
static void	GrrrAssist	(tGray*, void* , long , long , char*);
static void	GrrrInfo	(tGray*);


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
			(method) GrrrNew,		// Instance creation function
			(method) GrrrFree,		// Custom deallocation function
			sizeof(tGray),			// Class object size
			NIL,					// No menu function
			A_DEFLONG,				// Optional arguments:	1. Seed value
			0);		
	

	// Messages
	LITTER_TIMEBOMB addbang	((method) GrrrBang);
	addmess ((method) GrrrSeed,		"seed",		A_DEFLONG, 0);
	addmess	((method) GrrrTattle,	"dblclick",	A_CANT, 0);
	addmess	((method) GrrrTattle,	"tattle",	A_NOTHING);
	addmess	((method) GrrrAssist,	"assist",	A_CANT, 0);
	addmess	((method) GrrrInfo,		"info",		A_CANT, 0);
	
	//Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();
	
	}



#pragma mark -
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	GrrrFree(me)
 *	GrrrNew(iNN, iSeed)
 *	
 *	Arguments: nn factor, seed.
 *	
 *	If no seed is specified, use Taus88's own data pool. If max is 0 and min is positive, 
 *	set range to [0..min]. If both max and min are 0, set range to [0..1]. If min>max, no
 *	random numbers will be generated (output 0s instead).
 *
 ******************************************************************************************/

void GrrrFree(tGray* me)
	{ Taus88Free(me->tausData); }							// Taus88Free is NIL-safe

void*
GrrrNew(
	long	iSeed)
	
	{
	tGray*			me			= NIL;
	tTaus88DataPtr	myTausStuff	= NIL;	
	
	
	// Run through initialization parameters from right to left, handling defaults
	if (iSeed == 0) Taus88Init();
	else {
		myTausStuff = Taus88New(iSeed);
		}
	// Finished checking intialization parameters

	// Let Max allocate us, and andour outlet. We (currently) only have the default inled
	me = (tGray*) newobject(gObjectClass);
		
	floatout(me);								// Access through me->coreObject.o_outlet
	
	// Set up object components
	me->tausData = myTausStuff;
	me->prev = Taus88(myTausStuff);

	return me;
	}

#pragma mark -
#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	GrrrBang(me)
 *
 ******************************************************************************************/

void
GrrrBang(
	tGray* me)
	
	{
	unsigned long	gray = me->prev,
					mask;
	
	mask = Taus88(me->tausData);
	mask >>= mask & 0x0f;
	gray ^= mask;
	
	me->prev = gray;
	outlet_float(me->coreObject.o_outlet, ULong2Unit_ZO(gray));
	}

/******************************************************************************************
 *
 *	GrrrSeed(me, iSeed)
 *
 ******************************************************************************************/

void
GrrrSeed(
	tGray*	me,
	long		iSeed)		// Gets treated as unsigned, even if negative was specified
	
	{
	
	Taus88Seed(me->tausData, (unsigned long) iSeed);
	
	}


/******************************************************************************************
 *
 *	GrrrTattle(me)
 *
 *	Post state information
 *
 ******************************************************************************************/

void
GrrrTattle(
	tGray* me)
	
	{
	
	post(	"%s state",
			kClassName);
	post(	"  previous seed: %ld",
			me->prev);
	
	}


/******************************************************************************************
 *
 *	GrrrAssist
 *
 *	Fairly generic Assist Method
 *
 ******************************************************************************************/

void
GrrrAssist(
	tGray*	me,
	void*	box,
	long	iDir,
	long	iArgNum,
	char*	oCStr)
	
	{
	#pragma unused(me, box)
	
	LitterAssist(iDir, iArgNum, strIndexInBang, strIndexOutGray, oCStr);
	}

/******************************************************************************************
 *
 *	GrrrInfo(me)
 *
 *	Fairly basic Info Method
 *
 ******************************************************************************************/

void
GrrrInfo(
	tGray* me)
	
	{
	
	LitterInfo(kClassName, &me->coreObject, (method) GrrrTattle);
	
	}

