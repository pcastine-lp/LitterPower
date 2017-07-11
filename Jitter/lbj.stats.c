/*
	File:		lbj.stats.c

	Contains:	Max/Jitter external object performing statistical analysis of Jitter
				matrices. Sort of jit.3m on steroids.

	Written by:	Peter Castine

	Copyright:	© 2001-2003 Peter Castine

	Change History (most recent first):

         <4>   15–3–2005    pc      Update code to use Jitter attributes; extend documentation based
                                    on comments from Jeremy Bernstein
         <3>   14–3–2005    pc      Fixed calculations. Got communication between Jitter core and
                                    Max shell working. In this versio the Max object directly
                                    accesses members of the Jitter object, which may not be ideal
                                    but at least works and is efficient.
         <2>   12–3–2005    pc      Work on basic functionality.
         <1>      5–3–05    pc      Initial check in.
*/


/******************************************************************************************
	Previous history:

		21-Feb-2005:		First attempt at an implementation.
 ******************************************************************************************/

#pragma mark • Include Files

#include "LitterLib.h"
#include "TrialPeriodUtils.h"


#pragma mark • Constants

const char	kMaxClassName[]		= "lbj.stacey",			// Class name for Max, also used
														// for resources
			kJitClassName[]		= "lbj-stacey";

	// Indices for STR# resource
enum {
	strIndexTheInlet		= lpStrIndexLastStandard + 1,
	
	strIndexOutCount,
	strIndexOutMin,
	strIndexOutMax,
	strIndexOutMean,
	strIndexOutStdDev,
	strIndexOutSkew,
	strIndexOutKurtosis,
	strIndexOutDump,
	
	strIndexInLeft		= strIndexTheInlet,
	strIndexOutLeft		= strIndexOutCount
	};

	// Indices for collected statistics
enum {
	statMin			= 0,
	statMax,
	statSum,
	statSum2,				// Sum of squares
	statSum3,				// Sum of cubes
	statSum4,				// Sum of 4th power.
	
	statArraySize
	};


#pragma mark • Type Definitions

typedef double tPlaneStats[statArraySize];


#pragma mark • Object Structure

typedef struct {
	Object		coreObject;
	voidPtr		jitObEx;			// This is the opaque Jitter data structure that
									// is our sole means of communication with the
									// jcobStacey object (defined below)
	
		// Outlets
	voidPtr		minOutlet,
				maxOutlet,
				meanOutlet,
				stdDevOutlet,
				skewOutlet,
				kurtosisOutlet;
	
	} msobStacey;					// Max Shell Object

typedef struct {
	Object		coreObject;
	
	long		planeCount;
	Atom		minData[JIT_MATRIX_MAX_PLANECOUNT],
				maxData[JIT_MATRIX_MAX_PLANECOUNT],
				meanData[JIT_MATRIX_MAX_PLANECOUNT],
				stdDevData[JIT_MATRIX_MAX_PLANECOUNT],
				skewData[JIT_MATRIX_MAX_PLANECOUNT],
				kurtosisData[JIT_MATRIX_MAX_PLANECOUNT];
	
	} jcobStacey;					// Jitter Core Object


#pragma mark • Global Variables

void*			gStaceyJitClass	= NIL;
Messlist*		gStaceyMaxClass	= NIL;

	// Symbols for accessing Jitter attributes
	// (This is how our Jitter object communicates with the outside world)
SymbolPtr		gSymGetMin		= NIL,
				gSymGetMax		= NIL,
				gSymGetMean		= NIL,
				gSymGetStdDev	= NIL,
				gSymGetSkew		= NIL,
				gSymGetKurtosis	= NIL;

#pragma mark • Function Prototypes

	// Max methods/functions
static void*StaceyNewMaxShell	(Symbol*, long, Atom*);
static void	StaceyFreeMaxShell	(msobStacey*);

static void StaceyBang			(msobStacey*);
static void StaceyTattle		(msobStacey*);
static void	StaceyAssist		(msobStacey*, void*, long, long, char*);
static void	StaceyInfo			(msobStacey*);
static void StaceyMaxMProc		(msobStacey*, void*);

	// Jitter methods/functions
static jcobStacey*		StaceyJitNew	(void);
static void 			StaceyJitFree	(jcobStacey*);

static t_jit_err StaceyJitInit	(void);
static t_jit_err StaceyJitMCalc	(jcobStacey*, void*, void*);



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
	
		// Have to guess about what the following do.
		// Not much is documented in the Jitter SDK
	voidPtr	jitClassExt,
			jitClass;
	
	LITTER_CHECKTIMEOUT(kMaxClassName);
	
	StaceyJitInit();
	
	// Standard Max setup() call
	setup(	&gStaceyMaxClass,					// Pointer to our class definition
			(method) StaceyNewMaxShell,			// Instance creation function
			(method) StaceyFreeMaxShell,		// Custom deallocation function
			(short) sizeof(msobStacey),			// Class object size
			NIL,								// No menu function
			A_GIMME,							// Jitter objects always parse their own
			0);									// arguments
	
	// Jitter Magic... 
	jitClassExt	= max_jit_classex_setup(
					calcoffset(msobStacey, jitObEx)
					);
	jitClass	= jit_class_findbyname(
					gensym((char*) kMaxClassName)
					);    
    max_jit_classex_mop_wrap(jitClassExt, jitClass, kAttrFlags); 		
    max_jit_classex_mop_mproc(jitClassExt, jitClass, StaceyMaxMProc);	// Custom MProc
    max_jit_classex_standard_wrap(jitClassExt, jitClass, 0); 	
	
	// Add messages...
	LITTER_TIMEBOMB addbang	((method) StaceyBang);
	addmess	((method) StaceyTattle,	"dblclick",	A_CANT, 0);
	addmess	((method) StaceyTattle,	"tattle",	A_NOTHING);
	addmess	((method) StaceyAssist,	"assist",	A_CANT, 0);
	addmess	((method) StaceyInfo,	"info",		A_CANT, 0);
	
	// Define global symbols
	gSymGetMin		= gensym("getmin"),
	gSymGetMax		= gensym("getmax"),
	gSymGetMean		= gensym("getmean"),
	gSymGetStdDev	= gensym("getstddev"),
	gSymGetSkew		= gensym("getskew"),
	gSymGetKurtosis	= gensym("getkurtosis");
	
	// Initialize Litter Library
	LitterInit(kMaxClassName, 0);
	}


#pragma mark -
#pragma mark • Internal functions

void
StaceyMaxMProc(
	msobStacey*	me,
	void*				mop)
	
	{
	t_jit_err err = (t_jit_err) jit_object_method(
										max_jit_obex_jitob_get(me),
										_jit_sym_matrix_calc,
										jit_object_method(mop, _jit_sym_getinputlist),
										jit_object_method(mop, _jit_sym_getoutputlist)
										);
	
	if (err == JIT_ERR_NONE)
		 StaceyBang(me);
	else jit_error_code(me, err); 
	
	}

#pragma mark -
#pragma mark • Max Shell Methods

/******************************************************************************************
 *
 *	StaceyNewMaxShell(iSym, iArgC, iArgV)
 *
 ******************************************************************************************/

static void*
StaceyNewMaxShell(
	SymbolPtr	sym,
	long		iArgC,
	Atom		iArgV[])
	
	{
	#pragma unused(sym)
	
	msobStacey*		me			= NIL;
	voidPtr			jitObj		= NIL;
	Symbol*			classSym	= gensym((char*) kMaxClassName);
	
	// In the 1.0 SDK max_jit_obex_new() is prototyped to expect a Maxclass* instead of
	// a Messlist*. JKC said the next release of header files would be corrected to
	// use Messlist* and in the interim users can edit the file accordingly.
	// If we get a compiler "illegal implicit typecast" error, it means Josh forgot to
	// make the change; go to jit.max.h and do it again.
	me = (msobStacey*) max_jit_obex_new(gStaceyMaxClass, classSym);
		if (me == NIL) goto punt;
		
	jitObj = jit_object_new(classSym);
		if (jitObj == NIL) goto punt;
	
	
		// If we make it here, we should be able to set up a new object
		// Jitter voodoo
	max_jit_mop_setup_simple(me, jitObj, iArgC, iArgV);			
	max_jit_attr_args(me, iArgC, iArgV);
	
		// Create standard Max outlets, from right to left
	me->kurtosisOutlet	= outlet_new(me, NIL);
	me->skewOutlet		= outlet_new(me, NIL);
	me->stdDevOutlet	= outlet_new(me, NIL);
	me->meanOutlet		= outlet_new(me, NIL);
	me->maxOutlet		= outlet_new(me, NIL);
	me->minOutlet		= outlet_new(me, NIL);
	
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
 *	StaceyFreeMaxShell(me)
 *
 ******************************************************************************************/

static void
StaceyFreeMaxShell(
	msobStacey* me)
	
	{
	
	max_jit_mop_free(me);
	jit_object_free(max_jit_obex_jitob_get(me));
	max_jit_obex_free(me);
	
	}


/******************************************************************************************
 *
 *	StaceyBang(me)
 *
 ******************************************************************************************/

	static void SendDataToOutlet(void* iJitOb, Symbol* iWhichAttr, void* iWhichOutlet)
		{
		long	atomCount = 0;
		Atom*	outAtoms = NIL;
			
		jit_object_method(iJitOb, iWhichAttr, &atomCount, &outAtoms);

		if (atomCount > 0 && outAtoms != NIL) {	
			if (atomCount > 1)
				outlet_anything(iWhichOutlet, _jit_sym_list, atomCount, outAtoms);
			else switch (outAtoms[0].a_type) {
				case A_LONG:  outlet_int(iWhichOutlet, outAtoms[0].a_w.w_long);		break;
				case A_FLOAT: outlet_float(iWhichOutlet, outAtoms[0].a_w.w_float);	break;
				default:	  break;
				}
			
			jit_freebytes(outAtoms, sizeof(t_atom) * atomCount);
			}
		
		}

void
StaceyBang(
	msobStacey* me)
	
	{
	
	if (max_jit_mop_getoutputmode(me) != 0) {
		jcobStacey* jitOb		= (jcobStacey*) max_jit_obex_jitob_get(me);
		
		SendDataToOutlet(jitOb, gSymGetKurtosis, me->kurtosisOutlet);
		SendDataToOutlet(jitOb, gSymGetSkew, me->skewOutlet);
		SendDataToOutlet(jitOb, gSymGetStdDev, me->stdDevOutlet);
		SendDataToOutlet(jitOb, gSymGetMean, me->meanOutlet);
		SendDataToOutlet(jitOb, gSymGetMax, me->maxOutlet);
		SendDataToOutlet(jitOb, gSymGetMin, me->minOutlet);
		}

	}


/******************************************************************************************
 *
 *	StaceyTattle(me)
 *
 *	StaceyAssist(me, iBox, iDir, iArgNum, oCStr)
 *
 *	Generic Litter approach to standard Max messages
 *
 ******************************************************************************************/

void
StaceyTattle(
	msobStacey* me)
	
	{
	jcobStacey* jitOb = (jcobStacey*) max_jit_obex_jitob_get(me);
	
	post("%s state", kMaxClassName);

#ifdef __DEBUG__
	post("  Max object lcoated at %p", me);
	post("  core Jitter object located at %p", jitOb);
#endif

	post("  Collecting statistics for %ld planes of data", (long) jitOb->planeCount);
	
	}


void StaceyInfo(msobStacey* me)
	{ LitterInfo(kMaxClassName, &me->coreObject, (method) StaceyTattle); }

void StaceyAssist(msobStacey* me, void* box, long iDir, long iArgNum, char* oCStr)
	{
	#pragma unused(me, box)
	
	LitterAssist(iDir, iArgNum, strIndexInLeft, strIndexOutLeft, oCStr);
	}


#pragma mark -
#pragma mark • Jitter Methods

/******************************************************************************************
 *
 *	StaceyJitInit(me)
 *
 ******************************************************************************************/

	static void AddStaceyAttribute(const char iName[], long iDataOffset)
		{
		const long kAttrFlags = JIT_ATTR_SET_OPAQUE_USER | JIT_ATTR_GET_OPAQUE_USER;
		
		jit_class_addattr(
				gStaceyJitClass,
				jit_object_new(
						_jit_sym_jit_attr_offset_array, iName,
						_jit_sym_atom, JIT_MATRIX_MAX_PLANECOUNT,
						kAttrFlags, (method) NIL, (method) NIL,
						calcoffset(jcobStacey, planeCount), iDataOffset
						)
				);
		}

t_jit_err
StaceyJitInit(void) 

	{
	const int	kJitterInlets	= 1,	// One inlet for Jitter Matrices
				kJitterOutlets	= 0;	// No outlets for Jitter Matrices
										// The Max shell object will create outlets for
										// sending out the results of our calculations, but
										// that's none of our business.
	
	// 1) Set up Matrix Operator
	gStaceyJitClass = jit_class_new((char*) kMaxClassName,
									(method) StaceyJitNew,
									(method) StaceyJitFree,		// ?? Could be NIL ??
									sizeof(jcobStacey),
									A_CANT, 0L
									); 
	jit_class_addadornment(
				gStaceyJitClass,
				jit_object_new(_jit_sym_jit_mop, kJitterInlets, kJitterOutlets)
				);
	
	// 2) Add methods
	jit_class_addmethod(gStaceyJitClass, (method) StaceyJitMCalc, "matrix_calc", A_CANT, 0L);
	
	// 3) Add attributes (if any)
	AddStaceyAttribute("min", calcoffset(jcobStacey, minData));
	AddStaceyAttribute("max", calcoffset(jcobStacey, maxData));
	AddStaceyAttribute("mean", calcoffset(jcobStacey, meanData));
	AddStaceyAttribute("stddev", calcoffset(jcobStacey, stdDevData));
	AddStaceyAttribute("skew", calcoffset(jcobStacey, skewData));
	AddStaceyAttribute("kurtosis", calcoffset(jcobStacey, kurtosisData));
	
	// 4) Register the class
	//	This must happen last, after methods & attributes have been added
	jit_class_register(gStaceyJitClass);
	
	return JIT_ERR_NONE;
	}


/******************************************************************************************
 *
 *	StaceyJitNew()
 *	StaceyJitFree(me)
 *
 ******************************************************************************************/

static jcobStacey* StaceyJitNew()
	{ return (jcobStacey*) jit_object_alloc(gStaceyJitClass); }

static void StaceyJitFree(jcobStacey* me)
	{
	#pragma unused(me)
	/* nothing to do */
	}


/******************************************************************************************
 *
 *	StaceyJitMatrixCalc(me, inputs, outputs)
 *
 *	With helper functions:
 *
 *	CalcInit(oStats, iMInfo, iBytes)
 *	StatsChar1D(ioPlaneStats, iOpInfo, iLen)
 *	StatsLong1D(ioPlaneStats, iOpInfo, iLen)
 *	StatsFloat1D(ioPlaneStats, iOpInfo, iLen)
 *	StatsDouble1D(ioPlaneStats, iOpInfo, iLen)
 *	CalcNDim(iDimCount, iDimVec, oStats, iMInfo, iBytes)
 *	CalcEval(me, iStatData, iMInfo)
 *
 ******************************************************************************************/

	static void CalcInit(tPlaneStats oStatData[], t_jit_matrix_info* iMInfo, BytePtr iBytes)
		{
		Symbol*	mType = iMInfo->type;
		long	planeCount = iMInfo->planecount,
				i, j;
		
		// First initialize min/max values according to the type of data we've got
		if (mType == _jit_sym_char) for (i = 0; i < planeCount; i += 1)
			oStatData[i][statMin]	= oStatData[i][statMax]
									= (double) ((Byte*) iBytes)[i];
			
		else if (mType == _jit_sym_long) for (i = 0; i < planeCount; i += 1)
			oStatData[i][statMin]	= oStatData[i][statMax]
									= (double) ((long*) iBytes)[i];
			
		else if (mType == _jit_sym_float32) for (i = 0; i < planeCount; i += 1)
			oStatData[i][statMin]	= oStatData[i][statMax]
									= (double) ((float*) iBytes)[i];
			
		else if (mType == _jit_sym_float64) for (i = 0; i < planeCount; i += 1)
			oStatData[i][statMin]	= oStatData[i][statMax]
									= ((double*) iBytes)[i];
			
		// Initialize all running totals to 0
		for (i = 0; i < planeCount; i += 1) for (j = statSum; j < statArraySize; j += 1)
			oStatData[i][j] = 0.0;
		
		}

	static void
	StatsChar1D(
		tPlaneStats				ioPlaneStats,
		const t_jit_op_info* 	iOpInfo,
		long					iLen)
		 
		{
		BytePtr	myBytes	= (BytePtr) iOpInfo->p;
		long	stride	= iOpInfo->stride;
		
		do  {
			double	val = (double) *myBytes,
					val2;
			
			if (ioPlaneStats[statMin] > val) ioPlaneStats[statMin] = val;	
			if (ioPlaneStats[statMax] < val) ioPlaneStats[statMax] = val;	
			
			ioPlaneStats[statSum]	+= (val2 = val);
			ioPlaneStats[statSum2]	+= (val2 *= val);
			ioPlaneStats[statSum3]	+= (val2 *= val);
			ioPlaneStats[statSum4]	+= (val2 *= val);
			
			myBytes += stride;
			} while (--iLen > 0);
		
		}

	static void
	StatsLong1D(
		tPlaneStats				ioPlaneStats,
		const t_jit_op_info* 	iOpInfo,
		long					iLen)
		
		{
		long*	myLongs	= (long*) iOpInfo->p;
		long	stride	= iOpInfo->stride;
		
		do  {
			double	val = (double) *myLongs,
					val2;
			
			if (ioPlaneStats[statMin] > val) ioPlaneStats[statMin] = val;	
			if (ioPlaneStats[statMax] < val) ioPlaneStats[statMax] = val;	
			
			ioPlaneStats[statSum]	+= (val2 = val);
			ioPlaneStats[statSum2]	+= (val2 *= val);
			ioPlaneStats[statSum3]	+= (val2 *= val);
			ioPlaneStats[statSum4]	+= (val2 *= val);
			
			myLongs += stride;
			} while (--iLen > 0);
		
		}

	static void
	StatsFloat1D(
		tPlaneStats				ioPlaneStats,
		const t_jit_op_info* 	iOpInfo,
		long					iLen)
		
		{
		float*	myFloats	= (float*) iOpInfo->p;
		long	stride		= iOpInfo->stride;
		
		do  {
			double	val = (double) *myFloats,
					val2;
			
			if (ioPlaneStats[statMin] > val) ioPlaneStats[statMin] = val;	
			if (ioPlaneStats[statMax] < val) ioPlaneStats[statMax] = val;	
			
			ioPlaneStats[statSum]	+= (val2 = val);
			ioPlaneStats[statSum2]	+= (val2 *= val);
			ioPlaneStats[statSum3]	+= (val2 *= val);
			ioPlaneStats[statSum4]	+= (val2 *= val);
			
			myFloats += stride;
			} while (--iLen > 0);
		
		}

	static void
	StatsDouble1D(
		tPlaneStats				ioPlaneStats,
		const t_jit_op_info* 	iOpInfo,
		long					iLen)
		
		
		{
		double*	myDoubles	= (double*) iOpInfo->p;
		long	stride		= iOpInfo->stride;
		
		do  {
			double	val = *myDoubles,
					val2;
			
			if (ioPlaneStats[statMin] > val) ioPlaneStats[statMin] = val;	
			if (ioPlaneStats[statMax] < val) ioPlaneStats[statMax] = val;	
			
			ioPlaneStats[statSum]	+= (val2 = val);
			ioPlaneStats[statSum2]	+= (val2 *= val);
			ioPlaneStats[statSum3]	+= (val2 *= val);
			ioPlaneStats[statSum4]	+= (val2 *= val);
			
			myDoubles += stride;
			} while (--iLen > 0);
		
		}

static void
CalcNDim(
	long				iDimCount,
	long				iDimVec[],
	tPlaneStats			ioStatData[],
	t_jit_matrix_info*	iMInfo,
	BytePtr				iBytes)
	
	{
	long i;
		
	switch(iDimCount) {
	case 1:
		iDimVec[1] = 1;
		// fall into next case...
	case 2:
		{
		long 			len0  = iDimVec[0],
						plane;
		t_jit_op_info	opInfo;
		
		opInfo.stride = (iMInfo->dim[0] > 1) ? iMInfo->planecount : 0;
		
		if (iMInfo->type == _jit_sym_char)
			for (i = 0; i < iDimVec[1]; i += 1)
				for (plane = 0; plane < iMInfo->planecount; plane += 1) {
					opInfo.p = iBytes + i * iMInfo->dimstride[1] + plane * sizeof(Byte);
					StatsChar1D(ioStatData[plane], &opInfo, len0);
					}
		
		else if (iMInfo->type == _jit_sym_long)
			for (i = 0; i < iDimVec[1]; i += 1)
				for (plane = 0; plane < iMInfo->planecount; plane += 1) {
					opInfo.p = iBytes + i * iMInfo->dimstride[1] + plane * sizeof(long);
					StatsLong1D(ioStatData[plane], &opInfo, len0);
					}
		
		else if (iMInfo->type == _jit_sym_float32)
			for (i = 0; i < iDimVec[1]; i += 1)
				for (plane = 0; plane < iMInfo->planecount; plane += 1) {
					opInfo.p = iBytes + i * iMInfo->dimstride[1] + plane * sizeof(float);
					StatsFloat1D(ioStatData[plane], &opInfo, len0);
					}
		
		else if (iMInfo->type == _jit_sym_float64)
			for (i = 0; i < iDimVec[1]; i += 1)
				for (plane = 0; plane < iMInfo->planecount; plane += 1) {
					opInfo.p = iBytes + i * iMInfo->dimstride[1] + plane * sizeof(double);
					StatsDouble1D(ioStatData[plane], &opInfo, len0);
					}
			
		}
		break;
	
	default:
		// Larger values of iDimCount
		// Make absolutely sure that iDimCount really is larger
		// All hell would break loose with a negative or zero value
		if (iDimCount > 0) for (i=0; i < iDimVec[iDimCount-1]; i += 1)
			CalcNDim(iDimCount - 1,
					 iDimVec, ioStatData, iMInfo, 
					 iBytes  + i * iMInfo->dimstride[iDimCount-1]);
		
		break;
		}
	
	}


	static void
	CalcEval(
		jcobStacey*			me,
		const tPlaneStats	iStatData[],
		t_jit_matrix_info*	iMInfo)
		
		{
		long	cellCount = 1,
				i;
		double	oneOverCellCount;
		
		// Store plane count
		me->planeCount = iMInfo->planecount;
		
		// Store min/max values as Atoms of the appropriate type
		if (iMInfo->type == _jit_sym_char || iMInfo->type == _jit_sym_char)
			for (i = 0; i < iMInfo->planecount; i += 1) {
				AtomSetLong(&me->minData[i], iStatData[i][statMin]);
				AtomSetLong(&me->maxData[i], iStatData[i][statMax]);
				}
		
		else if (iMInfo->type == _jit_sym_float32 || iMInfo->type == _jit_sym_float64)
			for (i = 0; i < iMInfo->planecount; i += 1) {
				AtomSetFloat(&me->minData[i], iStatData[i][statMin]);
				AtomSetFloat(&me->maxData[i], iStatData[i][statMax]);
				}
		
		// Calculate mean, stdDev, skew, and kurtosis and store them in our atoms
		for (i = 0; i < iMInfo->dimcount; i += 1)
			cellCount *= iMInfo->dim[i];
		oneOverCellCount = 1.0 / (double) cellCount;
		
		for (i = 0; i < iMInfo->planecount; i += 1) {
			double	mean,
					stdDev		= 0.0,
					skew		= 0.0,
					kurtosis	= 0.0;
			
			// If all values are identical we can short-circuit the hard math
			if (iStatData[i][statMin] == iStatData[i][statMax])
				mean = iStatData[i][statMin];
			
			else {
				// Here goes...
				double	sum			= iStatData[i][statSum],
						sum2		= iStatData[i][statSum2],
						mean2,
						variance;
				
				// Mean (calculate mean squared while we're at it)
				mean	= sum * oneOverCellCount;
				mean2	= mean * mean;
				
				// Standard Deviation
				variance  = -mean * sum;
				variance += variance;
				variance += sum2;
				variance += cellCount * mean2;
					// It is unlikely at this stage that we have zero variance, but
					// check to avoid NaNs
				if (variance != 0.0) {
					double	sum3	= iStatData[i][statSum3],
							sum4	= iStatData[i][statSum4],
							mean3	= mean2 * mean,
							kurTemp1,
							kurTemp2;
					
					variance *= oneOverCellCount;
					stdDev	 = sqrt(variance);
					
					// Skew
					skew = mean2 * sum;
					skew -= mean * sum2;
					skew += skew + skew;			// Cheap but cheasy way to quadruple
					skew += sum3;
					skew -= cellCount * mean3;
					skew /= cellCount * variance * stdDev;
					
					// Kurtosis
					kurTemp1 = mean2 * sum2;
					kurTemp2 = mean * sum3 + mean3 * sum;
					kurTemp1 += kurTemp1 + kurTemp1 - kurTemp2 - kurTemp2;
					kurtosis = sum4 + kurTemp1 + kurTemp1 + cellCount * mean2 * mean2;
					kurtosis /= cellCount * variance * variance;
					kurtosis -= 3.0;
					}
				}
				
			AtomSetFloat(&me->meanData[i], mean);
			AtomSetFloat(&me->stdDevData[i], stdDev);
			AtomSetFloat(&me->skewData[i], skew);
			AtomSetFloat(&me->kurtosisData[i], kurtosis);
			}
		
		}

static t_jit_err
StaceyJitMCalc(
	jcobStacey*	me,
	void*		inputs,
	void*		outputs)
	
	{
	#pragma unused(outputs)
	
	t_jit_err			err = JIT_ERR_NONE;
	t_jit_matrix_info	inMInfo;
	BytePtr				inBytes;
	long				saveLock,
						i,
						dimCount,
						dim[JIT_MATRIX_MAX_DIMCOUNT];
	tPlaneStats			statData[JIT_MATRIX_MAX_DIMCOUNT];
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
			
	CalcInit(statData, &inMInfo, inBytes);
	CalcNDim(dimCount, dim, statData, &inMInfo, inBytes);
	CalcEval(me, statData, &inMInfo);
	
alohamora:
	jit_object_method(inMatrix, _jit_sym_lock, saveLock);
	return err;
	}
	

