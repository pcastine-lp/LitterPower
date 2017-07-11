/*
	File:		scampLib.c

	Contains:	Functions used by scampi/scampf/scamp~.

	Written by:	Peter Castine

	Copyright:	© 2003 Peter Castine

	Change History (most recent first):

        <11>    8–5–2006    pc      Fixed name conflict between the global enum Symmetry and the
                                    mapping enum Symmetry (now eMapSym)
        <10>      9–1–06    pc      Correct argument list for addmess("lin", ...). Update to use
                                    LitterAssistResFrag().
         <9>    13–12–04    pc      Update to us proxy_getinlet().
         <8>      8–9–04    pc      Investigate problem with calculating offset from input & output
                                    range in map mode.
         <7>     28–1–04    pc      Need to #include <string.h> now that we are abandoning the
                                    micro-stdlib.
         <6>     10–1–04    pc      Update for modified LitterInit()
         <5>      8–1–04    pc      Update for Windows.
         <4>    6–7–2003    pc      Need to set rangeInval flag when the output range changes.
         <3>    9–4–2003    pc      Renamed to scampLib.
         <2>   30–3–2003    pc      Sort out problems with scamp~
         <1>   15–3–2003    pc      Initial implementation and check in.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include <string.h>									// All for strncmp()

#include "scampLib.h"


#pragma mark • Global Variables


SymbolPtr	gSplitSym		= NIL,
			gStetSym		= NIL,
			gClipSym		= NIL,
			gReflectSym		= NIL,
			gWrapSym		= NIL,
			gLinSym			= NIL,
			gPowSym			= NIL,
			gExpSym			= NIL,
			gMapSym			= NIL,
			gSym0Sym		= NIL,
			gSym1Sym		= NIL,
			gSym2Sym		= NIL;

#if (SCAMP_TARGET == SCAMPI)
SymbolPtr	gTruncSym		= NIL,
			gRoundSym		= NIL,
			gFloorSym		= NIL,
			gCeilSym		= NIL,
			gToInfSym		= NIL;
#endif


#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark • Inline Functions

#pragma mark -
#pragma mark • Utilities

/******************************************************************************************
 *
 *	ScampGenSymbols()
 *	ScampAddMessages()
 *	ScampGetResInfo()
 *	
 *	Used in main() of individual objects
 *	
 ******************************************************************************************/

void
ScampGenSymbols()
	
	{
	gSplitSym	= gensym("split");
	gStetSym	= gensym("stet");
	gClipSym	= gensym("clip");
	gReflectSym	= gensym("reflect");
	gWrapSym	= gensym("wrap");
	gLinSym		= gensym("lin");
	gPowSym		= gensym("pow");
	gExpSym		= gensym("exp");
	gMapSym		= gensym("map");
	gSym0Sym	= gensym("sym0");
	gSym1Sym	= gensym("sym1");
	gSym2Sym	= gensym("sym2");
	}

void
ScampAddMessages()

	{
	
		// Core parameter messages
	addbang	((method) ScampBang);
	addint	((method) ScampInt);
	addfloat((method) ScampFloat);
	
		// Mapping modes
	addmess	((method) ScampMap,			"map",		A_GIMME, 0);	
	addmess	((method) ScampSym,			"sym",		A_DEFLONG, 0);
	addmess	((method) ScampLin,			"lin",		A_DEFFLOAT, 0);	
	addmess	((method) ScampPow,			"pow",		A_GIMME, 0);	
	addmess	((method) ScampExp,			"exp",		A_GIMME, 0);
	
		// Range correction messages
	addmess ((method) ScampClip,		"clip",		A_NOTHING);
	addmess ((method) ScampWrap,		"wrap",		A_NOTHING);
	addmess ((method) ScampReflect,		"reflect",	A_NOTHING);
	addmess ((method) ScampStet,		"stet",		A_NOTHING);
	
		// Informational methods
	addmess	((method) ScampTattle,		"tattle",	A_NOTHING);
	addmess ((method) ScampTattle,		"dblclick", A_CANT, 0);
	addmess	((method) ScampAssist,		"assist",	A_CANT, 0);
	addmess	((method) ScampInfo,		"info",		A_CANT, 0);
	
	}

/******************************************************************************************
 *
 *	ScampParseArgs()
 *	
 *	Used in ScampNew() of individual objects.
 *	
 *	Returns count of numeric arguments specified
 *	
 ******************************************************************************************/

void
ScampParseArgs(
	objScamp*	me,
	short		iArgC,
	Atom		iArgV[])
	
	{
	const short kUndefined	= -1;
	const int	kMaxNumArgs = 5;
	
	double	inVals[kMaxNumArgs];
	short	numArgsRead	= 0,
			action		= kUndefined,
			mappingType	= kUndefined,
			symmetry	= kUndefined;
	Boolean	map			= false;
	
	#if (SCAMP_TARGET == SCAMPI)
		short	roundMeth = kUndefined;
	#endif
	
	#if (SCAMP_TARGET != SCAMPSIG)
		SymbolPtr split = NIL;
	#endif
	
	
	while (iArgC-- > 0) {
		if (iArgV->a_type == A_SYM) {
			SymbolPtr mySym = iArgV->a_w.w_sym;
			
			// Map message?
			if (!map && mySym == gMapSym) {
				me->flags.map = map	= true;
				goto nextAtom;
				}
				
			// Range handling message?
			if (action == kUndefined) {
				if		(mySym == gClipSym)		action	= actClip;
				else if (mySym == gWrapSym)		action	= actWrap;
				else if (mySym == gReflectSym)	action	= actReflect;
				else if (mySym == gStetSym)		action	= actStet;
				
				// Until C supports 'continue'
				if (action != kUndefined) {
					me->action = action;
					goto nextAtom;
					}
				}
			
			// Scale/Map type message?
			if (mappingType == kUndefined) {
				if		(mySym == gPowSym)		mappingType = mapPow;
				else if (mySym == gExpSym)		mappingType = mapExp;
				else if (mySym == gLinSym)		mappingType	= mapLin;
				
				// Until C supports 'continue'
				if (mappingType != kUndefined) {
					me->mappingType = mappingType;
					goto nextAtom;
					}			
				}
			
			// Symmetry options?
			if (symmetry == kUndefined) {
				if		(mySym == gSym0Sym)		symmetry	= symNone;
				else if	(mySym == gSym1Sym)		symmetry	= symPoint;
				else if (mySym == gSym2Sym)		symmetry	= symAxis;
				
				// Until C supports 'continue'
				if (symmetry != kUndefined) {
					me->symmetry = symmetry;
					goto nextAtom;	
					}		
				}
			
	#if	(SCAMP_TARGET == SCAMPI)
			// Float-to-integer conversion message?
			if (mappingType == kUndefined) {
				if		(mySym == gTruncSym)	roundMeth	= roundToZero;
				else if (mySym == gRoundSym)	roundMeth	= roundRound;
				else if (mySym == gFloorSym)	roundMeth	= roundFloor;
				else if (mySym == gCeilSym)		roundMeth	= roundCeil;
				else if (mySym == gToInfSym)	roundMeth	= roundToInf;
				
				// Until C supports 'continue'
				if (roundMeth != kUndefined) {
					me->rounding = roundMeth;
					goto nextAtom;
					}			
				}
	#endif // SCAMP_TARGET == SCAMPI
			
	#if	(SCAMP_TARGET != SCAMPSIG)
			// Split message?
			if (split == NIL) {
				const int kSplitStrLen = 5;		// strlen("split")
				
				char* msg = mySym->s_name;
				
				if (strncmp(msg, "split", kSplitStrLen) == 0) {
					msg += kSplitStrLen - 1;
					while (*++msg == ' ')
						{ /* empty loop, side effect of the condition does the work */ }
					me->split = (*msg == '\0') ? kSplitValues : gensym(msg);
					
					// Until C supports 'continue'
					goto nextAtom;
					}
				}
	#endif // SCAMP_TARGET != SCAMPSIG
			
			// Dang, either we don't recognize this message, or the message type
			// has already been set.
			error("%s ignoring spurious symbol ‘%s’", kClassName, mySym->s_name);
			}
		
		else {
			// ASSERT: AtomIsNumeric(iArgV)
			// So we have a numeric value.
			// But can we afford another one?
			if (numArgsRead < kMaxNumArgs)
				inVals[numArgsRead++] = AtomGetFloat(iArgV);
			else {
				error("%s ignoring spurious argument", kClassName);
				postatom(iArgV);
				}
			}
		
	nextAtom:
		iArgV += 1;
		}
	
	// Finally, handle numeric arguments
	switch (numArgsRead) {
		// All cases fall through to next!
		case 5:	ScampCurve(me, inVals[4]);
		case 4: ScampMaxOut(me, inVals[3]);
		case 3: ScampMinOut(me, inVals[2]);
		case 2: ScampFloat2(me, inVals[1]);
		case 1: ScampFloat1(me, inVals[0]);
		default: break;
		}
	
	}


/******************************************************************************************
 *
 *	ScampCalcRange(me)
 *	ScampCalc(me)
 *
 ******************************************************************************************/

void
ScampCalcRange(
	objScamp* me)
	
	{
	
	if (me->flags.map) {
		double	slope	= (me->outMax - me->outMin) / (me->inMax - me->inMin),
//				offset	= slope * me->inMin + me->outMin;
				offset	= me->outMin - slope * me->inMin;
		
		me->slope	= slope;
		me->offset	= offset;
		}
	
	else {
		double	slopeInv	= 1.0 / me->slope,
				inRange		= (me->outMax - me->outMin) * slopeInv,
//				inMin		= (me->offset - me->outMin)  * slopeInv;
				inMin		= (me->outMin - me->offset)  * slopeInv;
		
		me->inMin = inMin;
		me->inMax = inMin + inRange;
		}
	
	me->flags.rangeInval = false;
	
	}

/******************************************************************************************
 *
 *	ScampInputFloat(me, iVal)
 *	ScampInputInt(me, iVal)
 *
 ******************************************************************************************/

void ScampFloat(objScamp* me, double iVal)
	{
#if (SCAMP_TARGET == SCAMPSIG)	
	switch ( ObjectGetInlet((Object*) me, me->coreObject.z_in) ) {
		default:	me->input = iVal; break;
		case 1:		ScampFloat1(me, iVal); break;
		case 2:		ScampFloat2(me, iVal); break;
		case 3:		ScampMinOut(me, iVal); break;
		case 4:		ScampMaxOut(me, iVal); break;
		case 5:		ScampCurve(me, iVal); break;
		}
#else
	me->input = iVal;
	ScampBang(me);
#endif
	}

void ScampInt(objScamp*	me, long iVal)
	{ ScampFloat(me, (double) iVal); }


/******************************************************************************************
 *
 *	ScampFloat1(me, iVal)
 *	ScampFloat2(me, iVal)
 *	ScampMinOut(me, iVal)
 *	ScampMaxOut(me, iVal)
 *	ScampCurve(me, iCurve);
 *
 ******************************************************************************************/

void ScampFloat1(objScamp* me, double iVal)
	{
	if (me->flags.map)
		 me->inMin = iVal;
	else {
		me->slope = iVal;
		me->flags.rangeInval = true;
		}
	}
	
void ScampFloat2(objScamp* me, double iVal)
	{
	if (me->flags.map)
		 me->inMax = iVal;
	else {
		me->offset = iVal;
		me->flags.rangeInval = true;
		}
	}

void ScampMinOut(objScamp* me, double iVal)
	{ me->outMin = iVal; me->flags.rangeInval = true; }
void ScampMaxOut(objScamp* me, double iVal)
	{ me->outMax = iVal; me->flags.rangeInval = true; }

void ScampCurve(objScamp* me, double iCurve)
	{
	const double kPhi = 1.6180339887;
	
	if (iCurve == 0.0)
		me->flags.degenerate = true;
	else {
		me->flags.degenerate = false;
		if	(iCurve > 0.0)
			 iCurve = sqrt(iCurve);
		else iCurve = -sqrt(-iCurve);
		}
	
	switch (me->mappingType) {
		default:		me->curve = pow(kPhi, iCurve); break;		// mapLin
		case mapPow:	me->curve = pow(2.0, iCurve); break;
		case mapExp:	me->curve = exp(iCurve); break;
		}
	
	}


#pragma mark -
#pragma mark • Mode Messages

/******************************************************************************************
 *
 *	ScampMap(me, iSym, iArgC, iArgV);
 *	ScampSym(me, iSymType);
 *
 ******************************************************************************************/

void ScampMap(objScamp* me, Symbol* sym, short iArgC, Atom iArgV[])
	{
	#pragma unused(sym)
	
	me->flags.map = (iArgC == 0)
						? true:
						(AtomGetLong(iArgV) != 0);
	}

void ScampSym(objScamp* me, long iSymType)
	{
	if (iSymType < symNone)			iSymType = symNone;
	else if (iSymType > symAxis)	iSymType = symAxis;
	
	me->symmetry = (eMapSym) iSymType;
	}

/******************************************************************************************
 *
 *	ScampLin(me, iBreak);
 *	ScampPow(me, iSym, iArgC, iArgV);
 *	ScampExp(me, iSym, iArgC, iArgV);
 *
 ******************************************************************************************/

	// Utility providing a warning in the Max window that looks just like what the
	// Max argument parser says.
	// Should move this to MaxUtils.
	static void WarnXtraArgs(const char iClassName[], Symbol* iMsg)
	{ post("warning: %s: extra arguments for message \"%s\"", iClassName, iMsg->s_name); }

void ScampLin(objScamp* me, double iBreak)
	{ me->mappingType = mapLin; ScampCurve(me, iBreak); }

void
ScampPow(
	objScamp*	me,
	Symbol*		iMsg,						// must be pow
	short		iArgC,
	Atom		iArgV[])
	
	{
	double curve = 1.0;
	
	if (iArgC > 0 && AtomIsNumeric(iArgV)) {
		curve = AtomGetFloat(iArgV);
		iArgC -= 1;
		}
					
	me->mappingType = mapPow;
	ScampCurve(me, curve);
	
	if (iArgC > 0) WarnXtraArgs(kClassName, iMsg);
	}
	
void
ScampExp(
	objScamp*	me,
	Symbol*		iMsg,						// must be exp
	short		iArgC,
	Atom		iArgV[])

	{
	double curve = 1.0;
	
	if (iArgC > 0 && AtomIsNumeric(iArgV)) {
		curve = AtomGetFloat(iArgV);
		iArgC -= 1;
		}
					
	me->mappingType = mapExp;
	ScampCurve(me, curve);
	
	if (iArgC > 0) WarnXtraArgs(kClassName, iMsg);
	}
	

/******************************************************************************************
 *
 *	ScampClip(me);
 *	ScampWrap(me);
 *	ScampReflect(me);
 *	ScampStet(me);
 *
 ******************************************************************************************/

void ScampClip(objScamp* me)		{ me->action = actClip; }
void ScampWrap(objScamp* me)		{ me->action = actWrap; }
void ScampReflect(objScamp* me)		{ me->action = actReflect; }
void ScampStet(objScamp* me)		{ me->action = actStet; }


/******************************************************************************************
 *
 *	ScampInfo(me)
 *
 ******************************************************************************************/

void ScampInfo(objScamp* me)
	{ LitterInfo(kClassName, (tObjectPtr) me, (method) ScampTattle); }

