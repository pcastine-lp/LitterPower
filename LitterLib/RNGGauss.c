/*
	File:		RNGGauss.c

	Contains:	Library functions for use by the Litter Power Package.

	Written by:	Peter Castine

	Copyright:	© 2001-2006 Peter Castine

	Change History (most recent first):

         <1>   30–3–2006    pc      Initial check in.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "LitterLib.h"				// Include first
#include "RNGGauss.h"				// Also include MiscUtils.h




#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/
#pragma mark • Utilities

/******************************************************************************************
 *
 *	UnitSq2Norm(io1, io2)
 *	UnitCirc2Norm(io1, io2)
 *	
 *	Preform Box-Muller transform for generating pairs of deviates from a normal (i.e.,
 *	Gaus(0, 1)) distribution. The parameters point, on entrance, to a pair of floating
 *	point values (double precision) representing a point on the Cartesian plane strictly
 *	inside the unit square or unit circle. On exit these memory locations will contain
 *	deviates. Assuming uniformly distributed input values, the output values will have
 *	a normal distribution.
 *	
 *	The unit circle transformation is much less processor-intensive; enough so to offset
 *	the additional overhead of possible multiple calls to a random number generator if we
 *	don't generate a point inside the unit circle on a first attempt. The unit square
 *	transform is provided for situations (such as mapping MIDI or other input) where it may
 *	not be feasible to enforce the condition of coordinates inside the unit circle.
 *	
 *	Cf. Box, G.E.P, M.E. Muller 1958; "A note on the generation of random normal
 *	deviates," Annals Math. Stat, V. 29, pp. 610-611 (cited in _Numerical Recipes_ and _The
 *	Art of Computer Programming_ (Vol. II).
 *	
 ******************************************************************************************/

void
UnitSq2Norm(
	double*	io1,
	double*	io2)
	// Transform:
	//		y1 = sqrt(-2 * ln(x1)) * cos(2 * pi * x2)
	//		y2 = sqrt(-2 * ln(x1)) * sin(2 * pi * x2)
	//
	// Input conditions:
	//		0 < x1 < 1
	//		0 < x2 < 1
	//						(x2 may exceed the range w/out causing arithmetic exception)
	{
	double	x1Transform	= sqrt(-2.0 * log(*io1)),
			x2Transform	= k2pi * *io2;
	
	*io1 = x1Transform * cos(x2Transform);
	*io2 = x1Transform * sin(x2Transform);
	}
	

void
UnitCirc2Norm(
	double*	io1,
	double*	io2)
	// Transform:
	//	y1 = sqrt(-2.0 * log(r2) / r2) * x1;
	//	y2 = sqrt(-2.0 * log(r2) / r2) * x2;
	//			with r2 = x1^2 + x2^2 
	//
	// Input conditions:
	//		0 < |x1| < 1
	//		0 < |x2| < 1		
	//						(Either parameter may exceed the range w/out causing
	//						arithmetic exception)
	{
	double	x1	= *io1,
			x2	= *io2,
			r2	= x1 * x1 + x2 * x2;
	
	if (r2 >= 1.0) {
		// This should not happen, but  
		*io1 = *io2 = 0.0;
		}
	if (r2 > 0.0) {
		double fact =  -log(r2);
		fact += fact;
		fact /= r2;
		fact =  sqrt(fact);
		
		*io1 = fact * x1;
		*io2 = fact * x2;
		}
	
	// If the condition (r2 == 0.0), then *io1 == *io2 == 0, and we just leave the values
	// alone. Of course, r2 cannot be negative.
		
	}
	

#pragma mark -
#pragma mark • Legacy Box-Muller approach

double
NormalBM(
	double*		oSpare,		// This alg. generates *2* values. Caller may cache 2nd variate.
	tRandomFunc	iFunc,
	void*		iData)
	
	{
	double	v1, v2,						// Two uniform random numbers...
			r2,							// Radius squared of above
			f;							// Magic factor
	
	// Take a pair of uniform random numbers inside the unit circle...
	do {
		v1 = Long2Signal((long) iFunc(iData));
		v2 = Long2Signal((long) iFunc(iData));
		r2 = v1 * v1 + v2 * v2;
		} while (r2 >= 1.0 || r2 == 0.0);
	
	// ... and hit the magic Box-Muller transformation.
	f =  -log(r2);
	f += f;
	f /= r2;
	f =  sqrt(f);
		
	if (oSpare != NULL)
		*oSpare = v2 * f;
	
	return v1 * f;
	}

double
NormalBMTaus88(
	double*			oSpare,	// This alg. generates *2* values. Caller may cache 2nd variate.
	tTaus88Data*	iData)
	
	{
	double	v1, v2,						// Two uniform random numbers...
			r2,							// Radius squared of above
			f;							// Magic factor
	UInt32	s1, s2, s3;
	
	// Look for a pair of uniform random numbers inside the unit circle...
	
	if (iData == NIL)
		iData = Taus88GetGlobals();
	Taus88Load(iData, &s1, &s2, &s3);
	
	do {
		v1 = Long2Signal( (long) Taus88Process(&s1, &s2, &s3) );
		v2 = Long2Signal( (long) Taus88Process(&s1, &s2, &s3) );
		r2 = v1 * v1 + v2 * v2;
		} while (r2 >= 1.0 || r2 == 0.0);
	
	Taus88Store(iData, s1, s2, s3);
	
	// ... and hit the magic Box-Muller transformation.
	f =  -log(r2);
	f += f;
	f /= r2;
	f =  sqrt(f);
		
	if (oSpare != NULL)
		*oSpare = v2 * f;
	
	return v1 * f;
	}


#pragma mark -
#pragma mark • Kinderman-Ramage Algorithm

	static inline double NormKRHelper(double x, double A)
		{ return 0.3989422804 * exp(-0.5 * x * x) - 0.1800251911 * (A - x); }

double
NormalKR(
	tRandomFunc	iFunc,
	void*		iData)
	
	{
    const double A = 2.2160358672;
    
	UInt32	u1;
	double	u2,
			u3,
			result;
	
	
	u1 = iFunc(iData);
	u2 = ULong2Unit_ZO( iFunc(iData) );
	u3 = ULong2Unit_ZO( iFunc(iData) );

	
	if (u1 < 3797053464UL)							// Main body of standard distribution
	    result = A * (u2 + u3 - 1.0);
	
	else if (u1 < 3914058587UL) do {					// Region 1
		double sign = 1.0;
			
		if (u3 < u2) {
			// Swap random deviates, generate negative variate
			double temp = u2;
			u2 = u3;
			u3 = temp;
			sign = -1.0;
			}										// ASSERT (u2 < u3)
	   
	    result = 0.4797274042 - 0.5955071380 * u2;
	    if (result >= 0.0) {						// Potential variate
		    if (u3 <= 0.8055779244
		    		|| 0.0533775495 * (u3-u2) <= NormKRHelper(result, A)) {
				result *= sign;						// We have a variate
				break;
				}
			}
	    
	    // Try again
	    u2 = ULong2Unit_ZO( iFunc(iData) );
	    u3 = ULong2Unit_ZO( iFunc(iData) );
		} while (true);
	
	else if (u1 < 4117674588UL) do {					// Region 2
		double sign = 1.0;
			
		if (u3 < u2) {
			// Swap random deviates, generate negative variate
			double temp = u2;
			u2 = u3;
			u3 = temp;
			sign = -1.0;
			}										// ASSERT (u2 < u3)
	   
		result = 0.4797274042 + 1.1054736610 * u2;
		if(u3 <= 0.8728349767
				|| 0.0492644964 * (u3-u2) <= NormKRHelper(result, A)) {
		    result *= sign;
		    break;
		    }
	    
	    // Try again
		u2 = ULong2Unit_ZO( iFunc(iData) );
		u3 = ULong2Unit_ZO( iFunc(iData) );
    	} while (true);		
	
	else if (u1 < 4180338716UL) do {					// Region 3
		double sign = 1.0;
			
		if (u3 < u2) {
			// Swap random deviates, generate negative variate
			double temp = u2;
			u2 = u3;
			u3 = temp;
			sign = -1.0;
			}										// ASSERT (u2 < u3)
		
		result = A - 0.6308348019 * u2;
		if (u3 <= 0.7555915317
				|| 0.0342405038 * (u3 - u2) <= NormKRHelper(result, A)) {
		    result *= sign;
		    break;
		    }
	    
	    // Try again
		u2 = ULong2Unit_ZO( iFunc(iData) );
		u3 = ULong2Unit_ZO( iFunc(iData) );
		} while (true);
	
	else do {										// Tail
		result  = -log(u3);
		result += result;
		result += A * A;
		// ASSERT (result == A * A - 2 * log(u3))
		
		if (result * u2 * u2 < A * A ) {				// We have a variate
			result = sqrt(result);
			if (u1 & 0x01)								// Toss a coin to determine sign
		    	result = -result;
		    break;
		    }
	    
	    // Try again
		u2 = ULong2Unit_ZO( iFunc(iData) );
		u3 = ULong2Unit_ZO( iFunc(iData) );
		} while (true);
		
	return result;
	}
	

double
NormalKRTaus88(
	tTaus88Data*	iData)
	
	{
    const double A = 2.2160358672;
    
	UInt32	u1, s1, s2, s3;
	double	u2,
			u3,
			result;
	
	if (iData == NIL)
		iData = Taus88GetGlobals();
	Taus88Load(iData, &s1, &s2, &s3);
	
	u1 = Taus88Process(&s1, &s2, &s3);
	u2 = ULong2Unit_ZO( Taus88Process(&s1, &s2, &s3) );
	u3 = ULong2Unit_ZO( Taus88Process(&s1, &s2, &s3) );

	
	if (u1 < 3797053464UL)							// Main body of standard distribution
	    result = A * (u2 + u3 - 1.0);
	
	else if (u1 < 3914058587UL) do {					// Region 1
		double sign = 1.0;
			
		if (u3 < u2) {
			// Swap random deviates, generate negative variate
			double temp = u2;
			u2 = u3;
			u3 = temp;
			sign = -1.0;
			}										// ASSERT (u2 < u3)
	   
	    result = 0.4797274042 - 0.5955071380 * u2;
	    if (result >= 0.0) {						// Potential variate
		    if (u3 <= 0.8055779244
		    		|| 0.0533775495 * (u3-u2) <= NormKRHelper(result, A)) {
				result *= sign;						// We have a variate
				break;
				}
			}
	    
	    // Try again
	    u2 = ULong2Unit_ZO( Taus88Process(&s1, &s2, &s3) );
	    u3 = ULong2Unit_ZO( Taus88Process(&s1, &s2, &s3) );
		} while (true);
	
	else if (u1 < 4117674588UL) do {					// Region 2
		double sign = 1.0;
			
		if (u3 < u2) {
			// Swap random deviates, generate negative variate
			double temp = u2;
			u2 = u3;
			u3 = temp;
			sign = -1.0;
			}										// ASSERT (u2 < u3)
	   
		result = 0.4797274042 + 1.1054736610 * u2;
		if(u3 <= 0.8728349767
				|| 0.0492644964 * (u3-u2) <= NormKRHelper(result, A)) {
		    result *= sign;
		    break;
		    }
	    
	    // Try again
		u2 = ULong2Unit_ZO( Taus88Process(&s1, &s2, &s3) );
		u3 = ULong2Unit_ZO( Taus88Process(&s1, &s2, &s3) );
    	} while (true);		
	
	else if (u1 < 4180338716UL) do {					// Region 3
		double sign = 1.0;
			
		if (u3 < u2) {
			// Swap random deviates, generate negative variate
			double temp = u2;
			u2 = u3;
			u3 = temp;
			sign = -1.0;
			}										// ASSERT (u2 < u3)
		
		result = A - 0.6308348019 * u2;
		if (u3 <= 0.7555915317
				|| 0.0342405038 * (u3 - u2) <= NormKRHelper(result, A)) {
		    result *= sign;
		    break;
		    }
	    
	    // Try again
		u2 = ULong2Unit_ZO( Taus88Process(&s1, &s2, &s3) );
		u3 = ULong2Unit_ZO( Taus88Process(&s1, &s2, &s3) );
		} while (true);
	
	else do {										// Tail
		result  = -log(u3);
		result += result;
		result += A * A;
		// ASSERT (result == A * A - 2 * log(u3))
		
		if (result * u2 * u2 < A * A ) {				// We have a variate
			result = sqrt(result);
			if (u1 & 0x01)								// Toss a coin to determine sign
		    	result = -result;
		    break;
		    }
	    
	    // Try again
		u2 = ULong2Unit_ZO( Taus88Process(&s1, &s2, &s3) );
		u3 = ULong2Unit_ZO( Taus88Process(&s1, &s2, &s3) );
		} while (true);
	
	Taus88Store(iData, s1, s2, s3);
	
	return result;
	}
	

