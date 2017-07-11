/*
	File:		latoocarfian~.c

	Contains:	MSP external object generating signals from Clifford Pickover's eponymous
				chaotic function. Extended (à la FRACTINT) to support variants using
				functions other than sine.

	Written by:	Peter Castine

	Copyright:	© 2002 Peter Castine

	Change History (most recent first):

         <6>    10–12–04    pc      Update to use proxy_getinlet()
         <5>     11–1–04    pc      Update for Windows (keep file up to conventions while I have
                                    the changes in my head).
         <4>   24–8–2003    pc      Use new LitterInfo method (incl. gModDate, LitterAddClass, etc.)
         <3>  30–12–2002    pc      Drop faux 'Vers' resource and related modifications. Add object
                                    version to DoInfo().
         <2>  28–11–2002    pc      Tidy up after check in.
         <1>  28–11–2002    pc      Initial check in.
		12-Apr-2002:	First implementation.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Identify Target


#pragma mark • Include Files

#include "LitterLib.h"
#include "z_dsp.h"


#pragma mark • Constants

const char*	kClassName		= "lp.fiana~";			// Class name



	// Indices for STR# resource
enum {
	strIndexInBang		= lpStrIndexLastStandard + 1,
	strIndexInA,
	strIndexInB,
	strIndexInC,
	strIndexInD,
	
	strIndexOutX,
	strIndexOutY,
	
	strIndexInLeft		= strIndexInBang,
	strIndexOutLeft		= strIndexOutX
	};


	// Indices for functions we support
enum Function {
	fnIdent			= 0,
	fnZero,
	fnOne,
	fnRecip,
	fnSin,
	fnCos,
	fnTan,
	fnCot,
	fnSinPi,
	fnCosPi,
	fnTanPi,
	fnCotPi,
	fnSinh,
	fnCosh,
	fnTanh,
	fnLog,
	fnExp,
	fnSqr,
	fnRoot,
	
	fnFirst		= fnIdent,
	fnLast		= fnRoot
	};

	// Function names
const char*		kFuncNames[fnLast + 1]
						= {	"ident",
							"zero",
							"one",
							"recip",
							"sin",
							"cos",
							"tan",
							"cot",
							"sinpi",
							"cospi",
							"tanpi",
							"cotpi",
							"sinh",
							"cosh",
							"tanh",
							"log",
							"exp",
							"sqr",
							"sqrt"
							};


#pragma mark • Type Definitions

typedef enum Function tFunction;

typedef double (*tLatooFunc)(double);
	


#pragma mark • Object Structure

typedef struct {
	t_pxobject		coreObject;
	
	double			x0,	y0,						// Initial coordinates
					x,	y,						// Current coordinates
					a, b, c, d;					// Parameters

	tFunction		fnA, fnB, fnC, fnD;			// Functions
	
	Boolean			autoParam,
					autoSeed;
	} tfiana;


#pragma mark • Global Variables



#pragma mark • Function Prototypes

	// Class message functions
void*		Newfiana(Symbol*, short, Atom*);

	// Object message functions
static void DoBang(tfiana*);
static void DoFloat(tfiana*, double);
static void DoInt(tfiana*, long);						// Because Max no longer does
														// this automatically :-(
static void DoSeed(tfiana*, Symbol*, short, Atom*);		// List message is specifically
														// new initial coordinate
static void DoSet(tfiana*, double, double);
static void DoReset(tfiana*);

	// Set functions (inlet 0 ==> global, else a specific slot
static void	DoIdent(tfiana*);
static void	DoZero(tfiana*);
static void	DoOne(tfiana*);
static void	DoRecip(tfiana*);
static void	DoSin(tfiana*);
static void	DoCos(tfiana*);
static void	DoTan(tfiana*);
static void	DoCot(tfiana*);
static void	DoSinPi(tfiana*);
static void	DoCosPi(tfiana*);
static void	DoTanPi(tfiana*);
static void	DoCotPi(tfiana*);
static void	DoSinh(tfiana*);
static void	DoCosh(tfiana*);
static void	DoTanh(tfiana*);
static void	DoLog(tfiana*);
static void	DoExp(tfiana*);
static void	DoSqr(tfiana*);
static void DoRoot(tfiana*);

static void DoTattle(tfiana*);
static void	DoAssist(tfiana*, void* , long , long , char*);
static void	DoInfo(tfiana*);

static void DoDSP(tfiana*, t_signal**, short*);
static int* Performfiana(int*);


#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark • Inline Functions


#pragma mark -

/******************************************************************************************
 *
 *	main()
 *	
 *	Standar Max External Object Entry Point Function
 *	
 ******************************************************************************************/

void
main(void*)				// Parameter is obsolete (68k legacy)
	
	{
	short	externResFile,
			prevResFile;
	
	// Standard Max setup() call
	setup(	&gObjectClass,				// Pointer to our class definition
			(method) Newfiana,			// Instance creation function
			(method) dsp_free,			// Std. MSP deallocation function
			sizeof(tfiana),				// Class object size
			NIL,						// No menu function
			A_GIMME,					// Arguments (all optional): 0 to 4 symbols,
			0);							//	followed 0 to 6 floats (Why do I do this?)
		
	dsp_initclass();

	// Messages
	addbang	((method) DoBang);
	addfloat((method) DoFloat);
	addint	((method) DoInt);
	addmess	((method) DoSeed,	"list",		A_GIMME, 0);
	addmess	((method) DoSet,	"set",		A_FLOAT, A_FLOAT, 0);
	addmess	((method) DoReset,	"reset",	A_NOTHING);
	
	addmess	((method) DoIdent,	"ident",	A_NOTHING);
	addmess	((method) DoZero,	"zero",		A_NOTHING);
	addmess	((method) DoOne,	"one",		A_NOTHING);
	addmess	((method) DoRecip,	"recip",	A_NOTHING);
	addmess	((method) DoSin,	"sin",		A_NOTHING);
	addmess	((method) DoCos,	"cos",		A_NOTHING);
	addmess	((method) DoTan,	"tan",		A_NOTHING);
	addmess	((method) DoCot,	"cot",		A_NOTHING);
	addmess	((method) DoSinPi,	"sinpi",	A_NOTHING);
	addmess	((method) DoCosPi,	"cospi",	A_NOTHING);
	addmess	((method) DoTanPi,	"tanpi",	A_NOTHING);
	addmess	((method) DoCotPi,	"cotpi",	A_NOTHING);
	addmess	((method) DoSinh,	"sinh",		A_NOTHING);
	addmess	((method) DoCosh,	"cosh",		A_NOTHING);
	addmess	((method) DoTanh,	"tanh",		A_NOTHING);
	addmess	((method) DoLog,	"log",		A_NOTHING);
	addmess	((method) DoExp,	"exp",		A_NOTHING);
	addmess	((method) DoSqr,	"sqr",		A_NOTHING);
	addmess	((method) DoRoot,	"sqrt",		A_NOTHING);
	
	addmess	((method) DoTattle,	"dblclick",	A_CANT, 0);
	addmess	((method) DoTattle,	"tattle",	A_NOTHING);
	addmess	((method) DoAssist,	"assist",	A_CANT, 0);
	addmess	((method) DoInfo,	"info",		A_CANT, 0);
	
	// MSP messages
	addmess	((method) DoDSP,	"dsp",		A_CANT, 0);

	// Initialize Litter Library
	// Bang message generates random (X0, Y0)
	LitterInit(kClassName, 0);
	
	}

#pragma mark -
#pragma mark • Utility Functions

/******************************************************************************************
 *
 *	MapStrToFunction(iStr)
 *	
 *	Also define non-built in functions and array of addresses here.
 *	
 ******************************************************************************************/

static tFunction
MapStrToFunction(
	const char* iStr)
	{
	tFunction result;
	
	for (result = fnLast; result >= 0; result -= 1) {
		if (strcmp(iStr, kFuncNames[result]) == 0) break;
		} 
	
	return result;
	}


#pragma mark -
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	Newfiana(iName, iArgC, iArgV)
 *	
 *	Arguments:
 *		iName is always "lp.poppy" and we do not use this paramter
 *		iArgC and iArgV specify a list of atoms, which will normally all be floats.
 *		The first item is a growth rate.
 *		If there is more than one item in the list, the last item is an initial seed.
 *		Any additional items before 
 *	
 ******************************************************************************************/

	static double RandXY(void)
		{ return 2.5 * ULong2Unit_zO(Taus88(NIL)); }
	
	static double RandAC(void)
		{ return kPi * ULong2Unit_zO(Taus88(NIL)); }
	
	static double RandBD(void)
		{ return ULong2Unit_zO(Taus88(NIL)); }

void*
Newfiana(
	Symbol*	,
	short	iArgC,
	Atom*	iArgV)
	
	{
	const tFunction	kDefFunc	= fnSin;	// One size fits all
	
	tfiana*		me			= NIL;
	int			symArgC		= 0,
				floatArgC	= 0;

	
	//
	// Let Max allocate us, our inlets, and outlets
	//
	me = (tfiana*) newobject(gObjectClass);
		if (me == NIL) goto punt;
	
		// fiana does proxies; even if we don't want signals in all of them.
	dsp_setup(&(me->coreObject), 5);
	
		// Two outlets
	outlet_new(me, "signal");
	outlet_new(me, "signal");
	
	//
	// Set up initial values
	//
	
	me->x = me->x0	= RandXY();
	me->y = me->y0	= RandXY();
	me->a			= RandAC();
	me->b			= RandBD();
	me->c			= RandAC();
	me->d			= RandBD();
	me->fnA			= kDefFunc;
	me->fnB			= kDefFunc;
	me->fnC			= kDefFunc;
	me->fnD			= kDefFunc;
	me->autoParam	= true;
	me->autoSeed	= true;
	
	//
	// Parse and store arguments
	//
	
	while (iArgC-- > 0) {
		Atom*	curAtom = iArgV++;
		
		if (curAtom->a_type == A_SYM) {
			char*		funcName = (iArgV++)->a_w.w_sym->s_name;
			tFunction	func = MapStrToFunction(funcName);
			
			if (func < 0)
				error("%s: doesn't understand %s", kClassName, funcName);
			else switch (++symArgC) {
				case 1:
					// First Function. Sets all functions.
					me->fnA = me->fnB = me->fnC = me->fnD = func;
					break;
				case 2:
					// Specify second function.
					me->fnB = func;
					break;
				case 3:
					// Specify third function.
					me->fnC = func;
					break;
				case 4:
					// Specify fourth function.
					me->fnD = func;
					break;
				default:
					// Any other value here is a user error
					error("%s: too many functions specified", kClassName);
				}
			
			}
		
		else {
			// Atom must be float or int; we only want floats
			ParseAtom(curAtom, false, true, 0, NIL, kClassName);	// This can't fail?
			switch (++floatArgC) {
				case 1:
					// First float sets all parameters, clears autoParam flag
					me->a = me->b = me->c = me-> d = curAtom->a_w.w_float;
					me->autoParam = false;
					break;
				case 2:
					// Second float sets paramter b. Also sets d.
					me->b = me->d = curAtom->a_w.w_float;
					break;
				case 3:
					// Third float sets paramter c
					me->c = curAtom->a_w.w_float;
					break;
				case 4:
					// Fourth float sets paramter d
					me->d = curAtom->a_w.w_float;
					break;
				case 5:
					// Fifth float sets initial x (and y), clears autoSeed flag
					me->x = me->x0 = me->y = me->y0 = curAtom->a_w.w_float;
					me->autoSeed = false;
					break;
				case 6:
					// Sixth float sets initial y
					me->y = me->y0 = curAtom->a_w.w_float;
					break;
				default:
					// Any other value here is a user error
					error("%s: too many numeric arguments", kClassName);
				}													// END switch
			}														// END if ... else
		}															// END while

	//
	// All done
	//
	
	return me;

punt:
	// Oops... cheesy exception handling
	error("Insufficient memory to create %s object.", kClassName);
	if (me) dsp_free(&(me->coreObject));
	
	return NIL;
	}


#pragma mark -
#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	DoBang(me)
 *
 ******************************************************************************************/

static void
DoBang(
	tfiana*	me)
	
	{
	
	if (me->autoParam) {
		me->a	= RandAC();
		me->b	= RandBD();
		me->c	= RandAC();
		me->d	= RandBD();
		}
	
	if (me->autoSeed) {
		me->x0	= RandXY();
		me->y0	= RandXY();
		}
	
	me->x = me->x0;
	me->y = me->y0;
	
	}


/******************************************************************************************
 *
 *	DoFloat(me, iVal)
 *	DoInt(me, iVal)
 *
 ******************************************************************************************/

static void
DoFloat(
	tfiana*	me,
	double	iVal)
	
	{
	
	switch ( ObjectGetInlet((Object*) me, me->coreObject.z_in) ) {
		case 4:		me->d = iVal;		break;
		case 3:		me->c = iVal;		break;
		case 2:		me->b = iVal;		break;
		case 1:		me->a = iVal;		break;
		
		default:
			me->d = me->c = me->b = me->a = iVal;
			break;
		}
	
	me->autoParam = false;
	
	}

static void DoInt(tfiana* me, long iVal)
			{ DoFloat(me, iVal); }
	
/******************************************************************************************
 *
 *	DoSeed(me, iSym, iArgC, iArgV)
 *	DoSet(me, iX0, iY0)
 *	DoReset(me)
 *
 ******************************************************************************************/

static void
DoSeed(
	tfiana*	me,
	Symbol*	,					// This is always "list"
	short	iArgC,
	Atom*	iArgV)
	
	{
	
	if (iArgC == 2
			&& ParseAtom(iArgV, false, true, 0, NIL, NIL)
			&& ParseAtom(iArgV+1, false, true, 0, NIL, NIL) ) {
		
		DoSet(me, iArgV[0].a_w.w_float, iArgV[1].a_w.w_float);
		
		}
	
	else error("%s: seed message requires two numeric values");
	
	}
	
static void
DoSet(
	tfiana*	me,
	double	iX0,
	double	iY0)
	
	{
	me->x = me->x0 = iX0;
	me->y = me->y0 = iY0;
	}

static void
DoReset(
	tfiana* me)
	
	{
	me->x = me->x0;
	me->y = me->y0;
	}	
	

/******************************************************************************************
 *
 *	Ident(me)
 *	Zero(me)
 *	One(me)
 *	Recip(me)
 *	SinPi(me)
 *	CosPi(me)
 *	TanPi(me)
 *	CotPi(me)
 *	Sinh(me)
 *	Cosh(me)
 *	Tanh(me)
 *	Log(me)
 *	Exp(me)
 *	Sqr(me)
 *	Root(me)
 *	
 ******************************************************************************************/

	static void SetFunc(tfiana* me, tFunction iFunc)
		{
		switch ( ObjectGetInlet((Object*) me, me->coreObject.z_in) ) {
			case 4:	me->fnD = iFunc; break;
			case 3:	me->fnC = iFunc; break;
			case 2:	me->fnB = iFunc; break;
			case 1:	me->fnA = iFunc; break;
			
			default:
				// Must be left inlet: set all functions
				me->fnA = me->fnB = me->fnC = me->fnD = iFunc;
				break;
			}
		}

static void	DoIdent(tfiana* me)		{ SetFunc(me, fnIdent); }
static void	DoZero(tfiana* me)		{ SetFunc(me, fnZero); }
static void	DoOne(tfiana* me)		{ SetFunc(me, fnOne); }
static void	DoRecip(tfiana* me)		{ SetFunc(me, fnRecip); }
static void	DoSin(tfiana* me)		{ SetFunc(me, fnSin); }
static void	DoCos(tfiana* me)		{ SetFunc(me, fnCos); }
static void	DoTan(tfiana* me)		{ SetFunc(me, fnTan); }
static void	DoCot(tfiana* me)		{ SetFunc(me, fnCot); }
static void	DoSinPi(tfiana* me)		{ SetFunc(me, fnSinPi); }
static void	DoCosPi(tfiana* me)		{ SetFunc(me, fnCosPi); }
static void	DoTanPi(tfiana* me)		{ SetFunc(me, fnTanPi); }
static void	DoCotPi(tfiana* me)		{ SetFunc(me, fnCotPi); }
static void	DoSinh(tfiana* me)		{ SetFunc(me, fnSinh); }
static void	DoCosh(tfiana* me)		{ SetFunc(me, fnCosh); }
static void	DoTanh(tfiana* me)		{ SetFunc(me, fnTanh); }
static void	DoLog(tfiana* me)		{ SetFunc(me, fnLog); }
static void	DoExp(tfiana* me)		{ SetFunc(me, fnExp); }
static void	DoSqr(tfiana* me)		{ SetFunc(me, fnSqr); }
static void DoRoot(tfiana* me)		{ SetFunc(me, fnRoot); }


/******************************************************************************************
 *
 *	DoTattle(me)
 *
 *	Post state information
 *
 ******************************************************************************************/

void
DoTattle(
	tfiana* me)
	
	{
	
	post("%s state", kClassName);
	post("  Current point [%lf, %lf] (Initial point: [%lf, %lf])",
			me->x, me->y, me->x0, me->y0);
	post("  Iteration formula:");
	post("    x' = %s(%lf y) + %lf %s(%lf x)",
			kFuncNames[me->fnA], me->a, me->b, kFuncNames[me->fnB], me->a);
	post("    y' = %s(%lf y) + %lf %s(%lf x)",
			kFuncNames[me->fnC], me->c, me->d, kFuncNames[me->fnD], me->c);
	
	}


/******************************************************************************************
 *
 *	DoAssist()
 *	DoInfo()
 *
 *	Generic Assist/Info methods
 *	
 *	Many parameters are not used.
 *
 ******************************************************************************************/

void DoAssist(tfiana*, void*, long iDir, long iArgNum, char* oCStr)
	{ assist_string(iDir, iArgNum, strIndexInLeft, strIndexOutLeft, oCStr); }

void DoInfo(tfiana* me)
	{ LitterInfo(kClassName, &me->coreObject.z_ob, (method) DoTattle); }




#pragma mark -
#pragma mark • DSP Methods

/******************************************************************************************
 *
 *	DoDSP(me, ioDSPVectors, iConnectCounts)
 *
 ******************************************************************************************/

static void
DoDSP(
	tfiana*		me,
	t_signal**	iDSPVectors,
	short*		iConnectCounts)
	
	{
	// Indices for MSP Inlets and Outlets.
	enum {
		inletBegin			= 0,
		inletA,
		inletB,
		inletC,
		inletD,
		
		outletX,
		outletY
		};

	if (iConnectCounts[outletX] == 0 && iConnectCounts[outletY] == 0)
		return;
	
	dsp_add(
		Performfiana, 4, me, 
		(long) iDSPVectors[outletX]->s_n,
		iDSPVectors[outletX]->s_vec,
		iDSPVectors[outletY]->s_vec
		);
		
	}
	

/******************************************************************************************
 *
 *	Performfiana(iParams)
 *
 *	Parameter block contains the following values values:
 *		- Address of the function
 *		- The performing fiana~ object
 *		- Vector size
 *		- output signal (X)
 *		- output signal (Y)
 *		- Address of the next link in the parameter chain
 *
 ******************************************************************************************/

	static double ident(double iVal)	{ return iVal; }
	static double zero(double)			{ return 0.0; }
	static double one(double)			{ return 1.0; }
	static double recip(double iVal)	{ return 1.0 / iVal; }
	static double cot(double iVal)		{ return 1.0 / tan(iVal); }
	static double sinpi(double iVal)	{ return sin(kPi * iVal); }
	static double cospi(double iVal)	{ return cos(kPi * iVal); }
	static double tanpi(double iVal)	{ return tan(kPi * iVal); }
	static double cotpi(double iVal)	{ return 1.0 / tan(kPi * iVal); }
	static double sqr(double iVal)		{ return iVal * iVal; }

	const tLatooFunc kFuncPtr[fnLast + 1] = {
									ident,	zero,	one,	recip,
									sin,	cos,	tan,	cot,
									sinpi,	cospi,	tanpi,	cotpi,
									sinh,	cosh,	tanh,
									log,	exp,	sqr,	sqrt };
	
static int*
Performfiana(
	int* iParams)
	
	{
	enum {
		paramFuncAddress	= 0,
		paramMe,
		paramVectorSize,
		paramOutX,
		paramOutY,
		
		paramNextLink
		};
	
	tfiana*			me = (tfiana*) iParams[paramMe];
	long			vecSize;
	tSampleVector	outX, outY;
	double			a, b, c, d, x, y;
	tLatooFunc		fnA, fnB, fnC, fnD;
	
	if (me->coreObject.z_disabled) goto exit;
	
	// Copy parameters into registers
	vecSize	= (long) iParams[paramVectorSize];
	outX	= (tSampleVector) iParams[paramOutX];
	outY	= (tSampleVector) iParams[paramOutY];
	
	a		= me->a;
	b		= me->b;
	c		= me->c;
	d		= me->d;
	x		= me->x;
	y		= me->y;
	fnA		= kFuncPtr[me->fnA];
	fnB		= kFuncPtr[me->fnB];
	fnC		= kFuncPtr[me->fnC];
	fnD		= kFuncPtr[me->fnD];
	
	do {
		double xx = fnA(a*y) + c * fnB(a*x);
		
		*outY++	= y = fnC(b*x) + d * fnD(b*y);
		*outX++ = x = xx;
		
		} while (--vecSize > 0);
	
	me->x	= x;
	me->y	= y;
		
exit:
	return iParams + paramNextLink;
	}


