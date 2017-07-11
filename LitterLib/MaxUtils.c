/*
	File:		MaxUtils.c

	Contains:	Utilities.

	Written by:	Peter Castine

	Copyright:	© 2000-08 Peter Castine

	Change History (most recent first):

        <14>     19–1–06    pc      Add ReverseBits() and CountBits() utilities.
        <13>     17–2–05    pc      Add ForwardXXX() utility functions to send arbitrary messages to
                                    receive objects.
        <12>     28–1–04    pc      The micro-stdlib (strlen, strcmp, etc.) implemented here is no
                                    longer compiled/linked in by default. Too many projects need the
                                    full MSL resulting in link warnings. Use __MICRO_STDLIB__ to
                                    access the code.
        <11>      7–1–04    pc      Add utils for converting floating-point to string (use OS Int'l
                                    utilities). Also some mods for Windows compatibility.
        <10>   27–8–2003    pc      Update MoveC2PStr() MoveP2CStr() to sue BlockMoveData() instead
                                    of BlockMove(). Should have done this years and years ago.
         <9>   8–16–2003    pc      Updated for Windows (sort of, there is not yet very clean).
         <8>   30–3–2003    pc      Add new Atom accessor abstractions. Begin migrating to new names
                                    (AtomGet..., AtomSet..., etc.) for these. Also add code for
                                    getting an external's Creation/Mod dates from the file system.
         <7>   15–2–2003    pc      Update to avoid void* where possible
         <6>  30–12–2002    pc      Use 'STR#' resource instead of faux 'Vers' resource for storing
                                    version information used at run-time.
         <5>  26–12–2002    pc      Use macro MAC_VERSION (only #defined starting with OS X) to
                                    determine which veresion of PrevResFile() to use.
         <4>  17–12–2002    pc      Conditional compile PrevResFile() to handle different
                                    Carbon/Classic calls.
         <2>   4–12–2002    pc      Tidy up initial check-in.
									Carbonize PrevResFile().
         <1>   4–12–2002    pc     Initial Check-in.
		10-Dec-2000:	First implementation (mostly pulled from imLib.c)
*/

/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "MaxUtils.h"

#ifndef __NEED_JITTER_HEADERS__
	// Jitter has its own version of <string.h> prototypes which are sufficiently
	// incompatible with MSL to generate compile errors
	#include <string.h>
#endif

/*****************************  I M P L E M E N T A T I O N  ******************************/
#pragma mark -
#pragma mark • Micro StdLib
#if __MICRO_STDLIB__

/******************************************************************************************
 *
 *	strlen(iStr)
 *	strcmp(iStr1, iStr2)
 *	strncmp(iStr1, iStr2, n)
 *	strchr(iStr, iSearchChar)
 *	
 *	We now prefer to use the Standard C libraries and no longer use these.
 *
 *	To avoid adding standard C libraries, we appropriate from standard PPC implementations
 *	here.
 *	
 *	In the process of theft, it was noticeable that our sources were using unsigned char
 *	where old-time C code would have used the plain-vanilla char data type. Since this
 *	is Mac-only code, we make use of the Toolbox types Byte and BytePtr.
 *	
 ******************************************************************************************/

unsigned long
strlen(
	const char*	iStr)
	
	{
	unsigned long	len = -1;
	BytePtr			p = (BytePtr) iStr - 1;
	
	do { len++; }
		while (*++p != '\0');
	
	return len;
	}

int
strcmp(
	const char* iStr1,
	const char* iStr2)
	
	{
	BytePtr			p1 = (BytePtr) iStr1 - 1,
					p2 = (BytePtr) iStr2 - 1;
	unsigned long	c1, c2;
		
	while ((c1 = *++p1) == (c2 = *++p2)) {
		if (c1 == 0) return 0;
		}

	return c1 - c2;
	}

int
strncmp(
	const char*		iStr1,
	const char*		iStr2,
	unsigned long	n)

	{
	BytePtr			p1 = (BytePtr) iStr1 - 1,
					p2 = (BytePtr) iStr2 - 1;
	unsigned long	c1, c2;
	
	n++;
	while (--n > 0) {
		if ((c1 = *++p1) != (c2 = *++p2))
			return c1 - c2;
		else if (c1 == 0)
			break;
		}

	return 0;
	}

char*
strchr(
	const char* iStr,
	int			iSearchChar)
	
	{
	BytePtr			p = (BytePtr) iStr - 1;
	unsigned long   c = (iSearchChar & 0xff);
	unsigned long   ch;
	
	while((ch = *++p) != NULL) {
		if (ch == c) return (char*) p;
		}
	
	return c ? NULL : (char*) p;
	}
#endif // defined(__MICRO_STDLIB__) && __MICRO_STDLIB__


#pragma mark -
#pragma mark • Pascal String Utilities

/******************************************************************************************
 *
 *	MoveC2PStr(iCStr, oPStr)
 *	MoveP2CStr(iPStr, oCStr)
 *
 *	Utilities for moving data from C-style strings (NULL-delimited, favored by Max) to
 *	Pascal-style strings (length-byte prefixed, favored by Mac OS). And back.
 *
 ******************************************************************************************/

void
MoveC2PStr(
	const char*	iCStr,
	Str255		oPStr)
	
	{
	unsigned long	len = strlen(iCStr);
	
	if (len >= sizeof(Str255))
		len = sizeof(Str255) - 1;
		
	BlockMoveData(iCStr, oPStr + 1, (Size) len);
	oPStr[0] = len;	// Do this after BlockMoveData(), that allows input and output
					// parameters to overlap!
	}

void
MoveP2CStr(
	ConstStr255Param	iPStr,
	char*				oCStr)		// Caller is responsible for making sure this buffer
									// is large enough fo hold a copy of iPStr. PC prefers
									// to delcare the C-String as char[sizeof(Str255)]
	
	{
	long 	len = iPStr[0];
	
	BlockMoveData(iPStr + 1, oCStr, len);
	oCStr[len] = '\0';
	}
	

#pragma mark -
#pragma mark • Bit fiddling

/********************************************************************************
 *
 *	ReverseBits (n)
 *	
 *	Returns the input value with bits reversed.
 *	
 ********************************************************************************/

UInt32
ReverseBits(
	UInt32 iBits)
	
	{
    iBits = ((iBits >>  1) & 0x55555555) | ((iBits <<  1) & 0xaaaaaaaa);
    iBits = ((iBits >>  2) & 0x33333333) | ((iBits <<  2) & 0xcccccccc);
    iBits = ((iBits >>  4) & 0x0f0f0f0f) | ((iBits <<  4) & 0xf0f0f0f0);
    iBits = ((iBits >>  8) & 0x00ff00ff) | ((iBits <<  8) & 0xff00ff00);
    iBits = ((iBits >> 16) & 0x0000ffff) | ((iBits << 16) & 0xffff0000);
    
    return iBits;
	}
	
/******************************************************************************************
 *
 *	CountBits(iBits)
 *	
 *	Count set bits in iBits.
 *
 ******************************************************************************************/

long
CountBits(
	UInt32	iBits)
	
	{
	static const Byte bitCounts[] = {
					0, 1, 1, 2, 1, 2, 2, 3,		1, 2, 2, 3, 2, 3, 3, 4,		// 0x00 - 0x0f
					1, 2, 2, 3, 2, 3, 3, 4,		2, 3, 3, 4, 3, 4, 4, 5,		// 0x10 - 0x1f
					1, 2, 2, 3, 2, 3, 3, 4,		2, 3, 3, 4, 3, 4, 4, 5,		// 0x20 - 0x2f
					2, 3, 3, 4, 3, 4, 4, 5,		3, 4, 4, 5, 4, 5, 5, 6,		// 0x30 - 0x3f
					1, 2, 2, 3, 2, 3, 3, 4,		2, 3, 3, 4, 3, 4, 4, 5,		// 0x40 - 0x4f
					2, 3, 3, 4, 3, 4, 4, 5,		3, 4, 4, 5, 4, 5, 5, 6,		// 0x50 - 0x5f
					2, 3, 3, 4, 3, 4, 4, 5,		3, 4, 4, 5, 4, 5, 5, 6,		// 0x60 - 0x6f
					3, 4, 4, 5, 4, 5, 5, 6,		4, 5, 5, 6, 5, 6, 6, 7,		// 0x70 - 0x7f
					
					1, 2, 2, 3, 2, 3, 3, 4,		2, 3, 3, 4, 3, 4, 4, 5,		// 0x80 - 0x8f
					2, 3, 3, 4, 3, 4, 4, 5,		3, 4, 4, 5, 4, 5, 5, 6,		// 0x90 - 0x9f
					2, 3, 3, 4, 3, 4, 4, 5,		3, 4, 4, 5, 4, 5, 5, 6,		// 0xa0 - 0xaf
					3, 4, 4, 5, 4, 5, 5, 6,		4, 5, 5, 6, 5, 6, 6, 7,		// 0xb0 - 0xbf
					2, 3, 3, 4, 3, 4, 4, 5,		3, 4, 4, 5, 4, 5, 5, 6,		// 0xc0 - 0xcf
					3, 4, 4, 5, 4, 5, 5, 6,		4, 5, 5, 6, 5, 6, 6, 7,		// 0xd0 - 0xdf
					3, 4, 4, 5, 4, 5, 5, 6,		4, 5, 5, 6, 5, 6, 6, 7,		// 0xe0 - 0xef
					4, 5, 5, 6, 5, 6, 6, 7,		5, 6, 6, 7, 6, 7, 7, 8		// 0xf0 - 0xff
					};
	
	return		bitCounts[iBits & 0xff]
			+	bitCounts[(iBits >> 8) & 0xff]
			+	bitCounts[(iBits >> 16) & 0xff]
			+	bitCounts[(iBits >> 24) & 0xff];
	
	}
	

#pragma mark -
#pragma mark • Keyboard Utilities

/********************************************************************************
 *
 *	SetControlItemValue (iDlog, iItemNr, iVal)
 *	
 *	Sets the  control item iItemNr in iDlog to the given value.
 *	
 ********************************************************************************/

	static inline Boolean KeyMapTest(KeyMap iKeyMap, unsigned short iCode)
		{ char* km = (char*) iKeyMap; return ( ( km[iCode>>3] >> (iCode & 7) ) & 1); }

Boolean
KeyIsPressed(unsigned short iCode)
	{ KeyMap keys; GetKeys(keys); return KeyMapTest(keys, iCode); }

short
KeyMapToMods(void)
	{
										
	unsigned short	result = 0;				// Use unsigned during processing.
	KeyMap			keys;
	
	GetKeys(keys);
	if ( KeyMapTest(keys, kcCmd) )			result |= cmdKey;
	if ( KeyMapTest(keys, kcShift) )		result |= shiftKey;
	if ( KeyMapTest(keys, kcAlphaLock) )	result |= alphaLock;
	if ( KeyMapTest(keys, kcOption) )		result |= optionKey;
	if ( KeyMapTest(keys, kcControl) )		result |= controlKey;
	
	return result;
	}




#pragma mark -
#pragma mark • Dialog Utilities

/********************************************************************************
 *
 *	SetControlItemValue (iDlog, iItemNr, iVal)
 *	
 *	Sets the  control item iItemNr in iDlog to the given value.
 *	
 ********************************************************************************/

void
SetControlItemValue(
	DialogPtr	iDlog,
	short		iItemNr,
	short		iVal)
	
	{
	short	itemType;
	Handle	itemData;
	Rect	itemRect;	
	
	GetDialogItem(iDlog, iItemNr, &itemType, &itemData, &itemRect);
	SetControlValue((ControlHandle) itemData, iVal);
	
	}


/********************************************************************************
 *
 *	GetControlItemValue (iDlog, iItemNr)
 *	
 *	Returns the  value of the given control item in iDlog.
 *	
 ********************************************************************************/

short 
GetControlItemValue(
	DialogPtr	iDlog,
	short		iItemNr)
	
	{
	short	itemType;
	Handle	itemData;
	Rect	itemRect;	
	
	GetDialogItem(iDlog, iItemNr, &itemType, &itemData, &itemRect);
	
	return GetControlValue((ControlHandle) itemData);
	}


/********************************************************************************
 *
 *	HiliteControlItem (iDlog, iItemNr, iPartCode)
 *	
 *	Hilites a control item (referenced by the parameter iItemNr) in a dialog
 *	(referenced by the parameter iDlog).
 *	
 ********************************************************************************/

void HiliteControlItem(
		DialogPtr	iDlog,
		short		iItemNr,
		short		iPartCode)
	
	{
	short	itemType;
	Handle	itemData;
	Rect	itemRect;	
	
	GetDialogItem(iDlog, iItemNr, &itemType, &itemData, &itemRect);
	HiliteControl((ControlHandle) itemData, iPartCode);
	
	}


/********************************************************************************
 *
 *	SetTextItemStr (iDlogPtr, iItemNr, iStr)
 *	
 *	
 ********************************************************************************/

void SetTextItemStr(
	DialogPtr			iDlogPtr,
	short				iItemNr,
	ConstStr255Param	iStr)
	
	{
	short	itemType;
	Handle	itemData;
	Rect	itemRect;	
	
	GetDialogItem(iDlogPtr, iItemNr, &itemType, &itemData, &itemRect);
	SetDialogItemText(itemData, iStr);
	
	}


/********************************************************************************
 *
 *	GetTextItemStr (iDlogPtr, iItemNr, iStr)
 *	
 *	
 ********************************************************************************/

void GetTextItemStr(
	DialogPtr	iDlogPtr,
	short		iItemNr,
	Str255		iStr)
	
	{
	short	itemType;
	Handle	itemData;
	Rect	itemRect;	
	
	GetDialogItem(iDlogPtr, iItemNr, &itemType, &itemData, &itemRect);
	GetDialogItemText(itemData, iStr);
	
	}


#pragma mark -
#pragma mark • Resource Manager Utilities

/******************************************************************************************
 *
 *	PrevResFile()
 *
 *	Returns the refNum of the resource file one higher up the resource chain than the
 *	current resource file (as returned by the CurResFile).
 *	
 *	A return value of -1 indicates an error
 *	
 ******************************************************************************************/

#ifdef MAC_VERSION 
//#if defined(MAC_VERSION) || defined(WIN_VERSION)

short 
PrevResFile(void)							// Using Carbon calls (not available with
											// CarbonLib under Classic
	{
	short	curRefNum	= CurResFile(),
			walkRefNum,
			nextRefNum;
	OSErr	myError;
	
	myError = GetTopResourceFile(&walkRefNum);
	
	while (myError == noErr) {
		myError = GetNextResourceFile(walkRefNum, &nextRefNum);
		
		if (walkRefNum == curRefNum) break;
		else walkRefNum = nextRefNum;
		}
	
	return (myError == noErr) ? nextRefNum : -1;
	}

#else
 											// Use non-carbon calls (direct access)
typedef struct {
	long	resHeader[4];
	Handle	nextMap;
	short	refNum;
	// There's more, but we don't use it
	} tResMap;
typedef tResMap** tResMapHdl;

	
short 
PrevResFile(void)

	{
	tResMapHdl	thisMap		= (tResMapHdl) LMGetTopMapHndl();
	short		curRefNum	= CurResFile();
	
	while (thisMap != NIL && *thisMap != NIL) {
		tResMapHdl	nextMap = (tResMapHdl) (*thisMap)->nextMap;
		short		refNum	= (*thisMap)->refNum;
		thisMap = nextMap;
		if (refNum == curRefNum)
			break;
		}
	
	return (thisMap != NIL && *thisMap != NIL) ? (*thisMap)->refNum : -1;
	}

#endif									// __CARBON__
	
/******************************************************************************************
 *
 *	FindResID(iResType, iResName)
 *
 *	Returns the ID for a resource specified by resource type and name. 0 (almost always)
 *	indicates that the resource does not exist; check ResError() to be sure.
 *
 *	The iResName parameter is expected to arrive as a C-Style string (since Max uses this
 *	convention throughout) and is converted to a Pascal string locally
 *
 ******************************************************************************************/

short
FindResID(
	OSType		iResType,
	const char*	iResName)
	
	{
	Str255	resName;
	Handle	resHandle;
	short	resID;
	OSType	resType;		// Required for _GetResInfo
	
	MoveC2PStr(iResName, resName);
	
	resHandle = GetNamedResource(iResType, resName);
	if (resHandle) {
		GetResInfo(resHandle, &resID, &resType, resName);
		}
	else resID = 0;
	
	return resID;
	}


/******************************************************************************************
 *
 *	GetVersStrings(iVersID, oVersNum, oVersText)
 *
 ******************************************************************************************/

void
GetVersStrings(
	short	iVersID,
	char	oVersNum[],
	char	oVersText[])
	
	{
	VersRecHndl	versHdl = NIL;
	StringPtr	versCharsPtr;
	
	if (iVersID > 0)
		versHdl	= (VersRecHndl) GetResource('Vers', iVersID);
//		versHdl	= (VersRecHndl) GetResource('vers', iVersID);
		
	if (versHdl != NIL && *versHdl != NIL) {
		// Dereferencing a handle; copy data but don't move memory.
		versCharsPtr = (*versHdl)->shortVersion;
		if (oVersNum != NIL) {
			MoveP2CStr(versCharsPtr, oVersNum);
			}
		if (oVersText != NIL) {
			versCharsPtr += versCharsPtr[0] + 1;
			MoveP2CStr(versCharsPtr, oVersText);
			}
		}
	
	else {
		if (oVersNum != NIL)	oVersNum[0] = '\0';
		if (oVersText != NIL)	oVersText[0] = '\0';
		}
	
	}

#pragma mark -
#pragma mark • Max-Specific Utilities

/******************************************************************************************
 *
 *	DateCString(oString, iDate, iStyle)
 *
 *	Originally a wrapper around the classic Mac API DateString() function followed by a
 *	Pascal-to-C String conversion, now supports different APIs
 *	
 ******************************************************************************************/

void
DateCString(
	char	oString[],
	UInt32	iDate,
	short	iStyle)
	
#ifdef __LITTER_MAX5__
	{
	t_datetime	dt;
	long		dFlags;		// Time flags parameter
	
	switch(iStyle) {
		default /*case shortDate*/:	dFlags = SYSDATEFORMAT_FLAGS_MEDIUM;	break;
		case longDate:				dFlags = SYSDATEFORMAT_FLAGS_LONG;		break;
		case abbrevDate:			dFlags = SYSDATEFORMAT_FLAGS_SHORT;		break;
		}
	
	systime_secondstodate(iDate, &dt);
	sysdateformat_formatdatetime(&dt, dFlags, 0, oString, kMaxResourceStrSize);
	
	}
#else
	{
	Str255 ds;
	
	DateString(iDate, iStyle, ds, NIL);
	MoveP2CStr(ds, oString);
	
	}
#endif



/******************************************************************************************
 *
 *	TweakAlert(iDeltaWidth, iDeltaHeight, iAutoPos)
 *
 *	Because the standard ouchstring() ALRT is sometimes too darn small.
 *	
 ******************************************************************************************/

	// This has been tested and seems to work. I'm not sure if using it is a good idea.
	// Partly because the changed size stays changed...
void
TweakOuchstringAlert(
		short	iDeltaWidth,
		short	iDeltaHeight,
		short	iAutoPos)
	
	{
	enum {
		offsetOKRect		=  6,
		offsetOKKey			= 14,
		offsetTextRect		= 22,
		offsetTextKey		= 30,
		
		keyValAndPStrOK		=  '\x4\x2OK',		// Button, enabled followed by "\pOK"
		keyValAndPStrText	=  '\x8\x2^0'		// Static text, enabled (!), followed by "\p^1"
		};
	const short kOuchAlertID	= 1011;
	const long	kOldAlrtSize	= 12,
				kNewAlrtSize	= 14,
				kOuchDitlSize	= 34;
	
	Handle	alertHdl	= GetResource('ALRT', kOuchAlertID),
			ditlHdl		= GetResource('DITL', kOuchAlertID);
	Rect*	rectPtr;					// Used for a variety of rectangles 
	
	// Avoid any DS-kinds of situations
	if (alertHdl == NIL || *alertHdl == NIL)	return;
	if (ditlHdl == NIL || *ditlHdl == NIL)		return;
	
	// Make sure that Max's DITL looks as we expect it to
	if (GetHandleSize(ditlHdl) != kOuchDitlSize
			|| *((long*) (*ditlHdl + offsetOKKey)) != keyValAndPStrOK
			|| *((long*) (*ditlHdl + offsetTextKey)) != keyValAndPStrText)
		return;
	
	// Change size of Alert rectangle. Caller is responsible for reasonable values
	rectPtr = (Rect*) *alertHdl;		// NB: boundsRect is first field of Alert Template
	rectPtr->right += iDeltaWidth;
	rectPtr->bottom += iDeltaHeight;
	
	// Pass on changes to Dialog Items
#ifdef WIN_VERSION
	MacOffsetRect((Rect*) (*ditlHdl + offsetOKRect), iDeltaWidth, iDeltaHeight);
#else
	OffsetRect((Rect*) (*ditlHdl + offsetOKRect), iDeltaWidth, iDeltaHeight);
#endif
	rectPtr = (Rect*) (*ditlHdl + offsetTextRect);
	rectPtr->right += iDeltaWidth;
	rectPtr->bottom += iDeltaHeight;
	
	// Center the dialog?
	// DDZ isn't going to do this, even though it's dead easy in ResEdit.
	if (GetHandleSize(alertHdl) == kOldAlrtSize) {
		if (iAutoPos != 0) {
			SetHandleSize(alertHdl, kNewAlrtSize);
			if (MemError() == noErr) {
				((short*) (*alertHdl))[6] = iAutoPos;
				}
			}
		}
	else {
		// ASSERT: GetHandleSize(alertHdl) >= kNewAlrtSize
		((short*) (*alertHdl))[6] = iAutoPos;
		}
	}

/******************************************************************************************
 *
 *	SayHello(iName, iVersID, iPackID)
 *	SayHelloFromStrResource(iName, iResID, iObjectVersIndex, iPackageVersIndex)
 *
 *	Post() a basic copyright message to the Max window. The iName parameter is a pointer to
 *	a (NULL-terminated) string containing the name of the external. The next two parameters
 *	contain the IDs of 'vers' resources copied into Max Temp; these are, respectively,
 *	copies of the 'vers'(1) and 'vers'(2) resources from the external.
 *	
 *	The PostHello() variant gets its strings from an STR# resource rather than
 *	from a 'vers' (or 'Vers') resource. SayHello() is now deprecated.
 *	
 ******************************************************************************************/

void
SayHello(
	const char	iName[],
	short		iVersID,
	short		iPackID)
	
	{
	char	longVers1[sizeof(Str255)],
			longVers2[sizeof(Str255)];

	GetVersStrings(iVersID, NIL, longVers1);
	GetVersStrings(iPackID, NIL, longVers2);
	
	post("Loaded %s. Version %s %s.", iName, longVers1, longVers2);
	
	}

void
PostHello(
	const char	iName[],
	short		iResID,
	short		iObjectVersIndex,
	short		iPackageVersIndex)
	
	{
#ifdef WIN_VERSION
	#pragma unused(iResID, iObjectVersIndex, iPackageVersIndex)
	
	post("Loaded %s.", iName);
#else
	char	longVers1[sizeof(Str255)],
			longVers2[sizeof(Str255)];
	
	GetIndCString(longVers1, iResID, iObjectVersIndex);
	GetIndCString(longVers2, iResID, iPackageVersIndex);
	
	post("Loaded %s. Version %s %s.", iName, longVers1, longVers2);
#endif
	
	}


/******************************************************************************************
 *
 *	ParseAtom(iAtom, iTakeLong, iTakeFloat, iSymCount, iGoodSyms, iName)
 *	
 *	Parse atom, posts error messages, etc.
 *	
 *	Returns A_LONG or A_FLOAT if the atom was numeric (handling typecasting between the
 *	two) or A_SYM + index into the symbols passed as acceptable matches.
 *	
 *	Added 5 July 2001:	If iSymCount is negative, ignore the iGoodSyms vector and return
 *						A_SYM. The Atom is not modified.
 *	
 *	Added 2 March 2002: - NIL iName parameter suppresses error messages
 *						- Modified iSymCount condition to strictly negative(!!).
 *
 ******************************************************************************************/

short
ParseAtom(
	AtomPtr		iAtom,
	Boolean		iTakeLong,
	Boolean		iTakeFloat,
	short		iSymCount,
	SymbolPtr	iGoodSyms[],
	const char*	iName)
	
	{
	static const char* kNonComprendo = "doesn't understand";
	
	short	result = A_NOTHING;
	
	switch (iAtom->a_type) {
		case A_LONG:
			if (iTakeLong) {
				result = A_LONG;
				}
			else if (iTakeFloat) {
				AtomSetFloat(iAtom, iAtom->a_w.w_long);
				result = A_FLOAT;
				}
			else if (iName) {
				error("%s $s %ld", iName, kNonComprendo, iAtom->a_w.w_long);
				}
			break;
		
		case A_FLOAT:
			if (iTakeFloat) {
				result = A_FLOAT;
				}
			else if (iTakeLong) {
				AtomSetLong(iAtom, iAtom->a_w.w_float);
				result = A_LONG;
				}
			else if (iName) {
				error("%s %s %lf", iName, kNonComprendo, (double) iAtom->a_w.w_float);
				}
			break;
		
		default:
			// ASSERT: a_type == A_SYM
			if (iSymCount >= 0) {
				for ( ; result < iSymCount; result += 1) {
					if (iGoodSyms[result] == iAtom->a_w.w_sym) break;
					}
				if (result < iSymCount)
					result += A_SYM;
				else if (iName) {
					error("%s %s %s", iName, kNonComprendo, iAtom->a_w.w_sym->s_name);
					result = A_NOTHING;
					}
				}
			else {
				// Negative iSymCount indicates all symbols accepted
				result = A_SYM;
				}
			break;
		
		}
	
	return result;
	}


/******************************************************************************************
 *
 *	AtomToString(iAtom, oString)
 *	AtomDraw(iAtom)
 *	
 ******************************************************************************************/

#ifdef __LITTER_MAX5__
void
AtomToString(
	AtomPtr	iAtom,
	char	oBuf[])
	
	{
	
	switch (iAtom->a_type) {
	case A_LONG:
		sprintf(oBuf, "%ld", iAtom->a_w.w_long);
		break;
	
	case A_FLOAT:
		sprintf(oBuf, "%lf", (double) iAtom->a_w.w_float);
		break;
	
	case A_SYM:
		strcpy(oBuf, iAtom->a_w.w_sym->s_name);
		break;
	
	case A_SEMI:
		strcpy(oBuf, ";");
		break;
	
	case A_COMMA:
		strcpy(oBuf, ",");
		break;
	
	case A_DOLLAR:
		strcpy(oBuf, "$");
		break;
	
	default:
		// Must be A_NOTHING or something else we don't draw
		oBuf[0] = '\0';
		break;
		}
	
	}

#else
void
AtomToString(
	AtomPtr		iAtom,
	StringPtr	oString)					// Typically a Str255
	
	{
	
	switch (iAtom->a_type) {
	case A_LONG:
		NumToString(iAtom->a_w.w_long, oString);
		break;
	
	case A_FLOAT:
		// I would prefer to use Int'l Utilities for converting doubles to
		// strings, but almost all Max objects are completely C-biased, so
		// localized float representations would probably confuse many people.
		// Perhaps this could be made a user option ??
		oString[0] = sprintf((char*) oString + 1, "%f", iAtom->a_w.w_float);
			// ASSERT: sprintf returns a value in the range [0..255]
		break;
	
	case A_SYM:
		MoveC2PStr(iAtom->a_w.w_sym->s_name, oString);
		break;
	
	case A_SEMI:
		oString[0] = 1;
		oString[1] = ';';
		break;
	
	case A_COMMA:
		oString[0] = 1;
		oString[1] = ',';
		break;
	
	case A_DOLLAR:
		oString[0] = 1;
		oString[1] = '$';
		break;
	
	default:
		// Must be A_NOTHING or something else we don't draw
		oString[0] = '\0';
		break;
		}
	
	
	}

void
AtomDraw(
	AtomPtr iAtom)
	
	{
	Str255 aString;
	
	AtomToString(iAtom, aString);
	DrawString(aString);
	
	}
#endif

/******************************************************************************************
 *
 *	ForwardBang(iRecip)
 *	ForwardLong(iRecip, iVal)
 *	ForwardFloat(iRecip, iVal)
 *	ForwardList(iRecip, iArgC, iArgV)
 *	ForwardAnything(iRecip, iMsg, iArgC, iArgV)
 *	
 ******************************************************************************************/

void
ForwardBang(
	Symbol*	iRecip)
	
	{
	static Symbol* sSymBang = NIL;
	
	if (sSymBang == NIL)
		sSymBang = gensym("bang");
	
	ForwardAnything(iRecip, sSymBang, 0, NIL);
	
	}
	
	
void
ForwardLong(
	Symbol*	iRecip,
	long	iVal)
	
	{
	static Symbol* sSymInt = NIL;
	
	Atom intAtom;
	
	if (sSymInt == NIL)
		sSymInt = gensym("int");
	
	AtomSetLong(&intAtom, iVal);
	
	ForwardAnything(iRecip, sSymInt, 1, &intAtom);
	
	}
	
	
void
ForwardFloat(
	Symbol*	iRecip,
	double	iVal)

	{
	static Symbol* sSymFloat = NIL;
	
	Atom floatAtom;
	
	if (sSymFloat == NIL)
		sSymFloat = gensym("float");
	
	AtomSetFloat(&floatAtom, iVal);
	
	ForwardAnything(iRecip, sSymFloat, 1, &floatAtom);
	
	}
	

void
ForwardList(
	Symbol*	iRecip,
	short	iArgC,
	Atom	iArgV[])

	{
	static Symbol* sSymList = NIL;
	
	if (sSymList == NIL)
		sSymList = gensym("list");
	
	ForwardAnything(iRecip, sSymList, iArgC, iArgV);
	
	}
	


void
ForwardAnything(
	Symbol*	iRecip,
	Symbol*	iMsg,
	short	iArgC,
	Atom	iArgV[])
	
	{
	static Symbol* sSymThrough = NIL;
	
	Object* thing = (Object*) iRecip->s_thing;
	
	// Initialize our static symbol first time through
	if (sSymThrough == NIL)
		sSymThrough = gensym("through");
	
	// Make sure we have a vaild thing
	if (thing == NIL)							return;		// no good
	if (thing->o_magic != MAGIC)				return;		// still no good
	if (ob_class(thing)->c_sym != sSymThrough)	return;		// last check
		
	// If we make it to here, we're good to go
	typedmess(thing, iMsg, iArgC, iArgV);
		
	}

#pragma mark -
#pragma mark • Cross-Platform Utilities

/******************************************************************************************
 *
 *	XObjGetModDate(iRefNum)
 *	
 *	All an external knows at main() time is its File Reference Number (returned by
 *	CurResFile()... Max guarantees that the external is the current resource at this time.)
 *	
 *	This is enough to grab the modification date, which is a useful bit of information that
 *	one can give the user.
 *
 *	The Mac version talks directly to the Mac OS API because that used to be the only way
 *	to get this information (some code appropriated from the MoreFiles library from Mac
 *	Developer Tech Support). The Windows version can only run on Max 4.3 or later and makes
 *	use of Max API calls added between Max 4.1 and 4.3.
 *	
 ******************************************************************************************/

//#ifdef WIN_VERSION
#if TRUE

UInt32
XObjGetModDate(
	const char* iClassName)
	
	{
	char			fName[256];
	short			err,
					fPath;
	long			fType;
	unsigned long	modDate = 0;
			
	strcpy(fName, iClassName);
	strcat(fName, ".mxe");
	err = locatefile_extended(fName, &fPath, &fType, NULL, 0);
	
	if (err == noErr)
		path_getmoddate(fPath, &modDate);
	return modDate;
	}


#else

	static OSErr FileGetLocation(
		short		iRefNum,
		short*		oVRefNum,
		long*		oParDirID,
		StringPtr	oFileName)

		{
		FCBPBRec	pb;
		OSErr		errCode = noErr;

		pb.ioNamePtr	= oFileName;
		pb.ioVRefNum	= 0;
		pb.ioRefNum		= iRefNum;
		pb.ioFCBIndx	= 0;
		
		errCode = PBGetFCBInfoSync(&pb);
		if (errCode != noErr)
			goto punt;
			
		*oVRefNum	= pb.ioFCBVRefNum;
		*oParDirID	= pb.ioFCBParID;
		
	punt:	
		return errCode;
		}

	static OSErr FileGetDates(
		short				iVRefNum,
		long				iDirID,
		ConstStr255Param	iFileName,
		long*				oCrDate,
		long*				oModDate)
		
		{
		HParamBlockRec	pb;
		OSErr			errCode;
		
		pb.fileParam.ioVRefNum		= iVRefNum;
		pb.fileParam.ioFVersNum		= 0;
		pb.fileParam.ioFDirIndex	= 0;
		pb.fileParam.ioNamePtr		= (StringPtr) iFileName;
		pb.fileParam.ioDirID		= iDirID;
		
		errCode = PBHGetFInfoSync(&pb);
		if (errCode != noErr)
			goto punt;
		
		*oCrDate	= pb.fileParam.ioFlCrDat;
		*oModDate	= pb.fileParam.ioFlMdDat;
		
	punt:	
		return errCode;
		}



unsigned long
XObjGetModDate(
	short	iRefNum)	// File Reference Number returned by CurResFile() in main
	
	{
	short	errCode	= noErr,
			vRefNum;
	long	parDirID,
			crDate,
			modDate;
	Str255	fileName;
	
	errCode = FileGetLocation(iRefNum, &vRefNum, &parDirID, fileName);
	if (errCode != noErr)
		goto exit;			// Cheesy exception handling

	errCode = FileGetDates(vRefNum, parDirID, fileName, &crDate, &modDate);
							// Even cheesier exception handling
	
exit:
	return (errCode == noErr) ? modDate : 0;
	}

#endif


#ifdef WIN_VERSION
/******************************************************************************************
 *
 *	SecondsFromMacEpoch(iFileTime)
 *
 *	Windows-only. Converts number of 100-nanosecond units from 1 Jan 1601 to the number
 *	of seconds from 1 Jan 1901. The latter (Mac OS-based convention) is used throughout
 *	Litter Power
 *	
 ******************************************************************************************/

#pragma longlong on

UInt32
SecondsFromMacEpoch(
	const FILETIME* iFileTime)
	
	{
	const SYSTEMTIME kMacEpoch = {1901, 1, 2, 1, 0, 0, 0, 0};
	
	FILETIME		epoch;
	ULARGE_INTEGER	x, y;
	
	x.LowPart	= iFileTime->dwLowDateTime;
	x.HighPart	= iFileTime->dwHighDateTime;
	
	SystemTimeToFileTime(&kMacEpoch, &epoch);
	y.LowPart	= epoch.dwLowDateTime;
	y.HighPart	= epoch.dwHighDateTime;
	
	x.QuadPart -= y.QuadPart;					// Offset from Mac Epoch
	x.QuadPart /= 100000000;					// Convert decimicroseconds to seconds
	
	return x.LowPart;							// ASSERT: x.HighPart == 0
	}

#pragma longlong reset

#endif			// WIN_VERSION
