/*
	File:		RNGGamma.c

	Contains:	<contents>

	Written by:	Peter Castine

	Copyright:	<copyright>

	Change History (most recent first):

         <1>   30–3–2006    pc      Initial check in.
*/


/*
	File:		RNGGamma.c

	Contains:	Gamma Distribution.

	Written by:	Peter Castine

	Copyright:	© 2001-06 Peter Castine

	Change History (most recent first):

*/


/******************************************************************************************
	Previous history:
	
	24-Mar-06:	Spun off from gammer.c so the code can be shared with other generators
	
 ******************************************************************************************/

#pragma mark • Include Files

#include "RNGGamma.h"
#include "RNGGauss.h"
#include "MoreMath.h"


#pragma mark • Constants


#pragma mark • Type Definitions


#pragma mark • Static Variables


#pragma mark • Global Variables



#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark • Functions

/******************************************************************************************
 *
 *	RecommendGammaAlg(iAlpha)
 *
 ******************************************************************************************/
 
eGammaAlg
RecommendGammaAlg(
	double iAlpha)
	
	{
	eGammaAlg	rec			= algUndef;
	double		intAlpha	= floor(iAlpha);
		
	if (iAlpha == intAlpha) {
			// Let's see if we can use one of the Erlang algorithms
		if		(iAlpha <= 13.0)				rec = algErlDir;
		else if	(iAlpha <= (double) kLongMax)	rec = algErlRej;
			// Otherwise it may be integer, but way too big for Erlang algorithms
		else									rec = algGD;
		}
	
	else {
		if		(iAlpha < 1.0)					rec = algGS;
		else									rec = algGD; 
		}
	
	return rec;
	}

 

/******************************************************************************************
 *
 *	GenErlangSmall(iAlpha, iFunc, iData)
 *	GerErlangSmallTaus88(iAlpha, iData)
 *
 *	Use direct method to calculate one Gamma-distributed deviate.
 *
 *	Some implementations embed the calculation inside a loop to check that the cumulative
 *	multiplication does not underflow and round to 0, which would generate an arithmetic
 *	exception on log(e). However, underflow will not possible with the 0 < x < 1 version
 *	of ULong2Unit while using double-precision floating point.
 *
 ******************************************************************************************/

double
GenErlangDir(
	UInt32		iAlpha,
	double		iBeta,
	tRandomFunc	iFunc,
	void*		iData)
	
	{
	double e = 1.0;
	
	do { e *= ULong2Unit_zo( iFunc(iData) ); }
		while (--iAlpha > 0);
	
	return -log(e) * iBeta;
	}

double
GerErlangDirTaus88(
	UInt32			iAlpha,
	double			iBeta,
	tTaus88Data*	iData)
	
	{
	double	e = 1.0;
	UInt32	s1, s2, s3;
	
	if (iData == NIL)
		iData = Taus88GetGlobals();
	Taus88Load(iData, &s1, &s2, &s3);
	
	do { e *= ULong2Unit_zo( Taus88Process(&s1, &s2, &s3) ); }
		while (--iAlpha > 0);
	
	Taus88Store(iData, s1, s2, s3);
	
	return -log(e) * iBeta;
	}

/******************************************************************************************
 *
 *	GenErlangRej(iAlpha, iFunc, iData)
 *	GenErlangRejTaus88(iAlpha, iData)
 *
 *	Use rejection method to calculate one Gamma-distributed deviate.
 *
 ******************************************************************************************/

double
GenErlangRej(
	UInt32		iAlpha,
	double		iBeta,
	double		iGamma,
	tRandomFunc	iFunc,
	void*		iData)
	
	{
	double	e,
			ratio,
			b,
			alpha1	= iAlpha - 1.0;
	
	do  {
		
		// Middle loop to protect against underflow
		do {
			// Get the ratio of a pair of uniformly distributed deviates inside
			//  the unit circle. Protect against division by 0.
			double num, denom;
			do {
				num		= Long2Signal(iFunc(iData));
				denom	= Long2Signal(iFunc(iData));
				} while ( (denom == 0.0) || (num * num + denom * denom > 1.0) );
			ratio = num / denom;
			// Calculate candidate unscaled Erlang value;
			e = iGamma * ratio + alpha1;
			} while (e <= 0.0);
		
		// Calculate magic test value.
		b =  1.0 + ratio * ratio;
		b *= exp(alpha1 * log(e/alpha1) - iGamma * ratio);
		} while (ULong2Unit_zo(iFunc(iData)) > b);
	
	return -log(e) * iBeta;
	}
	
double
GenErlangRejTaus88(
	UInt32			iAlpha,
	double			iBeta,
	double			iGamma,
	tTaus88Data*	iData)
	
	{
	double	e,
			thresh,
			alpha1	= iAlpha - 1.0;
	UInt32	s1, s2, s3;
	
	if (iData == NIL)
		iData = Taus88GetGlobals();
	Taus88Load(iData, &s1, &s2, &s3);
	
	do  {
		double ratio;
		
		// Middle loop to protect against underflow
		do {
			double num, denom;
			
			// Get the ratio of a pair of uniformly distributed deviates inside
			//  the unit circle. Protect against division by 0.
			do  {
				num		= Long2Signal( Taus88Process(&s1, &s2, &s3) );
				denom	= Long2Signal( Taus88Process(&s1, &s2, &s3) );
				} while ( (denom == 0.0) || (num * num + denom * denom > 1.0) );
			
			ratio = num / denom;
			// Calculate candidate unscaled Erlang value;
			e = iGamma * ratio + alpha1;
			} while (e <= 0.0);
		
		// Calculate magic test value.
		thresh  =  1.0 + ratio * ratio;
		thresh *= exp(alpha1 * log(e/alpha1) - iGamma * ratio);
		} while (ULong2Unit_zo( Taus88Process(&s1, &s2, &s3) ) > thresh);
	
	Taus88Store(iData, s1, s2, s3);
	
	return e * iBeta;
	}

	

/******************************************************************************************
 *
 *	GenGammaGS(iAlpha, iFunc, iData)
 *	GenGammaGSTaus88(iAlpha, iData)
 *
 *	Use GS method to calculate one Gamma-distributed deviate.
 *
 ******************************************************************************************/

double
GenGammaGS(
	double		iAlpha,
	double		iBeta,
	double		iGamma,
	tRandomFunc	iFunc,
	void*		iData)
	
	{
	double	gs,
			alpha1	= 1.0 / iAlpha;
	
	do  {
	    double	p = iGamma * ULong2Unit_zo( iFunc(iData)),
	    		x =  Unit2Exponential( ULong2Unit_zO(iFunc(iData)) );
	    
	    if (p < 1.0) {
			gs = exp(log(p) * alpha1);
			if (x >= gs) break;
			}
		else {
			gs = -log((iGamma - p) * alpha1);
			if (x >= (1.0 - iAlpha) * log(gs)) break;
			}
		
		} while (true);
	
	return gs * iBeta;
	}
	
double
GenGammaGSTaus88(
	double			iAlpha,
	double			iBeta,
	double			iGamma,
	tTaus88Data*	iData)
	
	{
	double	gs,
			alpha1	= 1.0 / iAlpha;
	UInt32	s1, s2, s3;
	
	if (iData == NIL)
		iData = Taus88GetGlobals();
	Taus88Load(iData, &s1, &s2, &s3);
	
	do  {
	    double	p = iGamma * ULong2Unit_zo( Taus88Process(&s1, &s2, &s3)),
	    		x =  Unit2Exponential( ULong2Unit_zO(Taus88Process(&s1, &s2, &s3)) );
	    
	    if (p < 1.0) {
			gs = exp(log(p) * alpha1);
			if (x >= gs) break;
			}
		else {
			gs = -log((iGamma - p) * alpha1);
			if (x >= (1.0 - iAlpha) * log(gs)) break;
			}
		
		} while (true);
	
	Taus88Store(iData, s1, s2, s3);
	
	return gs * iBeta;
	}
	
/******************************************************************************************
 *
 *	GenGammaGD(iAlpha, iFunc, iData)
 *	GenGammaGDTaus88(iAlpha, iData)
 *
 *	Use GS method to calculate one Gamma-distributed deviate.
 *
 ******************************************************************************************/

void
CalcGDParams(
	tGammaGDParams*	oParams,
	double			iAlpha,
	double			iBeta)
	
	{
	const double Q[7] = {
						2.424e-4,
						2.4511e-4,
						-7.388e-5,
						1.44121e-3,
						8.01191e-3,
						2.083148e-2,
						4.166669e-2
						};
	
	// Calculate additional dependent parameters
	double	s2	= iAlpha - 0.5,
			s	= sqrt(s2),
			r	= 1.0 / iAlpha;
	
	oParams->alpha	= iAlpha;
	oParams->beta	= iBeta;
	oParams->sigma2	= s2;
	oParams->sigma	= s;
	oParams->rho	= r;
	oParams->delta	= sqrt(32.0) - 12.0 * s;
	oParams->q0		= r * EvalPoly(r, Q, 6);
	
	// Approximation depending on size of the iOrder parameter.
	// The constants in the expressions for gamma, si, and c were established
	// by numerical experiments.
	if (iAlpha <= 3.686) {
		// case 1.0 <= alpha <= 3.686
	    oParams->gamma	= 0.463 + s + 0.178 * s2;
	    oParams->si		= 1.235;
	    oParams->c		= 0.195 / s - 7.9e-2 + 1.6e-1 * s;
		}
	else if (iAlpha <= 13.022) {
		// case  3.686 < alpha <= 13.022
	    oParams->gamma	= 1.654 + 7.6e-3 * s2;
	    oParams->si		= 1.68 / s + 0.275;
	    oParams->c		= 6.2e-2 / s + 2.4e-2;
		}
	else {
		// case alpha > 13.022
	    oParams->gamma	= 1.77;
	    oParams->si		= 0.75;
	    oParams->c		= 0.1515 / s;
		}
	
	}
	
	static inline double CalcMagicQ(double q0, double t, double s, double s2)
		{
		const double A[7] = {
							0.1233795,
							-0.1367177,
							0.1423657,
							-0.1662921,
							0.2000062,
							-0.250003,
							0.3333333
							};
		
		double v = t / (s + s);
		
		return (fabs(v) <= 0.25)
				? q0 + 0.5*t*t * EvalPoly(v, A, 6) * v
				: q0 - s * t + 0.25 * t * t + (s2 + s2) * log(1.0 + v);
		}

	static inline double CalcMagicW(double q)
		{
		const double E[5] = {
							1.0293e-2,
							4.07753e-2,
							0.166829,
							0.4999897,
							1.0
							};
							
		return (q <= 0.5) ? q * EvalPoly(q, E, 4) : exp(q) - 1.0;
		}

double
GenGammaGD(
	tGammaGDParams*	iParams,
	tRandomFunc		iFunc,
	void*			iData)
	
	{
	double	sigma	= iParams->sigma,
			t		= NormalKR(iFunc, iData),
			x		= sigma + 0.5 * t,
			gd		= x * x;
	
	// Accept result as is if (t >= 0.0), otherwise we need to tweak it...
	if (t < 0.0) {
		double	d	= iParams->delta,
				u	= ULong2Unit_zo(iFunc(iData));
		
		// Also accept if (d * u >= t^3)
		if (d * u > t * t * t) {
			const double tau1 = -0.71874483771719;
			
			double	q0		= iParams->q0,
					e, q, w;
			
			// Processing can possibly be simplified if x is positive
			if (x > 0.0) {
				double	alpha	= iParams->alpha,		// ?? Unused
						sigma2	= iParams->sigma2,
						r		= iParams->rho;			// ?? Unused
				
				if (log(1.0 - u) <= CalcMagicQ(q0, t, sigma, sigma2)) goto exit;	// We've got a winner
				}
			
			do  {
				// Generate an Laplace(si, b)-deviate and stuff it into t
				e = Unit2Exponential( ULong2Unit_zO(iFunc(iData)) );
				u = ULong2Unit_zo(iFunc(iData));
				u += u - 1.0;
				t = iParams->si * e;
				if (u < 0.0) {
					t = -t;
					u = -u;
					}
				t += iParams->gamma;
				// ASSERT: u > 0.0
				
				// Reject if t < tau(1)	
				if (t < tau1) continue;
				
				q = CalcMagicQ(q0, t, sigma, iParams->sigma2);
				if (q <= 0.0) continue;
				
				w = CalcMagicW(q);
				
				// Reject this t?
				// NB: We made sure that u was positive above!
				} while (iParams->c * u > w * exp(e - 0.5 * t * t));
			
			// We've got a winner. Finally
			x = sigma + 0.5 * t;
			gd = x * x;
			}
		
		}
	
exit:
	return gd * iParams->beta;
	}

double
GenGammaGDTaus88(
	tGammaGDParams*	iParams,
	tTaus88Data*	iData)
	
	{
	double	sigma	= iParams->sigma,
			t		= NormalKRTaus88(iData),
			x		= sigma + 0.5 * t,
			gd		= x * x;
	UInt32	s1, s2, s3;
	
	if (iData == NIL)
		iData = Taus88GetGlobals();
	Taus88Load(iData, &s1, &s2, &s3);
	
	// Accept result as is if (t >= 0.0), otherwise we need to tweak it...
	if (t < 0.0) {
		double	d	= iParams->delta,
				u	= ULong2Unit_zo( Taus88Process(&s1, &s2, &s3) );
		
		// Also accept if (d * u >= t^3)
		if (d * u > t * t * t) {
			const double tau1 = -0.71874483771719;
			
			double	q0		= iParams->q0,
					e, q, w;
			
			// Processing can possibly be simplified if x is positive
			if (x > 0.0) {
				double	alpha	= iParams->alpha,
						sigma2	= iParams->sigma2,
						r		= iParams->rho;
				
				if (log(1.0 - u) <= CalcMagicQ(q0, t, sigma, sigma2)) goto exit;	// We've got a winner
				}
			
			do  {
				// Generate an Laplace(si, b)-deviate and stuff it into t
				e = Unit2Exponential( ULong2Unit_zO( Taus88Process(&s1, &s2, &s3) ) );
				u = ULong2Unit_zo( Taus88Process(&s1, &s2, &s3) );
				u += u - 1.0;
				t = iParams->si * e;
				if (u < 0.0) {
					t = -t;
					u = -u;
					}
				t += iParams->gamma;
				// ASSERT: u > 0.0
				
				// Reject if t < tau(1)	
				if (t < tau1) continue;
				
				q = CalcMagicQ(q0, t, sigma, iParams->sigma2);
				if (q <= 0.0) continue;
				
				w = CalcMagicW(q);
				
				// Reject this t?
				// NB: We made sure that u was positive above!
				} while (iParams->c * u > w * exp(e - 0.5 * t * t));
			
			// We've got a winner. Finally
			x = sigma + 0.5 * t;
			gd = x * x;
			}
		
		}
	
exit:
	Taus88Store(iData, s1, s2, s3);
		
	return gd * iParams->beta;
	}
