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

	// Currently not to be built as a member of the Litter Power Package
	// {11.11.09 pc} Changed to use the full Litter Library (by #defining __LITTER_OBJECT__ to 1)
	//				 as the simplest way to provide Max 5 compatibility
#define __LITTER_OBJECT__	1

#pragma mark • Include Files

#include "LitterLib.h"
#include "trackdAPI_c.h"

#pragma mark • Constants

const char*	kClassName		= "vicon.tracker";			// Class name


	// Indices for STR# resource
enum {
	strIndexTheInlet		= lpStrIndexLastStandard + 1,
	
	strIndexOutSensCount,
	strIndexOutPos,
	strIndexOutEuler,
	strIndexOutMatrix,
	
		// Convenient abstract constants... what we really need to know
	strIndexLeftInlet		= strIndexTheInlet,
	strIndexLeftOutlet		= strIndexOutSensCount
	};



#pragma mark • Type Definitions

#pragma mark • Object Structure

typedef struct {
	Object	coreObject;
	
	voidPtr	tracker;
	long	key,
			sensorCount;
			
	voidPtr	outSensCount,
			outPos,
			outAngle,
			outMatrix;

	} objVTracker;


#pragma mark • Global Variables

SymbolPtr	gSymList	= NIL,
			gSymAPIVers	= NIL;


#pragma mark • Function Prototypes

	// Class message functions
static void* TrackDNew(long);

	// Object message functions
static void TrackDGetAPIVers(objVTracker*);
static void TrackDSensCount(objVTracker*);
static void TrackDPos(objVTracker*, long);
static void TrackDAngle(objVTracker*, long);
static void TrackDMatrix(objVTracker*, long);
static void	TrackDAssist(objVTracker*, void*, long, long, char*);
static void	TrackDInfo(objVTracker*);
static void TrackDTattle(objVTracker*);

#if !__LITTER_OBJECT__
static void TrackDVers(Object*, long, long);
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
	
	// Standard Max setup() call
	setup(	&gObjectClass,					// Pointer to our class definition
			(method) TrackDNew,			    // Allocation method
			NIL,							// No custom deallocation function
			sizeof(objVTracker),			// Class object size
			NIL,							// No menu function
			A_LONG,							// One mandatory initialization argument
			0);
	
	// Messages
	addbang	((method) TrackDSensCount);
	addmess ((method) TrackDSensCount,	"senscount",	A_NOTHING);

	addmess	((method) TrackDPos,		"pos",			A_LONG, 0);
	addmess	((method) TrackDAngle,		"angle",		A_LONG, 0);
	addmess	((method) TrackDMatrix,		"matrix",		A_LONG, 0);
	
	addmess ((method) TrackDGetAPIVers,	"getapivers",	A_NOTHING);
	
	addmess	((method) TrackDAssist,		"assist",		A_CANT, 0);
//	addmess	((method) TrackDInfo,		"info",			A_CANT, 0);
	addmess	((method) TrackDTattle,		"dblclick",		A_CANT, 0);
	addmess	((method) TrackDTattle,		"tattle",		A_NOTHING);
	
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
	addmess	((method) TrackDVers,	"vers",	A_DEFLONG, A_DEFLONG, 0);
	
#endif

	// Initialize global list symbol
	// We don't initialize the APIVers symbol until someone asks for it
	gSymList	= gensym("list");
	gSymAPIVers = gensym( (char*) getTrackdAPIVersion() );	// Have to typecast because
															// of gensym()'s dumb prototype
	

	}

#pragma mark -
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	TrackDNew(iOracle)
 *
 ******************************************************************************************/

void*
TrackDNew(
	long	iKey)
	
	{
	objVTracker*	me = (objVTracker*) newobject(gObjectClass);
	
	if (me == NIL) {
		error("%s: could not create object", kClassName);
		goto punt;
		}
	
	me->key		= iKey;
	me->tracker	= trackdInitTrackerReader(iKey);
	
	if (me->tracker != NIL) {
		me->sensorCount	= trackdGetNumberOfSensors(me->tracker);
		}
	else {
		error("%s: could not access tracker ID %ld", kClassName,iKey);
		me->sensorCount	= 0;
		}
	
	// Add outlets, right-to-left
	me->outMatrix		= listout(me);
	me->outAngle		= listout(me);
	me->outPos			= listout(me);
	me->outSensCount	= intout(me);

punt:
	return me;
	}

#pragma mark -
#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	TrackDSensCount(me)
 *
 ******************************************************************************************/

static void TrackDSensCount(objVTracker* me)
	{ outlet_int(me->outSensCount, me->sensorCount); }


/******************************************************************************************
 *
 *	TrackDPos(me, iSensor)
 *	TrackDAngle(me, iSensor)
 *	TrackDMatrix(me, iSensor)
 *
 ******************************************************************************************/

	static Boolean TrackDValidSensor(objVTracker* me, long iSensor)
		{
		// Note that if the tracker does not exist, any value for iSensor will return false
		Boolean result = false;
		
		if (iSensor < 0 || me->sensorCount <= iSensor) {
			error("%s: invalid sensor number %ld. Valid range: 0 - %ld",
					kClassName, iSensor, me->sensorCount - 1);
			}
		else result = true;						// Hooray!
		
		return result;
		}

void
TrackDPos(
	objVTracker*	me,
	long			iSensor)
	
	{
	float	pos[3];
	Atom	list[4];
	
	
	if ( !TrackDValidSensor(me, iSensor) )
		return;
	
	trackdGetPosition(me->tracker, iSensor, pos);
	AtomSetLong(&list[0], iSensor);
	AtomSetFloat(&list[1], pos[0]);
	AtomSetFloat(&list[2], pos[1]);
	AtomSetFloat(&list[3], pos[2]);
	
	outlet_list(me->outPos, gSymList, 4, list);
	}


void
TrackDAngle(
	objVTracker*	me,
	long			iSensor)

	{
	float	angles[3];
	Atom	list[4];
	
	if ( !TrackDValidSensor(me, iSensor) )
		return;
	
	trackdGetEulerAngles(me->tracker, iSensor, angles);
	AtomSetLong(&list[0], iSensor);
	AtomSetFloat(&list[1], angles[0]);
	AtomSetFloat(&list[2], angles[1]);
	AtomSetFloat(&list[3], angles[2]);
	
	outlet_list(me->outAngle, gSymList, 4, list);
	}
	
void
TrackDMatrix(
	objVTracker*	me,
	long			iSensor)
	
	{
	float	matrix[16];			// Actually [4][4], but a flattened vector is OK for our
	Atom	list[17];			// purposes
	int		i;
	
	if ( !TrackDValidSensor(me, iSensor) )
		return;
	
	trackdGetEulerAngles(me->tracker, iSensor, matrix);
	
	AtomSetLong(&list[0], iSensor);
	for (i = 0; i < 16; i += 1)
		AtomSetFloat(&list[i+1], matrix[i]);
	
	outlet_list(me->outMatrix, gSymList, 17, list);
	}
	

/******************************************************************************************
 *
 *	TrackDAPIVers(me)
 *
 ******************************************************************************************/

void TrackDGetAPIVers(objVTracker* me)
	{ outlet_anything(me->coreObject.o_outlet, gSymAPIVers, 0, NIL); }

/******************************************************************************************
 *
 *	TrackDTattle(me)
 *
 * 	Tell all we know.
 *
 ******************************************************************************************/

void
TrackDTattle(
	objVTracker*	me)
	
	{
	
	post("%s state:", kClassName);
	post("  tracker memory at %p", me->tracker);
	post("  sensor count: %ld", me->sensorCount);
	
	}

/******************************************************************************************
 *
 *	TrackDAssist()
 *	TrackDInfo(me)
 *	TrackDVers(me)
 *
 *	Generic implementations
 *
 ******************************************************************************************/

#if __LITTER_OBJECT__

	void TrackDAssist(objVTracker*, void*, long iDir, long iArgNum, char* oCStr)
		{ LitterAssist(iDir, iArgNum, strIndexLeftInlet, strIndexLeftOutlet, oCStr); }

	void TrackDInfo(objVTracker* me)
		{ LitterInfo(kClassName, &me->coreObject, (method) TrackDTattle); }

#else
	
	void TrackDAssist(objVTracker*, void*, long iDir, long iArgNum, char* oDestCStr)
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
	
	
	void TrackDVers(Object* iObject, long iVersID, long iVersType)
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