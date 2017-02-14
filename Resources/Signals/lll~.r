/*	File:		lll~.r	Contains:	Resources for Max/MSP external object lll~.	Written by:	Peter Castine	Copyright:	 � 2000-2002 Peter Castine. All rights reserved.	Change History (most recent first):         <6>     21�1�04    pc      Go to final status.         <5>     14�1�04    pc      Modify for Rez/RC compatibility         <4>    6�7�2003    pc      Bump revision to final. Use STR# resource for Object List                                    categories.         <3>  30�12�2002    pc      Make vers information carbon/classic-savvy. Drop faux 'Vers'                                    resource. Bump/beta minor revision.         <2>  29�11�2002    pc      Tidy up initial check in.         <1>  29�11�2002    pc      Initial check in.*/	//	// Configuration values for this object	//	// These must be unique within Litter Package. The Names must match constant values used	// in the C/C++ source code (we try never to access resources by ID).#define LPobjID			17528#define LPobjName		"lp.lll~"	// -----------------------------------------	// 'vers' stuff we need to maintain manually//#define LPobjStarter		1							// Comment out for Pro Bundles#define LPobjMajorRev		1							// 1-99	(decimal)#define LPobjMinorRev		2							// 0-9	(decimal)#define LPobjBugFix			0							// 0-9	(deicmal)#define LPobjStage			finalStage					// Standard Apple stage#define LPobjStageBuild		1							// 1-255 (0 for Golden Master only)#define LPobjRegion			0							// US#define LPobjVersStr		"1.2fc1"#define LPobjCRYears		"2001-08"#define	LPobjLitterCategory	"Litter Signals"			// Litter Category#define LPobjMax3Category	"MSP"						// Category for Max 2.2 - 3.6x#define LPobjMax4Category	"MSP Synthesis"				// Category starting at Max 4	// Description string (for Windows Properties box, taken from documentation)#define LPobjDescription	"Trinangular (dither) noise"	// The following sets up the 'mAxL' and 'vers' resources	// It relies on the values above for resource IDs and names, as	// well as concrete values for the 'vers'(1) resource.#include "Litter Globals.r"	//	// -----------------------------------------	//	// Other resource definitions 	//	// Assistance strings#define LPAssistIn1			"Int (Seed)"#define LPAssistIn2			"Int (Multiplier)"#define LPAssistIn3			"Int (Adder)"#define LPAssistIn4			"Int (Modulo)"#define LPAssistOut1		"Signal (Triangular noise)"#ifdef RC_INVOKED					// Must be Windows RC Compiler	STRINGTABLE DISCARDABLE		BEGIN		lpStrIndexLastStandard + 1,		LPAssistIn1		lpStrIndexLastStandard + 2,		LPAssistIn2		lpStrIndexLastStandard + 3,		LPAssistIn3		lpStrIndexLastStandard + 4,		LPAssistIn4		lpStrIndexLastStandard + 5,		LPAssistOut1		END#else								// Must be Mac OS Resource Compiler	resource 'STR#' (LPobjID, LPobjName) {		{	/* array StringArray: 4 elements */			LPStdStrings,						// Standard Litter Strings						// Assist strings			LPAssistIn1, LPAssistIn2,			// Inlets			LPAssistIn3, LPAssistIn4,			LPAssistOut1						// Outlets		}	};#endif