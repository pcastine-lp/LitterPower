/*
	File:		i.c

	Contains:	Max external object producing text of I Ching oracles

	Written by:	Peter Castine

	Copyright:	© 2001-2002 Peter Castine

	Change History (most recent first):

        <11>   26Ð4Ð2006    pc      Update for new LitterLib organization.
        <10>   23Ð3Ð2006    pc      Fix problem with gCommonSym array allocation.
         <9>     26Ð2Ð05    pc      Finally kill all references to the legacy <SetUpA4.h> and
                                    <A4Stuff.h> stuff. 
                                    <A4Stuff.h> stuff. 
         <8>     28Ð1Ð04    pc      Need to #include <string.h> now that we are abandoning the
                                    micro-stdlib.
         <7>     11Ð1Ð04    pc      Update for modified LitterInit()
         <6>      8Ð1Ð04    pc      Update for Windows.
         <5>    6Ð7Ð2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <4>  30Ð12Ð2002    pc      Add object version to IChingInfo()
         <3>  30Ð12Ð2002    pc      Use 'STR#' resource instead of faux 'Vers' resource for storing
                                    version information used at run-time.
         <2>  28Ð11Ð2002    pc      Tidy up after initial check in.
         <1>  28Ð11Ð2002    pc      Initial check in.
		10-Jul-2001:	First implementation
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark ¥ Include Files

#include <string.h>								// all for strchr()

#include "IChingCore.h"							// #includes LitterLib.h
#include "TrialPeriodUtils.h"
#include "Taus88.h"


#pragma mark ¥ Constants

const char	kClassName[]	= "lp.i";			// Class name

	// Indices for Assist STR# resource
enum {
	strIndexInMain		= lpStrIndexLastStandard + 1,
	strIndexInFuture,
	
	strIndexOutWisdom
	};

#ifdef WIN_VERSION
	enum {
		strIndexCommon		= strIndexOutWisdom + 1,
		strIndexTrigrams	= strIndexCommon + 20,
		strIndexChien		= strIndexTrigrams + 16,
		strIndexKan			= strIndexChien + 80,
		strIndexKen			= strIndexKan + 80,
		strIndexChen		= strIndexKen + 80,
		strIndexSun			= strIndexChen + 80,
		strIndexLi			= strIndexSun + 80,
		strIndexKun			= strIndexLi + 80,
		strIndexTui			= strIndexKun + 80
		};
#else
	const char	kGenStr[]		= "lp.i Common",	// Additional 'STR#' resources
				kTrigrams[]		= "lp.i Trigrams",
				kChienStrs[]	= "lp.i ChÕien",
				kKanStrs[]		= "lp.i KÕan",
				kKenStrs[]		= "lp.i Ken",
				kChenStrs[]		= "lp.i Chen",
				kSunStrs[]		= "lp.i Sun",
				kLiStrs[]		= "lp.i Li",
				kKunStrs[]		= "lp.i KÕun",
				kTuiStrs[]		= "lp.i Tui";
#endif
	// Indicies for Common texts STR# Resource
	// These are also used as indices into the gCommonSym array.
	// The gCommonSym[0] will be gensym("list"), which must be hardwaried.
	// This has the advantage that we can use the same (unit-based) indices for
	// the STR# resource as we use for the (zero-based) array of symbols.
enum CommonSymbols {
	csResID			= 17516,		// Only Resource accessed by ID in this external!
									// Probably the only resID used in the entire LP Package	
	csTriAbove		= 1,
	csTriBelow,
	csTriUpMid,
	csTriLoMid,
	csGovRuler,
	csConRuler,
	csJudge,
	csImage,
	csTheLines,
	csNoRulers,
	csSix,
	csNine,
	csLineBottom,
	csLine2,
	csLine3,
	csLine4,
	csLine5,
	csLineTop,
	csMeans,
	csFuture,
	
	csCount		= csFuture
	};

	// Offsets for hexagram informaion in STR# resource,
	// relative to first entry for the hexagram
enum HexagramInfoOffsets {
	offOrigName		= 0,
	offTransName,
	offJudgement,
	offImage,
#ifndef WIN_VERSION
	offOverflow,					// Used when one of the components is longer than
									// the 255 char limit for 'STR#' resources 
#endif
	offLineBottom,					// Will probably not need higher than bottom line
	offLine2,
	offLine3,
	offLine4,
	offLine5,
	offLineTop,
	
	offsetsPerHexagram = offLineTop + 1
	};

enum TrigramOptions {
	tOptsNone		= 0,			// Never send trigrams
	tOptsMain,						// Top and bottom trigrams
	tOptsAll						// Also include upper and lower middle trigrams
	};
	
enum LineOptions {
	lOptsNone		= 0,			// Never send texts of line oracles
	lOptsChange,					// Send texts of changing lines
	lOptsGov,						// Send texts of governing lines only
	lOptsAll						// Send texts of all lines.
	};

const Byte	kStrExtensionChar	= '+',
			kMaxNewlineChar		= ',';

#pragma mark ¥ Type Definitions

typedef enum LineOptions	eLineOpts;
typedef enum TrigramOptions	eTriOpts;
typedef enum CommonSymbols	eCommonSyms;


#pragma mark ¥ Object Structure

typedef struct {
	LITTER_CORE_OBJECT(Object, coreObject);
	
	int			mainHex,			// Stash input hexagrams here.
				futureHex;			// Valid range is [1 .. 64] only.
									// 0 in future indicates no change
									// 0 in main indicates silence.
										
	Boolean		bangResets		: 1,
				hexNumInName	: 1,
				origName		: 1,
				transName		: 1,
				rulers			: 1,
				image			: 1,
				judgements		: 1;
	
	eTriOpts	triOpts			: 2;		// Must match minimum size for enums
	eLineOpts	lineOpts		: 2;
	} objIChing;


#pragma mark ¥ Global Variables

#if __LITTER_UB__
	tStrListPtr	gChingStrings[9]	= {NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL};
#else
	short	gGenStrResID		= 0,
			gChingStrResID[9]	= {0, 0, 0, 0, 0, 0, 0, 0, 0};
								// 0-index for trigram names, other indices for Houses
#endif

Symbol*		gCommonSym[csCount + 1]	= {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};


#pragma mark -

/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark ¥ Utility Functions

static inline void
IChingGetResourceString(char oBuf[], short iResID, int iIndex)
	{
#ifdef WIN_VERSION
	LitterGetIndString(gChingStrResID[iResID] + iIndex - 1, oBuf);
//	post("IChingGetResourceString() iResID %ld, iIndex %d", (long) iResID, iIndex);
//	post("  returns '%s'", oBuf);
#elif __LITTER_UB__
	LitterGetCStringFromStrList(oBuf, gChingStrings[iResID], iIndex);
	
	if (LitterGetMaxVersion() >= 0x0500) {
		CFStringRef	ms = CFStringCreateWithCString(NULL, oBuf, kCFStringEncodingMacRoman);
		if (ms != NULL) {
			CFStringGetCString(ms, oBuf, kMaxResourceStrSize,  kCFStringEncodingUTF8);			
			CFRelease(ms);
			}
		}
#else
	GetIndCString(oBuf, iResID, iIndex);
#endif
	}

#pragma mark -
#pragma mark ¥ Class Message Handlers

/******************************************************************************************
 *
 *	IChingNew(iOracle)
 *
 ******************************************************************************************/

static void*
IChingNew(
	long	iMain,
	long	iFuture)
	
	{
	objIChing*	me			= NIL;
	
	//
	// Let Max allocate us, our outlets, and additional inlets
	//
	me = (objIChing*) LitterAllocateObject();
		if (me == NIL) goto punt;
	
	// One additional inlet
	intin(me, 1);

	// One outlet, accessed through me->coreObject.o_outlet
	listout(me);
	
	// Initialize object. A lot of stuff is hard-wired
	me->bangResets		= iMain == 0;
	me->mainHex			= iMain;
	me->futureHex		= iFuture;
	me->hexNumInName	= true;
	me->origName		= true;
	me->transName		= true;
	me->rulers			= true;
	me->image			= true;
	me->judgements		= true;
	me->lineOpts		= lOptsChange;
	me->triOpts			= tOptsMain;
	
	//
	// Function return
	//
punt:
	return me;
	}

#pragma mark -
#pragma mark ¥ Auxiliary Methods

/******************************************************************************************
 *
 *	DoWisdomOut(me)
 *
 ******************************************************************************************/

	static voidPtr WisdomOutCore(voidPtr iOutlet, char iCStr[])
		{
		voidPtr	noProblem;						// Follow on the SDK convention that
												// non-NIL means "everything OK"
		char*	curLine = iCStr;
		
		do {
			// Check if there is a linebreak character and get a pointer to it.
			char*	nextLine = strchr(curLine, '\r');
//			Atom	lineAsAtom;
			
			if (nextLine != NIL) {
				*nextLine++ = '\0';
				}
			
			//SetSym(&lineAsAtom, gensym(curLine));
			//noProblem = outlet_list(iOutlet, NIL, 1, &lineAsAtom);
			noProblem = outlet_anything(iOutlet, gensym(curLine), 0, NIL);
			
			curLine = nextLine;
			} while (curLine != NIL && noProblem);
		
		return noProblem;
		}

static void*
WisdomOut(
	void* iOutlet,
	short iHouseID,
	short iBaseIndex,
	short iOffset)
	
	{
	void*		noProblem;					// Follow on the SDK convention that
											// non-NIL means "everything OK"
	char		theStr[kMaxResourceStrSize];

#ifndef WIN_VERSION							// Don't have to worry about 'STR#' resources
	Boolean		useOverflow	= false;		// exceeding 255-char limit on Windows
	long		len;
#endif
			
	IChingGetResourceString(theStr, iHouseID, iBaseIndex + iOffset);
//	GetIndCString(theStr, iResID, iBaseIndex + iOffset);
#ifndef WIN_VERSION
	len = strlen(theStr);
	if (len > 0 && theStr[len-1] == kStrExtensionChar) {
		useOverflow = true;
		theStr[len-1] = '\0';
		}
#endif
	
	noProblem = WisdomOutCore(iOutlet, theStr);
	if (!noProblem) goto punt;
	
#ifndef WIN_VERSION
	if (useOverflow) {
		IChingGetResourceString(theStr, iHouseID, iBaseIndex + offOverflow);
//		GetIndCString(theStr, iResID, iBaseIndex + offOverflow);
		noProblem = WisdomOutCore(iOutlet, theStr);
		if (!noProblem) goto punt;
		}
#endif
	
	noProblem = outlet_anything(iOutlet, gCommonSym[csNoRulers], 0, NIL);
	
punt:
	return noProblem;
	}

/******************************************************************************************
 *
 *	TrigramOut(iOutlet, iSymIndex, iTrigram)
 *	
 ******************************************************************************************/

static voidPtr
TrigramOut(
	voidPtr		iOutlet,
	eCommonSyms iSymIndex,
	eTrigram	iTrigram)
	
	{
	char	theStr[kMaxResourceStrSize];
	short	strIndex;
	Atom	atomVec[3];				// Hardwired to three symbols: position,
									// Original Trigram name, and translation
	
	// 1: Position
	SetSym(atomVec, gCommonSym[iSymIndex]);
	
	// 2: Trigram name
	strIndex = 2 * iTrigram - 1;
	IChingGetResourceString(theStr, 0, strIndex);
	SetSym(atomVec + 1, gensym(theStr));
	
	// 3: Trigram meaning
	IChingGetResourceString(theStr, 0, ++strIndex);
	SetSym(atomVec + 2, gensym(theStr));
	
	return outlet_list(iOutlet, NIL, 3, atomVec);
	}


/******************************************************************************************
 *
 *	LineHeadOut(iOutlet, iInfo, iLineNum, iFlagRulers)
 *	
 ******************************************************************************************/

static voidPtr
LineHeadOut(
	voidPtr		iOutlet,
	tHexInfoPtr iInfo,
	int			iLineNum,
	Boolean		iFlagRulers)
	
	{
	// These strings aren't defined in the resource for vague legacy reasons.
	// Also note that they are declared const, although the qualifier has to be discarded
	// inside gensym(). 
	const char	kConRulerUTF[]	= "\342\200\242 ",
				kGovRulerUTF[]	= "\302\260 ";
#if WIN_VERSION
	const char	kConRulerNative[] =	"\245 ",
				kGovRulerNative[] = "\241 ";
#else
	const char	kConRulerNative[] =	"¥ ",
				kGovRulerNative[] = "¡ ";

#endif
	
	enum {
		atomRulers			= 0,	// Symbolic constants for accessing the local
		atomSixOrNine,				// vector of atoms
		atomLineNum,
		atomMeans,
		
		atomCount					// = atomMeans + 1
		};
	
	Symbol*		rulerSym	= gCommonSym[csNoRulers];	// Initial assumption
	eLineBits	lineBit		= 1 << (iLineNum - 1);
	Atom		atomVec[atomCount];
	
	// See if we're checking for ruler lines, and adjust rulerSym as necessary
	if (iFlagRulers) {
		Boolean	hasRulers = false,
				useUTF8	= LitterGetMaxVersion() >= 0x0500;
		
		if (lineBit & iInfo->con) {
			rulerSym = gensym((char*) (useUTF8 ? kConRulerUTF : kConRulerNative));
			hasRulers = true;
			}
		
		if (lineBit  & iInfo->gov) {
			rulerSym = gensym((char*) (useUTF8 ? kGovRulerUTF : kGovRulerNative));
			hasRulers = true;
			}
		
		}
	
	// Set up atoms
	SetSym(	atomVec + atomRulers,
			rulerSym);
	SetSym(	atomVec + atomSixOrNine,
			gCommonSym[(iInfo->theLines & lineBit) ? csSix : csNine]);
	SetSym(	atomVec + atomLineNum,
			gCommonSym[iLineNum + csLineBottom - 1]);
	SetSym(	atomVec + atomMeans,
			gCommonSym[csMeans]);
	
	return outlet_list(iOutlet, NIL, atomCount, atomVec);
	}

#pragma mark -
#pragma mark ¥ Defer Time Handlers

/******************************************************************************************
 *
 *	DoNameNow(me)
 *
 ******************************************************************************************/

static void
DoNameNow(
	objIChing*	me,
	Symbol*		sym,
	short		iArgC,						// Either 1 or 0. Take hexagram from me->main
	Atom*		iArgV)						// unless iArgV specifies something else.
	
	{
	#pragma unused(sym)
	
	long		hexID		= 0;
	tHexInfoPtr	mainInfo;
	short		/*houseID,*/					// Which STR# to use
				offsetBase,					// Base index into GetStrInd(houseID, xxx);
				atomCount	= 0;			// Variable number of atoms, three max:
	Atom		atomVec[3];					// number, original name, translated name.
	char		theStr[kMaxResourceStrSize];
	
	if (iArgC > 0 && iArgV->a_type == A_LONG)
		hexID = iArgV->a_w.w_long;
	if (hexID <= 0 || 64 < hexID)			// Check validity
		hexID = me->mainHex;
	
	mainInfo	= (tHexInfoPtr) (kHexagramInfo - 1) + hexID;
//	houseID		= gChingStrResID[mainInfo->house];
	offsetBase	= (mainInfo->housePos - 1) * offsetsPerHexagram + 1;
	
	if (me->hexNumInName) {
		Str32 numStr;
		NumToString(hexID, numStr);			// Convert number to text
		numStr[++numStr[0]] = '.';			// Append period
		MoveP2CStr(numStr, theStr);
		SetSym(atomVec, gensym((char*) theStr));
		atomCount += 1;
		}
	
	if (me->origName) {
//		GetIndCString(theStr, houseID, offsetBase + offOrigName);
		IChingGetResourceString(theStr, mainInfo->house, offsetBase + offOrigName);
		SetSym(atomVec + atomCount++, gensym(theStr));
		}
	
	if (me->transName) {
//		GetIndCString(theStr, houseID, offsetBase + offTransName);
		IChingGetResourceString(theStr, mainInfo->house, offsetBase + offTransName);
		SetSym(atomVec + atomCount++, gensym(theStr));
		}
	
	if (atomCount > 0)
		outlet_list(me->coreObject.o_outlet, NIL, atomCount, atomVec);
		
	}
	
/******************************************************************************************
 *
 *	DoTrigramsNow(me)
 *
 ******************************************************************************************/

static void
DoTrigramsNow(
	objIChing*	me,
	Symbol*		sym,
	short		argc,
	Atom		argv[])
	
	{
	#pragma unused(sym, argc, argv)
	
	eLineBits	theLines;
	voidPtr		noProblem,
				theOutlet	= me->coreObject.o_outlet;
	
	if (me->triOpts == tOptsNone)				// Sanity check
		return;									// ...and quick punt if it fails
	
	theLines = kHexagramInfo[me->mainHex - 1].theLines;
	
	noProblem = TrigramOut(theOutlet, csTriAbove, TopTrigram(theLines));
		if ( !noProblem ) return;
	
	if (me->triOpts == tOptsAll) {
		noProblem = TrigramOut(theOutlet, csTriUpMid, UpMidTrigram(theLines));
			if ( !noProblem ) return;
		noProblem = TrigramOut(theOutlet, csTriLoMid, LowMidTrigram(theLines));
			if ( !noProblem ) return;
		}
	
	noProblem = TrigramOut(theOutlet, csTriBelow, BottomTrigram(theLines));
		if ( !noProblem ) return;
	
	outlet_anything(theOutlet, gCommonSym[csNoRulers], 0, NIL);

	}
	
	
/******************************************************************************************
 *
 *	DoRulersNow(me)
 *
 ******************************************************************************************/

static void
DoRulersNow(
	objIChing*	me,
	Symbol*		sym,
	short		argc,
	Atom		argv[])
	
	{
	#pragma unused(sym, argc, argv)
	
	enum {						// Symbolic indices into atomVec
		atomIntro		= 0,
		atomRuler1Val,
		atomRuler1Pos,
		atomConjunction,
		atomRuler2Val,
		atomRuler2Pos,
		
		atomVecSize				// = atomRuler2Pos + 1
		};
		
	Atom		atomVec[atomVecSize];
	short		atomCount,
				curLine;
	tHexInfoPtr	hexInfo;
	eLineBits	rulingLines,
				curLineBit;
	voidPtr		noProblem;
	
	if ( !me->rulers )						// Sanity check
		return;								// ... and a quick punt if it fails
	
		// Typecast here to override const qualifier, but treat as if
		// const were still in effect
	hexInfo	= (tHexInfoPtr) (kHexagramInfo - 1) + me->mainHex;
	
	rulingLines = hexInfo->con;
	if (rulingLines) {
		SetSym(atomVec, gCommonSym[csConRuler]);
		atomCount = 1;
		
		curLine = 1;
		curLineBit = lineBottom;
		do {
			if (rulingLines & curLineBit) {
				if (atomCount >= atomConjunction) {
					// This is pretty rare, so lazy us gensym()s the conjunction
					// on the fly
					SetSym(atomVec + atomCount++, gensym("and"));
					}
				SetSym(	atomVec + atomCount++,
						gCommonSym[(hexInfo->theLines & curLineBit) ? csSix : csNine]);
				SetSym(	atomVec + atomCount++,
						gCommonSym[curLine + csLineBottom - 1]);
				
				if ((rulingLines ^= curLineBit) == linesNone) break;
				}
			curLine		+= 1;
			curLineBit	<<= 1;
			} while (true);						// Loop terminates internally!
		
		noProblem = outlet_list(me->coreObject.o_outlet, NIL, atomCount, atomVec);
		if ( !noProblem) return;			// other quick punt
		}
	
	// ASSERT: rulingLines != linesNone, so execute the following unconditionally
	rulingLines = hexInfo->gov;	
	SetSym(atomVec, gCommonSym[csGovRuler]);
	atomCount = 1;
	
	curLine = 1;
	curLineBit = lineBottom;
	do {
		if (rulingLines & curLineBit) {
			if (atomCount >= atomConjunction) {
				// This is pretty rare, so lazy us gensym()s the conjunction
				// on the fly
				SetSym(atomVec + atomCount++, gensym("and"));
				}
			SetSym(	atomVec + atomCount++,
					gCommonSym[(hexInfo->theLines & curLineBit) ? csSix : csNine]);
			SetSym(	atomVec + atomCount++,
					gCommonSym[curLine + csLineBottom - 1]);
			
			if ((rulingLines ^= curLineBit) == linesNone) break;
			}
		curLine		+= 1;
		curLineBit	<<= 1;
		} while (true);						// Loop terminates internally!
	
	noProblem = outlet_list(me->coreObject.o_outlet, NIL, atomCount, atomVec);
	if ( !noProblem) return;
	
	outlet_anything(me->coreObject.o_outlet, gCommonSym[csNoRulers], 0, NIL);
	
	}
	
/******************************************************************************************
 *
 *	DoJudgementNow(me)
 *
 ******************************************************************************************/

static void
DoJudgementNow(
	objIChing*	me,
	Symbol*		sym,
	short		iArgC,
	Atom*		iArgV)
	
	{
	#pragma unused(sym)
	
	long		hexID		= 0;
	tHexInfoPtr	hexInfo;
	short		/*houseID,*/				// Which STR# to use
				offsetBase;				// Base index into GetStrInd(houseID, xxx);
	
	if (iArgC > 0 && iArgV->a_type == A_LONG)
		hexID = iArgV->a_w.w_long;
	if (hexID <= 0 || 64 < hexID)			// Check validity
		hexID = me->mainHex;
	
					// Typecast here to override const qualifier, but treat as if
					// const were still in effect
	hexInfo		= (tHexInfoPtr) (kHexagramInfo - 1) + hexID;
//	houseID		= gChingStrResID[hexInfo->house];
	offsetBase	= (hexInfo->housePos - 1) * offsetsPerHexagram + 1;
	
	if (iArgC == 0)
		outlet_anything(me->coreObject.o_outlet, gCommonSym[csJudge], 0, NIL);
	WisdomOut(me->coreObject.o_outlet, hexInfo->house, offsetBase, offJudgement);
	
	}
		
		
/******************************************************************************************
 *
 *	DoImageNow(me)
 *
 ******************************************************************************************/

static void
DoImageNow(
	objIChing*	me,
	Symbol*		sym,
	short		argc,
	Atom		argv[])
	
	{
	#pragma unused(sym, argc, argv)
	
	tHexInfoPtr	hexInfo;
	short		/*houseID,*/				// Which STR# to use
				offsetBase;				// Base index into GetStrInd(houseID, xxx);
	
					// Typecast here to override const qualifier, but treat as if
					// const were still in effect
	hexInfo		= (tHexInfoPtr) (kHexagramInfo - 1) + me->mainHex;
	
//	houseID		= gChingStrResID[hexInfo->house];
	offsetBase	= (hexInfo->housePos - 1) * offsetsPerHexagram + 1;
	
	outlet_anything(me->coreObject.o_outlet, gCommonSym[csImage], 0, NIL);
//	WisdomOut(me->coreObject.o_outlet, houseID, offsetBase, offImage);
	WisdomOut(me->coreObject.o_outlet, hexInfo->house, offsetBase, offImage);
	
	}
	
	
/******************************************************************************************
 *
 *	DoLinesNow(me)
 *
 ******************************************************************************************/

static void
DoLinesNow(
	objIChing*	me,
	Symbol*		sym,
	short		argc,
	Atom		argv[])
	
	{
	#pragma unused(sym, argc, argv)
	
	tHexInfoPtr	mainInfo,
				futureInfo;
	short		/*houseID,*/					// Which STR# to use
				offsetBase,					// Base index into GetStrInd(houseID, xxx);
				curLine;
	eLineBits	linesToDo;
	
					// Typecast here to override const qualifier, but treat as if
					// const were still in effect
	mainInfo	= (tHexInfoPtr) (kHexagramInfo - 1) + me->mainHex;
	futureInfo	= (tHexInfoPtr) (kHexagramInfo - 1) + me->futureHex;
	
//	houseID		= gChingStrResID[mainInfo->house];
	offsetBase	= (mainInfo->housePos - 1) * offsetsPerHexagram + 1;
	
	switch (me->lineOpts) {
		case lOptsNone:
			linesToDo = linesNone;
			break;
		
		case lOptsChange:
			linesToDo = mainInfo->theLines ^ futureInfo->theLines;
			break;
		
		case lOptsGov:
			// Special case, Send out Constituting Rulers... then Constituting Rulers
			linesToDo = mainInfo->con;
			if (linesToDo) {
				curLine = 1;
				do {
					voidPtr noProblem =
						LineHeadOut(me->coreObject.o_outlet, mainInfo, curLine, me->rulers);
					if ( !noProblem ) return;		// Quick punt if the call fails
					
					if (linesToDo & 0x01) {
						noProblem = WisdomOut(	me->coreObject.o_outlet,
//												houseID, offsetBase,
												mainInfo->house, offsetBase,
												curLine - 1 + offLineBottom);
						if ( !noProblem ) return;
						}
					
					curLine		+= 1;
					linesToDo	>>= 1;
					} while (linesToDo);
				}
			
			// ... and let normal line handling do any Ruling governers
			linesToDo = mainInfo->gov;
			break;
		
		default:
			// Must be lOptsAll
			linesToDo = lineMaskAll;
			break;
		}
	
	if (linesToDo) {
		curLine = 1;
		do {
			if (linesToDo & 0x01) {
				voidPtr noProblem =
					LineHeadOut(me->coreObject.o_outlet, mainInfo, curLine, me->rulers);
				if ( !noProblem ) return;
				
				noProblem = WisdomOut(	me->coreObject.o_outlet,
//									houseID, offsetBase,
									mainInfo->house, offsetBase,
									curLine - 1 + offLineBottom);
				if ( !noProblem ) return;					
				}
			curLine		+= 1;
			linesToDo	>>= 1;
			} while (linesToDo);
		}
		
	}

/******************************************************************************************
 *
 *	IChingDoBang(me)
 *
 ******************************************************************************************/

static void
IChingDoBang(
	objIChing*	me,
	Symbol*		sym,
	short		argc,
	Atom		argv[])
	
	{
	#pragma unused(sym, argc, argv)
	
	DoNameNow(me, NIL, 0, NIL);
	DoTrigramsNow(me, NIL, 0, NIL);
	DoRulersNow(me, NIL, 0, NIL);
	DoJudgementNow(me, NIL, 0, NIL);
	DoImageNow(me, NIL, 0, NIL);
	DoLinesNow(me, NIL, 0, NIL);
	
	if (me->futureHex != 0 && me->futureHex != me->mainHex) {
		Atom	theParam;
		SetLong(&theParam, me->futureHex);
		
		outlet_anything(me->coreObject.o_outlet, gCommonSym[csFuture], 0, NIL);
		DoNameNow(me, NIL, 1, &theParam);
		DoJudgementNow(me, NIL, 1, &theParam);
		}
	
	}

#pragma mark -
#pragma mark ¥ Object Message Handlers

/******************************************************************************************
 *
 *	IChingBang(me)
 *
 ******************************************************************************************/

static void
IChingBang(
	objIChing* me)
	
	{
	
	if (me->bangResets || me->mainHex == 0) {
		tOracle theOracle;
		ThrowSticks(&theOracle);
		me->mainHex		= theOracle.mainHexagram;
		me->futureHex	= theOracle.futureHexagram;
		}
	
	defer(me, (method) IChingDoBang, NIL, 0, NIL);
	
	}

static void
IChingPresent(
	objIChing*	me,
	long		iHex)
	
	{
	
	if (0 < iHex && iHex <= 64) {
		me->mainHex = iHex;
		defer(me, (method) IChingDoBang, NIL, 0, NIL);
		}
	
	}

static void
IChingFuture(
	objIChing*	me,
	long		iHex)
	{
	if (0 < iHex && iHex <= 64) {
		me->futureHex = iHex;
		}
	}

static void
IChingName(
	objIChing*	me,
	long		iOptFlags)
	
	{
	enum {
		optNone			= 0x00,
		
		optNumber		= 0x01,
		optOrigName		= 0x02,
		optTransName	= 0x04,
		
		optMask			= 0x07
		};
	
	
	iOptFlags &= optMask;
	
	if (iOptFlags != optNone) {
		me->hexNumInName	= (iOptFlags & optNumber) != 0;
		me->origName		= (iOptFlags & optOrigName) != 0;
		me->transName		= (iOptFlags & optTransName) != 0;
		}
	
	defer(me, (method) DoNameNow, NIL, 0, NIL);
	
	}

static void
IChingTrigrams(
	objIChing*	me,
	long		iOpts)
	
	{
	
	if (iOpts == -1)
		iOpts = tOptsAll;
	
	if (0 < iOpts && iOpts <= tOptsAll)
		me->triOpts = iOpts;
	
	defer(me, (method) DoTrigramsNow, NIL, 0, NIL);
	
	}

static void
IChingRulers(
	objIChing* me)
	
	{
	
	me->rulers = true;
	
	defer(me, (method) DoRulersNow, NIL, 0, NIL);
	
	}

static void
IChingImage(
	objIChing* me)
	
	{
	
	me->image = true;
	
	defer(me, (method) DoImageNow, NIL, 0, NIL);
	
	}
		
static void
IChingJudgement(
	objIChing* me)
	
	{
	
	me->judgements = true;
	
	defer(me, (method) DoJudgementNow, NIL, 0, NIL);
	
	}

static void
IChingLines(
	objIChing*	me,
	long		iOpts)
	
	{
	
	if (iOpts == -1)
		iOpts = lOptsAll;
	
	if (0 < iOpts && iOpts <= lOptsAll)
		me->lineOpts = iOpts;
	
	defer(me, (method) DoLinesNow, NIL, 0, NIL);
	
	}

/******************************************************************************************
 *
 *	IChingSet(me, iSource)
 *
 ******************************************************************************************/

static void
IChingSet(
	objIChing*	me,
	long		iMain,
	long		iFuture)
	
	{
	
	if (0 < iMain && iMain <= 64)
		me->mainHex = iMain;
	if (0 < iFuture && iFuture <= 64)
		me->futureHex = iFuture;
	
	}


/******************************************************************************************
 *
 *	IChingTattle(me)
 *
 ******************************************************************************************/

static void
IChingTattle(
	objIChing* me)
	
	{
	
	post("%s state:", kClassName);
	post("  Current Main Hexagram: %d", me->mainHex);
	post("  Current Future Hexagram: %d", me->futureHex);
	post("  Bang resets: %s", me->bangResets ? "yes" : "no");
	post("  Include Hexagram Number with Name: %s", me->hexNumInName ? "yes" : "no");
	post("  Include Original Name: %s", me->origName ? "yes" : "no");
	post("  Include Translated Name: %s", me->transName ? "yes" : "no");
	post("  Indicate ruling lines: %s", me->rulers ? "yes" : "no");
	post("  Include image: %s", me->image ? "yes" : "no");
	post("  Include judgements: %s", me->judgements ? "yes" : "no");
	post("  Trigram Options: %d", (int) me->triOpts);
	post("  Line Options: %d", (int) me->lineOpts);
	
	}

/******************************************************************************************
 *
 *	IChingAssist
 *	IChingInfo(me)
 *
 ******************************************************************************************/

static void IChingAssist(objIChing* me, void* box, long iDir, long iArgNum, char* oDestCStr)
	{
	#pragma unused(me, box)
	
	LitterAssist(iDir, iArgNum, strIndexInMain, strIndexOutWisdom, oDestCStr);
	}

static void IChingInfo(objIChing* me)
	{ LitterInfo(kClassName, &me->coreObject, (method) IChingTattle); }


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
	const tTypeList myArgTypes = {
						A_DEFLONG,			// Two arguments:	1. Initial Main Hexagram
						A_DEFLONG,			// 					2. Initial Future Hexagram
						A_NOTHING
						};
	
	short	symIndex;
	
	LITTER_CHECKTIMEOUT(kClassName);
	
	// Standard Max setup() call
	
	LitterSetupClass(	kClassName,
						sizeof(objIChing),				// Class object size
						LitterCalcOffset(objIChing),	// Magic "Obex" Calculation
						(method) IChingNew,				// Instance creation function
						NIL,							// No custom deallocation function
						NIL,							// No menu function
						myArgTypes);				// See above

	// Messages
	LITTER_TIMEBOMB LitterAddBang	((method) IChingBang);
	LITTER_TIMEBOMB LitterAddInt	((method) IChingPresent);
	LitterAddMess1	((method) IChingFuture,		"in1",			A_LONG);
	LitterAddMess2	((method) IChingSet,		"set",			A_LONG, A_DEFLONG);
	LitterAddMess1	((method) IChingName,		"name",			A_DEFLONG);
	LitterAddMess1	((method) IChingTrigrams,	"trigrams",		A_DEFLONG);
	LitterAddMess0	((method) IChingJudgement,	"judgement");
	LitterAddMess0	((method) IChingImage,		"image");
	LitterAddMess1	((method) IChingLines,		"lines",		A_DEFLONG);
	
	LitterAddMess0	((method) IChingTattle,		"tattle"); 
	LitterAddCant	((method) IChingTattle,		"dblclick"); 
	LitterAddCant	((method) IChingAssist,		"assist");
	LitterAddCant	((method) IChingInfo,		"info");
	
	// Generate common symbols we use a lot
	for (symIndex = csCount; symIndex > 0; symIndex -= 1) {
		char cStr[kMaxResourceStrSize];
	#if WIN_VERSION	
		LitterGetIndString(symIndex + strIndexCommon - 1, cStr);
	#else
		GetIndCString(cStr, csResID, symIndex);
	#endif
		gCommonSym[symIndex] = gensym((char*) cStr);
		}
	gCommonSym[0] = gensym("list");
	 
	
#ifdef WIN_VERSION
	gChingStrResID[0]	= strIndexTrigrams;
	gChingStrResID[1]	= strIndexChien;
	gChingStrResID[2]	= strIndexKan;
	gChingStrResID[3]	= strIndexKen;
	gChingStrResID[4]	= strIndexChen;
	gChingStrResID[5]	= strIndexSun;
	gChingStrResID[6]	= strIndexLi;
	gChingStrResID[7]	= strIndexKun;
	gChingStrResID[8]	= strIndexTui;
#elif __LITTER_UB__
	gChingStrings[0] = LitterCopyStrListResourceByName(kTrigrams);
	gChingStrings[1] = LitterCopyStrListResourceByName(kChienStrs);
	gChingStrings[2] = LitterCopyStrListResourceByName(kKanStrs);
	gChingStrings[3] = LitterCopyStrListResourceByName(kKenStrs);
	gChingStrings[4] = LitterCopyStrListResourceByName(kChenStrs);
	gChingStrings[5] = LitterCopyStrListResourceByName(kSunStrs);
	gChingStrings[6] = LitterCopyStrListResourceByName(kLiStrs);
	gChingStrings[7] = LitterCopyStrListResourceByName(kKunStrs);
	gChingStrings[8] = LitterCopyStrListResourceByName(kTuiStrs);
#else
	{
	short	externResFile	= CurResFile(),
			prevResFile		= PrevResFile();
	
	// Get our resources...
	resnamecopy('STR#', (char*) kClassName);		// 'STR#' resource must be named "lp.i"
	resnamecopy('STR#', (char*) kGenStr);			// ... and so on, for other resources
	resnamecopy('STR#', (char*) kTrigrams);
	resnamecopy('STR#', (char*) kChienStrs);
	resnamecopy('STR#', (char*) kKanStrs);
	resnamecopy('STR#', (char*) kKenStrs);
	resnamecopy('STR#', (char*) kChenStrs);
	resnamecopy('STR#', (char*) kSunStrs);
	resnamecopy('STR#', (char*) kLiStrs);
	resnamecopy('STR#', (char*) kKunStrs);
	resnamecopy('STR#', (char*) kTuiStrs);
	
	// ... and store the IDs assigned to them in Max Temp
	if (prevResFile > 0) {
		UseResFile(prevResFile);
		
		gChingStrResID[0]	= FindResID('STR#', kTrigrams);
		gChingStrResID[1]	= FindResID('STR#', kChienStrs);
		gChingStrResID[2]	= FindResID('STR#', kKanStrs);
		gChingStrResID[3]	= FindResID('STR#', kKenStrs);
		gChingStrResID[4]	= FindResID('STR#', kChenStrs);
		gChingStrResID[5]	= FindResID('STR#', kSunStrs);
		gChingStrResID[6]	= FindResID('STR#', kLiStrs);
		gChingStrResID[7]	= FindResID('STR#', kKunStrs);
		gChingStrResID[8]	= FindResID('STR#', kTuiStrs);
		
		UseResFile(externResFile);
		}
	}
#endif				// WIN_VERSION/__LITTER_UB__
	
	// Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();

	}

