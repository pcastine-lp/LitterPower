/*
	File:		IChingCore.c

	Contains:	Extract core for generating I Ching hexagrams. Used by both the externals
				lp.ginger and lp.kg

	Written by:	Peter Castine

	Copyright:	© 2001-2002 Peter Castine

	Change History (most recent first):

         <4>   23–3–2006    pc      Update for new LitterLib file organization.
         <3>     10–1–04    pc      Update for Windows. (Actually, only cosmetic items for this
                                    library file.)
         <2>  28–11–2002    pc      Tidy up after initial check in.
         <1>  28–11–2002    pc      Initial check in.
		10-Jul-2001:	First implementation
*/


/******************************************************************************************
 ******************************************************************************************/

#pragma mark • Include Files

#include "IChingCore.h"
#include "Taus88.h"

#pragma mark • Constants

	// Use to look up hexagram number based on binary encoding of a hexagram with
	// yang = 0 and yin = 1.
const tPackHexagram kIChingMap[]
							= { 1, 44, 13, 33, 10,  6, 25, 12,
								9, 57, 37, 53, 61, 59, 42, 20,
							   14, 50, 30, 56, 38, 64, 21, 35,
							   26, 18, 22, 52, 41,  4, 27, 23,
							   43, 28, 49, 31, 58, 47, 17, 45,
							    5, 48, 63, 39, 60, 29,  3,  8,
							   34, 32, 55, 62, 54, 40, 51, 16,
							   11, 46, 36, 15, 19,  7, 24,  2};

const eTrigram kTrigramMap[]
							= {	triChien,
								triSun,
								triLi,
								triKen,
								triTui,
								triKan,
								triChen,
								triKun};

const eTrigram	kRevTrigramBits[]
							= { 0x00, 0x08, 0x04, 0x06,
								0x03, 0x05, 0x03, 0x07};

const tHexInfo	kHexagramInfo[]
		= {
		// Bitmap,	Governing Ruler,		Constitutional Ruler,	House,		Pos. in House	
			{0x00,	lineFifth,				linesNone,				triChien,	1},		// Ch’ien
			{0x3f,	lineSecond,				linesNone,				triLi,		1},		// K’un
			{0x2e,	lineBottom + lineFifth,	linesNone,				triKun,		3},		// Chun
			{0x1d,	lineSecond + lineFifth,	linesNone,				triSun,		5},		// Meng
			{0x28,	lineFifth,				linesNone,				triLi,		7},		// Hsü
			{0x05,	lineFifth,				linesNone,				triSun,		7},		// Sung
			{0x3d,	lineSecond + lineFifth,	linesNone,				triKun,		8},		// Shih
			{0x2f,	lineFifth,				linesNone,				triLi,		8},		// Pi
			{0x08,	lineFifth,				lineFourth,				triKen,		2},		// Hsiao Ch’u
			{0x04,	lineFifth,				lineThird,				triChen,	6},		// Lü
			{0x38,	lineSecond + lineFifth,	linesNone,				triLi,		4},		// T’ai
			{0x07,	lineFifth,				lineSecond,				triChien,	4},		// P’i
			{0x02,	lineSecond + lineFifth,	linesNone,				triSun,		8},		// T’ung Jen
			{0x10,	lineFifth,				linesNone,				triChien,	8},		// Ta Yu
			{0x3b,	lineThird,				linesNone,				triTui,		6},		// Ch’ien
			{0x37,	lineFourth,				linesNone,				triKan,		2},		// Yü
			{0x26,	lineBottom + lineFifth,	linesNone,				triKan,		8},		// Sui
			{0x19,	lineFifth,				linesNone,				triKen,		8},		// Ku
			{0x3c,	lineBottom + lineSecond, linesNone,				triLi,		3},		// Lin
			{0x0f,	lineFifth + lineTop,	linesNone,				triChien,	5},		// Kuan
			{0x16,	lineFifth,				linesNone,				triKen,		6},		// Shih Ho
			{0x1a,	lineSecond + lineTop,	linesNone,				triChen,	2},		// Pi
			{0x1f,	lineTop,				linesNone,				triChien,	6},		// Po
			{0x3e,	lineBottom,				linesNone,				triLi,		2},		// Fu
			{0x06,	lineBottom + lineFifth,	linesNone,				triKen,		5},		// Wu Wang
			{0x18,	lineFifth + lineTop,	linesNone,				triChen,	3},		// Ta Ch’u
			{0x1e,	lineFifth + lineTop,	linesNone,				triKen,		7},		// I
			{0x21,	lineSecond + lineFourth, linesNone,				triKan,		7},		// Ta Ko
			{0x2d,	lineSecond + lineFifth,	linesNone,				triKun,		1},		// K’an
			{0x12,	lineSecond + lineFifth,	linesNone,				triSun,		1},		// Li
			{0x23,	lineFourth + lineFifth,	linesNone,				triTui,		4},		// Hsien
			{0x31,	lineSecond,				linesNone,				triKan,		4},		// Heng
			{0x03,	lineFifth,				lineBottom + lineSecond, triChien,	3},		// Tun
			{0x30,	lineFourth,				linesNone,				triLi,		5},		// Ta Chuang
			{0x17,	lineFifth,				linesNone,				triChien,	7},		// Chin
			{0x3a,	lineSecond + lineFifth,	lineTop,				triKun,		7},		// Ming I
			{0x0a,	lineSecond + lineFifth,	linesNone,				triKen,		3},		// Chia Jen
			{0x14,	lineSecond + lineFifth,	linesNone,				triChen,	5},		// K’uei
			{0x2b,	lineFifth,				linesNone,				triTui,		5},		// Chien
			{0x35,	lineSecond + lineFifth,	linesNone,				triKan,		3},		// Hsieh
			{0x1c,	lineFifth,				lineThird + lineTop,	triChen,	4},		// Sun
			{0x0e,	lineSecond + lineFifth,	lineBottom + lineFourth, triKen,	4},		// I
			{0x20,	lineFifth,				lineTop,				triLi,		6},		// Kuai
			{0x01,	lineSecond + lineFifth,	lineBottom,				triChien,	2},		// Kou
			{0x27,	lineFourth + lineFifth,	linesNone,				triTui,		3},		// Ts’ui
			{0x39,	lineFifth,				lineBottom,				triKan,		5},		// Sheng
			{0x25,	lineSecond + lineFifth,	linesNone,				triTui,		2},		// K’un
			{0x29,	lineFifth,				linesNone,				triKan,		6},		// Ching
			{0x22,	lineFifth,				linesNone,				triKun,		5},		// Ko
			{0x11,	lineFifth + lineTop,	linesNone,				triSun,		3},		// Ting
			{0x36,	lineBottom,				linesNone,				triKan,		1},		// Chen
			{0x1b,	lineTop,				linesNone,				triChen,	1},		// Ken
			{0x0b,	lineSecond + lineFifth,	linesNone,				triChen,	8},		// Chien
			{0x34,	lineFifth,				lineThird + lineTop,	triTui,		8},		// Kuei Mei
			{0x32,	lineFifth,				linesNone,				triKun,		6},		// Feng
			{0x13,	lineFifth,				linesNone,				triSun,		2},		// Lü
			{0x09,	lineFifth,				lineBottom + lineFourth, triKen,	1},		// Sun
			{0x24,	lineSecond + lineFifth,	lineThird + lineTop,	triTui,		1},		// Tui
			{0x0d,	lineFifth,				lineSecond + lineFourth, triSun,	5},		// Huan
			{0x2c,	lineFifth,				linesNone,				triKun,		2},		// Chieh
			{0x0c,	lineFifth,				lineThird + lineFourth,	triChen,	7},		// Chung Fu
			{0x33,	lineSecond + lineFifth,	linesNone,				triTui,		7},		// Hsiao Kuo
			{0x2a,	lineSecond,				linesNone,				triKun,		4},		// Chi Chi
			{0x15,	lineFifth,				linesNone,				triSun,		4}		// Wei Chi
			};

#pragma mark • Type Definitions

#pragma mark • Object Structure


#pragma mark • Global Variables


#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark • Inline Functions

#pragma mark -
#pragma mark • Static Functions

/******************************************************************************************
 *
 *	DivideSticksAndCountFours(iHowMany)
 *
 ******************************************************************************************/

static int
DivideSticksAndCountFours(
				long iHowMany)
	
	{
	// For mystical reasons, we do not use stick number 0.
	// Other apparent inefficiencies are due to following the method for throwing
	// yarrow sticks (as described by Richard Wilhelm) as closely as possible.
	unsigned long	sticks[kYarrowStickCount];
	int				leftPile,
					rightPile,
					betweenFingers[3],
					s;						// Index into sticks[] array.
	
	
	// Divide sticks randomly into two piles
	do {
		unsigned long threshhold = Taus88(NULL);
		for (s = iHowMany; s > 0; s -= 1) {
			sticks[s] = Taus88(NULL);
			}
		leftPile = 0;
		rightPile = 0;
		for (s = iHowMany; s > 0; s -= 1) {
			if (sticks[s] >= threshhold)
					rightPile += 1;
			else	leftPile += 1;
			}
		} while (rightPile == 0 || leftPile == 0);	// If the condition fails,
													// we don't have two piles
	
	// Take one stick from the right pile and place it between 4th and 5th finger
	rightPile -= 1; betweenFingers[2] = 1;
	
	// Remove sticks from left pile in groups of four, place the remainder
	// between 3rd and 4th finger
	while (leftPile > 4) {
		leftPile -= 4;
		}
	betweenFingers[1] = leftPile;
	
	// Remove sticks from right pile in groups of four, place the remainder
	// between 2nd and 3rd finger
	while (rightPile > 4) {
		rightPile -= 4;
		}
	betweenFingers[0] = rightPile;
	
	return betweenFingers[2] + betweenFingers[1] + betweenFingers[0];
	}
	
	
/******************************************************************************************
 *
 *	ThrowSticks(oOracle)
 *
 ******************************************************************************************/

static void
FinalizeOracle
	(tOraclePtr ioOracle)
	
	{
	unsigned	present		= 0,
				future		= 0,
				lineIndex,
				bit;
	tLinePtr	lineBase	= ioOracle->lines - 1;	// Allows us to use unit-based indices
	
	// Generate binary representation of the present and future hexagrams,
	// with yang represented as a clear bit and yin with a set bit.
	for (lineIndex = bit = 1; lineIndex <= kLineCount; lineIndex += 1, bit += bit) {
		switch (lineBase[lineIndex].value) {	
			case oldYang:
				// Present is yang (leave bit clear); future is yin (set bit)
				future += bit;
				break;
			
			case youngYin:
				// Set bit for both present and future
				future += bit;
				present += bit;
				break;
			
			case oldYin:
				// Present is yin (set bit); future is yang (leave bit clear)
				present += bit;
				break;
			
			default:
				// Must be youngYang. Nothing to do.
				break;
			}
		}
	
	// Return the hexagram numbers
	ioOracle->mainHexagram = kIChingMap[present];
	ioOracle->futureHexagram = kIChingMap[future];
	}

#pragma mark -
#pragma mark • Exported Functions

/******************************************************************************************
 *
 *	ThrowSticks(oOracle)
 *
 ******************************************************************************************/

void
ThrowSticks(
	tOraclePtr oOracle)
	
	{
	tLinePtr	lineBase = oOracle->lines - 1;		// Allows us to use unit-based indices
	int			lineIndex;
	
	for (lineIndex = 1; lineIndex <= kLineCount; lineIndex += 1) {
		int		sticksToThrow	= kYarrowStickCount - 1,
				sticksInHand,
				yySum			= 0;
		
		sticksInHand = DivideSticksAndCountFours(sticksToThrow);
		yySum	+= lineBase[lineIndex].components[0]
				 = (sticksInHand == 5) ? yang : yin;
		
		sticksToThrow -= sticksInHand;
		sticksInHand = DivideSticksAndCountFours(sticksToThrow);
		yySum	+= lineBase[lineIndex].components[1]
				 = (sticksInHand == 4) ? yang : yin;
		
		sticksToThrow -= sticksInHand;
		sticksInHand = DivideSticksAndCountFours(sticksToThrow);
		yySum	+= lineBase[lineIndex].components[2]
				 = (sticksInHand == 4) ? yang : yin;
		
		// ASSERT: yySum is a valid eLineValues
		lineBase[lineIndex].value = (eYinYang) yySum;
		}

	FinalizeOracle(oOracle);
	
	}

/******************************************************************************************
 *
 *	TossCoins(oOracle)
 *
 ******************************************************************************************/

void
TossCoins(
	tOraclePtr oOracle)
	
	{
	const unsigned long kCoin1Mask		= 0x80000000,
						kCoin2Mask		= 0x00008000,
						kCoin3Mask		= 0x00000001,
						kThreeCoinMask	= kCoin1Mask + kCoin2Mask + kCoin3Mask;
	 
	tLinePtr	lineBase = oOracle->lines - 1;		// Allows us to use unit-based indices
	long		lineIndex;
	
	// For mystical reasons we count the line index from 1 up to 6, even though this
	// means we have to adjust the index for 0-based arrays inside the loop.
	// Note that the coin masks are taken from the sign bit for long and short integers
	// and the "odd" bit. Hence the mapping of "bit set" to yin.
	for (lineIndex = 1; lineIndex <= kLineCount; lineIndex += 1) {
		// Throw three coins at once.
		unsigned long	threeCoins	= Taus88(NULL) & kThreeCoinMask;
		int				yySum		= 0;
		
		yySum	+= lineBase[lineIndex].components[0]
				 = (threeCoins & kCoin1Mask) ? yin : yang;
		yySum	+= lineBase[lineIndex].components[1]
				 = (threeCoins & kCoin2Mask) ? yin : yang;
		yySum	+= lineBase[lineIndex].components[2]
				 = (threeCoins & kCoin3Mask) ? yin : yang;
		
		// ASSERT: yySum is a valid eLineValues
		lineBase[lineIndex].value = (eYinYang) yySum;
		}
		
	FinalizeOracle(oOracle);
	
	}
