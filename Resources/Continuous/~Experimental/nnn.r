/*	File:		nnn.r	Contains:	Resources for nnn	Written by:	Peter Castine	Copyright:	Copyright � 2000-2002 Peter Castine. All rights reserved.	Change History (most recent first):         <2>  29�11�2002    pc      Tidy up initial check in.         <1>  29�11�2002    pc      Initial check in.*/	//	// Configuration values for this object	//	// These must be unique within Litter Package. The Names must match constant values used	// in the C/C++ source code (we try never to access resources by ID).#define LPobjID			17541#define LPobjID2		29648		#define LPobjName		"lp.knnn"#define LPobjName2		"lp.knnn Package"	// Values used in the 'vers'(1) and 'Vers'(LPobjID)//#define LPobjStarter		1								// Comment out for Pro Bundles#define LPobjMajorRev		0x01#define LPobjMinorRev		0x00#define LPobjStage			development#define LPobjStageBuild		0x01#define LPobjRegion			0								// US#define LPobjShortStr		"1.0d1"#define LPobjLongStr		"1.0d1, Copyright � 2001-2002 Peter Castine."	// The following sets up the 'mAxL', 'vers', and 'Vers' resources	// It relies on the values above for resource IDs and names, as	// well as concrete values for the 'vers'(1) resource.#include "Litter Globals.r"	//	// Resource definitions 	//resource 'STR#' (, "lp.nnn") {	{	/* array StringArray */		"Bang",		"Float (Hurst Factor)",		"Int (NN factor)",		"Float (random value in [0 .. 1])"	}};