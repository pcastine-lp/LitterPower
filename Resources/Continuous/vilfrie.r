/*    File:       vilfrie.r    Contains:   Resources for vilfrie    Written by: Peter Castine    Copyright:  � 2006 Peter Castine All rights reserved.    Change History (most recent first):         <1>   23�3�2006    pc      first checked in.*//****************************************************************************************** ******************************************************************************************/	//	// Configuration values for this object	//	// These must be unique within Litter Package. The Names must match constant values used	// in the C/C++ source code (we try never to access resources by ID).#define LPobjID			17580#define LPobjName		"lp.vilfrie"	// 'vers' stuff we need to maintain manually//#define LPobjStarter		1							// Comment out for Pro Bundles#define LPobjMajorRev		1							// 1-99 (decimal)#define LPobjMinorRev		1							// 1-9	(decimal)#define LPobjBugFix			0							// 1-9	(decimal)#define LPobjStage			finalStage					// Apple standard stage #defines#define LPobjStageBuild		1							// 1-255 (0 for Golden Master only)#define LPobjRegion			0							// US#define LPobjVersStr		"1.1fc1"#define LPobjCRYears		"2006-08"#define	LPobjLitterCategory	"Litter RNGs"				// Litter Category#define LPobjMax3Category	"Arith/Logic/Bitwise"		// Category for Max 2.2 - 3.6x#define LPobjMax4Category	"Math"						// Category starting at Max 4	// Description string (for Windows Properties box, taken from documentation)#define LPobjDescription	"Generate random numbers from a Pareto distribution"	// The following sets up the 'mAxL' and 'vers' resources on Mac OS	// and VERSIONINFO resource on Windows	// It relies on the values above for resource IDs and names, as	// well as concrete values for the 'vers'/VERSIONINFO resources.#include "Litter Globals.r"	//	// -----------------------------------------	//	// Other resource definitions 	//	// Assistance strings	#define LPAssistIn1			"Bang (Generate random number)"#define LPAssistIn2			"Float (alpha >0: shape parameter)"#define LPAssistIn3			"Float (beta >0: location parameter)"#define LPAssistOut1		"Float (Random value)"#ifdef RC_INVOKED					// Must be Windows RC Compiler	STRINGTABLE DISCARDABLE		BEGIN		lpStrIndexLastStandard + 1,		LPAssistIn1		lpStrIndexLastStandard + 2,		LPAssistIn2		lpStrIndexLastStandard + 3,		LPAssistIn3		lpStrIndexLastStandard + 4,		LPAssistOut1		END#else								// Must be Mac OS Resource Compiler	resource 'STR#' (LPobjID, LPobjName) {		{	/* array StringArray */			LPStdStrings,					// Standard Litter Strings						// Assist strings			LPAssistIn1, LPAssistIn2, LPAssistIn3,			LPAssistOut1		}	};#endif