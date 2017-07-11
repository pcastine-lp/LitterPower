/*
	File:		latoocarfian.c

	Contains:	Max external object generating sequences of populations from Clifford
				Pickover's eponymous chaotic function. Extended (à la FRACTINT) to support
				variants using functions other than sine.

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
		11-Apr-2002:	First implementation.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Identify Target


#pragma mark • Include Files

#include "LitterLib.h"

#pragma mark • Constants

const char*	kClassName		= "lp.lana";			// Class name
const char*	kPackageName	= "lp.lana Package";	// Package name for 'vers'(2) resource


const int	kProxyCount		= 4;


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


#pragma mark • Object Structure

typedef struct {
	Object			coreObject;
	voidPtr			proxies[kProxyCount];
	long			curProxy;
	voidPtr			outY;						// outX accessed through coreObject.o_outlet
	
	double			x0,	y0,						// Initial coordinates
					x,	y,						// Current coordinates
					a, b, c, d;					// Parameters

	tFunction		fnA, fnB, fnC, fnD;			// Functions
	Boolean			autoParam,
					autoSeed;
	} tLana;


#pragma mark • Global Variables

t_messlist*	gLanaClass		= NIL;
short		gStrResID		= 0;
long		gModDate		= 0;


#pragma mark • Function Prototypes

	// Class message functions
void*		NewLana(Symbol*, short, Atom*);
void		FreeLana(tLana*);

	// Object message functions
static void DoBang(tLana*);
static void DoFloat(tLana*, double);
static void DoInt(tLana*, long);						// Because Max no longer does
														// this automatically :-(
static void DoSeed(tLana*, Symbol*, short, Atom*);		// List message is specifically
														// new initial coordinate
static void DoSet(tLana*, double, double);
static void DoReset(tLana*);

	// Set functions (inlet 0 ==> global, else a specific slot
static void	DoIdent(tLana*);
static void	DoZero(tLana*);
static void	DoOne(tLana*);
static void	DoRecip(tLana*);
static void	DoSin(tLana*);
static void	DoCos(tLana*);
static void	DoTan(tLana*);
static void	DoCot(tLana*);
static void	DoSinPi(tLana*);
static void	DoCosPi(tLana*);
static void	DoTanPi(tLana*);
static void	DoCotPi(tLana*);
static void	DoSinh(tLana*);
static void	DoCosh(tLana*);
static void	DoTanh(tLana*);
static void	DoLog(tLana*);
static void	DoExp(tLana*);
static void	DoSqr(tLana*);
static void DoRoot(tLana*);

static void DoTattle(tLana*);
static void	DoAssist(tLana*, void* , long , long , char*);
static void	DoInfo(tLana*);


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
	setup(	&gLanaClass,				// Pointer to our class definition
			(method) NewLana,			// Instance creation function
			(method) FreeLana,			// Custom deallocation function
			sizeof(tLana),				// Class object size
			NIL,						// No menu function
			A_GIMME,					// Arguments (all optional): 0 to 4 symbols,
			0);							//	followed 0 to 6 floats (Why do I do this?)
		
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
	
	// Initialize Litter Library
	// Lana will generate random starting values if the user doesn't say different
	LitterInit(kClassName, 0);
	Taus88Init();
	
	}

#pragma mark -
#pragma mark • Utility Functions

/******************************************************************************************
 *
 *	MapStrToFunction(iStr)
 *	
 ******************************************************************************************/

static short
MapStrToFunction(
	const char* iStr)
	{
	short	result;
	
	for (result = fnLast; result >= 0; result -= 1) {
		if (strcmp(iStr, kFuncNames[result]) == 0) break;
		} 
	
	return result;
	}


#pragma mark -
#pragma mark • Class Message Handlers

/******************************************************************************************
 *
 *	NewLana(iName, iArgC, iArgV)
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
NewLana(
	Symbol*	,
	short	iArgC,
	Atom*	iArgV)
	
	{
	const tFunction	kDefFunc	= fnSin;	// One size fits all
	
	tLana*		me			= NIL;
	int			symArgC		= 0,
				floatArgC	= 0;

	
	//
	// Let Max allocate us, our inlets, and outlets
	//
	me = (tLana*) newobject(gLanaClass);
		if (me == NIL) goto punt;
	
		// Extra inlets: all proxies
	{
	int i = kProxyCount;		// If the compiler's smart enough, it will 'borrow' a register
	while (i-- > 0) {			// Autodecrement here is standard C programmer's dirty trick
								// Conveniently, this defines proxies right-to-left.
		me->proxies[i] = proxy_new(me, i+1, &me->curProxy);
		if (me->proxies[i] == NIL) goto punt;
		}
	}
		
	
		// Two outlets, right to left
	me->outY = floatout(me);
	floatout(me);			// Accessed through me->coreObject.o_outlet
	
	//
	// Make sure pointers and counters are set to valid initial states
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
	me->autoParam	= true,
	me->autoSeed	= true;
	
	//
	// Parse and store arguments
	//
	
	while (iArgC-- > 0) {
		Atom*	curAtom = iArgV++;
		
		if (curAtom->a_type == A_SYM) {
			char*	funcName = (iArgV++)->a_w.w_sym->s_name;
			int		funcNum = MapStrToFunction(funcName);
			
			if (funcNum < 0)
				error("%s: doesn't understand %s", kClassName, funcName);
			else switch (++symArgC) {
				case 1:
					// First Function. Sets all functions.
					me->fnA = me->fnB = me->fnC = me->fnD = (tFunction) funcNum;
					break;
				case 2:
					// Specify second function. Also sets fourth function.
					me->fnB = me->fnD = (tFunction) funcNum;
					break;
				case 3:
					// Specify third function.
					me->fnC = (tFunction) funcNum;
					break;
				case 4:
					// Specify fourth function.
					me->fnD = (tFunction) funcNum;
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
			
			me->autoSeed = false;
			}														// END if ... else
		}															// END while

	//
	// All done
	//
	
	return me;

punt:
	// Oops... cheesy exception handling
	error("Insufficient memory to create %s object.", kClassName);
	if (me) FreeLana(me);
	
	return NIL;
	}

/******************************************************************************************
 *
 *	FreeLana(me)
 *	
 ******************************************************************************************/

static void
FreeLana(
	tLana* me)
	
	{
	int		i;
	voidPtr	p;
	
	if (me == NIL)		// Sanity check
		return;		
	
	i = kProxyCount;
	while (i-- > 0)
		if ( (p = me->proxies[i]) != NIL ) freeobject(p);
		
	}
	

#pragma mark -
#pragma mark • Object Message Handlers

/******************************************************************************************
 *
 *	DoBang(me)
 *	DoOut(me)
 *	DoNext(me)
 *
 ******************************************************************************************/

	static double CalcFunc(tFunction iFunc, double iVal)
		{
		
		switch (iFunc) {
			case fnZero:	return 0.0;						break;
			case fnOne:		return 1.0;						break;
			case fnRecip:	return 1.0 / iVal;				break;
			case fnSin:		return sin(iVal);				break;
			case fnCos:		return cos( iVal);				break;
			case fnTan:		return tan(iVal);				break;
			case fnCot:		return 1 / tan(iVal);			break;
			case fnSinPi:	return sin(kPi * iVal);			break;
			case fnCosPi:	return cos(kPi * iVal);			break;
			case fnTanPi:	return tan(kPi * iVal);			break;
			case fnCotPi:	return 1 / tan(kPi * iVal);		break;
			case fnSinh:	return sinh(iVal);				break;
			case fnCosh:	return cosh(iVal);				break;
			case fnTanh:	return tanh(iVal);				break;
			case fnLog:		return log(iVal);				break;
			case fnExp:		return exp(iVal);				break;
			case fnSqr:		return iVal * iVal;				break;
			case fnRoot:	return sqrt(iVal);				break;
				// Only thing left is fnIdent
			default:		return iVal;					break;
			}
		}

static void
DoNext(
	tLana* me)
	
	{
	// Values we need more than once go into registers
	double	x	= me->x,
			y	= me->y,
			a	= me->a,
			b	= me->b;
	
	me->x = CalcFunc(me->fnA, a*y) + me->c * CalcFunc(me->fnC, a*x);
	me->y = CalcFunc(me->fnB, b*x) + me->d * CalcFunc(me->fnD, b*y);
	}
	
static void DoOut(tLana* me)
	{
	// Right-to-left
	outlet_float(me->outY, me->y);
	outlet_float(me->coreObject.o_outlet, me->x);
	}

static void
DoBang(
	tLana* me)
	
	{
	const long kInlet = ObjectGetInlet((Object*) me, me->coreObject.z_in);
	
	// This may seem anal-retentive, but I think allowing bangs in any inlet is
	// confusing
	if (kInlet != 0)
		error("%s doesn't understand bang in inlet %ld", kClassName, kInlet);
	
	else {
		DoNext(me);
		DoOut(me);
		}
		
	}

/******************************************************************************************
 *
 *	DoFloat(me, iVal)
 *	DoInt(me, iVal)
 *
 ******************************************************************************************/

static void
DoFloat(
	tLana*	me,
	double	iVal)
	
	{
	
	switch ( ObjectGetInlet((Object*) me, me->coreObject.z_in) ) {
		case 4:		me->d = iVal;		break;
		case 3:		me->c = iVal;		break;
		case 2:		me->b = iVal;		break;
		case 1:		me->a = iVal;		break;
		
		default:
			// Must be left inlet
			me->d = me->c = me->b = me->a = iVal;
			break;
		}
	
	me->autoParam = false;
	
	}

static void DoInt(tLana* me, long iVal)
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
	tLana*	me,
	Symbol*	,							// This is always "list"
	short	iArgC,
	Atom*	iArgV)
	
	{
	
	if (iArgC == 2
			&& ParseAtom(iArgV, false, true, 0, NIL, NIL)
			&& ParseAtom(iArgV+1, false, true, 0, NIL, NIL) ) {
		
		DoSet(me, iArgV[0].a_w.w_float, iArgV[1].a_w.w_float);
		DoBang(me);
		
		}
	
	else error("%s: lists must contain exactly two numeric values", kClassName);
	
	}
	
static void
DoSet(
	tLana*	me,
	double	iX0,
	double	iY0)
	
	{
	me->x = me->x0 = iX0;
	me->y = me->y0 = iY0;
	me->autoSeed = false;
	}

static void
DoReset(
	tLana* me)
	
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

	static void SetFunc(tLana* me, tFunction iFunc)
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

static void	DoIdent(tLana* me)		{ SetFunc(me, fnIdent); }
static void	DoZero(tLana* me)		{ SetFunc(me, fnZero); }
static void	DoOne(tLana* me)		{ SetFunc(me, fnOne); }
static void	DoRecip(tLana* me)		{ SetFunc(me, fnRecip); }
static void	DoSin(tLana* me)		{ SetFunc(me, fnSin); }
static void	DoCos(tLana* me)		{ SetFunc(me, fnCos); }
static void	DoTan(tLana* me)		{ SetFunc(me, fnTan); }
static void	DoCot(tLana* me)		{ SetFunc(me, fnCot); }
static void	DoSinPi(tLana* me)		{ SetFunc(me, fnSinPi); }
static void	DoCosPi(tLana* me)		{ SetFunc(me, fnCosPi); }
static void	DoTanPi(tLana* me)		{ SetFunc(me, fnTanPi); }
static void	DoCotPi(tLana* me)		{ SetFunc(me, fnCotPi); }
static void	DoSinh(tLana* me)		{ SetFunc(me, fnSinh); }
static void	DoCosh(tLana* me)		{ SetFunc(me, fnCosh); }
static void	DoTanh(tLana* me)		{ SetFunc(me, fnTanh); }
static void	DoLog(tLana* me)		{ SetFunc(me, fnLog); }
static void	DoExp(tLana* me)		{ SetFunc(me, fnExp); }
static void	DoSqr(tLana* me)		{ SetFunc(me, fnSqr); }
static void DoRoot(tLana* me)		{ SetFunc(me, fnRoot); }


/******************************************************************************************
 *
 *	DoTattle(me)
 *
 *	Post state information
 *
 ******************************************************************************************/

void
DoTattle(
	tLana* me)
	
	{
	
	post("%s state", kClassName);
	post("  Current point [%lf, %lf] (Initial point: [%lf, %lf])",
			me->x, me->y, me->x0, me->y0);
	post("  Iteration formula:");
	post("    x' = %s(%lf y) + %lf %s(%lf x)",
			kFuncNames[me->fnA], me->a, me->c, kFuncNames[me->fnC], me->a);
	post("    y' = %s(%lf y) + %lf %s(%lf x)",
			kFuncNames[me->fnB], me->b, me->d, kFuncNames[me->fnD], me->b);
	
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

void DoAssist(tLana*, void*, long iDir, long iArgNum, char* oCStr)
	{ LitterInfo(iDir, iArgNum, strIndexInLeft, strIndexOutLeft, oCStr); }

void DoInfo(tLana* me)
	{ LitterInfo(kClassName, &me->coreObject, (method) DoTattle); }


