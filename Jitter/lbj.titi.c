/*
	File:		lbj.titi.c

	Contains:	Max/Jitter external object generating evenly distributed random values.
				"White noise".

	Written by:	Peter Castine

	Copyright:	© 2006 Peter Castine

	Change History (most recent first):

*/


/******************************************************************************************
	Previous History:

		11-Jul-2003:		First implementation.
		
 ******************************************************************************************/

#pragma mark • Include Files

	// Tell MaxUtils.h (#included by LitterLib.h) to #include Jitter headers
	// Jitter includes the most of the Max headers, which we have to explicitly
	// include otherwise.
#define __NEED_JITTER_HEADERS__	1

#include "LitterLib.h"
#include "MiscUtils.h"
#include "Taus88.h"


#pragma mark • Constants

const char	kMaxClassName[]		= "lbj.titi",			// Class name for Max
			kJitClassName[]		= "lbj-titi";			// Class name for Jitter

const int	kMaxNN			= 31;

	// Indices for STR# resource
enum {
	strIndexInBang		= lpStrIndexLastStandard + 1,
	
	strIndexOutWhite,
	strIndexOutDump,
	
	strIndexInLeft		= strIndexInBang,
	strIndexOutLeft		= strIndexOutWhite
	};


#pragma mark • Type Definitions

typedef struct {
	Atom	min,
			max;
	} tVecData;
typedef tVecData* tVecDataPtr;


#pragma mark • Object Structure

typedef struct {
	Object		coreObject;
	voidPtr		jitObject;
	} msobTiti;							// Mac Shell Object

typedef struct {
	Object	coreObject;
	
	long	minCount,
			maxCount;
	Atom	min[JIT_MATRIX_MAX_PLANECOUNT],
			max[JIT_MATRIX_MAX_PLANECOUNT];
	} jcobTiti;							// Jitter Core Object


#pragma mark • Global Variables

void*			gTitiJitClass	= NIL;
Messlist*		gTitiMaxClass	= NIL;

#pragma mark • Function Prototypes

	// Max methods/functions
static void*TitiNewMaxShell	(Symbol*, long, Atom*);
static void	TitiFreeMaxShell(msobTiti*);

static void TitiOutputMatrix(msobTiti*);
static void TitiTattle	(msobTiti*);
static void	TitiAssist	(msobTiti*, void* , long , long , char*);
static void	TitiInfo	(msobTiti*);

	// Jitter methods/functions
static t_jit_err TitiJitInit		(void);


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
	long			iCount,
	tVecData*		iVecData,
	t_jit_op_info*	out)
	
	{
	Byte*	data	= (Byte*) out->p;
	long	stride  = out->stride;
	long	min, max;
	
	if (iVecData->min.a_type == A_SYM)
		min = 0;
	else {
		min = AtomGetLong(&iVecData->min);
		CLIP(min, 0, 255);
		}
	
	if (iVecData->max.a_type == A_SYM)
		max = 255;
	else {
		max = AtomGetLong(&iVecData->max);
		CLIP(max, 0, 255);
		}
	
	if (min != max) {
		double scale;
		UInt32	s1, s2, s3;
		
		if (min > max) {
			// Put values in order
			long realMax = min;
			min = max;
			max = realMax;
			}
		
		scale = max - min + 1;		// The plus one is not obvious but necessary
		
		Taus88LoadGlobal(&s1, &s2, &s3);
		
		if (stride > 1 || scale < 256.0) {
			double fMin = min;
			
			while (iCount-- > 0) {
				*data =  fMin + scale * ULong2Unit_Zo( Taus88Process(&s1, &s2, &s3) );
				data += stride;
				}
			}
		
		else {
			// Funky optimization
			long*	longData = (long*) data;
			long	longCount = iCount >> 2;
			
			while (longCount-- >  0)
				*longData++ = Taus88Process(&s1, &s2, &s3);
			
			// Any bytes not yet accounted for?
			iCount &= 0x00000003;
			if (iCount > 0) {
				UInt32 lastBytes = Taus88Process(&s1, &s2, &s3);
				
				data = (Byte*) longData;
				do	{
					*data++ = lastBytes & 0x000000ff;
					lastBytes >>= 8;
					} while (--iCount > 0);
				}
			}
		
		Taus88StoreGlobal(s1, s2, s3);
		}
		
	else  while (iCount-- > 0) {
		*data = min;
		data += stride;
		}
	
	}


static inline void
 Taus88LongVector(
	long			iCount,
	tVecData*		iVecData,
	t_jit_op_info*	out)

	{
	long*	data	= (long*) out->p;
	long	stride  = out->stride;
	long	min, max;	
	
	if (iVecData->min.a_type == A_SYM)
		min = kLongMin;
	else {
		min = AtomGetLong(&iVecData->min);
		CLIP(min, kLongMin, kLongMax);
		}
	
	if (iVecData->max.a_type == A_SYM)
		max = kLongMax;
	else {
		max = AtomGetLong(&iVecData->max);
		CLIP(max, kLongMin, kLongMax);
		}
	
	if (min != max) {
		UInt32	s1, s2, s3;
		
		if (min > max) {
			// Put values in order
			long realMax = min;
			min = max;
			max = realMax;
			}
		
		Taus88LoadGlobal(&s1, &s2, &s3);
		
		if (min != kLongMin && max != kLongMax) {
			double	scale = ((double) (max - min)) + 1.0,
					fMin	= (double) min;
			
			while (iCount-- > 0) {
				*data =  fMin + scale * ULong2Unit_Zo( Taus88Process(&s1, &s2, &s3) );
				data += stride;
				}
			}
		else while (iCount-- > 0) {
			*data =  Taus88Process(&s1, &s2, &s3);
			data += stride;
			}
				
		
		Taus88StoreGlobal(s1, s2, s3);
		}
		
	else while (iCount-- > 0) {
		*data = min;
		data += stride;
		}
	
	}

static inline void
Taus88FloatVector(
	long			iCount,
	tVecData*		iVecData,
	t_jit_op_info*	out)

	{
	float*	data	= (float*) out->p;
	long	stride  = out->stride;
	double	min, max;	
	
	if (iVecData->min.a_type == A_SYM)
		min = 0.0;
	else {
		min = AtomGetFloat(&iVecData->min);
		CLIP(min, 0.0, 1.0);
		}
	
	if (iVecData->max.a_type == A_SYM)
		max = kLongMax;
	else {
		max = AtomGetFloat(&iVecData->max);
		CLIP(max, 0.0, 1.0);
		}
	
	if (min != max) {
		double scale;
		UInt32	s1, s2, s3;
		
		if (min > max) {
			// Put values in order
			double realMax = min;
			min = max;
			max = realMax;
			}
		
		scale = max - min;
				
		Taus88LoadGlobal(&s1, &s2, &s3);
		
		if (scale < 1.0) {
			if (min > 0.0) while (iCount-- > 0) {
				*data =  min + scale * ULong2Unit_ZO( Taus88Process(&s1, &s2, &s3) );
				data += stride;
				}
			else while (iCount-- > 0) {
				*data =  scale * ULong2Unit_ZO( Taus88Process(&s1, &s2, &s3) );
				data += stride;
				}
			}
		else while (iCount-- > 0) {
			*data =  ULong2Unit_ZO( Taus88Process(&s1, &s2, &s3) );
			data += stride;
			}
		
		Taus88StoreGlobal(s1, s2, s3);
		}
		
	else while (iCount-- > 0) {
		*data = min;
		data += stride;
		}
	
	}

static inline void 
Taus88DoubleVector(
	long			iCount,
	tVecData*		iVecData,
	t_jit_op_info*	out)
	
	{
	double*	data	= (double*) out->p;
	long	stride  = out->stride;
	double	min, max;	
	
	if (iVecData->min.a_type == A_SYM)
		min = 0.0;
	else {
		min = AtomGetFloat(&iVecData->min);
		CLIP(min, 0.0, 1.0);
		}
	
	if (min != max) {
		double scale;
		UInt32	s1, s2, s3;
		
		if (min > max) {
			// Put values in order
			double realMax = min;
			min = max;
			max = realMax;
			}
		
		scale = max - min;
				
		Taus88LoadGlobal(&s1, &s2, &s3);
		
		if (scale < 1.0) {
			if (min > 0.0) while (iCount-- > 0) {
				*data =  min + scale * ULong2Unit_ZO( Taus88Process(&s1, &s2, &s3) );
				data += stride;
				}
			else while (iCount-- > 0) {
				*data =  scale * ULong2Unit_ZO( Taus88Process(&s1, &s2, &s3) );
				data += stride;
				}
			}
		else while (iCount-- > 0) {
			*data =  ULong2Unit_ZO( Taus88Process(&s1, &s2, &s3) );
			data += stride;
			}
		
		Taus88StoreGlobal(s1, s2, s3);
		}
		
	else while (iCount-- > 0) {
		*data = min;
		data += stride;
		}
	
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
	
		// Have to guess about what the following do.
		// Not much is documented in the Jitter SDK
	voidPtr	p,
			q;
	
	LITTER_CHECKTIMEOUT(kClassName);
	
	TitiJitInit();
	
	// Standard Max setup() call
	setup(	&gTitiMaxClass,					// Pointer to our class definition
			(method) TitiNewMaxShell,		// Instance creation function
			(method) TitiFreeMaxShell,		// Custom deallocation function
			(short) sizeof(msobTiti),	// Class object size
			NIL,							// No menu function
			A_GIMME,						// Jitter objects always parse their own arguments
			0);		
	
	// Jitter Magic... 
	p = max_jit_classex_setup(calcoffset(msobTiti, jitObject));
	q = jit_class_findbyname(gensym((char*) kMaxClassName));    
    max_jit_classex_mop_wrap(p, q, kAttr); 		
    max_jit_classex_standard_wrap(p, q, 0); 	
	LITTER_TIMEBOMB max_addmethod_usurp_low((method) TitiOutputMatrix, "outputmatrix");	
	
	// Back to adding messages...
//	addbang	((method) TitiBang);
//	addmess ((method) TitiSeed,		"seed",		A_DEFLONG, 0);
	addmess	((method) TitiTattle,	"dblclick",	A_CANT, 0);
	addmess	((method) TitiTattle,	"tattle",	A_NOTHING);
	addmess	((method) TitiAssist,	"assist",	A_CANT, 0);
	addmess	((method) TitiInfo,		"info",		A_CANT, 0);
	
	// Initialize Litter Library
	LitterInit(kMaxClassName, 0);
	}


#pragma mark -
#pragma mark • Internal functions


#pragma mark -
#pragma mark • Max Shell Methods

/******************************************************************************************
 *
 *	TitiNewMaxShell(iSym, iArgC, iArgV)
 *
 ******************************************************************************************/

static void*
TitiNewMaxShell(
	SymbolPtr,
	long		iArgC,
	Atom		iArgV[])
	
	{
	msobTiti*	me			= NIL;
	void*				jitObj		= NIL;
	tTaus88DataPtr		myTausStuff	= NIL;
	Symbol*				classSym	= gensym((char*) kMaxClassName);
	
	// In the 1.0 SDK max_jit_obex_new() is prototyped to expect a Maxclass* instead of
	// a Messlist*. JKC said the next release of header files would be corrected to
	// use Messlist* and in the interim users can edit the file accordingly.
	// If we get a compiler "illegal implicit typecast" error, it means Josh forgot to
	// make the change; go to jit.max.h and do it again.
	me = (msobTiti*) max_jit_obex_new(gTitiMaxClass, classSym);
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
 *	TitiFreeMaxShell(me)
 *
 ******************************************************************************************/

static void
TitiFreeMaxShell(
	msobTiti* me)
	
	{
	max_jit_mop_free(me);
	jit_object_free(max_jit_obex_jitob_get(me));
	max_jit_obex_free(me);
	}


/******************************************************************************************
 *
 *	TitiOutputMatrix(me)
 *
 ******************************************************************************************/

static void
TitiOutputMatrix(
	msobTiti* me)
	
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
 *	TitiTattle(me)
 *	TitiInfo(me)
 *	TitiAssist(me, iBox, iDir, iArgNum, oCStr)
 *
 *	Litter responses to standard Max messages
 *
 ******************************************************************************************/

void
TitiTattle(
	msobTiti* /*me*/)
	
	{
	
	post("%s state",
			kMaxClassName);
	// ??? What else is there to talk about ???
	
	}

void TitiInfo(msobTiti* me)
	{ LitterInfo(kMaxClassName, &me->coreObject, (method) TitiTattle); }

void TitiAssist(msobTiti*, void*, long iDir, long iArgNum, char* oCStr)
	{ LitterAssist(iDir, iArgNum, strIndexInLeft, strIndexOutLeft, oCStr); }


#pragma mark -
#pragma mark • Jitter Methods

/******************************************************************************************
 *
 *	TitiJitMatrixCalc(me, inputs, outputs)
 *
 ******************************************************************************************/

static jcobTiti* TitiJitNew()
	{
	jcobTiti* me = (jcobTiti*) jit_object_alloc(gTitiJitClass);
	
	
	return me;
	}

static void TitiJitFree(jcobTiti*)
	{ /* nothing to do */ }


#pragma mark -
#pragma mark • Matrix Calculations

/******************************************************************************************
 *
 *	RecurseDimensions(iDimCount, iDimVec, iPlaneCount, iTaus88Data, iMInfo, iBOP)
 *
 ******************************************************************************************/

static void
RecurseDimensions(
	jcobTiti*			me,
	long				iDimCount,
	long				iDimVec[],
	long				iPlaneCount,
	t_jit_matrix_info*	iMInfo,
	char*				iBOP)
	
	{
	long 			i, j, n,
					minCount,
					maxCount;
	tVecData		vecData;
	t_jit_op_info	outOpInfo;
		
	if (iDimCount < 1)		// For safety: this also catches invalid (negative) values
		return;
	
	switch(iDimCount) {
	case 1:
		iDimVec[1] = 1;
		// fall into next case...
	case 2:
		// Planes always have same parameters, so flatten matrix for speed
		n				= iDimVec[0];
		minCount		= me->minCount,
		maxCount		= me->maxCount;
		outOpInfo.stride = iMInfo->dim[0] > 1
							? iMInfo->planecount
							: 0;
		
		// Test if it's appropriate to flatten to a single plane
		if (iPlaneCount > 1) {
			Boolean flatten = true;
			Atom	min	= me->min[0],
					max	= me->max[0];
			
			for (i = iPlaneCount - 1; i > 0; i -= 1) {
				long	minIndex = i % minCount,
						maxIndex = i % maxCount;
				if (me->min[minIndex].a_type != min.a_type
						|| me->max[maxIndex].a_type != max.a_type
							// The following conditions do what we want even if a_type is A_SYM
						|| me->min[minIndex].a_w.w_long != min.a_w.w_long
						|| me->max[maxIndex].a_w.w_long != max.a_w.w_long) {
					flatten = false;
					break;
					}
				}
			
			if (flatten) {
				n					*= iPlaneCount;
				iPlaneCount			 = 1;
				outOpInfo.stride	 = 1;
				}
			}
		
		
		if (iMInfo->type == _jit_sym_char) {
			for (i = 0; i < iDimVec[1]; i += 1) {
				long	minIndex = 0,
						maxIndex = 0;
				
				for (j = 0; j < iPlaneCount; j += 1) {
					if (minIndex >= minCount)
						minIndex = 0;
					vecData.min = me->min[minIndex++];
					if (maxIndex >= maxCount)
						maxIndex = 0;
					vecData.max = me->max[maxIndex++];
					outOpInfo.p = iBOP + i * iMInfo->dimstride[1] + j;
					Taus88CharVector(n, &vecData, &outOpInfo);
					}
				}
			}
		
		else if (iMInfo->type == _jit_sym_long) {
			for (i = 0; i < iDimVec[1]; i += 1) {
				long	minIndex = 0,
						maxIndex = 0;
				
				for (j = 0; j < iPlaneCount; j += 1) {
					if (minIndex >= minCount)
						minIndex = 0;
					vecData.min = me->min[minIndex++];
					if (maxIndex >= maxCount)
						maxIndex = 0;
					vecData.max = me->max[maxIndex++];
					outOpInfo.p = iBOP + i * iMInfo->dimstride[1] + j * sizeof(long);
					Taus88LongVector(n, &vecData, &outOpInfo);
					}
				}
			}
		
		else if (iMInfo->type == _jit_sym_float32) {
			for (i = 0; i < iDimVec[1]; i += 1) {
				long	minIndex = 0,
						maxIndex = 0;
				
				for (j = 0; j < iPlaneCount; j += 1) {
					if (minIndex >= minCount)
						minIndex = 0;
					vecData.min = me->min[minIndex++];
					if (maxIndex >= maxCount)
						maxIndex = 0;
					vecData.max = me->max[maxIndex++];
					outOpInfo.p = iBOP + i * iMInfo->dimstride[1] + j * sizeof(float);
					Taus88FloatVector(n, &vecData, &outOpInfo);
					}
				}
			}
		
		else if (iMInfo->type == _jit_sym_float64) {
			for (i = 0; i < iDimVec[1]; i += 1) {
				long	minIndex = 0,
						maxIndex = 0;
				
				for (j = 0; j < iPlaneCount; j += 1) {
					if (minIndex >= minCount)
						minIndex = 0;
					vecData.min = me->min[minIndex++];
					if (maxIndex >= maxCount)
						maxIndex = 0;
					vecData.max = me->max[maxIndex++];
					outOpInfo.p = iBOP + i * iMInfo->dimstride[1] + j * sizeof(double);
					Taus88DoubleVector(n, &vecData, &outOpInfo);
					}
				}
			}
		
		break;
	
	default:
		// Larger values
		for	(i = 0; i < iDimVec[iDimCount-1]; i += 1) {
			char* op  = iBOP  + i * iMInfo->dimstride[iDimCount-1];
			RecurseDimensions(me, iDimCount - 1, iDimVec, iPlaneCount, iMInfo, op);
			}
		}
	
	}


/******************************************************************************************
 *
 *	TitiJitMatrixCalc(me, inputs, outputs)
 *
 ******************************************************************************************/

static t_jit_err
TitiJitMatrixCalc(
	jcobTiti*	me,
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
				
		RecurseDimensions(	me,
							outMInfo.dimcount,
							dim,
							outMInfo.planecount,
							&outMInfo,
							outMData);
		}
	else err = JIT_ERR_INVALID_OUTPUT;
	
	jit_object_method(outMatrix, _jit_sym_lock, outSaveLock);
	
	return err;
	}
	
#pragma mark -
#pragma mark • Attribute functions

/******************************************************************************************
 *
 *	NormJitInit(me)
 *
 ******************************************************************************************/


#pragma mark -
#pragma mark • Jitter Initialization

/******************************************************************************************
 *
 *	TitiJitInit(me)
 *
 ******************************************************************************************/

	// !! convenience type, should move to MaxUtils.h or such
	typedef t_jit_object* tJitObjPtr;
	
t_jit_err
TitiJitInit(void) 

	{
	const long	kAttrRWFlags	= JIT_ATTR_GET_DEFER_LOW | JIT_ATTR_SET_USURP_LOW,
				kAttrROnlyFlags	= JIT_ATTR_GET_DEFER_LOW | JIT_ATTR_SET_OPAQUE_USER;
	const int	kJitterInlets	= 0,	// No inlets for Jitter Matrices
				kJitterOutlets	= 1;	// One outlet for Jitter Matrices (Gaussian noise)
	
	tJitObjPtr	mop,
				attr;
		
	gTitiJitClass = jit_class_new(	(char*) kMaxClassName,
									(method) TitiJitNew,
									(method) TitiJitFree,		// ?? Could be NIL ??
									sizeof(jcobTiti),
									A_CANT, 0L
									); 

	// Add matrix operator
	mop = jit_object_new(_jit_sym_jit_mop, kJitterInlets, kJitterOutlets); 
	jit_class_addadornment(gTitiJitClass, mop);
	
	//add methods
	jit_class_addmethod(gTitiJitClass,
						(method) TitiJitMatrixCalc,
						"matrix_calc",
						A_CANT, 0L);

	// Add attributes
		// Min
	attr = jit_object_new(	_jit_sym_jit_attr_offset_array,
							"min",
							_jit_sym_atom,
							JIT_MATRIX_MAX_PLANECOUNT,
							kAttrRWFlags,
							(method) NIL, (method) NIL,
							calcoffset(jcobTiti, minCount), calcoffset(jcobTiti, min)
							);
	jit_class_addattr(gTitiJitClass, attr);
		// Max
	attr = jit_object_new(	_jit_sym_jit_attr_offset_array,
							"max",
							_jit_sym_atom,
							JIT_MATRIX_MAX_PLANECOUNT,
							kAttrRWFlags,
							(method) NIL, (method) NIL,
							calcoffset(jcobTiti, maxCount), calcoffset(jcobTiti, max)
							);
	jit_class_addattr(gTitiJitClass, attr);
	
	

	jit_class_register(gTitiJitClass);

	return JIT_ERR_NONE;
	}

