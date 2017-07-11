/*
	File:		BIXcp.to.c

	Contains:	Max/Jitter external object converting Jitter matrices to the BIX CP protocol

	Written by:	Peter Castine

	Copyright:	© 2003 Peter Castine

	Change History (most recent first):

         <2>   12–8–2005    pc      #include bix.h after LiterLib.h (Windows needs typedef for
                                    Byte). Other changes for LBJ-inclusion.
         <1>      5–3–05    pc      Initial check in.
*/


/******************************************************************************************
	Previous history:
		16-Jul-2003:		First implementation.
 ******************************************************************************************/

#pragma mark • Identify Target

#ifdef __MWERKS__
	#if __ide_target("BIX Pack (Boron)")								\
			|| __ide_target("BIX Pack (Carbon)")						\
			|| __ide_target("BIX Pack (Windows)")
		
		// The target object is to be distributed as part of Litter Bundle Jitter
		#define __EX_LIBRIS__	0

	#elif	__ide_target("XL-BIX To (Boron)")							\
			|| __ide_target("XL-BIX To (Carbon)")						\
			|| __ide_target("XL-BIX To (Windows)") 
		
		// Building outside Litter Bundle Jitter
		#define __EX_LIBRIS__	1

	#else
		#error "Unknown target"
	#endif
#else
		#define __EX_LIBRIS__ 0
#endif

#pragma mark • Include Files

#include "LitterLib.h"
#include "TrialPeriodUtils.h"

#include "bix.h"



#pragma mark • Constants

#if __EX_LIBRIS__
	const char	kMaxClassName[]	= "jit.bixcp.to",		// Class name for Max, also used
														// for resources.
				kJitClassName[]	= "jit-bixcp-to";		// Class name for Jitter
#else
	const char	kMaxClassName[]	= "lbj.bixpack",		// Class name for Max, also used
														// for resources.
				kJitClassName[]	= "lbj-bixpack";		// Class name for Jitter
#endif	

	// Indices for STR# resource
enum strIndices {
	strIndexInMatrix		= lpStrIndexLastStandard + 1,
	
	strIndexOutFullPacket,
	strIndexOutDump,
	
	strIndexInLeft		= strIndexInMatrix,
	strIndexOutLeft		= strIndexOutFullPacket
	};
	

#pragma mark • Object Structure

typedef struct {
	Object		coreObject;
	voidPtr		jitObject;
	} msobBixCP;						// msob == Max Shell Object

typedef struct {
	Object			coreObject;
	
	Boolean			monochrome,			// False by default; ie, Grayscale
					swapbytes;
	long			bufSize;
	tBixCPPicturePtr bixBuf;
	} jcobBixCP;						// jcob == Jitter Core Object

#pragma mark • Global Variables

void*			gBixCPJitClass	= NIL;
Messlist*		gBixCPMaxClass	= NIL;

Symbol*			gSymFullPacket	= NIL;


#pragma mark • Function Prototypes

	// Max methods/functions
static msobBixCP*	BixCPNewMaxShell	(Symbol*, long, Atom*);
static void			BixCPFreeMaxShell	(msobBixCP*);

static void BixCPjit_matrix		(msobBixCP*, Symbol*, long, Atom[]);
static void BixCPFilter			(msobBixCP*, long, long, long);
static void	BixCPSleep			(msobBixCP*, long);
static void	BixCPDevice			(msobBixCP*, long);

static void BixCPTattle			(msobBixCP*);
static void	BixCPAssist			(msobBixCP*, void* , long , long , char*);
static void	BixCPInfo			(msobBixCP*);

	// Jitter methods/functions
static t_jit_err BixCPJitInit		(void);
static t_jit_err BixCPTranslate		(jcobBixCP*, void*, long*, tBixCPPicturePtr*);


#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark • Inline Functions

	// ?? Need to make sure how the compiler handles static inside an inline!!
	// (Should be OK because the function is internal, but watch out)
static inline unsigned long GetNextMessageID(void)
	{ static unsigned long sCurMsgID = 0; return ++sCurMsgID; }

static inline void* MainOutlet(msobBixCP* me)
	{ return me->coreObject.o_outlet; }
	
#pragma mark -

/******************************************************************************************
 *
 *	main()
 *	
 *	Standard Max External Object Entry Point Function
 *	
 ******************************************************************************************/

void
main(void)
	
	{
	voidPtr	mjClassEx,
			jClass;
	
	LITTER_CHECKTIMEOUT(kMaxClassName);
	
	BixCPJitInit();
	
	// Standard Max setup() call
	setup(	&gBixCPMaxClass,					// Pointer to our class definition
			(method) BixCPNewMaxShell,			// Instance creation function
			(method) BixCPFreeMaxShell,			// Custom deallocation function
			(short) sizeof(msobBixCP),			// Class object size
			NIL,								// No menu function
			A_GIMME,							// Jitter parses its own arguments
			0);		
	
	// Jitter Magic... 
	mjClassEx	= max_jit_classex_setup( calcoffset(msobBixCP, jitObject) );
	jClass		= jit_class_findbyname( gensym((char*) kJitClassName) );    
    max_jit_classex_standard_wrap(mjClassEx, jClass, 0); 	
	
	// Add messages
	//
		// Processing messages
	LITTER_TIMEBOMB addmess ((method) BixCPjit_matrix, "jit_matrix", A_GIMME, 0);
	addmess ((method) BixCPFilter,	"filter",	A_LONG, A_LONG, A_LONG, 0);
	addmess	((method) BixCPSleep,	"sleep",	A_LONG, 0);
	addmess ((method) BixCPDevice,	"device",	A_LONG,	0);
		// Informational messages
	addmess	((method) BixCPTattle,	"dblclick",	A_CANT, 0);
	addmess	((method) BixCPTattle,	"tattle",	A_NOTHING);
	addmess	((method) BixCPAssist,	"assist",	A_CANT, 0);
	addmess	((method) BixCPInfo,	"info",		A_CANT, 0);
	
	// Cache commonly used symbol
	gSymFullPacket	= gensym("FullPacket");
		
	// Initialize Litter Library
	LitterInit(kMaxClassName, 0);
	
	}


#pragma mark -
#pragma mark • Internal functions

/******************************************************************************************
 *
 *	FullPacketOut(me, iSize, iBuf)
 *	
 *	Sets up a vector of Atoms to send a FullPacket message through the main outlet.
 *
 ******************************************************************************************/

static void
FullPacketOut(
	msobBixCP*	me,
	long		iSize,		// Size of buffer
	void*		iBuf)		// Address where the buffered data are stored
	
	{
	const short kArgCount	= 2;
	
	Atom	args[kArgCount];

	AtomSetLong(&args[0], iSize);
	AtomSetLong(&args[1], (long) iBuf);
	
	outlet_anything(MainOutlet(me), gSymFullPacket, kArgCount, args);
	}


#pragma mark -
#pragma mark • Max Shell Methods

/******************************************************************************************
 *
 *	BixCPNewMaxShell(iSym, iArgC, iArgV)
 *
 ******************************************************************************************/

static msobBixCP*
BixCPNewMaxShell(
	SymbolPtr	sym,
	long		iArgC,
	Atom		iArgV[])
	
	{
	#pragma unused(sym)
	
	msobBixCP*	me			= NIL;
	void*		jitObj		= NIL;
	Symbol*		classSym	= gensym((char*) kJitClassName);
	
	// In the 1.0 SDK max_jit_obex_new() is prototyped to expect a Maxclass* instead of
	// a Messlist*. JKC said the next release of header files would be corrected to
	// use Messlist* and in the interim users can edit the file accordingly.
	// If we get a compiler "illegal implicit typecast" error, it means Josh forgot to
	// make the change; go to jit.max.h and do it again.
	me = (msobBixCP*) max_jit_obex_new(gBixCPMaxClass, classSym);
		if (me == NIL) goto punt;
	
	max_jit_obex_dumpout_set( me, outlet_new(me, NIL) );
	outlet_new(me, gSymFullPacket->s_name);
		
	jitObj = jit_object_new(classSym);
		if (jitObj == NIL) goto punt;
	
	max_jit_obex_jitob_set(me, jitObj);			
	max_jit_attr_args(me, iArgC, iArgV);
	
	return me;
	// ------------------------------------------------------------------------
	// End of normal processing
	
	// Poor man's exception handling
punt:
	error("%s: could not allocate object", kMaxClassName);
	if (me != NIL)
		freeobject(&me->coreObject);
		
	return NIL;
	}

/******************************************************************************************
 *
 *	BixCPFreeMaxShell(me)
 *
 ******************************************************************************************/

static void
BixCPFreeMaxShell(
	msobBixCP* me)
	
	{
	jit_object_free( max_jit_obex_jitob_get(me) );
	max_jit_obex_free(me);
	}


/******************************************************************************************
 *
 *	BixCPOutputMatrix(me)
 *
 ******************************************************************************************/

static void
BixCPjit_matrix(
	msobBixCP*	me,
	Symbol*		sym,
	long		iArgC,
	Atom		iArgV[])
	
	{
	#pragma unused(sym)
	
	void*			matrix;
	t_jit_err		err	= JIT_ERR_NONE;
	tBixCPPicture*	buf		= NIL;
	long			bufSize	= 0;

	// Sanity check
	if ((iArgV == NIL) || (iArgV[0].a_type != A_SYM) || iArgC != 1) {
		err = JIT_ERR_INVALID_INPUT;
		goto punt;
		}
	
	matrix = jit_object_findregistered(AtomGetSym(&iArgV[0]));
	if (matrix == NIL || jit_object_method(matrix, _jit_sym_class_jit_matrix) == NIL) {
		err = JIT_ERR_MATRIX_UNKNOWN;
		goto punt;
		}
		
	err = BixCPTranslate(max_jit_obex_jitob_get(me), matrix, &bufSize, &buf);
	if (err != JIT_ERR_NONE)
		goto punt;
		
	FullPacketOut(me, bufSize, buf);
	return;
	// ====================================================================================
	// End of normal processing
	
	
	// Poor man's exception handling
punt:
	jit_error_code(me, err);
	}


/******************************************************************************************
 *
 *	BixCPFilter(me)
 *	BixCPSleep(me)
 *	BixCPDevice(me)
 *
 *	Additional messages for the BIX Control Protocol
 *
 ******************************************************************************************/

static void
BixCPFilter(
	msobBixCP*	me,
	long		iWhich,
	long		iOnOff,
	long		iValue)
	
	{
	const long		kMinByte	= 0,
					kMaxByte	= 255;
	const Boolean	kSwapBytes	= ((jcobBixCP*) (me->jitObject))->swapbytes;
	
	tBixCPSelFilter	buf;
	
	// Sanity checks
	CLIP(iWhich, kMinByte, kMaxByte);
	CLIP(iValue, kMinByte, kMaxByte);
	iOnOff = (iOnOff != 0);
	
	buf.magic	= NETORDER_INT32( bixMagicSelFilter );
	buf.msgID	= NETORDER_INT32( GetNextMessageID() );

	// Are we forcing a byte swap?
	if (kSwapBytes) {
		buf.magic	= SWAP32(buf.magic);
		buf.msgID	= SWAP32(buf.msgID);
		}
	
		// The following are single bytes, so no need to muck with byte order
	buf.filtNum	= iWhich;
	buf.filtOn	= iOnOff;
	buf.filtVal	= iValue;
	
	FullPacketOut(me, sizeof(buf), &buf);
	}

static void
BixCPSleep(
	msobBixCP*	me,
	long		iDur)
	
	{
	const int		kMinDur		= 0,
					kMaxDur		= 1000;
	const Boolean	kSwapBytes	= ((jcobBixCP*) (me->jitObject))->swapbytes;
	
	tBixCPSleep	buf;
	
	// Sanity check
	CLIP(iDur, kMinDur, kMaxDur);
	
	buf.magic	= NETORDER_INT32( bixMagicSleep );
	buf.msgID	= NETORDER_INT32( GetNextMessageID() );
	buf.time	= NETORDER_INT32( iDur );
	
	// Are we forcing a byte swap?
	if (kSwapBytes) {
		buf.magic	= SWAP32(buf.magic);
		buf.msgID	= SWAP32(buf.msgID);
		buf.time	= SWAP32(buf.time);
		}
	
	FullPacketOut(me, sizeof(buf), &buf);
	}

static void
BixCPDevice(
	msobBixCP*	me,
	long		iOnOff)
	
	{
	const long		kBixCPDeviceCtrlBufSize = 9;
	const Boolean	kSwapBytes	= ((jcobBixCP*) (me->jitObject))->swapbytes;
	
	tBixCPDeviceCtrl	buf;
	
	// Sanity check
	iOnOff = (iOnOff != 0);
	
	buf.magic		= NETORDER_INT32( bixMagicDeviceCtrl );
	buf.msgID		= NETORDER_INT32( GetNextMessageID() );

	// Are we forcing a byte swap?
	if (kSwapBytes) {
		buf.magic	= SWAP32(buf.magic);
		buf.msgID	= SWAP32(buf.msgID);
		}

		// The following is a single byte, so no need to muck with byte order
	buf.deviceOn	= iOnOff;
	
	FullPacketOut(me, sizeof(buf), &buf);
	}


/******************************************************************************************
 *
 *	BixCPAssist(me, iBox, iDir, iArgNum, oCStr)
 *	BixCPInfo(me)
 *	BixCPTattle(me)
 *
 *	Fairly generic Assist/Info/Tattle Methods
 *
 ******************************************************************************************/

void BixCPAssist(msobBixCP* me, void* box, long iDir, long iArgNum, char* oCStr)
	{
	#pragma unused(me, box)
	
	LitterAssist(iDir, iArgNum, strIndexInLeft, strIndexOutLeft, oCStr);
	}

void BixCPInfo(msobBixCP* me)
	{ LitterInfo(kMaxClassName, &me->coreObject, (method) BixCPTattle); }

void
BixCPTattle(
	msobBixCP* me)
	
	{
	jcobBixCP*	jcob = (jcobBixCP*) max_jit_obex_jitob_get(me);
	
	post("%s state", kMaxClassName);
	
	post("  sending %s picture data", jcob->monochrome ? "monochrome" : "grayscale");
	post("  UDP data buffer is %ld bytes stored at %p", jcob->bufSize, jcob->bixBuf);
	
	}


#pragma mark -
#pragma mark • Jitter Methods

/******************************************************************************************
 *
 *	BixCPJitMatrixCalc(me, inputs, outputs)
 *	BixCPJitFree(me)
 *
 ******************************************************************************************/

static jcobBixCP*
BixCPJitNew()
	{
	jcobBixCP* me = (jcobBixCP*) jit_object_alloc(gBixCPJitClass);
	
	if (me != NIL) {
		me->monochrome	= false;
		me->swapbytes	= false;
		me->bufSize		= 0;
		me->bixBuf		= NIL;
		}
		
	return me;
	}

static void BixCPJitFree(jcobBixCP* me)
	{ 
	#pragma unused(me)
	/* nothing to do */
	}


/******************************************************************************************
 *
 *	BixCPTranslate(me, inputs, outputs)
 *
 ******************************************************************************************/

	static t_jit_err CheckInfo(t_jit_matrix_info* iMInfo)
		{
		long bufSize;
		
		if (iMInfo == NIL)
			return JIT_ERR_INVALID_INPUT;
		
		if (iMInfo->type != _jit_sym_char)
			return JIT_ERR_MISMATCH_TYPE;
		
		if (iMInfo->dimcount != kBixDimCount)
			return JIT_ERR_MISMATCH_DIM;
		
		if (iMInfo->planecount != kBixPlaneCount)
			return JIT_ERR_MISMATCH_PLANE;
			
		bufSize = CalcBufSize(iMInfo->dim[0], iMInfo->dim[1]);
		if (bufSize <= 0 || kMaxBufSize < bufSize)
			return JIT_ERR_OUT_OF_BOUNDS;
		
		if (iMInfo->dim[0] > kBIXWidth || iMInfo->dim[1] > kBIXHeight) {
			// This is not an error, but the user should be warned.
			// But not too often.
			const  unsigned long kWarnInterval = 216000;		// 1 hour
			static unsigned long sLastWarnTime = 0;
			
			unsigned long now = TickCount();
			
			if (sLastWarnTime == 0 || now > sLastWarnTime + kWarnInterval) {
				post("%swarning: %s: picture dimensions larger than physical limits",
					(sLastWarnTime > 0) ? "hourly " : "", kMaxClassName);
				sLastWarnTime = now;
				}
			}
		
		return JIT_ERR_NONE;
		}

static t_jit_err
BixCPTranslate(
	jcobBixCP*		me,
	void*			iMatrix,
	long*			oBufSize,
	tBixCPPicture**	oBixBuf)
	
	{
	t_jit_err			err = JIT_ERR_NONE;
	t_jit_matrix_info	mInfo;
	char*				mData;
	long				saveLock;
	
	// Sanity check
	if ((me == NIL) || (iMatrix == NIL))
		return JIT_ERR_INVALID_PTR;
	
	// ------------------------------------------------------------------------------------
	// Locking matrix now
	// State must be reset before leaving function
	//
	saveLock = (long) jit_object_method(iMatrix, _jit_sym_lock, 1L);
	
	// Get/check matrix data
	jit_object_method(iMatrix, _jit_sym_getdata, &mData);
	if (mData == NIL) {
		err = JIT_ERR_INVALID_INPUT;
		goto alohamora;
		}
	
	// Get matrix info and check that we can deal with it
	jit_object_method(iMatrix, _jit_sym_getinfo, &mInfo);
	err = CheckInfo(&mInfo);
	if (err != JIT_ERR_NONE)
		goto alohamora;
	
	// Data looks good. Build buffer.
	// CheckInfo() was also called inside CalcBufSize(), but it seems cleaner to calculate
	// it again then to rely on getting the value as a side effect of another function.
	// Anyway, the function is (a) inlined and (b) dirt cheap.
	{
	long	width	= mInfo.dim[0],
			height	= mInfo.dim[1],
			bufSize	= CalcBufSize(width, height);
	
	if (bufSize != me->bufSize) {
		if (me->bixBuf != NIL)
			freebytes(me->bixBuf, me->bufSize);
		
		me->bixBuf = (tBixCPPicture*) getbytes(bufSize);
		if (me->bixBuf == NIL) {
			me->bufSize = 0;
			err = JIT_ERR_OUT_OF_MEM;
			goto alohamora;
			}
		
		me->bufSize = bufSize;
		}
	
	// ASSERT: (me->bufSize == bufSize)
	me->bixBuf->msgID	= NETORDER_INT32( GetNextMessageID() );
	me->bixBuf->magic	= NETORDER_INT32( me->monochrome ? bixMagicMono : bixMagicGray );
	me->bixBuf->width	= NETORDER_INT16( width );
	me->bixBuf->height	= NETORDER_INT16( height );
	
	// Are we forcing a byte swap?
	if (me->swapbytes) {
		me->bixBuf->msgID	= SWAP32(me->bixBuf->msgID);
		me->bixBuf->magic	= SWAP32(me->bixBuf->magic);
		me->bixBuf->width	= SWAP16(me->bixBuf->width);
		me->bixBuf->height	= SWAP16(me->bixBuf->height);
		}
	
	// Luckily, we are dealing with byte-sized data, so we don't have to worry about
	// network byte order here.
	if (mInfo.dimstride[1] == width)
		BlockMoveData(mData, me->bixBuf->data, width * height);
	else {
		// Jitter is padding rows, so we must copy one row at a time
		long	i		= height,
				stride	= mInfo.dimstride[1];
		BytePtr	source	= (BytePtr) mData,
				dest	= me->bixBuf->data;
		
		while (i-- > 0) {
			BlockMoveData(source, dest, width);
			source	+= stride;
			dest	+= width;
			}
		}
	
	*oBufSize	= bufSize;
	*oBixBuf	= me->bixBuf;
	}	
	
alohamora:
	jit_object_method(iMatrix, _jit_sym_lock, saveLock);
	//
	// Matrix lock state reset
	// Safe to leave function
	// ------------------------------------------------------------------------------------

	return err;
	}
	

/******************************************************************************************
 *
 *	BixCPJitInit(me)
 *
 ******************************************************************************************/

t_jit_err
BixCPJitInit(void) 

	{
	const long	kAttrFlags			= JIT_ATTR_GET_DEFER_LOW | JIT_ATTR_SET_USURP_LOW,
				kMonochromeOffset	= calcoffset(jcobBixCP, monochrome),
				kSwapbyteOffset		= calcoffset(jcobBixCP, swapbytes);
	
	tObjectPtr	attr,
				mop;
	
	gBixCPJitClass = jit_class_new(	(char*) kJitClassName,
									(method) BixCPJitNew,
									(method) BixCPJitFree,		// ?? Could be NIL ??
									sizeof(jcobBixCP),
									A_CANT, 0L
									); 

	// Add Matrix Operator
	mop = jit_object_new(_jit_sym_jit_mop, 1, 1);
	jit_class_addadornment(gBixCPJitClass, mop);
	
	//
	// Add attributes
	//
	
	// Attribute mono
	attr = jit_object_new(_jit_sym_jit_attr_offset, "mono",
							_jit_sym_char, kAttrFlags,
							NIL, NIL, kMonochromeOffset);
	jit_attr_addfilterset_clip(attr, 0, 1, true, true);		// Clip to [0 .. 1]
	jit_class_addattr(gBixCPJitClass, attr);
	
	// Attribute swapbytes
	attr = jit_object_new(_jit_sym_jit_attr_offset, "swapbytes",
							_jit_sym_char, kAttrFlags,
							NIL, NIL, kSwapbyteOffset);
	jit_attr_addfilterset_clip(attr, 0, 1, true, true);		// Clip to [0 .. 1]
	jit_class_addattr(gBixCPJitClass, attr);
	
	jit_class_register(gBixCPJitClass);

	return JIT_ERR_NONE;
	}

