/*
	File:		RNGChi2.c

	Contains:	<contents>

	Written by:	Peter Castine

	Copyright:	<copyright>

	Change History (most recent first):

         <1>   30–3–2006    pc      Initial check in.
*/


/*
	File:		RNGChi2.c

	Contains:	Chi-square and related distributions (Chi, inverse-chi square, scale inverse).

	Written by:	Peter Castine

	Copyright:	© 2001-06 Peter Castine

	Change History (most recent first):

*/


/******************************************************************************************
	Previous history:
	
	24-Mar-06:	Spun off from LitterLib.c, newer faster better algorithms added, now uses
				faster Normal generator when needed

 ******************************************************************************************/

#pragma mark • Include Files

#include "RNGChi2.h"
#include "RNGGauss.h"



#pragma mark • Constants


#pragma mark • Type Definitions


#pragma mark • Static Variables


#pragma mark • Global Variables



#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark • Functions


/******************************************************************************************
 *
 *	GenChi2Dir(iFreedom, iFunc, iData)
 *	GenChi2DirTaus88(iFreedom, iData)
 *
 ******************************************************************************************/

double
GenChi2Dir(
	UInt32		iFreedom,
	tRandomFunc	iFunc,
	void*		iData)
	
	{
	double	chi2;
	Boolean isOdd	= iFreedom & 0x01;
	
	iFreedom >>= 1;
	chi2	   = 1.0;
	while (iFreedom-- > 0) {
		chi2 *= ULong2Unit_zO( iFunc(iData) );
		}
	chi2 = -2.0 * log(chi2);
		
	if (isOdd) {
		double n = NormalKR(iFunc, iData);
		chi2 += n * n;
		}
	
	return chi2;
	}

double
GenChi2DirTaus88(
	UInt32			iFreedom,
	tTaus88Data*	iData)
	
	{
	double	chi2;
	Boolean isOdd	= iFreedom & 0x01;
	
	iFreedom >>= 1;
	
	if (iFreedom > 0) {
		UInt32	s1, s2, s3;
		
		if (iData == NIL)
			iData = Taus88GetGlobals();
		Taus88Load(iData, &s1, &s2, &s3);
		
		chi2 = 1.0;
		do	{ chi2 *= ULong2Unit_zO( Taus88Process(&s1, &s2, &s3) ); }
			while (--iFreedom > 0);
		chi2 = -2.0 * log(chi2);
		
		Taus88Store(iData, s1, s2, s3);
		}
	
	else chi2 = 0.0;
		
	if (isOdd) {
		double n = NormalKRTaus88(iData);
		chi2 += n * n;
		}
	
	return chi2;
	}
	
/******************************************************************************************
 *
 *	GenChi2Rej(iFreedom, iFunc, iData)
 *	GenChi2RejTaus88(iFreedom, iData)
 *
 *	Leverage off the implementation used to generate Erlang variates (the code is complex
 *	enough that the overhead for an additional function call is not worth worrying about).
 *
 ******************************************************************************************/

double
GenChi2Rej(
	UInt32		iFreedom,
	double		iGamma,
	tRandomFunc	iFunc,
	void*		iData)
	
	{
	double chi2 = GenErlangRej(iFreedom >> 1, 2.0, iGamma, iFunc, iData);
	
	if (iFreedom & 0x01) {
		double n = NormalKR(iFunc, iData);
		chi2 += n * n;
		}
		
	return chi2;
	}


double
GenChi2RejTaus88(
	UInt32			iFreedom,
	double			iGamma,
	tTaus88Data*	iData)
	
	{
	double chi2 = GenErlangRejTaus88(iFreedom >> 1, 2.0, iGamma, iData);
	
	if (iFreedom & 0x01) {
		double n = NormalKRTaus88(iData);
		chi2 += n * n;
		}
		
	return chi2;
	}

