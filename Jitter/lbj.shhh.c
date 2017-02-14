/*
	File:		lbj.shhh.c

	Contains:	Max/Jitter external object generating evenly distributed random values.
				"White noise".

	Written by:	Peter Castine

	Copyright:	© 2003-2005 Peter Castine

	Change History (most recent first):

        <3+>   19Ð6Ð2006    pc      Correct problem in calculating stride component in non-char
                                    matrices
         <3>    8Ð5Ð2006    pc      Update for new LitterLib organization. In particular use inlined
                                    Taus88 for speed improvements.
         <2>   12Ð8Ð2005    pc      Optimize for char data (Taus88 gives us four bytes on each
                                    call).
         <1>      5Ð3Ð05    pc      Initial check in.
*/


/******************************************************************************************
	Previous History:

		11-Jul-2003:		First implementation.
		
 ******************************************************************************************/

#pragma mark ¥ Include Files

#include "LitterLib.h"
#include "TrialPeriodUtils.h"
#include "MiscUtils.h"
#include "Taus88.h"


#pragma mark ¥ Constants

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


#pragma mark ¥ Type Definitions



#pragma mark ¥ Object Structure

typedef struct {
	Object		coreObject;
	voidPtr		jitObject;
	} msobShhh;							// Mac Shell Object

typedef struct {
	Object			coreObject;
	
	int				nn;					// Number of bits to mask out
	unsigned long	nnMask,				// Values depends on nn
					nnOffset;
	} jcobShhh;							// Jitter Core Object


#pragma mark ¥ Global Variables

void*			gShhhJitClass	= NIL;
Messlist*		gMaxShhhClass	= NIL;
/*short			gStrResID		= 0;
unsigned long	gModDate		= 0;
*/

#pragma mark ¥ Function Prototypes

	// Max methods/functions
static void*ShhhNewMaxShell	(Symbol*, long, Atom*);
static void	ShhhFreeMaxShell(msobShhh*);

static void ShhhOutputMatrix(msobShhh*);
//static void ShhhBang	(msobShhh*);
//static void ShhhNN		(msobShhh*, long);
//static void ShhhSeed	(msobShhh*, long);
static void ShhhTattle	(msobShhh*);
static void	ShhhAssist	(msobShhh*, void* , long , long , char*);
static void	ShhhInfo	(msobShhh*);

	// Jitter methods/functions
static t_jit_err ShhhJitInit		(void);


#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark ¥ Inline Functions

/******************************************************************************************
 *
 *	Taus88CharVector(n, iTaus88Data, out)
 *	
 *	Standard Max External Object Entry Point Function
 *	
 ******************************************************************************************/

static inline void
Taus88CharVector(
	long			iCount,
	t_jit_op_info*	out)
	
	{
	Byte*	data	= (Byte*) out->p;
	long	stride  = out->stride,
			longCount;
	UInt32	s1, s2, s3;
	
	Taus88LoadGlobal(&s1, &s2, &s3);
	
	longCount	 = iCount >> 2;
	iCount		&= 0x00000003;
	
	if (stride == 1) {
		// Funky optimization #1
		long* longData = (long*) data;
		
		while (longCount-- >  0)
			*longData++ = Taus88Process(&s1, &s2, &s3);
		
		data = (Byte*) longData;
		}
	else {
		// Funky optimization #2
		while (longCount-- > 0) {
			UInt32	fourBytes = Taus88Process(&s1, &s2, &s3);
			int		i = 4;
			
			do	{
				*data = fourBytes;
				data += stride;
				fourBytes >>= 8;
				} while (--i > 0);
			}
		}
	
	// Any bytes not yet accounted for?
	if (iCount > 0) {
		UInt32 lastBytes = Taus88Process(&s1, &s2, &s3);
		
		do	{
			*data = lastBytes;
			data += stride;
			lastBytes >>= 8;
			} while (--iCount > 0);
		}

	Taus88StoreGlobal(s1, s2, s3);
	}


static inline void
 Taus88LongVector(
	long			n,
	t_jit_op_info*	out)

	{
	long*	data	= (long*) out->p;
	long	stride  = out->stride;
	UInt32			s1, s2, s3;
	
	Taus88LoadGlobal(&s1, &s2, &s3);
	
	if (stride == 1)
		 do { *data++ = Taus88Process(&s1, &s2, &s3); } while (--n > 0);
	else do { *data = Taus88Process(&s1, &s2, &s3); data += stride; } while (--n > 0);
	
	Taus88StoreGlobal(s1, s2, s3);
	}

static inline void
Taus88FloatVector(
	long			n,
	t_jit_op_info*	out)

	{
	float*	data	= (float*) out->p;
	long	stride  = out->stride;
	UInt32			s1, s2, s3;
	
	Taus88LoadGlobal(&s1, &s2, &s3);
	
	if (stride == 1)
		 do { *data++ = ULong2Unit_Zo(Taus88Process(&s1, &s2, &s3)); }
		 	while (--n > 0);
	else do { *data = ULong2Unit_Zo(Taus88Process(&s1, &s2, &s3)); data += stride; }
			while (--n > 0);
	
	Taus88StoreGlobal(s1, s2, s3);
	}

static inline void 
Taus88DoubleVector(
	long			n,
	t_jit_op_info*	out)
	
	{
	
	double*	data	= (double*) out->p;
	long	stride  = out->stride;
	UInt32			s1, s2, s3;
	
	Taus88LoadGlobal(&s1, &s2, &s3);
	
	if (stride == 1)
		 do { *data++ = ULong2Unit_Zo(Taus88Process(&s1, &s2, &s3)); }
		 	while (--n > 0);
	else do { *data = ULong2Unit_Zo(Taus88Process(&s1, &s2, &s3)); data += stride; }
			while (--n > 0);
	
	Taus88StoreGlobal(s1, s2, s3);
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
	const long kAttr = MAX_JIT_MOP_FLAGS_OWN_OUTPUTMATRIX | MAX_JIT_MOP_FLAGS_OWN_JIT_MATRIX;
	
	voidPtr	p,									// Have to guess about what these two do
			q;									// Not much is documented in the Jitter SDK
	
	LITTER_CHECKTIMEOUT(kMaxClassName);
	
	ShhhJitInit();
	
	// Standard Max setup() call
	setup(	&gMaxShhhClass,					// Pointer to our class definition
			(method) ShhhNewMaxShell,		// Instance creation function
			(method) ShhhFreeMaxShell,		// Custom deallocation function
			(short) sizeof(msobShhh),		// Class object size
			NIL,							// No menu function
			A_GIMME,						// Jitter objects always parse their own arguments
			0);		
	
	// Jitter Magic... 
	p = max_jit_classex_setup(calcoffset(msobShhh, jitObject));
	q = jit_class_findbyname(gensym((char*) kMaxClassName));    
    max_jit_classex_mop_wrap(p, q, kAttr); 		
    max_jit_classex_standard_wrap(p, q, 0); 	
	LITTER_TIMEBOMB max_addmethod_usurp_low((method) ShhhOutputMatrix, "outputmatrix");	
	
	// Back to adding messages...
	addmess	((method) ShhhTattle,	"dblclick",	A_CANT, 0);
	addmess	((method) ShhhTattle,	"tattle",	A_NOTHING);
	addmess	((method) ShhhAssist,	"assist",	A_CANT, 0);
	addmess	((method) ShhhInfo,		"info",		A_CANT, 0);
	
	// Initialize Litter Library
	LitterInit(kMaxClassName, 0);
	}


#pragma mark -
#pragma mark ¥ Internal functions


#pragma mark -
#pragma mark ¥ Max Shell Methods

/******************************************************************************************
 *
 *	ShhhNewMaxShell(iSym, iArgC, iArgV)
 *
 ******************************************************************************************/

static void*
ShhhNewMaxShell(
	SymbolPtr	sym,
	long		iArgC,
	Atom		iArgV[])
	
	{
	#pragma unused(sym)
	
	msobShhh*	me			= NIL;
	void*		jitObj		= NIL;
	Symbol*		classSym	= gensym((char*) kMaxClassName);
	
	// In the 1.0 SDK max_jit_obex_new() is prototyped to expect a Maxclass* instead of
	// a Messlist*. JKC said the next release of header files would be corrected to
	// use Messlist* and in the interim users can edit the file accordingly.
	// If we get a compiler "illegal implicit typecast" error, it means Josh forgot to
	// make the change; go to jit.max.h and do it again.
	me = (msobShhh*) max_jit_obex_new(gMaxShhhClass, classSym);
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
 *	ShhhFreeMaxShell(me)
 *
 ******************************************************************************************/

static void
ShhhFreeMaxShell(
	msobShhh* me)
	
	{
	max_jit_mop_free(me);
	jit_object_free(max_jit_obex_jitob_get(me));
	max_jit_obex_free(me);
	}


/******************************************************************************************
 *
 *	ShhhOutputMatrix(me)
 *
 ******************************************************************************************/

static void
ShhhOutputMatrix(
	msobShhh* me)
	
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
 *	ShhhTattle(me)
 *	ShhhInfo(me)
 *	ShhhAssist(me, iBox, iDir, iArgNum, oCStr)
 *
 *	Litter responses to standard Max messages
 *
 ******************************************************************************************/

void
ShhhTattle(
	msobShhh* me)
	
	{
	#pragma unused (me)
	
	post("%s state",
			kMaxClassName);
	// ??? What else is there to talk about ???
	
	}

void ShhhInfo(msobShhh* me)
	{ LitterInfo(kMaxClassName, &me->coreObject, (method) ShhhTattle); }

void ShhhAssist(msobShhh* me, void* box, long iDir, long iArgNum, char* oCStr)
	{
	#pragma unused(me, box)
	
	LitterAssist(iDir, iArgNum, strIndexInLeft, strIndexOutLeft, oCStr);
	}


#pragma mark -
#pragma mark ¥ Jitter Methods

/******************************************************************************************
 *
 *	ShhhJitMatrixCalc(me, inputs, outputs)
 *
 ******************************************************************************************/

static jcobShhh* ShhhJitNew()
	{
	jcobShhh* me = (jcobShhh*) jit_object_alloc(gShhhJitClass);
	
		// Still need to implement nn stuff !! ??
	me->nn			= 0;
	me->nnMask		= 0xffffffff;
	me->nnOffset	= 0;
	
	return me;
	}

static void ShhhJitFree(jcobShhh* me)
	{
	#pragma unused(me)
	 /* nothing to do */
	 }


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
				Taus88CharVector(n, &outOpInfo);
				}
			}
		
		else if (iMInfo->type == _jit_sym_long) {
			for (i = 0; i < iDimVec[1]; i += 1) {
				outOpInfo.p = iBOP + i * iMInfo->dimstride[1];
				Taus88LongVector(n, &outOpInfo);
				}
			}
		
		else if (iMInfo->type == _jit_sym_float32) {
			for (i = 0; i < iDimVec[1]; i += 1){
				outOpInfo.p = iBOP + i * iMInfo->dimstride[1];
				Taus88FloatVector(n, &outOpInfo);
				}
			}
		
		else if (iMInfo->type == _jit_sym_float64) {
			for (i = 0; i < iDimVec[1]; i += 1){
				outOpInfo.p = iBOP + i * iMInfo->dimstride[1];
				Taus88DoubleVector(n, &outOpInfo);
				}
			
			}
		
		break;
	
	default:
		// Larger values
		for	(i = 0; i < iDimVec[iDimCount-1]; i += 1) {
			char* op  = iBOP  + i * iMInfo->dimstride[iDimCount-1];
			RecurseDimensions(iDimCount - 1, iDimVec, iPlaneCount, iMInfo, op);
			}
		}
	
	}


/******************************************************************************************
 *
 *	ShhhJitMatrixCalc(me, inputs, outputs)
 *
 ******************************************************************************************/

static t_jit_err
ShhhJitMatrixCalc(
	jcobShhh*	me,
	void*		inputs,
	void*		outputs)
	
	{
	#pragma unused(inputs)
	
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
				
		RecurseDimensions(outMInfo.dimcount, dim, outMInfo.planecount, &outMInfo, outMData);
		}
	else err = JIT_ERR_INVALID_OUTPUT;
	
	jit_object_method(outMatrix, _jit_sym_lock, outSaveLock);
	
	return err;
	}
	

/******************************************************************************************
 *
 *	ShhhJitInit(me)
 *
 ******************************************************************************************/

t_jit_err
ShhhJitInit(void) 

	{
	void*	mop;
	
	gShhhJitClass = jit_class_new(	(char*) kMaxClassName,
									(method) ShhhJitNew,
									(method) ShhhJitFree,		// ?? Could be NIL ??
									sizeof(jcobShhh),
									A_CANT, 0L
									); 

	// add MOP, whatever the hell that is
	mop = jit_object_new(_jit_sym_jit_mop, 0, 1); 
	
	jit_class_addadornment(gShhhJitClass, mop);
	
	//add methods
	jit_class_addmethod(gShhhJitClass, (method) ShhhJitMatrixCalc, 	"matrix_calc", 	A_CANT, 0L);
	jit_class_register(gShhhJitClass);

	return JIT_ERR_NONE;
	}

