/*
	File:		mama.r

	Contains:	Resources for mama object

	Written by:	Peter Castine

	Copyright:	 © 2004 Peter Castine. All rights reserved.

	Change History (most recent first):

         <2>     10Ð2Ð06    pc      Go final
         <1>     10Ð2Ð06    pc      first checked in.
*/


	//
	// Configuration values for this object
	//
	// These must be unique within Litter Package. The Names must match constant values
	// used in the C/C++ source code (we try never to access resources by ID).
#define LPobjID			17577
#define LPobjName		"lp.mama"

	// -----------------------------------------
	// 'vers' stuff we need to maintain manually
#define LPobjStarter		1							// Comment out for Pro Bundles
#define LPobjMajorRev		1
#define LPobjMinorRev		1
#define LPobjBugFix			0
#define LPobjStage			finalStage
#define LPobjStageBuild		1
#define LPobjRegion			0							// US
#define LPobjVersStr		"1.fc1"
#define LPobjCRYears		"2006-08"

#define	LPobjLitterCategory	"Litter RNGs"				// Litter category
#define LPobjMax3Category	"Arith/Logic/Bitwise"		// Category for Max 2.2 - 3.6x
#define LPobjMax4Category	"Math"						// Category starting at Max 4

	// Description string (for Windows Properties box, taken from documentation)
#define LPobjDescription	"Generate random numbers using the Marsaglia \"Mother of All Random Number Generators\" algorithm"

	// The following sets up the 'mAxL' and 'vers' resources
	// It relies on the values above for resource IDs and names, as
	// well as concrete values for the 'vers'(1) resource.
#include "Litter Globals.r"
	//
	// -----------------------------------------


	//
	// Other resource definitions 
	//
	#ifdef RC_INVOKED
		#define LPlessEq		"<="
	#else
		#define LPlessEq		"²"
	#endif

	// Assistance strings
#define LPAssistIn1			"Bang (Generate random number)"
#define LPAssistIn2			"Int (Minimum)"
#define LPAssistIn3			"Int (Maximum)"
#define LPAssistOut1		"Int (Random value, %ld " LPlessEq " x " LPlessEq " %ld)"

#ifdef RC_INVOKED					// Must be Windows RC Compiler

	STRINGTABLE DISCARDABLE
		BEGIN
		lpStrIndexLastStandard + 1,		LPAssistIn1
		lpStrIndexLastStandard + 2,		LPAssistIn2
		lpStrIndexLastStandard + 3,		LPAssistIn3
		lpStrIndexLastStandard + 4,		LPAssistOut1
		END

#else								// Must be Mac OS Resource Compiler

	resource 'STR#' (LPobjID, LPobjName) {
		{	/* array StringArray: 4 elements */
			LPStdStrings,								// Standard Litter Strings
			
			// Assist strings
			LPAssistIn1, LPAssistIn2, LPAssistIn3,		// Inlets
			LPAssistOut1								// Outlets
		}
	};

#endif

