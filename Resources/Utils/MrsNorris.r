/*
	File:		MrsNorris.r

	Contains:	Resources for Litter Power external object mrsnorris.

	Written by:	Peter Castine

	Copyright:	 © 0006 Peter Castine. All rights reserved.

	Change History (most recent first):

         <2>      8Ð6Ð06    pc      Add conditional compile to set up path names correctly for Boron
                                    and Carbon (Window, we think, will work with Carbon conventions,
                                    but need to test).
         <1>    8Ð5Ð2006    pc      first checked in.
*/


	//
	// Configuration values for this object
	//
	// These must be unique within Litter Package. The Names must match constant values used
	// in the C/C++ source code (we try never to access resources by ID).
#define LPobjID			17580
#define LPobjName		"lp.mrsnorris"

	// -----------------------------------------
	// 'vers' stuff we need to maintain manually
#define LPobjStarter		1							// Comment out for Pro Bundles
#define LPobjMajorRev		1
#define LPobjMinorRev		1
#define LPobjBugFix			0
#define LPobjStage			finalStage
#define LPobjStageBuild		1
#define LPobjRegion			0							// US
#define LPobjVersStr		"1.1fc1"
#define LPobjCRYears		"2006-08"

#define	LPobjLitterCategory	"Litter Utilities"
#define LPobjMax3Category	"System"				// Category for Max 2.2 - 3.6x
#define LPobjMax4Category	"System"				// Category starting at Max 4

	// Description string (for Windows Properties box, taken from documentation)
#define LPobjDescription	"Open patcher files or bring open windows to front"

	// -----------------------------------------


	// The following sets up the 'mAxL', 'vers', and 'Vers' resources
	// It relies on the values above for resource IDs and names, as
	// well as concrete values for the 'vers'(1) resource.
#include "Litter Globals.r"
	//
	// -----------------------------------------

	// Helper strings for cross-platform file path identification
#ifdef RC_INVOKED							// Must be Windows RC Compiler
	#define LPfilePathMaxApp	"./"
	#define LPfilePathDelim		"/"
#else										// Must be Mac OS Resource Compiler, so
	#ifdef __LP_CARBON_VERSION__			// Classic or Carbon?
		#define LPfilePathMaxApp	"./"
		#define LPfilePathDelim		"/"
	#else
		#define LPfilePathMaxApp	":"
		#define LPfilePathDelim		":"
	#endif
#endif


	// -----------------------------------------

	//
	// Other resource definitions 
	//

	// Assistance strings
#define LPAssistIn1			"Messages"
#define LPAssistOut1		"Result of vers message"
#define LPHelpPath4			LPfilePathMaxApp "max-help"
#define LPHelpPath5			LPfilePathMaxApp "Cycling '74" LPfilePathDelim "max-help"
#define LPHelpSuffix4		".help"
#define LPHelpSuffix5		".maxhelp"
#define LPExtrasSubDir		LPfilePathMaxApp "patches" LPfilePathDelim "extras" LPfilePathDelim "Litter Power Extras"
#define LPProOverview4		"Litter Pro Overview"
#define LPStarterOverview4	"Litter Starter Pack Overview"
#define LPProOverview5		LPProOverview4 ".maxpat"
#define LPStarterOverview5	LPStarterOverview4 ".maxpat"

#ifdef RC_INVOKED					// Must be Windows RC Compiler

	STRINGTABLE DISCARDABLE
		BEGIN
		lpStrIndexLastStandard + 1,		LPAssistIn1
		lpStrIndexLastStandard + 2,		LPAssistOut1
		lpStrIndexLastStandard + 3,		LPHelpPath4
		lpStrIndexLastStandard + 4,		LPHelpPath5
		lpStrIndexLastStandard + 5,		LPHelpSuffix4
		lpStrIndexLastStandard + 6,		LPHelpSuffix5
		lpStrIndexLastStandard + 7,		LPExtrasSubDir
		lpStrIndexLastStandard + 8,		LPProOverview4 ".mxb"
		lpStrIndexLastStandard + 9,		LPStarterOverview4 ".mxb"
		lpStrIndexLastStandard + 10,	LPProOverview5
		lpStrIndexLastStandard + 11,	LPStarterOverview5
		END

#else								// Must be Mac OS Resource Compiler

	resource 'STR#' (LPobjID, LPobjName) {
		{	/* array StringArray */
			LPStdStrings,					// Standard Litter Strings
			
			// Assist strings
			LPAssistIn1,					// Inlets
			LPAssistOut1,					// Outlet
			
			// Path and special file names
			LPHelpPath4, LPHelpPath5, LPHelpSuffix4, LPHelpSuffix5,
			LPExtrasSubDir,
			LPProOverview4, LPStarterOverview4, LPProOverview5, LPStarterOverview5
		}
	};

#endif

