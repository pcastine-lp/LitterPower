/*	File:		feta~.r	Contains:	Resources for Max/MSP external object feta~ (1-bit noise).	Written by:	Peter Castine	Copyright:	 � 2004 Peter Castine. All rights reserved.	Change History (most recent first):         <2>     5�11�04    pc      Change default amplitude to 1/sqrt(3), to produce the same power                                    as a plain-vanilla white noise object.         <1>    14�10�04    pc      first checked in.*/	//	// Configuration values for this object	//	// These must be unique within Litter Package. The Names must match constant values used	// in the C/C++ source code (we try never to access resources by ID).#define LPobjID			17571#define LPobjName		"lp.feta~"	// -----------------------------------------	// 'vers' stuff we need to maintain manually#define LPobjStarter		1							// Comment out for Pro Bundles#define LPobjMajorRev		1							// 1-99	(decimal)#define LPobjMinorRev		2							// 0-9	(decimal)#define LPobjBugFix			0							// 0-9	(deicmal)#define LPobjStage			finalStage					// Standard Apple stage#define LPobjStageBuild		1							// 1-255 (0 for Golden Master only)#define LPobjRegion			0							// US#define LPobjVersStr		"1.2fc1"#define LPobjCRYears		"2001-04"#define	LPobjLitterCategory	"Litter Signals"			// Litter Category#define LPobjMax3Category	"MSP"						// Category for Max 2.2 - 3.6x#define LPobjMax4Category	"MSP Synthesis"				// Category starting at Max 4	// Description string (for Windows Properties box, taken from documentation)#define LPobjDescription	"Single-bit white noise"	// The following sets up the 'mAxL' and 'vers' resources	// It relies on the values above for resource IDs and names, as	// well as concrete values for the 'vers'(1) resource.#include "Litter Globals.r"	//	// -----------------------------------------	//	// Other resource definitions 	//	// Assistance strings#define LPAssistIn1			"Signal or float (amplitude)"#define LPAssistOut1		"Signal (One-bit noise)"#ifdef RC_INVOKED					// Must be Windows RC Compiler	STRINGTABLE DISCARDABLE		BEGIN		lpStrIndexLastStandard + 1,		LPAssistIn1		lpStrIndexLastStandard + 2,		LPAssistOut1		END#else								// Must be Mac OS Resource Compiler	resource 'STR#' (LPobjID, LPobjName) {		{	/* array StringArray */			LPStdStrings,						// Standard Litter Strings						// Assist strings			LPAssistIn1,						// Inlets			LPAssistOut1						// Outlets		}	};#endif