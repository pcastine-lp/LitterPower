/*
	File:		TT800.c

	Contains:	Library functions for use by the Litter Power Package.

	Written by:	Peter Castine

	Copyright:	© 2001-2002 Peter Castine

	Change History (most recent first):

         <1>     26–2–06    pc      First checked in. Split off  TT800 RNG from LitterLib.c.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "TT800.h"
#include "MiscUtils.h"				// For MachineKharma()


#pragma mark • Constants	


#pragma mark • Type Definitions


#pragma mark • Static (Private) Variables

static tTT800Data
				sTT800Data		= {
					// Matsumoto's seeds, but these are arbitrary
					// These should be explicitly initialized at run time, but
					// it doesn't hurt to have static initialization code...
					{0x95f24dab, 0x0b685215, 0xe76ccae7, 0xaf3ec239, 0x715fad23,
					 0x24a590ad, 0x69e4b5ef, 0xbf456141, 0x96bc1b7b, 0xa7bdf825,
					 0xc1de75b7, 0x8858a9c9, 0x2da87693, 0xb657f9dd, 0xffdc8a9f,
					 0x8121da71, 0x8b823ecb, 0x885d05f5, 0x4e20cd47, 0x5a9ad5d9,
					 0x512c0c03, 0xea857ccd, 0x4cc1d30f, 0x8891a8a1, 0xa6b7aadb},
					0 };


#pragma mark • Initialize Global Variables


#pragma mark • Function Prototypes

#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark • TT800 Functions

/******************************************************************************************
 *
 *	TT800(iData)
 *
 *	Matsumoto's TT800 algorithm, with a few minor speed optimizations.
 *
 ******************************************************************************************/

unsigned long
TT800(
	tTT800DataPtr	iData)
	
	{
	// The following values are magic that TT800 depends on, don't change.
	const UInt32	kMagicA[]			= {0x0, 0x8ebfd028},
	    			kMagicB				= 0x2b5b2500,
	    			kMagicC				= 0xdb8b0000;
		// The following  be less than kTT800SeedArraySize. There may be other
		// requirements but if 7 is good enough for Matsumoto, it's good enough for us!
	const int		kTT800SeedArrayOffset	= 7;	
	
	UInt32	y;
	
	if (iData == NULL)
		iData = &sTT800Data;
	
	if (iData->curSeed >= kTT800SeedArraySize) {
		// Generate N new random values at one time
    	unsigned long*	curSeed	= iData->seeds;
    	unsigned long*	offSet	= curSeed + kTT800SeedArrayOffset;
    	int				i;
		
		i = kTT800SeedArraySize - kTT800SeedArrayOffset;
		while (i-- > 0) {
			*curSeed = *offSet++ ^ (*curSeed >> 1) ^ kMagicA[*curSeed & 1];
			curSeed += 1;
			}
		*offSet -= kTT800SeedArraySize;		// ASSERT: offSet == me->seedArray
		
		i = kTT800SeedArrayOffset;
		while (i-- > 0) {
			*curSeed = *offSet++ ^ (*curSeed >> 1) ^ kMagicA[*curSeed & 1];
			curSeed += 1;
			}
		
		iData->curSeed = 0;
		}
    
    y = iData->seeds[iData->curSeed++];
    y ^= (y << 7) & kMagicB;
    y ^= (y << 15) & kMagicC;
    
	return y ^ (y >> 16);
	}


/******************************************************************************************
 *
 *	TT800Init()
 *	
 *	Seed TT800's private data pool. Can be called multiple times without harm (the function
 *	only actually does something on the first call). Use TT800Seed() with NULL if you want
 *	to modify the seed pool contents later.
 *	
 ******************************************************************************************/

void
TT800Init(void)
	
	{
	static Boolean sTT800Inited	= false;

	
	if (!sTT800Inited) {
		TT800Seed(NULL, 0);
		sTT800Inited = true;
		}
	}


/******************************************************************************************
 *
 *	TT800Seed(iData, iSalt)
 *	
 *	Seed TT800 data. If iSeed is 0, generate unique, random seed data.
 *
 *	NB: This function MUST be called before using a tTT800Data structure!
 *	
 *	See comments to Taus88Seed on seeding strategy. The new strategy makes the TT800 code
 *	a lot larger than I had expected, but this is not a serious problem.
 *	
 ******************************************************************************************/

void
TT800Seed(
	tTT800DataPtr	iData,
	UInt32			iSalt)
	
	{
	const UInt32 kDefSeeds[] = {
					// Matsumoto's seeds, but these are arbitrary
					0x95f24dab, 0x0b685215, 0xe76ccae7, 0xaf3ec239, 0x715fad23,
					0x24a590ad, 0x69e4b5ef, 0xbf456141, 0x96bc1b7b, 0xa7bdf825,
					0xc1de75b7, 0x8858a9c9, 0x2da87693, 0xb657f9dd, 0xffdc8a9f,
					0x8121da71, 0x8b823ecb, 0x885d05f5, 0x4e20cd47, 0x5a9ad5d9,
					0x512c0c03, 0xea857ccd, 0x4cc1d30f, 0x8891a8a1, 0xa6b7aadb};
	
	int	i;

	if (iData == NULL)
		iData = &sTT800Data;
	
	if (iSalt == 0) {
		iSalt =  MachineKharma();
		iSalt ^= (unsigned long) iData;
		}
	
	for (i = 0; i < kTT800SeedArraySize; i += 1) {
		UInt32 newSeed = kDefSeeds[i] ^ iSalt;
		iData->seeds[i] = newSeed;
		iSalt = RotateBits(iSalt, (newSeed & 0x01) ? i + 1 : 31 - i);
		}
	
	iData->curSeed = 0;
	
	}


/******************************************************************************************
 *
 *	TT800New(iSeed)
 *	
 *	Creates a new, independent pool of seeds for use by the TT800 functions.
 *	
 ******************************************************************************************/

tTT800DataPtr
TT800New(
	unsigned long iSeed)
	
	{
	tTT800DataPtr newTT800 = (tTT800DataPtr) getbytes(sizeof(tTT800Data));
		
	if (newTT800 != NULL)
		TT800Seed(newTT800, iSeed);
	
	// If newTT800 is NULL, this is sad but mostly harmless. The caller will pass NULL
	// as data and we'll use the global seed pool. 
	return newTT800;
	}


