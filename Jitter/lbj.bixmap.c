/*
	File:		lbj.bixmap.c

	Contains:	Max/Jitter external object mapping input values analogous to jit.charmap.
				Takes a 256 list of floating point values in the unit range for mapping 
				input to output, interpolating for input values in between.

	Written by:	Peter Castine

	Copyright:	© 2008 Peter Castine

	Change History (most recent first):

*/


/******************************************************************************************
	Previous history:
		30-Jul-2003:		First implementation.
 ******************************************************************************************/

#pragma mark ¥ Include Files

#include "LitterLib.h"
#include "TrialPeriodUtils.h"
#include "MiscUtils.h"


#pragma mark ¥ Constants

const char	kMaxClassName[]	= "lbj.bixmap",		// Class name for Max, also used
													// for resources.
			kJitClassName[]	= "lbj-bixmap";		// Class name for Jitter

	// Indices for STR# resource
enum strIndices {
	strIndexInMatrix		= lpStrIndexLastStandard + 1,
	strIndexInGroups,
	strIndexInFilters,
	
	strIndexOutMatrix,
	strIndexOutDump,
	
	strIndexInLeft		= strIndexInMatrix,
	strIndexOutLeft		= strIndexOutMatrix
	};

	// Number of different filters we're managing
	// 1 Global Filter, plus 31 filters for independent, non-overlapping regions
enum {
	kFiltersMax	= 32,
	kFilterMask	= 0x1f,
	kFilterSize = 256
	};
	
#pragma mark ¥ Data Types

typedef char* CharPtr;					// Should be typedef'd somewhere else ??
	
#pragma mark ¥ Object Structure

typedef struct {
	Object		coreObject;
	void*		xObject;								// Extended object (?)
//	long		inletNum;								// For tracking proxy
	}  msobBixMap;										// msob == Max Shell Object

typedef struct {
	Object	coreObject;
		// ASSERT: kFiltersMax * kFilterSize < ~32,000
	Byte	cooked[kFiltersMax][kFilterSize];
	float*	raw;										// kFiltersMax*kFilterSize array
														// allocated on heap
	} jcobBixMap;										// jcob = Jitter Core Object



#pragma mark ¥ Global Variables

void*		gJitClassBixMap	= NIL;
Messlist*	gMaxClassBixMap	= NIL;
Symbol*		gSymJitBixMap	= NIL;


#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark ¥ Inline Functions

static inline void* MainOutlet(msobBixMap* me)
	{ return me->coreObject.o_outlet; }

	

#pragma mark -
#pragma mark ¥ Jitter Methods

/******************************************************************************************
 *
 *	BixMapJitNew()
 *	BixMapJitFree()
 *
 *	Wrapper around default Jitter new method
 *
 ******************************************************************************************/

static jcobBixMap*
BixMapJitNew(void)
	{
	int			i, j;
	float*		r;
	
	jcobBixMap* me = (jcobBixMap*) jit_object_alloc(gJitClassBixMap);
	
	if (me == NIL)
		goto punt;
	
	me->raw = (float*) sysmem_newptr(kFiltersMax * kFilterSize * sizeof(float));
	if (me->raw == NULL) { freeobject((void*) me); me = NULL; goto punt; }
	
	// Initialize components to default setting
	for (i = 0, r = me->raw; i < kFiltersMax; i += 1) for (j = 0; j < kFilterSize; j += 1) {
		*r++				= 1.0;
		me->cooked[i][j]	= j;
		}
	
punt:
	return me;
	}

static void BixMapJitFree(jcobBixMap*	me)
	{ if (me->raw != NULL) sysmem_freeptr(me->raw); }


/******************************************************************************************
 *
 *	BixMapVector(n, vecdata, in1, in2, out)
 *	BixMapVector4Plane(n, vecdata, in1, in2, out)
 *	BixMapCalcNDim()
 *	BixMapMatrix()
 *
 *	Matrix processing method
 *
 ******************************************************************************************/

#if 0
static void
BixMapVector(
	long			n,
	void*			vecdata,
	t_jit_op_info*	in1,
	t_jit_op_info*	in2,
	t_jit_op_info*	out) 
	
	{
	BytePtr	iPtr1		= ((BytePtr) in1->p),
			iPtr2		= ((BytePtr) in2->p),
			oPtr		= ((BytePtr) out->p);
	long	stride1		= in1->stride,
			stride2		= in2->stride,
			strideOut	= out->stride;
		
	while (n-- > 0) {
		*oPtr	 = iPtr2[(*iPtr1) * stride2];
		oPtr	+= strideOut;
		iPtr1	+= stride1; 
		}
		
	}
#endif


static void
BixMapVectorChar1Plane(
	long			n,
	Byte			iMaps[kFiltersMax][kFilterSize],
	t_jit_op_info*	iInfo1,
	t_jit_op_info*	iInfo2,
	t_jit_op_info*	oInfo) 

	{
	BytePtr	ip1	= (Byte*) iInfo1->p,
			ip2	= (Byte*) iInfo2->p,
			op  = (Byte*) oInfo->p;
		
	// ASSERT stride == 1 for all threee t_jit_op_info structs
	while (n-- > 0) {
		*op++ = iMaps[*ip2++ & kFilterMask][*ip1++];
		}	
	}


static void
BixMapCalcNDim(
	void*				iMapData,
	long				iDimCount,
	long*				iDimensions,
	long				iPlaneCount,
	t_jit_matrix_info*	iMatrixInfo1,
	Byte*				iData1, 
	t_jit_matrix_info*	iMatrixInfo2,
	Byte*				iData2,
	t_jit_matrix_info*	oMatrixInfo,
	Byte*				oData)

	{
	long			i,
					// j,
					n;
	t_jit_op_info	opInfoIn1,
					opInfoIn2,
					opInfoOut;
		
	if (iDimCount < 1) return;		// Sanity check
	
	switch (iDimCount) {
	case 1:
		iDimensions[1] = 1;
		// Fall into next caseÉ
	case 2:
		n = iDimensions[0];
		
		// If planecount is the same, flatten planes - ie treat as single plane data for speed
		opInfoIn1.stride = iMatrixInfo1->dim[0] > 1 ? iMatrixInfo1->planecount : 0;
		opInfoIn2.stride = iMatrixInfo2->dim[0] > 1 ? iMatrixInfo2->planecount : 0;
		opInfoOut.stride = oMatrixInfo->dim[0] > 1 ? oMatrixInfo->planecount : 0;
		
		// ASSERT: (iMatrixInfo1->type == _jit_sym_char && iPlaneCount == 1) 
#if 1
		for (i = 0; i < iDimensions[1]; i += 1) {
			opInfoIn1.p = iData1 + i * iMatrixInfo1->dimstride[1];
			opInfoIn2.p = iData2 + i * iMatrixInfo2->dimstride[1];
			opInfoOut.p = oData  + i * oMatrixInfo->dimstride[1];
			BixMapVectorChar1Plane(n, iMapData, &opInfoIn1, &opInfoIn2, &opInfoOut);
			}
#else
		// For variable plane count
		for (i = 0; i < iDimensions[1]; i += 1) for (j = 0; j < iPlaneCount; j += 1) {
			opInfoIn1.p = iData1 + i * iMatrixInfo1->dimstride[1] + j % iMatrixInfo1->planecount;
			opInfoIn2.p = iData2 + j % iMatrixInfo2->planecount;
			opInfoOut.p = oData  + i * oMatrixInfo->dimstride[1] + j % oMatrixInfo->planecount;
			BixMapVectorChar1Plane(n, iMapData, &opInfoIn1, &opInfoIn2, &opInfoOut);
			}
#endif
		break;
	
	default:
		for	(i = 0; i < iDimensions[iDimCount-1]; i += 1) {
			BytePtr	subMatrixIn		= iData1 + i * iMatrixInfo1->dimstride[iDimCount-1],
					subMatrixOut	= oData  + i * oMatrixInfo->dimstride[iDimCount-1];
			BixMapCalcNDim(	iMapData, iDimCount-1, iDimensions, iPlaneCount,
							iMatrixInfo1, subMatrixIn,
							iMatrixInfo2, iData2,
							oMatrixInfo, subMatrixOut);
			}
		}
	}

static t_jit_err
BixMapMatrix(
	jcobBixMap*	me,
	void*		inputs,
	void*		outputs)
	
	{
	const long	kFlags1	= 0,
				kFlags2	= JIT_PARALLEL_NDIM_FLAGS_FULL_MATRIX,
				kFlags3 = 0;
	
	t_jit_err	err			= JIT_ERR_NONE;
	voidPtr		matrixIn1 	= jit_object_method(inputs, _jit_sym_getindex, 0),
				matrixIn2 	= jit_object_method(inputs, _jit_sym_getindex, 1),
				matrixOut 	= jit_object_method(outputs, _jit_sym_getindex, 0);
	
	long				saveLockIn1,
						saveLockIn2,
						saveLockOut;
	t_jit_matrix_info	matrixInfoIn1,
						matrixInfoIn2,
						matrixInfoOut;
	CharPtr				DataIn1,
						DataIn2,
						DataOut;
	long				i,
						dimcount,
						planecount,
						dim[JIT_MATRIX_MAX_DIMCOUNT];
	
	// Sanity check
	if (matrixIn1 == NULL || matrixIn2 == NULL || matrixOut == NULL) {
		err = JIT_ERR_INVALID_PTR; goto punt;
		}
	
	// Lock down all our pointers
	saveLockIn1 = (long) jit_object_method(matrixIn1, _jit_sym_lock, 1);
	saveLockIn2 = (long) jit_object_method(matrixIn2, _jit_sym_lock, 1);
	saveLockOut = (long) jit_object_method(matrixOut, _jit_sym_lock, 1);
	
	// Copy matrix info into our placeholders
	jit_object_method(matrixIn1, _jit_sym_getinfo, &matrixInfoIn1);
	jit_object_method(matrixIn2, _jit_sym_getinfo, &matrixInfoIn2);
	jit_object_method(matrixOut, _jit_sym_getinfo, &matrixInfoOut);
	
	// Get raw data
	jit_object_method(matrixIn1, _jit_sym_getdata, &DataIn1);
	jit_object_method(matrixIn2, _jit_sym_getdata, &DataIn2);
	jit_object_method(matrixOut, _jit_sym_getdata, &DataOut);
	
	// More sanity checking
	if		(DataIn1 == NULL || DataIn2 == NULL)	err = JIT_ERR_INVALID_INPUT;
	else if (DataOut == NULL)						err = JIT_ERR_INVALID_OUTPUT;
	else if (matrixInfoIn1.type != _jit_sym_char
				|| matrixInfoIn2.type != _jit_sym_char
				|| matrixInfoOut.type != _jit_sym_char)
													err = JIT_ERR_MISMATCH_TYPE;
	else if (matrixInfoIn1.dimcount != matrixInfoIn2.dimcount
				|| matrixInfoIn1.dimcount != matrixInfoOut.dimcount)
													err = JIT_ERR_MISMATCH_DIM;
	else if (matrixInfoIn1.planecount != matrixInfoOut.planecount
				|| matrixInfoIn2.planecount != 1)	err = JIT_ERR_MISMATCH_PLANE;
	if (err != JIT_ERR_NONE)						goto alohamora;

	// Get dimensions & planecount
	dimcount   = matrixInfoIn1.dimcount;
	planecount = matrixInfoIn1.planecount;			
	
	// Copy dimensions
	for (i=0; i < dimcount; i += 1) {
		// if dimsize is 1, treat as infinite domain across that dimension.
		// otherwise truncate if less than the output dimsize
		dim[i] = matrixInfoOut.dim[i];
		if ( (matrixInfoIn1.dim[i] < dim[i]) && (matrixInfoIn1.dim[i] > 1) ) {
			dim[i] = matrixInfoIn1.dim[i];
			}
		}
			
	jit_parallel_ndim_simplecalc3(	(method) BixMapCalcNDim, me->cooked,
									dimcount, dim, planecount,
									&matrixInfoIn1, DataIn1,
									&matrixInfoIn2, DataIn2, 
									&matrixInfoOut, DataOut,
									kFlags1, kFlags2, kFlags3);

alohamora:
	jit_object_method(matrixOut, _jit_sym_lock, saveLockOut);
	jit_object_method(matrixIn2, _jit_sym_lock, saveLockIn2);
	jit_object_method(matrixIn1, _jit_sym_lock, saveLockIn1);
	// End of normal processing
	// ------------------------------------------------------------------------------------

	// ------------------------------------------------------------------------------------
	// Poor man's exception handling
punt:
	return err;
	}


/******************************************************************************************
 *
 *	BixMapJitSetFilter()
 *
 *	The format for a setfilter message is:
 *
 *		setfilter <list>
 *
 *	The first value in the list must be a number from 0 to 15, with 0 indicating the global 
 *	filter and values from 1 to 15 indicating the region filters. This is followed by at
 *	least two more values, indicating the mapping for char input 0 and 255. If there are
 *	more than two mapping values, they will indicate the mapping at equidistant points 
 *	between 0 and 255. The list values can either be ints (clipped to the range 0..255) or
 *	floats, which will map values in the unit range to the 0..255 range, clipping if
 *	necessary. The list should not contain symbols, but any item containing a symbol will
 *	be treated as a zero and not treated as an error.
 *
 ******************************************************************************************/
 
	static double EvalFilterAtom(Atom* iAtom)
		{
		const double	kInverse = 1.0 / 256.0;
		
		double val = 0.0;
		
		switch (iAtom->a_type) {
		case A_LONG:
			val = ((double) ClipLong(iAtom->a_w.w_long, 0, 255)) * kInverse;
			break;
		
		case A_FLOAT:
			val = ClipDouble(iAtom->a_w.w_float, 0, 1.0);
			break;
		
		default:
			// This shouldn't happen, and we've already zeroed the return value in case it does
			break;
			}
		
		return val;
		}
	
		// NOTE: CookFilter does no validity checking of its input parameters.
		//		 Check validity before calling or you will crash.
	static void CookFilter(jcobBixMap* me, int iFilterNo)
		{
		int		i;
		float*	rb = me->raw + (iFilterNo * kFilterSize);
		
		for (i = 0; i < kFilterSize; i += 1) {
			double cumFilter = me->raw[i];		// Global filter values are at offset 0
												// into raw array
			
			if (iFilterNo > 0) cumFilter *= rb[i];
			
			me->cooked[iFilterNo][i] = 255.0 * cumFilter + 0.5;
			}
		}

static t_jit_err
BixMapJitSetFilter(
	jcobBixMap*		me,
	Symbol*			iSym,						// Unused
	short			iArgCount,
	Atom			iArgVec[])
	
	{
	#pragma unused(iSym)
	
	t_jit_err	myErr = JIT_ERR_NONE;
	long		filterNo,
				i;
	float*		rb;
	double		left,
				right,
				stride,
				stride1,
				slope,
				relPos;
	
	// Check format of list
	if (iArgCount < 3) {
		myErr = JIT_ERR_GENERIC;
		goto punt;
		}
		
	filterNo = AtomGetLong(iArgVec++); iArgCount -= 1;
	if (filterNo < 0 || kFiltersMax <= filterNo) {
		myErr = JIT_ERR_GENERIC;
		goto punt;
		}
	if (iArgCount > 256)
		iArgCount = 256;					// Silently truncate list
		
	// Map the input to the raw value, stretching the input values as necessary
	left	= EvalFilterAtom(iArgVec++);
	right	= EvalFilterAtom(iArgVec++);
	stride	= ((double) kFilterSize) / ((double) (iArgCount - 1));
	stride1	= 1.0 / stride;
	slope	= (right - left) * stride1;
	relPos	= 0.0;
	rb = me->raw + (filterNo * kFilterSize);
	for (i = 0; i < kFilterSize; i += 1) {
		rb[i] = left;
		
		relPos	+= 1.0;
		if (relPos > stride) {
			relPos	-= stride;
			left	 = right;
			right	 = EvalFilterAtom(iArgVec++);
			slope	 = (right - left) * stride1;
			left	+= slope * relPos;
			}
		else left += slope;
		}
	
	// Update cooked values
	if (filterNo == 0)
		for (i = 0; i < kFiltersMax; i += 1) CookFilter(me, i);
	else									 CookFilter(me, filterNo);

punt:
	return myErr;
	}


/******************************************************************************************
 *
 *	BixMapJitInit()
 *
 *	Initialize Jitter class and associated symbol
 *
 ******************************************************************************************/

static t_jit_err
BixMapJitInit(void) 
	
	{
	const long	kJitterInlets	= 2,
				kJitterOutlets	= 1,
				kAttributeFlags	= JIT_ATTR_GET_OPAQUE
									| JIT_ATTR_GET_OPAQUE_USER
									| JIT_ATTR_SET_USURP_LOW;
	
	
	tObjectPtr	mop,
				m2,
				attr;;
	
	// Symbol we use a lot
	gSymJitBixMap = gensym((char*) kJitClassName);
	
	// The Class
	gJitClassBixMap = jit_class_new((char*) kJitClassName,		// Ignore 'const' qualifier
									(method) BixMapJitNew,
									(method) BixMapJitFree,
									sizeof(jcobBixMap),
									A_NOTHING);
	
	// Add Matrix Operator
	mop = jit_object_new(_jit_sym_jit_mop, kJitterInlets, kJitterOutlets);
	jit_class_addadornment(gJitClassBixMap, mop);
	
	// Override defaults for 2nd input matrix
	//	- Don't link dimension or type to left matrix
	//	- Must be char
	jit_mop_single_type(mop, _jit_sym_char);
	jit_mop_single_planecount(mop, 1);
	
	m2 = jit_object_method(mop, _jit_sym_getinput, 2);
//	jit_attr_setlong(m2, _jit_sym_dimlink, 0);
//	jit_attr_setlong(m2, _jit_sym_typelink, 0);
	
	// Add main method and register
	jit_class_addmethod(gJitClassBixMap, (method) BixMapMatrix, "matrix_calc", A_CANT, 0);
	jit_class_register(gJitClassBixMap);
	
	// Add filters attribute
	attr = jit_object_new(_jit_sym_jit_attribute, "filter", 
							_jit_sym_float32, kAttributeFlags,
							NIL, (method) BixMapJitSetFilter);
	jit_class_addattr(gJitClassBixMap, attr);

	return JIT_ERR_NONE;
	}

#pragma mark -
#pragma mark ¥ Max Wrapper Methods

/******************************************************************************************
 *
 *	BixMapNewMaxShell(iSym, iArgC, iArgV)
 *	BixMapFreeMaxShell(me)
 *
 ******************************************************************************************/

static msobBixMap*
BixMapNew(
	SymbolPtr	sym,
	long		iArgC,
	Atom		iArgV[])
	
	{
	#pragma unused(sym)
	
	msobBixMap*	me = (msobBixMap*) max_jit_obex_new(gMaxClassBixMap, gSymJitBixMap);
	if (me != NIL) {
		// proxy_new(&me->coreObject, 2, &me->inletNum);
		
		void* jitObj =jit_object_new(gSymJitBixMap);
		if (jitObj != NIL) {
			max_jit_mop_setup_simple(me, jitObj, iArgC, iArgV);			
			max_jit_attr_args(me , iArgC, iArgV);
			}
		else {
			freeobject((void*) me);
			me = NIL;
			}
		}
	
	if (me == NIL) error("%s: could not allocate object", kMaxClassName);
	
	return me;
	}

static void
BixMapFree(
	msobBixMap* me)
	
	{
	// All of this is Jitter/mop magic
	max_jit_mop_free(me);
	jit_object_free( max_jit_obex_jitob_get(me) );
	max_jit_obex_free(me);
	}
	
/******************************************************************************************
 *
 *	BixMapAssist(me, iBox, iDir, iArgNum, oCStr)
 *	BixMapInfo(me)
 *	BixMapTattle(me)
 *
 *	Fairly generic Assist/Info/Tattle Methods
 *
 ******************************************************************************************/

static void BixMapAssist(msobBixMap* me, void* box, long iDir, long iArgNum, char* oCStr)
	{
	#pragma unused(me, box)
	
	LitterAssist(iDir, iArgNum, strIndexInLeft, strIndexOutLeft, oCStr);
	}

static void
BixMapTattle(
	msobBixMap* me)
	
	{
	
	post("%s state", kMaxClassName);
	
//	post("  using matrix named %s, located at %p", me->matrixSym->s_name, me->matrix);
	
	}

static void BixMapInfo(msobBixMap* me)
	{ LitterInfo(kMaxClassName, &me->coreObject, (method) BixMapTattle); }




#pragma mark -
#pragma mark ¥ Entry Point

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
	
	BixMapJitInit();
		// Side effect: kJitClassName = gensym(gSymJitBixMap);
	
	// Standard Max setup() call
	setup(	&gMaxClassBixMap,				// Pointer to our class definition
			(method) BixMapNew,				// Instance creation function
			(method) BixMapFree,			// Custom deallocation function
			sizeof(msobBixMap),				// Class object size
			NIL,							// No menu function
			A_GIMME,						// Jitter parses its own arguments
			0);		
	
	// Jitter Magic... 
	mjClassEx	= max_jit_classex_setup( calcoffset(msobBixMap, xObject) );
	jClass		= jit_class_findbyname(gSymJitBixMap);
	max_jit_classex_mop_wrap(mjClassEx, jClass, 0); 		
	max_jit_classex_standard_wrap(mjClassEx, jClass, 0); 	
	
	/*
	 *	Add messages and attributes
	 */
			
	
		// Tack on informational messages
	addmess	((method) BixMapTattle,	"dblclick",	A_CANT, 0);
	addmess	((method) BixMapTattle,	"tattle",	A_NOTHING);
	addmess	((method) BixMapAssist,	"assist",	A_CANT, 0);
	addmess	((method) BixMapInfo,	"info",		A_CANT, 0);	
	
	// Initialize Litter Library
	LitterInit(kMaxClassName, 0);
	
	}


