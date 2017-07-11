/*
	File:		UniformExpectatsions.c

	Contains:	Utility used by objects generating (nominally) uniform random deviates:
				titi, tata, mama, mrmr, and lili.

	Written by:	Peter Castine

	Copyright:	© 2006 Peter Castine

	Change History (most recent first):

         <1>     26–2–06    pc      first checked in.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "UniformExpectations.h"

//#include <math.h>				// For log(), cos(), etc.

#ifdef _WINDOWS_
	#include "Folders.h"		// For FindFolder(), used in OpenPrefsFile()
	#include <ShLwApi.h>
#endif


#pragma mark • Constants

	


#pragma mark • Type Definitions



#pragma mark • Static (Private) Variables



#pragma mark • Function Prototypes



#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

/******************************************************************************************
 *
 *	UniformExpectationsDiscrete(iMin, iMax, iSel)
 *	UniformExpectationsContinuous(iMin, iMax, iSel)
 *
 *	Returns the expected value of the specified property
 *
 ******************************************************************************************/

double
UniformExpectationsDiscrete(
	long			iMin,
	long			iMax,
	eExpectSelector	iSel)
	
	{
	double	result	= 0.0 / 0.0;								// Initially undefined
	
	switch(iSel) {
	case expMean:
	case expMedian:
		result  = ((double) iMin) + ((double) iMax);
		result *= 0.5;
		break;
	case expMode:												// Undefined	
		break;
	case expVar:
	case expStdDev:
		result  = ((double) iMax) - ((double) iMin) + 1.0;
		result *= result;
		result -= 1.0;
		result *= 1.0 / 12.0;
		if (iSel == expStdDev)
			result = sqrt(result);
		break;
	case expSkew:
		result  = 0.0;
		break;
	case expKurtosis:
		result  = ((double) iMax) - ((double) iMin) + 1.0;
		if (result < 4096.0) {
			result *= result;
			result  = (result + 1.0) / (result - 1.0);
			result *= -1.2;
			}
		else result = -1.2;						// Max can't display the difference between
		break;									// this and a higher precision result
	case expMin:
		result = iMin;
		break;
	case expMax:
		result = iMax;
		break;
	case expEntropy:
		result  = ((double) iMax) - ((double) iMin) + 1.0;
		result = log2(result);
		break;
	default:
		break;
		}
		
	return result;
	}


double
UniformExpectationsContinuous(
	double			iMin,
	double			iMax,
	eExpectSelector	iSel)
	
	{
	double	result	= 0.0 / 0.0;				// Initially undefined
	
	switch(iSel) {
	case expMean:
	case expMedian:
		result = 0.5 * (iMin + iMax);
		break;
	case expMode:								// Undefined
		break;
	case expVar:
	case expStdDev:
		result  = iMax - iMin;
		result *= result;
		result *= 1.0 / 12.0;
		if (iSel == expStdDev)
			result = sqrt(result);
		break;
	case expSkew:
		result  = 0.0;
		break;
	case expKurtosis:
		result = -1.2;
		break;
	case expMin:
		result = iMin;
		break;
	case expMax:
		result = iMax;
		break;
	case expEntropy:
		result = log2(iMax - iMin);
		break;
	default:
		break;
		}
		
	return result;
	}

