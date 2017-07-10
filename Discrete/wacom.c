/* wacom.c -- access Wacom graphics tablet (via WinTab library) -- */
/* (c) 2005-2006 by Olaf Matthes, <olaf.matthes@gmx.de> */

#ifdef PD
#include "m_pd.h"
#include "g_canvas.h"			// for information about the canvas we're sitting in
#include <windows.h>
#include <stdio.h>				// for sprintf()
// define some things that we don't have in Pd
#define outlet_int outlet_float
#define SETLONG SETFLOAT
#define CLIP(a, lo, hi) ( (a)>(lo)?( (a)<(hi)?(a):(hi) ):(lo) )
#else	// Max
#include "ext.h"
#include "ext_common.h"			// for CLIP()
#endif

#define POLLTIME		5		// poll the tablet queue every 5ms when in automatic polling

#ifdef _WINDOWS
//
// under Windows we use the WinTab API from Wacom
//
#include <wintab.h>
#define PACKETDATA	(PK_X | PK_Y | PK_BUTTONS | PK_NORMAL_PRESSURE | PK_ORIENTATION | PK_CURSOR)
#define PACKETMODE	PK_BUTTONS
#include <pktdef.h>

#define NPACKETQSIZE	32		// packet queue size, set as needed for your app

//////////
// wintab32 calls:
// we define our own pointers to the calls in the DLL, so that the external
// can be run even without the DLL present and at least issue a nice warning
typedef UINT (API *WTINFO_FUNC)(UINT, UINT, LPVOID);
typedef HCTX (API *WTOPEN_FUNC)(HWND, LPLOGCONTEXTA, BOOL);
typedef BOOL (API *WTCLOSE_FUNC)(HCTX);
typedef BOOL (API *WTPACKET_FUNC)(HCTX, UINT, LPVOID);
typedef BOOL (API *WTPACKETSGET_FUNC)(HCTX, UINT, LPVOID);
typedef BOOL (API *WTOVERLAP_FUNC)(HCTX, BOOL);
typedef BOOL (API *WTQUEUESIZESET_FUNC)(HCTX, int);

WTINFO_FUNC WTInfoPtr = NULL;
WTOPEN_FUNC WTOpenPtr = NULL;
WTCLOSE_FUNC WTClosePtr = NULL;
WTPACKET_FUNC WTPacketPtr = NULL;
WTPACKETSGET_FUNC WTPacketsGetPtr = NULL;
WTOVERLAP_FUNC WTOverlapPtr = NULL;
WTQUEUESIZESET_FUNC WTQueueSizeSetPtr = NULL;

HINSTANCE winTabLib = NULL;		// global handle to WinTab library
#else	// Mac
#endif

int wacom_instance = 0;			// number of objects that have been opened so far
int wacom_objcount = 0;			// number of objects being in use right now

typedef struct wacom
{
#ifdef PD
	t_object x_obj;
	t_canvas *x_canvas;			// reference to the TclTk canvas we're sitting in (not used)
	// outlets
    t_outlet *x_out_x_pos;
    t_outlet *x_out_y_pos;
    t_outlet *x_out_press;
    t_outlet *x_out_az;
    t_outlet *x_out_alt;
    t_outlet *x_out_twist;
    t_outlet *x_out_but;
    t_outlet *x_out_cursor;
	t_clock *x_clock;
#else	// Max
	t_object x_obj;
	t_patcher *x_patcher;		// reference to the patcher we're sitting in (not used)
	// outlets
    void *x_out_x_pos;
    void *x_out_y_pos;
    void *x_out_press;
    void *x_out_az;
    void *x_out_alt;
    void *x_out_twist;
    void *x_out_but;
    void *x_out_cursor;
	void *x_clock;
	void *x_qelem;				// queue element for checking the tablet packet queue
#endif
	// tablet device settings
	int x_instance;				// instance number
#ifdef _WINDOWS
	UINT x_device;				// device number of tablet to use (0 - n)
	HWND x_hwnd;				// window handle (of Max patcher window)
	HCTX x_tablethandle;		// handle to tablet
	long x_queuesize;			// packet queue size
#else	// Mac
#endif
	unsigned short x_nbuttons;	// number of buttons supported by device
	// user setable info
	float x_x_offset;
	float x_y_offset;
	long x_context;				// context type of cursor (tablet or mouse)
	short x_poll;				// indictaes whether automatic polling is on or off
	// resolution info (depends on tablet make)
	long x_res_x;				// max. x coordinate
	long x_res_y;				// max. y coordinate
	long x_res_press;			// max. pressure value
	long x_res_twist;			// max. twist value
	long x_res_azi;				// max. azimuth value
	long x_res_alt;				// max. altitude value
	// tablet data (already scaled to 0. - 1. range, where needed)
    float x_x_pos;				// 'real' x position (0. - 1., without offset)
    float x_y_pos;				// 'real' y position (0. - 1., without offset)
    float x_press;				// pressure (0. - 1.)
    float x_azi;				// azimuth
    float x_alt;				// altitude (elevation)
    float x_twist;				// twist
    float x_x_pos_old;			// previous value for x position
    float x_y_pos_old;			// previous value for  y position
    float x_press_old;			// previous value for pressure
    float x_azi_old;			// previous value for azimuth
    float x_alt_old;			// previous value for altitude (elevation)
    float x_twist_old;			// previous value for twist
	unsigned int x_cursor;		// cursor type (i.e. tool ID)
	unsigned int x_cursor_old;	// previous cursor type
} t_wacom;


#ifdef PD
static t_class *wacom_class;
static char *wacom_version = "wacom v1.1test4, (c) 2005-2006 Olaf Matthes";
#else
void *wacom_class;
char *wacom_version = "wacom v1.1test4, ÔøΩ 2005-2006 Olaf Matthes";
t_symbol *ps_preset;
#endif

void wacom_motion(t_wacom *x, float x_pos, float y_pos, float pressure);
void wacom_rotation(t_wacom *x, float azimuth, float altitude, float twist);
void wacom_button(t_wacom *x, unsigned short which, unsigned short state);
void wacom_cursor(t_wacom *x, unsigned int cursor);
void wacom_poll(t_wacom *x);
void wacom_nopoll(t_wacom *x);
void wacom_zero(t_wacom *x);
void wacom_reset(t_wacom *x);
void wacom_activate (t_wacom *x, short active);
void *wacom_packet(t_wacom *x);
void *wacom_tick(t_wacom *x);
void wacom_bang(t_wacom *x);
void wacom_close(t_wacom *x);
void wacom_print(t_wacom *x);
void wacom_setfront(t_wacom *x);
void wacom_free(t_wacom *x);
#ifdef PD
void wacom_setpoll(t_wacom *x, t_floatarg state);
void wacom_offset(t_wacom *x, t_floatarg x_off, t_floatarg y_off);
void wacom_mode(t_wacom *x, t_floatarg mode);
void wacom_open(t_wacom *x, t_floatarg device, t_floatarg context);
void *wacom_new(t_floatarg fdevice, t_floatarg fcontext, t_floatarg fqueuesize);
#else	// Max
void wacom_setpoll(t_wacom *x, long state);
void wacom_offset(t_wacom *x, double x_off, double y_off);
void wacom_preset(t_wacom *x);
void wacom_assist(t_wacom *x, void *b, long m, long a, char *s);
void wacom_mode(t_wacom *x, long mode);
void wacom_open(t_wacom *x, long device, long context);
void *wacom_new(long device, long context, long queuesize);
#endif	// PD

#ifdef PD
void wacom_setup(void)
{
    wacom_class = class_new(gensym("wacom"), (t_newmethod)wacom_new, 
    	(t_method)wacom_free, sizeof(t_wacom), 0, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(wacom_class, (t_method)wacom_open, gensym("open"), A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(wacom_class, (t_method)wacom_close, gensym("close"), 0);
    class_addmethod(wacom_class, (t_method)wacom_mode, gensym("mode"), A_DEFFLOAT, 0);
    class_addmethod(wacom_class, (t_method)wacom_zero, gensym("zero"), 0);
    class_addmethod(wacom_class, (t_method)wacom_offset, gensym("offset"), A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(wacom_class, (t_method)wacom_reset, gensym("reset"), 0);
    class_addmethod(wacom_class, (t_method)wacom_poll, gensym("poll"), 0);
    class_addmethod(wacom_class, (t_method)wacom_nopoll, gensym("nopoll"), 0);
    class_addmethod(wacom_class, (t_method)wacom_setpoll, gensym("setpoll"), A_DEFFLOAT, 0);
    class_addmethod(wacom_class, (t_method)wacom_setfront, gensym("setfront"), 0);
    class_addmethod(wacom_class, (t_method)wacom_print, gensym("print"), 0);
    class_addbang(wacom_class, (t_method)wacom_bang);
    class_sethelpsymbol(wacom_class, gensym("help-wacom~.pd"));
    post(wacom_version);
}

#else
void main()
{
	setup((t_messlist **)&wacom_class, (method)wacom_new, (method)wacom_free, 
		(short)sizeof(t_wacom), 0L, A_DEFLONG, A_DEFLONG, A_DEFLONG, 0);
	addbang((method)wacom_bang);
	addmess((method)wacom_open, "open", A_DEFLONG, A_DEFLONG, 0);
	addmess((method)wacom_close, "close", 0);
	addmess((method)wacom_mode, "mode", A_DEFLONG, 0);
	addmess((method)wacom_zero, "zero", 0);
	addmess((method)wacom_offset, "offset", A_DEFFLOAT, A_DEFFLOAT, 0);
	addmess((method)wacom_reset, "reset", 0);
	addmess((method)wacom_poll, "poll", 0);
	addmess((method)wacom_nopoll, "nopoll", 0);
	addmess((method)wacom_setpoll, "setpoll", A_DEFLONG, 0);
	addmess((method)wacom_setfront, "setfront", 0);
	addmess((method)wacom_print, "print", 0);
	addmess((method)wacom_assist, "assist", A_CANT, 0);
	addmess((method)wacom_preset, "preset", 0);
	finder_addclass("Devices", "wacom");
	post(wacom_version);
	ps_preset = gensym("_preset");		// preset is bound to this symbol
}
#endif

#ifdef _WINDOWS
/////////////////////////////////////////////////////////
// wacom_init_tablet
//
// initialize a tablet and return the tablet handle
//
/////////////////////////////////////////////////////////
HCTX wacom_tablet_init(t_wacom *x, HWND hWnd)
{
	LOGCONTEXT	lcMine;
	AXIS		tabletX, tabletY;	// the maximum tablet size
	AXIS		np;					// pressure info
	AXIS		or[3];				// orientation info
	UINT		NDevices;			// total number of devices
	UINT		wDevice = x->x_device;
	UINT		NCursors;			// number of supported cursors
	UINT		context;			// context to use (tablet or mouse)
	UINT		n1, n2, i;
	WCHAR		devicename[LCNAMELEN] = {0};
	HCTX		hResult;

	// get device count
	if (!(*WTInfoPtr)(WTI_INTERFACE, IFC_NDEVICES, &NDevices))
	{
		error("wacom: unable to get tablet info");
		return (NULL);
	}
	// else post("wacom: found %d devices", NDevices);

	if (x->x_device >= NDevices)
	{
		error("wacom: device number %d out of range", wDevice + 1);
		return (NULL);
	}

#if 0
	(*WTInfoPtr)(WTI_INTERFACE, IFC_NCONTEXTS, &n1);
	(*WTInfoPtr)(WTI_STATUS, STA_CONTEXTS, &n2);
	if (n2 >= n1)
	{
		error("wacom: out of device space (%d/%d)", n2, n1);
		return (NULL);
	}
	post("wacom: %d of %d wintab devices used", n2, n1);
#endif

	// get name of device and present it to the user
	(*WTInfoPtr)(WTI_DEVICES + wDevice, DVC_NAME, devicename);
	post("wacom: using device %d: \"%s\"", wDevice + 1, devicename);

	if (x->x_context)	// system context (moves cursor)
		context = WTI_DEFSYSCTX;
	else	// default context (does not move cursor)
		context = WTI_DEFCONTEXT;

	// get default or system context region
	if (!(*WTInfoPtr)(context, 0, &lcMine))
	{
		error("wacom: unable to get tablet info");
		return (NULL);
	}
	
	// modify the digitizing region
	wsprintf(lcMine.lcName, "Max Tablet %x", x->x_instance);
	lcMine.lcOptions |= CXO_MESSAGES;
	lcMine.lcPktRate = (UINT)(1000 / POLLTIME);	// we poll every 5ms
	lcMine.lcPktData = PACKETDATA;
	lcMine.lcPktMode = PACKETMODE;
	lcMine.lcMoveMask = PACKETDATA;
	lcMine.lcBtnUpMask = lcMine.lcBtnDnMask;
	lcMine.lcDevice = wDevice;					// set device number to use
	lcMine.lcSysMode = 0;						// absolute (relative = 1)

	// get number of buttons from lcBtnUpMask
	// -> every bit represents a button that we process
	x->x_nbuttons = 0;
	for (i = 0; i < sizeof(lcMine.lcBtnUpMask); i++)
	{
		if (lcMine.lcBtnUpMask & (1 << i))
			x->x_nbuttons++;
	}
	post("wacom: found %d buttons", x->x_nbuttons);

	// get device capabilities (i.e. max. values)
	(*WTInfoPtr)(WTI_DEVICES + wDevice, DVC_X, &tabletX);
	(*WTInfoPtr)(WTI_DEVICES + wDevice, DVC_Y, &tabletY);
	(*WTInfoPtr)(WTI_DEVICES + wDevice, DVC_NPRESSURE, &np);
	(*WTInfoPtr)(WTI_DEVICES + wDevice, DVC_ORIENTATION, &or);

	// set the entire tablet as active
	lcMine.lcInOrgX = 0;
	lcMine.lcInOrgY = 0;
	lcMine.lcInExtX = tabletX.axMax;
	lcMine.lcInExtY = tabletY.axMax;
	lcMine.lcOutOrgX = 0;
	lcMine.lcOutOrgY = 0;
	lcMine.lcOutExtX = tabletX.axMax;
	lcMine.lcOutExtY = tabletY.axMax;

	// save tablet's resolution ranges for later use
	x->x_res_x = (UINT)tabletX.axMax;
	x->x_res_y = (UINT)tabletY.axMax;
	x->x_res_press = (UINT)np.axMax;

	// does the tablet support tilt?
	if (or[0].axResolution && or[0].axResolution)
	{
		x->x_res_azi = or[0].axMax;
		x->x_res_alt = or[1].axMax;
	}
	else	// no, so lets set to 1 to avoid 'divide by zero'
	{		
		x->x_res_azi = 1;
		x->x_res_alt = 1;
	}

	if (or[2].axResolution > 0)
	{
		x->x_res_twist = or[2].axMax;
	}
	else	// set to 1 to avoid 'divide by zero'
	{	
		x->x_res_twist = 1;
	}

	// output using full value range provided by tablet
	lcMine.lcOutOrgX = 0;
	lcMine.lcOutOrgY = 0;
	lcMine.lcOutExtX = x->x_res_x;
	lcMine.lcOutExtY = x->x_res_y;

	// get number of cursors
	// (*WTInfoPtr)(WTI_INTERFACE, IFC_NCURSORS, &NCursors);
	// post("wacom: found %d cursors", NCursors);

	// open the region
	hResult = ((*WTOpenPtr)(hWnd, &lcMine, TRUE));

	// set the queue size
	while (!(*WTQueueSizeSetPtr)(hResult, x->x_queuesize))
	{
		// if setting the queue size fails, reduce the 
		// size by one and try again
		if (--x->x_queuesize == 0)
		{
			error("wacom: failed to set queue size");
			(*WTClosePtr)(hResult);	// close pointer to tablet
			return NULL;
		}
	}

	// set us on top of the overlap queue
	(*WTOverlapPtr)(hResult, TRUE);

	return hResult;
}


// cleanup the library (only called by last instance)
void wacom_tablet_free(t_wacom *x)
{
	if (winTabLib)
	{
		FreeLibrary(winTabLib);
	}
	WTInfoPtr = NULL;
	WTOpenPtr = NULL;
	WTClosePtr = NULL;
	WTPacketPtr = NULL;
	WTPacketsGetPtr = NULL;
	WTOverlapPtr = NULL;
	WTQueueSizeSetPtr = NULL;
	winTabLib = NULL;                            
}


// resolve the library functions (only called by first instance)
int wacom_tablet_new(t_wacom *x)
{
	// see if the user has the wintab library installed on their machine
	winTabLib = LoadLibrary("wintab32.dll");
	if (!winTabLib)
	{
		error("wacom: unable to find wintab32.dll");
		return (0);
	}

	// dynamically load all of the functions that we care about
	WTInfoPtr = (WTINFO_FUNC)GetProcAddress(winTabLib, (const char *)ORD_WTInfoA);
	if (!WTInfoPtr)
	{
		error("wacom: unable to find WinTab32 WTInfo()");
		goto error;
	}
	WTOpenPtr = (WTOPEN_FUNC)GetProcAddress(winTabLib, (const char *)ORD_WTOpen);
	if (!WTOpenPtr)
	{
		error("wacom: unable to find WinTab32 WTOpen()");
		goto error;
	}
	WTClosePtr = (WTCLOSE_FUNC)GetProcAddress(winTabLib, (const char *)ORD_WTClose);
	if (!WTOpenPtr)
	{
		error("wacom: unable to find WinTab32 WTClose()");
		goto error;
	}
	WTPacketPtr = (WTPACKET_FUNC)GetProcAddress(winTabLib, (const char *)ORD_WTPacket);
	if (!WTPacketPtr)
	{
		error("wacom: unable to find WinTab32 WTPacket()");
		goto error;
	}
	WTPacketsGetPtr = (WTPACKET_FUNC)GetProcAddress(winTabLib, (const char *)ORD_WTPacketsGet);
	if (!WTPacketsGetPtr)
	{
		error("wacom: unable to find WinTab32 WTPacketsGet()");
		goto error;
	}
	WTOverlapPtr = (WTOVERLAP_FUNC)GetProcAddress(winTabLib, (const char *)ORD_WTOverlap);
	if (!WTOverlapPtr)
	{
		error("wacom: unable to find WinTab32 WTOverlap()");
		goto error;
	}
	WTQueueSizeSetPtr = (WTQUEUESIZESET_FUNC)GetProcAddress(winTabLib, (const char *)ORD_WTQueueSizeSet);
	if (!WTQueueSizeSetPtr)
	{
		error("wacom: unable to find WinTab32 WTQueueSizeSetPtr()");
		goto error;
	}
	return (1);

error:
	error("wacom: make sure to have the latest version of wintab32.dll installed");
	wacom_tablet_free(x);
	return (0);
}
#endif // _WINDOWS


///////////////////////////////////////////////////////////
//
//  routines for outputting the data
//
///////////////////////////////////////////////////////////

// output x / y position and pressure
void wacom_motion(t_wacom *x, float x_pos, float y_pos, float pressure)
{
	x_pos -= x->x_x_offset;
	y_pos -= x->x_y_offset;

	outlet_float(x->x_out_press, (float)pressure);
	outlet_float(x->x_out_y_pos, (float)y_pos);
	outlet_float(x->x_out_x_pos, (float)x_pos);
}


// output rotation values (azimuth, altitude, twist)
void wacom_rotation(t_wacom *x, float azimuth, float altitude, float twist)
{
	outlet_float(x->x_out_twist, (float)twist);
	outlet_float(x->x_out_alt, (float)altitude);
	outlet_float(x->x_out_az, (float)azimuth);
}


// output button state
void wacom_button(t_wacom *x, unsigned short which, unsigned short state)
{
	t_atom at[2];

	if (which >= x->x_nbuttons)
		return;	// button number out of range

	SETLONG(at, which);
	SETLONG(at + 1, state);
	outlet_list(x->x_out_but, NULL, 2, at);
}


// output cursor type
void wacom_cursor(t_wacom *x, unsigned int cursor)
{
    outlet_int(x->x_out_cursor, (long)cursor);
}


///////////////////////////////////////////////////////////
//
//  routines call by the user
//
///////////////////////////////////////////////////////////

// switch polling on/off
#ifdef PD
void wacom_setpoll(t_wacom *x, t_floatarg state)
#else
void wacom_setpoll(t_wacom *x, long state)
#endif
{
	if ((short)state != x->x_poll)
	{
		if (x->x_poll)
			wacom_nopoll(x);
		else
			wacom_poll(x);

		x->x_poll = (short)state;
	}
}


// enable automatic polling
void wacom_poll(t_wacom *x)
{
	if (!x->x_poll)
	{
		x->x_poll = 1;
		clock_delay(x->x_clock, 0);
	}
}


// disable automatic polling
void wacom_nopoll(t_wacom *x)
{
	if (x->x_poll)
	{
		x->x_poll = 0;
		clock_unset(x->x_clock);
	}
}


// look for new packts (turns polling off!)
void wacom_bang(t_wacom *x)
{
	wacom_nopoll(x);
	// go get a packet
#ifdef PD
	wacom_packet(x);
#else
	qelem_set(x->x_qelem);
#endif
}


// set zero offset
#ifdef PD
void wacom_offset(t_wacom *x, t_floatarg x_off, t_floatarg y_off)
#else
void wacom_offset(t_wacom *x, double x_off, double y_off)
#endif
{
	x->x_x_offset = (float)x_off;
	x->x_y_offset = (float)y_off;
	post("wacom: set zero offset to %g / %g", x->x_x_offset, x->x_y_offset);
}


// set zero (0/0) to current pen position
void wacom_zero(t_wacom *x)
{
	x->x_x_offset = x->x_x_pos;
	x->x_y_offset = x->x_y_pos;
	post("wacom: set zero (0 / 0) to %g / %g", x->x_x_pos, x->x_y_pos);
}


// reset zero to 0/0 (bottom left corner)
void wacom_reset(t_wacom *x)
{
	x->x_x_offset = 0.;
	x->x_y_offset = 0.;
}


// change tablet mode (tablet / cursor)
#ifdef PD
void wacom_mode(t_wacom *x, t_floatarg mode)
#else
void wacom_mode(t_wacom *x, long mode)
#endif
{
	char *mode_name[] = {"tablet", "mouse"};
	if (mode) mode = 1;
	/* check whether value changed */
	if (x->x_context != (long)mode)
	{
		x->x_context = (long)mode;
		if (x->x_tablethandle)
		{
			wacom_close(x);
			wacom_open(x, x->x_device, x->x_context);
		}
		post("wacom: changed mode to \"%s\"", mode_name[x->x_context]);
	}
}


// set the tablet context to the front of the queue
void wacom_setfront(t_wacom *x)
{
	if (x->x_tablethandle)
	{
		(*WTOverlapPtr)(x->x_tablethandle, TRUE);
	}
}


///////////////////////////////////////////////////////////
//
//  low level stuff dealing with tablet directly
//
///////////////////////////////////////////////////////////

// get and process packets from the tablet
void *wacom_packet(t_wacom *x)
{
	PACKET packet[NPACKETQSIZE];
	int i, p;

#if 0
	static short visibility = 0;

#ifdef PD
	// check whether visibilty of canvas changed
	if ((short)glist_isvisible(x->x_canvas) != visibility)
	{
		visibility = (short)glist_isvisible(x->x_canvas);
		wacom_activate(x, visibility);
	}
#else
	// check whether visibilty of patcher-window changed
	if ((short)x->x_patcher->p_wind->w_vis != visibility)
	{
		visibility = (short)x->x_patcher->p_wind->w_vis;
		wacom_activate(x, visibility);
	}
#endif
#endif

	if (x->x_tablethandle)
	{
		// store 'old' values, we only send new ones in case they differ
		x->x_x_pos_old  = x->x_x_pos;
		x->x_y_pos_old  = x->x_y_pos;
		x->x_press_old  = x->x_press;
		x->x_azi_old    = x->x_azi;
		x->x_alt_old    = x->x_alt;
		x->x_twist_old  = x->x_twist;
		x->x_cursor_old = x->x_cursor;

		p = (*WTPacketsGetPtr)(x->x_tablethandle, x->x_queuesize, &packet);

		for (i = 0; i < p; i++)	// for all packets we got
		{
			// process and output packet data from right to left
			x->x_cursor = (unsigned int)packet[i].pkCursor;
			if (x->x_cursor != x->x_cursor_old)
			{
				wacom_cursor(x, x->x_cursor);
			}

			if (HIWORD(packet[i].pkButtons) == TBN_DOWN)
			{
				WORD which = LOWORD(packet[i].pkButtons);
				wacom_button(x, which, 1);
			}
			else if (HIWORD(packet[i].pkButtons) == TBN_UP)
			{
				WORD which = LOWORD(packet[i].pkButtons);
				wacom_button(x, which, 0);
			}

			x->x_azi = (float)packet[i].pkOrientation.orAzimuth / (float)x->x_res_azi;
			x->x_alt = (float)packet[i].pkOrientation.orAltitude / (float)x->x_res_alt;
			x->x_twist = (float)packet[i].pkOrientation.orTwist / (float)x->x_res_twist;
			if (x->x_azi != x->x_azi_old ||
				x->x_alt != x->x_alt_old ||
				x->x_twist != x->x_twist_old )
			{
				wacom_rotation(x, x->x_azi, x->x_alt, x->x_twist);
			}

			x->x_x_pos = (float)packet[i].pkX / (float)x->x_res_x;
			x->x_y_pos = (float)packet[i].pkY / (float)x->x_res_y;
			x->x_press = (float)packet[i].pkNormalPressure / (float)x->x_res_press;
			if (x->x_x_pos != x->x_x_pos_old ||
				x->x_y_pos != x->x_y_pos_old ||
				x->x_press != x->x_press_old )
			{
				wacom_motion(x, x->x_x_pos, x->x_y_pos, x->x_press);
			}
		}

		// flush packet queue, just in case we haven't read everything
		(*WTPacketsGetPtr)(x->x_tablethandle, 0, NULL);
	}
	return NULL;
}


// periodically look for data
void *wacom_tick(t_wacom *x)
{
	// get and process packets
#ifdef PD
	wacom_packet(x);
#else
	qelem_set(x->x_qelem);
#endif
	// set clock again if in polling mode
	if (x->x_poll)
		clock_delay(x->x_clock, POLLTIME);

	return NULL;
}


// close tablet
void wacom_close(t_wacom *x)
{
	if (x->x_tablethandle)
	{
		// set us to bottom of the overlap queue
		// thus bringing other instance to top
		(*WTOverlapPtr)(x->x_tablethandle, FALSE);

		// close pointer to tablet
		(*WTClosePtr)(x->x_tablethandle);

		x->x_tablethandle = NULL;
	}
}


// open tablet
#ifdef PD
void wacom_open(t_wacom *x, t_floatarg device, t_floatarg context)
#else
void wacom_open(t_wacom *x, long device, long context)
#endif
{
	// make sure tablet is closed
	wacom_close(x);

	// get device number
	if (device)
		x->x_device = (UINT)CLIP((UINT)(device - 1), 0, 8);
	else
		x->x_device = 0;

	x->x_context = context;		// tablet or mouse mode?

	// init tablet (this also gets the number if buttons)
	if (!(x->x_tablethandle = wacom_tablet_init(x, x->x_hwnd)))
	{
		error("wacom: failed to load tablet %d", device + 1);
		return;
	}
}


// print info about available devices
void wacom_print(t_wacom *x)
{
	UINT		NDevices;			// total number of devices
	UINT		NCursors;			// number of supported cursors

	// some advertisement
	post(wacom_version);

    // make sure the dll was actually loaded in
    if (!WTInfoPtr)
	{
		error("wacom: failed to access WinTab library");
        return;
	}

	// get device count
	if (!(*WTInfoPtr)(WTI_INTERFACE, IFC_NDEVICES, &NDevices))
	{
		error("wacom: unable to get tablet info");
		return;
	}
	else
	{
		WCHAR		buf[LCNAMELEN] = {0};
		BOOL		active;
		UINT		i;

		post("wacom: found %d devices:", NDevices);

		for (i = 0; i < NDevices; i++)
		{
			// get name of device
			(*WTInfoPtr)(WTI_DEVICES + i, DVC_NAME, buf);
			if (x->x_tablethandle != NULL && x->x_device == i)
				post("wacom:     device %d: \"%s\" (opened)", i + 1, buf);
			else
				post("wacom:     device %d: \"%s\"", i + 1, buf);
		}

		// get number of cursors
		(*WTInfoPtr)(WTI_INTERFACE, IFC_NCURSORS, &NCursors);
		post("wacom: found %d cursors:", NCursors);

		for (i = 0; i < NCursors; i++)
		{
			active = FALSE;
			(*WTInfoPtr)(WTI_CURSORS + i, CSR_ACTIVE, &active);
			(*WTInfoPtr)(WTI_CURSORS + i, CSR_NAME, buf);
			if (active)
				post("wacom:     cursor %d: \"%s\" (active)", i + 1, buf);
			else
				post("wacom:     cursor %d: \"%s\"", i + 1, buf);
		}
	}
}


#ifndef PD
// respond to the preset object
void wacom_preset(t_wacom *x)
{
	// preset stores: zero offset, tablet mode, polling
	preset_store("ossff", x, ob_sym(x), gensym("offset"), x->x_x_offset, x->x_y_offset);
	preset_store("ossl", x, ob_sym(x), gensym("mode"), x->x_context);
	preset_store("ossl", x, ob_sym(x), gensym("setpoll"), x->x_poll);
}


// assist: tell user what the inlets and outlets do
void wacom_assist(t_wacom *x, void *b, long m, long a, char *s)
{
	if (m == 1)	/* inlets */
	{
		switch (a)
		{	
			case 0:
				sprintf(s,"(bang) output current values");
				break;
			default:
				sprintf(s,"where did you get this inlet from?");
				break;
		}
	}
	else	/* outlets */
	{
		switch (a)
		{	
			case 0:
				sprintf(s,"(float) x position");
				break;
			case 1:
				sprintf(s,"(float) y position");
				break;
			case 2:
				sprintf(s,"(float) pressure");
				break;
			case 3:
				sprintf(s,"(float) azimuth");
				break;
			case 4:
				sprintf(s,"(float) altitude");
				break;
			case 5:
				sprintf(s,"(float) twist");
				break;
			case 6:
				sprintf(s,"(int) tool type");
				break;
			case 7:
				sprintf(s,"(list) buttons: <button> <state>");
				break;
			default:
				sprintf(s,"where did you get this outlet from?");
				break;
		}
	}
}
#endif	// !PD


void wacom_free(t_wacom *x)
{
	// close tablet
	wacom_close(x);

	clock_free(x->x_clock);
#ifndef PD
	qelem_free(x->x_qelem);
#endif

	// free the tablet stuff if this was the last instance
	if (--wacom_objcount == 0)
	{
		wacom_tablet_free(x);
	}
}

#ifdef PD
void *wacom_new(t_floatarg fdevice, t_floatarg fcontext)
{
	long device  = (long)fdevice;
	long context = (long)fcontext;
	HWND hWnd;

	t_wacom *x = (t_wacom *)pd_new(wacom_class);

	// get canvas we're sitting in
	x->x_canvas = canvas_getcurrent();

	// get the window which will be sending messages about 
	// the tablet.  Using the desktop window since this
	// class is not using any particular window.
	hWnd = GetDesktopWindow();

#else	// Max

void *wacom_new(long device, long context, long queuesize)
{
	unsigned int i;
	HWND hWnd;

	t_wacom *x = (t_wacom *)newobject(wacom_class);

    // zero out the struct, to be careful
    if (x)
    { 
        for (i = sizeof(t_object); i < sizeof(t_wacom); i++)  
                ((char *)x)[i] = 0; 
	}

	// get our patcher window -- this must be in our instance creation function
	x->x_patcher = (t_patcher *)(gensym("#P")->s_thing);
	// hWnd = patcher_gethwnd(p);	// this function returns NULL in case patcher not yet created

	// in Max we use the window handle of the patcher our
	// object is living in
	hWnd = main_get_client();

#endif	// PD

	// check the window handle
	if (!hWnd || hWnd == INVALID_HANDLE_VALUE)
	{
		// could not open window context, so giving up...
		error("wacom: failed to get window handle");
		if (hWnd == INVALID_HANDLE_VALUE)
			error("hWnd == INVALID_HANDLE_VALUE");
		return NULL;
	}

	// set default values
	x->x_hwnd = hWnd;
	x->x_x_offset = 0.;
	x->x_y_offset = 0.;
	x->x_poll = 1;		// poll for input automagically
	if (queuesize)
		x->x_queuesize = CLIP(queuesize, 1, 128);
	else
		x->x_queuesize = NPACKETQSIZE;	// set default

	x->x_instance = ++wacom_instance;	// count number of instances of this object
	if (wacom_objcount == 0)			// init WinTab if this is first instance
	{
		// load tablet functions
		if (!wacom_tablet_new(x))
			return NULL;				// don't create object in case this failed!
	}
	// count number of objects currently being used
	wacom_objcount++;

	// open the device and check tablet handle
	wacom_open(x, device, context);
	if (!x->x_tablethandle)
	{
		// return NULL;
	}


#ifdef PD

	// create outlets from left to right
    x->x_out_x_pos  = outlet_new(&x->x_obj, gensym("float"));
    x->x_out_y_pos  = outlet_new(&x->x_obj, gensym("float"));
    x->x_out_press  = outlet_new(&x->x_obj, gensym("float"));
    x->x_out_az     = outlet_new(&x->x_obj, gensym("float"));
    x->x_out_alt    = outlet_new(&x->x_obj, gensym("float"));
    x->x_out_twist  = outlet_new(&x->x_obj, gensym("float"));
    x->x_out_cursor = outlet_new(&x->x_obj, gensym("float"));
	x->x_out_but    = outlet_new(&x->x_obj, gensym("list"));

	x->x_clock = clock_new(x, (t_method)wacom_tick);

#else	// Max

	// create outlets from right to left
	x->x_out_but    = listout(x);
    x->x_out_cursor = intout(x);
    x->x_out_twist  = floatout(x);
    x->x_out_alt    = floatout(x);
    x->x_out_az     = floatout(x);
    x->x_out_press  = floatout(x);
    x->x_out_y_pos  = floatout(x);
    x->x_out_x_pos  = floatout(x);

	x->x_clock = clock_new(x, (method)wacom_tick);
	x->x_qelem = qelem_new(x, (method)wacom_packet);

#endif

	// start polling and return pointer to object
	clock_delay(x->x_clock, 0);
	return x;
}
