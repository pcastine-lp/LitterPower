/*
	File:		LitterLib.h

	Contains:	Header file for LitterLib.

	Written by:	Peter Castine

	Copyright:	© 2001-2006 Peter Castine

	Change History (most recent first):

         <4>     27Ð2Ð06    pc      Move symmetry constants here, too.
         <3>     27Ð2Ð06    pc      Move constants (kPi, etc.) to main LitterLib files. The
                                    constants are used in lots of projects that don't need MoreMath.
         <2>     26Ð2Ð06    pc      Add expMin, expMax to enumerator Expect.
         <1>     26Ð2Ð06    pc      First checked in after moving file and splitting off RNGs,
                                    Utilities, etc. into separate files
*/

/******************************************************************************************
	Prior History:
 
        <14>     21Ð2Ð06    pc      Add simple inline beta() function
        <13>     18Ð2Ð06    pc      Add general digamma() function (supporting all reals).
        <12>     15Ð2Ð06    pc      Need to implement lgamma() and gamma() functions because the
                                    braindead MSL/Windows doesn't provide them.  Also added
                                    Factorial(), LogFactorial() and two special cases of the digamma
                                    function.
        <11>     15Ð2Ð06    pc      Add LitterExpect(), support for an 'expect' message for all
                                    Litter RNGs.
        <10>     12Ð2Ð06    pc      Move CountBits() to MaxUtils. Cleaned up some code.
         <9>     11Ð1Ð06    pc      Add LitterAssistResF() utility function.
         <8>     13Ð1Ð04    pc      Use Max API getbytes() & freebytes() for the Taus88 and TT800
                                    new/free routines. It seems to be the most reliable way to make
                                    the relevant code cross-platform compatible, and the amounts of
                                    memory required are within getbytes() capabilities.
         <7>     10Ð1Ð04    pc      Major updates to support cross-platform development. Added
                                    LitterGetVersVal() & Co. Freezing features for Litter 1.5 (bar
                                    beta bug fixes).
         <6>      4Ð1Ð04    pc      Add LitterVers() and LetterAssist(): default methods for
                                    handling vers and assist messages.
         <5>      1Ð1Ð04    pc      Add LitterVers() and LetterAssist(): default methods for
                                    handling vers and assist messages.
         <4>   30Ð3Ð2003    pc      Add constant ID for a STR# identifying Max "finder" categories.
                                    And a function LitterAddClass() to use it.
         <3>  30Ð12Ð2002    pc      Add constants to access version information stored in 'STR#"
                                    resources.
         <2>  29Ð11Ð2002    pc      Initial check in.
         <2>  28Ð11Ð2002    pc      Tidy up after check in.
         <1>  28Ð11Ð2002    pc      Initial check in.

		2-Apr-2001:		First implementation. Based on M. Matsumoto's code, but tweaked
						for speed.
	
 ******************************************************************************************/

#pragma once
#ifndef LITTERLIB_H
#define LITTERLIB_H

#pragma mark ¥ Include Files


#include "MaxUtils.h"

#ifndef LITTER_USE_OBEX
	#define LITTER_USE_OBEX 0
#else
	#include "ext_obex.h"
#endif



#pragma mark ¥ Macros

#ifdef __GNUC__
	// GNU C requires the entrypoint function to match one of two possible implicit prototypes
	// To support this, the Max API changed to use the form int main(), although the return
	// value is a dummy.
	#define MAIN_RETURN_TYPE		int
	#define MAIN_RETURN_STATEMENT	return 0
#else
	// Traditionally, they entrypoint function for Max externals was declared void main()
	#define MAIN_RETURN_TYPE		void
	#define MAIN_RETURN_STATEMENT
#endif

#if LITTER_USE_OBEX
	#define LITTER_CORE_OBJECT(TYPE, NAME)		TYPE NAME; void* obex
			// TYPE can be Object, Box, t_pxobject, etc.
	#define LitterCalcOffset(STRUCT)			calcoffset(STRUCT, obex)
#else
	#define LITTER_CORE_OBJECT(TYPE, NAME)		TYPE NAME
			// TYPE can be Object, Box, t_pxobject, etc.
	#define LitterCalcOffset(STRUCT)			0
#endif


#pragma mark ¥ Constants

	// Standard STR# indices
#ifdef WIN_VERSION

	enum {
		lpStrIndexObjectVersShort	= 1,
		lpStrIndexObjectVersLong,
		lpStrIndexPackageVersShort,
		lpStrIndexPackageVersLong,
		lpStrIndexLitterCategory,
		lpStrIndexMax4Category,
		
		lpStrIndexLastStandard		= lpStrIndexMax4Category
		};
		
#else

	enum {
		lpStrIndexObjectVersShort		= 1,
		lpStrIndexObjectVersLong,
		lpStrIndexObjectVersVal,
		lpStrIndexPackageVersShort,
		lpStrIndexPackageVersLong,
		lpStrIndexPackageVersVal,
		
		lpStrIndexLastStandard		= lpStrIndexPackageVersVal
		};
		
#endif

	// ...and for accessing version information
enum {
		// ID's according to Mac OS 'vers' conventions
	lpVersIDObject		= 1,	// Use information from 'vers'(1) resource
	lpVersIDPackage,			// Use information from 'vers'(2) resource
	
		// Values for the  
	lpVersTypeShort		= 1,	// Version short string
	lpVersTypeLong,				// Version long string
	lpVersTypeVal,				// Version as a 32-bit integer value (encoded as string)
	
	lpVersIDDefault		= lpVersIDObject,
	lpVersTypeDefault	= lpVersTypeShort
	};


	// Range correction values
enum ActionType {
		actStet,
		actClip,
		actWrap,
		actReflect
		};

enum Expect {
	expMean	= 0,
	expMedian,
	expMode,
	expVar,
	expStdDev,
	expSkew,
	expKurtosis,
	
	expMin,
	expMax,
	
	expEntropy,
	
	expCount
	};


enum LitterLicense {
	llStarter		= 'Strt' ^ 'LLic',
	llArtist		= 'Arts' ^ 'LLic',
	llDeveloper		= 'Dvlp' ^ 'LLic'
	};

enum CategoryClass {
	ccNone		= 0,		// Don't add Litter externals to Max' New Object list
	ccMatchMax,				// Match categories to Max2 or Max4 conventions, depending
							// on what is running
	ccLitterGen,			// List everything under Litter
	ccLitterSpec			// List according to specific classes (Chance, Mutation, etc.)
	};


	// Some constants we need. This stuff is simple enough to handle with macros
#ifndef kULongMax
	#define kULongMax			((unsigned long) 0xffffffff)
	#define kLongMax			((long) 0x7fffffff)
	#define kLongMin			((long) 0x80000000)
	#define kUShortMax			((unsigned short) 0xffff)
#endif

	// For floating point constants it's better to use extern const...
extern const double kPi,
					k2pi,
					kHalfPi,
					kQuarterPi,
					kNatLogBase,			// e
					
					kFloatEpsilon,
					kULongMaxInv;			// Maps long integers to [0 .. 1]

	
	// Symmetry values, used by coshy, expo, linny, ppp~, and others
	// NB:	The constants are set up to allow multiplication, comparison with sign(),
	//		etc.
enum Symmetry {
	symNeg		= -1,
	symSym,
	symPos
	};

// Various constants
#define kLPcatID			128

#if LITTER_USE_OBEX
	#define kAttrFlagsReadOnly	(ATTR_SET_OPAQUE | ATTR_SET_OPAQUE_USER)
#endif



#pragma mark ¥ Type Definitions

#ifndef MAXARG      // Not sure why this is not defined here, must track down
#define MAXARG 7
#endif
typedef int tTypeList[MAXARG];		// Up to seven arguments types (A_LONG, etc.)
									// unused entries filled with A_NOTHING

#if LITTER_USE_OBEX
	typedef long tArgCount;
#else
	typedef short tArgCount;
#endif

typedef enum Symmetry	eSymmetry;		// Need to rename to eSymmetry ??
typedef enum ActionType	eActionType;

typedef enum Expect		eExpectSelector;
typedef double (*tExpectFunc)(Object*, eExpectSelector);

typedef enum LitterLicense	eLitterLicense;
typedef enum CategoryClass	eCategoryClass;

#if 0
	// Not using this right now
typedef struct {
	long			vers;		// Version number of tPrefs struct
	eLitterLicense	license;	// License code
	eCategoryClass	cc;
	Byte			padByte;	// Padding for struct alignment.
	short			padShort;	// May be used in future versions.
	} tPrefs;
#endif

typedef struct strList* tStrListPtr;



#pragma mark ¥ Global Variables

#if LITTER_USE_OBEX
	extern t_class*			gObjectClass;
#else
	extern t_messlist*		gObjectClass;
#endif

#ifdef WIN_VERSION
	extern HINSTANCE	gDLLInstanceHdl;
	extern FILETIME		gModDate;
#else
	extern short			gStrResID;
	extern unsigned long	gModDate;
#endif

#pragma mark ¥ Function Prototypes

	// Wrappers for class_new()/setup() and class_addmethod()/addXXX() functions
	// These are aware of obex-availability and Do The Right Thing, so Litter Objects
	// don't need to worry (quite so much) about this.
void	LitterSetupClass	(const char		iName[],
							 long			iSize,
							 long			iObexOffset,	// Use LitterCalcOffset() macro
							 method			iNewMeth,
							 method			iFreeMeth,
							 method			iPaletteMeth,	// For UI objects, otherwise NIL
							 const tTypeList iArgTypes);
void	LitterSetupClassGimme(const char	iName[],
							  long			iSize,
							  long			iObexOffset,	// Use LitterCalcOffset() macro
							  method		iNewMeth,
							  method		iFreeMeth,
							  method		iPaletteMeth);	// For UI objects, otherwise NIL

	//	Initialize the Litter Toolbox (call once)
void	LitterInit			(const char[], UInt32);

	// Pattr-agnostice wrapper for object_alloc()/newobject()
Object* LitterAllocateObject(void);


	// Useful & common functions
void	LitterAddClass		(const char[], short);
void	LitterAssist		(long, long, short, short, char[]);
void	LitterAssistVA		(long, long, short, short, char[], ...);
void	LitterAssistResFrag	(long, long, short, short, char[], short);
void	LitterGetVersStr	(long, long, char[]);
long	LitterGetVersVal	(long);
void	LitterVers			(Object*, long, long);
void	LitterGetIndString	(short, char[]);
void	LitterHello			(const char[]);

tStrListPtr LitterCopyStrListResourceByName(const char[]);
void	LitterGetCStringFromStrList(char[kMaxResourceStrSize], const tStrListPtr, int);

#if LITTER_USE_OBEX
	// Attribute helper functions
t_max_err LitterGetAttrAtom(Atom*, long*, Atom**);
t_max_err LitterGetAttrInt(long, long*, Atom**);
t_max_err LitterGetAttrFloat(double, long*, Atom**);
t_max_err LitterGetAttrSym(Symbol*, long*, Atom**);
#endif

void	LitterInfo			(const char[], tObjectPtr, method);
void	LitterExpect		(tExpectFunc, Object*, Symbol*, Symbol*, Boolean);



	// Cheesy trial protection
// static inline Boolean LitterTimeBomb(const char[]);

short	OpenPrefsFile		(SignedByte, Boolean, long*);


#ifdef WIN_VERSION
	// Mac OS functionality we emulate on Windows
void	GetTime				(DateTimeRec*);
#endif

#pragma mark -
#pragma mark ¥ Inline Functions

static inline short		LitterGetMaxVersion(void)
				{ return maxversion() & 0x3fff; }				// Mask out standalone flag
static inline Boolean	LitterInsideStandalone(void)
				{ return (maxversion() & 0x4000) != 0; }

#if LITTER_USE_OBEX

static inline void LitterAddBang(method iMeth)
		{ class_addmethod(gObjectClass, iMeth, "bang", A_NOTHING); }

static inline void LitterAddInt(method iMeth)
		{ class_addmethod(gObjectClass, iMeth, "int", A_LONG, 0); }

static inline void LitterAddFloat(method iMeth)
		{ class_addmethod(gObjectClass, iMeth, "float", A_FLOAT, 0); }

static inline void LitterAddMess0(method iMeth, const char iName[])
		{ class_addmethod(gObjectClass, iMeth, (char*) iName, A_NOTHING); }

static inline void LitterAddMess1(method iMeth, const char iName[], short iArg1)
		{ class_addmethod(gObjectClass, iMeth, (char*) iName, iArg1, 0); }

static inline void LitterAddMess2(	method iMeth, const char iName[],
									short iArg1, short iArg2)
		{ class_addmethod(gObjectClass, iMeth, (char*) iName, iArg1, iArg2, 0); }

static inline void LitterAddMess3(	method iMeth, const char iName[],
									short iArg1, short iArg2, short iArg3)
		{ class_addmethod(gObjectClass, iMeth, (char*) iName, iArg1, iArg2, iArg3, 0); }

static inline void LitterAddMess4(	method iMeth, const char iName[],
									short iArg1, short iArg2, short iArg3, short iArg4)
		{ class_addmethod(gObjectClass, iMeth, (char*) iName, iArg1, iArg2, iArg3, iArg4, 0); }

static inline void LitterAddCant(method iMeth, const char iName[])
		{ class_addmethod(gObjectClass, iMeth, (char*) iName, A_CANT, 0); }

static inline void LitterAddGimme(method iMeth, const char iName[])
		{ class_addmethod(gObjectClass, iMeth, (char*) iName, A_GIMME, 0); }

#else		// LITTER_USE_OBEX

static inline void LitterAddBang(method iMeth)
		{ addbang(iMeth); }

static inline void LitterAddInt(method iMeth)
		{ addint(iMeth); }

static inline void LitterAddFloat(method iMeth)
		{ addfloat(iMeth); }

static inline void LitterAddMess0(method iMeth, const char iName[])
		{ addmess(iMeth, (char*) iName, A_NOTHING); }

static inline void LitterAddMess1(method iMeth, const char iName[], short iArg1)
		{ addmess(iMeth, (char*) iName, iArg1, 0); }

static inline void LitterAddMess2(	method iMeth, const char iName[],
									int iArg1, int iArg2)
		{ addmess(iMeth, (char*) iName,  iArg1, iArg2, 0); }

static inline void LitterAddMess3(	method iMeth, const char iName[],
									int iArg1, int iArg2, int iArg3)
		{ addmess(iMeth, (char*) iName, iArg1, iArg2, iArg3, 0); }

static inline void LitterAddMess4(	method iMeth, const char iName[],
									int iArg1, int iArg2, int iArg3, int iArg4)
		{ addmess(iMeth, (char*) iName, iArg1, iArg2, iArg3, iArg4, 0); }

static inline void LitterAddCant(method iMeth, const char iName[])
		{ addmess(iMeth, (char*) iName, A_CANT, 0); }

static inline void LitterAddGimme(method iMeth, const char iName[])
		{ addmess(iMeth, (char*) iName, A_GIMME, 0); }

#endif			// LITTER_USE_OBEX

#endif			// LITTERLIB_H
