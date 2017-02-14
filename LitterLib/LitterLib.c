/*
	File:		LitterLib.c

	Contains:	Library functions for use by the Litter Power Package.

	Written by:	Peter Castine

	Copyright:	© 2001-2002 Peter Castine

	Change History (most recent first):

         <4>   30Ð3Ð2006    pc      Fixed problems w/Windows crashing in the MSL.
         <3>     27Ð2Ð06    pc      Move constants (kPi, etc.) to main LitterLib files. The
                                    constants are used in lots of projects that don't need MoreMath.
         <2>     26Ð2Ð06    pc      Add expMin, expMax to enumerator Expect.
         <1>     26Ð2Ð06    pc      First checked in after moving file and splitting off RNGs,
                                    Utilities, etc. into separate files
*/


/******************************************************************************************
	Previous History:

        <21>     18Ð2Ð06    pc      Add general digamma() function (supporting all reals).
        <20>     16Ð2Ð06    pc      Preflight to the resnamecopy() call in LitterInit(): make sure
                                    resource is available. This is not the case in Collectives post
                                    Max-4.1, and leads to spurious error messages.
        <19>     15Ð2Ð06    pc      Need to implement lgamma() and gamma() functions because the
                                    braindead MSL/Windows doesn't provide them.  Also added
                                    Factorial(), LogFactorial() and two special cases of the digamma
                                    function.
        <18>     15Ð2Ð06    pc      Add LitterExpect(), support for an 'expect' message for all
                                    Litter RNGs.
        <17>     12Ð2Ð06    pc      Modify MachineKharma() to have the sAutoSeedCount part of the
                                    algorithm make a bigger impact on successive seeds.
        <16>     19Ð1Ð06    pc      Improve constant used in AddPepper()
        <15>     19Ð1Ð06    pc      Improve seeding algorith used for Taus88. Moved CountBits() to
                                    MaxUtils
        <14>     11Ð1Ð06    pc      Previous solution for LitterAssistVA() requires user to have
                                    MSL_All-DLL_x86.lib installed. We need to hack a different
                                    solution for Windows.
        <13>      9Ð1Ð06    pc      Tracked down bug in Windows with variable-argument versions of
                                    Assistance (really a error in the MSL_Allxxx Library linked in).
                                    Added LitterAssistResFrag() function.
        <12>     14Ð1Ð04    pc      Make constants kMaxResourceStrSize and kMaxResourceStrLen
                                    available to all objects.
        <11>     13Ð1Ð04    pc      Use Max API getbytes() & freebytes() for the Taus88 and TT800
                                    new/free routines. It seems to be the most reliable way to make
                                    the relevant code cross-platform compatible, and the amounts of
                                    memory required are within getbytes() capabilities.
        <10>     10Ð1Ð04    pc      Major updates to support cross-platform development. Added
                                    LitterGetVersVal() & Co. Freezing features for Litter 1.5 (bar
                                    beta bug fixes).
         <9>      4Ð1Ð04    pc      Add LitterVers() and LetterAssist(): default methods for
                                    handling vers and assist messages.
         <8>      1Ð1Ð04    pc      More updating for Windows-compatibility. Add __MAX_MSP_OBJECT__
                                    and __MAX_USER_INTERFACE_OBJECT__ macros to conditionally
                                    include/link/compile relevant parts of the Max API only when
                                    needed.
         <7>   8Ð16Ð2003    pc      Updated for Windows (sort of, there is not yet very clean).
         <6>    5Ð7Ð2003    pc      Revised strategy for seeding RNGs. Cf comments to Taus88Seed()
                                    and TT800Seed() for more details.
         <5>   30Ð3Ð2003    pc      Add function LitterAddClass() as a standardized "pick the right
                                    category for the current version of Max" tool.
         <4>  28Ð11Ð2002    pc      Tidy up after check-in
         <1>  28Ð11Ð2002    pc     Initial check in.
		
		4-Apr-2001:		TT800 now maintains it's own private pool of seeds, allowing users
						to call the TT800 functions with NULL for iData (in which case the
						private pool is used for generating numbers) or an object can
						allocate memory for its own pool and be independent of everything
						else that's going on. Wheeeee.
 ******************************************************************************************/

#pragma mark ¥ Include Files

#include "LitterLib.h"

#include <stdarg.h>				// We need variable-length arg lists for LitterAssistVA()
#include <stdio.h>				// ... it also needs vsprint()

#ifdef _WINDOWS_
	#include "Folders.h"		// For FindFolder(), used in OpenPrefsFile()
	#include <ShLwApi.h>
#endif


#pragma mark ¥ Constants

	// We occassionally need these
	// High precision values from Abramowitz & Stegun _Handbook of Mathematical Functions_
const double	kPi			= 3.141592653589793238462643,
				k2pi		= 6.283185307179586476925286,
				kHalfPi		= 1.570796326794896619231322,
				kQuarterPi	= 0.7853981633974483096154214,
				
				kNatLogBase	= 2.718281828459045235360287,			// e
				
				kFloatEpsilon		= 1.0842e-19,
				kULongMaxInv		= 1.0 / ((double) kULongMax);

#ifdef __COREFOUNDATION_CFBASE__

#define	LITTER_POST_MAX4	1

CFStringRef		kLPPackageVersValKey		= CFSTR("LPPackageVersVal"),
				kLPPackageShortVersionKey	= CFSTR("LPPackageShortVersionString"),
				kLPPackageLongVersionKey	= CFSTR("LPPackageLongVersionString"),
				kLPObjectShortVersionKey	= CFSTR("LPObjectShortVersionString"),
				kLPObjectLongVersionKey		= CFSTR("LPObjectLongVersionString"),
				kCFBundleGetInfoKey			= CFSTR("CFBundleGetInfoString");

#else

#define LITTER_POST_MAX4	0
				
#endif


#pragma mark ¥ Type Definitions

#if LITTER_USE_OBEX
	typedef t_class*		tMaxClassPtr;
#else
	typedef t_messlist*		tMaxClassPtr;
#endif



#pragma mark ¥ Static (Private) Variables



#pragma mark ¥ Initialize Global Variables

tMaxClassPtr		gObjectClass	= NIL;


#ifdef WIN_VERSION
	HINSTANCE		gDLLInstanceHdl = NIL;
	FILETIME		gModDate		= {0, 0};
	unsigned long	gObjVersVals[2]	= {0, 0};
#else
	short			gStrResID		= 0;
	unsigned long	gModDate		= 0;
	
	// Added for Max 5.0
	CFBundleRef	gLPObjBundle		= NULL;
	CFURLRef	gResRef				= NULL;
	tStrListPtr	gStringTable		= NULL;	// Used with Max 5 for storing the object's
											// STR# resource, since Max is no longer
											// supporting
#endif



#pragma mark ¥ Function Prototypes

#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark ¥ OS Utilities

/******************************************************************************************
 *
 *	OpenPrefsFile(iPermissions, oFileID)
 *	
 *	Open the preferences file with the given permission. Returns the "hard" file ID in
 *	oFileID (undefined if file could not be opened).
 *	
 ******************************************************************************************/

#if 0		// Currently unused

short
OpenPrefsFile(
	SignedByte	iPermissions,
	Boolean		iCreateFlag,
	long*		oFileID)

	{
	const long		kPrefsCreatorType	= 0x3F3F3F3F,		// The literal '????' generates
															// trigraph warnings from the
															// gcc preprocessor... 
					kPrefsFileType		= 'LPDr';
	const StringPtr	kPrefsFileName		= (StringPtr) "\pLitter Druthers";
	
	short		prefsRefNum		= -1,
				prefsVRefNum;
	long		prefsDirID;
	FSSpec		prefsFSSpec;
	OSErr		errCode;
	
	// Use FindFolder to locate the Preferences folder.
	// As long as this is all PPC only, we know that we are running on System 7.1.2 or
	// later, and FindFolder, Gestalt, etc. are available.
	// Should we add support for 68k machines, we will need to check if CodeWarrior provides 
	// the glue necessary to use FindFolder (and Gestalt) safely under System 6 and earlier.
	errCode	= FindFolder(kOnSystemDisk, kPreferencesFolderType, kCreateFolder,
						 &prefsVRefNum, &prefsDirID);
	if (errCode != noErr)
		goto punt;
	
	// Make a file spec for the prefs file, create if necessary and desired
	errCode = FSMakeFSSpec(prefsVRefNum, prefsDirID, kPrefsFileName, &prefsFSSpec);
	
	if (errCode == fnfErr && iCreateFlag) {
		FSpCreateResFile(&prefsFSSpec, kPrefsCreatorType, kPrefsFileType, smSystemScript);
		errCode = ResError();
		}
	
	if (errCode != noErr)
		goto punt;
	
	if (oFileID != NIL) {
		// Get the file's 'hard' ID
		CInfoPBRec	info;
		
		info.hFileInfo.ioNamePtr	= kPrefsFileName;
		info.hFileInfo.ioVRefNum	= prefsFSSpec.vRefNum;
		info.hFileInfo.ioFDirIndex	= 0;						// No indexing
		info.hFileInfo.ioDirID		= prefsFSSpec.parID;
		info.hFileInfo.ioFVersNum	= 0;						// To be on the safe side
		
		errCode = PBGetCatInfoSync(&info);
		
		*oFileID = (errCode == noErr) ? info.hFileInfo.ioDirID : 0;
		}
	
	// Open the file
	prefsRefNum = FSpOpenResFile(&prefsFSSpec, iPermissions);
	
punt:
	return prefsRefNum;
	}
	
#endif
	
/******************************************************************************************
 *
 *	GetPrefs()
 *	
 ******************************************************************************************/

#if 0		// Currently unused

static OSErr
GetPrefs(
	tPrefs*	oPrefs)
	
	{
	const unsigned long		kCurPrefVers	= 0x01000000,
							kPrefsResType	= 'Pref';
	const short				kPrefsResID		= 129;
	const eCategoryClass	kDefCategories	= ccMatchMax;
	
	short	prefsRefNum;
	long	prefsDirID;
	
	// Initialize prefs structure in case we can't get a valid set 
	oPrefs->vers		= kCurPrefVers;
	oPrefs->license		= llStarter;
	oPrefs->cc			= kDefCategories;
	oPrefs->padByte		= 0;
	oPrefs->padShort	= 0;
	
	// Open (but don't create) the prefs file 
	prefsRefNum = OpenPrefsFile(fsRdPerm, false, &prefsDirID);
	
	if (prefsRefNum != -1) {
		// File opened successfully, get the prefs resource 
		Handle tempHdl = Get1Resource(kPrefsResType, kPrefsResID);

		// If the resource is there *and* the size is right *and* the versions match,
		// *and* the license is OK, copy the data and decode it.
		// If there is an exact size match, do a blind copy, otherwise
		// copy component by component.
		if (tempHdl != nil && GetHandleSize(tempHdl) == sizeof(tPrefs)) {
			tPrefs*	resPrefs = (tPrefs*) *tempHdl;
			
			if (resPrefs->vers == kCurPrefVers) {
				
				switch (resPrefs->license) {
					case llArtist:
					case llDeveloper:
						oPrefs->license = resPrefs->license;
						oPrefs->cc		= resPrefs->cc;
						break;
					
					default:
						// Either llStarter or invalid license
						// Nothing to do.
						break;
					}
				}
			}
			
		// Release the pref resource and close the file 
		CloseResFile(prefsRefNum);
		}
	
	return ResError();
	}

#endif
	
	
/******************************************************************************************
 *
 *	GetTime()
 *	
 *	Cheesy cover for a standard Mac OS function not provided with QTMLClient.
 *	
 ******************************************************************************************/
#ifdef WIN_VERSION

void 
GetTime(DateTimeRec*	oDTPtr)
	{
	SYSTEMTIME xpTime;
	
	GetSystemTime(&xpTime);
	
	oDTPtr->year		= xpTime.wYear;
	oDTPtr->month		= xpTime.wMonth;
	oDTPtr->day			= xpTime.wDay;
	oDTPtr->hour		= xpTime.wHour;
	oDTPtr->minute		= xpTime.wMinute;
	oDTPtr->second		= xpTime.wSecond;
	oDTPtr->dayOfWeek	= xpTime.wDayOfWeek + 1;
	}
	
#endif


/******************************************************************************************
 *
 *	DllMain()
 *	
 *	Used to intialized variables needed by the external object when the DLL is loaded under
 *	Windows.
 *	
 ******************************************************************************************/

#ifdef WIN_VERSION

	static void LitterWinGetFileModDate(const char iFPath[], FILETIME* oModDate)
		{
		HANDLE fileHandle = CreateFile(	iFPath,
										GENERIC_READ, FILE_SHARE_READ,
										NULL, OPEN_EXISTING,
										FILE_ATTRIBUTE_NORMAL, NULL);
		
		if (fileHandle != INVALID_HANDLE_VALUE) {
			GetFileTime(fileHandle, NIL, NIL, oModDate);
			CloseHandle(fileHandle);
			}
			
		}
	
	static unsigned long LitterWinCalcVersVal(unsigned long iMajMin, unsigned long iBuildPlatform)
		{
		unsigned long	majorRev		= (iMajMin & 0x00ff0000) >> 16,
						minorRev		= iMajMin & 0x000000ff,
						bugStageBuild	= (iBuildPlatform >> 16);

		// Convert busStageBuild from 16-bit binary value to the 6-digit decimal representation
		// used in Litter
		bugStageBuild =	  ((bugStageBuild & 0xf000) >> 12) * 100000		// Bug fix
						+ ((bugStageBuild & 0x0f00) >>  8) *   1000		// Stage
						+ ((bugStageBuild & 0x00ff));					// Build
						
		return	  majorRev			* 10000000
				+ minorRev			*  1000000
				+ bugStageBuild;
		}
	
	static void LitterWinGetVersVals(const char iFPath[], unsigned long oVVals[2])
		{
		unsigned long	versHandle,
						len = GetFileVersionInfoSize((char*) iFPath, &versHandle);
		
		if (len > 0) {
			char* versBuf = getbytes(len);		// malloc() fails for some reason,
												// so use Max memory.
			
			if (versBuf == NIL)					// Quick sanity check...
				return;							// ... and punt
			
			if (GetFileVersionInfo((char*) iFPath, versHandle, len, versBuf)) {
				VS_FIXEDFILEINFO*	ffi;
				unsigned int		ffiLen;
				
				if (VerQueryValue(versBuf, "\\", &((void*) ffi), &ffiLen)
						&& ffiLen >= sizeof(VS_FIXEDFILEINFO)) {
					oVVals[0] = LitterWinCalcVersVal(ffi->dwFileVersionMS, ffi->dwFileVersionLS);
					oVVals[1] = LitterWinCalcVersVal(ffi->dwProductVersionMS, ffi->dwProductVersionLS);
					}
				}
			
			freebytes(versBuf, len);
			}
		
		}
	
BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID);

BOOL WINAPI
DllMain(
	HINSTANCE	iDLLInstance,
	DWORD		iReason,
	LPVOID		iReserved)
	
	{
	// This DllMain() is called by the actual entry point, DllMainCRTStartup().
	// Must set x86 Linker settings to use "Default" Entry point for this code to work.
	
	#pragma unused(iReserved)
	
	CHAR	externalPath[MAX_PATH];
	
	// Perform actions based on the reason for calling.    
	switch(iReason) {         
	case DLL_PROCESS_ATTACH:
        gDLLInstanceHdl = iDLLInstance;
		
		GetModuleFileName(iDLLInstance, externalPath, MAX_PATH);
		
		LitterWinGetFileModDate(externalPath, &gModDate);
		LitterWinGetVersVals(externalPath, gObjVersVals);
		break;
    
    default:
    	// Ignore DLL_THREAD_ATTACH, DLL_THREAD_DETACH, DLL_PROCESS_DETACH        
        break;    
		}    
	
	return TRUE; 
	}

/*	
	// The following is the way I used to do it. It used a custom entry point
	//
	// (CW Target Settings, x86 Linker Tab, Subsytem Windows Console (CUI), Entry
	// point usage popup: User Specified, Entry point _LitterDllEntry@12
	//
	// Somewhere I had read that this was the way to grab HINSTANCE-related data at
	// DLL start time. What my source neglected to explain is that doing this does
	// not set up some stuff that the C Runtime system needs--usually only in exceptional
	// circumstances, which proved indeed to be rare (although not as rare as one would
	// like) but fatal (the DLL would crash, taking Max/MSP with it). The more effective way
	// to get the data we need is given above. The trick is that Metrowerks provides
	// a default DllMain() which will get linked in if none is written in the project's
	// source code, but due to the way the linker is organized, a user-written DllMain()
	// will get linked in first and cause the default function to be ignored (we don't
	// even get Linker errors or warning, which is nice).
	//
	// Keeping this code around for a while just in case.
	//
	// PC 27-Mar-2006

	BOOL WINAPI LitterDllEntry(HINSTANCE, DWORD, LPVOID);
	
	BOOL WINAPI
	LitterDllEntry(HINSTANCE iDLLInstance, DWORD iReason, LPVOID lpReserved) 
		{
		#pragma unused(lpReservec)
		
		CHAR	externalPath[MAX_PATH];
		
		// Perform actions based on the reason for calling.    
		switch(iReason) {         
		case DLL_PROCESS_ATTACH:
        	// Initialize once for each new process.  Return FALSE to fail DLL load.
            gDLLInstanceHdl = iDLLInstance;
			
			GetModuleFileName(iDLLInstance, externalPath, MAX_PATH);
			
			LitterWinGetFileModDate(externalPath, &gModDate);
			LitterWinGetVersVals(externalPath, gObjVersVals);

			// Since the DLL_THREAD_ATTACH and DLL_THREAD_DETACH cases below don't actually
			// do anything, we don't need to receive those messages.
			// Tell Windows to optimize them out.  
			DisableThreadLibraryCalls(iDLLInstance);
			break;
        
        case DLL_THREAD_ATTACH:         
			// Do any thread-specific initialization.
            break;        
		
		case DLL_THREAD_DETACH:
			// Do any thread-specific cleanup.            
			break;
        
        case DLL_PROCESS_DETACH:        
			// Perform any necessary cleanup.
            break;    
			}    
		
		return TRUE; 
		}
*/	
#endif	
	
/******************************************************************************************
 *
 *	LitterCopyStrListResourceByName(iName)
 *
 ******************************************************************************************/

typedef struct strList {
	// This is an image of the content of an 'STR#' resource
	short	strCount;
	Byte	strings[0];			// Really variable length
	} tStrList;

tStrList*
LitterCopyStrListResourceByName(
	const char				iName[])
	{
	Str255		resName;
	Handle		rHandle;
	tStrList*	result	= NIL;
	
	MoveC2PStr(iName, resName);
	
	rHandle = GetNamedResource('STR#', resName);
	if (rHandle != NULL && *rHandle != NULL) {
		Size strTableSize = GetHandleSize(rHandle);
		
		result = (tStrList*) NewPtr(strTableSize);
		if (result != NULL) {
			BlockMoveData((*rHandle), result, strTableSize);
			}
		
		ReleaseResource(rHandle);
		}
	
	return result;
	}

void
LitterGetCStringFromStrList(
	char				oBuffer[kMaxResourceStrSize],
	const tStrListPtr	iStrList,
	int					iIndex)
	
	{
	
	// In case anything goes wrong
	oBuffer[0] = '\0';
	
	if (iStrList != NIL)  {
		short stringCount = iStrList->strCount;
		
		if (0 < iIndex && iIndex <= stringCount) {
			Byte* theStr = iStrList->strings;
			
			while (--iIndex > 0) 
				theStr += ((short) theStr[0]) + 1;
				
			MoveP2CStr(theStr, oBuffer);
			}
		}
	
	}


/******************************************************************************************
 *
 *	LitterSetupClass(iClassName, iNew, iFree, iSize, iMenu, iArgs)
 *
 *	Pattr-agnostic wrapper for setup()/class_new()
 *	Uses the right function depending on the setting of the macro LITTER_USE_OBEX
 *	
 ******************************************************************************************/

void
LitterSetupClass(
	const char		iName[],					// Name of the object class
	long			iSize,						// Size, should be <= 32764
	long			iObexOffset,				// Magic Obex cacluation
	method			iNew,						// Instantiation method
	method			iFree,						// Free method (may be NIL)
	method			iUI,						// Instantiation for UI objects
	const tTypeList	iArgs)						// List of argument types

	{
#if LITTER_USE_OBEX
	gObjectClass = class_new((char*) iName, iNew, iFree, iSize, iUI,
							 iArgs[0], iArgs[1], iArgs[2], iArgs[3],
							 iArgs[4], iArgs[5], iArgs[6], 0);
	
	class_obexoffset_set(gObjectClass, iObexOffset);						 
	
#else
	#pragma unused(iName, iObexOffset)
	
	setup(	&gObjectClass, iNew, iFree, (short) iSize, iUI,
			iArgs[0], iArgs[1], iArgs[2], iArgs[3], iArgs[4], iArgs[5], iArgs[6], 0);
	
#endif
	}

void
LitterSetupClassGimme(
	const char	iName[],						// All parameters as LitterSetupClass
	long		iSize,
	long		iObexOffset,
	method		iNew,
	method		iFree,
	method		iUI)
	
	{
#if LITTER_USE_OBEX
	gObjectClass = class_new((char*) iName, iNew, iFree, iSize, iUI, A_GIMME, 0);
	
	class_obexoffset_set(gObjectClass, iObexOffset);						 
	
#else
	#pragma unused(iName, iObexOffset)
	
	setup(&gObjectClass, iNew, iFree, (short) iSize, iUI, A_GIMME, 0);
	
#endif
	}
	

Object* LitterAllocateObject(void)
#if LITTER_USE_OBEX
	{ return (Object*) object_alloc(gObjectClass); }
#else
	{ return (Object*) newobject(gObjectClass); }
#endif


/******************************************************************************************
 *
 *	LitterInit(iClassName)
 *	
 *	On Mac: copy 'STR#' resource by name to MaxTemp and grab the resource's new ID. Also
 *			get file modification date from the resource fork.
 *
 *	On Windows: Initialize the Max interface to QTML. The resource and date stuff have been
 *			dealt with in the custom DLL entry point code.
 *
 *	On both platforms: Add standard message[s], list ourselves in the New Object List, and
 *			post a version/copyright message to the Max window
 *
 *	NB: LitterInit() no longer calls Taus88Init(); objects requiring random numbers must do
 *			this themselves.
 *	
 ******************************************************************************************/

	#if LITTER_USE_OBEX
	
	t_max_err LitterGetAttrAtom(Atom* iAtom, long* ioArgC, Atom** ioArgV)
		{
		
		if (ioArgC == 0 || *ioArgV == NIL) {
			// Need to allocate memory
			*ioArgV = (Atom*) getbytes(sizeof(Atom));
			if (*ioArgV == NIL) return MAX_ERR_OUT_OF_MEM;
			}
		
		*ioArgC		= 1;
		*ioArgV[0]	= *iAtom;
		
		return MAX_ERR_NONE;
		}

	
	t_max_err LitterGetAttrInt(long iVal, long* ioArgC, Atom** ioArgV)
		{
		Atom a;
		
		AtomSetLong(&a, iVal);
		
		return LitterGetAttrAtom(&a, ioArgC, ioArgV);
		}
		
	t_max_err LitterGetAttrFloat(double iVal, long* ioArgC, Atom** ioArgV)
		{
		Atom a;
		
		AtomSetFloat(&a, iVal);
		
		return LitterGetAttrAtom(&a, ioArgC, ioArgV);
		}
		
	t_max_err LitterGetAttrSym(Symbol* iSym, long* ioArgC, Atom** ioArgV)
		{
		Atom a;
		
		AtomSetSym(&a, iSym);
		
		return LitterGetAttrAtom(&a, ioArgC, ioArgV);
		}
	
	void LitterGetVersStr(long, long, char[]);
	static t_max_err LitterGetVers(Object* me, void* iAttr, long* ioArgC, Atom** ioArgV)
		{
		#pragma unused(me, iAttr)
		
		char	versStr[kMaxResourceStrSize];
		
		LitterGetVersStr(1, lpVersTypeShort, versStr);
		
		return LitterGetAttrSym(gensym(versStr), ioArgC, ioArgV);
		}
	#endif

void LitterInit(
	const char	iClassName[],
	UInt32		iFlags)
	{
	#pragma unused(iFlags)					// Reserved for future use
											// We may need to make parts of LitterInit()
											// optional someday. For ease of code maintenance,
											// the parameter is already defined, and a value
											// of 0 will indicate default behavior.
	
	Boolean	foundResource = false;
	
#ifdef WIN_VERSION

//	XQT_InitializeQTML(0);
	
	foundResource = true;
	
#else

	Str255	pStrClassName;

#if LITTER_POST_MAX4
	if (LitterGetMaxVersion() >= 0x0500) {
			// Core Foundation data
		CFStringRef bundleID	= NULL,		// Reference will be 'created'
					classStr	= NULL;		// Reference will be 'created'
		CFURLRef	rsrcURL		= NULL,		// Reference will be 'created'
					bundleURL	= NULL;		// Reference will be 'created'
		CFStringRef	urlStr		= NULL;		// Reference will NOT be 'created'
		FSRef		myResource;				// Fixed-size structure
			// Bridge between CF and Max API
		char		bundlePath[kMaxResourceStrSize],
					bundleName[kMaxResourceStrSize];
			// Max API data
		short		pathID;
		
		bundleID = CFStringCreateWithFormat(kCFAllocatorDefault,	NULL,
											CFSTR("de.castine.%s"),	iClassName);
		if (bundleID == NULL) goto puntmax5;
		
			// These are not allowed to fail in a deployment object
		gLPObjBundle = CFBundleGetBundleWithIdentifier(bundleID);
		bundleURL = CFBundleCopyBundleURL(gLPObjBundle);
		CFURLGetFileSystemRepresentation(bundleURL, true, (Byte*) bundlePath, kMaxResourceStrSize);
		path_frompathname((char*) bundlePath, &pathID, bundleName);
		path_getmoddate(pathID, &gModDate);
		
		classStr = CFStringCreateWithCString(kCFAllocatorDefault,
											 iClassName,
											 kCFStringEncodingMacRoman);
		if (classStr == NULL) goto puntmax5;
			
		rsrcURL = CFBundleCopyResourceURL(gLPObjBundle, classStr, CFSTR("rsrc"), NULL); 
		if (rsrcURL == NULL) goto puntmax5;
			
		urlStr = CFURLGetString(rsrcURL);
			// API ref indicates this will not return NULL
		
		if ( !CFURLGetFSRef(rsrcURL, &myResource) )
			goto puntmax5;
		
		// Grab all strings defined in the STR# resource bearing our name
		gStringTable = LitterCopyStrListResourceByName(iClassName);
		
		// If we make it here, we've found our resources stored in the Bundle
		foundResource = true;
		
	// -----------------------------------------------------------------------------
	// Clear up all references following 'Create Rule'
	puntmax5:
		if (bundleID != NULL)	{ CFRelease(bundleID);	bundleID	= NULL; }
		if (classStr != NULL)	{ CFRelease(classStr);	classStr	= NULL; }
		if (rsrcURL != NULL)	{ CFRelease(rsrcURL);	rsrcURL		= NULL; }
		if (bundleURL != NULL)	{ CFRelease(bundleURL); bundleURL	= NULL; }
		}
		
	else {
#endif		// LITTER_POST_MAX4

	// First test if we can access our 'STR#' resource
	// If not, we're probably in a collective and won't be able to get any resource
	// based data.
	// This is a bit heavy-handed, but solid.
	Handle	testResH;
	
	MoveC2PStr(iClassName, pStrClassName);
	testResH = GetNamedResource('STR#', pStrClassName);
	foundResource = (testResH != NIL);
	
	if (foundResource) {
		short	externResFile	= CurResFile(),
				prevResFile		= PrevResFile();
		// Get *named* resource
//		resnamecopy('STR#', (char*) iClassName);
		
		// ... and store the IDs assigned to them in Max Temp
		if (prevResFile > 0) {
			UseResFile(prevResFile);
			gStrResID = FindResID('STR#', iClassName);
			UseResFile(externResFile);
			}
		
		gModDate = XObjGetModDate(externResFile);
		
		// We assume that the calling app is not using our 'STR#' resource,
		// so free it up now
		ReleaseResource(testResH);
		}

#if LITTER_POST_MAX4
		}
#endif		// LITTER_POST_MAX4
		
#endif
	
#if LITTER_USE_OBEX
	{
	Object*	attr;
	
	// Add global message/attribute that all Litter objects implement uniformly
	class_addmethod	(gObjectClass, (method) LitterVers,	"vers",	A_DEFLONG, A_DEFLONG, 0);
	attr = attribute_new("vers",
						 gensym("symbol"),
						 kAttrFlagsReadOnly,
						 (method) LitterGetVers,
						 NULL);
	class_addattr(gObjectClass, attr);
	
	// Obex voodoo
	class_addmethod(gObjectClass, (method) object_obex_quickref, "quickref", A_CANT, 0);
	class_register(CLASS_BOX, gObjectClass);
	
	}
#else
	// Add global message that all Litter objects implement uniformly
	addmess	((method) LitterVers,	"vers",	A_DEFLONG, A_DEFLONG, 0);
#endif
	
	LitterAddClass(iClassName, kLPcatID);
	
	// Coy copyright message. But only if we could read the resource data
	if (foundResource)
		LitterHello(iClassName);
	
	}
	
/******************************************************************************************
 *
 *	LitterAddClass()
 *	
 *	Reads categories for the Max "New Object List" from a given STR# resource. The string
 *	list *must* consist of three entries in the following order: Litter category, category
 *	used in Max 2.x - 3.x, and category used starting at Max 4.
 *	
 *	This function relies on the file defining the current external to also be the current
 *	resource file. This is the case inside any external's main().
 *	
 ******************************************************************************************/

void
LitterAddClass(
	const char*	iClassName,
	short		iResID)
	
	{
	const short	kMaxStandaloneMask	= 0x4000,
				kMax4Vers			= 0x0400;
	
#ifdef WIN_VERSION
	#pragma unused(iResID)
#else	
	enum {
		strIDLitter	= 1,
		strIDMaxPre4,
		strIDMax4OrLater
		};
#endif
	
	short		hostVers	= maxversion();
	
	if ((hostVers & kMaxStandaloneMask) == 0) {
		// If the stand-alone bit is set there's no reason to bother with any of the following.
		char	catStr[kMaxResourceStrSize];
	
		// Litter Category
	#ifdef WIN_VERSION
		LoadString(gDLLInstanceHdl, lpStrIndexLitterCategory, catStr, kMaxResourceStrSize);
	#else
		GetIndCString(catStr, iResID, strIDLitter);		// LitterAddClass() is called from
														// main(), so this call is still
														// safe
	#endif
		
		finder_addclass((char*) catStr, (char*) iClassName);
		
		// Max Category
	#ifdef WIN_VERSION
		LoadString(gDLLInstanceHdl, lpStrIndexMax4Category, catStr, kMaxResourceStrSize);
	#else
		// On Mac OS this depends upon current version
		// We know the stand-alone bit is clear
		// Currently we are only concerned if the version is pre-4.0 or not.
		// Cf. note regarding GetIndCString() above.
		GetIndCString(catStr,
					 iResID,
					 (hostVers < kMax4Vers) ? strIDMaxPre4 : strIDMax4OrLater);
	#endif
		
		finder_addclass((char*) catStr, (char*) iClassName);
		}
	
	}

/******************************************************************************************
 *
 *	LitterInfo(iWhat, iClassName, iTattleMeth)
 *	
 *	Puts up a little informational alert. Optionally sends the calling object a tattle
 *	message. In this case it's more convenient for the calling object to pass us a pointer
 *	to its tattle method rather than using the Max method-lookup facilities.
 *	
 ******************************************************************************************/

void
LitterInfo(
	const char	iClassName[],				// Name of the object
	tObjectPtr	iTattleParam,				// Parameter to tattle method (if specified)
	method		iTattleMeth)				// Address of tattle method
	
	{
	char	versStr[kMaxResourceStrSize],	// Could be smaller
			packStr[kMaxResourceStrSize],
			dateStr[kMaxResourceStrSize];

#ifdef WIN_VERSION
	SYSTEMTIME	modDateSysTime;
	char		aboutStr[kMaxResourceStrSize],
				msgStr[kMaxResourceStrSize];
	
#else
	const short	kNoAutoCenter				= 0,
				kAlertPositionParentWindow	= 0xb00a;
				
#endif
		// Test the following once at beginning of call.
		// Avail ourselves of C's mindless willingness to treat pointers and anything
		// else as pseudo-Booleans.
	Boolean	doTattle = iClassName && iTattleMeth && !KeyIsPressed(kcShift);

	LitterGetVersStr(1, lpVersTypeLong, versStr);
	LitterGetVersStr(2, lpVersTypeLong, packStr);

#ifdef WIN_VERSION
	FileTimeToSystemTime(&gModDate, &modDateSysTime);
	GetDateFormat(LOCALE_USER_DEFAULT, DATE_LONGDATE, &modDateSysTime, NULL, dateStr, sizeof(dateStr));
	sprintf(aboutStr, "%s info...", iClassName);
	sprintf(msgStr, "Version %s.\r%s.\r\rLast modified %s.",
				versStr, packStr, dateStr);
	
	MessageBox(NIL, TEXT(msgStr), TEXT(aboutStr), MB_OK);
#else
	DateCString(dateStr, gModDate, 2);

	// Increase size of Max's "Ouchstring" alert to something more reasonable and
	// add OS 7 autopositioning capabilities. Call the alert and then return to its
	// previous state. At the end of the alert, also test for the shift key, to give
	// the user a second chance to subvert tattle information.
//	TweakOuchstringAlert(50, 24, kAlertPositionParentWindow);
	ouchstring("%s\rVersion %s.\r%s.\r\rLast modified %s.",
				(char*) iClassName, versStr, packStr, dateStr);
	doTattle = doTattle && !KeyIsPressed(kcShift);
//	TweakOuchstringAlert(-50, -24, kNoAutoCenter);
#endif
	
	// So, are we going to tattle or aren't we?
	if (doTattle)
		iTattleMeth(iTattleParam);
	
	}

/******************************************************************************************
 *
 *	LitterExpect(iExpFunc, iObj, iSel, iLabel, iTarg)
 *
 ******************************************************************************************/


void
LitterExpect(
	tExpectFunc	iExpFunc,
	Object*		iObj,
	Symbol*		iSel,
	Symbol*		iTarg,
	Boolean		iLabel)
	
	{
	static SymbolPtr sExpSyms[expCount]	= {NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL, NIL};
	
	eExpectSelector	expSel;
	
	if (sExpSyms[expMean] == NIL) {
		// Initialize array of symbols
		sExpSyms[expMean]		= gensym("mean");
		sExpSyms[expMedian]		= gensym("median");
		sExpSyms[expMode]		= gensym("mode");
		sExpSyms[expVar]		= gensym("variance");
		sExpSyms[expStdDev] 	= gensym("stddev");
		sExpSyms[expSkew]		= gensym("skew");
		sExpSyms[expKurtosis]	= gensym("kurtosis");
		sExpSyms[expMin]		= gensym("min");
		sExpSyms[expMax]		= gensym("max");
		sExpSyms[expEntropy]	= gensym("entropy");
		}
	
	// Find enumerator for the input symbol
	for (expSel = expMean; expSel < expCount; expSel += 1) {
		if (iSel == sExpSyms[expSel]) break;
		}
	
	if (expSel < expCount) {										// Valid selector
		double	expVal = iExpFunc(iObj, expSel);
		Atom	msgParam;
		
		if (iLabel) AtomSetFloat(&msgParam, expVal);
		
		if (iTarg != NIL && iTarg->s_name[0] != '\0') {
			// Explicit "target" symbol, try to forward 
			if (iLabel)	ForwardAnything(iTarg, iSel, 1, &msgParam);
			else		ForwardFloat(iTarg, expVal);
			}
		else {
			// Use main outlet
			if (iLabel)	outlet_anything(iObj->o_outlet, iSel, 1, &msgParam);
			else		outlet_float(iObj->o_outlet, expVal);
			}
		}
		
	else {															// Invalid selector
		error("%s: invalid selector for expect message: %s", ob_name(iObj), iSel->s_name);
		}
		
	
	}

/******************************************************************************************
 *
 *	LitterAssist(iDir, iArgNum, iFirstIn, iFirstOut, oDest)
 *	LitterAssistVA(iDir, iArgNum, iFirstIn, iFirstOut, oDest, ...)
 *	
 *	Cross-platform handling of assistance strings.
 *
 *	On MacOS this is simply a wrapper for assist_string. It relies on the global variable
 *	gStrResID.
 *
 *	On Windows we call the Windows Resource Manager directly, relying on the global variable
 *	gDLLInstanceHdl.
 *
 ******************************************************************************************/

void
LitterAssist(
	long		iDir,
	long		iArgNum,
	short		iFirstIn,
	short		iFirstOut,
	char		oDestCStr[])
	
	{
	iArgNum += (iDir == ASSIST_INLET) ? iFirstIn : iFirstOut;
		
	LitterGetIndString(iArgNum, oDestCStr);
	}


void
LitterAssistVA(
	long		iDir,
	long		iArgNum,
	short		iFirstIn,
	short		iFirstOut,
	char		oDestCStr[],
				...)
	
	{
	int		strIndex = ((iDir == ASSIST_INLET) ? iFirstIn : iFirstOut) + iArgNum;
	char 	formatStr[kMaxResourceStrSize];
	va_list	argPtr;
	
	LitterGetIndString(strIndex, formatStr);
	
	va_start(argPtr, oDestCStr);
	vsnprintf(oDestCStr, kMaxAssistStrLen, formatStr, argPtr);
	va_end(argPtr);						// Does nothing, but ANSI says it should be here
	
	}

void
LitterAssistResFrag(
	long		iDir,
	long		iArgNum,
	short		iFirstIn,
	short		iFirstOut,
	char		oDestCStr[],
	short		iFragIndex)
	
	{
	char	fragStr[kMaxResourceStrSize];
	
	LitterGetIndString(iFragIndex, fragStr);
	LitterAssistVA(iDir, iArgNum, iFirstIn, iFirstOut, oDestCStr, fragStr);
	
	}
	
				


/******************************************************************************************
 *
 *	LitterGetIndString(iClassName)
 *	
 ******************************************************************************************/

void
LitterGetIndString(
	short	iStrIndex,
	char	oDestCStr[])
	
#ifdef WIN_VERSION
	{ LoadString(gDLLInstanceHdl, iStrIndex, oDestCStr, kMaxResourceStrSize); }
	
#else
	{
	if (LitterGetMaxVersion() >= 0x0500)
		 LitterGetCStringFromStrList(oDestCStr, gStringTable, iStrIndex); 
	else GetIndCString(oDestCStr, gStrResID, iStrIndex);
	}
#endif

/******************************************************************************************
 *
 *	LitterGetVersStr(iVersID, iVerboseFlag oDestCStr)
 *	
 *	Cross-platform capable accesss to version strings. For handling of the "vers" message
 *	and also used by LitterInfo.
 *
 *	The iVersID should either have the value one (for object version information) or two
 *	(for package version information). Any other value is interpreted as one. Set iVerboseFlag
 *	to true for the "long" string.
 *
 *	TO DO:	Think about grabbing version information under Windows directly from the
 *			VERSIONINFO resource, assuming this is copied with the DLL into collectives.
 *			(The whole reason we keep copies of this information in 'STR#'/STRINGTABLE
 *			resources is because 'vers' resources are stripped when copied into collectives
 *			in the Mac world.)
 *
 ******************************************************************************************/

void
LitterGetVersStr(
	long	iVersID,
	long	iVersType,
	char	oDestCStr[])
	
	{
#ifdef WIN_VERSION
	
	switch (iVersType) {
	default:
		sprintf(oDestCStr, "%x", LitterGetVersVal(iVersID));
		break;
	
	case lpVersTypeShort:
	case lpVersTypeLong:
		{
		short i = iVersType
					+ ( (iVersID == lpVersIDPackage)
							? lpStrIndexPackageVersShort
							: lpStrIndexObjectVersShort )
					- 1;
		
		LoadString(gDLLInstanceHdl, i, oDestCStr, kMaxResourceStrSize);
		}
		break;
		}
	
#else

#if LITTER_POST_MAX4
	if (LitterGetMaxVersion() >= 0x0500) {
	
		if (iVersType == lpVersTypeVal)
			sprintf(oDestCStr, "%lx", LitterGetVersVal(iVersID));
	
		else  {
			CFStringRef	versString = NULL;
			
			if (gLPObjBundle != NULL) {
				CFStringRef versKey = (iVersType == lpVersTypeShort)
											? ( (iVersID == lpVersIDPackage)
													? kLPPackageShortVersionKey
													: kCFBundleVersionKey)
											: ( (iVersID == lpVersIDPackage)
													? kLPPackageLongVersionKey
													: kLPObjectLongVersionKey);

				versString = (CFStringRef)
							 CFBundleGetValueForInfoDictionaryKey(gLPObjBundle, versKey);
				}
			
			if (versString != NULL)
				 CFStringGetCString(versString,
									oDestCStr,
									kMaxResourceStrSize,
									kCFStringEncodingUTF8);
			else {
				// Plan B
				UInt32 versNum = CFBundleGetVersionNumber(gLPObjBundle);
				sprintf(oDestCStr, "0x%lx", versNum);
				//	strcpy(	oDestCStr, (iVersType == lpVersTypeShort) ? "0.0" : "Unknown");
				}
			}
		
		}
	
	else {
#endif
	short i = iVersType
				+ ( (iVersID == lpVersIDPackage)
						? lpStrIndexPackageVersShort
						: lpStrIndexObjectVersShort )
				- 1;
	
	GetIndCString(oDestCStr, gStrResID, i);
#if LITTER_POST_MAX4
		}
#endif
	
#endif
	}

long
LitterGetVersVal(
		long iVersID)
	
	{
	long	result;
	
#ifdef WIN_VERSION
	
	result = gObjVersVals[iVersID - 1];
	
#else

#if LITTER_POST_MAX4

	if (LitterGetMaxVersion() >= 0x0500) {
		// With Max 5 we finally move to Carbon/Cocoa Bundle-based resources
		if (gLPObjBundle != NULL) {
			if (iVersID == lpVersIDObject)
				 result = (long) CFBundleGetVersionNumber(gLPObjBundle);
			
			else result = (long) CFBundleGetValueForInfoDictionaryKey(	gLPObjBundle,
																		kLPPackageVersValKey);
			// !!	This is actually not working for the Package version number
			//		I'm not sure why, but the cleaner solution would be to get the short
			//		version string and parse it, much the way CFBundleGetVersionNumber()
			//		does it with the short version string for the object.
			//		This is fairly tedious and AFAIK no one is using this facility anyway
			//		(I'm not) so this gets a low priority to fix.
			 }
		else result = 0;
		}
	else {

#endif			// LITTER_POST_MAX4

		Str255	valStr;
		short	strIndex = (iVersID == lpVersIDPackage)
								? lpStrIndexPackageVersVal
								: lpStrIndexObjectVersVal;
		
		GetIndString(valStr, gStrResID, strIndex);
		StringToNum(valStr, &result);

#if LITTER_POST_MAX4
		}
#endif			// LITTER_POST_MAX4
		
	#endif
	
	return result;
	}

void
LitterVers(
	Object*	iObject,
	long	iVersID,
	long	iVersType)
	
	{
	
	if (iVersID == 0)
		iVersID = lpVersIDDefault;
	if (iVersType == 0)
		iVersType = lpVersTypeDefault;
	
	if (iVersType == lpVersTypeVal)
		outlet_int(iObject->o_outlet, LitterGetVersVal(iVersID));
	else {
		char versStr[kMaxResourceStrSize];
	
		LitterGetVersStr(iVersID, iVersType, versStr);
		
		outlet_anything(iObject->o_outlet, gensym(versStr), 0, NIL);
		}
		
	}


/******************************************************************************************
 *
 *	LitterHello(iClassName)
 *	
 ******************************************************************************************/

void
LitterHello(
	const char	iClassName[])
	
	{
#if MAC_VERSION && LITTER_POST_MAX4
	if (maxversion() >= 0x0500) {
		CFStringRef helloCFStr = CFBundleGetValueForInfoDictionaryKey(	gLPObjBundle,
																		kCFBundleGetInfoKey);
		char		helloCStr[kMaxResourceStrLen];
			
		if (helloCFStr != NULL)
			 CFStringGetCString(helloCFStr,
								helloCStr,
								kMaxResourceStrLen,
								kCFStringEncodingUTF8);
		else strcpy(helloCStr, "unknown");		
		
		post("Loaded %s. Version %s.", iClassName, helloCStr);
		}
		
	else {
#endif

	char	longVers1[kMaxResourceStrSize],
			longVers2[kMaxResourceStrSize];
	
	LitterGetVersStr(lpVersIDObject, lpVersTypeLong, longVers1);
	LitterGetVersStr(lpVersIDPackage, lpVersTypeLong, longVers2);
	
	post("Loaded %s. Version %s.", iClassName, longVers1, longVers2);
	
#if MAC_VERSION && LITTER_POST_MAX4
		}
#endif

	}


