/*
	File:		MoreMath.c

	Contains:	Advanced mathematical functions not provided by the standard C mathlib.

	Written by:	Peter Castine

	Copyright:	© 2001-2006 Peter Castine

	Change History (most recent first):

         <3>   27–3–2006    pc      Export EvalPoly()
         <2>   23–3–2006    pc      Move constants (kPi, etc.) to main LitterLib files. The
                                    constants are used in lots of projects that don't need the
                                    heavy-duty math functions.
         <1>     26–2–06    pc      First checked in. Split off mathematical functions that had
                                    collected in LitterLib.c
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "MoreMath.h"						// also #includes <math.h>
#include "LitterLib.h"


#pragma mark • Constants

	

#pragma mark • Type Definitions



#pragma mark • Static Variables



#pragma mark • Initialize Global Variables



#pragma mark • Function Prototypes



#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark • Private & Inline Functions



#pragma mark -
#pragma mark • Factorial, Gamma, and Related Functions

/******************************************************************************************
 *
 *	lgamma(x)
 *	
 *	I don't understand why MSL for Windows doesn't provide gamma() and lgamma(), but it
 *	doesn't.
 *
 *	Using the same function names and parameters as C9X for compatibility.
 *
 *	gamma() is currently defined in LitterLib.h as a simple inline. There are probably
 *	better ways.
 *	
 ******************************************************************************************/
 
#ifdef _WINDOWS_

double
lgamma(
	double x)
 	
	{
	const double kCoefficients[6] = {	 76.18009172947146,
										-86.50532032941677,
										 24.01409824083091,
										 -1.231739572450155,
										  0.1208650973866179e-2,
										  -0.5395239384953e-5
										  };
	
	double	y	= x,
			tmp,
			ser;
	int		j;
	
	tmp = x + 5.5;
	tmp -= (x + 0.5) * log(tmp);
	
	ser	= 1.000000000190015;
	for (j=0; j < 6; j++)
		ser += kCoefficients[j] / ++y;
	
	return -tmp + log(2.5066282746310005 * ser / x);
	}

#endif

/******************************************************************************************
 *
 *	Factorial(n)
 *	LogFactorial(n)
 *	
 *	Factorial of an integer is faster than the Gamma function.
 *
 *	The result is returned in a double to take advantage of the greater range available
 *	
 ******************************************************************************************/
 
	// These value are shared between Factorial() and LogFactorial()
const long	kMinInput	= 0,			// Factorial undefined for negative values
			kMaxInput	= 170,			// Maximum n! that can be calculated with
										// double-precision
			kPrecalc	= 10,			// Cache n! for 0..10
										// 10! is the largest that will fit into single-
										// precision float without loss of accuracy
			kBruteForce	= 32;			// Value up to which we use a brute force
										// approach, after which we opt for using the
										// Gamma function.
double
Factorial(
	long n)
	
	{
				// Must be kPrecalc+1 but XCode is too lame to initialize
				// an array when size is specified with a symbolic constant
	const float	kPrecalcVals[11]			= {		1.0,		// 0!
													1.0,		// 1!
													2.0,		// 2!
													6.0,		// 3!
											       24.0,		// 4!
												  120.0,		// 5!
												  720.0,		// 6!
												 5040.0,		// 7!
												40320.0,		// 8!
											   362880.0,		// 9!
											  3628800.0};		// 10!
	double result;
	
	if (n < kMinInput)
		result = 0.0 / 0.0;					// Value is undefined, return NaN
	
	else if	(n <= kPrecalc)
		result = kPrecalcVals[n];			// Float-to-double conversion is the price we
											// pay to keep table size down
	
	else if (n <= kBruteForce) {
		double	jj;							// Want to minimize int-to-float conversions,
		int		j;							// which are surprisingly expensive
		
		result = kPrecalcVals[kPrecalc];
		for (jj = j = n; j > kPrecalc; jj -= 1.0, j -= 1)
			result *= jj;
		}
	
	else if (n <= kMaxInput)				// Use Gamma function
		result = exp(lgamma(n + 1.0));
	
	else result = 1.0 / 0.0;				// Out of range, which is close enough to
											// infinity for all practical purposes
	return result;
	}

double
LogFactorial(
		long n)
	
	{
	// As opposed to Factorial() skip over 0! and 1!
				// Must be kPrecalc-1 but XCode is too lame to initialize
				// an array when size is specified with a symbolic constant
	const double	kPrecalcVals[9]			=  { 0.6931471806,	// log(2!)
												 1.7917594692,	// log(3!)
												 3.1780538303,	// log(4!)
												 4.7874917428,	// log(5!)
												 6.5792512120,	// log(6!)
												 8.5251613611,	// log(7!)
												10.6046029027,	// log(8!)
												12.8018274801,	// log(9!)
												15.1044125731};	// log(10!)
	
	double result;
	
	if (n < kMinInput)
		result = 0.0 / 0.0;					// Value is undefined, return NaN
	
	else if	(n <= 1)						// 0! and 1! are very special cases 
		result = 0.0;						// Clearing bits is even faster than table
											// lookup
	
	else if (n <= kPrecalc)
		result = kPrecalcVals[n-2];			// ¡Hola! 
	
	else if (n <= kBruteForce)				// Brute force multiplication is still cheaper
		result = log( Factorial(n) );		// than adding lots of brute force log()s.
	
	else result = lgamma(n + 1);			// As opposed to factorial, this won't overflow
	
	return result;
	}
 

/******************************************************************************************
 *
 *	digamma(n)
 *	DigammaInt(n)
 *	DigammaIntPlusHalf(n)
 *	
 *	Digamma (aka psi) functiosn. Integer and half-integer values can
 *	be calculated with these functions.
 *
 ******************************************************************************************/


	// General digamma implememtataion for all positive real arguments
	// Based on code by Bill Venables, written in S+
double
digamma(
	double x)
	
	{
	double result;
	
	if (x > 5.0) {
		// Numeric approximation
		double	tail,
				invX2;
		
		invX2  = 1.0 / x;
		invX2 *= invX2;
		
		tail  = (3617.0 / 8160.0);
		tail *= invX2;
		tail -= 1.0 / 12.0;
		tail *= invX2;
		tail += 691.0 / 32760.0;
		tail *= invX2;
		tail -= 1.0 / 132.0;
		tail *= invX2;
		tail += 1.0 / 240.0;
		tail *= invX2;
		tail -= 1.0 / 252.0;
		tail *= invX2;
		tail += 1.0 / 120.0;
		tail *= invX2;
		tail -= 1.0 / 12.0;
		tail *= invX2;
		
		result  = log(x);
		result -= 0.5 / x;
		result += tail;
		}
	
	else if (x > 0.0) {
		// x is too small to be calculated accurately with the numeric relationship
		// used above. Calculate for a larger value and then use recurrence relationship
		//
		//		psi(x+1) = psi(x) + 1/x
		
		result  = -1.0 / x++;				// result =	- 1/x
		result += -1.0 / x++;				//			- 1/(x+1)
		result += -1.0 / x++;				//			- 1/(x+2)
		result += -1.0 / x++;				//			- 1/(x+3)
		result += -1.0 / x++;				//			- 1/(x+4)
		result += digamma(x);				//			+ psi(x+5)
		}
	
	else {
		// Use reflection formula
		//
		//		psi(1-x) = psi(x) + pi cot(pi * x) 
		//
		// Note that cot(pi * x) is undefined for integer x
		double	xx		= 1.0 - x,			 // ASSERT xx > 0
				xFract	= xx - floor(xx);
		
		if (xFract != 0.0) {
			result = digamma(xx);
			if (xFract != 0.5) {
				result += kPi / tan(kPi * xFract);
				}
			}
		else result = 0.0 / 0.0;
		}
	
	return result;
	}
	
 

double
DigammaInt(
		long n)												// Calculate psi(n)
	
	{
	const long		kMaxDirectCalc	= 10;					// Rough estimate, should test!!
															// Possibly much less, the int
															// versions may not be worth the
															// effort
	const double	kEulerGamma		= 0.5772156649;

	double result;
	
	if (n > kMaxDirectCalc)
		result = digamma(n);
		
	else if (n > 0) {
		long	j;
		double	jj;
		
		result = -kEulerGamma;
		// Use recursion formula
		for (j = 1, jj = 1.0; j < n; j += 1, jj += 1.0) {
			result += 1.0 / jj;
			}
		}
		
	else result = 0.0 / 0.0;							// Undefined
	 
	return result;
	}

	// Calculate psi(n + 1/2)
double
DigammaIntPlusHalf(
				long n)
	
	{
	const long		kMaxDirectCalc	= 10;					// Rough estimate, should test!!
															// Possibly much less, the int
															// versions may not be worth the
															// effort
															
	const double kMinusGammaHalfMinusLn2 = 0.981755013;	// (-gamma-2*log(2))/2
	
	double result;
	
	if (n >= kMaxDirectCalc)
		result = digamma((double) n + 0.5);
		
	if (n >= 0) {
		long	j;
		double	jj;
		
		result = kMinusGammaHalfMinusLn2;
		for (j = 1, jj = 1.0; j <= n; j += 1, jj += 2.0) {
			result += 1.0 / jj;
			}
		result += result;
		}
	
	else {
		// Use reflection formula
		//
		//		psi(1-x) = psi(x) + pi / tan(pi * x) 
		//
		// Note that tan(pi/2) is infinite, so pi/tan(pi/2) is zero
		
		result = DigammaIntPlusHalf(-n);
		}
	
	return result;
	}
	
/******************************************************************************************
 *
 *	EvalPoly(x, iCoeffs, iOrder)
 *	EvalPolyNorm(x, iCoeffs, iOrder)
 *	
 *	Helper functions for evaluating polynomials.
 *
 *	NOTE: Neither function is safe if used for 0-th order polynomials. Furthermore, 
 *		  EvalPolyNorm() is not safe for the identity function. Currently callers must
 *		  special-case these situations.
 *		  TO-DO this functionality should be added without a performance hit.
 *
 ******************************************************************************************/

double
EvalPoly(
	double			x,
	const double	iCoeffs[],
	UInt32			iOrder)
	
	{
	UInt32	i		= iOrder;
	double*	p		= (double*) iCoeffs;
	double	result	= *p;
	
	do { result *= x; result += *++p; } while (--i > 0);

	return result;
	}
	
	// "Normalized" polynomial, i.e. coefficient of x^N is unity
double
EvalPolyNorm(
	double			x,
	const double	iCoeffs[],
	UInt32			iOrder)
	
	{
	UInt32	i		= iOrder - 1;
	double*	p		= (double*) iCoeffs;
	double	result	= x + *p;

	do { result *= x; result += *++p; } while (--i > 0);

	return result;
	}


/******************************************************************************************
 *
 *	RiemannZetaFrac(n)
 *	
 *	Returns the "fractional" part of the Riemann zeta function (ie, that which exceeds one).
 *	Most values of the zeta function are very close to one.
 *
 ******************************************************************************************/

double
RiemannZetaFrac(
		double x)
	
	{
	double b, s, w, zeta;

	if( x < 0.0 ) {
		if(x < -170.6243) zeta = 0.0;
		else {
			// Use reflection relation
			s = 1.0 - x;
			w = RiemannZetaFrac(s);
			zeta  = sin((kPi / 2.0) * x);
			zeta *= pow(2.0 * kPi, x);
			zeta *= gamma(s);
			zeta *= (1.0 + w);
			zeta *= 1.0 / kPi;
			zeta -= 1.0;
			}
		}
	
	else if (x >= 127)				// Maximum binary exponent
		zeta = 0.0;					// First term is 2^(-x)

	else {
		w = floor(x);
		
		if (w == x && x <= 31.0) {
			// Use precalculated values
			const double kPreCalc[] = {	-1.5,
										1.0 / 0.0,					// infinity
										6.44934066848226436472E-1,
										2.02056903159594285400E-1,
										8.23232337111381915160E-2,
										3.69277551433699263314E-2,
										1.73430619844491397145E-2,
										8.34927738192282683980E-3,
										4.07735619794433937869E-3,
										2.00839282608221441785E-3,
										9.94575127818085337146E-4,
										4.94188604119464558702E-4,
										2.46086553308048298638E-4,
										1.22713347578489146752E-4,
										6.12481350587048292585E-5,
										3.05882363070204935517E-5,
										1.52822594086518717326E-5,
										7.63719763789976227360E-6,
										3.81729326499983985646E-6,
										1.90821271655393892566E-6,
										9.53962033872796113152E-7,
										4.76932986787806463117E-7,
										2.38450502727732990004E-7,
										1.19219925965311073068E-7,
										5.96081890512594796124E-8,
										2.98035035146522801861E-8,
										1.49015548283650412347E-8,
										7.45071178983542949198E-9,
										3.72533402478845705482E-9,
										1.86265972351304900640E-9,
										9.31327432419668182872E-10};
			
			zeta = kPreCalc[(int) x];
			}

		else if (x < 1.0) {
							// (1-x) (zeta(x) - 1), 0 <= x <= 1
			const double	R[6] = {	-3.28717474506562731748E-1,
										1.55162528742623950834E1,
										-2.48762831680821954401E2,
										1.01050368053237678329E3,
										1.26726061410235149405E4,
										-1.11578094770515181334E5},
							
							// The following omits the "real" zero-th element, which is one.
							S[5] = {	1.95107674914060531512E1,
										3.17710311750646984099E2,
										3.03835500874445748734E3,
										2.03665876435770579345E4,
										7.43853965136767874343E4};
			zeta  = EvalPoly(x, R, 5);
			zeta /= (1.0 - x) * EvalPolyNorm( x, S, 5);
			}

		else if (x == 1.0)			// Zeta is infinite
			zeta = 1.0 / 0.0;

		else if (x <= 10.0) {
							// 2^x (1 - 1/x) (zeta(x) - 1) = P(1/x)/Q(1/x), 1 <= x <= 10
			const double	P[9] = {	5.85746514569725319540E11,
										2.57534127756102572888E11,
										4.87781159567948256438E10,
										5.15399538023885770696E9,
										3.41646073514754094281E8,
										1.60837006880656492731E7,
										5.92785467342109522998E5,
										1.51129169964938823117E4,
										2.01822444485997955865E2},
						
							// The following omits the "real" zero-th element, which is one.
							Q[8] = {	3.90497676373371157516E11,
										5.22858235368272161797E10,
										5.64451517271280543351E9,
										3.39006746015350418834E8,
										1.79410371500126453702E7,
										5.66666825131384797029E5,
										1.60382976810944131506E4,
										1.96436237223387314144E2};
			
			b = pow(2.0, x) * (x - 1.0);
			w = 1.0 / x;
			zeta  = x * EvalPoly(w, P, 8);
			zeta /= b * EvalPolyNorm(w, Q, 8);
			}

		else if (x <= 50.0) {
							// log(zeta(x) - 1 - 2^(-x)), 10 <= x <= 50
			const double	A[11] = {	8.70728567484590192539E6,
										1.76506865670346462757E8,
										2.60889506707483264896E10,
										5.29806374009894791647E11,
										2.26888156119238241487E13,
										3.31884402932705083599E14,
										5.13778997975868230192E15,
										-1.98123688133907171455E15,
										-9.92763810039983572356E16,
										7.82905376180870586444E16,
										9.26786275768927717187E16},
										
							// The following omits the "real" zero-th element, which is one.
							B[10] = {	-7.92625410563741062861E6,
										-1.60529969932920229676E8,
										-2.37669260975543221788E10,
										-4.80319584350455169857E11,
										-2.07820961754173320170E13,
										-2.96075404507272223680E14,
										-4.86299103694609136686E15,
										 5.34589509675789930199E15,
										 5.71464111092297631292E16,
										-1.79915597658676556828E16};
			
			zeta  = EvalPoly(x, A, 10);
			zeta /= EvalPolyNorm(x, B, 10);
			zeta  = exp(zeta);
			zeta += pow(2.0, -x);
			}

		else {
			// Brute force sum of inverse powers
			const double kDoubleEpsilon =  1.11022302462515654042E-16;		// 2^(-53)
			
			double	a = 1.0;
			
			s = 0.0;
			do  {
				a += 2.0;
				b = pow(a, -x);
				s += b;
				} while (b > kDoubleEpsilon * s);

			b = pow(2.0, -x);
			zeta = (s + b) / (1.0 - b);
			}
		}
	
	return zeta;							// In all cases really zeta(x) - 1
	}

/******************************************************************************************
 *
 *	Chebyshev1(n, x)
 *	
 *	Calculates Chebyshev functions of the first kind using the divide-and-conquer method
 *	developed by Wolfram Koepf in "Efficient Computation of Chebyshev Polynomials in 
 *	Computer Algebra" <http://www.mathematik.uni-kassel.de/~koepf/cheby.pdf>
 *
 ******************************************************************************************/

double
Chebyshev1(
	long	n,
	double	x)
	
	{
	double result;
	
	if (n <= 0)
		result = 1;
		
	else if (n == 1)
		result = x;
		
	else if (n & 0x01) {
		// For odd n, use the recursion formula T(n, x) = T((n-1)/2, x) * T((n+1)/2, x) - x
		
		n >>= 1;						// Integer divide by two, truncate
		
		result  = Chebyshev1(n, x);
		result *= Chebyshev1(n + 1, x);
		result += result;
		result -= x;
		}
	
	else {
		// For even n, use the recursion formula T(n, x) = 2 * T(n/2, x)^2 - 1
		result = Chebyshev1(n >> 1, x);
		result *= result;
		result += result;
		result -= 1.0;
		}
		
	return result;
	}
