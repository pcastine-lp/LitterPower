/*
	File:		mystes.c

	Contains:	Get a machine-dependent unique ID.

	Written by:	Peter Castine

	Copyright:	© 2001-2002 Peter Castine

	Change History (most recent first):

         <1>    8–5–2006    pc      First check in.
         <2>     26–2–05    pc      It turns out UuidCreateSequential() is senstive to whether the
                                    ethernet card[s] are connected to a network at boot time. Let's
                                    try reading a serial number from the C: disk instead.
         <1>    18–10–04    pc      first checked in.
*/


/******************************************************************************************
 ******************************************************************************************/

	// Delete the following line (or #define to 1) if we ever want to include this in
	// Litter Power
	// {11.11.09 pc} Changed to use the full Litter Library (by #defining __LITTER_OBJECT__ to 1)
	//				 as the simplest way to provide Max 5 compatibility
#define __LITTER_OBJECT__	1


#pragma mark • Include Files

#include "LitterLib.h"
#include "trackdAPI_c.h"

#pragma mark • Constants

const char*	kClassName		= "vicon.controller";			// Class name


	// Indices for STR# resource
enum {
	strIndexTheInlet		= lpStrIndexLastStandard + 1,
	
	strIndexTheOutlet,
	
		// Convenient abstract constants... what we really need to know
	strIndexLeftInlet		= strIndexTheInlet,
	strIndexLeftOutlet		= strIndexTheOutlet
	};

	// UUID constants
const int 	kIDChars	= 8,				// # of elems in UUID.Data4 array
			kTypeChars	= 2;				// Code return val from UuidCreateSequential


#pragma mark • Type Definitions

#pragma mark • Object Structure

typedef struct {
	Object	coreObject;
	
	void*	controller;
	long	key,
			butCount,
			valCount;
			
	voidPtr	outCount,
			outButVal,
			outValVal;
	} objTrackDCtl;


#pragma mark • Global Variables

voidPtr	gSymAPIVers = NIL;


#pragma mark • Function Prototypes

	// Class message functions
static void* TrackDCtlrNew(long);

	// Object message functions
static void TrackDCtlrBang(objTrackDCtl*);
static void TrackDCtlrButCount(objTrackDCtl*);
static void TrackDCtlrValCount(objTrackDCtl*);
static void TrackDCtlrButtonVal(objTrackDCtl*, long);
static void TrackDCtlrValuatorVal(objTrackDCtl*, long);

static void TrackDCtlrGetAPIVers(objTrackDCtl*);
static void	TrackDCtlrAssist(objTrackDCtl*, void*, long, long, char*);
static void	TrackDCtlrInfo(objTrackDCtl*);
static void TrackDCtlrTattle(objTrackDCtl*);

#if !__LITTER_OBJECT__
static void TrackDCtlrVers(Object*, long, long);
#endif


#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark • Inline Functions

#pragma mark -

/******************************************************************************************
 *
 *	main()
 *	
 *	Standard Max External Object Entry Point Function
 *	
 ******************************************************************************************/

void
main()
	
	{
	
	// Initialize global variables that can't be statically initialized
	gSymAPIVers = gensym( (char*) getTrackdAPIVersion() );	// Have to typecast because
															// of gensym()'s dumb prototype
	
	// Standard Max setup() call
	setup(	&gObjectClass,					// Pointer to our class definition
			(method) TrackDCtlrNew,			// Allocation method
			NIL,							// No custom deallocation function
			sizeof(objTrackDCtl),			// Class object size
			NIL,							// No menu function
			A_LONG,							// One mandatory initialization argument
			0);
	
	// Messages
	addbang	((method) TrackDCtlrBang);
	addmess	((method) TrackDCtlrButCount,	"butcount",		A_NOTHING);
	addmess	((method) TrackDCtlrValCount,	"valcount",		A_NOTHING);
	addmess	((method) TrackDCtlrButtonVal,	"getbut",		A_LONG, 0);
	addmess	((method) TrackDCtlrValuatorVal,"getval",		A_LONG, 0);
	addmess ((method) TrackDCtlrGetAPIVers,	"getapivers",	A_NOTHING);
	
	addmess	((method) TrackDCtlrAssist,		"assist",		A_CANT, 0);
//	addmess	((method) TrackDCtlrInfo,		"info",			A_CANT, 0);
	addmess	((method) TrackDCtlrTattle,		"dblclick",		A_CANT, 0);
	addmess	((method) TrackDCtlrTattle,		"tattle",		A_NOTHING);
	
	// Initialize Litter Library... or do it manually
#if __LITTER_OBJECT__

	LitterInit(kClassName, 0);

#else
	
	#ifdef WIN_VERSION
	
		XQT_InitializeQTML(0);
		
	#else
	
		{
		short	externResFile	= CurResFile(),
				prevResFile		= PrevResFile();
		
		// Get *named* resource
		resnamecopy('STR#', (char*) iClassName);
		
		// ... and store the IDs assigned to them in Max Temp
		if (prevResFile > 0) {
			UseResFile(prevResFile);
			gStrResID	= FindResID('STR#', iClassName);
			UseResFile(externResFile);
			}
		
		gModDate = XObjGetModDate(externResFile);
		}
			
	#endif
	
	// Add vers message, otherwise done by LitterInit
	addmess	((method) TrackDCtlrVers,	"vers",	A_DEFLONG, A_DEFLONG, 0);
	
#endif

	}

#pragma mark -
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	TrackDCtlrNew(iOracle)
 *
 ******************************************************************************************/

void*
TrackDCtlrNew(
	long	iKey)
	
	{
	objTrackDCtl* me = (objTrackDCtl*) newobject(gObjectClass);
	
	if (me == NIL) {
		error("%s: could not create object", kClassName);
		goto punt;
		}
	
	me->key		= iKey;
	me->controller	= trackdInitControllerReader(iKey);
	
	if (me->controller != NIL) {
		me->butCount	= trackdGetNumberOfButtons(me->controller);
		me->valCount	= trackdGetNumberOfValuators(me->controller);
		}
	else {
		error("%s: could not access controller ID %ld", kClassName,iKey);
		me->butCount	= 0;
		me->valCount	= 0;
		}
	
	// Add outlets, right-to-left
	me->outValVal		= intout(me);
	me->outButVal		= intout(me);
	me->outCount		= intout(me);

punt:
	return me;
	}

#pragma mark -
#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	TrackDCtlrBang(me)
 *
 ******************************************************************************************/

void
TrackDCtlrBang(
	objTrackDCtl* me)
	
	{
	// Combined butcount/valcount
	Atom	list[2];
	
	AtomSetLong(&list[0], me->butCount);
	AtomSetLong(&list[1], me->valCount);
	
	outlet_list(me->outCount, NIL, 2, list);
	}

void TrackDCtlrButCount(objTrackDCtl* me)
	{ outlet_int(me->outCount, me->butCount); }

void TrackDCtlrValCount(objTrackDCtl* me)
	{ outlet_int(me->outCount, me->valCount); }

void
TrackDCtlrButtonVal(
	objTrackDCtl*	me,
	long			iBut)
	
	{
	Atom	list[2];
	
	if (iBut < 0 || me->butCount <= iBut) {
		error("%s: invalid button number %ld. Valid range: 0 - %ld",
				kClassName, iBut, me->butCount - 1);
		return;
		}
	
	AtomSetLong(&list[0], iBut);
	AtomSetLong(&list[1], trackdGetButton(me->controller, iBut));
	
	outlet_list(me->outButVal, NIL, 2, list);	
	}
	
	
	
void
TrackDCtlrValuatorVal(
	objTrackDCtl*	me,
	long			iVal)

	{
	Atom	list[2];
	
	if (iVal < 0 || me->valCount <= iVal) {
		error("%s: invalid button number %ld. Valid range: 0 - %ld",
				kClassName, iVal, me->valCount - 1);
		return;
		}
	
	AtomSetLong(&list[0], iVal);
	AtomSetLong(&list[1], trackdGetValuator(me->controller, iVal));
	
	outlet_list(me->outValVal, NIL, 2, list);	
	}


/******************************************************************************************
 *
 *	TrackDAPIVers(me)
 *
 ******************************************************************************************/

void
TrackDCtlrGetAPIVers(
	objTrackDCtl*	me)
	
	{
	
	if (gSymAPIVers == NIL)
		gSymAPIVers	= gensym( (char*) getTrackdAPIVersion() );	// Have to typecast because
																// of gensym()'s prototype
	
	outlet_anything(me->coreObject.o_outlet, gSymAPIVers, 0, NIL);
	}

/******************************************************************************************
 *
 *	TrackDCtlrTattle(me)
 *
 * 	Tell all we know.
 *
 ******************************************************************************************/

void
TrackDCtlrTattle(
	objTrackDCtl*	me)
	
	{
	
	post("%s state:", kClassName);
	post("  controller memory at %p", me->controller);
	post("  button count: %ld", me->butCount);
	post("  valuator count: %ld", me->valCount);
	
	}

/******************************************************************************************
 *
 *	TrackDCtlrAssist()
 *	TrackDCtlrInfo(me)
 *	TrackDCtlrVers(me)
 *
 *	Generic implementations
 *
 ******************************************************************************************/

#if __LITTER_OBJECT__

	void TrackDCtlrAssist(objTrackDCtl*, void*, long iDir, long iArgNum, char* oCStr)
		{ LitterAssist(iDir, iArgNum, strIndexLeftInlet, strIndexLeftOutlet, oCStr); }

	void TrackDCtlrInfo(objTrackDCtl* me)
		{ LitterInfo(kClassName, &me->coreObject, (method) TrackDCtlrTattle); }

#else
	
	void TrackDCtlrAssist(objTrackDCtl*, void*, long iDir, long iArgNum, char* oDestCStr)
		{
		#ifdef WIN_VERSION
			
			int	iStrResID = ((iDir == ASSIST_INLET) ? strIndexLeftInlet : strIndexLeftOutlet)
							 + iArgNum;
				
			LoadString(gDLLInstanceHdl, iStrResID, oDestCStr, kMaxAssistStrLen);
			
		#else
		
			assist_string(	gStrResID, iDir, iArgNum,
							strIndexLeftInlet,
							strIndexLeftOutlet,
							oDestCStr);
			
		#endif
		}
	
	
	void TrackDCtlrVers(Object* iObject, long iVersID, long iVersType)
		{
		
		if (iVersID == 0)
			iVersID = lpVersIDDefault;
		if (iVersType == 0)
			iVersType = lpVersTypeDefault;
		
		if (iVersType == lpVersTypeVal)
			outlet_int(iObject->o_outlet, LitterGetVersVal(iVersID));
		else {
			char versStr[kMaxResourceStrSize];
		
			LitterGetVersStr(iVersID, iVersType, versStr);
			
			outlet_anything(iObject->o_outlet, gensym(versStr), 0, NIL);
			}
			
		}

#endif