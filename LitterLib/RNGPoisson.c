/*
	File:		RNGPoissons.c

	Contains:	Alogirthms for generating deviates from a Poisson distribution.

	Written by:	Peter Castine

	Copyright:	© 2001-2006 Peter Castine

	Change History (most recent first):

         <1>   23–3–2006    pc      first checked in.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "RNGPoisson.h"			// Also include MiscUtils.h

#ifdef WIN_VERSION
	#include "MoreMath.h"		// Need this for lgamma
#endif
//#include <math.h>				// For log(), cos(), etc.


#pragma mark • Constants
	


#pragma mark • Type Definitions


#pragma mark • Static (Private) Variables


#pragma mark • Initialize Global Variables


#pragma mark • Function Prototypes



#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/


/******************************************************************************************
 *
 *	RecommendPoisAlg(iLambda)
 *
 ******************************************************************************************/

ePoisAlg
RecommendPoisAlg(
	double iLambda)
	
	{
	
	if		(iLambda > 25.0)	return algReject;		// OR algLogistic?? ??
	else if	(iLambda > 5.0)		return algInversion;
	else if (iLambda > 0.0)		return algDirect;
	else if (iLambda == 0.0)	return algConstZero;
	else						return algUndef;		// This should never happen!
	
	}


/******************************************************************************************
 *
 *	GenPoissonDir		(iLambda, iRandFunc, iData)
 *	GenPoissonDirTaus88	(iThresh, iData)
 *
 ******************************************************************************************/

long
GenPoissonDir(
	double		iThresh,
	tRandomFunc	iFunc,
	void*		iData)
	
	{
	long	p	= -1;
	double	x	= 1.0;
	
	do {
		p += 1;
		x *= ULong2Unit_Zo( iFunc(iData) );
		} while (x > iThresh);
	
	return p;
	}
		
long
GenPoissonDirTaus88(
	double			iThresh,
	tTaus88Data*	iData)
	
	{
	long	p	= -1;
	double	x	= 1.0;
	UInt32	s1, s2, s3;
	
	if (iData == NIL)
		iData = Taus88GetGlobals();
	Taus88Load(iData, &s1, &s2, &s3);
	
	do {
		p += 1;
		x *= ULong2Unit_Zo( Taus88Process(&s1, &s2, &s3) );
		} while (x > iThresh);
	
	Taus88Store(iData, s1, s2, s3);
	
	return p;
	}
		
		
/******************************************************************************************
 *
 *	GenPoissonDir		(iParams, iRandFunc, iData)
 *	GenPoissonDirTaus88	(iParams, iData)
 *
 ******************************************************************************************/

long
GenPoissonInvTaus88(
	const tPoisInvParams*	iParams,
	tTaus88Data*			iData)
	
	{
	double	l = iParams->lambda,
			t = iParams->thresh,
			u = ULong2Unit_zo( Taus88(iData) ),
			p = 0.0;
	
	while (u > t) {
		u -= t;
		p += 1.0;
		t *= l / p;
		}
	
	return (long) p;
	}
	
/******************************************************************************************
 *
 *	CalcPoisRejParams(oParams, iLambda)
 *	GenPoissonDir(iParams, iRandFunc, iData)
 *	GenPoissonDirTaus88(iParams, iData)
 *
 ******************************************************************************************/

void
CalcPoisRejParams(
	tPoisRejParams* oParams,
	double			iLambda)
	
	{
	double	ll = log(iLambda);
	
	oParams->lambda			= iLambda;
	oParams->sqrt2Lambda	= sqrt(iLambda + iLambda);
	oParams->logLambda		= ll;
	oParams->lambdaMagic	= iLambda * ll - lgamma(iLambda + 1.0);
	}


long
GenPoissonRejTaus88(
	const tPoisRejParams*	iParams,
	tTaus88Data*			iData)
	
	{
	double	la = iParams->lambda,
			sq = iParams->sqrt2Lambda,
			ll = iParams->logLambda,
			lm = iParams->lambdaMagic,
			p,
			thresh;
	UInt32	s1, s2, s3;
	
	if (iData == NIL)
		iData = Taus88GetGlobals();
	Taus88Load(iData, &s1, &s2, &s3);
	
	do  {
		double y;
		
	    do  {
			y = tan(kPi * ULong2Unit_ZO(Taus88Process(&s1, &s2, &s3)));
			p = sq * y + la;
		    } while (p < 0.0);
		    
	    p = floor(p);
	    
	    thresh  = 0.9;
	    thresh *= y*y + 1.0;
	    thresh *= exp(p * ll - lgamma(p + 1.0) - lm);
	    
		} while (ULong2Unit_ZO( Taus88Process(&s1, &s2, &s3) ) > thresh);
	
	Taus88Store(iData, s1, s2, s3);
	
	return (long) p;
	}
		
/******************************************************************************************
 *
 *	CalcPoisRejParams(oParams, iLambda)
 *	GenPoissonDir(iParams, iRandFunc, iData)
 *	GenPoissonDirTaus88(iParams, iData)
 *
 *	Dagpunar's "Logistic" algorithm for large lambda
 *
 ******************************************************************************************/

/*

There's a bug somewhere in here causing constant values to be generated.
Since the rejection method above is working well, I've not bothered to find & fix it.
The ode is based on an algorithm published in Dagpunar's _Principles of Random Variate
Generation_, Clarendon (Oxford) 1988.

void
CalcPoisDLParams(
	tPoisDLParams*	oParams,
	double			iLambda)
	
	{
	double	beta	= kPi / sqrt(3.0 * iLambda),
			gamma	= exp(beta * (iLambda + 0.5)),
			delta,
			magic;
	
	if		(iLambda >= 1000.0)	magic = 1.276;
	else if (iLambda >= 200.0)	magic = 1.323;
	else if (iLambda >= 100.0)	magic = 1.36;
	else if (iLambda >= 50.0)	magic = 1.421;
	else if (iLambda >= 30.0)	magic = 1.484;
	else if (iLambda >= 20.0)	magic = 1.546;
	else if (iLambda >= 10.0)	magic = 1.7;
	else if (iLambda >= 5.0)	magic = 2.049;
	else if (iLambda >= 1.0)	magic = 4.854;
	else						magic = 17.48;
	
	delta  = -iLambda;
	delta += log((gamma + 1.0) / magic);
	delta += -1.51436227472;					// ln(1.5/(pi^3)) / 2
	
	oParams->lambda	= iLambda;
	oParams->kappa	= log(iLambda);
	oParams->beta	= beta;
	oParams->gamma	= gamma;
	oParams->delta1	= delta;
	oParams->delta2 = delta + log(2.0 * kPi * iLambda);
	}
	
long
GenPoissonDLTaus88(
	const tPoisDLParams*	iParams,
	tTaus88Data*			iData)
	
	{
	double	l	= iParams->lambda,
			k	= iParams->kappa,
			b	= iParams->beta,
			g	= iParams->gamma,
			d	= iParams->delta1,
			dd	= iParams->delta2,
			w, p;
	UInt32	s1, s2, s3;
	
	if (iData == NIL)
		iData = Taus88GetGlobals();
	Taus88Load(iData, &s1, &s2, &s3);
	
	do  {
		double	u1	= ULong2Unit_zo( Taus88Process(&s1, &s2, &s3) ),
				u2	= ULong2Unit_zo( Taus88Process(&s1, &s2, &s3) ),
				gu1	= g * u1 + 1.0,
				uu1	= 1.0 - u1;
		
		w = log(u2 * uu1 * gu1);
		p = floor(log(gu1 / uu1) / b);
		
		if (p > 0.0) {
			double z = w + (p + 0.5) * log(p / l) - p;
			
			if (z + 1.0/(12.0*p) <= d)
				break;							// We've got a winner
			
			if (z > d)							// This is a loser
				continue;
			
			w += lgamma(p + 1.0);
			w -= p * k;
			}
		
		} while (w > dd);
	
	return (long) p;
	}*/