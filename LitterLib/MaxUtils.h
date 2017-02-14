/*
	File:		MaxUtils.h

	Contains:	Utilities.

	Written by:	Peter Castine

	Copyright:	© 2000-2008 Peter Castine. All rights reserved.

	Change History (most recent first):

        <19>     19Ð1Ð06    pc      Add ReverseBits() and CountBits() utilities.
        <18>     11Ð1Ð06    pc      In removing the definition of AtomBuf we neglected to take into
                                    account that we use a different capitalization then C74. Add
                                    typedefs to allow us to contiue using our spelling conventions.
                                    Moved kMaxAssistStrLen here.
        <17>   13Ð4Ð2005    pc      Cycling '74 has now deigned to provide typedefs for atombuf, so
                                    we need to remove the provisional ones we added here.
        <16>     25Ð2Ð05    pc      Finally kill all references to the legacy <SetUpA4.h> and
                                    <A4Stuff.h> files. CodeWarrior is dropping this bit of
                                    backwards-compatibility.
                                    <A4Stuff.h> files. CodeWarrior is dropping this bit of
                                    backwards-compatibility.
        <15>     17Ð2Ð05    pc      Add ForwardXXX() utility functions to send arbitrary messages to
                                    receive objects.
        <14>     24Ð1Ð05    pc      Since Max 4.3 or 4.5 the SetUpA4.h and AfStuff.h files are no
                                    longer available. However, null-macros are provided in
                                    ext_prefix.h
        <13>     9Ð12Ð04    pc      Make non-Carbon look-alikes of the Carbon Accessor functions, so
                                    that we can use the same code for Carbon and Boron UI externals
        <12>      5Ð3Ð04    pc      Declare and define ObjectGetInlet() abstraction for Windows.
        <11>     28Ð1Ð04    pc      The micro-stdlib (strlen, strcmp, etc.) implemented here is no
                                    longer compiled/linked in by default. Too many projects need the
                                    full MSL resulting in link warnings. Use __MICRO_STDLIB__ to
                                    access the code.
        <10>     14Ð1Ð04    pc      Fix typo in #define kMaxResourceStrLen
         <9>     14Ð1Ð04    pc      Make constants kMaxResourceStrSize and kMaxResourceStrLen
                                    available to all objects.
         <8>      7Ð1Ð04    pc      More updating for Windows-compatibility. Move #include
                                    incantations from inidividual files to here for centralized
                                    maintenance.
         <7>   8Ð16Ð2003    pc      Updated for Windows & Jitter (sort of, there is not yet very
                                    clean).
         <6>   17Ð4Ð2003    pc      Add typedef for AtomBuf (documented in the Max SDK, but not
                                    defined anywhere in SDK headers). Previously defined and tested
                                    in TEditor.h (part of the Lattice project).
         <5>   30Ð3Ð2003    pc      Add new Atom accessor abstractions. Begin migrating to new names
                                    (AtomGet..., AtomSet..., etc.) for these. Also add code for
                                    getting an external's Creation/Mod dates from the file system.
         <4>   15Ð2Ð2003    pc      Add typedefs for Max data types not found in ext_mess.h & Co.
                                    (In many cases, these are just synonyms for void*, but makes
                                    usage clearer).
         <3>  30Ð12Ð2002    pc      Use 'STR#' resource instead of faux 'Vers' resource for storing
                                    version information used at run-time.
         <2>   4Ð12Ð2002    pc      Tidy up initial check in.
         <1>   4Ð12Ð2002    pc      Initial Check-in.
         
*/


/******************************************************************************************
	Previous history
		30-Jan-2002:	In a fit of pique, #define ASSIST_INLET and ASSIST_OUTLET, since
						they're nowhere to be found in the SDK :-(
		10-Dec-2000:	First implementation (mostly pulled from imLib.c)
 ******************************************************************************************/

#pragma once
#ifndef MAXUTILS_H
#define MAXUTILS_H

#pragma mark ¥ Include Files

#ifdef __NEED_JITTER_HEADERS__
	#include "jit.common.h"		// This #includes "ext.h"
	#include "max.jit.mop.h"
#else
	#include "ext.h"
#endif

	// Floating point-to-string conversion
#if defined(__MWERKS__) && !(TARGET_API_MAC_CARBON)
	// Need this to compile for Classic Mac OS
	#include <fp.h>
#endif


#ifdef WIN_VERSION

	#ifdef USE_QTML
		#include "ext_qtstubs.h"
		#include "TextUtils.h"
		#include "Resources.h"
		#include "Dialogs.h"
		#include "LowMem.h"
	#endif
	
	// For backwards-compatibility with archaic MacOS 68k/A4 world-based code
	#define EnterCodeResource()
	#define ExitCodeResource()
	#define PrepareCallback() 
	#define EnterCallback()
	#define ExitCallback()
	
	// Abstraction for accessing proxies. Under Windows always use the post-4.2 approach
     static inline long ObjectGetInlet(Object* iObj, long)
          { return proxy_getinlet(iObj); }

#else

	#ifdef MAC_VERSION
          long proxy_getinlet(Object*);	// Provisional, should be defined in SDK headers someday

          static inline long ObjectGetInlet(Object* iObj, long recepticle)
				{
			    #pragma unused(recepticle)
				
				return proxy_getinlet(iObj);
				}
     #else
          static inline long ObjectGetInlet(Object*, long iRecepticle)
               { return iRecepticle; }
     #endif

#endif		// WIN_VERSION


#ifdef __MAX_USER_INTERFACE_OBJECT__
	#include "ext_user.h"	// For Box data type
#endif

#ifdef __MAX_MSP_OBJECT__
	#include "z_dsp.h"		// For signal processing data types
	#include "buffer.h"		// Assume we'll need the buffer, too.
#endif

#pragma mark ¥ Constants

	// Compiler flag to use for projects in which we are not linking in the
	// standard C libraries
	// Once upon a time it was efficacious to write our own code for strcmp() and
	// some other simple string functions. Nowadays it seems better to link in the
	// whole MSL (or whatever).
#ifndef __MICRO_STDLIB__
	#define __MICRO_STDLIB__	0
#endif

	// Used with KeyMapToMods() and KeyisPressed()
enum keyCode {
	kcCmd		= 0x37,
	kcShift,
	kcAlphaLock,
	kcOption,
	kcControl
	};

	// Promised in ext.h or somewhere.
#ifndef ASSIST_INLET
	#define ASSIST_INLET	1
	#define ASSIST_OUTLET	2
#endif

#define kMaxResourceStrSize		256
#define kMaxResourceStrLen		255
#define kMaxAssistStrLen		 60


#pragma mark ¥ Type Definitions

	// Prior to Max/MSP 4.5, the definition of an atombuf was not exported
	// When it's exported, it's done in the file ext_maxtypes, which #defines
	// the following macro
#ifndef _EXT_MAXTYPES_H_
	typedef struct atombuf {
		long a_argc;
		t_atom a_argv[1];
	} t_atombuf;
#endif	


	// Sometimes useful to have these:
typedef Atom*		AtomPtr;
typedef Symbol*		SymbolPtr;
typedef Object*		tObjectPtr;
typedef union word	tWord;
typedef t_atombuf	AtomBuf, *AtomBufPtr;

#ifdef __MAX_MSP_OBJECT__
	typedef t_sample*	tSampleVector;
	typedef t_buffer*	tBufferPtr;
#endif

#ifdef __MAX_USER_INTERFACE_OBJECT__
	typedef Box*		tBoxPtr;
#else
	typedef void*		tBoxPtr;
#endif

	// The following are opaque, but I like to know what I'm talking about.
typedef	void*	tProxyPtr;
typedef	void*	tOutletPtr;
typedef void*	tBinBufPtr;
typedef void*	tMaxQElemPtr;			// Not to be confused with the Mac OS QElemPtr
typedef void*	tJitObject;


	// 'PC' Strings: buffers that are big enough to hold the same number of
	// characters as the analogously named Pascal String types, but guaranteed
	// to have room to append a NULL byte at the end of the string, allowing
	// them to be used as C-style strings with no more than a typecase (after
	// setting the NULL byte
typedef Byte PCStr255[sizeof(Str255) + 1];
typedef Byte PCStr63[sizeof(Str63) + 1];
typedef Byte PCStr31[sizeof(Str31) + 1];
typedef Byte PCStr15[sizeof(Str15) + 1];

#ifndef __THREADS__
	typedef void*	voidPtr;				// Normally defined in <Threads.h>, 
#endif										// Don't need to #include the whole thing.

typedef float*		floatPtr;				// Apparently not defined anywhere else


#pragma mark ¥ Extra macros

#ifndef botRight
	// How much you want to bet we're in XCode/GNU C if we need this?
	#define botRight(r)	(((Point *) &(r))[1])
#endif
#ifndef topLeft
	// See comment for macro botRight
	#define topLeft(r)	(((Point *) &(r))[0])
#endif



#pragma mark ¥ Function Prototypes

	// String stuff
void		MoveC2PStr	(const char*, Str255);
void		MoveP2CStr	(ConstStr255Param, char*);

#ifdef MAC_VERSION			// Only defined when using ext.h & Co. from OS X Max SDK
#ifndef __LITTER_UB__		// ...except don't define when building Universal Binaries
	static inline pascal char* P2CStr(Str255 iString)
		{
		CopyPascalStringToC(iString, (char*) iString);
		return (char*) iString;
		}
#endif						// __LITTER_UB__
#endif						// MAC_VERSION

	// Bit munging
UInt32		ReverseBits(UInt32);
long		CountBits(UInt32);

	// Keyboard Utilities
Boolean		KeyIsPressed(unsigned short);
short		KeyMapToMods(void);

	// Resource stuff
short		FindResID	(OSType, const char*);
void		GetVersStrings(short, char[], char[]);
short 		PrevResFile	(void);

	// Telling the user about our object.
	// Also utilities to get information from the external file
//void		TweakOuchstringAlert(short, short);
void		TweakOuchstringAlert(short, short, short);			// New, preferred version.
void		SayHello(const char[], short, short);
void		PostHello(const char[], short, short, short);

//#ifdef WIN_VERSION
#if TRUE
	UInt32	XObjGetModDate(const char*);
#ifdef WIN_VERSION
    UInt32	SecondsFromMacEpoch(const FILETIME* iFileTime);
#endif
#else
	UInt32	XObjGetModDate(short);
#endif

#ifdef WIN_VERSION
	// Convert milliseconds returned by GetTickCount() to 'ticks' (1/60 second time unit)
	static inline UInt32 XObjTickCount(void)
		{ return (UInt32) (((double) GetTickCount()) * 0.06 + 0.5); }
#else
	static inline UInt32 XObjTickCount(void)
		{ return TickCount(); }
#endif

#if defined(WIN_VERSION)
	static inline UInt32 XObjGetDateTime()
		{
		SYSTEMTIME	nowST;
		FILETIME	nowFT;
		
		GetSystemTime(&nowST);
		SystemTimeToFileTime(&nowST, &nowFT);
		
		return SecondsFromMacEpoch(&nowFT);
		}
#elif defined(__GNUC__)
	// We're supposed to be moving away from the toolbox to Core Foundation
	// For backwards compatibility we convert to the toolbox convention of counting
	// seconds since 1 Jan 1904 (always positive) rather than the CF convention of
	// counting seconds relative (positive or negative) to 1 Jan 2000
	static inline UInt32 XObjGetDateTime()
		{
		const UInt32 kXObjCFDateOffset = 3029529600u;

		return CFAbsoluteTimeGetCurrent() + kXObjCFDateOffset;
		}
#else
	static inline UInt32 XObjGetDateTime(void)
		{ UInt32 dt; GetDateTime(&dt); return dt; }	
	
#endif


	// General atom parsing utility
short		ParseAtom(AtomPtr, Boolean, Boolean, short, SymbolPtr[], const char*);

	// Other atom utilities
#ifdef __LITTER_MAX5__
	void	AtomToString(Atom*, char[]);
#else
	void	AtomToString(AtomPtr, StringPtr);
	void	AtomDraw(AtomPtr iAtom);
#endif

	// Code to forward arbitrary messages to receive objects
void		ForwardBang		(Symbol*);
void		ForwardLong		(Symbol*, long);
void		ForwardFloat	(Symbol*, double);
void		ForwardList		(Symbol*, short, Atom[]);
void		ForwardAnything	(Symbol*, Symbol*, short, Atom[]);

#if __MICRO_STDLIB__
		// We grab a few choice functions from the Standard C library so
		//	1) we don't have to load the whole danged library, and
		//	2) we don't get duplicate functions w/Max (which has Std. C File functions)
	unsigned long	strlen	(const char*);
	int				strcmp	(const char*, const char*);
	int				strncmp	(const char*, const char*, unsigned long);
	char*			strchr	(const char*, int);
#endif

	// Some resource stuff
void			SetTextItemStr	(DialogPtr, short, ConstStr255Param);
void			GetTextItemStr	(DialogPtr, short, Str255);
void			HiliteControlItem(DialogPtr, short, short);
void			SetControlItemValue(DialogPtr, short, short);
short			GetControlItemValue( DialogPtr	iDlog, short);


#pragma mark ¥ Inlined Functions

	// These are substitutes for the SETxxx macros.
	// Sometimes one wants to merge arithmetic with these calls, and macros can cause havoc.
	// Now migrating from the old "Setxxx()" names to "AtomSetxxx()".
	// Using macros for the transitional period
#define SetLong		AtomSetLong
#define SetFloat	AtomSetFloat
#define SetSym		AtomSetSym

static inline void	AtomSetSym(AtomPtr a, SymbolPtr s)
	{ a->a_type = A_SYM; a->a_w.w_sym = s; }
static inline void	AtomSetLong(AtomPtr a, long l)
	{ a->a_type = A_LONG; a->a_w.w_long = l; }
static inline void	AtomSetFloat(AtomPtr a, double f)
	{ a->a_type = A_FLOAT; a->a_w.w_float = f; }
static inline void AtomSetObject(AtomPtr iAtom, tObjectPtr iObj)
	{ iAtom->a_type = A_OBJ; iAtom->a_w.w_obj = iObj; }
static inline void AtomSetNothing(AtomPtr iAtom)
	{ iAtom->a_type = A_NOTHING; }

static inline double AtomGetFloat(AtomPtr iAtom)
	{
	double result;
	
	switch (iAtom->a_type) {
		case A_LONG:	result = (double) iAtom->a_w.w_long; break;
		case A_FLOAT:	result = (double) iAtom->a_w.w_float; break;
		default:		result = 0.0; break;				
		// NaN might be more accurate for the default case, but leave hardcare
		// typechecking to those callers who need it
		}
	
	return result;
	}

static inline long AtomGetLong(AtomPtr iAtom)
	{
	long result;
	
	switch (iAtom->a_type) {
		case A_LONG:	result = iAtom->a_w.w_long; break;
		case A_FLOAT:	result = (long) iAtom->a_w.w_float; break;
		default:		result = 0;	break;
		// Again, leave hardcare typechecking to those callers who need it
		}
	
	return result;
	}
	
static inline Boolean AtomIsNumeric(AtomPtr iAtom)
	{ return iAtom->a_type == A_LONG || iAtom->a_type == A_FLOAT; }

static inline Symbol* AtomGetSym(AtomPtr iAtom)
	{ return (iAtom->a_type == A_SYM) ? iAtom->a_w.w_sym : NIL; }
	
	// Helpers for postatom()
static inline void PostSymbol(SymbolPtr iSym)
	{ Atom symAtom; AtomSetSym(&symAtom, iSym); postatom(&symAtom); }
static inline void PostLong(long iVal)
	{ Atom longAtom; AtomSetLong(&longAtom, iVal); postatom(&longAtom); }
static inline void PostFloat(double iVal)
	{ Atom floatAtom; AtomSetFloat(&floatAtom, iVal); postatom(&floatAtom); }
	
static inline void AtomSwap(Atom* a1, Atom* a2)
	{ Atom temp = *a1; *a1 = *a2; *a2 = temp; }

	// Little helpers to get System strings into the C-style format so loved by Max
	// NOTE: As of Max 5.0 an external's resource fork is only available during main()
	//		 rescopy() does not actually copy a resource but creates a table that is
	//		 only available to assist_string() and nothing else. 
	//		 So this function can only be used during main(), which includes PostHello()
	//		 and LitterAddClass() in LitterLib.
	//		 All other Litter functions accessing strings bottle-neck through
	//		 LitterGetIndString(), which has its own private string table.
static inline void GetIndCString(char oString[], short iID, short iIndex)
	{
	GetIndString((StringPtr) oString, iID, iIndex);
	MoveP2CStr((StringPtr) oString, oString);
	}


#ifdef MAC_VERSION			// Only defined when using ext.h & Co. from OS X Max SDK
	#ifdef DateString		// Not sure why my Universal Headers are #defining this to
	#undef DateString		// IUDatePString, but that's not what we want in Carbon
	#endif
	#ifdef TimeString		// Ditto to DateString
	#undef TimeString
	#endif
#endif
#ifdef WIN_VERSION			// We have the same problem with Windows
	#ifdef DateString		// Not elegant to repeat the #undef, but easiest way to
	#undef DateString		// change for one platform w/out affecting the other
	#endif
	#ifdef TimeString		// Ditto to DateString
	#undef TimeString
	#endif
#endif

void DateCString(char oString[], UInt32 iDate, short iStyle);
static inline void TimeCString(char oString[], long iTime, Boolean iWantSecs)
	{ Str255 ts; TimeString(iTime, iWantSecs, ts, NIL); MoveP2CStr(ts, oString); }

//static inline void GetIndPCString(Byte oString[], short iStrListID, short iIndex)
//	{ GetIndString(oString, iStrListID, iIndex); oString[oString[0]+1] = '\0'; }


#ifndef OPAQUE_TOOLBOX_STRUCTS
	#define OPAQUE_TOOLBOX_STRUCTS 0
#endif
#if !OPAQUE_TOOLBOX_STRUCTS

Rect*			GetPortBounds			(CGrafPtr, Rect*);
short			GetPortTextFont			(CGrafPtr);
short			GetPortTextSize			(CGrafPtr);
short			GetPortTextFace			(CGrafPtr);

	// This one is not in our CarbonAccessors.o
static inline const BitMap*	GetPortBitMapForCopyBits(CGrafPtr iGWorld)
	{ return &(((WindowPeek) iGWorld)->port).portBits; }

Rect*			GetRegionBounds			(RgnHandle, Rect*);

Pattern*		GetQDGlobalsBlack		(Pattern*);
long			GetQDGlobalsRandomSeed	(void);
void			SetQDGlobalsRandomSeed	(long);

#endif

#endif		// MAXUTILS_H
