/*
	File:		RNGDistBeta.c

	Contains:	Generate Random variates from a beta distribution (including the special
				case of an arcsine distribution).

	Written by:	Peter Castine

	Copyright:	© 2001-2006 Peter Castine

	Change History (most recent first):

         <1>     16Ð3Ð06    pc      first checked in.
*/


/******************************************************************************************

	References:
	Cheng, R. C. H., Generating Beta Variates with Non-integral Shape Parameters, Comm. ACM,
	21, 317-322 (1978). (Algorithms BB and BC).
	
	Jšhnk, M.D., Erzeugung von Betaverteilten und Gammaverteilten Zufallszahlen," Metrika,
	vol 8 pp.5-15, 1964.
	 
 ******************************************************************************************/

#pragma mark ¥ Include Files

#include "RNGDistBeta.h"				// Also include MiscUtils.h

//#include <math.h>				// For log(), cos(), etc.


#pragma mark ¥ Constants
	

#pragma mark ¥ Type Definitions


#pragma mark ¥ Static (Private) Variables


#pragma mark ¥ Initialize Global Variables


#pragma mark ¥ Function Prototypes



#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/
#pragma mark ¥ Functions

/******************************************************************************************
 *
 *	RecommendBetaAlg(a, b)
 *	
 ******************************************************************************************/

eBetaAlg
RecommendBetaAlg(
			double a,
			double b)
	
	{
	eBetaAlg result;
	
	if (a == 0.0)
		result = (b == 0.0) ? algIndeterm : algConstZero;
	
	else if (b == 0.0)
		result = algConstOne;
	
	else if (a == 0.5 && b == 0.5)
		result = algArcSine;
	
	else if (a == 1.0 && b == 1.0)
		result = algUniform;
	
	else if (a <= 0.5 && b <= 0.5)
		result = algJoehnk;
	
	else if (a > 1.0 && b > 1.0)
		result = algChengBB;
	
	else result = algChengBC;
	
	return result;
	}


/******************************************************************************************
 *
 *	CalcJKParams(oParams, a, b)
 *	GenBetaJK(iParams, iFunc, iData);
 *	GenBetaJKTaus88(iParams, iData);
 *	
 ******************************************************************************************/

void
CalcJKParams(
	tJKParams*	oParams,
	double		a,
	double		b)
	
	{
	oParams->a1		= 1.0 / a;
	oParams->b1		= 1.0/  b;
	}

double
GenBetaJK(
	const tJKParams*	iParams,
	tRandomFunc			iFunc,
	void*				iData)
	
	{
	double	a1	= iParams->a1,
			b1	= iParams->b1,
			u1, u2;
	
	do  {
	    u1 = ULong2Unit_zO( iFunc(iData) ),
	    u2 = ULong2Unit_zO( iFunc(iData) );
		// The following is a slightly compressed implementation of the more explicit:
		//		y1 = u1 ^ 1/a
		//		y2 = u2 ^ 1/b;
		//		s = y1 + y2;
		u1 = pow(u1, a1);
		u2 = pow(u2, b1);
		u2 += u1;				// stash sum in u2 instead of separate register
		} while (u2 > 1.0);
	
    return u1 / u2;   
	}

double
GenBetaJKTaus88(
	const tJKParams*	iParams,
	tTaus88Data*		iData)
	
	{
	double	a1	= iParams->a1,
			b1	= iParams->b1,
			u1, u2;
	UInt32	s1, s2, s3;
	
	if (iData == NIL)
		iData = Taus88GetGlobals();
	Taus88Load(iData, &s1, &s2, &s3);
	
	do  {
	    u1 = ULong2Unit_zO( Taus88Process(&s1, &s2, &s3) ),
	    u2 = ULong2Unit_zO( Taus88Process(&s1, &s2, &s3) );
		// The following is a slightly compressed implementation of the more explicit:
		//		y1 = u1 ^ 1/a
		//		y2 = u2 ^ 1/b;
		//		s = y1 + y2;
		u1 = pow(u1, a1);
		u2 = pow(u2, b1);
		u2 += u1;				// stash sum in u2 instead of separate register
		} while (u2 > 1.0);
	
	Taus88Store(iData, s1, s2, s3);
	
    return u1 / u2;   
	}



/******************************************************************************************
 *
 *	CalcBBParams(oParams, a, b)
 *	GenBetaBB(iParams, iFunc, iData);
 *	GenBetaBBTaus88(iParams, iData);
 *	
 ******************************************************************************************/

void
CalcBBParams(
	tBBParams*	oParams,
	double		a,
	double		b)
	
	{
	double	alpha,
			beta,
			gamma;
	
	oParams->mirror = (a > b);
	if (oParams->mirror) {
		// swap
		alpha = a; a = b; b = alpha;
		}
	
	alpha	= a + b;
	
	beta	 = a * b;
	beta	+= beta;
	beta	-= alpha;
	beta	 = sqrt((alpha - 2.0) / beta);
	
	gamma	= a + 1.0 / beta;
	
	oParams->a		= a;
	oParams->b		= b;
	oParams->alpha	= alpha;
	oParams->beta	= beta;
	oParams->gamma	= gamma;
	}

double
GenBetaBB(
	const tBBParams*	iParams,
	tRandomFunc			iFunc,
	void*				iData)
	
	{
	const double	a		= iParams->a,
					b		= iParams->b,
					alpha	= iParams->alpha,
					beta	= iParams->beta,
					gamma	= iParams->gamma;
					
	double r, s, t, v, w, z;
	
	do  {
	    double	u1 = ULong2Unit_zo( iFunc(iData) ),
	    		u2 = ULong2Unit_zo( iFunc(iData) );
		
		v  = beta * log(u1 / (1.0-u1)),
	    w  = a * exp(v);						// ?? Can this overflow ??
		z  = u1 * u1 * u2,
		r  = gamma * v - 1.3862944;
		s = a + r - w;
	    		
	    if (s + 2.609438 >= 5.0 * z) break;
	    t = log(z);
	    if (s > t) break;
		} while (r + alpha * log(alpha/(b+w)) < t);
	
    return iParams->mirror ? b / (b+w) : w / (b+w);   
	}

double
GenBetaBBTaus88(
	const tBBParams*	iParams,
	tTaus88Data*		iData)
	
	{
	double	a		= iParams->a,
			b		= iParams->b,
			alpha	= iParams->alpha,
			beta	= iParams->beta,
			gamma	= iParams->gamma,
			r, s, t, v, w, z;
	UInt32	s1, s2, s3;
	
	if (iData == NIL)
		iData = Taus88GetGlobals();
	Taus88Load(iData, &s1, &s2, &s3);
	
	do  {
	    double	u1 = ULong2Unit_zo( Taus88Process(&s1, &s2, &s3) ),
	    		u2 = ULong2Unit_zo( Taus88Process(&s1, &s2, &s3) );
		
		v  = beta * log(u1 / (1.0-u1)),
	    w  = a * exp(v);						// ?? Can this overflow ??
		z  = u1 * u1 * u2,
		r  = gamma * v - 1.3862944;
		s = a + r - w;
	    		
	    if (s + 2.609438 >= 5.0 * z) break;
	    t = log(z);
	    if (s > t) break;
		} while (r + alpha * log(alpha/(b+w)) < t);
	
	Taus88Store(iData, s1, s2, s3);	
	
    return iParams->mirror ? b / (b+w) : w / (b+w);   
	}


/******************************************************************************************
 *
 *	CalcBCParams(oParams, a, b)
 *	GenBetaBC(iParams, iFunc, iData);
 *	GenBetaBCTaus88(iParams, iData);
 *	
 ******************************************************************************************/

void
CalcBCParams(
	tBCParams*	oParams,
	double		a,
	double		b)
	
	{
	const double	kBCMagic1 = 1.0 / 72.0,
					kBCMagic2 = 1.0 / 24.0,
					kBCMagic3 = 7.0 / 9.0;
	
	double	alpha,
			beta,
			delta,
			k1,
			k2,
			maxExp;
	
	oParams->mirror = (a > b);
	if (oParams->mirror)		// swap
		{ alpha = a; a = b; b = alpha; }
	
	alpha	= a + b;
	beta	= 1.0 / b;
	delta	= 1.0 + a - b;
#if 1
	// There may be a typo here
	k1		= delta * (kBCMagic1 + kBCMagic2 * b) / (a * beta - kBCMagic3);
#else
	k1		= delta * (kBCMagic1 + kBCMagic2 - 2 * b) / (a * beta - kBCMagic3);

#endif
	k2		= 0.25 + (0.5 + 0.25 / delta) * b;
	
	maxExp	= DBL_MAX_EXP * M_LN2 - 1.0;
	if (a > 1.0) maxExp -= ceil(log(a));
	
	oParams->a		= a;
	oParams->b		= b;
	oParams->alpha	= alpha;
	oParams->beta	= beta;
//	oParams->delta	= delta;
	oParams->k1		= k1;
	oParams->k2		= k2;
	oParams->maxExp	= maxExp;
	}

double
GenBetaBC(
	const tBCParams*	iParams,
	tRandomFunc			iFunc,
	void*				iData)
	
	{
	const double	kAlmostInfinity	= DBL_MAX, 
					a				= iParams->a,
					b				= iParams->b,
					alpha			= iParams->alpha,
					beta			= iParams->beta,
					k1				= iParams->k1,
					k2				= iParams->k2,
					maxExp			= iParams->maxExp;
					
	double v, w, z, bw1;
	
	do	{ 
        double	u1 = ULong2Unit_zo( iFunc(iData) ),
        		u2 = ULong2Unit_zo( iFunc(iData) );

        if (u1 < 0.5) {
		    v = u1 * u2;			// Temporarily misuse register v for the following test
		    z = u1 * v;
		    if (0.25 * u2 + z >= k1 + v) continue;
		    // ...otherwise fall through to the tests at the end of the loop
			}
		else {
		    z = u1 * u1 * u2;
		    if (z <= 0.25) {
				// We can skip the more complex loop test condition below. We still need to
				// calculate w and bw1, however.
#if 0
				// FORTRAN implementations add extra code to trap potential
				// overflow/underflow conditions. Given that our parameters have, at most,
				// data from the float/integer domain, whereas the following is calculated
				// with 64-bit precision, overflow is almost impossible.
				w = a * pow(u1/(1.0-u1), beta);
#else
				v = beta * log(u1/(1.0-u1));
				w = (v > maxExp) ? DBL_MAX: a * exp(v);
#endif
				bw1 = 1.0 / (b + w);
				break;
			    }
		    if (z >= k2) continue;
		    // ...otherwise fall through to the tests at the end of the loop
			}
        
        v = beta * log(u1/(1.0-u1));
#if 0
        // We still don't need to worry about overflow/underflow in calculating w, but the
        // intermediate result stored in the variable v is needed for the test condition
        w = a * exp(v);
#else
        w = (v > maxExp) ? DBL_MAX : a * exp(v);
#endif
        bw1 = 1.0 / (b + w);
#if 0
		} while (pow(v * alpha * bw1, alpha) < 4.0 * z);
#else
		} while (alpha * (log(alpha/(b+w))+v) - 1.3862944 < log(z));
#endif
	        // Some implementations formulate the test as follows (or equivalent):
	        //
	        //		alpha * (log(alpha/(b+w) + v) + v) < log(z) + 1.3862943611
	        //
	        // The expression used needs one transcendental math function less, which
	        // is generally a Good Idea.
	
    return bw1 * (iParams->mirror ? b : w);   
	}

double
GenBetaBCTaus88(
	const tBCParams*	iParams,
	tTaus88Data*		iData)
	
	{
	double	a		= iParams->a,
			b		= iParams->b,
			alpha	= iParams->alpha,
			beta	= iParams->beta,
//			delta	= iParams->delta,		// ?? Unused !!
			k1		= iParams->k1,
			k2		= iParams->k2,
			v, w, z, bw1;
	UInt32	s1, s2, s3;
	
	if (iData == NIL)
		iData = Taus88GetGlobals();
	
	Taus88Load(iData, &s1, &s2, &s3);
	
	do	{ 
        double	u1 = ULong2Unit_zo( Taus88Process(&s1, &s2, &s3) ),
        		u2 = ULong2Unit_zo( Taus88Process(&s1, &s2, &s3) );

        if (u1 < 0.5) {
		    v = u1 * u2;			// Temporarily misuse register v for the following test
		    z = u1 * v;
		    if (0.25 * u2 + z >= k1 + v) continue;
		    // ...otherwise fall through to the tests at the end of the loop
			}
		else {
		    z = u1 * u1 * u2;
		    if (z <= 0.25) {
				// We can skip the more complex loop test condition below. We still need to
				// calculate w and bw1, however.
				// FORTRAN implementations add extra code to trap potential
				// overflow/underflow conditions. Given that our parameters have, at most,
				// data from the float/integer domain, whereas the following is calculated
				// with 64-bit precision, overflow is almost impossible.
				w = a * pow(u1/(1.0-u1), beta);
				bw1 = 1.0 / (b + w);
				break;
			    }
		    if (z >= k2) continue;
		    // ...otherwise fall through to the tests at the end of the loop
			}
        
        // We still don't need to worry about overflow/underflow in calculating w, but the
        // intermediate result stored in the variable v is needed for the test condition
        v = beta * log(u1/(1.0-u1));
        w = a * exp(v);
        bw1 = 1.0 / (b + w);
		} while (pow(v * alpha * bw1, alpha) < 4.0 * z);
	        // Some implementations formulate the test as follows (or equivalent):
	        //
	        //		alpha * (log(alpha/(b+w) + v) + v) < log(z) + 1.3862943611
	        //
	        // The expression used needs one transcendental math function less, which
	        // is generally a Good Idea.
	
	Taus88Store(iData, s1, s2, s3);
	
    return bw1 * (iParams->mirror ? b : w);   
	}

