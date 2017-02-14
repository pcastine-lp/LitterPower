/*	File:		vera+.r	Contains:	Resources for vera+	Written by:	Peter Castine	Copyright:	� 2005 Peter Castine. All rights reserved.	Change History (most recent first):         <1>   23�3�2006    pc      first checked in.*/	//	// Configuration values for this object	//	// These must be unique within Litter Package. The Names must match constant values used	// in the C/C++ source code (we try never to access resources by ID).#define LPobjID			17552#define LPobjID2		29658#define LPobjName		"lp.vera+"#define LPobjName2		"lp.vera+ Package"	// Values used in the 'vers'(1) and 'Vers'(LPobjID)//#define LPobjStarter		1								// Comment out for Pro Bundles#define LPobjMajorRev		0x01#define LPobjMinorRev		0x00#define LPobjStage			development#define LPobjStageBuild		0x01#define LPobjRegion			0								// US#define LPobjShortStr		"1.0.d1"#define LPobjLongStr		"1.0.d1, Copyright � 2002 Peter Castine."	// The following sets up the 'mAxL', 'vers', and 'Vers' resources	// It relies on the values above for resource IDs and names, as	// well as concrete values for the 'vers'(1) resource.#include "Litter Globals.r"	//	// Resource definitions 	//resource 'STR#' (LPobjID, LPobjName) {	{	/* array StringArray */		/* Inlets */		"Bang (Generate next point), Float/List (Growth rates), Symbol (Function)",		"Float (Seed population)",				/* Outlets */		"Float (Next population)"	}};