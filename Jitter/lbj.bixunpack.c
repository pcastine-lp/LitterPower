/*
	File:		lbj.bixunpack.c

	Contains:	Max/Jitter external object converting data from the BIX CP protocol into
				Jitter matrices

	Written by:	Peter Castine

	Copyright:	© 2003 Peter Castine

	Change History (most recent first):

         <2>   12Ð8Ð2005    pc      #include bix.h after LiterLib.h (Windows needs typedef for
                                    Byte). Other changes for LBJ-inclusion.
         <1>      5Ð3Ð05    pc      Initial check in.
*/


/******************************************************************************************
	Previous history:
		30-Jul-2003:		First implementation.
 ******************************************************************************************/

#pragma mark ¥ Identify Target

#ifdef __MWERKS__
	#if __ide_target("BIX Unpack (Boron)")								\
			|| __ide_target("BIX Unpack (Carbon)")						\
			|| __ide_target("BIX Unpack (Windows)")
		
		// The target object is to be distributed as part of Litter Bundle Jitter
		#define __EX_LIBRIS__	0

	#elif	__ide_target("XL-BIX From (Boron)")							\
			|| __ide_target("XL-BIX From (Carbon)")						\
			|| __ide_target("XL-BIX From (Windows)") 
		
		// Building outside Litter Bundle Jitter
		#define __EX_LIBRIS__	1

	#else
		#error "Unknown target"
	#endif
#else
	#define __EX_LIBRIS__ 0
#endif


#pragma mark ¥ Include Files

#include "LitterLib.h"
#include "TrialPeriodUtils.h"

t_jit_err jit_ob3d_set(void *x, void *p);

#include "bix.h"



#pragma mark ¥ Constants

#if __EX_LIBRIS__
	const char	kMaxClassName[]	= "jit.bixcp.from",		// Class name for Max, also used
														// for resources.
				kJitClassName[]	= "jit-bixcp-from";		// Class name for Jitter
#else
	const char	kMaxClassName[]	= "lbj.bixunpack",		// Class name for Max, also used
														// for resources.
				kJitClassName[]	= "lbj-bixunpack";		// Class name for Jitter
#endif	

	// Indices for STR# resource
enum strIndices {
	strIndexInFullPacket		= lpStrIndexLastStandard + 1,
	
	strIndexOutMatrix,
	strIndexOutDump,
	
	strIndexInLeft		= strIndexInFullPacket,
	strIndexOutLeft		= strIndexOutMatrix
	};
	
#pragma mark ¥ BIXCP Data

typedef struct {
	long	magic,
			msgID;
	} tBixCPGeneric;
typedef tBixCPGeneric* tBixCPGenericPtr;
	


#pragma mark ¥ Object Structure

typedef struct {
	Object		coreObject;
	voidPtr		jitObject;
	tOutletPtr	idOut;				// Additional outlet for message IDs			
	Symbol*		matrixSym;
	voidPtr		matrix;
	Boolean		autoswapbytes;		// If true, test for endianness of incoming packets and 
									// switch if we recognize data coming in that were not
									// properly written to Network Byte Order
									
	} msobBixCP;					// msob == Max Shell Object


#pragma mark ¥ Global Variables

void*			gBixCPJitClass	= NIL;
Messlist*		gBixCPMaxClass	= NIL;

SymbolPtr		gsymJitMatrix		= NIL,
				gsymSleep			= NIL,
				gsymSleepOK			= NIL,
				gsymSelFilter		= NIL,
				gsymDeviceCtrl		= NIL,
				gsymDeviceCtrlOK	= NIL;


#pragma mark ¥ Function Prototypes

	// Max methods/functions
static msobBixCP*	BixCPNew	(Symbol*, long, Atom*);
static void			BixCPFree	(msobBixCP*);

static void BixCPFullPacket		(msobBixCP*, long, tBixCPGeneric*);	// Lie to Max about third parameter

static void BixCPTattle			(msobBixCP*);
static void	BixCPAssist			(msobBixCP*, void* , long , long , char*);
static void	BixCPInfo			(msobBixCP*);

	// Jitter methods/functions
//static t_jit_err BixCPJitInit		(void);
//static t_jit_err BixCPTranslate		(jcobBixCP*, void*, long*, tBixCPPicturePtr*);


#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark ¥ Inline Functions

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
	const long kAttrFlags = JIT_ATTR_GET_DEFER_LOW + JIT_ATTR_SET_DEFER_LOW;
	
		// Have to guess about what the following do.
		// Not much is documented in the Jitter SDK
	voidPtr	mjClassEx,
			attrPtr;
	
	LITTER_CHECKTIMEOUT(kMaxClassName);
	
	// Standard Max setup() call
	setup(	&gBixCPMaxClass,					// Pointer to our class definition
			(method) BixCPNew,					// Instance creation function
			(method) BixCPFree,					// Custom deallocation function
			(short) sizeof(msobBixCP),			// Class object size
			NIL,								// No menu function
			A_GIMME,							// Jitter parses its own arguments
			0);		
	
	// Jitter Magic... 
	mjClassEx	= max_jit_classex_setup( calcoffset(msobBixCP, jitObject) );
	
	/*
	 *	Add messages and attributes
	 */
		// Processing messages
	LITTER_TIMEBOMB addmess ((method) BixCPFullPacket, "FullPacket", A_LONG, A_LONG, 0);
	
		// Attributes
	attrPtr = jit_object_new(	_jit_sym_jit_attr_offset,
								"matrix_name",
								_jit_sym_symbol,
								kAttrFlags,
								NIL, NIL,
								calcoffset(msobBixCP, matrixSym) );
	max_jit_classex_addattr(mjClassEx, attrPtr);
	
		// Wrap up class
	max_jit_classex_standard_wrap(mjClassEx, NIL, 0); 	
	
		// Tack on informational messages
	addmess	((method) BixCPTattle,	"dblclick",	A_CANT, 0);
	addmess	((method) BixCPTattle,	"tattle",	A_NOTHING);
	addmess	((method) BixCPAssist,	"assist",	A_CANT, 0);
	addmess	((method) BixCPInfo,	"info",		A_CANT, 0);
	
	
	// Cache commonly used symbols
	gsymJitMatrix		= gensym("jit_matrix");
	gsymSleep			= gensym("sleep");
	gsymSleepOK			= gensym("sleepOK");
	gsymSelFilter		= gensym("filter");
	gsymDeviceCtrl		= gensym("device");
	gsymDeviceCtrlOK	= gensym("deviceOK");
	
	// Initialize Litter Library
	LitterInit(kMaxClassName, 0);
	
	}


#pragma mark -
#pragma mark ¥ Max Shell Methods

/******************************************************************************************
 *
 *	BixCPNewMaxShell(iSym, iArgC, iArgV)
 *
 ******************************************************************************************/

static msobBixCP*
BixCPNew(
	SymbolPtr	sym,
	long		iArgC,
	Atom		iArgV[])
	
	{
	#pragma unused(sym)
	
	msobBixCP*			me			= NIL;
	long				attrStart	= 0;
	voidPtr				matrix		= NIL;
	t_jit_matrix_info	info;
	
	// In the 1.0 SDK max_jit_obex_new() is prototyped to expect a Maxclass* instead of
	// a Messlist*. JKC said the next release of header files would be corrected to
	// use Messlist* and in the interim users can edit the file accordingly.
	// If we get a compiler "illegal implicit typecast" error, it means Josh forgot to
	// make the change; go to jit.max.h and do it again.
	me = (msobBixCP*) max_jit_obex_new(gBixCPMaxClass, NIL);		// No Jitter object
		if (me == NIL) goto punt;
	
	// Create outlets
	max_jit_obex_dumpout_set( me, outlet_new(me, NIL) );
	me->idOut = intout(me);
	outlet_new(me, "jit_matrix");						// Access via me->coreObject.o_outlet
	
	// Set up object members
	me->matrixSym		= jit_symbol_unique();
	me->autoswapbytes	= true;							// This is a bit brutal. Should attributize this.
														// Or at least make it an option at instantiation time.
		
	// Let Jitter parse initialization arguments
	max_jit_attr_args(me, iArgC, iArgV);
	
	// Let Jitter create the actual matrix member
	jit_matrix_info_default(&info);
	info.type		= _jit_sym_char;
	info.dimcount	= 2;
	info.dim[0]		= kBIXWidth;
	info.dim[1]		= kBIXHeight;
	info.planecount	= 1;
	matrix = jit_object_method(jit_object_new(_jit_sym_jit_matrix, &info), _jit_sym_register, me->matrixSym);
	if (matrix == NIL)
		error("%s: ", kMaxClassName);
	jit_object_attach(me->matrixSym, me);
	me->matrix = matrix;
	
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
BixCPFree(
	msobBixCP* me)
	
	{
	jit_object_detach(me->matrixSym, me);
	jit_object_free(me->matrix);
	max_jit_obex_free(me);
	}


/******************************************************************************************
 *
 *	BixCPFullPacket(me)
 *
 ******************************************************************************************/

#ifdef __GNUC__

		// Lame gcc can't handle symbolic constants in switch statements
	#define kJitErrPacketWrongSize	FOUR_CHAR('bSiz')
	#define kJitErrUnknownBIXCP		FOUR_CHAR('bMag')

#else

	const t_jit_err	kJitErrPacketWrongSize	= FOUR_CHAR('bSiz'),
					kJitErrUnknownBIXCP		= FOUR_CHAR('bMag');

#endif
	
	static t_jit_err DoGrayPict(msobBixCP* me, long iSize, tBixCPPicture* iPictBuf)
		{
		const int	kMaxArgs = 2;
		
		t_jit_err			err		= JIT_ERR_NONE;
		short				width	= NETORDER_INT16( iPictBuf->width ),
							height	= NETORDER_INT16( iPictBuf->height );
		void*				matrix;
		long				saveLock;
		t_jit_matrix_info	mInfo;
		char*				mData;					// Really a pointer to (unsigned) Bytes
		Atom				args[kMaxArgs];
		
		// Check data size
		if (iSize < sizeof(tBixCPPicture)	// Basic sanity check before accessing members
				|| iSize != CalcBufSize(width, height))
			return kJitErrPacketWrongSize;
		
		// Find matrix
		matrix = jit_object_findregistered(me->matrixSym);
		if (matrix == NIL || jit_object_method(matrix, _jit_sym_class_jit_matrix) == NIL)
			return JIT_ERR_MATRIX_UNKNOWN;
		
		// --------------------------------------------------------------------------------
		// Locking matrix now
		// State must be reset before leaving function
		//
		saveLock = (long) jit_object_method(matrix, _jit_sym_lock, 1L);
		
		// Set up Jitter matrix info
		jit_object_method(matrix, _jit_sym_getinfo, &mInfo);
		mInfo.type			= _jit_sym_char;
		mInfo.planecount	= 1;
		mInfo.dimcount		= 2;
		mInfo.dim[0]		= width;
		mInfo.dim[1]		= height;
		jit_object_method(matrix, _jit_sym_setinfo, &mInfo);	// Caclulates dimstride
		jit_object_method(matrix, _jit_sym_getinfo, &mInfo);	// Now get calculated dimstride
		
		// Get pointer to Jitter matrix data
		jit_object_method(matrix, _jit_sym_getdata, &mData);
		if (mData == NIL) {
			jit_error_sym(me, _jit_sym_err_calculate);
			goto alohamora;
			}
		
		if (mInfo.dimstride[1] == width) 
			BlockMoveData(iPictBuf->data, mData, width * height);
		else {
			// Jitter is padding rows, so we must copy one row at a time
			long	i		= height,
					stride	= mInfo.dimstride[1];
			BytePtr	source	= iPictBuf->data,
					dest	= (BytePtr) mData;
			
			while (i-- > 0) {
				BlockMoveData(source, dest, width);
				source	+= width;
				dest	+= stride;
				}
			}
		
	alohamora:
		jit_object_method(matrix, _jit_sym_lock, saveLock);				
		//
		// Matrix lock state now reset
		// --------------------------------------------------------------------------------
		
		if (err == JIT_ERR_NONE) {
			AtomSetSym(&args[0], me->matrixSym);
			outlet_anything(MainOutlet(me), gsymJitMatrix, 1, args);
			}
		
		return err;
		}


	static t_jit_err DoMonoPict(msobBixCP* me, long iSize, tBixCPPicture* iPictBuf)
		{
		const int	kMaxArgs = 2;
		
		t_jit_err			err		= JIT_ERR_NONE;
		short				width	= NETORDER_INT16( iPictBuf->width ),
							height	= NETORDER_INT16( iPictBuf->height );
		void*				matrix;
		long				saveLock;
		t_jit_matrix_info	mInfo;
		char*				mData;					// Really a pointer to (unsigned) Bytes
		Atom				args[kMaxArgs];
		
		// Check data size
		if (iSize < sizeof(tBixCPPicture)	// Basic sanity check before accessing members
				|| iSize != CalcBufSize(width, height))
			return kJitErrPacketWrongSize;
		
		// Find matrix
		matrix = jit_object_findregistered(me->matrixSym);
		if (matrix == NIL || jit_object_method(matrix, _jit_sym_class_jit_matrix) == NIL)
			return JIT_ERR_MATRIX_UNKNOWN;
		
		// --------------------------------------------------------------------------------
		// Locking matrix now
		// State must be reset before leaving function
		//
		saveLock = (long) jit_object_method(matrix, _jit_sym_lock, 1L);
		
		// Set up Jitter matrix info
		jit_object_method(matrix, _jit_sym_getinfo, &mInfo);
		mInfo.type			= _jit_sym_char;
		mInfo.planecount	= 1;
		mInfo.dimcount		= 2;
		mInfo.dim[0]		= width;
		mInfo.dim[1]		= height;
		jit_object_method(matrix, _jit_sym_setinfo, &mInfo);
		
		// Get pointer to Jitter matrix data
		jit_object_method(matrix, _jit_sym_getdata, &mData);
		if (mData == NIL) {
			jit_error_sym(me, _jit_sym_err_calculate);
			goto alohamora;
			}
		
		// Process data: copy byte by byte, subtracting 1 from (inByte == 0)
		if (mInfo.dimstride[1] == width) {
			long	i		= width * height;
			BytePtr	source	= iPictBuf->data,
					dest	= (BytePtr) mData;
			do { *dest++ = (*source++ == 0) - 1; } while (--i > 0);
			}
		else {
			// Have to deal with Jitter's row padding
			// ASSERT: mInfo.dimstride[1] > width
			long	i		= height,
					padding	= mInfo.dimstride[1] - width;
			BytePtr	source	= iPictBuf->data,
					dest	= (BytePtr) mData;
			
			while (--i >= 0) {
				long j = width;
				while (--j >= 0) *dest++ = (*source++ == 0) - 1;
				dest += padding;
				}
				
			}

	alohamora:
		jit_object_method(matrix, _jit_sym_lock, saveLock);				
		//
		// Matrix lock state now reset
		// --------------------------------------------------------------------------------
		
		if (err == JIT_ERR_NONE) {
			AtomSetSym(&args[0], me->matrixSym);
			outlet_anything(MainOutlet(me), gsymJitMatrix, 1, args);
			}
		
		return err;
		}


	static t_jit_err DoSleep(msobBixCP* me, tBixCPSleep* iSleepBuf)
		{
		t_jit_err	err		= JIT_ERR_NONE;
		long		howLong	= NETORDER_INT32( iSleepBuf->time );
		Atom		arg;
		
		AtomSetLong(&arg, CLAMP(howLong, 1, 1000));
		outlet_anything(MainOutlet(me), gsymSleep, 1, &arg);
		
		return err;
		}


	static t_jit_err DoSleepAck(msobBixCP* me)
		{
		t_jit_err	err = JIT_ERR_NONE;
		
		outlet_anything(MainOutlet(me), gsymSleepOK, 0, NIL);
		
		return err;
		}


	static t_jit_err DoSelFilter(msobBixCP* me, tBixCPSelFilter* iFiltBuf)
		{
		t_jit_err	err = JIT_ERR_NONE;
		Atom		args[3];
		
		AtomSetLong(&args[0], iFiltBuf->filtNum);
		AtomSetLong(&args[1], iFiltBuf->filtOn != 0);
		AtomSetLong(&args[2], iFiltBuf->filtVal);
		outlet_anything(MainOutlet(me), gsymSelFilter, 3, args);
		
		return err;
		}


	static t_jit_err DoDeviceCtrl(msobBixCP* me, tBixCPDeviceCtrl* iDCtrlBuf)
		{
		t_jit_err	err = JIT_ERR_NONE;
		Atom		arg;
		
		AtomSetLong(&arg, iDCtrlBuf->deviceOn != 0);
		outlet_anything(MainOutlet(me), gsymDeviceCtrl, 1, &arg);
		
		return err;
		}


	static t_jit_err DoDeviceAck(msobBixCP* me)
		{
		t_jit_err	err = JIT_ERR_NONE;
		
		outlet_anything(MainOutlet(me), gsymDeviceCtrlOK, 0, NIL);
		
		return err;
		}


static void
BixCPFullPacket(
	msobBixCP*		me,
	long			iSize,
	tBixCPGeneric*	iBuf)		// Evil implicit typecast. Max thinks this is a long int
	
	{
	
	t_jit_err	err	= JIT_ERR_NONE;

	if (iSize < sizeof(tBixCPGeneric)) {
		// This is not a standard Jitter error, so handle it ourselves.
		err = kJitErrPacketWrongSize;
		goto punt;
		}
	
	outlet_int(me->idOut, NETORDER_INT32( iBuf->msgID ));

	switch (NETORDER_INT32( iBuf->magic )) {
	case bixMagicGray:
		err = DoGrayPict(me, iSize, (tBixCPPicture*) iBuf);
		break;
	case SWAP32(bixMagicGray):
		// We appear to be receiving bytes in the wrong order.
		if (me->autoswapbytes) {
			// Rictify the situation
			// Get a typed pointer for convenience
			tBixCPPicture* buf = (tBixCPPicture*) iBuf;
			buf->magic	= SWAP32(buf->magic);
			buf->msgID	= SWAP32(buf->msgID);
			buf->width	= SWAP16(buf->width);
			buf->height	= SWAP16(buf->height);
			err = DoGrayPict(me, iSize, buf);
			break;
			}
		else err = kJitErrUnknownBIXCP;			// We're not automatically byteswapping.
		break;
	
	case bixMagicMono:
		err = DoMonoPict(me, iSize, (tBixCPPicture*) iBuf);
		break;
	case SWAP32(bixMagicMono):
		// We appear to be receiving bytes in the wrong order.
		if (me->autoswapbytes) {
			// Rictify the situation
			// Get a typed pointer for convenience
			tBixCPPicture* buf = (tBixCPPicture*) iBuf;
			buf->magic	= SWAP32(buf->magic);
			buf->msgID	= SWAP32(buf->msgID);
			buf->width	= SWAP16(buf->width);
			buf->height	= SWAP16(buf->height);
			err = DoMonoPict(me, iSize, buf);
			break;
			}
		else err = kJitErrUnknownBIXCP;			// We're not automatically byteswapping.
		break;
	
	case bixMagicSleep:
		if (iSize == sizeof(tBixCPSleep))
			 DoSleep(me, (tBixCPSleep*) iBuf);
		else err = kJitErrPacketWrongSize;
		break;
	case SWAP32(bixMagicSleep):
		// We appear to be receiving bytes in the wrong order.
		if (me->autoswapbytes) {
			// Check buffer size, and if that's good,  rectify byte order.
			if (iSize == sizeof(tBixCPSleep)) {
				tBixCPSleep* buf = (tBixCPSleep*) iBuf;	// Get a typed pointer for convenience
				buf->magic	= SWAP32(buf->magic);
				buf->msgID	= SWAP32(buf->msgID);
				buf->time	= SWAP32(buf->time);
				DoSleep(me, buf);
				}
			else err = kJitErrPacketWrongSize;
			} 
		else err = kJitErrUnknownBIXCP;
		break;
	
	case bixMagicSleepAck:
		if (iSize == sizeof(tBixCPSleepAck))
			 DoSleepAck(me);
		else err = kJitErrPacketWrongSize;
		break;
	case SWAP32(bixMagicSleepAck):
		// We appear to be receiving bytes in the wrong order.
		if (me->autoswapbytes) {
			if (iSize == sizeof(tBixCPSleepAck))
				 DoSleepAck(me);
			else err = kJitErrPacketWrongSize;
			}
		else err = kJitErrUnknownBIXCP;
		break;
	
	case bixMagicSelFilter:
		if (iSize == sizeof(tBixCPSelFilter))
			 DoSelFilter(me, (tBixCPSelFilter*) iBuf);
		else err = kJitErrPacketWrongSize;
		break;
	case SWAP32(bixMagicSelFilter):
		if (me->autoswapbytes) {
			if (iSize == sizeof(tBixCPSelFilter)) {
				 tBixCPSelFilter* buf = (tBixCPSelFilter*) iBuf;
				 buf->magic	= SWAP32(buf->magic);
				 buf->msgID	= SWAP32(buf->msgID);
				 DoSelFilter(me, buf);
				 }
			else err = kJitErrPacketWrongSize;
			}
		else err = kJitErrUnknownBIXCP;
		break;
	
	case bixMagicDeviceCtrl:
		if (iSize == sizeof(tBixCPDeviceCtrl))
			 DoDeviceCtrl(me, (tBixCPDeviceCtrl*) iBuf);
		else err = kJitErrPacketWrongSize;
		break;
	case SWAP32(bixMagicDeviceCtrl):
		if (me->autoswapbytes) {
			if (iSize == sizeof(tBixCPDeviceCtrl)) {
				 tBixCPDeviceCtrl* buf = (tBixCPDeviceCtrl*) iBuf;
				 buf->magic	= SWAP32(buf->magic);
				 buf->msgID	= SWAP32(buf->msgID);
				 DoDeviceCtrl(me, buf);
				 }
			else err = kJitErrPacketWrongSize;
			}
		else err = kJitErrUnknownBIXCP;
		break;
	
	case bixMagicDeviceAck:
		if (iSize == sizeof(tBixCPDeviceAck))
			 DoDeviceAck(me);
		else err = kJitErrPacketWrongSize;
		break;
	case SWAP32(bixMagicDeviceAck):
		if (me->autoswapbytes) {
			if (iSize == sizeof(tBixCPDeviceAck))
				 DoDeviceAck(me);
			else err = kJitErrPacketWrongSize;
			}
		else err = kJitErrUnknownBIXCP;
		break;
	
	default:
		err = kJitErrUnknownBIXCP;
		break;
		}
	
punt:
	switch (err) {
	case JIT_ERR_NONE:
		break;		// No complaints!
	
	case kJitErrUnknownBIXCP:
		error(	"%s: received unknown BIXCP message. (Magic ID was 0x%lX)",
				kMaxClassName, NETORDER_INT32( iBuf->magic ));
		break;
	
	case kJitErrPacketWrongSize:
		error("%s: Bad data packet received", kMaxClassName);
		break;
	
	default:
		jit_error_code(me, err);
		break;
		}
	
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
	
	post("%s state", kMaxClassName);
	
	post("  using matrix named %s, located at %p", me->matrixSym->s_name, me->matrix);
	
	}


#pragma mark -
#pragma mark ¥ Jitter Methods

