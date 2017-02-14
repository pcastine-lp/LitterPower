/*	File:		alea.r	Contains:	Resources for Max/MSP external object lp.crabelms.	Written by:	Peter Castine	Copyright:	 � 2006 Peter Castine. All rights reserved.	Change History (most recent first):*//****************************************************************************************** ******************************************************************************************/	//	// Configuration values for this object	//	// These must be unique within Litter Package. The Names must match constant values used	// in the C/C++ source code (we try never to access resources by ID).#define LPobjID			17593#define LPobjName		"lp.ale"	// -----------------------------------------	// 'vers'  and String stuff we need to maintain manually#define LPobjStarter		1								// Comment out for Pro Bundles#define LPobjMajorRev		1#define LPobjMinorRev		1#define LPobjBugFix			0#define LPobjStage			finalStage#define LPobjStageBuild		1#define LPobjRegion			0								// US#define LPobjVersStr		"1.1fc1"#define LPobjCRYears		"2006-08"#define	LPobjLitterCategory	"Litter Utilities"#define LPobjMax3Category	"Data"						// Category for Max 2.2 - 3.6x, N/A#define LPobjMax4Category	"Data"						// Category starting at Max 4	// Description string (for Windows Properties box, taken from documentation)#define LPobjDescription	"Scramble a list"	// The following sets up the 'mAxL', 'vers', and 'Vers' resources	// It relies on the values above for resource IDs and names, as	// well as concrete values for the 'vers'(1) resource.#include "Litter Globals.r"	//	// -----------------------------------------	//	// Other resource definitions 	//		// Assistance strings#define LPAssistIn		"List, Bang, Set"#define LPAssistOut		"An element taken at random from the input list"#ifdef RC_INVOKED					// Must be Windows RC Compiler	STRINGTABLE DISCARDABLE		BEGIN		lpStrIndexLastStandard + 1,		LPAssistIn		lpStrIndexLastStandard + 2,		LPAssistOut		END#else								// Must be Mac OS Resource Compiler	resource 'STR#' (LPobjID, LPobjName) {		{	/* array StringArray */			LPStdStrings,					// Standard Litter Strings						// Assist strings			LPAssistIn,						// Inlets			LPAssistOut						// Outlets		}	};#endif