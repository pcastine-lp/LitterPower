/*
	File:		ArgusFilch.c

	Contains:	Tries to force Max patcher windows follow certain "Rules" at loadbang time
				Automatically repositions off-screen window.
				If another copy of the patch is already open, bring it to the front and
				self-destruct the unnecessary copy.
				
				This is currently only a sketch/stub with some code copied over from
				MrsNorris.c
				
				TO DO: Full(er) for UI Guidelines of the host architecture. On Mac OS this
				would include: storing screen dimensions on save and adjusting position
				on re-open if the screen has changed; forcing smart-zoom; adjust for size of
				window frame (which depends on OS version).
				Need to investigate what Windwos behavior is.

	Written by:	Peter Castine

	Copyright:	© 2006 Peter Castine

	Change History (most recent first):

         <2>    8–5–2006    pc      Remove debugging post() calls
         <1>   26–4–2006    pc      First checked in. Still contains some debugging code to trim.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "LitterLib.h"	// Also #includes MaxUtils.h, ext.h
#include "TrialPeriodUtils.h"
#include <string.h>		// We use strncmp()

#if !defined(TARGET_API_MAC_CARBON) || !TARGET_API_MAC_CARBON
	// Need to get Patcher typedef, need to explicitly #include
	#include "ext_user.h"
	#include "ext_wind.h"
#endif


#pragma mark • Constants

const char*	kClassName		= "lp.argus";			// Class name

	// Indices for STR# resource
enum {
	strIndexInLeft		= lpStrIndexLastStandard + 1,
	strIndexOutLeft
	};

	// Some compilers allow definition of array size constants with
	//' const int' syntax, but some don't. Enum works for everyone
enum {
	kMaxFileNameLen	= 255
	};


#pragma mark • Windows Dynamic Linking

#ifdef WIN_VERSION
typedef void		(*tWindAPIVisFunc)(t_wind*);
typedef t_syswind*	(*tWindAPIGetSysWindFunc)(t_wind*);
typedef void		(*tWindAPISysMove)(t_syswind, long, long, Boolean);

tWindAPIVisFunc				WindVisFunc			= NIL,
							WindInvisFunc		= NIL,
							WindCloseFunc		= NIL;
tWindAPIGetSysWindFunc		WindGetSysWindFunc	= NIL;
tWindAPISysMove				WindSysWindMoveFunc	= NIL;
#endif


#pragma mark • Object Structure

typedef struct {
	Object		coreObject;
	Patcher*	owner;
	} objFilch;


#pragma mark • Global Variables

static Boolean gHasWindAPI = false;


#pragma mark • Function Prototypes

	// Class message functions
void*	ArgusNew(void);

	// Object message functions
static void ArgusBang		(objFilch*);
static void ArgusLoadbang	(objFilch*);

static void	ArgusTattle	(objFilch*);
static void ArgusAssist	(objFilch*, void*, long, long, char*);
static void ArgusInfo	(objFilch*);


#pragma mark -

/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark • Inline Functions

/******************************************************************************************
 *
 *	Wrappers around the conditionally available t_wind API calls.
 *	
 ******************************************************************************************/

static inline void WindAPICallWindVis(t_wind* iWindow)
	{
#ifdef WIN_VERSION
	if (WindVisFunc != NIL) WindVisFunc(iWindow);
#else
	if (gHasWindAPI) wind_vis(iWindow);
#endif
	}

static inline void WindAPICallWindInvis(t_wind* iWindow)
	{
#ifdef WIN_VERSION
	if (WindInvisFunc != NIL) WindInvisFunc(iWindow);
#else
	if (gHasWindAPI) wind_invis(iWindow);
#endif
	}
	
static inline void WindAPICallWindClose(t_wind* iWindow)
	{
#ifdef WIN_VERSION
	if (WindInvisFunc != NIL) WindCloseFunc(iWindow);
#else
	if (gHasWindAPI) wind_close(iWindow);
#endif
	}
	
static inline t_wind* WindAPICallWindGetSysWind(t_wind* iWindow)
	{
#ifdef WIN_VERSION
	return (WindGetSysWindFunc != NIL) ? WindGetSysWindFunc(iWindow) : NIL;
#else
	return (gHasWindAPI) ? wind_syswind(iWindow) : NIL;
#endif
	}

static inline void WindAPICallSysWindMove(t_syswind sw, long x, long y, Boolean front)
	{
#ifdef WIN_VERSION
	if (WindSysWindMoveFunc != NIL) WindSysWindMoveFunc(sw, x, y, front);
#else
	if (gHasWindAPI) syswindow_move(sw, x, y, front);
#endif
	}


#pragma mark -

/******************************************************************************************
 *
 *	main()
 *	
 *	Standard Max/MSP External Object Entry Point Function
 *	
 ******************************************************************************************/

void
main(void)
	
	{
	LITTER_CHECKTIMEOUT(kClassName);
	
	// Standard Max/MSP initialization mantra
	setup(	&gObjectClass,				// Pointer to our class definition
			(method) ArgusNew,			// Instance creation function
			NIL,						// Default deallocation function
			sizeof(objFilch),			// Class object size
			NIL,						// No menu function
			A_NOTHING);					// Optional arguments: Currently none
	
	// Messages
	LITTER_TIMEBOMB addbang	((method) ArgusBang);
	LITTER_TIMEBOMB addmess	((method) ArgusLoadbang,	"loadbang",	A_CANT, 0);
	addmess	((method) ArgusTattle,		"dblclick",	A_CANT, 0);
	addmess	((method) ArgusTattle,		"tattle",	A_NOTHING);
	addmess	((method) ArgusAssist,		"assist",	A_CANT, 0);
	addmess	((method) ArgusInfo,		"info",		A_CANT, 0);
	

	// Initialize Litter Library
	LitterInit(kClassName, 0);
	
	// Set up t_wind API Calls, which are only avaliable prior to Max 5
	if (LitterGetMaxVersion() < 0x0500) {
	#ifdef WIN_VERSION
		HINSTANCE lib = LoadLibrary("MaxAPI.lib");
		
		// ASSERT(lib != NIL), because Max versions 4.3-4.6 won't be running on Windows
		// without it. Still, our programming style always checks for the worst.
		// Also note that although we could theoretically keep track of instantiation
		// count and free up everything when there are no objects using the library, but
		// given that our HINSTANCE is simply a pointer to the globally open instance that
		// isn't going to be freed ever, there doesn't seem to be much point 
		if (lib != NIL) {
			WindVisFunc			= (tWindAPIVisFunc)		 	GetProcAddress(lib, "wind_vis");
			WindInvisFunc		= (tWindAPIVisFunc) 		GetProcAddress(lib, "wind_invis");
			WindCloseFunc		= (tWindAPIVisFunc)			GetProcAddress(lib, "wind_close");
			WindGetSysWindFunc	= (tWindAPIGetSysWindFunc)	GetProcAddress(lib, "wind_syswind");
			WindSysWindMoveFunc	= (tWindAPISysMove)			GetProcAddress(lib, "syswindow_move");
					
			// Again, it seems impossible that the procedure pointer should be NIL
			if ((WindVisFunc == NIL) || (WindInvisFunc == NIL)
					|| (WindCloseFunc == NIL) || (WindGetSysWindFunc == NIL)
					|| (WindSysWindMoveFunc == NIL)) {
				error("%s: couldn't get address of t_wind functions");
				// Make sure both are zeroed
				WindVisFunc			= NIL;
				WindInvisFunc		= NIL;
				WindCloseFunc		= NIL;
				WindGetSysWindFunc	= NIL;
				WindSysWindMoveFunc	= NIL;
				}
			else gHasWindAPI = true;
			}
	#else
		gHasWindAPI = true;
	#endif
		}
	}

#pragma mark -
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	ArgusNew(iName, iHelp)
 *	
 ******************************************************************************************/

void* ArgusNew(void)
	{
	objFilch* me = newobject(gObjectClass);
	
	if (me == NIL)
		goto punt;									// Poor man's exception handling
	
	me->owner = (Patcher*) gensym("#P")->s_thing;	// Requires some faith in the Max API
	
	// We need an outlet for the vers message
	// Access it through me->coreObject.o_outlet
	outlet_new (me, NIL);
	
punt:
	return me;
	}

#pragma mark -
#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	ArgusBang(me)
 *	ArgusLoad(me, iName)
 *	
 ******************************************************************************************/

	// Parameter block for FilchAround()
	struct filchParams {
		short		directory;
		char		name[kMaxFileNameLen + 1];
		Patcher*	original;
		};
	
	static short FilchAround(Patcher* iPatcher, struct filchParams* ioParams)
		{
		t_wind*	pWind	= (t_wind*) iPatcher->p_wind;
							// Typecast  required for the legacy Boron Max API.
		Boolean	found	= false;
		
		if (pWind->w_vol == ioParams->directory
				&& strcmp(pWind->w_name, ioParams->name) == 0 ) {
			found = true;
			ioParams->original = iPatcher;
			}
		
		return found;
		}
	

	// #define following to 1, 2, or 3
#define _DELETE_STRATEGY_ 2		// Safest so far

void
ArgusBang(
	objFilch*	me)
	
	{
	struct filchParams	params;
	t_wind*				myWind = (t_wind*) me->owner->p_wind;
	
	strcpy(params.name, myWind->w_name);
	params.directory = myWind->w_vol;
	params.original = NIL;
	
	patcher_eachdo((eachdomethod)FilchAround, (void*) &params);
	
	if (params.original != NIL && gHasWindAPI) {
		t_wind* origWind = (t_wind*) params.original->p_wind;
		
		WindAPICallWindVis((t_wind*) origWind);
				// Typecast required for legacy Boron Max
		// ?? !! Need to trigger hide&destruct of our own window
	#if		(_DELETE_STRATEGY_ == 1)
		freeobject((Object*) me->owner);
		// Generates "zgetfn.... corrupt object" error messages some time later.
		
	#elif 	(_DELETE_STRATEGY_ == 2)
		WindAPICallWindInvis(myWind);
		#if defined(MAC_VERSION) 
			// Do nothing currently :-(
			// What to do is dependent on what the Max 5 API offers 
			// Under Max 4.2 and later we compiled the same source for (MAC_VERSION) and (WIN_VERSION)
		#elif defined(WIN_VERSION)
			WindSysWindMoveFunc(WindGetSysWindFunc(origWind), origWind->w_x1, origWind->w_y1, false);
		#else
			MoveWindow((WindowPtr)&(origWind->w_wind), origWind->w_x1, origWind->w_y1, false);
		#endif
		outlet_anything(me->coreObject.o_outlet, gensym("dispose"), 0, NIL);
		// Draws window before the WindAPICallWindInvis() call takes effect
		// So we get flashing on screen
		
	#elif 	(_DELETE_STRATEGY_ == 3)
		myWind->w_dirty = false;		// Be safe
		WindAPICallWindClose(myWind);
		// Like _DELETE_STRATEGY_ == 1 if called at loadbang time
		// If this is a real bang, it crashes Max.
		
	#endif
		}
//#if MAC_VERSION
#if 0
	else {
		// Need to make sure that the window is visible with the available screen
		// configuration, which may have changed since the patcher was saved.
		// ?? !! This is not water-tight. At all.
		BitMap	screenBits;
		Rect	screenRect,
				windRect,
				visWindRect;
		
		GetQDGlobalsScreenBits(&screenBits);
		screenRect = screenBits.bounds;
		
		windRect.left	= myWind->w_x1;
		windRect.right	= myWind->w_x2;
		windRect.top	= myWind->w_y1;
		windRect.bottom	= myWind->w_y2;
		
		SectRect(&screenRect, &windRect, &visWindRect);
//post("ArgusBang(): screenRect (%ld, %ld) (%ld, %ld)", (long) screenRect.left, (long) screenRect.top, (long) screenRect.right, (long) screenRect.bottom);
//post("  windRect (%ld, %ld) (%ld, %ld)", (long) windRect.left, (long) windRect.top, (long) windRect.right, (long) windRect.bottom);
		InsetRect(&visWindRect, 8, 8);
//post("  visible area (%ld, %ld) (%ld, %ld)", (long) visWindRect.left, (long) visWindRect.top, (long) visWindRect.right, (long) visWindRect.bottom);
		if (EmptyRect(&visWindRect)) {
//post("  about to move window");
			WindSysWindMoveFunc(WindGetSysWindFunc(myWind), 20, 20, true);
//post("  ...new top left: %ld %ld", myWind->w_x1, myWind->w_y1);
			}
		}
#endif

	}

void ArgusLoadbang(objFilch* me)
#if 0
	{ ArgusBang(me); }
#else
	// Testing
	{ if (maxversion() < 0x0500) ArgusBang(me); }
#endif


/******************************************************************************************
 *
 *	ArgusTattle(me)
 *	ArgusAssist
 *	ArgusInfo(me)
 *
 *	Post state information
 *
 ******************************************************************************************/

void ArgusTattle(objFilch* me)
	{
	
	post("%s state", kClassName);
	post("  owner located at address %p", me->owner);
	post("  window name: %s", ((t_wind*) me->owner->p_wind)->w_name);
	post("  home directory: %ld", (long) ((t_wind*) me->owner->p_wind)->w_vol);
	post("  Delete strategy is %ld", (long) _DELETE_STRATEGY_);
	}


void ArgusAssist(objFilch* me, void* iBox, long iDir, long iArgNum, char* oCStr)
	{
	#pragma unused(me, iBox)
	
	LitterAssist(iDir, iArgNum, strIndexInLeft, strIndexOutLeft, oCStr);
	}

void ArgusInfo(objFilch* me)
	{ LitterInfo(kClassName, &me->coreObject, (method) ArgusTattle); }

	
