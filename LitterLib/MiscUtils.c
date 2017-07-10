/*
	File:		MiscUtils.c

	Contains:	Utility functions that don't fit anywhere else.

	Written by:	Peter Castine

	Copyright:	© 2001-08 Peter Castine

	Change History (most recent first):

         <2>   23–3–2006    pc      Need to add #includes for Windows. Added some high-precision
                                    math constants
         <1>     26–2–06    pc      First checked in. Split off  miscellaneous functions for
                                    bit-munging, etc., that had collected in LitterLib.
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "MiscUtils.h"
#include "MaxUtils.h"				// This #includes some stuff we need for Windows


#pragma mark • Constants



#pragma mark • Type Definitions



#pragma mark • Static (Private) Variables



#pragma mark • Initialize Global Variables



#pragma mark • Function Prototypes

#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark • Private & Inline Functions

/******************************************************************************************
 *
 *	LongRandom()
 *	
 *	Calls the OS Random() function several times to generate a pseudo-random long value.
 *	We use this to bootstrap a set of seed values for TT800. 
 *	
 ******************************************************************************************/

static unsigned long
LongRandom()
	
	{
	// Overlap values from multiple calls to Random() to alleviate the problem of the
	// low bits not being very random.
	return (((unsigned long) Random()) << 16)
			^ (((unsigned long) Random()) << 5)
			^ (((unsigned long) Random()) >> 5);
	}

/******************************************************************************************
 *
 *	HashString()
 *	
 *	Generate an unsigned long integer from a string.
 *	
 ******************************************************************************************/

UInt32
HashString(
	ConstStr255Param iStr)
	
	{
	UInt32	result	= 0;
	BytePtr	curChar	= (BytePtr) iStr;
	int		i		= iStr[0],
			shift	= (i < 32) ? 27 : 24;
	
	while (i-- > 0) {
		result ^= (*curChar++ << shift);
		shift -= 7;
		if (shift < 0) {
			shift += 25;
			}
		}
	
	return result;
	}


#pragma mark -
#pragma mark • OS Utilities

/******************************************************************************************
 *
 *	MachineKharma()
 *	
 *	Generates a vaguely random number based on System Date and Uptime. 
 *	
 ******************************************************************************************/
 
	static UInt32 ProcessIDMagic(void)
		{
		OSStatus			myErr	= noErr;
		ProcessSerialNumber procID	= {kNoProcess, kNoProcess};
		UInt32				result  = (UInt32) &procID;
		int					bitShift = 0;

		while (true) {
			myErr = GetNextProcess(&procID);
			
			if (myErr == noErr) {
				UInt32 procIDLo = procID.lowLongOfPSN;
			
				if (bitShift > (32 - CountBits(procIDLo)))
					bitShift = 0;
				result ^= (procIDLo << bitShift++);
				}
			else break;
			}
		
		return result;
		}
 
	#include <uuid/uuid.h>
	static UInt32 UUIDMagic(void)
		{
		uuid_t	uuid;
		UInt32*	uuidl = (UInt32*) uuid;
		
		uuid_generate(uuid);
		
		return uuidl[0] ^ uuidl[1] ^ uuidl[2] ^ uuidl[3];
		}

UInt32
MachineKharma()
	{
	static int sAutoSeedCount = 0;
	
	UInt32	kharma = evnum_get();
	
	if (kharma < 65536)
		kharma *= kharma;
		
	kharma ^= XObjGetDateTime();
	
	// Use OS-native 'uptime' functions.
	// Normally we would use the Cross-Platform wrapper function XObjGetTickCount(),
	// but the native Windows function, with its nominally higher resolution (milliseconds
	// instead of 1/60sec ticks) will probably give a bit more entropy.
#ifdef WIN_VERSION
	kharma ^= GetTickCount();
#else
	kharma ^= TickCount();
#endif
	
	kharma ^= (long) (MachineKharma);
	kharma ^= ProcessIDMagic();
	kharma ^= UUIDMagic();
	
	kharma *= ++sAutoSeedCount;		// Salt MachineKharma with sAutoSeedCount in case there
									// are multiple calls during one machine tick. 
									// Modified 10-Feb-06 to multiply instead of use
									// exclusive-or. As Marsaglia says, multiplication
									// modifies more bits than any other simple arithmetic
									// operation.
	
	

	return kharma;
	}

