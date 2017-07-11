/*
	File:		RNGCauchy.c

	Contains:	<contents>

	Written by:	Peter Castine

	Copyright:	<copyright>

	Change History (most recent first):

         <1>   30–3–2006    pc      Initial check in.
*/


/*
	File:		RNGCauchy.c

	Contains:	Generate random variates from a Cauchy distribution.

	Written by:	Peter Castine

	Copyright:	© 2006 Peter Castine

	Change History (most recent first):

*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "RNGCauchy.h"



#pragma mark • Constants

	// Constants 
const double	kAD1	= 0.6380631366077803,
				kAD2	= 0.9339962957603656,
				kAD3	= 0.6366197723675813,
				kAD4	= 0.0214949004570452,
				kAD5	= 0.5959486060529070,
				kAD6	= 0.2488702280083841,
				kAD7	= 0.5972997593539963,
				kAD8	= 4.9125013953033204;


#pragma mark • Type Definitions


#pragma mark • Static Variables


#pragma mark • Global Variables



#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark • Functions


/******************************************************************************************
 *
 *	GenCauchyStd(iFunc, iData)
 *	GenCauchyStdTaus88(iData)
 *
 *	After Ahrens and Dieter (1988)
 *
 ******************************************************************************************/

double
GenCauchyStd(
	tRandomFunc		iFunc,
	void*			iData)
	
	{
	double	t		= ULong2Unit_zo( iFunc(iData) ) - 0.5,
			thresh	= kAD6 - t * t,
			cauchy;
	
	// Quick acceptance condition, works 99.77% of the time
	if (thresh > 0)
		cauchy = t * (kAD1 / thresh + kAD7);
	
	else do {
		// Gird our loins and do this the hard way
		double	u = ULong2Unit_zo( iFunc(iData) ),
				temp;
		
		t = ULong2Unit_zo( iFunc(iData) ) - 0.5;
		
		// Calculate candidiate variate
		// use thresh as temporary register (we need the value later!)
		thresh  = 0.25 - t * t;
		cauchy  = t * (kAD1 / thresh + kAD5);
		
		// Calculate acceptance threshhold
		temp    = cauchy * cauchy;
		temp   += 1.0;
		temp   *= kAD4 * u + kAD8;
		temp   -= kAD2;
		temp   *= thresh;
		temp   *= thresh;
		thresh += temp;
		} while (thresh > 0.5);
	
	return cauchy;
	}
	
	
double
GenCauchyStdTaus88(
	tTaus88Data* iData)
	
	{
	// This initial candidate variate is accepted so often we don't bother with
	// getting Taus88 static variables into local registers. Yet.
	double	t		= ULong2Unit_zo( Taus88(iData) ) - 0.5,
			thresh	= kAD6 - t * t,
			cauchy;
	
	// Quick acceptance condition, works 99.77% of the time
	if (thresh > 0)
		cauchy = t * (kAD1 / thresh + kAD7);
	
	else {
		// In this case we're likely to need a few random variates, so get Taus88 data
		// into local registers
		UInt32 s1, s2, s3;
		
		if (iData == NIL)
			iData = Taus88GetGlobals();
		Taus88Load(iData, &s1, &s2, &s3);
		
		do  {
			double	u = ULong2Unit_zo( Taus88Process(&s1, &s2, &s3) ),
					temp;
			
			t = ULong2Unit_zo( Taus88Process(&s1, &s2, &s3) ) - 0.5;
			
			// Calculate candidiate variate
			// use thresh as temporary register (we need the value later!)
			thresh  = 0.25 - t * t;
			cauchy  = t * (kAD1 / thresh + kAD5);
			
			// Calculate acceptance threshhold
			temp    = cauchy * cauchy;
			temp   += 1.0;
			temp   *= kAD4 * u + kAD8;
			temp   -= kAD2;
			temp   *= thresh;
			temp   *= thresh;
			thresh += temp;
			} while (thresh > 0.5);
		
		Taus88Store(iData, s1, s2, s3);
		}
	
	return cauchy;
	}

/******************************************************************************************
 *
 *	GenCauchyPos(iFunc, iData)
 *	GenCauchyPosTaus88(iData)
 *
 *	After Dagpunar (1988)
 *
 ******************************************************************************************/

	// Static variable used by both GenCauchy() and GenCauchyPosTaus88()
static UInt32	sInvBits = 0,
	 			sCounter = 0;

double
GenCauchyPos(
	tRandomFunc iFunc,
	void*		iData)
	
	{
	double	u1, u2, cauchy;
	
	// Check if any bits are left for deciding whether how to handle the "Y"-variate
	if (sCounter == 0) {
		sInvBits = iFunc(iData);
		sCounter = 32;
		}
	
	// Find candidate "Y"-variate
	do  {
		u1	= ULong2Unit_zo( iFunc(iData) ),
		u2	= ULong2Unit_zo( iFunc(iData) );
		} while (u2 * (u1 * u1 + 1.0) >= 1.0);
	
	// Decide what to do with the "Y"-variate
	cauchy = (sInvBits & 0x01) ? u1 : 1.0 / u1;
	
	// Shift bits, keep track of how many are left
	sInvBits >>= 1;
	sCounter  -= 1;
	
	// All done
	return cauchy;
	}


double
GenCauchyPosTaus88(
	tTaus88Data* iData)
	
	{
	double	u1, u2, cauchy;
	UInt32	s1, s2, s3;
	
	if (iData == NIL)
		iData = Taus88GetGlobals();
	Taus88Load(iData, &s1, &s2, &s3);
	
	// Check if any bits are left for deciding whether how to handle the "Y"-variate
	if (sCounter == 0) {
		sInvBits = Taus88Process(&s1, &s2, &s3);
		sCounter = 32;
		}
	
	// Find candidate "Y"-variate
	do  {
		u1	= ULong2Unit_zo( Taus88Process(&s1, &s2, &s3) ),
		u2	= ULong2Unit_zo( Taus88Process(&s1, &s2, &s3) );
		} while (u2 * (u1 * u1 + 1.0) >= 1.0);
	
	Taus88Store(iData, s1, s2, s3);
	
	// Decide what to do with the "Y"-variate
	cauchy = (sInvBits & 0x01) ? u1 : 1.0 / u1;
	
	// Shift bits, keep track of how many are left
	sInvBits >>= 1;
	sCounter  -= 1;
	
	// All done
	return cauchy;
	}


