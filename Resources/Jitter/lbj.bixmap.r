/*
	File:		lbj.bixmap.r

	Contains:	Resources for the lbj.bixmap object

	Written by:	Peter Castine

	Copyright:	Copyright © 2005 Peter Castine. All rights reserved.

	Change History (most recent first):

         <2>   12Ð8Ð2005    pc      Update version (now final); add option to build outside of the
                                    Litter family (ie, as the previous jit.bixcp.to object)
         <1>      5Ð3Ð05    pc      Initial check in.
*/


	//
	// Configuration values for this object
	//
	// These must be unique within Litter Package. The Names must match constant values used
	// in the C/C++ source code (we try never to access resources by ID).
#define LPobjID			17593
#define LPobjName		"lbj.bixmap"

	// 'vers' stuff we need to maintain manually
#define LPobjLBJ			1

#define LPobjMajorRev		1
#define LPobjMinorRev		0
#define LPobjBugFix			0
#define LPobjStage			betaStage
#define LPobjStageBuild		2
#define LPobjRegion			0							// US
#define LPobjVersStr		"1.0b2"
#define LPobjCRYears		"2003-08"

#define	LPobjLitterCategory	"Litter Bundle Jitter"
#define LPobjMax3Category	""							// Not available in Max 2.2 - 3.6x
#define LPobjMax4Category	"Jitter Data"				// Category starting at Max 4

	// Description string (for Windows Properties box, taken from documentation)
#define LPobjDescription	"BIX input-to-output mapping"



	// The following sets up the 'mAxL' and 'vers' resources on Mac OS
	// and VERSIONINFO resource on Windows
	// It relies on the values above for resource IDs and names, as
	// well as concrete values for the 'vers'/VERSIONINFO resources.
#include "Litter Globals.r"
	//
	// -----------------------------------------


	//
	// Other resource definitions 
	//

	// Assistance strings
	
#define LPAssistIn1			"Jitter Matrix input"
#define LPAssistIn2			"16-Plane Matrix (Filter Group Membership)"
#define LPAssistIn3			"Lists of Filter values"
#define LPAssistOut1		"Jitter Matrix output"
#define LPAssistOut2		"Dump"

#ifdef RC_INVOKED					// Must be Windows RC Compiler

	STRINGTABLE DISCARDABLE
		BEGIN
		lpStrIndexLastStandard + 1,		LPAssistIn1
		lpStrIndexLastStandard + 2,		LPAssistIn2
		lpStrIndexLastStandard + 3,		LPAssistIn3
		lpStrIndexLastStandard + 4,		LPAssistOut1
		lpStrIndexLastStandard + 5,		LPAssistOut2
		END

#else								// Must be Mac OS Resource Compiler

	resource 'STR#' (LPobjID, LPobjName) {
		{	/* array StringArray */
			LPStdStrings,					// Standard Litter Strings
			
			// Assist strings
			LPAssistIn1, LPAssistIn2, LPAssistIn3,
			LPAssistOut1, LPAssistOut2
		}
	};

#endif
