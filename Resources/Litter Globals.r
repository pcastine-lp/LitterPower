/*
    File:       Litter Globals.r

    Contains:   Resource contents common to all Litter Objects

    Written by: Peter Castine.

    Copyright:  © 2000-2002 Peter Castine. All rights reserved.

    Change History (most recent first):

        <11>      7Ð3Ð05    pc      Prepare for next release: 1.6
        <10>     21Ð2Ð05    pc      Version 1.5.1 (added emeric and epoisse~, updated scamp~)
         <9>     21Ð1Ð04    pc      Go 1.5 Golden Master
         <8>     10Ð1Ð04    pc      Modify decimal representation of version number. Set package
                                    version to 1.5 (to reflect degree of changes since 1.2). Freeze
                                    features in core code (bar fixing beta bugs).
         <7>      8Ð1Ð04    pc      Modify for Rez/RC compatibility.
         <6>    6Ð7Ð2003    pc      Bump package 'vers' to 1.2 final (no longer a "candidate").
         <5>    9Ð4Ð2003    pc      Add constant ID for a STR# identifying Max "finder" categories.
                                    Bumped version to 1.2fc1
         <4>    7Ð3Ð2003    pc      Bump revision to Final Candidate 1
         <3>  30Ð12Ð2002    pc      Drop faux 'Vers' resource. Bump/beta minor revision.
         <2>  29Ð11Ð2002    pc      Tidy up initial check in.
         <1>  29Ð11Ð2002    pc      Initial Check-in.

*/

#ifndef __NOPACKAGE__
	#define __NOPACKAGE__ 0
#endif


	// -----------------------------------------
	// 'vers'(2) stuff we need to maintain manually
	// Should be consistent across all objects in the same release
	//
#if __NOPACKAGE__

	// NIL Settings
	#define LPMajorRev		0
	#define LPMinorRev		0
	#define LPBugFix		0
	#define LPStage			0
	#define LPStageBuild	0
	#define LPRegion		0
	#define LPShortStr		""

#else													// LBJ or Litter Power?

	#ifdef LPobjLBJ
	
		#define LPMajorRev		1						// 1-99 (decimal)
		#define LPMinorRev		2						// 1-9	(decimal)
		#define LPBugFix		0						// 1-9	(decimal)
		#define LPStage			betaStage				// Apple #defined build stage
		#define LPStageBuild	2						// 1-255 (0 for Golden Master only)
		#define LPRegion		0						// US

		#define LPShortStr		"1.2"
	
	#else

		#define LPMajorRev		1						// 1-99 (decimal)
		#define LPMinorRev		8						// 1-9	(decimal)
		#define LPBugFix		0						// 1-9	(decimal)
		#define LPStage			finalStage				// Apple #defined build stage
		#define LPStageBuild	2						// 1-255 (0 for Golden Master only)
		#define LPRegion		0						// US

		#define LPShortStr		"1.8i1"					// For 1.8 we are calling fc builds
														// "interim"
	
	#endif												// LPobjLBJ
#endif													// __NOPACKAGE__
	//
	// -----------------------------------------

	// Now some compound strings we need for all version resources
	// Check version values and compile Short strings for version resources
#define LPobjVersLongStr		LPobjVersStr " Copyright © " LPobjCRYears " Peter Castine"
#define LPobjVersFinderStr		LPobjVersStr " (" LPobjOSTarget ")"
#define LPobjVersFinderLongStr	LPobjVersStr " (" LPobjOSTarget ")"	" Copyright © " LPobjCRYears " Peter Castine"

#ifdef RC_INVOKED							// Must be Windows RC Compiler
	#define LPobjOSTarget	"Windows XP"
#else										// Must be Mac OS Resource Compiler, so...
	#ifdef __LP_CARBON_VERSION__			// Classic or Carbon?
		#define LPobjOSTarget	"OS X (Carbon)"
	#else
		#ifdef __LB_UB_VERSION__
			#define LPobjOSTarget	"Universal Binary"
		#else
			#define LPobjOSTarget	"Classic Mac OS"
		#endif
	#endif
#endif

#ifdef LPobjLBJ
	#define LPLongStr	"Litter Bundle for Jitter " LPShortStr
#else
	#ifdef LPobjStarter
		#define LPLongStr	"Litter Power Package " LPShortStr " (All Bundles)"
	#else
		#define LPLongStr	"Litter Power Package " LPShortStr " (Pro Bundles)"
	#endif
#endif
	

#ifdef RC_INVOKED			// Must be Windows RC compiler

	#include "WINVER.H"
	
	// #define Mac OS Stage macros for Windows' benefit
	#define developStage		0x20
	#define alphaStage			0x40
	#define betaStage			0x60
	#define finalStage			0x80
	
	// ...and pack Bug fix, stage, and build into a single 16-bit value
	#if (LPobjStage == finalStage && LPobjStageBuild == 0)
		#define LPobjBugStageBuild	(LPobjBugFix << 12) + 0xa000
	#else
		#define LPobjBugStageBuild	(LPobjBugFix << 12) + (LPobjStage << 4) + LPobjStageBuild
	#endif

	#if (LPStage == finalStage && LPStageBuild == 0)
		#define LPBugStageBuild	(LPBugFix << 12) + 0xa000
	#else
		#define LPBugStageBuild	(LPBugFix << 12) + (LPStage << 4) + LPStageBuild
	#endif
	
   	
	1 VERSIONINFO
	FILEVERSION		LPobjMajorRev, LPobjMinorRev,	LPobjBugStageBuild, 0
	PRODUCTVERSION	LPMajorRev,	   LPMinorRev,		LPBugStageBuild,  0
	FILEFLAGSMASK	VS_FFI_FILEFLAGSMASK
	FILEFLAGS		0x00000000L
	FILEOS			VOS_NT_WINDOWS32	// Actually XP only, but this is the official designator?
	FILETYPE		VFT_DLL
	FILESUBTYPE		VFT_UNKNOWN			// Neither official nor unofficial subtypes for Max externals
		BEGIN
		BLOCK "StringFileInfo"
			BEGIN
			BLOCK "040904E4"			// US English, Windows character set
				BEGIN
				VALUE "FileDescription",		LPobjName " external object for MaxMSP"
				VALUE "LegalCopyright",			"251 " LPobjCRYears " Peter Castine"
				VALUE "FileVersion",			LPobjVersStr
			
			#ifndef __NOPACKAGE__
				VALUE "Package Version",		LPLongStr
			#endif
			
				VALUE "Product Description",	LPobjDescription
				VALUE "CompanyName",			"4-15 Music & Technology"
				VALUE "Contact e-mail",			"4-15@kagi.com"
				END
			END
		END

	// Define indices for use in Windows STRINGTABLE resource
	// The matching Rez 'STR#' resource does not need these indices,
	// but we *do* define an enum with these values in LitterLib.c
	// ?? I would like to find a way to maintain these #defines and
	//		the associated enum at the same time and in the same file

	#define lpStrIndexObjectVersShort		1
	#define lpStrIndexObjectVersLong		2
	#define lpStrIndexPackageVersShort		3
	#define lpStrIndexPackageVersLong		4
	#define lpStrIndexLitterCategory		5
	#define lpStrIndexMax4Category			6
	#define lpStrIndexLastStandard			lpStrIndexMax4Category
	
	STRINGTABLE DISCARDABLE
		BEGIN
		lpStrIndexObjectVersShort,		LPobjVersStr
		lpStrIndexObjectVersLong,		LPobjVersLongStr
		lpStrIndexPackageVersShort,		LPShortStr
		lpStrIndexPackageVersLong,		LPLongStr
		lpStrIndexLitterCategory,		LPobjLitterCategory
		lpStrIndexMax4Category,			LPobjMax4Category
		END

#else						// Must be Mac OS Rez compiler

	#ifndef __LB_UB_VERSION__
		// LPobjID and LPobjName must be #defined in the .r file
		// that #includes this file
	data 'mAxL' (LPobjID, LPobjName) {
		$"600A 0000 6D41 784C 0C42 0000 4EFA 0030 4E75 225F 201F 2E80 6712 2040 7000 1010"                    /* `Â..mAxL.B..Nú.0Nu"_ ..€g. @p... */
		$"6004 10E8 0001 51C8 FFFA 4210 4ED1 594F 2F2F 0008 4EBA FFDC 205F 2008 4E75 4E56"                    /* `..è..QÈÿúB.NÑYO//..NºÿÜ _ .NuNV */
		$"FEFA 48E7 0030 266E 0008 594F 2F3C 6D41 784C 3F3C 0001 A80E 205F 2448 200A 672E"                    /* þúHç.0&n..YO/<mAxL?<..¨. _$H Âg. */
		$"2F0A 486E FEFA 486E FEFC 486E FF00 A9A8 486E FF00 4EBA FFB8 486E FF00 41FA 002E"                    /* /ÂHnþúHnþüHnÿ.©¨Hnÿ.Nºÿ¸Hnÿ.Aú.. */
		$"4850 206B 0090 4E90 4FEF 000C 600E 41FA 0036 4850 206B 0090 4E90 584F 4CDF 0C00"                    /* HP k.NOï..`.Aú.6HP k.NXOLß.. */
		$"4E5E 4E75 846D 6169 6E00 0040 2573 3A20 6E6F 7420 6176 6169 6C61 626C 6520 666F"                    /* N^Nu„main..@%s: not available fo */
		$"7220 3638 4B00 7468 6973 206F 626A 6563 7420 6973 206E 6F74 2061 7661 696C 6162"                    /* r 68K.this object is not availab */
		$"6C65 2066 6F72 2036 384B 0000"                                                                      /* le for 68K.. */
		};
	#endif

	// Resource ID to be used in all Litter objects for strings identifying the category
	// to be used in the Max New Object list.
	// This ID is only used in an external's main() (ie, at class initialization time), so
	// we use the same ID for all objects.
#define LPcatID			128

	resource 'STR#' (LPcatID) {
		{	/* array StringArray */
			LPobjLitterCategory,				// Litter category
			LPobjMax3Category,					// Category for Max 2.2 - 3.6x
			LPobjMax4Category					// Category starting at Max 4
		}
	};

	resource 'vers' (1) {
		(LPobjMajorRev / 10) * 16 + LPobjMajorRev % 10,		// Major revision in BCD
		LPobjMinorRev * 16 + LPobjBugFix,					// Merge Minor rev. and Bug Fix
		LPobjStage,
		LPobjStageBuild,
		LPobjRegion,
		LPobjVersFinderStr, LPobjVersFinderLongStr
		};

#if (__NOPACKAGE__ == 0)
	resource 'vers' (2) {
		(LPMajorRev / 10) * 16 + LPMajorRev % 10,			// Major revision in BCD
		LPMinorRev * 16 + LPBugFix,							// Merge Minor rev. and Bug Fix
		LPStage,
		LPStageBuild,
		LPRegion,
		LPShortStr, LPLongStr
	};
#endif

	// Prepare values for use in the 'STR#' resource (must be defined in the calling file)
	#if (LPobjStage == finalStage && LPobjStageBuild == 0)
	   	#define LPobjStageAndBuild	10000
	#else
	   	#define LPobjStageAndBuild (LPobjStage >> 4) * 1000 + LPobjStageBuild
	#endif
	#define LPobjVersVal	  LPobjMajorRev		* 10000000		\
							+ LPobjMinorRev		*  1000000		\
							+ LPobjBugFix		*    10000		\
							+ LPobjStageAndBuild

	#if (LPStage == finalStage && LPStageBuild == 0)
	   	#define LPStageAndBuild		10000
	#else
	   	#define LPStageAndBuild (LPStage >> 4) * 1000 + LPStageBuild
	#endif

	#define LPVersVal		  LPMajorRev	 	*  10000000		\
							+ LPMinorRev		*   1000000		\
							+ LPBugFix			*     10000		\
							+ LPStageAndBuild

	#define LPobjVersValStr	$$Format("%d", LPobjVersVal)
	#define LPVersValStr	$$Format("%d", LPVersVal)
	#define LPStdStrings 	LPobjVersStr,						\
							LPobjVersLongStr,					\
							LPobjVersValStr,					\
							LPShortStr,							\
							LPLongStr, 							\
							LPVersValStr
	
#endif


