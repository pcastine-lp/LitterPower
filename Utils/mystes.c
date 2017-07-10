/*
	File:		mystes.c

	Contains:	Get a machine-dependent unique ID.

	Written by:	Peter Castine

	Copyright:	© 2001-2002 Peter Castine

	Change History (most recent first):

         <2>     26–2–05    pc      It turns out UuidCreateSequential() is senstive to whether the
                                    ethernet card[s] are connected to a network at boot time. Let's
                                    try reading a serial number from the C: disk instead.
         <1>    18–10–04    pc      first checked in.
*/


/******************************************************************************************
 ******************************************************************************************/

	// Determine whether mystes is to be built as a member of the Litter Power Package
#if		__ide_target("X-Mystes (Windows)")
	#define __LITTER_OBJECT__	0
#else
	#define __LITTER_OBJECT__	1
#endif


#pragma mark • Include Files

#include "LitterLib.h"
#include "TrialPeriodUtils.h"



#pragma mark • Constants

const char*	kClassName		= "ru.mystes";			// Class name


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



#pragma mark • Object Structure

typedef struct {
	Object		coreObject;
	
	unsigned long	serNo,
					mystery;
	} objMystes;






#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark -
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	MystesNew(iOracle)
 *
 *	Instantiate new object.
 *	Part ot this deal is to grab serial number identification from hard drive.
 *
 ******************************************************************************************/

static void*
MystesNew(void)
	
	{
	objMystes*	me = (objMystes*) newobject(gObjectClass);	// Create object w/left inlet
	
	// Create list outlet
	// Left outlet can be accessed as me->coreObject.o_outlet
	listout(me);
	

	// Get Serial number of C:/ disk
	if ( GetVolumeInformation("C:\\", NIL, 0, &me->serNo, 0, 0, NIL, 0) )
		me->mystery = me->serNo ^ me->coreObject.o_magic;
	else {
		// This shouldn't happen
		me->serNo = me->coreObject.o_magic;
		me->mystery = 0;
		}
	

	return me;
	}

#pragma mark -
#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	MystesBang(me)
 *
 ******************************************************************************************/

static void
MystesBang(
	objMystes* me)
	
	{
	Atom	charVec[8];
	
	AtomSetLong(&charVec[0], me->serNo & 0x000000ff);
	AtomSetLong(&charVec[1], (me->serNo >> 8) & 0x000000ff);
	AtomSetLong(&charVec[2], (me->serNo >> 16) & 0x000000ff);
	AtomSetLong(&charVec[3], (me->serNo >> 24) & 0x000000ff);
	AtomSetLong(&charVec[4], (me->mystery >> 24) & 0x000000ff);
	AtomSetLong(&charVec[5], (me->mystery >> 16) & 0x000000ff);
	AtomSetLong(&charVec[6], (me->mystery >> 8) & 0x000000ff);
	AtomSetLong(&charVec[7], me->mystery & 0x000000ff);
	
	outlet_list(me->coreObject.o_outlet, NIL, 8, charVec);
	
	}


/******************************************************************************************
 *
 *	MystesTattle(me)
 *
 * 	Tell all we know.
 *
 ******************************************************************************************/

static void
MystesTattle(
	objMystes*	me)
	
	{
	
	post("%s state:", kClassName);
	post("  serial number: %lx", me->serNo);
	post("  mystery value: %lx", me->mystery);
	
	}

/******************************************************************************************
 *
 *	MystesAssist()
 *	MystesInfo(me)
 *	MystesVers(me)
 *
 *	Generic implementations
 *
 ******************************************************************************************/

#if __LITTER_OBJECT__

	static void MystesAssist(objMystes*, void*, long iDir, long iArgNum, char* oCStr)
		{ LitterAssist(iDir, iArgNum, strIndexLeftInlet, strIndexLeftOutlet, oCStr); }

	static void MystesInfo(objMystes* me)
		{ LitterInfo(kClassName, &me->coreObject, (method) MystesTattle); }

#else
	
	void MystesAssist(objMystes*, void*, long iDir, long iArgNum, char* oDestCStr)
		{
		#ifdef WIN_VERSION
			
			const int kMaxAssistStrLen = 60;
			
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
	
	
	void MystesVers(Object* iObject, long iVersID, long iVersType)
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


#pragma mark • Entry point

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
	LITTER_CHECKTIMEOUT(kClassName);
	
	// Standard Max setup() call
	setup(	&gObjectClass,					// Pointer to our class definition
			(method) MystesNew,			    // Allocation method
			NIL,							// No custom deallocation function
			sizeof(objMystes),				// Class object size
			NIL,							// No menu function
			A_NOTHING);						// No initialization arguments
	
	// Messages
	LITTER_TIMEBOMB addbang	((method) MystesBang);
	addmess	((method) MystesAssist,		"assist",		A_CANT, 0);
	
	/*		-- Standard messages we are currently deliberately NOT implementing
				as a security measure --
	addmess	((method) MystesInfo,		"info",			A_CANT, 0);
	addmess	((method) MystesTattle,		"dblclick",		A_CANT, 0);
	addmess	((method) MystesTattle,		"tattle",		A_NOTHING);
	*/
	
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
	addmess	((method) MystesVers,	"vers",	A_DEFLONG, A_DEFLONG, 0);
	
#endif

	}

