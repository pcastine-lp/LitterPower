/*
	File:		lbj.penize.c

	Contains:	Max/Jitter external object tracking changes on a sampling of cells in a
				matrix.

	Written by:	Peter Castine

	Copyright:	© 2005 Peter Castine

	Change History (most recent first):

         <1>   23–3–2006    pc      first checked in.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "LitterLib.h"
#include "TrialPeriodUtils.h"


#pragma mark • Constants

const char	kMaxClassName[]		= "lbj.penize",			// Class name for Max, also used
														// for resources
			kJitClassName[]		= "lbj-penize";

	// Indices for STR# resource
enum {
	strIndexTheInlet		= lpStrIndexLastStandard + 1,
	
	strIndexOutSamples,
	strIndexOutDump,
	
	strIndexInLeft		= strIndexTheInlet,
	strIndexOutLeft		= strIndexOutSamples
	};

	// Change modes
enum changeMode {
	modeSign		= 0,			// Signed differences
	modeAbs,						// Absolute differences
	modeRMS							// RMS differences (useful for multi-plane matrices)
	};


#pragma mark • Type Definitions

typedef enum changeMode		eChangeMode;


#pragma mark • Object Structure

typedef struct {
	Object		coreObject;
	voidPtr		jitObEx;			// This is the opaque Jitter data structure that
									// is our sole means of communication with the
									// jcobPenize object (defined below)
	
	} msobPenize;					// Max Shell Object

typedef struct {
	Object			coreObject;
	
	eChangeMode		mode;
	Symbol*			matrixType;		// char, long, float32, or float64
	
	Byte			planeCount;
	long			sampleCount,	// Number of cells to track
					sampleSize;		// Size of each cell in bytes
									// (planeCount * sizeof(char|long|float|double))
									
	Byte*			PrevSamples;	// Variable-length pointer to variable-type data:
									// buffer data sampled from last matrix
									// Must typecast to use
									// Buffer must be sampleCount * sampleSize bytes in size
	
	Atom*			changeResults;	// Variable-length vector, one Atom per sample
	
	unsigned long*	offsets;		// Variable-length vector, one offset per sample
	
	long			seed;			// Seed for generating offsets
									
	} jcobPenize;					// Jitter Core Object


#pragma mark • Global Variables

void*			gPenizeJitClass	= NIL;
Messlist*		gPenizeMaxClass	= NIL;

	// Symbols for accessing Jitter attributes
	// (This is how our Jitter object communicates with the outside world)
SymbolPtr		gSymGetDiffs	= NIL;

#pragma mark • Function Prototypes

	// Max methods/functions
static void*PenizeNewMaxShell	(Symbol*, long, Atom*);
static void	PenizeFreeMaxShell	(msobPenize*);

static void PenizeBang			(msobPenize*);
static void PenizeTattle		(msobPenize*);
static void	PenizeAssist		(msobPenize*, void*, long, long, char*);
static void	PenizeInfo			(msobPenize*);
static void PenizeMaxMProc		(msobPenize*, void*);

	// Jitter methods/functions
jcobPenize*		PenizeJitNew	(void);
void 			PenizeJitFree	(jcobPenize*);

static t_jit_err PenizeJitInit	(void);
static t_jit_err PenizeJitMCalc	(jcobPenize*, void*, void*);



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
	const long kAttrFlags = MAX_JIT_MOP_FLAGS_OWN_OUTPUTMATRIX
								| MAX_JIT_MOP_FLAGS_OWN_BANG;
	
	voidPtr	p,									// Have to guess about what these two do
			q;									// Not much is documented in the Jitter SDK
	
	LITTER_CHECKTIMEOUT(kClassName);
	
	PenizeJitInit();
	
	// Standard Max setup() call
	setup(	&gPenizeMaxClass,					// Pointer to our class definition
			(method) PenizeNewMaxShell,			// Instance creation function
			(method) PenizeFreeMaxShell,		// Custom deallocation function
			(short) sizeof(msobPenize),			// Class object size
			NIL,								// No menu function
			A_GIMME,							// Jitter objects always parse their own
			0);									// arguments
	
	// Jitter Magic... 
	p	= max_jit_classex_setup(
					calcoffset(msobPenize, jitObEx)
					);
	q	= jit_class_findbyname(
					gensym((char*) kMaxClassName)
					);    
    max_jit_classex_mop_wrap(p, q, kAttrFlags); 		
    max_jit_classex_mop_mproc(p, q, PenizeMaxMProc);	// Custom MProc
    max_jit_classex_standard_wrap(p, q, 0); 	
	
	// Add messages...
	LITTER_TIMEBOMB addbang	((method) PenizeBang);
	addmess	((method) PenizeTattle,	"dblclick",	A_CANT, 0);
	addmess	((method) PenizeTattle,	"tattle",	A_NOTHING);
	addmess	((method) PenizeAssist,	"assist",	A_CANT, 0);
	addmess	((method) PenizeInfo,	"info",		A_CANT, 0);
	
	// Define global symbols
	gSymGetDiffs	= gensym("getdiffs");
	
	// Initialize Litter Library
	LitterInit(kMaxClassName, 0);
	}


#pragma mark -
#pragma mark • Internal functions

void
PenizeMaxMProc(
	msobPenize*	me,
	void*				mop)
	
	{
	t_jit_err err = (t_jit_err) jit_object_method(
										max_jit_obex_jitob_get(me),
										_jit_sym_matrix_calc,
										jit_object_method(mop, _jit_sym_getinputlist),
										jit_object_method(mop, _jit_sym_getoutputlist)
										);
	
	if (err == JIT_ERR_NONE)
		 PenizeBang(me);
	else jit_error_code(me, err); 
	
	}

#pragma mark -
#pragma mark • Max Shell Methods

/******************************************************************************************
 *
 *	PenizeNewMaxShell(iSym, iArgC, iArgV)
 *
 ******************************************************************************************/

static void*
PenizeNewMaxShell(
	SymbolPtr,
	long		iArgC,
	Atom		iArgV[])
	
	{
	msobPenize*		me			= NIL;
	voidPtr			jitObj		= NIL;
	Symbol*			classSym	= gensym((char*) kMaxClassName);
	
	// In the 1.0 SDK max_jit_obex_new() is prototyped to expect a Maxclass* instead of
	// a Messlist*. JKC said the next release of header files would be corrected to
	// use Messlist* and in the interim users can edit the file accordingly.
	// If we get a compiler "illegal implicit typecast" error, it means Josh forgot to
	// make the change; go to jit.max.h and do it again.
	me = (msobPenize*) max_jit_obex_new(gPenizeMaxClass, classSym);
		if (me == NIL) goto punt;
		
	jitObj = jit_object_new(classSym);
		if (jitObj == NIL) goto punt;
	
	
		// If we make it here, we should be able to set up a new object
		// Jitter voodoo
	max_jit_mop_setup_simple(me, jitObj, iArgC, iArgV);			
	max_jit_attr_args(me, iArgC, iArgV);
	
		// Create standard Max outlets, from right to left
		// Access through core Max object
	outlet_new(me, NIL);
	
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
 *	PenizeFreeMaxShell(me)
 *
 ******************************************************************************************/

static void
PenizeFreeMaxShell(
	msobPenize* me)
	
	{
	
	max_jit_mop_free(me);
	jit_object_free(max_jit_obex_jitob_get(me));
	max_jit_obex_free(me);
	
	}


/******************************************************************************************
 *
 *	PenizeBang(me)
 *
 ******************************************************************************************/

		// Should think about generalizing and moving this to MaxUtils... 
	static inline void SendDataToOutlet(void* iJitOb, Symbol* iAttr, void* iOutlet)
		{
		long		atomCount = 0;
		Atom*		outAtoms = NIL;
		
			
		jit_object_method(iJitOb, iAttr, &atomCount, &outAtoms);
			// This method attempts to memory for outAtoms
			// Don't forget to call jit_freebytes() when done

		if (atomCount > 0 && outAtoms != NIL) {	
			if (atomCount > 1)
				outlet_anything(iOutlet, _jit_sym_list, atomCount, outAtoms);
			else switch (outAtoms[0].a_type) {
				case A_LONG:  outlet_int(iOutlet, outAtoms[0].a_w.w_long);		break;
				case A_FLOAT: outlet_float(iOutlet, outAtoms[0].a_w.w_float);	break;
				default:	  break;
				}
			
			jit_freebytes(outAtoms, sizeof(t_atom) * atomCount);
			}
		
		}

void
PenizeBang(
	msobPenize* me)
	
	{
	
	if (max_jit_mop_getoutputmode(me) != 0) {
		jcobPenize* jitOb = (jcobPenize*) max_jit_obex_jitob_get(me);
		
		SendDataToOutlet(jitOb, gSymGetDiffs, me->coreObject.o_outlet);
		}

	}


/******************************************************************************************
 *
 *	PenizeTattle(me)
 *
 *	PenizeAssist(me, iBox, iDir, iArgNum, oCStr)
 *
 *	Generic Litter approach to standard Max messages
 *
 ******************************************************************************************/

void
PenizeTattle(
	msobPenize* me)
	
	{
	jcobPenize* jitOb = (jcobPenize*) max_jit_obex_jitob_get(me);
	
	post("%s state", kMaxClassName);

#ifdef __DEBUG__
	post("  Max object lcoated at %p", me);
	post("  core Jitter object located at %p", jitOb);
#endif

	post("  Collecting statistics for %ld planes of data", (long) jitOb->planeCount);
	
	}


void PenizeInfo(msobPenize* me)
	{ LitterInfo(kMaxClassName, &me->coreObject, (method) PenizeTattle); }

void PenizeAssist(msobPenize*, void*, long iDir, long iArgNum, char* oCStr)
	{ LitterAssist(iDir, iArgNum, strIndexInLeft, strIndexOutLeft, oCStr); }


#pragma mark -
#pragma mark • Jitter Methods

/******************************************************************************************
 *
 *	PenizeJitInit(me)
 *
 ******************************************************************************************/

	static inline void AddPenizeAttribute(const char iName[], long iDataOffset)
		{
		const long kAttrFlags = JIT_ATTR_SET_OPAQUE_USER | JIT_ATTR_GET_OPAQUE_USER;
		
		jit_class_addattr(
				gPenizeJitClass,
				jit_object_new(
						_jit_sym_jit_attr_offset_array, iName,
						_jit_sym_atom, JIT_MATRIX_MAX_PLANECOUNT,
						kAttrFlags, (method) NIL, (method) NIL,
						calcoffset(jcobPenize, planeCount), iDataOffset
						)
				);
		}

t_jit_err
PenizeJitInit(void) 

	{
	const int	kJitterInlets	= 1,	// One inlet for Jitter Matrices
				kJitterOutlets	= 0;	// No outlets for Jitter Matrices
										// The Max shell object will create outlets for
										// sending out the results of our calculations, but
										// that's none of our business.
	
	// 1) Set up Matrix Operator
	gPenizeJitClass = jit_class_new((char*) kMaxClassName,
									(method) PenizeJitNew,
									(method) PenizeJitFree,		// ?? Could be NIL ??
									sizeof(jcobPenize),
									A_CANT, 0L
									); 
	jit_class_addadornment(
				gPenizeJitClass,
				jit_object_new(_jit_sym_jit_mop, kJitterInlets, kJitterOutlets)
				);
	
	// 2) Add methods
	jit_class_addmethod(gPenizeJitClass, (method) PenizeJitMCalc, "matrix_calc", A_CANT, 0L);
	
	// 3) Add attributes (if any)
	AddPenizeAttribute("diffs", calcoffset(jcobPenize, changeResults));
	
	// 4) Register the class
	//	This must happen last, after methods & attributes have been added
	jit_class_register(gPenizeJitClass);
	
	return JIT_ERR_NONE;
	}


/******************************************************************************************
 *
 *	PenizeJitNew()
 *	PenizeJitFree(me)
 *
 ******************************************************************************************/

static jcobPenize* PenizeJitNew()
	{ return (jcobPenize*) jit_object_alloc(gPenizeJitClass); }

static void PenizeJitFree(jcobPenize*)
	{ /* nothing to do */ }


/******************************************************************************************
 *
 *	PenizeJitMatrixCalc(me, inputs, outputs)
 *
 *	With helper functions:
 *
 *	StatsChar1D(ioPlaneStats, iOpInfo, iLen)
 *	StatsLong1D(ioPlaneStats, iOpInfo, iLen)
 *	StatsFloat1D(ioPlaneStats, iOpInfo, iLen)
 *	StatsDouble1D(ioPlaneStats, iOpInfo, iLen)
 *	CalcNDim(iDimCount, iDimVec, oStats, iMInfo, iBytes)
 *	CalcEval(me, iStatData, iMInfo)
 *
 ******************************************************************************************/

	static void PenizeCalcOffsets(
		jcobPenize*			me,
		long				iDimCount,
		long				iDimVec[],
		t_jit_matrix_info*	iMInfo,
		BytePtr				iBytes)
		
		{
		
		}
	


static void
PenizeCalcDiffs(
	jcobPenize*			me,
	long				iDimCount,
	long				iDimVec[],
	t_jit_matrix_info*	iMInfo,
	BytePtr				iBytes)
	
	{
	long i;
		
	}



static t_jit_err
PenizeJitMCalc(
	jcobPenize*	me,
	void*		inputs,
	void*		/* outputs */)
	
	{
	t_jit_err			err = JIT_ERR_NONE;
	t_jit_matrix_info	inMInfo;
	BytePtr				inBytes;
	long				saveLock,
						i,
						dimCount,
						dim[JIT_MATRIX_MAX_DIMCOUNT];
	void*				inMatrix = jit_object_method(inputs, _jit_sym_getindex, 0);

	// Sanity check
	if ((me == NIL) || (inMatrix == NIL))
		return JIT_ERR_INVALID_PTR;
	
	saveLock = (long) jit_object_method(inMatrix, _jit_sym_lock, 1);
	
	jit_object_method(inMatrix, _jit_sym_getinfo, &inMInfo);
	jit_object_method(inMatrix, _jit_sym_getdata, &inBytes);
	
	// More sanity
	if (inBytes == NIL) {
		err = JIT_ERR_INVALID_OUTPUT;
		me->planeCount = 0;					// Prevents us from sending garbage out
		goto alohamora;						// Need to unlock 
		}
	
	// Get dimensions
	// In a departure from standard procedure, we don't need the plane count here
	dimCount   = inMInfo.dimcount;
	for (i=0; i < dimCount; i += 1)
		dim[i] = inMInfo.dim[i];
			
	PenizeCalcDiffs(me, dimCount, dim, &inMInfo, inBytes);
	
alohamora:
	jit_object_method(inMatrix, _jit_sym_lock, saveLock);
	return err;
	}
	

