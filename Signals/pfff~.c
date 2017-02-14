/*
	File:		pfff~.c

	Contains:	"Brown" noise. Uses Taus88 algorithm for random values, also has
				unique NN factor.

	Written by:	Peter Castine

	Copyright:	© 2001-2002 Peter Castine

	Change History (most recent first):

         <7>   24Ð3Ð2006    pc      Update #includes for new LitterLib organization.
         <6>     14Ð1Ð04    pc      Update for Windows.
         <5>    6Ð7Ð2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <4>  30Ð12Ð2002    pc      Drop faux 'Vers' resource and related modifications. Add object
                                    version to PvvvInfo().
         <3>  29Ð12Ð2002    pc      Update parameters for #pragma __ide_target()
         <2>  28Ð11Ð2002    pc      Tidy up after initial check in.
         <1>  28Ð11Ð2002    pc      Initial check in.
		 8-Feb-2000:	Added support for variable Hurst factor (lp.pvvv~)
		14-Apr-2000:	First implementation.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark ¥ Identify Target

#ifdef __MWERKS__
	// With Metrowerks CodeWarrior make use of the __ide_target() preprocessor function
	// to identify which variant of the scamp family we're generating.
	// On other platforms (like GCC) we have to use a command-line flag or something
	// when building the individual targets.
	#if		__ide_target("Brown (Classic)")			\
			|| __ide_target("Brown (Carbon)")		\
			|| __ide_target("Brown (Windows)")
		#define BROWN 1

	#elif	__ide_target("Black (Classic)")			\
			|| __ide_target("Black (Carbon)")		\
			|| __ide_target("Black (Windows)")
		#define BLACK 1

	#elif	__ide_target("Multicolor (Classic)")	\
			|| __ide_target("Multicolor (Carbon)")	\
			|| __ide_target("Multicolor (Windows)")
		#define VARICOLOR 1

	#else
		#error "Undefined target"
	#endif
#endif

// Kludge
#ifndef TARGET_CPU_X86
	#if defined(WIN_VERSION) && WIN_VERSION
		#define TARGET_CPU_X86 1
		#define TARGET_CPU_PPC 0
	#else
		#define TARGET_CPU_X86 0
		#define TARGET_CPU_PPC 1
	#endif
#endif

#if (!TARGET_CPU_PPC && !TARGET_CPU_X86)
	#error This file can currently only compiler for PPC or Intel (X86) processors
#endif
	

#pragma mark ¥ Include Files

#include <stdlib.h>		// For rand(), RAND_MAX

#include "LitterLib.h"	// Also #includes MaxUtils.h, ext.h
#include "TrialPeriodUtils.h"
#include "Taus88.h"
#include "MiscUtils.h"

#include "z_dsp.h"


#pragma mark ¥ Constants

#if		defined(BROWN)
	const char*	kClassName		= "lp.pfff~";			// Class name
#elif	defined(BLACK)
	const char*	kClassName		= "lp.phhh~";			// Class name
#elif	defined(VARICOLOR)
	const char*	kClassName		= "lp.pvvv~";			// Class name
#endif


const int	kMaxNN			= 31;
	// gcc barfs later on if kMaxBuf or kBufferSize are defined const int, which
	// is how they always used to be defined. But it accepts enum, and other compilers
	// also accept enum, so anonymous enum it is.
enum {
	kMaxBuf			= 4096,
	kBufferSize					// == kMaxBuf + 1
	};
	
enum architecture {
	archNative		= 0,
	archPPC,
	archIntel,
	archPC415,					// A fictional processor. See GNB_PC415Loop for details.
	
	archCount					// Don't actually need this now, but it's convenient for
	};							// maintenance.


	// Indices for STR# resource
#ifdef VARICOLOR
	enum {
		strIndexInHurst		= lpStrIndexLastStandard + 1,
		strIndexInNN,
		strIndexOutBrown,
		
		strIndexInLeft		= strIndexInHurst,
		strIndexOutLeft		= strIndexOutBrown
		};
#else		// Brown and Black have the same structure
	enum {
		strIndexInNN		= lpStrIndexLastStandard + 1,
		strIndexOutBrown,
		
		strIndexInLeft		= strIndexInNN,
		strIndexOutLeft		= strIndexOutBrown
		};
#endif

#pragma mark ¥ Type Definitions

typedef enum architecture eArch;

#pragma mark ¥ Object Structure

typedef struct {
	t_pxobject		coreObject;
	
	#ifdef VARICOLOR
		double	hurstExp,
				hurstFac;
	#endif
	
	int		nn,							// Number of bits to mask out
			bufPos;
	long	buffer[kBufferSize],
			mask,						// Values depends on nn
			offset;
	eArch	arch;
	} tBrown;


#pragma mark ¥ Global Variables



#pragma mark ¥ Function Prototypes

	// Class message functions
#ifdef VARICOLOR
	void*	PvvvNew(double, long, long);
#else		// Brown and Black have the same structure
	void*	PvvvNew(long, long);
#endif

	// Object message functions
#ifdef VARICOLOR
	static void PvvvHurst(tBrown*, double);
#endif
static void PvvvNN(tBrown*, long);
static void PvvvMode(tBrown*, long);
static void PvvvTattle(tBrown*);
static void	PvvvAssist(tBrown*, void* , long , long , char*);
static void	PvvvInfo(tBrown*);

	// MSP Messages
static void	PvvvDSP(tBrown*, t_signal**, short*);
static int*	PvvvPerform(int*);


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
			(method) PvvvNew,			// Instance creation function
			(method) dsp_free,			// Default deallocation function
			sizeof(tBrown),				// Class object size
			NIL,						// No menu function
										// Optional arguments:
#ifdef VARICOLOR
			A_DEFFLOAT,					//		Vari-color only	-- Hurst factor
#endif		
			A_DEFLONG,					//		All				-- NN Factor
			A_DEFLONG,					//		All				-- Mode/Architecture
			0);		
	
	dsp_initclass();

	// Messages
#ifdef VARICOLOR
	addfloat((method) PvvvHurst);
	addinx	((method) PvvvNN, 1);
#else
	addint	((method) PvvvNN);
#endif
	addmess	((method) PvvvMode,		"mode",		A_DEFLONG, 0);
	addmess	((method) PvvvTattle,	"dblclick",	A_CANT, 0);
	addmess	((method) PvvvTattle,	"tattle",	A_NOTHING);
	addmess	((method) PvvvAssist,	"assist",	A_CANT, 0);
	addmess	((method) PvvvInfo,		"info",		A_CANT, 0);
	
	// MSP-Level messages
	LITTER_TIMEBOMB addmess	((method) PvvvDSP,		"dsp",		A_CANT, 0);

	//Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();
	
	}



#pragma mark -
#pragma mark ¥ Class Message Handlers

/******************************************************************************************
 *
 *	PvvvNew(iNN)
 *	
 ******************************************************************************************/

	// Silly little zero-buffer routine. Does not rely on any other members, so can be
	// called before initializing nn, arch, or anything else.
	static void InitBuf(tBrown* me)
		{
		int i;
		
		for (i = 0; i <= kMaxBuf; i += 1)
			me->buffer[i] = 0;
		me->bufPos = kMaxBuf;
		
		}

void*
PvvvNew(
#ifdef VARICOLOR
	double	iHurst,
#endif
	long	iNN,
	long	iMode)
	
	{
	tBrown*		me	= NIL;
	
	// Default NN value doesn't need massaging
	// Ditto for Hurst factor (if we're compiling the multi-color object)

	// Let Max/MSP allocate us, our inlets, and outlets.
	me = (tBrown*) newobject(gObjectClass);
	dsp_setup(&(me->coreObject), 1);				// Signal inlet for benefit of begin~
													// Otherwise left inlet does "NN" only
													// or, with the multi-color object, the
													// Hurst factor
#ifdef VARICOLOR
	// Multi-color needs, of course, an extra inlet for NN
	intin(me, 1);
#endif

	outlet_new(me, "signal");
	
	// Set up object components
	InitBuf(me);									// InitBuf() must not rely on mode or nn
	PvvvMode(me, iMode);
	PvvvNN(me, iNN);
#ifdef VARICOLOR
	PvvvHurst(me, iHurst);
#endif

	return me;
	}

#pragma mark -
#pragma mark ¥ Object Message Handlers

/******************************************************************************************
 *
 *	PvvvNN(me, iNN)
 *	PvvvHurst(me, iHurst)
 *	
 *	Set parameters, making sure nothing bad happens.
 *
 ******************************************************************************************/

void PvvvNN(
	tBrown* me,
	long	iNN)
	
	{
	
	if (iNN <= 0) {
		me->nn		= 0;
		me->mask	= -1;
		me->offset	= 0;
		}
	else {
		long mask;
		
		if (iNN > kMaxNN)
			iNN = kMaxNN;
			
		mask = -1 << iNN;
		
		me->nn		= iNN;
		me->mask	= (mask);
		me->offset	= ((~mask) >> 1);
		}
	
	}

#ifdef VARICOLOR
	void PvvvHurst(
		tBrown* me,
		double	iHurst)
		
		{
		
		me->hurstExp	= iHurst;
		me->hurstFac	= pow(0.5, iHurst);
		
		}
#endif


/******************************************************************************************
 *
 *	PvvvMode(me, 0)
 *
 ******************************************************************************************/
 
 void
 PvvvMode(
	tBrown*	me,
	long	iMode)
	
	{
	if (iMode < archNative || archCount <= iMode) {
		error("%s: unknown mode %ld, setting to Native", kClassName, iMode);
		iMode = 0;
		}
	
	me->arch = (eArch) iMode;
	
	}


/******************************************************************************************
 *
 *	PvvvTattle(me)
 *
 *	Post state information
 *
 ******************************************************************************************/

void
PvvvTattle(
	tBrown* me)
	
	{
	int		bufPos = me->bufPos;
	char*	archStr;
	
	switch (me->arch) {
		default: archStr = "Native "
	#if		TARGET_CPU_PPC
							"(PPC)";
	#elif	TARGET_CPU_X86
							"(Intel/X86)";
	#endif
			break;
			
		case archPPC:	archStr = "PPC";							break;
		case archIntel:	archStr = "Intel";							break;
		case archPC415:	archStr = "The mythical PC415 processor";	break;
		}
	
	
	post("%s state", kClassName);
#ifdef VARICOLOR
	post("	Hurst exponent is: %f (Hurst factor: %f)",
			me->hurstExp, me->hurstFac);
#endif
	post("  NN factor is: %d (mask = 0x%lx, offset = 0x%lx)",
			me->nn, me->mask, me->offset);
	post("  Buffer starts at address 0x%lx",
			me->buffer);
	post("  Current buffer postion: %d, containing %ld",
			bufPos, me->buffer[bufPos]);
	post("  Double-to-int conversion mode: %ld -- %s", me->arch, archStr);
	
	}


/******************************************************************************************
 *
 *	PvvvAssist()
 *	PvvvInfo()
 *
 *	Fairly generic Assist/Info methods
 *
 ******************************************************************************************/

void PvvvAssist(tBrown* me, void* box, long iDir, long iArgNum, char* oCStr)
	{
	#pragma unused(me, box)
	
	LitterAssist(iDir, iArgNum, strIndexInLeft, strIndexOutLeft, oCStr);
	}

void PvvvInfo(tBrown* me)
	{ LitterInfo(kClassName, &me->coreObject.z_ob, (method) PvvvTattle); }



#pragma mark -
#pragma mark ¥ DSP Methods

/******************************************************************************************
 *
 *	PvvvDSP(me, ioDSPVectors, iConnectCounts)
 *
 ******************************************************************************************/

void
PvvvDSP(
	tBrown*		me,
	t_signal**	ioDSPVectors,
	short*		connectCounts)
	
	{
	#pragma unused(connectCounts)
	
	// Indices for MSP Inlets and Outlets.
	enum {
		inletBrown			= 0,		// For begin~
		outletBrown
		};

	dsp_add(
		PvvvPerform, 3,
		me, (long) ioDSPVectors[outletBrown]->s_n, ioDSPVectors[outletBrown]->s_vec
		);
	
	}
	

/******************************************************************************************
 *
 *	PvvvPerform(iParams)
 *
 *	Parameter block for PerformSync contains 8 values:
 *		- Address of this function
 *		- The performing schhh~ object
 *		- Vector size
 *		- output signal
 *		- Imaginary output signal
 *
 ******************************************************************************************/

	static void GNB_NativeLoop(long* ioBuf, int iStride, double iHurst)
		{
		int		i,
				stride	= iStride,
				offset	= iStride / 2;
		double	scale	= 0.5;			// Initial scale value
		
		// Recursive interpolation
		// Initial state: stride = cycle/2, offset = stride/2
		while (offset > 0) {
			// Interpolate initial values at midpoints between values
			// calculated so far
			for (i = offset; i <= kMaxBuf - offset; i += stride)
			    ioBuf[i] = ioBuf[i-offset] / 2 + ioBuf[i+offset] / 2;
					// Let compiler optimize division to right-shifts. But don't simplify to
					// (a + b) / 2 because that has a real potential for arithmetic
			    	// overflow.
					
			// Add noise with reduced variance
			scale *= iHurst;
			for (i = offset; i <= kMaxBuf; i += offset) {	
				ioBuf[i] += (long) Taus88(NIL) * scale;
				} 
			
			// Next generation: halve stride and offset
			stride = offset;										// ASSERT: offset == stride/2
			offset /= 2;											// Again, let compiler optimize.
			}
		}

#if !TARGET_CPU_PPC
	// Need to be able to emulate PPC handling of double-to-int overflow
	// See GNB_NativeLoop for more extensive comments
	static void GNB_PPCLoop(long* ioBuf, int iStride, double iHurst)
		{
		const double	kLongMaxAsDouble	= kLongMax,
						kLongMinAsDouble	= kLongMin;
		
		int		i,
				stride	= iStride,
				offset	= iStride / 2;
		double	scale	= 0.5;
		
		while (offset > 0) {
			for (i = offset; i <= kMaxBuf - offset; i += stride)
			    ioBuf[i] = ioBuf[i-offset] / 2 + ioBuf[i+offset] / 2;
					
			scale *= iHurst;
			for (i = offset; i <= kMaxBuf; i += offset) {	
				// Here's where we emulate PPC
				double t = (long) Taus88(NIL) * scale + ioBuf[i];
				
				ioBuf[i] = (t > kLongMaxAsDouble)
							? kLongMax
							: (t < kLongMinAsDouble) ? kLongMin : (long) t;
				} 
			
			stride = offset;
			offset /= 2;
			}
		}
#endif

#if !TARGET_CPU_X86
	// Need to be able to emulate Intel handling of double-to-int overflow
	// See GNB_NativeLoop for more extensive comments
	static void GNB_IntelLoop(long* ioBuf, int iStride, double iHurst)
		{
		const double	kLongMaxAsDouble	= kLongMax,
						kLongMinAsDouble	= kLongMin;
						
		int		i,
				stride	= iStride,
				offset	= iStride / 2;
		double	scale	= 0.5;
		
		while (offset > 0) {
			for (i = offset; i <= kMaxBuf - offset; i += stride)
			    ioBuf[i] = ioBuf[i-offset] / 2 + ioBuf[i+offset] / 2;
			scale *= iHurst;
			for (i = offset; i <= kMaxBuf; i += offset) {	
				// Here's where we emulate Intel
				double t = (long) Taus88(NIL) * scale + ioBuf[i];
				
				ioBuf[i] = (t > kLongMaxAsDouble || t < kLongMinAsDouble)
							? kLongMin
							: (long) t;
				} 
			
			stride = offset;
			offset /= 2;
			}
		}
#endif

	// This emulates the behavior of the PC415 processor, a mythical processor that
	// exists only in the imagination of Peter Castine. It handles overflow in double-to-int
	// typecasts by taking the integer part of the double value modulo 2^32.
	static void GNB_PC415Loop(long* ioBuf, int iStride, double iHurst)
		{
		const double	kLongMaxAsDouble	= kLongMax,
						kLongMinAsDouble	= kLongMin;
						
		int		i,
				stride	= iStride,
				offset	= iStride / 2;
		double	scale	= 0.5;
		
		while (offset > 0) {
			for (i = offset; i <= kMaxBuf - offset; i += stride)
			    ioBuf[i] = ioBuf[i-offset] / 2 + ioBuf[i+offset] / 2;
			scale *= iHurst;
			for (i = offset; i <= kMaxBuf; i += offset) {	
				// Here's where we emulate the PC415
				// We take advantage of the fact that -2^32 <= t < 2^32, allowing us to
				// substitute a simple addition or subtraction for a more expensive modulo
				// operation
				double t = (long) Taus88(NIL) * scale + ioBuf[i];
				
				ioBuf[i] = (t > kLongMaxAsDouble)
							? (long) (t + kLongMinAsDouble)
							: (t < kLongMinAsDouble)
								? (long) (t - kLongMinAsDouble)
								: (long) t;
				} 
			
			stride = offset;
			offset /= 2;
			}
		}

	static void GenerateNewBuffer(tBrown* me)
		{
#if defined(BROWN)
		const double hurstFac	= sqrt(0.5);			// Equiv. to pow(0.5, hurstExp) with														// hurstExp = 0.5 (ie, brown noise).
#elif defined(BLACK)
		const double hurstFac	= 0.5;					// Equiv. to pow(0.5, hurstExp) with
														// hurstExp = 1.0 (ie, black noise).
#elif defined(VARICOLOR)
		const double hurstFac	= me->hurstFac;
#endif
		
		long*	buf			= me->buffer;
		int		stride		= kMaxBuf / 2;
		
		//
		// Voss random addition algorithm
		//
		
		// pfff takes buf[0] as it stands, initialize midpoint and endpoint
		// (Inheriting buf[0] from the previous run and leaving it unmodified is the main
		// modification to the Voss algorithm made in this implementation.)
		//
		buf[0]			= buf[kMaxBuf];
		buf[stride]		= (long) Taus88(NIL) / 4;			// ASSERT: stride = kMaxBuffer/2
		buf[kMaxBuf]	= (long) Taus88(NIL) / 4;
							// Taking random values in the signed int equivalent of
							// [-.25 .. .25) was a completely arbitrary decision made
							// in the first implementation. It works well enough for
							// Hurst values under 0.5, but should be increased for
							// higher values. However, I am loathe to change things now.
							// At some stage an 'autoscale' option similar to pvvv2 might
							// be hip.
		
		switch (me->arch) {
			default:		GNB_NativeLoop(buf, stride, hurstFac);	break;
			// The conditional compilation conveniently  causes the object to use the
			// 'native' loop when the host architecture matches the specified float-to-int
			// conversion type
#if !TARGET_CPU_PPC
			case archPPC:	GNB_PPCLoop(buf, stride, hurstFac);		break;
#endif
#if !TARGET_CPU_X86
			case archIntel:	GNB_IntelLoop(buf, stride, hurstFac);	break;
#endif
			case archPC415: GNB_PC415Loop(buf, stride, hurstFac);	break;
			}
				
		me->bufPos = 0;
		}
	
int*
PvvvPerform(
	int* iParams)
	
	{
	enum {
		paramFuncAddress	= 0,
		paramMe,
		paramVectorSize,
		paramOut,
		
		paramNextLink
		};
	
	long*			curSamp;
	long			vecSize;
	tSampleVector	outNoise;			// Do integer arithmetic in buffer, then convert
										// to floating point before exit.
	tBrown*			me = (tBrown*) iParams[paramMe];
	
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
	
	// Do we have to deal with NN factor?
	if (me->nn == 0)
		do { *outNoise++ = Long2Signal(*curSamp++); } while (--vecSize > 0);
	else {
		long	mask	= me->mask,
				offset	= me->offset;

		do {
			*outNoise++ = Long2Signal((*curSamp++ & mask) + offset);
			} while (--vecSize > 0);
		}
	
exit:
	return iParams + paramNextLink;
	}
