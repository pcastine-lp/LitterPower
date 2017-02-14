/*
	File:		qvvv~.c

	Contains:	Variable-colored noise. Uses Taus88 algorithm for random values, also has
				unique NN factor. New version does all arithmetic with floating point
				and supports float input for NN factor.

	Written by:	Peter Castine

	Copyright:	© 2001-2008 Peter Castine

	Change History (most recent first):

*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark ¥ Include Files

#include <stdlib.h>		// For rand(), RAND_MAX

#include "LitterLib.h"	// Also #includes MaxUtils.h, ext.h
#include "TrialPeriodUtils.h"
#include "Taus88.h"
#include "MiscUtils.h"

#include "z_dsp.h"


#pragma mark ¥ Constants

const char*	kClassName		= "lp.qvvv~";			// Class name

	// We used to define these as const int, but that causes GCC (XCode) to barf.
	// enum seems to work with all compilers we've tried so far, so we'll go with it.
enum {
	kMaxBuf		= 4096, 
	kBufferSize
	};

const double	kMaxNN		= 31.0;
const int		kIterations	= 12;					// log2(kMaxBuf)


	// Indices for STR# resource
enum {
	strIndexInHurst		= lpStrIndexLastStandard + 1,
	strIndexInNN,
	strIndexOuobjBrown,
	
	strIndexInLeft		= strIndexInHurst,
	strIndexOutLeft		= strIndexOuobjBrown
	};


#pragma mark ¥ Type Definitions



#pragma mark ¥ Object Structure

typedef struct {
	t_pxobject		coreObject;
	
	double		hurstExp,
				hurstFac,
				hurstGain;
	eActionType	action;
	
	int			bufPos;
	float		nn,							// Number of bits to mask out
				factor,						// Values depends on nn
				factor1,
				offset;
	float		buffer[kBufferSize];
	} objBrown;


#pragma mark ¥ Global Variables



#pragma mark ¥ Function Prototypes

	// Class message functions
void*	Pvvv2New(double, double);

	// Object message functions
static void Pvvv2Hurst(objBrown*, double);
static void Pvvv2IntNN(objBrown*, long);
static void Pvvv2FloatNN(objBrown*, double);
static void Pvvv2Stet(objBrown*);
static void Pvvv2Clip(objBrown*);
static void Pvvv2Wrap(objBrown*);
static void Pvvv2Reflect(objBrown*);
static void Pvvv2Tattle(objBrown*);
static void	Pvvv2Assist(objBrown*, void* , long , long , char*);
static void	Pvvv2Info(objBrown*);

	// MSP Messages
static void	Pvvv2DSP(objBrown*, t_signal**, short*);
static int*	Pvvv2Perform(int*);


#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark ¥ Inline Functions



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
	setup(	&gObjectClass,				// Pointer to our class definition
			(method) Pvvv2New,			// Instance creation function
			(method) dsp_free,			// Default deallocation function
			sizeof(objBrown),				// Class object size
			NIL,						// No menu function
										// Optional arguments:
			A_DEFFLOAT,					//		-- Hurst factor
			A_DEFLONG,					//		-- NN Factor
			0);		
	
	dsp_initclass();

	// Messages
	addfloat((method) Pvvv2Hurst);
	addftx	((method) Pvvv2FloatNN, 1);
	addinx	((method) Pvvv2IntNN, 1);
	
	// Range correction
	addmess ((method) Pvvv2Clip,		"clip",		A_NOTHING);
	addmess ((method) Pvvv2Wrap,		"wrap",		A_NOTHING);
	addmess ((method) Pvvv2Reflect,	"reflect",	A_NOTHING);
	addmess ((method) Pvvv2Stet,		"stet",		A_NOTHING);
	
	// Information messages
	addmess	((method) Pvvv2Tattle,	"dblclick",	A_CANT, 0);
	addmess	((method) Pvvv2Tattle,	"tattle",	A_NOTHING);
	addmess	((method) Pvvv2Assist,	"assist",	A_CANT, 0);
	addmess	((method) Pvvv2Info,		"info",		A_CANT, 0);
	
	// MSP-Level messages
	LITTER_TIMEBOMB addmess	((method) Pvvv2DSP,		"dsp",		A_CANT, 0);

	//Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();
	
	}



#pragma mark -
#pragma mark ¥ Class Message Handlers

/******************************************************************************************
 *
 *	Pvvv2New(iNN)
 *	
 ******************************************************************************************/

	static void InitBuf(objBrown* me)
		{
		int i;
		
		for (i = 0; i < kMaxBuf; i += 1)
			me->buffer[i] = 0;
		me->buffer[kMaxBuf] = Taus88Signal(NIL) / 3.0;			// Dividing by  3 is a fudge
	
		me->bufPos = kMaxBuf;
		
		}

void*
Pvvv2New(
	double	iHurst,
	double	iNN)
	
	{
	objBrown*		me	= NIL;
	
	// Default NN value doesn't need massaging
	// Ditto for Hurst factor

	// Let Max/MSP allocate us, our inlets, and outlets.
	me = (objBrown*) newobject(gObjectClass);
	dsp_setup(&(me->coreObject), 1);				// Signal inlet for benefit of begin~
													// Otherwise left inlet does "NN" only
													// or, with the multi-color object, the
													// Hurst factor
	floatin(me, 1);
	outlet_new(me, "signal");
	
	// Set up object components
	InitBuf(me);
	Pvvv2FloatNN(me, iNN);
	Pvvv2Hurst(me, iHurst);

	return me;
	}

#pragma mark -
#pragma mark ¥ Object Message Handlers

/******************************************************************************************
 *
 *	Pvvv2NN(me, iNN)
 *	Pvvv2Hurst(me, iHurst)
 *	
 *	Set parameters, making sure nothing bad happens.
 *
 ******************************************************************************************/

void Pvvv2IntNN(objBrown* me, long iNN)
	{ Pvvv2FloatNN(me, (double) iNN); }

void Pvvv2FloatNN(
	objBrown* me,
	double	iNN)
	
	{
	if (iNN <= 0.0) {
		me->nn		= 0.0;
			// These values are never used
		me->factor	= 0.0;
		me->factor1	= 0.0;
		me->offset	= 0.0;
		}
	else {
		if (iNN > kMaxNN)
			iNN = kMaxNN;
			
		me->nn		= iNN;
		
		if (iNN < 16.0) {
			// In the range nn in (0..8) we would be demanding higher effective bit signal
			// resolutions (24 to 32 bit) then floating point signals offer (assuming the
			// signal is in the standard range [-1..1]). In order for nn factors in this
			// range to have any effect on the signal at all, we map the entire range
			// (0..16) to (8..16), ie resolutions of 24 to 16 bits/sample. Additionally,
			// we choose to use a parabolic mapping chosen such that the "seam" between
			// the ranges (0..16) and [16..31] is continuous (even at the first derivative).
			// All of this is to some extent overkill, since the effect of the nn factor
			// in this range is barely audible. Nevertheless, in case this object's signal
			// is further processed, it seems appropriate to take these precautions
			iNN /= 16.00;
			iNN *= iNN;
			iNN *= 8.0;
			iNN += 8.0;
			}
		
		me->factor	= pow(2.0, 31.0 - iNN);
		me->factor1 = 1.0 / me->factor,
		me->offset	= 0.5 * me->factor1;
		}
	
	}

void Pvvv2Hurst(
	objBrown* me,
	double	iHurst)
	
	{
	int		i;					// counter
	double	hf,					// Hurst factor
			xn,					// Hurst factor taken to the nth power
			power;				// Expected signal power if initial gain == 1.0
	
	
	// RMS calculation of expected power sum
	hf		= pow(0.5, iHurst);
	power	= xn = 1.0;
	for (i = kIterations - 1; i > 0; i -= 1) {
		xn		*= hf;
		power	+= xn * xn;
		}

	me->hurstExp	= iHurst;
	me->hurstFac	= hf;
	me->hurstGain	= 1.0 / sqrt(power);
	
	}


/******************************************************************************************
 *
 *	Pvvv2Clip(me);
 *	Pvvv2Wrap(me);
 *	Pvvv2Reflect(me);
 *	Pvvv2Stet(me);
 *
 ******************************************************************************************/

void Pvvv2Clip(objBrown* me)		{ me->action = actClip; }
void Pvvv2Wrap(objBrown* me)		{ me->action = actWrap; }
void Pvvv2Reflect(objBrown* me)		{ me->action = actReflect; }
void Pvvv2Stet(objBrown* me)		{ me->action = actStet; }


/******************************************************************************************
 *
 *	Pvvv2Tattle(me)
 *
 *	Post state information
 *
 ******************************************************************************************/

void
Pvvv2Tattle(
	objBrown* me)
	
	{
	int		bufPos = me->bufPos;
	char*	actString;
	
	post("%s state", kClassName);
	post("	Hurst exponent is: %lf (Hurst factor: %lf, Hurst gain: %lf)",
			me->hurstExp, me->hurstFac, me->hurstGain);
	post("  NN factor is: %lf (factor = %lf, inverse factor = %lf offset = %lf)",
			(double) me->nn, (double) me->factor, (double) me->factor1, (double) me->offset);
	
	switch (me->action) {
		default:			actString = "stet -- no action";	break;
		case actClip:		actString = "clip";					break;
		case actWrap:		actString = "wrap";					break;
		case actReflect:	actString = "reflect";				break;
		}
	post("  Range handling action: %ld (%s)", (long) me->action, actString);
	
	post("  Buffer starts at address 0x%lx",
			me->buffer);
	post("  Current buffer position: %ld, containing %lf",
			(long) bufPos, (double) me->buffer[bufPos]);

	}


/******************************************************************************************
 *
 *	Pvvv2Assist()
 *	Pvvv2Info()
 *
 *	Fairly generic Assist/Info methods
 *
 ******************************************************************************************/

void Pvvv2Assist(objBrown* me, void* box, long iDir, long iArgNum, char* oCStr)
	{
	#pragma unused(me, box)
	
	LitterAssist(iDir, iArgNum, strIndexInLeft, strIndexOutLeft, oCStr);
	}

void Pvvv2Info(objBrown* me)
	{ LitterInfo(kClassName, &me->coreObject.z_ob, (method) Pvvv2Tattle); }



#pragma mark -
#pragma mark ¥ DSP Methods

/******************************************************************************************
 *
 *	Pvvv2DSP(me, ioDSPVectors, iConnectCounts)
 *
 ******************************************************************************************/

void
Pvvv2DSP(
	objBrown*		me,
	t_signal**	ioDSPVectors,
	short*		connectCounts)
	
	{
	#pragma unused(connectCounts)

	enum {
		inleobjBrown		= 0,
		outleobjBrown
		};

	
	dsp_add(
		Pvvv2Perform, 3,
		me, (long) ioDSPVectors[outleobjBrown]->s_n, ioDSPVectors[outleobjBrown]->s_vec
		);
	
	}
	

/******************************************************************************************
 *
 *	Pvvv2Perform(iParams)
 *
 *	Parameter block for PerformSync contains 8 values:
 *		- Address of this function
 *		- The performing schhh~ object
 *		- Vector size
 *		- output signal
 *		- Imaginary output signal
 *
 ******************************************************************************************/

	static void GenerateNewBuffer(objBrown* me)
		{
		const double hurstFac	= me->hurstFac;
		
		double	scale		= me->hurstGain;
		float*	buf			= me->buffer;
		int		stride		= kMaxBuf / 2,
				offset		= kMaxBuf / 4;
		
		//
		// Voss random addition algorithm
		//
		
		// pfff takes buf[0] as it stands, initialize midpoint and endpoint
		// (Inheriting buf[0] from the previous run and leaving it unmodified is the main
		// modification to the Voss algorithm made in this implementation.)
		//
		buf[0]			= buf[kMaxBuf];
		buf[stride]		= scale * Taus88Signal(NIL);
		buf[kMaxBuf]	= scale * Taus88Signal(NIL);
		
		// Recursive interpolation
		// Initial state: stride = cycle/2, offset = stride/2
		while (offset > 0) {
			int i;
			
			// Interpolate initial values at midpoints between values
			// calculated so far
			// Micro-optimization: original loop termination condition was
			// (i <= kMaxBuf - offset)
			// The condition (i < kMaxBuf) terminates for identical values of i and shaves
			// off a few cycles
			// Add noise with reduced variance while we're at it
			scale *= hurstFac;
			for (i = offset; i < kMaxBuf; i += stride) 
				buf[i] = 0.5 * (buf[i-offset] + buf[i+offset]) + scale * Taus88Signal(NIL);
			
			// Next generation: halve stride and offset
			stride = offset;								// ASSERT: offset == stride/2
			offset /= 2;									// Again, let compiler optimize.
			}
		
		me->bufPos = 0;
		}
	
int*
Pvvv2Perform(
	int* iParams)
	
	{
	enum {
		paramFuncAddress	= 0,
		paramMe,
		paramVectorSize,
		paramOut,
		
		paramNextLink
		};
	
	float*			curSamp;
	long			vecSize;
	tSampleVector	outNoise;			// Do integer arithmetic in buffer, then convert
										// to floating point before exit.
	objBrown*			me = (objBrown*) iParams[paramMe];
	
	if (me->coreObject.z_disabled) goto exit;
	
	// Copy parameters into registers
	vecSize		= (long) iParams[paramVectorSize];
	outNoise	= (tSampleVector) iParams[paramOut];
	
	// Time to generate new buffer?
	// Condition must also take possibility of vector size changing mid-buffer
	if (me->bufPos + vecSize > kMaxBuf) {
		GenerateNewBuffer(me);
		}
	curSamp	= me->buffer + me->bufPos;
	me->bufPos += vecSize;
	
	// Do we have to deal with NN factor and/or Range actions?
	switch(me->action) {
	default:
		// actStet
		if (me->nn == 0)
			do { *outNoise++ = *curSamp++; } while (--vecSize > 0);
		else {
			float	factor	= me->factor,
					factor1	= me->factor1,
					offset	= me->offset;
			
			do { *outNoise++ = factor1 * floor(factor * (*curSamp++)) + offset; } 
			while (--vecSize > 0);
			}
		break;
	
	case actClip:
		if (me->nn == 0)
			do { *outNoise++ = ClipSignal(*curSamp++); } while (--vecSize > 0);
		else {
			float	factor	= me->factor,
					factor1	= me->factor1,
					offset	= me->offset;
			
			do { *outNoise++ = factor1 * floor(factor * ClipSignal(*curSamp++)) + offset; }
			while (--vecSize > 0);
			}
		break;
	
	case actWrap:
		if (me->nn == 0)
			do { *outNoise++ = WrapSignal(*curSamp++); } while (--vecSize > 0);
		else {
			float	factor	= me->factor,
					factor1	= me->factor1,
					offset	= me->offset;
			
			do { *outNoise++ = factor1 * floor(factor * WrapSignal(*curSamp++)) + offset; }
			while (--vecSize > 0);
			}
		break;
	
	case actReflect:
		if (me->nn == 0)
			do { *outNoise++ = ReflectSignal(*curSamp++); } while (--vecSize > 0);
		else {
			float	factor	= me->factor,
					factor1	= me->factor1,
					offset	= me->offset;
			
			do { *outNoise++ = factor1 * floor(factor * ReflectSignal(*curSamp++)) + offset; }
			while (--vecSize > 0);
			}
		break;
		}
	
exit:
	return iParams + paramNextLink;
	}
