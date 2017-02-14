/*
	File:		MrsNorris.c

	Contains:	Checks that Max patcher windows follow "The Rules"
				Loads a patch once only.
				If the patch is already open, brings it to the front.

	Written by:	Peter Castine

	Copyright:	© 2006 Peter Castine

	Change History (most recent first):

         <2>      8Ð6Ð06    pc      Modified a variable name to make it more readable. In the end
                                    that was all.
         <1>   26Ð4Ð2006    pc      first checked in.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark ¥ Include Files

#include "LitterLib.h"			// Also #includes MaxUtils.h, ext.h
#include "TrialPeriodUtils.h"
#include <string.h>		// We use strncmp()

#if !defined(TARGET_API_MAC_CARBON) || !TARGET_API_MAC_CARBON
	// Need to get Patcher typedef, need to explicitly #include
	#include "ext_user.h"
	#include "ext_wind.h"
#endif


#pragma mark ¥ Constants

const char*	kClassName		= "lp.mrsnorris";			// Class name



	// Indices for STR# resource
enum {
	strIndexInLeft		= lpStrIndexLastStandard + 1,
	strIndexOutLeft,
	
	strIndexHelpPath4,									// Help folder moved in Max 5
	strIndexHelpPath5,
	strIndexHelpSuffix4,
	strIndexHelpSuffix5,
	strIndexExtrasSubDir,
	strIndexProOverview4,
	strIndexStarterOverview4,
	strIndexProOverview5,
	strIndexStarterOverview5
	};


#pragma mark ¥ Windows Dynamic Linking

#ifdef WIN_VERSION
typedef void		(*tWindAPIVisFunc)(t_wind*);

tWindAPIVisFunc		WindVisFunc	= NIL;
#endif



#pragma mark ¥ Object Structure

typedef struct {
	Object		coreObject;
	Symbol*		filename;
	short		searchDir;
	Boolean		helpSuffix,
				recurse;
	} objMrsNorris;


#pragma mark ¥ Global Variables

static Boolean		gHasWindAPI = false;

static SymbolPtr	gSymLPO			= NIL,
					gSymOverview	= NIL;
static short		gOverviewDir	= 0,
					gHelpDir		= 0;


#pragma mark ¥ Function Prototypes

	// Class message functions
void*	MrsNorrisNew(Symbol*, long);

	// Object message functions
static void MrsNorrisBang	(objMrsNorris*);
static void MrsNorrisSet	(objMrsNorris*, Symbol*, long);
static void MrsNorrisLoad	(objMrsNorris*, Symbol*);
static void MrsNorrisHelp	(objMrsNorris*, Symbol*);
static void MrsNorrisLPO	(objMrsNorris*);

static void	MrsNorrisTattle	(objMrsNorris*);
static void MrsNorrisAssist	(objMrsNorris*, void*, long, long, char*);
static void MrsNorrisInfo	(objMrsNorris*);




#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark ¥ Inline Functions

/******************************************************************************************
 *
 *	Wrappers around the conditionally available t_wind API calls.
 *	
 ******************************************************************************************/

static inline void WindAPICallWindVis(t_wind* iWindow)
	{
#ifdef WIN_VERSION
	if (WindVisFunc != NIL) WindVisFunc(iWindow);
#else
	if (gHasWindAPI) wind_vis(iWindow);
#endif
	}


#pragma mark -

/******************************************************************************************
 *
 *	main()
 *	
 *	Standard Max/MSP External Object Entry Point Function
 *	
 ******************************************************************************************/

	static Boolean SearchForFile(char ioName[])
		{
		short	mVer = LitterGetMaxVersion(),
				searchError,
				pathID,
				binary;
		
		if (mVer < 0x0400)
			searchError = locatefile(ioName, &pathID, &binary);
		else {
			long	fTypeOut	= 0,
					fTypeIn		= (mVer < 0x0500) ? 'maxb' : 'JSON';
					
			searchError = locatefile_extended(ioName, &pathID, &fTypeOut, &fTypeIn, 1);
			}
		
		if (searchError == 0) {
			gSymOverview	= gensym(ioName);
			gOverviewDir	= pathID;
			}
		
		return searchError == 0;
		}

	static void InitializeGlobals(void)
		{
		Boolean	preMax5 = LitterGetMaxVersion() < 0x0500;
		char 	filename[kMaxResourceStrSize],
				tempStr[kMaxResourceStrSize];
		short	pathID;
		
		// First, set up t_wind API Calls, which are only avaliable prior to Max 5
		if (LitterGetMaxVersion() < 0x0500) {
		#ifdef WIN_VERSION
			HINSTANCE lib = LoadLibrary("MaxAPI.lib");
			
			// ASSERT(lib != NIL), because Max versions 4.3-4.6 won't be running on Windows
			// without it. Still, our programming style always checks for the worst.
			// Also note that although we could theoretically keep track of instantiation
			// count and free up everything when there are no objects using the library, but
			// given that our HINSTANCE is simply a pointer to the globally open instance that
			// isn't going to be freed ever, there doesn't seem to be much point 
			if (lib != NIL) {
				WindVisFunc			= (tWindAPIVisFunc)		 	GetProcAddress(lib, "wind_vis");
						
				// Again, it seems impossible that the procedure pointer should be NIL
				if (WindVisFunc == NIL)
					error("%s: couldn't get address of t_wind functions");
				else gHasWindAPI = true;
				}
		#else
			gHasWindAPI = true;
		#endif
			}
		
		// Now get the help path
		LitterGetIndString(	preMax5 ? strIndexHelpPath4 : strIndexHelpPath5, 
							filename);
		if (path_frompathname(filename, &pathID, tempStr) == noErr)
			 gHelpDir = pathID;
		else error("%s: could not find Max Help path (%s)", kClassName, filename);
			
		// Now try to find Pro Overview
		LitterGetIndString(	preMax5 ? strIndexProOverview4 : strIndexProOverview5,
							filename);
		if (SearchForFile(filename)) return;
		
		// That didn't work. Try the Starter Pack
		LitterGetIndString(	preMax5 ? strIndexStarterOverview4 : strIndexStarterOverview5,
							filename);
		if (SearchForFile(filename)) return;
		
		// If we get to this point, leave gSymOverview and gOverviewDir alone (NIL and
		// zero) but warn user
		error("%s: could not find Litter Power overview patcher", kClassName);
		
		}

void
main(void)
	
	{
	LITTER_CHECKTIMEOUT(kClassName);
	
	// Standard Max/MSP initialization mantra
	setup(	&gObjectClass,				// Pointer to our class definition
			(method) MrsNorrisNew,		// Instance creation function
			NIL,						// Default deallocation function
			sizeof(objMrsNorris),		// Class object size
			NIL,						// No menu function
			A_DEFSYM,					// Optional arguments:	1. patch filename
			A_DEFLONG,					//						2. Search in max-help/ ?
			0);	
	
	// Messages
	LITTER_TIMEBOMB addbang	((method) MrsNorrisBang);
	LITTER_TIMEBOMB addmess	((method) MrsNorrisLoad,	"load",		A_SYM, 0);
	addmess	((method) MrsNorrisHelp,	"help",		A_SYM, 0);
	addmess ((method) MrsNorrisLPO,		"lpo",		A_NOTHING);
	addmess	((method) MrsNorrisSet,		"set",		A_SYM, A_DEFLONG, 0);
	addmess	((method) MrsNorrisTattle,	"dblclick",	A_CANT, 0);
	addmess	((method) MrsNorrisTattle,	"tattle",	A_NOTHING);
	addmess	((method) MrsNorrisAssist,	"assist",	A_CANT, 0);
	addmess	((method) MrsNorrisInfo,	"info",		A_CANT, 0);
	
	// Initialize Litter Library
	LitterInit(kClassName, 0);
	
	// Initialize global symbols
	gSymLPO = gensym("lpo");
	InitializeGlobals();				// Initializes gSymOverview, gOverviewPath, and
										// gHelpPath
	
	}

#pragma mark -
#pragma mark ¥ Class Message Handlers

/******************************************************************************************
 *
 *	MrsNorrisNew(iName, iHelp)
 *	
 ******************************************************************************************/

void*
MrsNorrisNew(
	Symbol*	iName,
	long	iHelp)
	
	{
	objMrsNorris*	me	= newobject(gObjectClass);
	
	if (me != NIL) {
		MrsNorrisSet(me, iName, iHelp);
			// Currently only need  outlet for vers message
			// Access through me->coreObject.o_outlet
		outlet_new (me, NIL);
		}
	
	return me;
	}

#pragma mark -
#pragma mark ¥ Object Message Handlers

/******************************************************************************************
 *
 *	MrsNorrisBang(me)
 *	MrsNorrisLoad(me, iName)
 *	
 ******************************************************************************************/

	// Parameter block for SniffAndMeow()
	struct samParams {
		char	name[kMaxResourceStrSize];
		t_wind*	openWind;
		};
	
		// !!
		//	SniffAndMeow() no longer functions correctly under Max 5 because the
		//	t_wind structure has changed. It is also currently undocumented.
		//	Bummer
		// !!
	static short SniffAndMeow(Patcher* iPatcher, struct samParams* iSamParams)
		{
		t_wind*	pWind		= (t_wind*) iPatcher->p_wind;
								// The typecast is superfluous ever since the introduction
								// of the SDK for Max 4.2 or so, but it doesn't hurt.
								// For earlier v4.0 SDKs it is necessary because Max 
								// distinguished between a "public" and an "private"
								// interface, and the p_wind member normally exposed only
								// the public interface, while we need the private one 
								// I have not yet investigated the situation for pre-4.0
								// Max versions, where it may be necessary to get the window
								// title from the Window Manager WindowRecord.
		char*	wName		= pWind->w_name;
		int		matchVal	= strcmp(wName, iSamParams->name);
		
		if (matchVal == 0)
			iSamParams->openWind = pWind;
		
		return !matchVal;
		}
	
/*
	static void* LoadHelpFile(const char* iFileName)
		{
		void*	result = NIL;
		short	helpPath, 
				oldPath;
		char	mt[kMaxResourceStrSize];			// String will be empty on return from
													// path_setdefault()
		
		if (path_frompathname("./max-help", &helpPath, mt) == noErr) {
			oldPath = path_getdefault();
			path_setdefault(helpPath, true);		// Yes! Search help folder recursively
			result = stringload((char*) iFileName);
			path_setdefault(oldPath, false);
			}
		
		return result;
		}
*/
	
void
MrsNorrisBang(
	objMrsNorris*	me)
	
	{
	struct samParams	samp;
	
	// Quick sanity check
	if (me->filename == NIL || me->filename->s_name[0] == '\0') {
		error("%s: no file specified", kClassName);
		return;
		}
	
	samp.openWind	= NIL;
	strcpy(samp.name, me->filename->s_name);
	
	if (LitterGetMaxVersion() < 0x0500) {
		patcher_eachdo((eachdomethod)SniffAndMeow, (void*) &samp);
		}
		// !!	It is a temporary stopgap that we skip SniffAndMeow() on Max 5
		//		Until we have a published API we can't do anything with the pointer
		//		returned in samp.openWind anyway
	
	if (samp.openWind != NIL) {
		// Looks like we found what we're looking for
		WindAPICallWindVis(samp.openWind);
		}
	else {
		// Looks like we'll have to open a new window
		short	oldPath		= 0;
		void*	loadResult	= NIL;
		
		if (me->searchDir != 0) {
			oldPath = path_getdefault();
			path_setdefault(me->searchDir, me->recurse);
			}
		
		if (me->helpSuffix) {
			char	helpSuffix[kMaxResourceStrSize],
					helpFileName[kMaxResourceStrSize];
			
			if (LitterGetMaxVersion() >= 0x0500) {
				// Try the new help suffix first
				LitterGetIndString(strIndexHelpSuffix5, helpSuffix);
				strncpy(helpFileName, samp.name, kMaxResourceStrLen);
				strncat(helpFileName, helpSuffix, kMaxResourceStrLen);
				
				loadResult = stringload(helpFileName);
				}
			
			if (loadResult == NIL) {
				// Either we're running on Max 4 (or earlier!) or the help file is still
				// using the old .help suffix
				LitterGetIndString(strIndexHelpSuffix4, helpSuffix);
				strncpy(helpFileName, samp.name, kMaxResourceStrLen);
				strncat(helpFileName, helpSuffix, kMaxResourceStrLen);
				
				loadResult = stringload(helpFileName);
				}
			
			if (loadResult == NIL) {
				// If we still haven't found anything, we're out of luck
				error("%s: no help available for %s", kClassName, samp.name);
				}
			}
			
		else {
			loadResult = stringload(samp.name);
			if (loadResult == NIL) {
				error("%s: not found: %s", kClassName, samp.name);
				}
			}
		
		if (me->searchDir != 0) path_setdefault(oldPath, false);
		// SDK documentation for this function explicitly states that Max itself will never
		// create a hierarchical default search path. So previous "recurse" setting must be
		// false.
		}
	
	}

void MrsNorrisSet(objMrsNorris* me, Symbol* iName, long iHelp)
	{
		
	if (iName == gSymLPO) {
		// Special case. Use the Overview file Mrs. Norris found at launch time
		me->filename	= gSymOverview;
		me->searchDir	= gOverviewDir;
		me->helpSuffix	= false;
		me->recurse		= false;
		}
	
	else if (iHelp == 0) {
		// We will use standard search path and the filename as-is
		me->filename	= iName;
		me->searchDir	= 0;
		me->helpSuffix	= false;
		me->recurse		= false;			// This value is ignored in this case
		}
	
	else {
		me->filename	= iName;
		me->searchDir	= gHelpDir;
		me->helpSuffix	= true;
		me->recurse		= true;
		}
	
	}

void MrsNorrisLoad(objMrsNorris* me, Symbol* iName)
	{ MrsNorrisSet(me, iName, 0); MrsNorrisBang(me); }

void MrsNorrisHelp(objMrsNorris* me, Symbol* iName)
	{ MrsNorrisSet(me, iName, 1); MrsNorrisBang(me); }

void MrsNorrisLPO(objMrsNorris* me)
	{ MrsNorrisSet(me, gSymLPO, 0); MrsNorrisBang(me); }


/******************************************************************************************
 *
 *	MrsNorrisTattle(me)
 *	MrsNorrisAssist
 *	MrsNorrisInfo(me)
 *
 *	Post state information
 *
 ******************************************************************************************/

void
MrsNorrisTattle(
	objMrsNorris* me)
	
	{
	
	post("%s state",
			kClassName);
	post("  will open patcher named %s", 
			(me->filename != NIL) ? me->filename->s_name : "");
	post("  search in directory %ld", (long) me->searchDir);
	post("  recursive search: %s", (me->recurse) ? "ON" : "off");

	}


void MrsNorrisAssist(objMrsNorris* me, void* iBox, long iDir, long iArgNum, char* oCStr)
	{
	#pragma unused(me, iBox)
	
	LitterAssist(iDir, iArgNum, strIndexInLeft, strIndexOutLeft, oCStr);
	}

void MrsNorrisInfo(objMrsNorris* me)
	{ LitterInfo(kClassName, &me->coreObject, (method) MrsNorrisTattle); }

	
