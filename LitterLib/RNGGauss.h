/*
	File:		RNGGauss.h

	Contains:	Header file for RNGGauss.c.

	Written by:	Peter Castine

	Copyright:	© 2001-2006 Peter Castine

	Change History (most recent first):

         <1>   30Ð3Ð2006    pc      Initial check in.
*/


/******************************************************************************************
 ******************************************************************************************/


#pragma once

#pragma mark ¥ Include Files

#include "MiscUtils.h"
#include "Taus88.h"


#pragma mark ¥ Function Prototypes

	// General purpose transformations we expose in case anyone else needs them
	
void	UnitSq2Norm		(double*, double*);			// Params: on input (x, y)
void	UnitCirc2Norm	(double*, double*);			// coordinates inside unit square
													// or circle; on exit two normally
													// distributed deviates

	// Box-Muller transform
double	NormalBM		(double*		oSpare,
						 tRandomFunc	iFunc,
						 void*			iData);
double	NormalBMTaus88	(double*		oSpare,
						 tTaus88Data*	iData);
	 
	// Kinderman-Ramage			
double	NormalKR		(tRandomFunc	iFunc,
						 void*			iData);
double	NormalKRTaus88	(tTaus88Data*	iData);


#pragma mark -
#pragma mark ¥ Inline Functions & Macros

	
