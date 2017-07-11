/*
	File:		Taus88.c

	Contains:	Implementation of L'Ecuyer's Taus88 algorithm.

	Written by:	Peter Castine

	Copyright:	© 2001-2006 Peter Castine

	Change History (most recent first):

         <3>   24–3–2006    pc      Fix compile problems with the signal vector versions we hadn't
                                    dealt with yet.
         <2>   23–3–2006    pc      Expose "global" seed set. The data must only be used in
                                    conjunction with the inline Taus88 calls. Modified for new
                                    LitterLib organization.
         <1>     26–2–06    pc      First checked in. Split off  Taus88 functions that had collected
                                    in LitterLib.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "Taus88.h"
#include "LitterLib.h"
#include "MiscUtils.h"


#pragma mark • Constants

	


#pragma mark • Type Definitions


#pragma mark • Global Variables

		// The static initialization below should be overwritten by the calling functions,
		// but I'm wary of leaving this stuff initialized to 0s.
tTaus88Data gTausData = {0x4a1fcf79, 0xb86271cc, 0x6c986d11};

#pragma mark • Initialize Global Variables


#pragma mark • Function Prototypes

#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark • Private & Inline Functions



#pragma mark -
#pragma mark • Public Functions

/******************************************************************************************
 *
 *	Taus88(iData)
 *	Taus88SigVector(iCount, oVector)
 *	Taus88TriSig()
 *	
 *	Generate a random 32-bit value using the Tausworthe 88 algorithm. Pass a pointer to a
 *	tTaus88Data structure to maintain a private seed pool or pass NULL to use Taus88's
 *	global seed pool.
 *	
 *	The Taus88SigVector variant fills an entire vector with data, caching seed data in
 *	registers for more efficient processing. The iCount parameter specifies the number of
 *	values and oVector points to the data storage area. Tuas88SigVector always uses the
 *	default pool of seeds.
 *	
 *	The Taus88TriSig returns a triangular (dithering) signal in the range -1 ≤ x < 1.
 *	
 ******************************************************************************************/

UInt32
Taus88(
	tTaus88DataPtr	iData)
	
	{
	UInt32	x, s1, s2, s3;
	
	if (iData == NULL)
		iData = &gTausData;
	
	// Model sequence for calling the three core inlined functions defined in LitterLib.h
	Taus88Load(iData, &s1, &s2, &s3);
	
	x = Taus88Process(&s1, &s2, &s3);
	
	Taus88Store(iData, s1, s2, s3);
	
	return x; 
	}

	


#ifdef __MAX_MSP_OBJECT__
	void
	Taus88SigVector(
		tSampleVector	ioVector,
		UInt32			iCount)
		
		{
		UInt32			s1, s2, s3;
		
		// Get seed data into registers
		Taus88Load(&gTausData, &s1, &s2, &s3);
		
		while (iCount-- > 0)
			*ioVector++ = ULong2Signal( Taus88Process(&s1, &s2, &s3) );
		
		// Save seed data
		Taus88Store(&gTausData, s1, s2, s3);
		
		}

	void
	Taus88SigVectorMasked(
		tSampleVector	ioVector,
		UInt32			iCount,
		UInt32			iMask,
		UInt32			iOffset)
		
		{
		UInt32	s1, s2, s3;
		
		// Get seed data into registers
		Taus88Load(&gTausData, &s1, &s2, &s3);
		
		while (iCount-- > 0)
			*ioVector++ = ULong2Signal((Taus88Process(&s1, &s2, &s3) & iMask) + iOffset);
		
		// Save seed data
		Taus88Store(&gTausData, s1, s2, s3);
		
		}

	float
	Taus88TriSig(void)
		
		{
		UInt32	s1, s2, s3, t;
		
		// Get seed data into registers
		Taus88Load(&gTausData, &s1, &s2, &s3);
		
		// Calculate first value, store it in fraction field (23 least significant bits)
		// of what will become our result, then add second value
		t  = Taus88Process(&s1, &s2, &s3) >> 10;
		t += Taus88Process(&s1, &s2, &s3) >> 10;
		
		// Save seed data
		Taus88Store(&gTausData, s1, s2, s3);
		
		// Munge t to look like a float in the range 2 ≤ t < 4
		t  |= 0x40000000;
		
		// Typecast and move t into the range -1 ≤ t < 1
		return *((float*) &t) - 3.0f; 
		}
#endif		// __MAX_MSP_OBJECT__


/******************************************************************************************
 *
 *	Taus88Init(iMagic)
 *	
 *	Use Taus88Init() to generate a random starting state for Taus88's global seed pool.
 *	This should be called exactly once (multiple calls are harmless, the function will
 *	return without doing anything after the first call).
 *	
 ******************************************************************************************/

void
Taus88Init(void)
	
	{
	static Boolean sTaus88Inited = false;

	if (!sTaus88Inited) {
		Taus88Seed(NULL, 0);
		sTaus88Inited = true;
		}
	
	}
	

/******************************************************************************************
 *
 *	Taus88Seed(iData, iSeed)
 *	
 *	Use Taus88Seed() to seed a private data pool for the Taus88 generator. If the
 *	iSeed parameter is 0, a random seed will be generated based on "machine kharma" (a
 *	combination of current date and time, machine up time, and an auto seed counter. If
 *	iData is NULL, the global seed pool is re-initialized.
 *	
 *	4/5-July-2003: Changed the seeding strategy. We had been using our seed to set
 *	qd.randSeed and then using several calls to the Toolbox Random() function to generate
 *	seed values. This stopped working, probably with Max 4 (although we didn't notice it
 *	immediately). Now we use our "seed" value as a kind of "salt", XOR-ing default seed
 *	values to generate new seeds.  
 *
 *	18 Jan 2006: Improve seeding. In previous version adjacent input seeds could produce
 *	identical seed sets.
 *	
 ******************************************************************************************/

	static inline unsigned long AddPepper(unsigned long n)
		{ n = (n >> 16) * (n & 0x0000ffff); return (n == 0) ? 0xfedcfedc : n; }

void
Taus88Seed(
	tTaus88DataPtr	iData,
	UInt32          iSalt)
	
	{
	const unsigned long kSeed1 = 0x4a1fcf79,
						kSeed2 = 0xb86271cc,
						kSeed3 = 0x6c986d11;
	
	unsigned long	s;
	if (iData == NULL)
		iData = &gTausData;
	
	if (iSalt == 0) {
		// Generate a new seed based on date, time, etc.
		iSalt = MachineKharma();
		// Spice with the address where our data is stored.
		// This gives us a value unique to the calling Max class
		iSalt ^= (unsigned long) iData;
		}
	
	// (Re-)initialize data pool, making sure seed requirements are met.
	s = kSeed1 ^ iSalt ^ (iSalt << 1);
	iData->seed1 = (s >= 2) ? s : kSeed1;
		
	iSalt ^= AddPepper(iSalt);
	s = kSeed2 ^ iSalt ^ (iSalt << 3);
	iData->seed2 = (s >= 8) ? s : kSeed2;
	
	iSalt ^= AddPepper(iSalt);
	s = kSeed3 ^ iSalt ^ (iSalt << 4);
	iData->seed3 = (s >= 16) ? s : kSeed3;
	
	}

/******************************************************************************************
 *
 *	Taus88New(iSalt)
 *	Taus88Free()
 *	
 *	Allocate/deallocate memory for a new pool of seeds. Allocation allows caller to set
 *	a seed.
 *	
 ******************************************************************************************/

tTaus88DataPtr
Taus88New(
	UInt32	iSalt)
	
	{
	tTaus88DataPtr newTaus88 = (tTaus88DataPtr) NewPtr(sizeof(tTaus88Data));
	
	if (newTaus88 != NULL)
		Taus88Seed(newTaus88, iSalt);
	
	// If newTaus88 is NULL, this is sad but mostly harmless. The caller will pass NULL
	// as data and we'll use the global seed pool. 
	return newTaus88;
	}

void Taus88Free(tTaus88DataPtr iTaus)
	{ if (iTaus != NIL) DisposePtr((Ptr) iTaus); }

