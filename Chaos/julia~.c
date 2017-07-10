/*
	File:		julia~.c

	Contains:	"Julie" noise. Uses Taus88 algorithm for random values, also has
				unique NN factor.

	Written by:	Peter Castine

	Copyright:	© 2001-2002 Peter Castine

	Change History (most recent first):

         <6>    10–12–04    pc      Update to use proxy_getinlet()
         <5>     11–1–04    pc      Update for Windows (keep file up to conventions while I have
                                    the changes in my head).
         <4>   24–8–2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <3>  30–12–2002    pc      Drop faux 'Vers' resource and related modifications. Add object
                                    version to DoInfo().
         <2>  28–11–2002    pc      Tidy up after check in.
         <1>  28–11–2002   Demo     
         <1>  28–11–2002    pc      Initial check in.
		28-Mar-2002:	First implementation.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "LitterLib.h"	// Also #includes MaxUtils.h, ext.h
#include "z_dsp.h"


#pragma mark • Constants

const char*	kClassName		= "lp.julie~";			// Class name



	// Indices for STR# resource
enum {
	strIndexInZ0Real	= lpStrIndexLastStandard + 1,
	strIndexInZ0Imag,
	strIndexInCReal,
	strIndexInCImag,
	
	strIndexOutReal,
	strIndexOutImag,
	
	strIndexInLeft		= strIndexInZ0Real,
	strIndexOutLeft		= strIndexOutReal
	};

	// Indices for MSP Inlets and Outlets.
enum {
	inletZ0Real		= 0,
	inletZ0Imag,
	inletCReal,
	inletCImag,
	
	outletReal,
	outletImag
	};

#pragma mark • Type Definitions



#pragma mark • Object Structure

typedef struct {
	t_pxobject		coreObject;
	
	double			zReal,			// Coordinates of current point
					zImag,			// (cache between calls to Perform method)
					z0Real,			// Coordinates of starting point
					z0Imag,
					cReal,			// Offset parameter
					cImag;
	} tJulie;


#pragma mark • Global Variables


#pragma mark • Function Prototypes

	// Class message functions
void*	NewJulie(Symbol*, short, Atom*);

	// Object message functions
static void DoFloat(tJulie*, double);
static void DoReset(tJulie*);
static void DoTattle(tJulie*);
static void	DoAssist(tJulie*, void* , long , long , char*);
static void	DoInfo(tJulie*);

	// MSP Messages
static void	BuildDSPChain(tJulie*, t_signal**, short*);
static int*	PerformJulieDynamic(int*);
static int*	PerformJulieStatic(int*);


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
	
	// Standard Max/MSP initialization mantra
	setup(	&gObjectClass,				// Pointer to our class definition
			(method) NewJulie,			// Instance creation function
			(method) dsp_free,			// Default deallocation function
			sizeof(tJulie),				// Class object size
			NIL,						// No menu function
			A_GIMME,					// Optional arguments:	1. Real component of z[0]
										//						2. Imaginary component of z[0]
										//						3. Real component of offset param
										// 						4. Imag. comp. of offset param
			0);							// Amazingly, Max cannot handle fourfold A_DEFFLOAT
										// in an argument list.
	
	dsp_initclass();

	// Messages
	addfloat((method) DoFloat);
	addmess	((method) DoReset,	"reset",	A_NOTHING);
	
	addmess	((method) DoTattle,	"dblclick",	A_CANT, 0);
	addmess	((method) DoTattle,	"tattle",	A_NOTHING);
	addmess	((method) DoAssist,	"assist",	A_CANT, 0);
	addmess	((method) DoInfo,	"info",		A_CANT, 0);
	
	// MSP-Level messages
	addmess	((method) BuildDSPChain, "dsp",	A_CANT, 0);

	// Initialize LitterLit
	LitterInit(kClassName, 0);
	
	}



#pragma mark -
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	NewJulie(iName, iArgC, iArgV)
 *	
 ******************************************************************************************/

void*
NewJulie(
	Symbol*	,
	short	iArgC,
	Atom*	iArgV)
	
	{
	tJulie*	me	= NIL;
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

	// Let Max/MSP allocate us, our inlets, and outlets.
	me = (tJulie*) newobject(gObjectClass);
		if (me == NIL) goto punt;
	
	dsp_setup(&(me->coreObject), 4);				// All four inlets accept signal vectors

	outlet_new(me, "signal");						// Two outlets (for real and imaginary
	outlet_new(me, "signal");						// components of Julia set elements)
	
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
punt:
	return me;
	}

#pragma mark -
#pragma mark • Object Message Handlers

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

static void DoZ0Real(tJulie* me, double iZ0Real)
	{ me->z0Real = iZ0Real; DoReset(me); }
	
static void DoZ0Imag(tJulie* me, double iZ0Imag)
	{ me->z0Imag = iZ0Imag; }

static void DoCReal(tJulie* me, double iCReal)
	{ me->cReal = iCReal; }
	
static void DoCImag(tJulie* me, double iCImag)
	{ me->cImag = iCImag; }

void
DoFloat(
	tJulie*	me,
	double	iVal)
	
	{
	
	switch ( ObjectGetInlet((Object*) me, me->coreObject.z_in) ) {
		case 0:
			DoZ0Real(me, iVal);
			break;
		case 1:
			DoZ0Imag(me, iVal);
			break;
		case 2:
			DoCReal(me, iVal);
			break;
		case 3:
			DoCImag(me, iVal);
			break;
		default:
			// This can't happen...
			break;
		}
	
	}

void
DoReset(
	tJulie* me)
	
	{
	me->zReal = me->z0Real;
	me->zImag = me->z0Imag;
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
	tJulie*		me,
	t_signal**	iDSPVectors,
	short*		iConnectCounts)
	
	{
	
	if (iConnectCounts[outletReal] == 0 && iConnectCounts[outletImag] == 0)
		return;
	
	if ( iConnectCounts[inletZ0Real] || iConnectCounts[inletZ0Imag]
			|| iConnectCounts[inletCReal] || iConnectCounts[inletCImag] ) {
	
		dsp_add(
			PerformJulieDynamic, 8,
			me, (long) iDSPVectors[outletReal]->s_n,
			iConnectCounts[inletZ0Real]	? iDSPVectors[inletZ0Real]->s_vec	: NIL,
			iConnectCounts[inletZ0Imag]	? iDSPVectors[inletZ0Imag]->s_vec	: NIL,
			iConnectCounts[inletCReal]	? iDSPVectors[inletCReal]->s_vec	: NIL,
			iConnectCounts[inletCImag]	? iDSPVectors[inletCImag]->s_vec	: NIL,
			iConnectCounts[outletReal]	? iDSPVectors[outletReal]->s_vec	: NIL,
			iConnectCounts[outletImag]	? iDSPVectors[outletImag]->s_vec	: NIL
			);
		
		}
	
	else {
		
		dsp_add(
			PerformJulieStatic, 4,
			me, (long) iDSPVectors[outletReal]->s_n,
			iConnectCounts[outletReal] ? iDSPVectors[outletReal]->s_vec : NIL,
			iConnectCounts[outletImag] ? iDSPVectors[outletImag]->s_vec : NIL
			);
		
		}
	
	}
	

/******************************************************************************************
 *
 *	PerformJulieDynamic(iParams)
 *	PerformJulieStatic(iParams)
 *
 *	Parameter block contains the following values values:
 *		- Address of the function
 *		- The performing julie~ object
 *		- Vector size
 *		- output signal (real component)
 *		- output signal (imaginary component)
 *	  and, for the Dynamic version,
 *		- Input Z0 signal (real and imaginary components)
 *		- Input C signal (real and imaginary components)
 *	  tagged at the end of poth parameter lists by
 *		- Address of the next link in the parameter chain
 *
 ******************************************************************************************/

int*
PerformJulieStatic(
	int* iParams)
	
	{
	enum {
		paramFuncAddress	= 0,
		paramMe,
		paramVectorSize,
		paramOutReal,
		paramOutImag,
		
		paramNextLink
		};
	
	long			vecSize;
	tSampleVector	outReal,
					outImag;
	double			curReal,
					curImag,
					cReal,
					cImag;
	tJulie*			me = (tJulie*) iParams[paramMe];
	
	if (me->coreObject.z_disabled) goto exit;
	
	// Copy parameters into registers
	vecSize	= (long) iParams[paramVectorSize];
	outReal	= (tSampleVector) iParams[paramOutReal];
	outImag = (tSampleVector) iParams[paramOutImag];
	curReal	= me->zReal;
	curImag = me->zImag;
	cReal	= me->cReal;
	cImag	= me->cImag;
	
	do {
		double	r	= curReal,
				i	= curImag;
		
		// Complex z' = z^2 + c
		*outReal++ = curReal = r * r - i * i + cReal;
		*outImag++ = curImag = 2 * r * i + cImag;
		
		} while (--vecSize > 0);
	
	// Save last values produced
	me->zReal = curReal;
	me->zImag = curImag;
	
exit:
	return iParams + paramNextLink;
	}


int*
PerformJulieDynamic(
	int* iParams)
	
	{
	enum {
		paramFuncAddress	= 0,
		paramMe,
		paramVectorSize,
		paramInZReal,
		paramInZImag,
		paramInCReal,
		paramInCImag,
		paramOutReal,
		paramOutImag,
		
		paramNextLink
		};
	
	long			vecSize;
	tSampleVector	inZReal,
					inZImag,
					inCReal,
					inCImag,
					outReal,
					outImag;
	double			zReal,
					zImag,
					cReal,
					cImag;
	tJulie*			me = (tJulie*) iParams[paramMe];
	
	if (me->coreObject.z_disabled) goto exit;
	
	// Copy parameters into registers
	vecSize	= (long) iParams[paramVectorSize];
	inZReal	= (tSampleVector) iParams[paramInZReal];
	inZImag	= (tSampleVector) iParams[paramInZImag];
	inCReal	= (tSampleVector) iParams[paramInCReal];
	inCImag	= (tSampleVector) iParams[paramInCImag];
	outReal	= (tSampleVector) iParams[paramOutReal];
	outImag	= (tSampleVector) iParams[paramOutImag];
	zReal	= me->zReal;
	zImag	= me->zImag;
	cReal	= me->cReal;
	cImag	= me->cImag;
	
	do {
		double	r	= inZReal ? *inZReal++ : zReal,
				i	= inZImag ? *inZImag++ : zImag,
				cr	= inCReal ? *inCReal++ : cReal,
				ci	= inCImag ? *inCImag++ : cImag;
		
		// Complex z' = z^2 + c
		*outReal++ = zReal = r * r - i * i + cr;
		*outImag++ = zImag = 2 * r * i + ci;
		
		} while (--vecSize > 0);
	
	// Save last values produced
	me->zReal = zReal;
	me->zImag = zImag;
	
exit:
	return iParams + paramNextLink;
	}
