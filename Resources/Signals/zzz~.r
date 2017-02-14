/*
	File:		zzz~.r

	Contains:	Resources for Max/MSP external object zzz~.

	Written by:	Peter Castine

	Copyright:	 © 2000-2002 Peter Castine. All rights reserved.

	Change History (most recent first):

         <7>     10Ð2Ð06    pc      Update "bug fix" to reflect updated MachineKharma()
         <6>     21Ð1Ð04    pc      Go to final status.
         <5>     14Ð1Ð04    pc      Modify for Rez/RC compatibility
         <4>    6Ð7Ð2003    pc      Bump revision to final. Use STR# resource for Object List
                                    categories.
         <3>  30Ð12Ð2002    pc      Make vers information carbon/classic-savvy. Drop faux 'Vers'
                                    resource. Bump/beta minor revision.
         <2>  29Ð11Ð2002    pc      Tidy up initial check in.
         <1>  29Ð11Ð2002    pc      Initial check in.
*/


	//
	// Configuration values for this object
	//
	// These must be unique within Litter Package. The Names must match constant values used
	// in the C/C++ source code (we try never to access resources by ID).
#define LPobjID			17539
#define LPobjName		"lp.zzz~"

	// -----------------------------------------
	// 'vers' stuff we need to maintain manually
//#define LPobjStarter		1							// Comment out for Pro Bundles
#define LPobjMajorRev		1							// 1-99	(decimal)
#define LPobjMinorRev		2							// 0-9	(decimal)
#define LPobjBugFix			0							// 0-9	(decimal)
#define LPobjStage			finalStage					// Standard Apple stage
#define LPobjStageBuild		1							// 1-255 (0 for Golden Master only)
#define LPobjRegion			0							// US
#define LPobjVersStr		"1.2fc1"
#define LPobjCRYears		"2001-08"

#define	LPobjLitterCategory	"Litter Signals"			// Litter Category
#define LPobjMax3Category	"MSP"						// Category for Max 2.2 - 3.6x
#define LPobjMax4Category	"MSP Synthesis"				// Category starting at Max 4

	// Description string (for Windows Properties box, taken from documentation)
#define LPobjDescription	"""Pink"" (1/f^2) noise (McCartney algorithm)"

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
#define LPAssistIn1			"Int (NN factor)"
#define LPAssistOut1		"Signal (Pink noise)"

#ifdef RC_INVOKED					// Must be Windows RC Compiler

	STRINGTABLE DISCARDABLE
		BEGIN
		lpStrIndexLastStandard + 1,		LPAssistIn1
		lpStrIndexLastStandard + 2,		LPAssistOut1
		END

#else								// Must be Mac OS Resource Compiler

	resource 'STR#' (LPobjID, LPobjName) {
		{	/* array StringArray */
			LPStdStrings,						// Standard Litter Strings
			
			// Assist strings
			LPAssistIn1,						// Inlets
			LPAssistOut1						// Outlets
		}
	};

#endif


