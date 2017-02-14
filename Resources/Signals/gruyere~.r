/*	File:		gruy�re~.r	Contains:	Resources for Max/MSP external object gruy�re~ (Markov chain noise).	Written by:	Peter Castine	Copyright:	� 2004 Peter Castine. All rights reserved.	Change History (most recent first):         <1>     9�12�04    pc      first checked in.*/	//	// Configuration values for this object	//	// These must be unique within Litter Package. The Names must match constant values used	// in the C/C++ source code (we try never to access resources by ID).#define LPobjID			17573#define LPobjName		"lp.gruy�re~"	// -----------------------------------------	// 'vers' stuff we need to maintain manually//#define LPobjStarter		1							// Comment out for Pro Bundles#define LPobjMajorRev		1							// 1-99	(decimal)#define LPobjMinorRev		1							// 0-9	(decimal)#define LPobjBugFix			0							// 0-9	(deicmal)#define LPobjStage			betaStage					// Standard Apple stage#define LPobjStageBuild		1							// 1-255 (0 for Golden Master only)#define LPobjRegion			0							// US#define LPobjVersStr		"1.0b1"#define LPobjCRYears		"2001-04"#define	LPobjLitterCategory	"Litter Signals"			// Litter Category#define LPobjMax3Category	"MSP"						// Category for Max 2.2 - 3.6x#define LPobjMax4Category	"MSP Synthesis"				// Category starting at Max 4	// Description string (for Windows Properties box, taken from documentation)#define LPobjDescription	"Uniform Markov noise"	// The following sets up the 'mAxL' and 'vers' resources	// It relies on the values above for resource IDs and names, as	// well as concrete values for the 'vers'(1) resource.#include "Litter Globals.r"	//	// -----------------------------------------	//	// Other resource definitions 	//	// Assistance strings#define LPAssistIn1			"Signal or number (frequency)"#define LPAssistIn2			"Signal or number (epsilon: phase shift probability)"#define LPAssistOut1		"Signal (Uniform Markov noise)"#ifdef RC_INVOKED					// Must be Windows RC Compiler	STRINGTABLE DISCARDABLE		BEGIN		lpStrIndexLastStandard + 1,		LPAssistIn1		lpStrIndexLastStandard + 2,		LPAssistIn2		lpStrIndexLastStandard + 3,		LPAssistOut1		END#else								// Must be Mac OS Resource Compiler	resource 'STR#' (LPobjID, LPobjName) {		{	/* array StringArray */			LPStdStrings,								// Standard Litter Strings						// Assist strings			LPAssistIn1, LPAssistIn2,					// Inlets			LPAssistOut1								// Outlets		}	};#endif