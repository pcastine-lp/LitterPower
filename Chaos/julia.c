/*
	File:		julia.c

	Contains:	Max external object generating sequences of points from the Julia set.

	Written by:	Peter Castine

	Copyright:	© 2002 Peter Castine

	Change History (most recent first):

         <5>     11–1–04    pc      Update for Windows (keep file up to conventions while I have
                                    the changes in my head).
         <4>   24–8–2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <3>  30–12–2002    pc      Drop faux 'Vers' resource and related modifications. Add object
                                    version to DoInfo().
         <2>  28–11–2002    pc      Tidy up after check in.
         <1>  28–11–2002    pc      Initial check in.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "LitterLib.h"


#pragma mark • Constants

const char*	kClassName		= "lp.julie";			// Class name
const char*	kPackageName	= "lp.julie Package";	// Package name for 'vers'(2) resource



	// Indices for STR# resource
enum {
	strIndexInBang		= lpStrIndexLastStandard + 1,
	strIndexInZ0Imag,
	strIndexInCReal,
	strIndexInCImag,
	
	strIndexOutReal,
	strIndexOutImag,
	
	strIndexInLeft		= strIndexInBang,
	strIndexOutLeft		= strIndexOutReal
	};


#pragma mark • Type Definitions



#pragma mark • Object Structure

typedef struct {
	Object			coreObject;
	voidPtr			outImag;			// Main outlet accessed through coreObject.o_outlet
	
	double			zReal,				// Coordinates of current point
					zImag,
					z0Real,				// Coordinates of starting point
					z0Imag,
					cReal,				// Offset parameter
					cImag;
	
	} tJulie;


#pragma mark • Global Variables


#pragma mark • Function Prototypes

	// Class message functions
void*	NewJulie(Symbol*, short, Atom*);

	// Object message functions
static void DoBang(tJulie*);
static void DoZ0Real(tJulie*, double);
static void DoZ0Imag(tJulie*, double);
static void DoCReal(tJulie*, double);
static void DoCImag(tJulie*, double);
static void DoReset(tJulie*);
static void DoTattle(tJulie*);
static void	DoAssist(tJulie*, void* , long , long , char*);
static void	DoInfo(tJulie*);


#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark • Inline Functions



#pragma mark -

/******************************************************************************************
 *
 *	main()
 *	
 *	Standar Max External Object Entry Point Function
 *	
 ******************************************************************************************/

void
main(void)				// Parameter is obsolete (68k legacy)
	
	{
	
	// Standard Max setup() call
	setup(	&gObjectClass,				// Pointer to our class definition
			(method) NewJulie,			// Instance creation function
			NIL,						// No custom deallocation function
			sizeof(tJulie),				// Class object size
			NIL,						// No menu function
			A_GIMME,					// Optional arguments:	1. Real component of z[0]
										//						2. Imaginary component of z[0]
										//						3. Real component of offset param
										// 						4. Imag. comp. of offset param
			0);							// Amazingly, Max cannot handle fourfold A_DEFFLOAT
										// in an argument list.
		
	// Messages
	addbang	((method) DoBang);
	addfloat((method) DoZ0Real);
	addftx	((method) DoZ0Imag,	1);
	addftx	((method) DoCReal,	2);
	addftx	((method) DoCImag,	3);
	addmess	((method) DoReset,	"reset",	A_NOTHING);
	
	addmess	((method) DoTattle,	"dblclick",	A_CANT, 0);
	addmess	((method) DoTattle,	"tattle",	A_NOTHING);
	addmess	((method) DoAssist,	"assist",	A_CANT, 0);
	addmess	((method) DoInfo,	"info",		A_CANT, 0);
	
	// Initialize LitterLib
	LitterInit(kClassName, 0);
	
	}



#pragma mark -
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	NewJulie(iName, iArgC, iArgV)
 *	
 *	Arguments: z0 (real and imaginary components) and c (real and imaginary components)
 *	
 ******************************************************************************************/

void*
NewJulie(
	Symbol*	,
	short	iArgC,
	Atom*	iArgV)
	
	{
	tJulie*	me		= NIL;
	double	z0Real	= 0.0, 
			z0Imag	= 0.0, 
			cReal	= 0.0, 
			cImag	= 0.0;
	
	// Take all initialization arguments as passed to us.
	// This is a little dull for the Julia set, but there is no "natural" alternative
	switch (iArgC) {
		default:
			error("%s: ignoring spurious arguments", kClassName);
			// fall into next case...
		case 4:
			if ( ParseAtom(iArgV + 3, false, true, 0, NIL, kClassName) )
				cImag = iArgV[3].a_w.w_float;
			// fall into next case...
		case 3:
			if ( ParseAtom(iArgV + 2, false, true, 0, NIL, kClassName) )
				cReal = iArgV[2].a_w.w_float;
			// fall into next case...
		case 2:
			if ( ParseAtom(iArgV + 1, false, true, 0, NIL, kClassName) )
				z0Imag = iArgV[1].a_w.w_float;
			// fall into next case...
		case 1:
			if ( ParseAtom(iArgV, false, true, 0, NIL, kClassName) )
				z0Real = iArgV[0].a_w.w_float;
			// fall into next case...
		case 0:
			break;
		}
	
	// Let Max allocate us, our inlets, and outlets
	me = (tJulie*) newobject(gObjectClass);
		if (me == NIL) goto punt;
	
		// Inlets, from right to left
	floatin(me, 3);
	floatin(me, 2);
	floatin(me, 1);
	
		// Two outlets, from right to left
	me->outImag	= floatout(me);
	floatout(me);					// Accessed through me->coreObject.o_outlet
	
	//
	// Store initial values
	//
	me->zReal		= me->z0Real
					= z0Real;
	me->zImag		= me->z0Imag
					= z0Imag;
	me->cReal		= cReal;
	me->cImag		= cImag;
	
	//
	// All done
	//
	
	return me;

punt:
	// Oops... cheesy exception handling
	error("Insufficient memory to create %s object.", kClassName);
	if (me) freeobject((Object*) me);
	return NIL;
	}


#pragma mark -
#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	DoOut(me)
 *	DoNext(me)
 *	DoBang(me)
 *
 ******************************************************************************************/

static void
DoOut(
	tJulie* me)
	
	{
	outlet_float(me->outImag, me->zImag);
	outlet_float(me->coreObject.o_outlet, me->zReal);
	}

static void
DoNext(
	tJulie* me)
	
	{
	double	r	= me->zReal,
			i	= me->zImag;
	
	// Complex z' = z^2 + c
	me->zReal = r * r - i * i + me->cReal;
	me->zImag = 2 * r * i + me->cImag;
	}
	
static void
DoBang(
	tJulie* me)
	
	{
	DoNext(me);
	DoOut(me);
	}

/******************************************************************************************
 *
 *	DoZ0Real(me, iZ0Real)
 *	DoZ0Imag(me, iZ0Imag)
 *	DoCReal(me, iCReal)
 *	DoCImag(me, iCImag)
 *	
 *	Set parameters, making sure nothing bad happens.
 *	
 *	Side effect of DoZ0Real is to reset the current point coordinates
 *
 ******************************************************************************************/

static void DoReset(tJulie* me)
	{ me->zReal = me->z0Real; me->zImag = me->z0Imag; }

static void DoZ0Real(tJulie* me, double iZ0Real)
	{ me->z0Real = iZ0Real; DoReset(me); }
	
static void DoZ0Imag(tJulie* me, double iZ0Imag)
	{ me->z0Imag = iZ0Imag; }

static void DoCReal(tJulie* me, double iCReal)
	{ me->cReal = iCReal; }
	
static void DoCImag(tJulie* me, double iCImag)
	{ me->cImag = iCImag; }


/******************************************************************************************
 *
 *	DoTattle(me)
 *
 *	Post state information
 *
 ******************************************************************************************/

void
DoTattle(
	tJulie* me)
	
	{
	
	post("%s state", kClassName);
	post("  Initial point is: (%lf, %lf)",
			me->z0Real, me->z0Imag);
	post("  Offset point is: (%lf, %lf)",
			me->cReal, me->cImag);
	post("  Current point is: (%lf, %lf)",
			me->zReal, me->zImag);
	
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

void DoAssist(tJulie*, void*, long iDir, long iArgNum, char* oCStr)
	{ LitterAssist(iDir, iArgNum, strIndexInLeft, strIndexOutLeft, oCStr); }

void DoInfo(tJulie* me)
	{ LitterInfo(kClassName, &me->coreObject, (method) DoTattle); }


