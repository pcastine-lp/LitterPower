/*
	File:		imLib.h

	Contains:	Headers for code common to all interval mutations.

	Written by:	Peter Castine

	Copyright:	© 2000-2002 Peter Castine. All rights reserved.

	Change History (most recent first):

         <6>    8Ð5Ð2006    pc      Update to support Jitter matrices. More work may still need to
                                    be done.
         <5>   26Ð4Ð2006    pc      Update for new LitterLib organization.
         <4>     10Ð1Ð04    pc      Update for Windows using new LitterLib functions.
         <3>    7Ð7Ð2003    pc      Initial check-in.
         <2>  30Ð12Ð2002    pc      Use 'STR#' resource instead of faux 'Vers' resource for storing
                                    version information used at run-time.
         <1>  30Ð12Ð2002    pc      Initial Check-In
		
		26-Dec-2002:	Update parameters to __ide_target() to match new Carbon/Classic
						target names.
		15-Nov-2000:	First implementation (based on previous Mutator~.c)
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark ¥ Identify Target

#ifdef __MWERKS__
	// With Metrowerks CodeWarrior make use of the __ide_target() preprocessor function
	// to identify which variant of the scamp family we're generating.
	// On other platforms (like GCC) we have to use a command-line flag or something
	// when building the individual targets.
	#if		__ide_target("Frequency Domain (Classic)")			\
				|| __ide_target("Frequency Domain (Carbon)")	\
				|| __ide_target("Frequency Domain (Windows)")
		#define __IM_MSP__			1
		#define __IM_HISTORY__		1
		#define __IM_DOUBLEBARREL__	1
		#define __IM_JITTER__		0

	#elif	__ide_target("Time Domain (Classic)")				\
				|| __ide_target("Time Domain (Carbon)")			\
				|| __ide_target("Time Domain (Windows)")
		#define __IM_MSP__			1
		#define __IM_HISTORY__		0
		#define __IM_DOUBLEBARREL__	0
		#define __IM_JITTER__		0

	#elif	__ide_target("Value Interval (Classic)")			\
				|| __ide_target("Value Interval (Carbon)")		\
				|| __ide_target("Value Interval (Windows)")
		#define __IM_MSP__			0
		#define __IM_HISTORY__		0
		#define __IM_DOUBLEBARREL__	0
		#define __IM_JITTER__		0

	#elif	__ide_target("Matrix Mutation (Boron)")			\
				|| __ide_target("Matrix Mutation (Carbon)")		\
				|| __ide_target("Matrix Mutation (Windows)")
		#define __IM_MSP__			0
		#define __IM_HISTORY__		0
		#define __IM_DOUBLEBARREL__	0
		#define __IM_JITTER__		1

	#else
		#error "Unknown target"
	#endif
#endif

#pragma mark ¥ Include Files

#if __IM_JITTER__
	#define __NEED_JITTER_HEADERS__ 1
#endif 

#include "LitterLib.h"
#include "Taus88.h"

#include <math.h>				// For pow()



#pragma mark ¥ Global Constants

#if __IM_HISTORY__
	extern const long	kMaxHistory;
#endif

	// Indices for STR# resource
enum {
	
	#if __IM_DOUBLEBARREL__
		strIndexInSource1		= lpStrIndexLastStandard + 1,
		strIndexInSource2,
		strIndexInTarget1,
		strIndexInTarget2,
	#else
		strIndexInSource		= lpStrIndexLastStandard + 1,
		strIndexInTarget,
	#endif
	
	strIndexInOmega,
	strIndexInPi,
	strIndexInDelta,
	
	#if __IM_DOUBLEBARREL__
		strIndexOutMutant1,
		strIndexOutMutant2,
	#else
		strIndexOutMutant,
	#endif
	
	strIndexFirstIn				= lpStrIndexLastStandard + 1,
	strIndexFirstOut			= strIndexInDelta + 1
	};

	// Indices for Inlets and Outlets. Users count starting at 1, we count from 0
	// 
enum {
	#if __IM_DOUBLEBARREL__
		inletSource1	= 0,
		inletSource2,
		inletTarget1,
		inletTarget2,
	#else
		inletSource		= 0,
		inletTarget,
	#endif
		
	inletOmega,
	inletPi,
	inletDelta,
	
	#if __IM_DOUBLEBARREL__
		outletMutant1,
		outletMutant2,
	#else
		outletMutant,
	#endif
	
	outletRangeCheck,
	
	#if __IM_DOUBLEBARREL__
		outletFirst		= outletMutant1
	#else
		outletFirst		= outletMutant
	#endif
	
	};

	// Numeric codes for the Interval Mutation functions (maps to MENU items, strings, etc.)
enum {
	imUSIM			= 0,
	imISIM,
	imUUIM,
	imIUIM,
	imWCM,
	imLCM,
	
	imLast			= imLCM,
	imDefault		= imUSIM
	};

	// Mutation state for irregular mutations
enum mutationState {
	stateIndeterminate	= 0,
	stateSource,
	stateTarget,
	
	stateFollowLeadBit	= 4
	};

	// These are defined in imLib.c
extern const char*		kShortNames[imLast + 1];
extern const char*		kLongNames[imLast + 1];
extern const double		kMaxPi;

	// This is defined in target-specific source code files.
extern const char*		kClassName;


#pragma mark ¥ Type Definitions


	// tInterval and tAtomicInterval are legacy.
	// Originally I distinguished between interval domain mutations and floating point
	// domain mutations. Now (almost) everything is double-precision floating point. 
	// I want to see about getting rid of the "atomic" size data type (that is guaranteed
	// to fit into a 32-bit Max atom). Then we can revert to doing everything with doubles.
typedef double	tInterval;
typedef float	tAtomicInterval;

typedef double		(*tMutateFunc)(tInterval, tInterval, double);	// Weighted Mutation
typedef tInterval	(*tMutate1Func)(tInterval, tInterval);			// Special-case for ½=1
		// We don't special-case mutation with ½ == 0, it's just the identity function

typedef union {
	tMutateFunc		uniformFunc;		// Used with uniform mutations
	tMutate1Func	irregularFunc;		// Used with irregular mutations
	} tMutationFunction;

typedef tInterval	(*tEmphasisFunction)(tInterval, double, double);

typedef struct {
	double				omega,
						pi,
						delta;
					
	long				clumpLen;				// 0 for Markov-chain ("strict") clumping,
												// Negative for "hard" clumping (cut-off feedback),
												// Positive for "tight" clumping (linear feedback)
	
	tMutationFunction	function;
	
	Byte				irregular		: 1,	// Set if Irregular mutations, clear for Uniform
						magOnly			: 1,	// Set for Unsigned Interval Mutations (UUIM and IUIM)
						signOnly		: 1,	// Set for Contour Mutations (LCM and WCM)
						relInterval		: 1 
					
	#if __IM_MSP__
		#if __IM_DOUBLEBARREL__
						,
						bandsPerOctave	: 4
		
		#else
						,
						omegaSR			: 1,	// Set if signal input for Omega, Pi, or Delta
						piSR			: 1,	// is updated at sample rate; clear otherwise
						deltaSR			: 1		// (i.e., using "control rate").
		#endif
	#endif
						;
	} tMutationParams;

#if __IM_HISTORY__
	typedef struct {
		#if __IM_DOUBLEBARREL__
		float	source1,
				source2,
				target1,
				target2,
				mutant1,
				mutant2;
		#else
		float	source,
				target,
				mutant;
		#endif
		} tMutation;
	typedef tMutation* tMutationPtr;
	
	
	typedef struct {
		// Ring buffer to maintain state over "history" (generally to maintain
		// state for each individual bin of a Fourier transform)
		unsigned short	bufSize,
						bufIndex,
						nyquist;
		tMutationPtr	muBuffer,
						curMu;
		BytePtr			stateBuf,
						curState;
		} tHistory;
#else
	typedef struct {
		double	source,
				target,
				mutant;
		Byte	state;
		} tHistory;
#endif

#pragma mark ¥ Object Structure

typedef struct {
	#if __IM_MSP__
		t_pxobject		coreObject;
	#else
		t_object		coreObject;
		tInterval		pendTarget;
	#endif
	
	tHistory			history;
	tMutationParams		params;
	
	#if __IM_HISTORY__
		tMutationParams frameParams;
		Boolean			paramUpdatePending;
	#endif
	
	} tMutator;


#pragma mark ¥ Global Variables

	// These are all set once, in main()
//extern t_messlist*	gMutatorClass;
//extern short		gStrResID;
//extern long			gModDate;



#pragma mark ¥ Function Prototypes

short		MapStrToMutationType(const char* iStr);
int			CalcMutationIndex(const tMutator*);

double		CalcSIMutant(tInterval, tInterval, double);	// Mutation calculations that depend on
double		CalcUIMutant(tInterval, tInterval, double);	// the Mutation Index are always done with
double		CalcWCMutant(tInterval, tInterval, double);	// Floating Point arithemtic and return
tInterval	CalcSIMutant1(tInterval, tInterval);		// a floating point value (which may be
tInterval	CalcUIMutant1(tInterval, tInterval);		// further processed by an Emphasis Function).
tInterval	CalcLCMutant1(tInterval, tInterval);

double		CalcOmegaPrime(double, double);

unsigned long CalcInitThresh(double);
unsigned long CalcMutantToMutantThresh(double);
unsigned long CalcSourceToMutantThresh(double, double);
unsigned long CalcMutantThreshholds(double, double, unsigned long*, unsigned long*);

double		EmphasizePos(tInterval, double, double);
double		EmphasizeNeg(tInterval, double, double);
double		EmphasizeZero(tInterval, double, double);

	// Changed float to double 4-23-02
void		Initialize(tMutator*, int, double, double, double, Boolean);

void		DoBang(tMutator* me);
	// Changed float to double 4-23-02
void		DoFloat(tMutator*, double);
void		DoOmega(tMutator*, double);
void		DoPi(tMutator*, double);
void		DoDelta(tMutator*, double);
void		DoClear(tMutator*);

void		SetMutatorType(tMutator* me, short iType);
void		DoUSIM(tMutator*);
void		DoISIM(tMutator*);
void		DoUUIM(tMutator*);
void		DoIUIM(tMutator*);
void		DoWCM(tMutator*);
void		DoLCM(tMutator*);

void		DoAbsInt(tMutator*);
void		DoRelInt(tMutator*, float);

#if __IM_MSP__ && !__IM_DOUBLEBARREL__
	void	DoControlRate(tMutator*);
	void	DoSampleRate(tMutator*);
#endif

#if __IM_HISTORY__
		void UpdateFrameParams(tMutator* me);
#endif

	

#pragma mark -
#pragma mark ¥ Inline Functions

	// The math library has an absolute value function, but I want to be sure
	// that it's inlined. One way of the other, we have to build our own Sign()
	// function. Rolling our own also allows us to define the functions in terms
	// of the value domain the object is working with (integers or floats).
static inline tInterval Abs(tInterval iValue)
	{ return iValue >= 0.0 ? iValue : -iValue; }
static inline tInterval Sign(tInterval iValue)
	{ return iValue > 0.0 ? 1.0 : (iValue == 0.0 ? 0.0 : -1.0); }
static inline tInterval Mod(tInterval iVal, tInterval iMod)
	{ return fmod(iVal, iMod); }
	

static inline double
CalcOmegaPrimeCore(double iOmega, double iPi)
	{ return  pow(iOmega, (iPi > kMaxPi) ? 1.0 - kMaxPi : 1.0 - iPi); }

static inline unsigned long
CalcInitThreshCore(double iOmega)
	{ return ((double) kULongMax) * iOmega + 0.5; }

static inline unsigned long
CalcMutantToMutantThreshCore(double iOmegaPrime)
	{ return ((double) kULongMax) * iOmegaPrime + 0.5; }
	
static inline unsigned long
CalcSourceToMutantThreshCore(double iOmega, double iOmegaPrime)
	{
	return ((double) kULongMax)	* (1.0 - iOmegaPrime)
								* iOmega / (1.0 - iOmega) + 0.5;
	}
	
static inline Boolean
WannaMutateStrict(unsigned long iThresh)
	{ return Taus88(NIL) < iThresh; }	
		
static inline Boolean
WannaMutateTight(unsigned long iThresh, int iWishMutants, int iSamplesLeft)
	{
	return (iWishMutants >= iSamplesLeft)
			? true
			: (iWishMutants <= 0) ? false : Taus88(NIL) < iThresh;
	}
	
static inline double
EffectiveDeltaValue(double iDeltaEmphasis)
	// iDeltaEmphasis should be in [-1.0 .. 1.0]
	{ return (iDeltaEmphasis >= 0.0) ? 1.0 - iDeltaEmphasis : 1.0 + iDeltaEmphasis; }

static inline double
ValidOmega(double iOmega)
	{
	if (iOmega < 0.0)	return 0.0;
	if (iOmega > 1.0)	return 1.0;
	return iOmega;
	}

static inline double
ValidPi(double iPi)
	{
	if (iPi < 0.0)		return 0.0;
	if (iPi > kMaxPi)	return kMaxPi;
	return iPi;
	}

static inline double
ValidDelta(double iDelta)
	{
	if (iDelta < -1.0)	return -1.0;
	if (iDelta > 1.0)	return 1.0;
	return iDelta;
	}