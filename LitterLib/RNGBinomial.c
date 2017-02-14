/*
	File:		BinomialRNG.c

	Contains:	Generate random deviates from a given binomial distribution.

	Written by:	Peter Castine

	Copyright:	© 2001-2006 Peter Castine

	Change History (most recent first):

         <2>   26Ð4Ð2006    pc      Renamed LitterLib files for Binomial distribution
         <1>   30Ð3Ð2006    pc      first checked in (after renaming)
*/


/******************************************************************************************
	Previous history:
	
	Originally part of bernie.c, the source code for the object lp.bernie from the Litter
	Power Package.
	
	Added Kachitvichyanukul/Schmeiser algorithm 2006
	
	Bibiliography:
	
	- Kachitvichyanukul, V. and Schmeiser, B. W. (1988). Binomial random variate generation.
	  Communications of the ACM 31, 216-222. (Algorithm BTPEC).

 ******************************************************************************************/

#pragma mark ¥ Include Files

#include "RNGBinomial.h"
#include "MaxUtils.h"							// for CountBits(), etc.
#include "MoreMath.h"							// for Stirling()


#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark ¥ Private & Inline Functions



#pragma mark -
#pragma mark ¥ Public Functions

/******************************************************************************************
 *
 *	GenDevBinomialFC(iNTrials, iFunc, iData)
 *	GenDevBinomialFCTaus88(iNTrials, iData)
 *	
 *	Generate random deviate from a Binomial distribution B(n, p=0.5).
 *
 *	This special case of a brute force approach performs better than the BINV and BTPE
 *	algorithms as long as we can rely on a fast uniform RNG that delivers 32 random bits.
 *	This is the case with Taus88 and would probably also work well with the Mersenne Twister.
 *	In my (pc) empirical testing, using an inlined Taus88 generator, this "flip-coins"
 *	approach is clearly faster up to about n=256, with a neck-and-neck zone up to about n=320
 *	before BTPE is a clear improvement. I suggest 288 as a cutoff.
 *
 *	The second version is hardwired to inline the Taus88 algorithm
 *
 ******************************************************************************************/

UInt32
GenDevBinomialFC(
	UInt32		iNTrials,
	tRandomFunc	iFunc,
	void*		iData)
	
	{
	UInt32	result	= 0,
			mask	= kULongMax << (32 - (iNTrials & 0x0000001f));
	
	if (mask != 0)
		result += CountBits(iFunc(iData) && mask);
	
	iNTrials >>= 5;
	while (iNTrials-- > 0)
		result += CountBits(iFunc(iData));
	
	return result;
	}
	

UInt32
GenDevBinomialFCTaus88(
	UInt32			iNTrials,
	tTaus88Data*	iData)
	
	{
	UInt32	result	= 0,
			mask	= 0xffffffff << (32 - (iNTrials & 0x0000001f)),
			s1, s2, s3;
	
	if (iData == NIL)
		iData = Taus88GetGlobals();
	
	Taus88Load(iData, &s1, &s2, &s3);
	
	if (mask != 0)
		result += CountBits(Taus88Process(&s1, &s2, &s3) & mask);
	
	iNTrials >>= 5;
	while (iNTrials-- > 0)
		result += CountBits( Taus88Process(&s1, &s2, &s3) );
	
	Taus88Store(iData, s1, s2, s3);
	
	return result;
	}


/******************************************************************************************
 *
 *	GenBinomialBF(iNTrials, iThresh, iFunc, iData)
 *	GenBinomialBFTaus88(iNTrials, iProb, iData)
 *	
 *	GenBinomialBF() generates random variates from a Binomial distribution B(n, p) by
 *	simulating n Bernoulli experiments, each with probability p. The value iThresh is an
 *	unsigned 32-bit integer mapping p into the domain [0.. 2^32). Use the inline function
 *	CalcBFThresh() defined in BinomialRNG.h to calculate this value.
 *
 *	GenBinomialBFTaus88() is a version of the algorithm hard-wired to the Taus88 uniform
 *	genrator for enhanced performance.
 *
 *	This brute force approach was the method used in the original version of the Litter
 *	Power Package. For quite small values of n it is an efficient approach. 
 *
 ******************************************************************************************/

UInt32
GenDevBinomialBF(
	UInt32		iNTrials,
	UInt32		iThresh,
	tRandomFunc	iFunc,
	void*		iData)

	{
	UInt32	result = 0;
			
	while (iNTrials-- > 0)
		if(iFunc(iData) < iThresh) result += 1;
	
	return result;			 
	}
	
UInt32
GenDevBinomialBFTaus88(
	UInt32			iNTrials,
	UInt32			iThresh,
	tTaus88Data*	iData)

	{
	UInt32	result = 0,
			s1, s2, s3;
			
	if (iData == NIL)
		iData = Taus88GetGlobals();
	
	Taus88Load(iData, &s1, &s2, &s3);
	
	while (iNTrials-- > 0)
		if(Taus88Process(&s1, &s2, &s3) < iThresh) result += 1;
	
	Taus88Store(iData, s1, s2, s3);
	
	return result;			 
	}
	

/******************************************************************************************
 *
 *	CalcBINVParams(oParams, iNTrials, iProb)
 *	GenBinomialBINV(oParams, iFunc, iData)
 *	
 *	GenBinomialBINV() generates random variates from a Binomial distribution B(n, p). It
 *	performs faster than the Kachitvichyanukul/Schmeiser BTPE algorithm when the mean
 *	(ie n*p) is small. K&S set the threshhold at 30, whereas GSL reports 14 as a more
 *	appropriate value. My own (pc) empirical indicates that the cutoff can be set even 
 *	lower, around 7.5.
 *
 *	CalcBINVParams() must be called to before calling GenBinomialBINV() to fill a 
 *	tBINVParams data structure with precalculated values for the given n and p. 
 *
 *	Since the BINV typically only requires a single uniform variate, a version hard-wired
 *	to Taus88 is not provided.
 *
 ******************************************************************************************/

void
CalcBINVParams(
	tBINVParams*	oParams,
	UInt32			iNTrials,
	double			iProb)
	
	{
	double q;
	
	oParams->n = iNTrials;
	
	if (iProb > 0.5) {
		q = iProb;
		
		oParams->mirror = true;
		oParams->p		= iProb	= 1.0 - iProb;
		}
	else {
		q = 1.0 - iProb;
		
		oParams->mirror = false;
		oParams->p		= iProb;
		}
	
	oParams->qPowN	= pow(q, iNTrials);
	oParams->pOverQ	= iProb / q;
	}

UInt32
GenBinomialBINV(
	const tBINVParams*	iParams,
	tRandomFunc			iFunc,
	void*				iData)
	
	{
	const double	n 		= (double) iParams->n,
					pOverQ	= iParams->pOverQ;
					
	double	x, nx;
	Boolean	underflow;
	
	// Normally the do-loop below will only be performed once. However, in extremely rare
	// circumstances (large n, u within a few epsilons of 1), 64-bit floating point can
	// still underflow on us and we will have to try again.
	do	{
		double	u	= ULong2Unit_ZO( iFunc(iData) ),
				fx	= iParams->qPowN;					// Probability of current point
		
		x		= 0.0;									// Current point
		nx		= n;									// Mirror of current point (n - x)
		underflow = false;
		while (u > fx) {
			u -= fx;
			
			// Use recursion f(x+1) = f(x) * (n-x) / (x+1) * [p/(1-p)]
			fx *= nx--;
			fx /= ++x;
			fx *= pOverQ;
			
			if (x > 110.0) {
				// If x has gotten past this value it indicates that the cumulative
				// calculations have underflowed past the limits of double-precision
				// floating point accuracy.
				// The threshhold value is taken from Kachitvichyanukul and Schmeiser.
				// Bummer. Wonder how often this happens??
				underflow = true;
				break;
				}
			}
		} while(underflow);
	
	return (UInt32) (iParams->mirror ? nx : x);
    }


/******************************************************************************************
 *
 *	GenBinomialBTPE(iNTrials, iFunc, iData)
 *	CalcBTPEParams(iNTrials, iData)
 *	
 *	GenBinomialBTPE() generates random deviates from a Binomial distribution B(n, p).
 *
 *	CalcBTPEParams() must be called to before calling GenBinomialBINV() to fill a 
 *	tBTPEParams data structure with precalculated values for the given n and p.
 *
 ******************************************************************************************/


void CalcBTPEParams(
	tBTPEParams*	oParams,
	UInt32			iNTrials,
	double			iProb)	// must be strictly inside unit range, otherwise NaNs and Infs
	
	{
	// These magic numbers are not adjustable. Leave them alone.
	const double	kBTPEMagic1 = 2.195,
					kBTPEMagic2 = 4.6,
					kBTPEMagic3 = 0.134,
					kBTPEMagic4 = 20.5,
					kBTPEMagic5 = 15.3;
					
	long	m;								// floor[n*p+p] = mode of distribution
	double	q,								// 1 - p
			mean,							// n * p
			var,							// variance = n * p * q
			npp,							// n * p + p 
			mm,								// floor(npp), == (double) m
			xm,								// tip of triangle
			p1, p2, p3, p4,					// cumulative area of tri, para, exp tails
    		xl, xr,							// left and right edges of triangle
    		lambdaL, lambdaR,				// 
    		a, c;   						// various intermediate values
	
	if (iProb > 0.5) {
		oParams->q		= q		= iProb;
		oParams->p		= iProb = 1.0 - iProb;
		oParams->mirror = true;
		}
	else {
		oParams->q		= q		= 1.0 - iProb;
		oParams->p		= iProb;
		oParams->mirror = false;
		}
							
	mean	= iProb * iNTrials;
	var		= mean * q;
	npp		= mean + iProb;
	m		= npp;							// Typecast to double truncates to int
	mm		= m;							// (implicit floor)
	xm		= mm + 0.5;
			
	// pi/p4 is the probability of i'th area (i=1,2,3,4)
	p1 = floor(kBTPEMagic1 * sqrt(var) - kBTPEMagic2 * q) + 0.5;
	// p1: radius of triangle region; since height=1, also: area of region
	
	xl = xm - p1;							// left and right edges of triangle
	xr = xm + p1;							// Need these to calc parallelogram
	
	// Left tail:  t(x) = c * exp(-lambdaL*[xl - (x+0.5)])
	// Right tail: t(x) = c*exp(-lambdaR*[(x+0.5) - xr])
	c	= kBTPEMagic3 + kBTPEMagic4 / (kBTPEMagic5 + mm);
	p2	= p1 * (1.0 + c + c);				// p1 + area of parallelogram region
	
	a	= (npp - xl) / (npp - xl * iProb);	lambdaL = a * (1.0 + 0.5*a);
	a	= (xr - npp) / (xr * q);			lambdaR = a * (1.0 + 0.5*a);
	p3	= p2 + c / lambdaL;					// p2 + area of left tail
	p4	= p3 + c / lambdaR;					// p3 + area of right tail
	
	// Cache important values
	oParams->var	= var;
	oParams->p1		= p1;
	oParams->p2		= p2;
	oParams->p3		= p3;
	oParams->p4		= p4;
	oParams->xm		= xm;
	oParams->c		= c;
	oParams->xl		= xl;
	oParams->xr		= xr;
	oParams->lambdaL = lambdaL;
	oParams->lambdaR = lambdaR;
	oParams->n		= iNTrials;
	oParams->mm		= mm;
    }

UInt32
GenBinomialBTPE(
	const tBTPEParams*	iParams,		// Should we make provision for a NIL iParams ??
	tRandomFunc			iFunc,
	void*				iData)

	{
	long	result,
			m		= iParams->mm;
	double	p		= iParams->p,
			var		= iParams->var,
			p1		= iParams->p1,
			p2		= iParams->p2,
			p3		= iParams->p3,
			p4		= iParams->p4,
			xm		= iParams->xm,
			c		= iParams->c,
			xl		= iParams->xl,
			xr		= iParams->xr,
			lambdaL = iParams->lambdaL,
			lambdaR = iParams->lambdaR,
			n		= iParams->n,
			mm		= iParams->mm;
	
	do	{
		double	u	= ULong2Unit_ZO( iFunc(iData) ) * p4,	// Start off with two uniform
				v	= ULong2Unit_ZO( iFunc(iData) );	// deviates...
		long	k;
				
		if (u <= p1) {									// Triangular region
			result = xm - p1 * v + u;
			break;										// That was easy
			}
		
		else if (u <= p2) {								// Parallelogram region
			double x = xl + (u-p1) / c;
			v *= c;
			v += 1.0;
			v -= fabs(x-xm) / p1;
			if(v > 1.0 || v <= 0.0)						// Bad luck, try again
				 continue;
			else result = x;							// Potential solution
			}
		
		else if (u <= p3) {								// Left tail
			result = xl + log(v)/lambdaL;
			if (result < 0)								// Bad luck, try again
	    		 continue;
	  		else v *= (u-p2)*lambdaL;					// Potential solution
	  		}
			
		else {											// Right tail
			result = xr - log(v)/lambdaR;
			if (result > n)
				 continue;								// Bad luck, try again
	   		else v *= (u-p3) * lambdaR;					// Potential solution
			}
			  
		// At this point, the goal is to test whether v <= f(x)/f(m)
		//
		//												   m!(n-m)!
		//												<= -------- * (p/q)^(x-m)
		//												   x!(n-x)!
		//
    	// The following "squeeze" technique is an efficient determination of said condition
    	//
    	
		k = (result > m)
				? result - m
				: m - result;
		if (k > 20) {										// Empirically determined constant
    		// If result is far from the mean m: k=ABS(result-m) large 
		    double	x1, w1, f1, z1,
		    		lnv = log(v);
		
			if (k < 0.5 * var - 1) { 
	   			// "Squeeze" using upper and lower bounds on log(f(x)) 
	      		// The squeeze condition was derived under the condition k < npq/2-1
		        double amaxp = k / var * ((k* (k/3.0 + 0.625) + (1.0/6.0)) / var +0.5);
		        double ynorm = -(k*k / (2.0 * var));
		        if(lnv < ynorm-amaxp)
		        	break;									// Done!
		        if(lnv > ynorm+amaxp)
		        	continue;								// Bad luck, try again
				}
			
			// Now, in the following test, according to the math we appararently need to add
			// Stirling(f1,f2) and Stirling(z1,z2) and subtract Stirling(x1,x2) and
			// Stirling(w1,w2) from the right hand side of the inequality, with Stirling
			// as defined in MoreMath.h
			//
			// Empirical tests have shown that the the Stirling values are so close
			// to zero that they seem not to make a difference. In fact, the algorithm
			// published by Kachitvichyanukul and Schmeiser mistakenly adds all four
			// Stirling()s, (instead of subtracting the last two) and gets exactly the same
			// results. OTOH, this section of code is used in perhaps 1% of function calls
			// and in the vast majority of my (pc) empirical tests performance does not
			// deteriorate significantly because of the calls.
	    	f1 = mm + 1.0;
	    	w1 = n - result + 1.0;
			x1 = result + 1.0;
	    	z1 = n + 1.0 - mm;

			if(lnv <= xm * log(f1/x1)
						+ (n-mm+0.5) * log(z1/w1)
						+ (result - m) * log(w1*p/(x1*iParams->q))
						+ Stirling(f1) + Stirling(z1)
						- Stirling(x1) - Stirling(w1))
	      		 break;										// At last!
	   		else continue;									// Damn. Back to square one
			}
		
		else {
			// If result is near to m (ie, |result-m| <= 20), then do an explicit evaluation
			// using the recursion relation for f(x)
			double	pq	= p / iParams->q,
		  			g	= (n+1) * pq,
		  			f	= 1.0;
		  	int		i;
		 
			if		(m < result) for (i= m + 1; i <= result; i += 1) f *= (g/i - pq);
			else if (m > result) for (i= result + 1; i <= m; i += 1) f /= (g/i - pq);
			
			if (v <= f) break;								// At last!
			else 		continue;							// Damn. Back to square one
			}
  	
		} while (true);
	
	return (iParams->mirror) ? n - result : result;
	}
