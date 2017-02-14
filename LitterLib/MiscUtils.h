/*
	File:		MiscUtils.h

	Contains:	Header file for MiscUtils.

	Written by:	Peter Castine

	Copyright:	© 2001-08 Peter Castine

	Change History (most recent first):

         <3>   30Ð3Ð2006    pc      Put Unit2Exponential() and Unit2Laplace() in here.
         <2>   23Ð3Ð2006    pc      Need to #include ext_types for Windows.
         <1>     26Ð2Ð06    pc      First checked in. Split off  miscellaneous functions for
                                    bit-munging, etc., that had collected in LitterLib.
*/

/******************************************************************************************
 ******************************************************************************************/

#pragma once
#ifndef __MISCUTILS_H__
#define __MISCUTILS_H__

#pragma mark ¥ Include Files

#include <math.h>							// for declaration of log()

#ifndef __MACTYPES__
//	#include <MacTypes.h>					// for UInt32
#endif

#pragma mark ¥ Constants

#ifndef kULongMax
	#define kULongMax 0xffffffff
#endif

#ifndef kLongMax
	#define kLongMax 0x7fffffff
#endif

#pragma mark ¥ Type Definitions


typedef UInt32	(*tRandomFunc)(void*);


#pragma mark ¥ Global Variables


#pragma mark ¥ Function Prototypes

	
UInt32					MachineKharma	(void);
UInt32					HashString		(ConstStr255Param);
static inline UInt32	RotateBits		(UInt32, int);


	// General purpose transformations	
static inline double	Unit2Exponential(double);					// Convert Uniform distrib.
static inline double	Unit2Laplace	(double);					// defined for 0 < iVal <= 1

	
	// ...and conversion utilities
static inline double	ULong2Unit_zo	(UInt32);
static inline double	ULong2Unit_Zo	(UInt32);
static inline double	ULong2Unit_zO	(UInt32);
static inline double	ULong2Unit_ZO	(UInt32);
static inline float		ULong2Signal	(UInt32);				// ! returns a simple float !
static inline float		Long2Signal		(long);




#pragma mark -
#pragma mark ¥ Inline Functions & Macros

	
	// This should probably be moved to MaxUtils
	// Positive values rotate left, negative values rotate right
	inline UInt32 RotateBits(UInt32 x, int n)
		{ n &= 0x001f; return (x << n) | (x >> (32-n)); } 

	// ULong2xxx() functions
	// These scaling functions were designed for mapping 32 bit integers from random
	// number generators such as Taus88, TT800, and any other generator that produces
	// values over the entire range of unsigned 32 bit values. 
	// 
	// With the ULong2Unit_xx functions, the inclusion of zero or one in the range
	// is indiciated by a capital initial (z or o) appended to the function name; lowercase
	// initials indicate that the range is open at the end indicated by the letter.

inline double	ULong2Unit_zo(UInt32 iVal)			// Return range: 0 < x < 1
					{
					const double kScale = 1.0 / ((double) kULongMax + 2.0);
					
					return kScale * (1.0 + iVal);
					}

inline double	ULong2Unit_Zo(UInt32 iVal)			// Return range: 0 <= x < 1
					{
					const double kScale = 1.0 / ((double) kULongMax + 1.0);
					
					return kScale * iVal;
					}


inline double	ULong2Unit_zO(UInt32 iVal)			// Return range: 0 < x <= 1
					{
					const double kScale = 1.0 / ((double) kULongMax + 1.0);
					
					return kScale * (1.0 + iVal);
					}

inline double	ULong2Unit_ZO(UInt32 iVal)			// Return range: 0 <= x <= 1
					{
					const double kScale = 1.0 / (double) kULongMax;
					
					return kScale * iVal;
					}

inline float	ULong2Signal(UInt32 iVal)			// Return range: -1 <= x < 1
					{
					// This may not to be as highly optimized as it appears
					// ?? !! Investigate !! ??
					iVal >>= 9;
					iVal  |= 0x40000000;
					
					return *((float*) &iVal) - 3.0f; 
					}

inline float	Long2Signal(long iVal)						// Return range: -1 <= x < 1
					{
					const double kScale = 1.0 / ((double) kLongMax + 1.0);
					
					return kScale * iVal;
					}

inline double	Unit2Exponential(double iVal)
					{ return -log(iVal); }
inline double	Unit2Laplace	(double iVal)
					{ iVal += iVal; return (iVal <= 1.0) ? -log(iVal) : log(2.0 - iVal); }
	

	// Range correction utils
static inline long ClipLong(long x, long min, long max)
	{ return (x < min) ? min : (x > max) ? max : x; }
static inline double ClipDouble(double x, double min, double max)
	{ return (x < min) ? min : (x > max) ? max : x; }
static inline double ClipUnit(double x)
	{ return ClipDouble(x, 0.0, 1.0); }
static inline float ClipSignal(float x)
	{ return (x < -1.0f) ? -1.0f : (x > 1.0f) ? 1.0f : x; }
	
	// WrapLongRange() is more efficient than WrapLong() if the extent (max-min) is a
	// loop invariant. Ditto for the double versions
static inline long WrapLongRange(long x, long min, long extent)
	{ x = (x - min) % extent; if (x < 0) x += extent; return x + min; }
static inline long WrapLong(long x, long min, long max)
	{ return WrapLongRange(x, min, max - min); }
static inline double WrapDoubleRange(double x, double min, double extent)
	{ x = fmod(x-min, extent); if (x < 0.0) x += extent; return x + min; }
static inline double WrapDouble(double x, double min, double max)
	{ return WrapDoubleRange(x, min, max - min); }
static inline double WrapUnit(double x)
	{ x = fmod(x, 1.0); return (x < 0.0) ? x += 1.0 : x;  }
static inline float WrapSignal(float x)
	{ x = fmodf(x + 1.0, 2.0); return (x < 0.0) ? x + 1.0 : x - 1.0; }


	// ReflectLongRange2() is more efficient than ReflectLongRange(), which is more
	// efficient than ReflectLong(), assuming that extent (max-min) or extent2 (2*extent)
	// can be precalculated, for instance as a loop invariant. The same holds for the
	// double versions 
static inline long ReflectLongRange2(long x, long min, long extent, long extent2)
	{
	x = x % extent2;
	if (x < 0) x += extent2;
	if (x > extent) x = extent2 - x;
	return x + min;
	}
static inline long ReflectLongRange(long x, long min, long extent)
	{ return ReflectLongRange2(x, min, extent, extent + extent); }
static inline long ReflectLong(long x, long min, long max)
	{ return ReflectLongRange(x, min, max - min); }

static inline double ReflectDoubleRange2(double x, double min, double extent, double extent2)
	{
	x = fmod(x, extent2);
	if (x < 0.0) x += extent2;
	if (x > extent) x = extent2 - x;
	return x + min;
	}
static inline double ReflectDoubleRange(double x, double min, double extent)
	{ return ReflectDoubleRange2(x, min, extent, extent + extent); }
static inline double ReflectDouble(double x, double min, double max)
	{ return ReflectDoubleRange(x, min, max - min); }

static inline double ReflectUnit(double x)
	{
	x = fmod(x, 2.0);
	if (x < 0.0) x += 2.0;
	
	return (x > 1.0) ?  2.0 - x : x;
	}
static inline float ReflectSignal(float x)
	{
	x = fmodf(x + 1.0, 4.0f);
	if (x < 0.0f) x += 4.0f;
	
	return (x > 2.0f) ?  3.0f - x : x - 1.0;
	}
	

#endif														// __MISCUTILS_H__