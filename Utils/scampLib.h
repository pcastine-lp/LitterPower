/*
	File:		scamp Common.h

	Contains:	Header file used by scampi/scampf/scamp~.
				Might have been more appropriate to call this scampLib.h

	Written by:	Peter Castine

	Copyright:	© 2003 Peter Castine

	Change History (most recent first):

         <5>    8Ð5Ð2006    pc      Fixed name conflict between the global enum Symmetry and the
                                    mapping enum Symmetry (now eMapSym)
         <4>     11Ð1Ð04    pc      Update for Windows.
         <3>    6Ð7Ð2003    pc      Decided to take sync function out of scamp~.
The object has more
                                    than enough features as it is.
         <2>   30Ð3Ð2003    pc      Sort out problems with scamp~
         <1>   15Ð3Ð2003    pc      Initial implementation and check in.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark ¥ Identify Target

#define	SCAMPI		1
#define SCAMPF		2
#define SCAMPSIG	3

#ifdef __MWERKS__
	// With Metrowerks CodeWarrior make use of the __ide_target() preprocessor function
	// to identify which variant of the scamp family we're generating.
	// On other platforms (like GCC) we have to use a command-line flag or something
	// when building the individual targets.
	#if		 __ide_target("Map/Limit Int (Classic)")			\
				|| __ide_target("Map/Limit Int (Carbon)")		\
				|| __ide_target("Map/Limit Int (Windows)")
		#define SCAMP_TARGET	SCAMPI

	#elif	__ide_target("Map/Limit FP (Classic)")				\
				|| __ide_target("Map/Limit FP (Carbon)")		\
				|| __ide_target("Map/Limit FP (Windows)")
		#define SCAMP_TARGET	SCAMPF

	#elif	__ide_target("Map/Limit Signals (Classic)")			\
				|| __ide_target("Map/Limit Signals (Carbon)")	\
				|| __ide_target("Map/Limit Signal (Windows)")
		#define SCAMP_TARGET	SCAMPSIG

	#else
		#error "Unknown target"
	#endif
#endif


#pragma mark ¥ Include Files

#include "LitterLib.h"


#pragma mark ¥ Constants

extern const char		kClassName[];

	// Default values for linear/power/exponential mappings
	// We need this both at instantiation time and when receiving mapping messages without
	// parameters
	// Note that the default value for power can't be calculated at compile time, so
	// we'll have the preprocessor do macro substitution
#define		kDefKink		0.0
#define		kDefExp			1.0
#define		kDefPower		log(2.0)


	// Mapping types
enum MapType {
		mapLin,					// Linear scaling of input range to output range
		mapPow,					// Power scaling
		mapExp					// Exponential scaling
		};

enum MapSymmetry {
		symNone,
		symPoint,
		symAxis
		};

#if (SCAMP_TARGET == SCAMPI)
		// Rounding values
	enum RoundMeth {
			roundToZero,		// i.e., Truncation, what C does automatically
			roundRound,			// Standard arithmetic rounding, i.e. trunc(x + 0.5)
			roundFloor,
			roundCeil,
			roundToInf			// Opposite of "to zero" truncation
			};
#endif


#if (SCAMP_TARGET != SCAMPSIG)
	#define kSplitValues	((SymbolPtr) -1L)
#endif


#pragma mark ¥ Type Definitions

typedef enum MapType		eMapType;
typedef enum MapSymmetry	eMapSym;

#if (SCAMP_TARGET == SCAMPI)
	typedef enum RoundMeth	eRoundMeth;
#endif

typedef struct ScampFlags {
	Boolean		map			: 1,	// If false, do traditional scampf scale/offset
									// Otherwise, map between input and output ranges
				rangeInval	: 1,	// If true, we need to recalc range information
				degenerate	: 1,	// Flag degenerate case of simple linear mapping
									// Happens whenever curve is 1.0 ($f6 == 0.0).
							: 0;
	} tScampFlags;

#pragma mark ¥ Object Structure

typedef struct {
#if (SCAMP_TARGET == SCAMPSIG)
	t_pxobject	coreObject;
	long		limCounter;
#else
	Object		coreObject;
	Symbol*		split;				// If NULL, then no splitting
									// If (Symbol*) -1, then send out-of-range
									// values out right outlet
									// Any other value is presumed to be a valid
									// pointer to a symbol, and this is sent out
									// the right outlet.
#endif
	
	tOutletPtr	out2;				// For scamp~ this provides a count of how many times
									// the mapping went out of range. For other objects
									// this is an outlet for out-of-range values.
	
	// Use double-precision floating point for all arithmetic.
	// Doubles have sufficient precision in the mantissa to store all incoming
	// 32-bit integers without loss of accuracy and can also handle the entire
	// range of 32-bit floating point values that Max might send through a patch
	// chord.
	double		input,				// User input
				inMin,				// User input in map mode, calculated in scale mode
				inMax,				// ditto
				slope,				// User Input in scale mode, ignored in map mode
				offset,				// ditto
				curve;				// Calculated from user input, exp($f6)
									//		Exponent for power mapping
									//		Base for exponential mapping
									//		'Kink' for linear mapping

#if (SCAMP_TARGET == SCAMPI)
	long		outMin,				// User Input
				outMax;				// User Input
	
	eRoundMeth	rounding;
#else
	double		outMin,				// User Input
				outMax;				// User Input
#endif

	eActionType	action;				// Clip/Wrap/Reflect/Stet
	eMapType	mappingType;		// Linear, Power, or Exponential
	eMapSym	symmetry;			// Point Symmetry, Axis Symmetry, or None
	tScampFlags	flags;		

	} objScamp;


#pragma mark ¥ Global Variables

extern SymbolPtr	gSplitSym,
					gStetSym,
					gClipSym,
					gReflectSym,
					gWrapSym,
					gLinSym,
					gPowSym,
					gExpSym,
					gMapSym,
					gSym0Sym,
					gSym1Sym,
					gSym2Sym;

#if (SCAMP_TARGET == SCAMPI)
extern SymbolPtr	gTruncSym,
					gRoundSym,
					gFloorSym,
					gCeilSym,
					gToInfSym;
#endif

#pragma mark ¥ Function Prototypes

	// Defined in scampCommon.c
void	ScampGenSymbols(void);
void	ScampAddMessages(void);
void	ScampCalcRange(objScamp* me);
void	ScampParseArgs(objScamp*, short, Atom[]);

	//
	// Methods provided by all scamp objects
	//

	// Class message functions
objScamp*	NewScamp(Symbol*, short, Atom*);

	// Object message functions
void	ScampBang(objScamp*);
void	ScampInt(objScamp*, long);
void	ScampFloat(objScamp*, double);
void	ScampFloat1(objScamp*, double);
void	ScampFloat2(objScamp*, double);
void	ScampMinOut(objScamp*, double);
void	ScampMaxOut(objScamp*, double);
void	ScampCurve(objScamp*, double);

#if (SCAMP_TARGET == SCAMPSIG)
	void ScampSync(objScamp*, long, double);
#else
	void ScampSet(objScamp*, Symbol*, short, Atom[]);
	void ScampSplit(objScamp*, Symbol*, short, Atom[]);
#endif


void	ScampMap(objScamp*, Symbol*, short, Atom[]);
void	ScampSym(objScamp*, long);

void	ScampLin(objScamp*, double);
void	ScampPow(objScamp*, Symbol*, short, Atom[]);
void	ScampExp(objScamp*, Symbol*, short, Atom[]);

void	ScampClip(objScamp*);
void	ScampWrap(objScamp*);
void	ScampReflect(objScamp*);
void	ScampStet(objScamp*);

#if 	(SCAMP_TARGET == SCAMPI)
	void ScampRound(objScamp*);
	void ScampFloor(objScamp*);
	void ScampCeil(objScamp*);
	void ScampToInf(objScamp*);
	void ScampToZero(objScamp*);
#elif	(SCAMP_TARGET == SCAMPSIG)
	void ScampDSP(objScamp*, t_signal**, short*);
#endif

void	ScampTattle(objScamp*);
void	ScampAssist(objScamp*, void* , long , long , char*);
void	ScampInfo(objScamp*);

#pragma mark -
#pragma mark ¥ Inline Functions

static inline tOutletPtr LeftOutlet(objScamp* me)
	{
	#if (SCAMP_TARGET == SCAMPSIG)
		return me->coreObject.z_ob.o_outlet;
	#else
		return me->coreObject.o_outlet;
	#endif
	}
	
static inline double MapToUnit(double x, double inMin, double inRange)
	{ return (x - inMin) / inRange; }
static inline double MapFromUnit(double x, double outMin, double outRange)
	{ return x * outRange + outMin; }
static inline double MapLinear(double x, double scale, double offset)
	{ return x * scale + offset; }
static inline double MapKink(double x, double kink)
	{ return (x <= kink / (kink + 1.0)) ? x / kink : kink * (x - 1.0) + 1.0; }
static inline double MapPower(double x, double exp)
	{ return pow(x, exp); }
static inline double MapExponential(double x, double base)
	{ return x * pow(base, x - 1.0); }
