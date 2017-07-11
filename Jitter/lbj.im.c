/*
	File:		lbj.im.c

	Contains:	Max/Jitter external object performing morphological mutations with Jitter
				matrices.

	Written by:	Peter Castine

	Copyright:	© 2006 Peter Castine

	Change History (most recent first):

        <3+>    9–6–2006    pc      Add support for irregular mutations
         <3>      8–6–06    pc      Mostly cosmetic changes to make attribute handling clearer.
         <2>    9–5–2006    pc      Attributize algorithm and mode.
         <1>    8–5–2006    pc      first checked in.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "TrialPeriodUtils.h"
#include "imLib.h"										// #includes LitterLib.h
														// and almost everything else...

#include "MiscUtils.h"


#pragma mark • Constants

const char	kMaxClassName[]		= "lbj.jim",			// Class name for Max
			kJitClassName[]		= "lbj-jim";			// Class name for Jitter


	// Add'l Indices for STR# resource
enum {
	strIndexOutDump		= strIndexOutMutant + 1
	};


#pragma mark • Type Definitions



#pragma mark • Object Structure

typedef struct {
	Object		coreObject;
	voidPtr		obex;								// The magic extended object thing.
	} msobJim;										// Mac Shell Object

	// This has been copied from imLib.h for modification
	// Ideally, I think it would be good to get this back into imLib.h with
	// conditional compilation as necessary. Or maybe not ??
typedef struct {
	tMutator		coreMutatorObj;
	
	tHistory		history;
	tMutationParams	params;
	
		// Components needed to maintain state in irregular mutations
	Symbol*			stateMatrixSym;
	void*			stateMatrix;
	} jcobJim;


#pragma mark • Global Variables

void*			gJimJitClass	= NIL;
Messlist*		gJimMaxClass	= NIL;

#pragma mark • Function Prototypes

	// Max methods/functions
static void*	JimNewMaxShell	(Symbol*, long, Atom*);
static void		JimFreeMaxShell(msobJim*);

static void JimOutputMatrix(msobJim*);
//static void JimBang	(msobJim*);

static void JimTattle	(msobJim*);
static void	JimAssist	(msobJim*, void* , long , long , char*);
static void	JimInfo		(msobJim*);

	// Jitter methods/functions
static t_jit_err	JimJitInit		(void);
static jcobJim*		JimJitNew		(void);
static void			JimJitFree		(jcobJim*);
static t_jit_err	JimJitMatrixCalc(jcobJim*, void*, void*);


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
main(void)
	
	{
//	const long kAttr = MAX_JIT_MOP_FLAGS_OWN_OUTPUTMATRIX | MAX_JIT_MOP_FLAGS_OWN_JIT_MATRIX;
	const long kAttr = 0;
	
	voidPtr	p,									// Have to guess about what these two do
			q;									// Not much is documented in the Jitter SDK
	
	LITTER_CHECKTIMEOUT(kClassName);
	
	JimJitInit();
		
	// Standard Max setup() call
	setup(	&gJimMaxClass,					// Pointer to our class definition
			(method) JimNewMaxShell,		// Instance creation function
			(method) JimFreeMaxShell,		// Custom deallocation function
			(short) sizeof(msobJim),		// Class object size
			NIL,							// No menu function
			A_GIMME,						// Jitter objects always parse their own arguments
			0);		
	
	// Jitter Magic... 
	p = max_jit_classex_setup(calcoffset(msobJim, obex));
	q = jit_class_findbyname(gensym((char*) kJitClassName));    
    max_jit_classex_mop_wrap(p, q, kAttr); 		
    max_jit_classex_standard_wrap(p, q, 0); 	
	LITTER_TIMEBOMB max_addmethod_usurp_low((method) JimOutputMatrix, "outputmatrix");	
	
	// Back to adding messages...
	addmess	((method) JimTattle,	"dblclick",	A_CANT, 0);
	addmess	((method) JimTattle,	"tattle",	A_NOTHING);
	addmess	((method) JimAssist,	"assist",	A_CANT, 0);
	addmess	((method) JimInfo,		"info",		A_CANT, 0);
	
	// Initialize Litter Library
	LitterInit(kMaxClassName, 0);
	}


#pragma mark -
#pragma mark • Internal functions


#pragma mark -
#pragma mark • Max Shell Methods

/******************************************************************************************
 *
 *	JimNewMaxShell(iSym, iArgC, iArgV)
 *
 ******************************************************************************************/

static void*
JimNewMaxShell(
	SymbolPtr,
	long		iArgC,
	Atom		iArgV[])
	
	{
	msobJim*		me			= NIL;
	void*			jitObj		= NIL;
	Symbol*			jitClassSym	= gensym((char*) kJitClassName);
	
	me = (msobJim*) max_jit_obex_new(gJimMaxClass, jitClassSym);
		if (me == NIL) goto punt;
		
	jitObj = jit_object_new(jitClassSym);
		if (jitObj == NIL) goto punt;
	
	max_jit_mop_setup_simple(me, jitObj, iArgC, iArgV);			
	max_jit_attr_args(me, iArgC, iArgV);

	return me;
	// End of normal processing
	// ------------------------------------------------------------------------
	
	// ------------------------------------------------------------------------
	// Poor man's exception handling
punt:
	error("%s: could not allocate object", kMaxClassName);
	if (me != NIL)
		freeobject(&me->coreObject);
		
	return NIL;
	}

/******************************************************************************************
 *
 *	JimFreeMaxShell(me)
 *
 ******************************************************************************************/

static void
JimFreeMaxShell(
	msobJim* me)
	
	{
	max_jit_mop_free(me);
	jit_object_free(max_jit_obex_jitob_get(me));
	max_jit_obex_free(me);
	}


/******************************************************************************************
 *
 *	JimOutputMatrix(me)
 *
 ******************************************************************************************/

static void
JimOutputMatrix(
	msobJim* me)
	
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
 *	JimTattle(me)
 *	JimInfo(me)
 *	JimAssist(me, iBox, iDir, iArgNum, oCStr)
 *
 *	Litter responses to standard Max messages
 *
 ******************************************************************************************/

void
JimTattle(
	msobJim* /*me*/)
	
	{
	
	post("%s state",
			kMaxClassName);
	// ??? What else is there to talk about ???
	
	}

void JimInfo(msobJim* me)
	{ LitterInfo(kMaxClassName, &me->coreObject, (method) JimTattle); }

void JimAssist(msobJim*, void*, long iDir, long iArgNum, char* oCStr)
	{ LitterAssist(iDir, iArgNum, strIndexFirstIn, strIndexFirstOut, oCStr); }


#pragma mark -
#pragma mark • Jitter Attributes

/******************************************************************************************
 *
 *	JimJitInit()
 *
 ******************************************************************************************/

// !! convenience type, should move to MaxUtils.h or such
typedef t_jit_object* tJitObjPtr;

// Attribute access wrappers
static t_jit_err
JimSetAlgorithm(
	jcobJim*	me,
	void*		iAttr,
	long		iArgCount,
	Atom*		iArgVec)
	
	{
	#pragma unused(iAttr)
	
	short alg;	
	
	if (iArgCount <= 0 || iArgVec == NIL || iArgVec[0].a_type != A_SYM)
		return JIT_ERR_INVALID_INPUT;
	
	alg = MapStrToMutationType((const char*) iArgVec[0].a_w.w_sym->s_name);
	SetMutatorType((tMutator*) me, alg);
	
	return JIT_ERR_NONE;
	}

static t_jit_err 
JimGetAlgorithm(
	jcobJim*	me,
	void*		iAttr,
	long*		iArgCount,
	Atom**		iArgVec)
	
	{
	#pragma unused(iAttr)
	
	short algIndex;
	
	if (*iArgCount <= 0 || *iArgVec == NIL) {
		*iArgVec = (Atom*) getbytes(sizeof(Atom));
		if (*iArgVec == NIL)
			return JIT_ERR_OUT_OF_MEM;
		}
		
	*iArgCount	= 1;
	algIndex	= me->params.irregular + 2 * me->params.magOnly + 4 * me->params.signOnly;
	AtomSetSym(	*iArgVec, gensym((char*) kShortNames[algIndex]) );
	
	return JIT_ERR_NONE;
	}

static t_jit_err
JimSetMode(
	jcobJim*	me,
	void*		iAttr,
	long		iArgCount,
	Atom*		iArgVec)
	
	{
	#pragma unused(iAttr)
	
	Symbol* iMode;	
	
	if (iArgCount <= 0 || iArgVec == NIL || iArgVec[0].a_type != A_SYM)
		return JIT_ERR_INVALID_INPUT;
	
	iMode = iArgVec[0].a_w.w_sym;
	
	if (iMode == gensym("abs"))
		DoAbsInt((tMutator*) me);
	else if (iMode == gensym("rel"))
		DoRelInt((tMutator*) me, 0.0);
	else return JIT_ERR_INVALID_INPUT;
	
	return JIT_ERR_NONE;
	}

static t_jit_err
JimGetMode(
	jcobJim*	me,
	void*		iAttr,
	long*		iArgCount,
	Atom**		iArgVec)
	
	{
	#pragma unused(iAttr)
	
	if (*iArgCount <= 0 || *iArgVec == NIL) {
		*iArgVec = (Atom*) getbytes(sizeof(Atom));
		if (*iArgVec == NIL)
			return JIT_ERR_OUT_OF_MEM;
		}
		
	*iArgCount	= 1;
	AtomSetSym(	*iArgVec, gensym(me->params.relInterval ? "rel" : "abs") );
	
	return JIT_ERR_NONE;
	}
	

#pragma mark -
#pragma mark • Jitter Methods

/******************************************************************************************
 *
 *	JimJitInit()
 *
 ******************************************************************************************/

t_jit_err
JimJitInit(void) 

	{
	const long	kAttrFlags		= JIT_ATTR_GET_DEFER_LOW | JIT_ATTR_SET_USURP_LOW;
	const int	kJitterInlets	= 2,	// Two inlets for Jitter Matrices (Src/Tar)
				kJitterOutlets	= 1;	// One outlet for Jitter Matrices (Mutant)
										// The Max shell object will create add'l outlets 
										// but that's none of our business.
	
	tJitObjPtr	mop,
				attr;
	
	// Define Jitter class
	gJimJitClass = jit_class_new(	(char*) kJitClassName,
									(method) JimJitNew,
									(method) JimJitFree,		// ?? Could be NIL ??
									sizeof(jcobJim),
									A_CANT, 0L
									); 

	// Add Matrix Operator object
	mop = jit_object_new(_jit_sym_jit_mop, kJitterInlets, kJitterOutlets);
//	jit_mop_single_planecount(mop, 4);	
	jit_class_addadornment(gJimJitClass, mop);
	
	// Add methods
	jit_class_addmethod(gJimJitClass, (method) JimJitMatrixCalc, "matrix_calc", A_CANT, 0L);
		// Methods for compatibility with other mutation objects, attributized below
	jit_class_addmethod(gJimJitClass, (method) DoUSIM,		"usim", A_NOTHING);
	jit_class_addmethod(gJimJitClass, (method) DoISIM,		"isim", A_NOTHING);
	jit_class_addmethod(gJimJitClass, (method) DoUUIM,		"uuim", A_NOTHING);
	jit_class_addmethod(gJimJitClass, (method) DoIUIM,		"iuim", A_NOTHING);
	jit_class_addmethod(gJimJitClass, (method) DoLCM,		"lcm", A_NOTHING);
	jit_class_addmethod(gJimJitClass, (method) DoWCM,		"wcm", A_NOTHING);

	jit_class_addmethod(gJimJitClass, (method) DoAbsInt,	"abs", A_NOTHING);
	jit_class_addmethod(gJimJitClass, (method) DoRelInt,	"rel", A_DEFFLOAT, 0);
	
	// Add attributes
		// Algorithm
	attr = jit_object_new(	_jit_sym_jit_attr_offset,
							"alg",
							_jit_sym_symbol,
							kAttrFlags,
							(method) JimGetAlgorithm,
							(method) JimSetAlgorithm,
							0
							);
	jit_class_addattr(gJimJitClass, attr);
		// Mode
	attr = jit_object_new(	_jit_sym_jit_attr_offset,
							"mode",
							_jit_sym_symbol,
							kAttrFlags,
							(method) JimGetMode,
							(method) JimSetMode,
							0
							);
	jit_class_addattr(gJimJitClass, attr);
		// Mutation index: omega
	attr = jit_object_new(	_jit_sym_jit_attr_offset,
							"omega",
							_jit_sym_float64,
							kAttrFlags,
							NIL, NIL,
							calcoffset(jcobJim, params.omega)
							);
	jit_attr_addfilterset_clip(attr, 0.0, 1.0, true, true);					// Clip to [0 .. 1]
	jit_class_addattr(gJimJitClass, attr);
	
		// Clumping index: pi
	attr = jit_object_new(	_jit_sym_jit_attr_offset,
							"pi",
							_jit_sym_float64,
							kAttrFlags,
							NIL, NIL,
							calcoffset(jcobJim, params.pi)
							);
	jit_attr_addfilterset_clip(attr, 0.0, 0.99999976158, true, true);		// Clip to [0 .. 1)
	jit_class_addattr(gJimJitClass, attr);
	
		// Delta epsilon
	attr = jit_object_new(	_jit_sym_jit_attr_offset,
							"delta",
							_jit_sym_float64,
							kAttrFlags,
							NIL, NIL,
							calcoffset(jcobJim, params.delta)
							);
	jit_attr_addfilterset_clip(attr, -1.0, 1.0, true, true);				// Clip to [0 .. 1)
	jit_class_addattr(gJimJitClass, attr);
	
	// Not sure how to handle "function" (usim, isim, etc.) as an attribute (partic. w/assoc. flags)
	// Maybe the pattr SDK will help ??
	
	// params.clumpLen is, I think(??) constant for most other mutators 
	
	// All done. Register class and go
	jit_class_register(gJimJitClass);
	return JIT_ERR_NONE;
	}

/******************************************************************************************
 *
 *	JimJitNew()
 *	JimJitFree(me)
 *
 ******************************************************************************************/

static jcobJim* JimJitNew()
	{
	jcobJim* 			me = (jcobJim*) jit_object_alloc(gJimJitClass);
	t_jit_matrix_info	info;
	
	if (me != NIL) {
		me->params.omega	= 0.0;
		me->params.pi		= 0.0;
		me->params.delta	= 0.0;
		me->params.clumpLen	= 0;
		
		DoUSIM((tMutator*) me);						// Initialize function and flags
		
		me->stateMatrixSym	= jit_symbol_unique();
		
		jit_matrix_info_default(&info);
		info.type		= _jit_sym_char;
		me->stateMatrix	= jit_object_method(jit_object_new(_jit_sym_jit_matrix, &info),
											_jit_sym_register,
											me->stateMatrixSym);
		if (me->stateMatrix == NIL)
			error("%s: could not create state matrix", kMaxClassName);
		jit_object_attach(me->stateMatrixSym, me);
		}
	
	return me;
	}

static void JimJitFree(jcobJim* me)
	{
	jit_object_detach(me->stateMatrixSym, me);
	jit_object_free(me->stateMatrix);
	}


/******************************************************************************************
 *
 *	RecurseDimensionsUnifAbs(iDimCount, iDimVec, iPlaneCount, iTaus88Data, iMInfo, iBOP)
 *
 *	Everything we need for uniform absolute mutations
 *
 ******************************************************************************************/

	static void MutateCharVectorUnifAbs(long n, tMutationParams* iMutParams,
								 BytePtr iSrcVec, BytePtr iTarVec, BytePtr oMutVec)
		{
		double		omega		= iMutParams->omega;
		tMutateFunc unifFunc	= iMutParams->function.uniformFunc;
		
		while (n-- > 0) {
			*oMutVec++ = unifFunc(*iSrcVec++, *iTarVec++, omega);
			}
			
		}

	static void MutateLongVectorUnifAbs(long n, tMutationParams* iMutParams,
								 long* iSrcVec, long* iTarVec, long* oMutVec)
		{
		double		omega		= iMutParams->omega;
		tMutateFunc unifFunc	= iMutParams->function.uniformFunc;
		
		while (n-- > 0) {
			*oMutVec++ = unifFunc(*iSrcVec++, *iTarVec++, omega);
			}
			
		}

	static void MutateFloatVectorUnifAbs(long n, tMutationParams* iMutParams,
								 float* iSrcVec, float* iTarVec, float* oMutVec)
		{
		double		omega		= iMutParams->omega;
		tMutateFunc unifFunc	= iMutParams->function.uniformFunc;
		
		while (n-- > 0) {
			*oMutVec++ = unifFunc(*iSrcVec++, *iTarVec++, omega);
			}
			
		}

	static void MutateDoubleVectorUnifAbs(long n, tMutationParams* iMutParams,
								 double* iSrcVec, double* iTarVec, double* oMutVec)
		{
		double		omega		= iMutParams->omega;
		tMutateFunc unifFunc	= iMutParams->function.uniformFunc;
		
		while (n-- > 0) {
			*oMutVec++ = unifFunc(*iSrcVec++, *iTarVec++, omega);
			}
			
		}

static void
RecurseDimensionsUnifAbs(
	long				iDimCount,
	long				iDimVec[],
	long				iPlaneCount,
	tMutationParams*	iMutParams,
	t_jit_matrix_info*	iSrcInfo,
	char*				iSrcData,
	t_jit_matrix_info*	iTarInfo,
	char*				iTarData,
	t_jit_matrix_info*	iMutInfo,
	char*				iMutData)
	
	{
	long i, n;
		
	if (iDimCount < 1)		// For safety: this also catches invalid (negative) values
		return;
	
	switch(iDimCount) {
	case 1:
		iDimVec[1] = 1;
		// fall into next case...
	case 2:
		// Always treat as single plane data for speed...
		n			= iDimVec[0] * iPlaneCount;
		iPlaneCount	= 1;
		
		if (iSrcInfo->type == _jit_sym_char)
			for (i = 0; i < iDimVec[1]; i += 1) {
				BytePtr	srcVec = (Byte*) (iSrcData + i * iSrcInfo->dimstride[1]),
						tarVec = (Byte*) (iTarData + i * iTarInfo->dimstride[1]),
						mutVec = (Byte*) (iMutData + i * iMutInfo->dimstride[1]);
				
				MutateCharVectorUnifAbs(n, iMutParams, srcVec, tarVec, mutVec);
				}
		
		else if (iSrcInfo->type == _jit_sym_long) 
			for (i = 0; i < iDimVec[1]; i += 1) {
				long*	srcVec = (long*) (iSrcData + i * iSrcInfo->dimstride[1]);
				long*	tarVec = (long*) (iTarData + i * iTarInfo->dimstride[1]);
				long*	mutVec = (long*) (iMutData + i * iMutInfo->dimstride[1]);
				
				MutateLongVectorUnifAbs(n, iMutParams, srcVec, tarVec, mutVec);
				}
		 
		else if (iSrcInfo->type == _jit_sym_float32)
			for (i = 0; i < iDimVec[1]; i += 1) {
				float*	srcVec = (float*) (iSrcData + i * iSrcInfo->dimstride[1]);
				float*	tarVec = (float*) (iTarData + i * iTarInfo->dimstride[1]);
				float*	mutVec = (float*) (iMutData + i * iMutInfo->dimstride[1]);
				
				MutateFloatVectorUnifAbs(n, iMutParams, srcVec, tarVec, mutVec);
				}
		
		else if (iSrcInfo->type == _jit_sym_float64)
			for (i = 0; i < iDimVec[1]; i += 1) {
				double*	srcVec = (double*) (iSrcData + i * iSrcInfo->dimstride[1]);
				double*	tarVec = (double*) (iTarData + i * iTarInfo->dimstride[1]);
				double*	mutVec = (double*) (iMutData + i * iMutInfo->dimstride[1]);
				
				MutateDoubleVectorUnifAbs(n, iMutParams, srcVec, tarVec, mutVec);
				}
		
		break;
	
	default:
		for	(i=0; i < iDimVec[iDimCount-1]; i += 1) {
			char* srcSubvec = iSrcData + i * iSrcInfo->dimstride[iDimCount-1];
			char* tarSubvec = iTarData + i * iTarInfo->dimstride[iDimCount-1];
			char* mutSubvec = iMutData + i * iMutInfo->dimstride[iDimCount-1];
			
			RecurseDimensionsUnifAbs(iDimCount - 1, iDimVec,
									 iPlaneCount, iMutParams,
									 iSrcInfo, srcSubvec,
									 iTarInfo, tarSubvec,
									 iMutInfo, mutSubvec);
			}
		
		break;
		}
	
	}

/******************************************************************************************
 *
 *	RecurseDimensionsIrregAbs(iDimCount, iDimVec, iPlaneCount, iTaus88Data, iMInfo, iBOP)
 *
 *	Everything we need for uniform absolute mutations
 *
 ******************************************************************************************/

	static inline Boolean MutateOrNot(double iOmega, double iPi, Byte iState)
		{
		UInt32	thresh;
		
		switch (iState) {
			case stateSource:
				thresh = CalcSourceToMutantThresh(iOmega, CalcOmegaPrime(iOmega, iPi));
				break;
			case stateTarget:
				thresh = CalcMutantToMutantThresh(CalcOmegaPrime(iOmega, iPi));
				break;
			default:
				thresh = CalcInitThresh(iOmega);
				break;
			}
		
		return WannaMutateStrict(thresh);
		}
	
	static void MutateCharVectorIrregAbs(long n, tMutationParams* iMutParams,
								 BytePtr iSrcVec, BytePtr iTarVec, BytePtr oMutVec)
		{
		double		omega		= iMutParams->omega;
		tMutateFunc unifFunc	= iMutParams->function.uniformFunc;
		
		while (n-- > 0) {
			*oMutVec++ = unifFunc(*iSrcVec++, *iTarVec++, omega);
			}
			
		}

	static void MutateLongVectorIrregAbs(long n, tMutationParams* iMutParams,
								 long* iSrcVec, long* iTarVec, long* oMutVec)
		{
		double		omega		= iMutParams->omega;
		tMutateFunc unifFunc	= iMutParams->function.uniformFunc;
		
		while (n-- > 0) {
			*oMutVec++ = unifFunc(*iSrcVec++, *iTarVec++, omega);
			}
			
		}

	static void MutateFloatVectorIrregAbs(long n, tMutationParams* iMutParams,
								 float* iSrcVec, float* iTarVec, float* oMutVec)
		{
		double		omega		= iMutParams->omega;
		tMutateFunc unifFunc	= iMutParams->function.uniformFunc;
		
		while (n-- > 0) {
			*oMutVec++ = unifFunc(*iSrcVec++, *iTarVec++, omega);
			}
			
		}

	static void MutateDoubleVectorIrregAbs(long n, tMutationParams* iMutParams,
								 double* iSrcVec, double* iTarVec, double* oMutVec)
		{
		double		omega		= iMutParams->omega;
		tMutateFunc unifFunc	= iMutParams->function.uniformFunc;
		
		while (n-- > 0) {
			*oMutVec++ = unifFunc(*iSrcVec++, *iTarVec++, omega);
			}
			
		}

static void
RecurseDimensionsIrregAbs(
	long				iDimCount,
	long				iDimVec[],
	long				iPlaneCount,
	tMutationParams*	iMutParams,
	t_jit_matrix_info*	iSrcInfo,
	char*				iSrcData,
	t_jit_matrix_info*	iTarInfo,
	char*				iTarData,
	t_jit_matrix_info*	iMutInfo,
	char*				iMutData,
	t_jit_matrix_info*	iStateInfo,
	char*				iStateData)
	
	{
	long i, n;
		
	if (iDimCount < 1)		// For safety: this also catches invalid (negative) values
		return;
	
	switch(iDimCount) {
	case 1:
		iDimVec[1] = 1;
		// fall into next case...
	case 2:
		// Flatten planes, treat as single plane data for speed...
		n			= iDimVec[0] * iPlaneCount;
		iPlaneCount	= 1;
		
		if (iSrcInfo->type == _jit_sym_char)
			for (i = 0; i < iDimVec[1]; i += 1) {
				BytePtr	srcVec = (Byte*) (iSrcData + i * iSrcInfo->dimstride[1]),
						tarVec = (Byte*) (iTarData + i * iTarInfo->dimstride[1]),
						mutVec = (Byte*) (iMutData + i * iMutInfo->dimstride[1]);
				
				MutateCharVectorIrregAbs(n, iMutParams, srcVec, tarVec, mutVec);
				}
		
		else if (iSrcInfo->type == _jit_sym_long) 
			for (i = 0; i < iDimVec[1]; i += 1) {
				long*	srcVec = (long*) (iSrcData + i * iSrcInfo->dimstride[1]);
				long*	tarVec = (long*) (iTarData + i * iTarInfo->dimstride[1]);
				long*	mutVec = (long*) (iMutData + i * iMutInfo->dimstride[1]);
				
				MutateLongVectorIrregAbs(n, iMutParams, srcVec, tarVec, mutVec);
				}
		 
		else if (iSrcInfo->type == _jit_sym_float32)
			for (i = 0; i < iDimVec[1]; i += 1) {
				float*	srcVec = (float*) (iSrcData + i * iSrcInfo->dimstride[1]);
				float*	tarVec = (float*) (iTarData + i * iTarInfo->dimstride[1]);
				float*	mutVec = (float*) (iMutData + i * iMutInfo->dimstride[1]);
				
				MutateFloatVectorIrregAbs(n, iMutParams, srcVec, tarVec, mutVec);
				}
		
		else if (iSrcInfo->type == _jit_sym_float64)
			for (i = 0; i < iDimVec[1]; i += 1) {
				double*	srcVec = (double*) (iSrcData + i * iSrcInfo->dimstride[1]);
				double*	tarVec = (double*) (iTarData + i * iTarInfo->dimstride[1]);
				double*	mutVec = (double*) (iMutData + i * iMutInfo->dimstride[1]);
				
				MutateDoubleVectorIrregAbs(n, iMutParams, srcVec, tarVec, mutVec);
				}
		
		break;
	
	default:
		for	(i=0; i < iDimVec[iDimCount-1]; i += 1) {
			char* srcSubvec = iSrcData + i * iSrcInfo->dimstride[iDimCount-1];
			char* tarSubvec = iTarData + i * iTarInfo->dimstride[iDimCount-1];
			char* mutSubvec = iMutData + i * iMutInfo->dimstride[iDimCount-1];
			char* stateSubvec = iStateData + i * iStateInfo->dimstride[iDimCount-1];
			
			RecurseDimensionsIrregAbs(iDimCount - 1, iDimVec,
									 iPlaneCount, iMutParams,
									 iSrcInfo, srcSubvec,
									 iTarInfo, tarSubvec,
									 iMutInfo, mutSubvec,
									 iStateInfo, stateSubvec);
			}
		
		break;
		}
		
	
	}

static t_jit_err
JimPrepareStateMatrix(
	jcobJim*			me,
	long				iDimCount,
	long				iDimVec[],
	long				iPlaneCount,
	t_jit_matrix_info*	oStateMInfo,
	char**				oStateMData,
	long*				oStateSaveLock)
	
	
	{
kill kill kill	
/*
	t_jit_matrix_info	stateInfo;
	char*				stateData;
	voidPtr				stateMatrix		= jit_object_findregistered(me->stateMatrixSym);
	long				stateSaveLock	= jit_object_method(stateMatrix, _jit_sym_lock, 1);
	Boolean				resetState		= false;
	
	// Test if state matrix matches current planes/dimensions
	jit_object_method(stateMatrix, _jit_sym_getinfo, &stateInfo);
	if (stateInfo.dimcount != dimCount || stateInfo.planecount != planeCount)
		resetState = true;
	else for (i = 0; i < dimCount; i += 1)
		if (dim[i] != stateInfo.dim[i]) { resetState = true; break; }
	
	if (resetState) {
		char*	stateData;
		
		
		stateInfo.dimcount		= dimCount;
		stateInfo.planecount	= planeCount;
		for (i = 0; i < dimCount; i += 1) stateInfo.dim[i] = dim[i];
		jit_object_method(matrix, _jit_sym_setinfo, &stateInfo);
		// Get recalculated dimstride
		jit_object_method(matrix, _jit_sym_getinfo, &stateInfo);
		
		// Reset data
		}
*/		
	}

/******************************************************************************************
 *
 *	JimJitMatrixCalc(me, inputs, outputs)
 *
 ******************************************************************************************/

	// ?? Another one for MaxUtils.h, Grommit
	typedef char* charPtr;

static t_jit_err
JimJitMatrixCalc(
	jcobJim*	me,
	void*		inputs,
	void*		outputs)
	
	{
	t_jit_err			err = JIT_ERR_NONE;
	long				i,
						dimCount,
						planeCount,
						dim[JIT_MATRIX_MAX_DIMCOUNT],
						srcSaveLock,
						tarSaveLock,
						mutSaveLock;
	t_jit_matrix_info	srcMInfo,
						tarMInfo,
						mutMInfo;
	charPtr				srcMData,
						tarMData,
						mutMData;
	voidPtr				srcMatrix	= jit_object_method(inputs, _jit_sym_getindex, 0),
						tarMatrix	= jit_object_method(inputs, _jit_sym_getindex, 1),
						mutMatrix	= jit_object_method(outputs, _jit_sym_getindex, 0);

	// Sanity check
	if ((me == NIL) || (srcMatrix == NIL) || (tarMatrix == NIL) || (mutMatrix == NIL))
		return JIT_ERR_INVALID_PTR;
	
	srcSaveLock = (long) jit_object_method(srcMatrix, _jit_sym_lock, 1);
	tarSaveLock = (long) jit_object_method(tarMatrix, _jit_sym_lock, 1);
	mutSaveLock = (long) jit_object_method(mutMatrix, _jit_sym_lock, 1);
	
	jit_object_method(srcMatrix, _jit_sym_getinfo, &srcMInfo);
	jit_object_method(tarMatrix, _jit_sym_getinfo, &tarMInfo);
	jit_object_method(mutMatrix, _jit_sym_getinfo, &mutMInfo);
	
	jit_object_method(srcMatrix, _jit_sym_getdata, &srcMData);
	jit_object_method(tarMatrix, _jit_sym_getdata, &tarMData);
	jit_object_method(mutMatrix, _jit_sym_getdata, &mutMData);
	
	// Do we have data to work with?
	if (srcMData == NIL || tarMData == NIL) {
		err = JIT_ERR_INVALID_INPUT;
		goto alohamora;
		}
	if (mutMData == NIL) {
		err = JIT_ERR_INVALID_OUTPUT;
		goto alohamora;
		}
		
	// Are the types and plane counts compatible?
	dimCount   = srcMInfo.dimcount;
	if (dimCount != tarMInfo.dimcount || dimCount != tarMInfo.dimcount) { 
		err = JIT_ERR_MISMATCH_TYPE; 
		goto alohamora;
		}		
	
	planeCount = srcMInfo.planecount;			
	if (planeCount !=tarMInfo.planecount || planeCount != tarMInfo.planecount) { 
		err = JIT_ERR_MISMATCH_PLANE; 
		goto alohamora;
		}		
	
	// Copy dimensions to our private buffer
	for (i = 0; i < dimCount; i += 1) {
		long minDim = srcMInfo.dim[i];
		
		if (minDim > tarMInfo.dim[i])	minDim = tarMInfo.dim[i];
		if (minDim > mutMInfo.dim[i])	minDim = mutMInfo.dim[i];
		
		dim[i] = minDim;	
		}
			
	if (me->coreMutatorObj.params.irregular) {
		if (me->coreMutatorObj.params.relInterval) {
			// Not there yet...
			}
		else {
			t_jit_matrix_info	stateMInfo;
			char*				stateMData;
			long				stateSaveLock;
			
			err = JimPrepareStateMatrix(me, dimCount, dim, planeCount,
										&stateMInfo, &stateMData, &stateSaveLock);
			if (err == JIT_ERR_NONE) {
				RecurseDimensionsIrregAbs(	dimCount, dim, planeCount, &me->params,
											&srcMInfo, srcMData,
											&tarMInfo, tarMData,
											&mutMInfo, mutMData,
											&stateMInfo, stateMData);
				jit_object_method(me->stateMatrix, _jit_sym_lock, stateSaveLock);
				}
			}
		}
	else {
		if (me->coreMutatorObj.params.relInterval) {
			// Not there yet...
			}
		else RecurseDimensionsUnifAbs(	dimCount, dim, planeCount, &me->params,
										&srcMInfo, srcMData,
										&tarMInfo, tarMData,
										&mutMInfo, mutMData);
		}
	
	
	// Reset the lock state
alohamora:
	jit_object_method(srcMatrix, _jit_sym_lock, srcSaveLock);
	jit_object_method(tarMatrix, _jit_sym_lock, tarSaveLock);
	jit_object_method(mutMatrix, _jit_sym_lock, mutSaveLock);
	
	return err;
	}
	

