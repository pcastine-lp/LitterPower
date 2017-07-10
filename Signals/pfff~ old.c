/*
	File:		pfff~.c

	Contains:	"Brown" noise. Uses Taus88 algorithm for random values, also has
				unique NN factor.

	Written by:	Peter Castine

	Copyright:	© 2001-2002 Peter Castine

	Change History (most recent first):

         <7>   24–3–2006    pc      Update #includes for new LitterLib organization.
         <6>     14–1–04    pc      Update for Windows.
         <5>    6–7–2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <4>  30–12–2002    pc      Drop faux 'Vers' resource and related modifications. Add object
                                    version to PvvvInfo().
         <3>  29–12–2002    pc      Update parameters for #pragma __ide_target()
         <2>  28–11–2002    pc      Tidy up after initial check in.
         <1>  28–11–2002    pc      Initial check in.
		 8-Feb-2000:	Added support for variable Hurst factor (lp.pvvv~)
		14-Apr-2000:	First implementation.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Identify Target

#ifdef __MWERKS__
	// With Metrowerks CodeWarrior make use of the __ide_target() preprocessor function
	// to identify which variant of the scamp family we're generating.
	// On other platforms (like GCC) we have to use a command-line flag or something
	// when building the individual targets.
	#if		__ide_target("Brown (Classic)")			\
			|| __ide_target("Brown (Carbon)")		\
			|| __ide_target("Brown (Windows)")
		#define __BROWN__ 1

	#elif	__ide_target("Black (Classic)")			\
			|| __ide_target("Black (Carbon)")		\
			|| __ide_target("Black (Windows)")
		#define __BLACK__ 1

	#elif	__ide_target("Multicolor (Classic)")	\
			|| __ide_target("Multicolor (Carbon)")	\
			|| __ide_target("Multicolor (Windows)")
		#define __VARICOLOR__ 1

	#else
		#error "Undefined target"
	#endif
#endif

#pragma mark • Include Files

#include <stdlib.h>		// For rand(), RAND_MAX

#include "LitterLib.h"	// Also #includes MaxUtils.h, ext.h
#include "TrialPeriodUtils.h"
#include "Taus88.h"
#include "MiscUtils.h"

#include "z_dsp.h"


#pragma mark • Constants

#if		defined(__BROWN__)
	const char*	kClassName		= "lp.pfff~";			// Class name
#elif	defined(__BLACK__)
	const char*	kClassName		= "lp.phhh~";			// Class name
#elif	defined(__VARICOLOR__)
	const char*	kClassName		= "lp.pvvv~";			// Class name
#endif


#ifdef __MWERKS__
	// CodeWarrior  allows you to define arrays using const ints
const int	kMaxNN			= 31,
			kMaxBuf			= 4096,					// Must be:
													//		- power of two
													//		- multiple of largest I/O vector
													//			size.

			kBufferSize		= kMaxBuf + 1;
#else
	// Talk about lame. GCC won't compile unless I use macros
	// Haven't tested this with other compilers, will need to see if it is necessary
	// for further checks
	#define kMaxNN		31
	#define kMaxBuf		4096
	#define kBufferSize 4097
#endif


	// Indices for STR# resource
#ifdef __VARICOLOR__
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

	// Indices for MSP Inlets and Outlets.
enum {
	inletBrown			= 0,		// For begin~
	
	outletBrown
	};

#pragma mark • Type Definitions



#pragma mark • Object Structure

typedef struct {
	t_pxobject		coreObject;
	
	#ifdef __VARICOLOR__
		double	hurstExp,
				hurstFac;
	#endif
	
	int		nn,							// Number of bits to mask out
			bufPos;
	long	buffer[kBufferSize],
			mask,						// Values depends on nn
			offset;
	} tBrown;


#pragma mark • Global Variables



#pragma mark • Function Prototypes

	// Class message functions
#ifdef __VARICOLOR__
	void*	PvvvNew(double, long);
#else		// Brown and Black have the same structure
	void*	PvvvNew(long);
#endif

	// Object message functions
#ifdef __VARICOLOR__
	static void PvvvHurst(tBrown*, double);
#endif
static void PvvvNN(tBrown*, long);
static void PvvvTattle(tBrown*);
static void	PvvvAssist(tBrown*, void* , long , long , char*);
static void	PvvvInfo(tBrown*);

	// MSP Messages
static void	PvvvDSP(tBrown*, t_signal**, short*);
static int*	PvvvPerform(int*);


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
	setup(	&gObjectClass,				// Pointer to our class definition
			(method) PvvvNew,			// Instance creation function
			(method) dsp_free,			// Default deallocation function
			sizeof(tBrown),				// Class object size
			NIL,						// No menu function
										// Optional arguments:
#ifdef __VARICOLOR__
			A_DEFFLOAT,					//		Vari-color only	-- Hurst factor
#endif		
			A_DEFLONG,					//		All				-- NN Factor
			0);		
	
	dsp_initclass();

	// Messages
#ifdef __VARICOLOR__
	addfloat((method) PvvvHurst);
	addinx	((method) PvvvNN, 1);
#else
	addint	((method) PvvvNN);
#endif
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
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	PvvvNew(iNN)
 *	
 ******************************************************************************************/

void*
PvvvNew(
#ifdef __VARICOLOR__
	double	iHurst,
#endif
	long	iNN)
	
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
#ifdef __VARICOLOR__
	// Multi-color needs, of course, an extra inlet for NN
	intin(me, 1);
#endif

	outlet_new(me, "signal");
	
	// Set up object components
	me->bufPos			= kMaxBuf;
	me->buffer[kMaxBuf] = (long) Taus88(NIL) / 4;
	PvvvNN(me, iNN);
#ifdef __VARICOLOR__
	PvvvHurst(me, iHurst);
#endif

	return me;
	}

#pragma mark -
#pragma mark • Object Message Handlers

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
		if (iNN > kMaxNN)
			iNN = kMaxNN;
		me->nn		= iNN;
		me->mask	= -1 << iNN;
		me->offset	= (~me->mask) >> 1;
		}
	
	}

#ifdef __VARICOLOR__
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
 *	PvvvTattle(me)
 *
 *	Post state information
 *
 ******************************************************************************************/

void
PvvvTattle(
	tBrown* me)
	
	{
	int bufPos = me->bufPos;
	
	post("%s state", kClassName);
#ifdef __VARICOLOR__
	post("	Hurst exponent is: %f (Hurst factor: %f)",
			me->hurstExp, me->hurstFac);
#endif
	post("  NN factor is: %d (mask = 0x%lx, offset = 0x%lx)",
			me->nn, me->mask, me->offset);
	post("  Buffer starts at address 0x%lx",
			me->buffer);
	post("  Current buffer postion: %d, containing %ld",
			bufPos, me->buffer[bufPos]);
	
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
#pragma mark • DSP Methods

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

	static void GenerateNewBuffer(tBrown* me)
		{
#ifdef __BROWN__
		const double hurstFac	= sqrt(0.5);			// Equiv. to pow(0.5, hurstExp) with
														// hurstExp = 0.5 (ie, brown noise).
#endif __BROWN__
#ifdef __BLACK__
		const double hurstFac	= 0.5;					// Equiv. to pow(0.5, hurstExp) with
														// hurstExp = 1.0 (ie, black noise).
#endif __BLACK__
#ifdef __VARICOLOR__
		const double hurstFac	= me->hurstFac;
#endif
		
		double	scale		= 0.5;
		long*	buf			= me->buffer;
		long	stride		= kMaxBuf / 2,
				offset		= kMaxBuf / 4;
		
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
		
		// Recursive interpolation
		// Initial state: stride = cycle/2, offset = stride/2
		while (offset > 0) {
			int i;
			
			// Interpolate initial values at midpoints between values
			// calculated so far
			for (i = offset; i <= kMaxBuf - offset; i += stride) {
			    buf[i] = buf[i-offset] / 2 + buf[i+offset] / 2;		// Let compiler optimize
			    													// to right-shifts. But don't
			    													// simplify to (a + b) / 2
			    													// because that has a real
			    													// potential for arithmetic
			    													// overflow.
				if (buf[i] == 0x80000000) {
					// do math slowly
					long	x1 = buf[i-offset],
							x2 = buf[i+offset],
							x3 = x1 / 2,
							x4 = x2 / 2,
							x5 = x3 + x4;					
					}		
				}
			// Add noise with reduced variance
			scale *= hurstFac;
			for (i = offset; i <= kMaxBuf; i += offset) {				
				// Debug slowly buf[i] += (long) Taus88(NIL) * scale;
				long t = Taus88(NIL);
				double tt = t * scale;
				long ttt = tt;
				buf[i] += ttt;
				if (buf[i] == 0x8000000 || buf[i] == 0x7fffffff)
					ttt = tt;					
				} 
			
			// Next generation: halve stride and offset
			stride = offset;										// ASSERT: offset == stride/2
			offset /= 2;											// Again, let compiler optimize.
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
