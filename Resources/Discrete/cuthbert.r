/*
	File:		cuthbert.r

	Contains:	Resources for ernie

	Written by:	Peter Castine

	Copyright:	 © 2000-2008 Peter Castine. All rights reserved.

	Change History (most recent first):

*/


	//
	// Configuration values for this object
	//
	// These must be unique within Litter Package. The Names must match constant values used
	// in the C/C++ source code (we try never to access resources by ID).
#define LPobjID			17591
#define LPobjName		"lp.cuthbert"

	// -----------------------------------------
	// 'vers' stuff we need to maintain manually
//#define LPobjStarter		1							// Comment out for Pro Bundles

#define LPobjMajorRev		1							// 1-99	(decimal)
#define LPobjMinorRev		0							// 1-9	(decimal)
#define LPobjBugFix			0							// 0-9	(decimal)
#define LPobjStage			developStage				// Apple standard stage #defines
#define LPobjStageBuild		2							// 1-255 (0 for Golden Master only)
#define LPobjRegion			0							// US
#define LPobjVersStr		"1.0.d2"
#define LPobjCRYears		"2008"

#define	LPobjLitterCategory	"Litter RNGs"				// Litter category
#define LPobjMax3Category	"Arith/Logic/Bitwise"		// Category for Max 2.2 - 3.6x
#define LPobjMax4Category	"Math"						// Category starting at Max 4

	// Description string (for Windows Properties box, taken from documentation)
#define LPobjDescription	"Random values without repetition of recent values"

	// The following sets up the 'mAxL' and 'vers' resources
	// It relies on the values above for resource IDs and names, as
	// well as concrete values for the 'vers'(1) resource.
#include "Litter Globals.r"
	//
	// -----------------------------------------


	//
	// Other resource definitions 
	//

	// Assistance strings
#define LPAssistIn1			"Bang (Generate random value)"
#define LPAssistIn2			"Int (Extent of memory)"
#define LPAssistOut1		"Int (Random value)"

#ifdef RC_INVOKED					// Must be Windows RC Compiler

	STRINGTABLE DISCARDABLE
		BEGIN
		lpStrIndexLastStandard + 1,		LPAssistIn1
		lpStrIndexLastStandard + 2,		LPAssistIn2
		lpStrIndexLastStandard + 3,		LPAssistOut1
		END

#else								// Must be Mac OS Resource Compiler

	resource 'STR#' (LPobjID, LPobjName) {
		{	/* array StringArray: 4 elements */
			LPStdStrings,								// Standard Litter Strings
			
			// Assist strings
			LPAssistIn1, LPAssistIn2,					// Inlets
			LPAssistOut1								// Outlet
		}
	};

#endif

