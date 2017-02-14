/*	File:		ksks~.r	Contains:	Resources for Max/MSP external object creating "plucked noise".	Written by:	Peter Castine	Copyright:	 � 2004 Peter Castine. All rights reserved.	Change History (most recent first):         <1>     5�11�04    pc      first checked in.*/	//	// Configuration values for this object	//	// These must be unique within Litter Package. The Names must match constant values used	// in the C/C++ source code (we try never to access resources by ID).#define LPobjID			17570#define LPobjName		"lp.ksks~"	// -----------------------------------------	// 'vers' stuff we need to maintain manually#define LPobjStarter		1							// Comment out for Pro Bundles#define LPobjMajorRev		0							// 1-99	(decimal)#define LPobjMinorRev		0							// 0-9	(decimal)#define LPobjBugFix			0							// 0-9	(deicmal)#define LPobjStage			alphaStage					// Standard Apple stage#define LPobjStageBuild		1							// 1-255 (0 for Golden Master only)#define LPobjRegion			0							// US#define LPobjVersStr		"1.0a1"#define LPobjCRYears		"2004"#define	LPobjLitterCategory	"Litter Signals"			// Litter Category#define LPobjMax3Category	"MSP"						// Category for Max 2.2 - 3.6x#define LPobjMax4Category	"MSP Synthesis"				// Category starting at Max 4	// Description string (for Windows Properties box, taken from documentation)#define LPobjDescription	"Plucked noise"	// The following sets up the 'mAxL' and 'vers' resources	// It relies on the values above for resource IDs and names, as	// well as concrete values for the 'vers'(1) resource.#include "Litter Globals.r"	//	// -----------------------------------------	//	// Other resource definitions 	//	// Assistance strings#define LPAssistIn1			"Bang or signal (trigger)"#define LPAssistIn2			"Signal or float (frequency [Hz])"#define LPAssistIn3			"Signal or float (decay time [ms])"#define LPAssistIn4			"Signal or float (blend factor, in [-1 .. 1])"#define LPAssistOut1		"Signal (Plucked noise)"#ifdef RC_INVOKED					// Must be Windows RC Compiler	STRINGTABLE DISCARDABLE		BEGIN		lpStrIndexLastStandard + 1,		LPAssistIn1		lpStrIndexLastStandard + 2,		LPAssistIn2		lpStrIndexLastStandard + 3,		LPAssistIn3		lpStrIndexLastStandard + 4,		LPAssistIn4		lpStrIndexLastStandard + 5,		LPAssistOut1		END#else								// Must be Mac OS Resource Compiler	resource 'STR#' (LPobjID, LPobjName) {		{	/* array StringArray */			LPStdStrings,						// Standard Litter Strings						// Assist strings			LPAssistIn1, LPAssistIn2, LPAssistIn3, LPAssistIn4,	// Inlets			LPAssistOut1										// Outlets		}	};#endif