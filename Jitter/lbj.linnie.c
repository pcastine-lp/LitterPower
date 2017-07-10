/*
	File:		lbj.shhh.c

	Contains:	Max/Jitter external object generating evenly distributed random values.
				"White noise".

	Written by:	Peter Castine

	Copyright:	© 2003-2005 Peter Castine

	Change History (most recent first):

        <3+>   19–6–2006    pc      Correct problem in calculating stride component in non-char
                                    matrices
         <3>    8–5–2006    pc      Update for new LitterLib organization. In particular use inlined
                                    Taus88 for speed improvements.
         <2>   12–8–2005    pc      Optimize for char data (Taus88 gives us four bytes on each
                                    call).
         <1>      5–3–05    pc      Initial check in.
*/


/******************************************************************************************
	Previous History:

		11-Jul-2003:		First implementation.
		
 ******************************************************************************************/

#pragma mark • Include Files

#include "LitterLib.h"
#include "TrialPeriodUtils.h"
#include "MiscUtils.h"
#include "Taus88.h"


#pragma mark • Constants

const char	kMaxClassName[]		= "lbj.shhh",			// Class name for Max
			kJitClassName[]		= "lbj-shhh";			// Class name for Jitter

const int	kMaxNN			= 31;

	// Indices for STR# resource
enum {
	strIndexInBang		= lpStrIndexLastStandard + 1,
	strIndexInNN,
	
	strIndexOutWhite,
	strIndexOutDump,
	
	strIndexInLeft		= strIndexInBang,
	strIndexOutLeft		= strIndexOutWhite
	};


#pragma mark • Type Definitions



#pragma mark • Object Structure

typedef struct {
	Object		coreObject;
	voidPtr		jitObject;
	} msobLinnie;							// Mac Shell Object

typedef struct {
	Object			coreObject;
	
	tTaus88DataPtr	theData;
	
	int				nn;					// Number of bits to mask out
	unsigned long	nnMask,				// Values depends on nn
					nnOffset;
	} jcobLinnie;							// Jitter Core Object


#pragma mark • Global Variables

void*			gLinnieJitClass	= NIL;
Messlist*		gMaxLinnieClass	= NIL;
/*short			gStrResID		= 0;
unsigned long	gModDate		= 0;
*/

#pragma mark • Function Prototypes

	// Max methods/functions
static void*LinnieNewMaxShell	(Symbol*, long, Atom*);
static void	LinnieFreeMaxShell(msobLinnie*);

static void LinnieOutputMatrix(msobLinnie*);
//static void LinnieBang	(msobLinnie*);
//static void LinnieNN		(msobLinnie*, long);
//static void LinnieSeed	(msobLinnie*, long);
static void LinnieTattle	(msobLinnie*);
static void	LinnieAssist	(msobLinnie*, void* , long , long , char*);
static void	LinnieInfo	(msobLinnie*);

	// Jitter methods/functions
static t_jit_err LinnieJitInit		(void);


#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark • Inline Functions

/******************************************************************************************
 *
 *	Taus88CharVector(n, iTaus88Data, out)
 *	
 *	Standard Max External Object Entry Point Function
 *	
 ******************************************************************************************/

static inline void
Taus88CharVector(
	long			n,
	tTaus88DataPtr	iTaus88Data,
	t_jit_op_info*	out)
	
	{
	Byte*			data	= (Byte*) out->p;
	long			stride  = out->stride;
	UInt32			s1, s2, s3;
	
	if (iTaus88Data == NIL)
		iTaus88Data = Taus88GetGlobals();
	Taus88Load(iTaus88Data, &s1, &s2, &s3);
	
	if (stride == 1) {
		if ((n & 3) == 0) {
			long* longData = (long*) data;
			do { *longData++ = Taus88Process(&s1, &s2, &s3); n -= 4; } while (n > 0);
			}
		else do { *data++ = Taus88Process(&s1, &s2, &s3); } while (--n > 0);
		}
	else do { *data = Taus88Process(&s1, &s2, &s3); data += stride; } while (--n > 0);
	
	Taus88Store(iTaus88Data, s1, s2, s3);
	}


static inline void
 Taus88LongVector(
	long			n,
	tTaus88DataPtr	iTaus88Data,
	t_jit_op_info*	out)

	{
	long*	data	= (long*) out->p;
	long	stride  = out->stride;
	UInt32			s1, s2, s3;
	
	if (iTaus88Data == NIL)
		iTaus88Data = Taus88GetGlobals();
	Taus88Load(iTaus88Data, &s1, &s2, &s3);
	
	if (stride == 1)
		 do { *data++ = Taus88Process(&s1, &s2, &s3); } while (--n > 0);
	else do { *data = Taus88Process(&s1, &s2, &s3); data += stride; } while (--n > 0);
	
	Taus88Store(iTaus88Data, s1, s2, s3);
	}

static inline void
Taus88FloatVector(
	long			n,
	tTaus88DataPtr	iTaus88Data,
	t_jit_op_info*	out)

	{
	float*	data	= (float*) out->p;
	long	stride  = out->stride;
	UInt32			s1, s2, s3;
	
	if (iTaus88Data == NIL)
		iTaus88Data = Taus88GetGlobals();
	Taus88Load(iTaus88Data, &s1, &s2, &s3);
	
	if (stride == 1)
		 do { *data++ = ULong2Unit_Zo(Taus88Process(&s1, &s2, &s3)); }
		 	while (--n > 0);
	else do { *data = ULong2Unit_Zo(Taus88Process(&s1, &s2, &s3)); data += stride; }
			while (--n > 0);
	
	Taus88Store(iTaus88Data, s1, s2, s3);
	}

static inline void 
Taus88DoubleVector(
	long			n,
	tTaus88DataPtr	iTaus88Data,
	t_jit_op_info*	out)
	
	{
	
	double*	data	= (double*) out->p;
	long	stride  = out->stride;
	UInt32			s1, s2, s3;
	
	if (iTaus88Data == NIL)
		iTaus88Data = Taus88GetGlobals();
	Taus88Load(iTaus88Data, &s1, &s2, &s3);
	
	if (stride == 1)
		 do { *data++ = ULong2Unit_Zo(Taus88Process(&s1, &s2, &s3)); }
		 	while (--n > 0);
	else do { *data = ULong2Unit_Zo(Taus88Process(&s1, &s2, &s3)); data += stride; }
			while (--n > 0);
	
	Taus88Store(iTaus88Data, s1, s2, s3);
	}



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
	const long kAttr = MAX_JIT_MOP_FLAGS_OWN_OUTPUTMATRIX
						| MAX_JIT_MOP_FLAGS_OWN_JIT_MATRIX;
	
	voidPtr	p,									// Have to guess about what these two do
			q;									// Not much is documented in the Jitter SDK
	
	LITTER_CHECKTIMEOUT(kMaxClassName);
	
	LinnieJitInit();
	
	// Standard Max setup() call
	setup(	&gMaxLinnieClass,					// Pointer to our class definition
			(method) LinnieNewMaxShell,		// Instance creation function
			(method) LinnieFreeMaxShell,		// Custom deallocation function
			(short) sizeof(msobLinnie),	// Class object size
			NIL,							// No menu function
			A_GIMME,						// Jitter objects always parse their own arguments
			0);		
	
	// Jitter Magic... 
	p = max_jit_classex_setup(calcoffset(msobLinnie, jitObject));
	q = jit_class_findbyname(gensym((char*) kMaxClassName));    
    max_jit_classex_mop_wrap(p, q, kAttr); 		
    max_jit_classex_standard_wrap(p, q, 0); 	
	LITTER_TIMEBOMB max_addmethod_usurp_low((method) LinnieOutputMatrix, "outputmatrix");	
	
	// Back to adding messages...
	addmess	((method) LinnieTattle,	"dblclick",	A_CANT, 0);
	addmess	((method) LinnieTattle,	"tattle",	A_NOTHING);
	addmess	((method) LinnieAssist,	"assist",	A_CANT, 0);
	addmess	((method) LinnieInfo,		"info",		A_CANT, 0);
	
	// Initialize Litter Library
	LitterInit(kMaxClassName, 0);
	}


#pragma mark -
#pragma mark • Internal functions


#pragma mark -
#pragma mark • Max Shell Methods

/******************************************************************************************
 *
 *	LinnieNewMaxShell(iSym, iArgC, iArgV)
 *
 ******************************************************************************************/

static void*
LinnieNewMaxShell(
	SymbolPtr,
	long		iArgC,
	Atom		iArgV[])
	
	{
	msobLinnie*	me			= NIL;
	void*				jitObj		= NIL;
	tTaus88DataPtr		myTausStuff	= NIL;
	Symbol*				classSym	= gensym((char*) kMaxClassName);
	
	// In the 1.0 SDK max_jit_obex_new() is prototyped to expect a Maxclass* instead of
	// a Messlist*. JKC said the next release of header files would be corrected to
	// use Messlist* and in the interim users can edit the file accordingly.
	// If we get a compiler "illegal implicit typecast" error, it means Josh forgot to
	// make the change; go to jit.max.h and do it again.
	me = (msobLinnie*) max_jit_obex_new(gMaxLinnieClass, classSym);
		if (me == NIL) goto punt;
		
	jitObj = jit_object_new(classSym);
		if (jitObj == NIL) goto punt;
	
	max_jit_mop_setup_simple(me, jitObj, iArgC, iArgV);			
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
 *	LinnieFreeMaxShell(me)
 *
 ******************************************************************************************/

static void
LinnieFreeMaxShell(
	msobLinnie* me)
	
	{
	max_jit_mop_free(me);
	jit_object_free(max_jit_obex_jitob_get(me));
	max_jit_obex_free(me);
	}


/******************************************************************************************
 *
 *	LinnieOutputMatrix(me)
 *
 ******************************************************************************************/

static void
LinnieOutputMatrix(
	msobLinnie* me)
	
	{
	void*		mop = max_jit_obex_adornment_get(me, _jit_sym_jit_mop);
	t_jit_err	err = noErr;	
	
	// Sanity check: don't output if mop is NIL
	if (mop == NIL)
		return;
	
	if (max_jit_mop_getoutputmode(me) == 1) {
		err = (t_jit_err) jit_object_method(
								max_jit_obex_jitob_get(me), 
								_jit_sym_matrix_calc,
								jit_object_method(mop, _jit_sym_getinputlist),
								jit_object_method(mop, _jit_sym_getoutputlist));
		
		if (err == noErr)						
				max_jit_mop_outputmatrix(me);
		else	jit_error_code(me, err); 
		}
		
	}


/******************************************************************************************
 *
 *	LinnieTattle(me)
 *	LinnieInfo(me)
 *	LinnieAssist(me, iBox, iDir, iArgNum, oCStr)
 *
 *	Litter responses to standard Max messages
 *
 ******************************************************************************************/

void
LinnieTattle(
	msobLinnie* me)
	
	{
	#pragma unused(me)
	
	post("%s state",
			kMaxClassName);
	// ??? What else is there to talk about ???
	
	}

void LinnieInfo(msobLinnie* me)
	{ LitterInfo(kMaxClassName, &me->coreObject, (method) LinnieTattle); }

void LinnieAssist(msobLinnie* me, void* iBox, long iDir, long iArgNum, char* oCStr)
	{
	#pragma unused(me, iBox)
	
	LitterAssist(iDir, iArgNum, strIndexInLeft, strIndexOutLeft, oCStr);
	}


#pragma mark -
#pragma mark • Jitter Methods

/******************************************************************************************
 *
 *	LinnieJitMatrixCalc(me, inputs, outputs)
 *
 ******************************************************************************************/

static jcobLinnie* LinnieJitNew()
	{
	jcobLinnie* me = (jcobLinnie*) jit_object_alloc(gLinnieJitClass);
	
	me->theData		= NIL;
	me->nn			= 0;
	me->nnMask		= 0xffffffff;
	me->nnOffset	= 0;
	
	return me;
	}

static void LinnieJitFree(jcobLinnie*)
	{ /* nothing to do */ }


/******************************************************************************************
 *
 *	RecurseDimensions(iDimCount, iDimVec, iPlaneCount, iTaus88Data, iMInfo, iBOP)
 *
 ******************************************************************************************/

static void
RecurseDimensions(
	long				iDimCount,
	long				iDimVec[],
	long				iPlaneCount,
	tTaus88DataPtr		iTaus88Data,
	t_jit_matrix_info*	iMInfo,
	char*				iBOP)
	
	{
	long 			i, n;
	t_jit_op_info	outOpInfo;
		
	if (iDimCount < 1)		// For safety: this also catches invalid (negative) values
		return;
	
	switch(iDimCount) {
	case 1:
		iDimVec[1] = 1;
		// fall into next case...
	case 2:
		// Planes always have same parameters, so flatten matrix for speed
		n = iDimVec[0] * iPlaneCount;
		outOpInfo.stride = 1;
		iPlaneCount = 1;
		
		if (iMInfo->type == _jit_sym_char) {
			for (i = 0; i < iDimVec[1]; i += 1) {
				outOpInfo.p = iBOP + i * iMInfo->dimstride[1];
				Taus88CharVector(n, iTaus88Data, &outOpInfo);
				}
			}
		
		else if (iMInfo->type == _jit_sym_long) {
			for (i = 0; i < iDimVec[1]; i += 1) {
				outOpInfo.p = iBOP + i * iMInfo->dimstride[1];
				Taus88LongVector(n, iTaus88Data, &outOpInfo);
				}
			}
		
		else if (iMInfo->type == _jit_sym_float32) {
			for (i = 0; i < iDimVec[1]; i += 1){
				outOpInfo.p = iBOP + i * iMInfo->dimstride[1];
				Taus88FloatVector(n, iTaus88Data, &outOpInfo);
				}
			}
		
		else if (iMInfo->type == _jit_sym_float64) {
			for (i = 0; i < iDimVec[1]; i += 1){
				outOpInfo.p = iBOP + i * iMInfo->dimstride[1];
				Taus88DoubleVector(n, iTaus88Data, &outOpInfo);
				}
			
			}
		
		break;
	
	default:
		// Larger values
		for	(i = 0; i < iDimVec[iDimCount-1]; i += 1) {
			char* op  = iBOP  + i * iMInfo->dimstride[iDimCount-1];
			RecurseDimensions(iDimCount - 1, iDimVec, iPlaneCount, iTaus88Data, iMInfo, op);
			}
		}
	
	}


/******************************************************************************************
 *
 *	LinnieJitMatrixCalc(me, inputs, outputs)
 *
 ******************************************************************************************/

static t_jit_err
LinnieJitMatrixCalc(
	jcobLinnie*	me,
	void*,
	void*			outputs)
	
	{
	t_jit_err			err = JIT_ERR_NONE;
	long				outSaveLock;
	t_jit_matrix_info	outMInfo;
	char*				outMData;
	void*				outMatrix = jit_object_method(outputs, _jit_sym_getindex, 0);

	// Sanity check
	if ((me == NIL) || (outMatrix == NIL))
		return JIT_ERR_INVALID_PTR;
	
	outSaveLock = (long) jit_object_method(outMatrix, _jit_sym_lock, 1);
	
	jit_object_method(outMatrix, _jit_sym_getinfo, &outMInfo);
	jit_object_method(outMatrix, _jit_sym_getdata, &outMData);
	
	if (outMData != NIL) {
		// Copy dimensions to our private buffer
		long	i,
				dim[JIT_MATRIX_MAX_DIMCOUNT];
		
		for (i = 0; i < outMInfo.dimcount; i += 1)
			dim[i] = outMInfo.dim[i];
				
		RecurseDimensions(outMInfo.dimcount, dim, outMInfo.planecount, me->theData, &outMInfo, outMData);
		}
	else err = JIT_ERR_INVALID_OUTPUT;
	
	jit_object_method(outMatrix, _jit_sym_lock, outSaveLock);
	
	return err;
	}
	

/******************************************************************************************
 *
 *	LinnieJitInit(me)
 *
 ******************************************************************************************/

t_jit_err
LinnieJitInit(void) 

	{
	void*	mop;
	
	gLinnieJitClass = jit_class_new(	(char*) kMaxClassName,
									(method) LinnieJitNew,
									(method) LinnieJitFree,		// ?? Could be NIL ??
									sizeof(jcobLinnie),
									A_CANT, 0L
									); 

	// add MOP, whatever the hell that is
	mop = jit_object_new(_jit_sym_jit_mop, 0, 1); 
	
	jit_class_addadornment(gLinnieJitClass, mop);
	
	//add methods
	jit_class_addmethod(gLinnieJitClass, (method) LinnieJitMatrixCalc, 	"matrix_calc", 	A_CANT, 0L);
	jit_class_register(gLinnieJitClass);

	return JIT_ERR_NONE;
	}

