/*	File:		y.r	Contains:	Resources for y	Written by:	Peter Castine	Copyright:	Copyright � 2000-2002 Peter Castine. All rights reserved.	Change History (most recent first):        <11>   23�3�2006    pc      Update version (expect message and other new stuff).        <10>     15�2�06    pc      Update minor revision to reflect change to MachineKharma().         <9>     10�1�06    pc      Nudge revision. Updated to use more appropriate MSL version                                    under Windows.         <8>     21�1�04    pc      Go to final status.         <7>     11�1�04    pc      Update for Rez/RC cross-platform compatibility         <6>    7�7�2003    pc      Bump revision to final. Use STR# resource for Object List                                    categories.         <5>    5�7�2003    pc      Bump revision number (fixed seed & assist string problems).         <4>    2�3�2003    pc      Bump version to Final Candidate.         <3>  30�12�2002    pc      Make vers information carbon/classic-savvy. Drop faux 'Vers'                                    resource. Bump/beta minor revision.         <2>  29�11�2002    pc      Tidy up initial check in.         <1>  29�11�2002    pc      Initial check in.*/	//	// Configuration values for this object	//	// These must be unique within Litter Package. The Names must match constant values used	// in the C/C++ source code (we try never to access resources by ID).#define LPobjID			17537#define LPobjName		"lp.y"	// 'vers' stuff we need to maintain manually//#define LPobjStarter		1							// Comment out for Pro Bundles#define LPobjMajorRev		1							// 1-99 (decimal)#define LPobjMinorRev		3							// 1-9	(decimal)#define LPobjBugFix			0							// 1-9	(decimal)#define LPobjStage			finalStage					// Apple standard stage #defines#define LPobjStageBuild		1							// 1-255 (0 for Golden Master only)#define LPobjRegion			0							// US#define LPobjVersStr		"1.3fc1"#define LPobjCRYears		"2001-08"#define	LPobjLitterCategory	"Litter RNGs"				// Litter Category#define LPobjMax3Category	"Arith/Logic/Bitwise"		// Category for Max 2.2 - 3.6x#define LPobjMax4Category	"Math"						// Category starting at Max 4	// Description string (for Windows Properties box, taken from documentation)#define LPobjDescription	"Generate random numbers from Weibull and Rayleigh distributions"	// The following sets up the 'mAxL' and 'vers' resources on Mac OS	// and VERSIONINFO resource on Windows	// It relies on the values above for resource IDs and names, as	// well as concrete values for the 'vers'/VERSIONINFO resources.#include "Litter Globals.r"	//	// -----------------------------------------	//	// Other resource definitions 	//	// Assistance strings	#define LPAssistIn1			"Bang (Generate random number)"#define LPAssistIn2			"Float (s: scaling factor)"#define LPAssistIn3			"Float (t: distribution curvature)"#define LPAssistOut1		"Float (Random value)"#ifdef RC_INVOKED					// Must be Windows RC Compiler	STRINGTABLE DISCARDABLE		BEGIN		lpStrIndexLastStandard + 1,		LPAssistIn1		lpStrIndexLastStandard + 2,		LPAssistIn2		lpStrIndexLastStandard + 3,		LPAssistIn3		lpStrIndexLastStandard + 4,		LPAssistOut1		END#else								// Must be Mac OS Resource Compiler	resource 'STR#' (LPobjID, LPobjName) {		{	/* array StringArray */			LPStdStrings,					// Standard Litter Strings						// Assist strings			LPAssistIn1,			LPAssistIn2,			LPAssistIn3,			LPAssistOut1		}	};#endif