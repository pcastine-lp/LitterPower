/*
	File:		druthers.c

	Contains:	Track basic statistics on incoming data: count, minimum, maximum, mean,
				standard deviation, skew, and kurtosis.

	Written by:	Peter Castine

	Copyright:	© 2001-2002 Peter Castine

	Change History (most recent first):

         <7>   23–3–2006    pc      Use __MICRO_STDLIB__
         <6>    6–3–2005    pc      Drop legacy A4 #includes
         <5>     11–1–04    pc      Update for modified LitterInit()
         <4>      8–1–04    pc      Some updates for potential Windows compatibility (if we ever
                                    impelement this item).
         <3>  30–12–2002    pc      Use 'STR#' resource instead of faux 'Vers' resource for storing
                                    version information used at run-time.
         <2>  28–11–2002    pc      Tidy up after initial check in.
         <1>  28–11–2002    pc      Initial check in.
		2-Apr-2001:		First implementation.
*/


/******************************************************************************************
 ******************************************************************************************/

	// Use our own implementation of (a hyperminimalist) stdio
#define __MICRO_STDLIB__ 1

	// Set this to 0 for production code
#define __DEBUG__ 1

#if __DEBUG__
	#define DebugPost	post
#else
	static inline void DebugPost(char*, ...) { }
#endif

#pragma mark • Include Files

#include "LitterLib.h"


#pragma mark • Constants

const char	kClassName[]		= "lp.druthers";			// Class name


	// Indices for STR# resource
enum {
	strIndexTheInlet		= lpStrIndexLastStandard + 1
	};

	// License Stuff
const UInt32		kCurPrefVers	= 0x01000000,
					kVersEssence	= 0xff000000,
					kFileIDMask		= ~'4-15',
					kRegResType		= 'Pref';				// Look like just another pref
const SInt16		kRegResID		= 128;
const int			kKeyChainLen	= 4;					// "Key Chain", used for
															// Decoding/Encoding


#pragma mark • Type Definitions

typedef struct {
	long	vers,			// Identical in Resource and Memory
			license,		// In Resource XOR-ed against (decoded) File ID
			authCode1,		// 	Ditto
			authCode2,		// 	Ditto
			fileID,			// In Resource XOR-ed against ~'4-15' 
			bogus;			// Random number, ingored. To confuse the curious
	Str255	name,			// In Resource XOR-ed (sequentially) against 
			email;			//	fileID, authCode1, authCode1, and license. FileID is
							//	left-shifted once each time through sequence. Unused long
							//	words in the Str255s are filled with random numbers prior
							//	to coding. To confuse the curious.
	}	tRegInfo;
typedef tRegInfo*	tRegInfoPtr;
typedef tRegInfo**	tRegInfoHdl;

typedef UInt32		tKeyChain[kKeyChainLen];				// "Key Chain", used for
															// Decoding/Encoding


#pragma mark • Object Structure

typedef struct {
	Object			coreObject;
	} tDruthers;


#pragma mark • Global Variables

short		gDlogID			= 0;
eLitterLicense
			gRegLicense		= 0;
Str255		gName			= "\p",
			gEMail			= "\p";
Str15		gAuthCode		= "\p";

#pragma mark • Function Prototypes

	// Registration stuff
static void		GetLicense();

	// Class message functions
void*	NewDruthers();

	// Object message functions
static void DoReg(tDruthers*);
static void DoCategories(tDruthers*, long);
static void DoAssist(tDruthers*, void*, long, long, char*);

#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark • Inline Functions

static inline UInt32
RndLong(void)											// Quick'n'dirty but good enough.
	{ return Random() + ((UInt32) Random() << 16); }

#pragma mark -

/******************************************************************************************
 *
 *	main()
 *	
 *	Standard Max External Object Entry Point Function
 *	
 ******************************************************************************************/

void
main()
	
	{
	LITTER_CHECKTIMEOUT(kClassName);
	
	// Standard Max setup() call
	setup(	&gObjectClass,			// Pointer to our class definition
			(method) NewDruthers,			    
			NIL,						// No custom deallocation function
			sizeof(tDruthers),			// Class object size
			NIL,						// No menu function
			A_NOTHING);					// No arguments
	
	// Messages
	LITTER_TIMEBOMB addbang	((method) DoReg);
	LITTER_TIMEBOMB addmess	((method) DoReg,	"reg", A_NOTHING);
//	addint	((method) DoCategories);
//	addmess ((method) DoCategories,	"categories", A_LONG);

		// Currently not using LitterInit(), so explicitly set vers method
	addmess	((method) LitterVers, "vers",A_DEFLONG, A_DEFLONG, 0);
	
	// Get our resources...
	resnamecopy('STR#', (char*) kClassName);		// 'STR#' resource must be named "LitterPrefs"
	resnamecopy('DLOG', (char*) kClassName);
	resnamecopy('DITL', (char*) kClassName);
	
	// ... and store the IDs assigned to them in Max Temp
	
	// Check if we're licensed
	GetLicense();
	if (gRegLicense == 0)
			ouchstring("Please register your copy of the Litter Power Package");
	else	post("This copy of Litter Pro is registered to %s.", gName + 1);
	
	// Initialize Litter Library
	LitterInit(kClassName, 0);
	Taus88Init();
	
	}


#pragma mark -
#pragma mark • Registration

/******************************************************************************************
 *
 *	StringCodec(ioStr, ioKeys, iFill)
 *	EncodeRegRes(ioRes)
 *	DecodeRegRes(ioRes)
 *
 ******************************************************************************************/

static void
StringCodec(
	Str255		ioStr,
	tKeyChain	ioKeys,							// ioKeys must contain exactly 4 keys!!!
	Boolean 	iDecoding)
	
	{
	int		sigBytes	= iDecoding
								? (ioStr[0] ^ (ioKeys[3] >> 24)) + 1
								: ioStr[0] + 1,
			bytesLeft	= sigBytes,
			curKey		= 3;
	long*	curChunk	= (long*) ioStr;
			
	while (bytesLeft > 0) {
		*curChunk++		^= ioKeys[curKey];
		ioKeys[curKey]	^= ioKeys[curKey] >> (curKey + 1);
		
		if (--curKey < 0)
			curKey = 3;
		bytesLeft -= sizeof(long);
		}
	
	
	if (iDecoding) {
		// Clear all bytes not used by string
		while (++bytesLeft <= 0) {
			ioStr[sigBytes++] = '\0';			// sigBytes no longer means "significant"
			}									// but "how many bytes we've handled"
		
		bytesLeft = sizeof(Str255) - sigBytes;
		
		while (bytesLeft > 0) {
			*curChunk++ = 0;
			bytesLeft -= sizeof(long);
			}
		}
	
	else {
		// Fill remainder of string with noise.
		bytesLeft += sizeof(Str255) - sigBytes;
		while(bytesLeft > 0) {
			*curChunk++ = RndLong();
			bytesLeft -= sizeof(long);
			}
		
		}
	
	
	}

static void
EncodeRegRes(
	tRegInfoHdl	ioRes)
	
	{
	tRegInfoPtr	regInfoPtr = (tRegInfoPtr) *ioRes;		// Don't move memory!
	tKeyChain	keys;
	
	keys[0] = regInfoPtr->fileID;
	keys[1] = regInfoPtr->license;
	keys[2] = regInfoPtr->authCode1;
	keys[3] = regInfoPtr->authCode2;
	
	regInfoPtr->fileID		^= kFileIDMask;
	regInfoPtr->license		^= kFileIDMask << 2;
	regInfoPtr->authCode1	^= kFileIDMask << 3;
	regInfoPtr->authCode2	^= kFileIDMask << 5;
	
	StringCodec(regInfoPtr->name, keys, false);
	StringCodec(regInfoPtr->email, keys, false);
	
	}

static void
DecodeRegRes(
	tRegInfoHdl	ioRes)
	
	{
	tRegInfoPtr	regInfoPtr = (tRegInfoPtr) *ioRes;		// Don't move memory!
	tKeyChain	keys;
	
	regInfoPtr->fileID		^= kFileIDMask;
	regInfoPtr->license		^= kFileIDMask << 2;
	regInfoPtr->authCode1	^= kFileIDMask << 3;
	regInfoPtr->authCode2	^= kFileIDMask << 5;
	
	keys[0] = regInfoPtr->fileID;
	keys[1] = regInfoPtr->license;
	keys[2] = regInfoPtr->authCode1;
	keys[3] = regInfoPtr->authCode2;

	StringCodec(regInfoPtr->name, keys, true);
	StringCodec(regInfoPtr->email, keys, true);

	DebugPost("Decoded file id as %ld", regInfoPtr->fileID);
	DebugPost("Decoded license as %s", (regInfoPtr->license == llArtist)
									? "Artist"
									: (regInfoPtr->license == llDeveloper) ? "Developer" : "Unknown!");
	DebugPost("Decoded authorization codes as %lx, %lx", regInfoPtr->authCode1, regInfoPtr->authCode2);
	DebugPost("Decoded name as %s", regInfoPtr->name + 1);	
	DebugPost("Decoded email as %s", regInfoPtr->email + 1);
	}

static void
GenAuthCodes(
	ConstStr255Param	iName,
	ConstStr255Param	iEMail,
	eLitterLicense		iLic,
	UInt32*				oAuth1,
	UInt32*				oAuth2)
	
	{
	*oAuth1 = HashString(iName);
	*oAuth2 = HashString(iEMail);
	
	if (iLic == llArtist)
		*oAuth1 ^= llArtist;
	else if (iLic == llDeveloper)
		*oAuth1 ^= llDeveloper;
	else *oAuth1 = *oAuth2 = 0;
	DebugPost("Generated Authorization codes %lx, %lx", *oAuth1, *oAuth2);	
	}
	
	
/******************************************************************************************
 *
 *	GetLicense()
 *
 ******************************************************************************************/

static void
GetLicense(void)
	{
	short		prefsRefNum		= -1;
	SInt32		prefsFileID;
	UInt32		authCode1,
				authCode2;
	Handle		tempHdl;
	tRegInfoPtr	regInfoPtr;
	
	// Open (but don't create) the prefs file 
	prefsRefNum = OpenPrefsFile(fsRdPerm, false, &prefsFileID);
		if (prefsRefNum == -1) return;
	DebugPost("GetLicense: OpenPresFile succeeded.");

	// File opened successfully, get the Registration resource
	// The prefs resource file has just been opened, so it will be at the top of
	// the resource chain. 
	tempHdl = Get1Resource(kRegResType, kRegResID);
		if (tempHdl == NIL || *tempHdl == NIL) goto punt;
	DebugPost("GetLicense: Get1Resource succeeded.");
	
	DetachResource(tempHdl);
		if (ResError() != noErr) goto punt;
	DebugPost("GetLicense: DetachResource succeeded.");
		
	// Now check that the size is right and that versions match
	if (GetHandleSize(tempHdl) != sizeof(tRegInfo))
		goto punt;
	regInfoPtr = (tRegInfoPtr) *tempHdl;
	DebugPost("GetLicense: GetHandleSize succeeded.");
	if (regInfoPtr->vers != kCurPrefVers)
		goto punt;
	DebugPost("GetLicense: version number matched.");

	// Decode resource and check validity
	DecodeRegRes((tRegInfoHdl) tempHdl);
	DebugPost("GetLicense: DecodeRegRes succeeded.");
	
	if (regInfoPtr->fileID != prefsFileID)
		goto punt;
	
	// Check name and e-mail against registration codes
	GenAuthCodes(regInfoPtr->name, regInfoPtr->email, regInfoPtr->license,
				 &authCode1, &authCode2);
	if (authCode1 != regInfoPtr->authCode1 || authCode2 != regInfoPtr->authCode2)
		goto punt;
	
	//
	//	All done, prepare to clear out
	//
	gRegLicense = regInfoPtr->license;
	BlockMove(regInfoPtr->name, gName, regInfoPtr->name[0] + 1);
	BlockMove(regInfoPtr->email, gEMail, regInfoPtr->email[0] + 1);
	
punt:
	if (tempHdl != NIL) DisposeHandle(tempHdl);
	if (prefsRefNum > 0) CloseResFile(prefsRefNum);
	}


/******************************************************************************************
 *
 *	NewDruthers(iOracle)
 *
 ******************************************************************************************/

static void
Register(
	Str255	iName,
	Str255	iEMail,
	UInt32	iAuth1,
	UInt32	iAuth2,
	Boolean	iDevLic)
	
	{
	short		prefsRefNum;
	long		prefsFileID;
	Handle		regInfoHdl;
	tRegInfoPtr	regInfoPtr;
	
	// Copy registration data into static memory. Safe as tofu
	BlockMove(iName, gName, iName[0] + 1);
	BlockMove(iEMail, gEMail, iEMail[0] + 1);
//	!!! And auth code
	gRegLicense = iDevLic ? llDeveloper : llArtist;
	
	// Store encoded registration data into prefs file.
	// A gazillion things can go wrong.
	prefsRefNum = OpenPrefsFile(fsRdWrPerm, true, &prefsFileID);
		if (prefsRefNum < 0) goto punt;
	
	regInfoHdl = NewHandle(sizeof(tRegInfo));
		if (regInfoHdl == NIL || *regInfoHdl == NIL) goto punt;
	
	regInfoPtr = (tRegInfoPtr) *regInfoHdl;
	regInfoPtr->vers		= kCurPrefVers;
	regInfoPtr->fileID		= prefsFileID;
	regInfoPtr->license		= iDevLic ? llDeveloper : llArtist;
	regInfoPtr->authCode1	= iAuth1;
	regInfoPtr->authCode2	= iAuth2;
	regInfoPtr->bogus		= RndLong();
	
	BlockMove(iName, regInfoPtr->name, sizeof(Str255));
	BlockMove(iEMail, regInfoPtr->email, sizeof(Str255));
	
	EncodeRegRes((tRegInfoHdl) regInfoHdl);
	
	AddResource(regInfoHdl, kRegResType, kRegResID, "\p");
		if (ResError() != noErr) { DisposeHandle(regInfoHdl); goto punt; }
	
	ChangedResource(regInfoHdl);
		if (ResError() != noErr) goto punt;
	CloseResFile(prefsRefNum);
	
	// All done
	return;
	
	// Poor man's exception handling
punt:
	ouchstring("Unable to complete registration. Error code %d", (int) ResError());
	if (prefsRefNum > 0)
		CloseResFile(prefsRefNum);
	return;
	}
		
#pragma mark -
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	NewDruthers(iOracle)
 *
 ******************************************************************************************/

void*
NewDruthers()
	
	{
	tDruthers*	me	= NIL;
	
	me = (tDruthers*) newobject(gObjectClass);
	
	return me;
	}

#pragma mark -
#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	DoBang(me)
 *
 ******************************************************************************************/

	enum {
		itemOK		= 1,
		itemCancel,
		itemSTextTitle,
		itemSTextName,
		itemSTextEMail,
		itemSTextAuthCode,
		itemETextName,
		itemETextEMail,
		itemETextAuth,
		itemSTextCat,
		itemCBoxCatStd,
		itemCBoxCatLit,
		itemCBoxCatLitSpec
		};
	
	const Byte	FiveBit2Code[32] = "456789ABCDEFGH2JKLMN3PQRSTUVWXYZ";
	
	static OSErr DecodeAuthCode(ConstStr255Param iStr, UInt32* oPart1, UInt32* oPart2)
		{
		int		i;
		BytePtr	curChar = (BytePtr) iStr + 1,		// But don't change contents of iStr!
				codePos;
		UInt32	p1, p2;
		
		// Scan string to check for general syntactical correctness
		if (iStr[0] != 15)		return bdNamErr;
		if (iStr[7] != '-')		return bdNamErr;
		if (iStr[14] != '-')	return bdNamErr;
		
		// Parse first quintet
		p1 = 0;
		for (i = 0; i < 30; i += 5) {
			codePos = (BytePtr) strchr((char*) FiveBit2Code, *curChar++);
				if (codePos == NIL) return bdNamErr;
			p1 |= (codePos - FiveBit2Code) << i;
			}
		
		// Skip hyphen
		curChar += 1;
		
		// Parse second quintet
		p2 = 0;
		for (i = 0; i < 30; i += 5) {
			codePos = (BytePtr) strchr((char*) FiveBit2Code, *curChar++);
				if (codePos == NIL) return bdNamErr;
			p2 |= (codePos - FiveBit2Code) << i;
			}
		
		// Skip hyphen
		curChar += 1;
		
		// Parse final character
		codePos = (BytePtr) strchr((char*) FiveBit2Code, *curChar);
			if (codePos == NIL) return bdNamErr;
		p1 |= (codePos - FiveBit2Code) << 30;
		p2 |= ((codePos - FiveBit2Code) << 28) & 0xc0000000;
		
		// Return results
		*oPart1 = p1;
		*oPart2 = p2;
		return noErr;
		}
	
	static void EncodeAuthCode(UInt32 iPart1, UInt32 iPart2, Str255 oStr)
		{
		int		i;
		Byte*	curChar = oStr + 1;
		
		for (i = 6; i > 0; i -= 1) {
			*curChar++ = FiveBit2Code[iPart1 & 31];
			iPart1 >>= 5;
			}
		
		*curChar++ = '-';
		
		for (i = 6; i > 0; i -= 1) {
			*curChar++ = FiveBit2Code[iPart2 & 31];
			iPart2 >>= 5;
			}
		
		*curChar++ = '-';
		*curChar++ = FiveBit2Code[iPart1 + (iPart2 << 2)];
		
		oStr[0] = 15;
		}
	
	static Boolean CheckParamsAndReg(DialogPtr iDlog)
		{
		Str255	nameStr,
				emailStr,
				authCodeStr;
		UInt32	nameHash,
				emailHash,
				authCode1,
				authCode2;
		eLitterLicense
				lic			= (eLitterLicense) 0;		// Initially no license.
		
		GetTextItemStr(iDlog, itemETextName, nameStr);
		GetTextItemStr(iDlog, itemETextEMail, emailStr);
		GetTextItemStr(iDlog, itemETextAuth, authCodeStr);
		
		nameHash = HashString(nameStr);
		emailHash = HashString(emailStr);
		
		UpperString(authCodeStr, true);
		if (DecodeAuthCode(authCodeStr, &authCode1, &authCode2) == noErr) {
			
			// Check if the code included an artist license
			if (authCode1 == nameHash ^ llArtist && authCode2 == emailHash) {
				Register(nameStr, emailStr, authCode1, authCode2, false);
				return true;
				}
			
			// OK, see if it's a developer's license
			if (authCode1 == nameHash && authCode2 == emailHash ^ llDeveloper) {
				Register(nameStr, emailStr, authCode1, authCode2, true);
				return true;
				}
			
			}
		
		// ASSERT: lic == 0
		ouchstring("Invalid Authorization Code.");
#if __DEBUG__
		EncodeAuthCode(nameHash ^ llArtist, emailHash, authCodeStr);
		authCodeStr[authCodeStr[0]+1] = '\0';
		DebugPost("Suggest trying %s for Artist", authCodeStr + 1);
		EncodeAuthCode(nameHash, emailHash ^ llDeveloper, authCodeStr);
		authCodeStr[authCodeStr[0]+1] = '\0';
		DebugPost("Suggest trying %s for Developer", authCodeStr + 1);
#endif
		SelectDialogItemText(iDlog, itemETextAuth, 0, 32767);
		SysBeep(0);
		
		return false;
		}

void
DoReg(
	tDruthers*)
	
	{
	DialogPtr	dlogPtr;
	Boolean		dlogDone	= false,
				success		= false;
	
	dlogPtr = GetNewDialog(gDlogID, NIL, (WindowPtr) -1);
		if (dlogPtr == NULL) { error("lp.druthers: Can't open dialog!"); return; }
	
	if (gRegLicense == 0) {
		// Not registered yet; disable prefs items
		HiliteControlItem(dlogPtr, itemCBoxCatStd, 255);
		HiliteControlItem(dlogPtr, itemCBoxCatLit, 255);
		HiliteControlItem(dlogPtr, itemCBoxCatLitSpec, 255);
		}
	else {
		// Registered. Copy name, address, and reg code; 
		SetTextItemStr(dlogPtr, itemETextName, gName);
		SetTextItemStr(dlogPtr, itemETextEMail, gEMail);
		}
	
	do {
		short itemHit;
		
		ModalDialog(NIL, &itemHit);
		
		switch (itemHit) {
			case itemOK:
				if ( CheckParamsAndReg(dlogPtr) ) {
					success	= true;
					dlogDone = true;
					}
				break;
			
			case itemCancel:
				dlogDone = true;
				break;
			
			default:
				// Nothing to do
				break;
			}
		} while (!dlogDone);
	
	DisposeDialog(dlogPtr);
	
	if (success) {
		ouchstring("Thank you for purchasing Litter Power!");
		}
	
	}


/******************************************************************************************
 *
 *	DoPrefs(me, iVal)
 *	DoInt(me, iVal)
 *	DoFloat(me, iVal)
 *	DoList(me, iVal)
 *
 * 	DoPrefs() does the brute work; DoInt and DoFloat just typecast to double and call it.
 *	DoList iterates through the list and calls DoPrefs()
 *
 ******************************************************************************************/

static void
DoPrefs(
	tDruthers*,
	double)
	
	{
	
	
	}


/******************************************************************************************
 *
 *	DoAssist
 *
 *	Fairly generic Assist Method
 *
 ******************************************************************************************/

void
DoAssist(
	tDruthers*,
	void*,					// We don't use this parameter
	long		iDir,
	long		iArgNum,
	char*		oCStr)
	
	{
	
	LitterAssist(iDir, iArgNum, strIndexTheInlet, 0, oCStr);
	
	}


#pragma mark -
#pragma mark OS Helpers

/******************************************************************************************
 *
 *	SavePrefs(iPrefs)
 *
 *	Save the prefs structure in the prefs resource file
 *
 ******************************************************************************************/

/*static OSErr
SavePrefs(
	PrefsType*	iPrefs)

	{
	OSErr	errCode			= noErr;
	short	prefsResRefNum;
	Handle	prefHandle,
			alisHandle,
			finderMessageHandle;
	
	// Open (and, if necessary, create) the prefs file 
	prefsResRefNum = OpenPrefsResFile(fsRdWrPerm, true);
	if (prefsResRefNum == -1) {
		// Couldn't open the res file
		errCode = ResError();
		if (errCode == noErr)
			errCode = resFNotFound;
		goto punt;
		}
		
	//  File opened successfully, get the prefs resource 
	prefHandle = Get1Resource(kRegResType, kPrefsResourceID);
	
	if (prefHandle == NIL) {
		// Create a new resource 
		prefHandle = NewHandle(sizeof(PrefsType));
		if (prefHandle != NIL) {
			// Copy the prefs struct into the handle and make it into a resource 
			*(PrefsType*) *prefHandle = *iPrefs;
			AddResource(prefHandle, kRegResType, kPrefsResourceID, "\pPrefsType");
			errCode = ResError();
			if (errCode != noErr) DisposeHandle(prefHandle);
			} 
		else errCode = MemError(); //  NewHandle failed 
		}
	
	else {
		// prefHandle != NIL 
		// Update the existing resource 
		SetHandleSize(prefHandle, sizeof(PrefsType));
		errCode = MemError();
		if (errCode != noErr) goto punt;

		// Copy the prefs struct into the handle and tell the rsrc manager 
		*(PrefsType *)*prefHandle = *iPrefs;
		ChangedResource(prefHandle);
	
		}
	
	
	// Update and close the preference resource file, releasing its resources from memory
	CloseResFile(prefsResRefNum);

punt:
	return errCode;
	}
*/