/*
 *  MaxAttributeUtiles.c
 *  iCE
 *
 *  Created by Peter Castine on 17.12.10.
 *  Copyright 2010 4-15 Music & Technology. All rights reserved.
 *
 *	Utilities for initializing class attributes.
 *	Requires obex support (Max 5 or Max 4 w/Jitter)
 *
 */

/******************************************************************************************
 ******************************************************************************************/

#pragma mark ‚Ä¢ Include Files

#include "MaxAttributeUtils.h"


#pragma mark ‚Ä¢ Constants


#pragma mark -
#pragma mark ‚Ä¢ Functions

/******************************************************************************************
 *
 *	LitterInitClassAttributes(ioClass, iData, iCount)
 *
 ******************************************************************************************/

	const char*	kSymStrs[aTypeCount] =
					{"char", "long", "float32", "float64", "atom", "symbol", "object"};
	const char* kStyleStrs[aStyleCount] =
					{ "text", "onoff", "rgba", "enum", "enumindex", "rect", "font", "file" };
	
	static void InitTypeSymsArray(Symbol* oSyms[], long iCount)
		{ while (iCount-- > 0) *oSyms++ = NULL; }
	
	static Symbol* GenTypeSym(Symbol* ioSyms[], eAttrType iType)
		{
		if (ioSyms[iType] == NULL)
			ioSyms[iType] = gensym(kSymStrs[iType]);
		
		return ioSyms[iType];
		}

void
LitterInitClassAttributes(
	t_class*			ioClass,
	const tAttrData*	iData,
	unsigned long		iCount)
	
	{
	Symbol*	typeSyms[aTypeCount];
	long	order = 0;
	
	InitTypeSymsArray(typeSyms, aTypeCount);
	
	while (iCount-- > 0) {
		long	count = iData->count;
		Symbol* typeSym;
		Object*	attr;
		
		switch (iData->type) {
			default:		typeSym = GenTypeSym(typeSyms, iData->type);			break;
			case aTypeRect:	// fall into next case
			case aTypeColor:typeSym = GenTypeSym(typeSyms, aTypeDouble); count *= 4;	break;
			case aTypeFont:	/* !! ?? */												break;
			}
		
		if (iData->offset > 0) {
			attr = (count == 1)
					? attr_offset_new( iData->name,
										typeSym,
										iData->flags,
										(method) iData->getter,
										(method) iData->setter,
										iData->offset)
					: attr_offset_array_new(
										iData->name,
										typeSym,
										count,
										iData->flags,
										iData->getter,
										iData->setter,
										0,
										iData->offset);
			}
		else attr = attribute_new(	iData->name,
									typeSym,
									iData->flags,
									(method) iData->getter,
									(method) iData->setter);
		class_addattr(ioClass, attr);
			
		if (0 <= iData->style && iData->style < aStyleCount)
			switch (iData->style) {
			default:
				CLASS_ATTR_STYLE(ioClass, iData->name, iData->flags, kStyleStrs[iData->style]);
				break;
			case aStylePopupInd:
				CLASS_ATTR_ENUMINDEX(ioClass, iData->name, iData->flags, iData->popupStr);
				break;
			case aStylePopupVal:
				CLASS_ATTR_ENUM(ioClass, iData->name, iData->flags, iData->popupStr);
				break;
				}
		if (iData->label[0] != '\0')
			CLASS_ATTR_LABEL(ioClass, iData->name, iData->flags, iData->label);
		if (iData->category[0] != '\0')
			CLASS_ATTR_CATEGORY(ioClass, iData->name, iData->flags, iData->category);
		if (iData->initStr[0] != '\0')
			CLASS_ATTR_DEFAULT(ioClass, iData->name, iData->flags, iData->initStr);
		if (iData->save)
			CLASS_ATTR_SAVE(ioClass, iData->name, iData->flags);
		if (iData->paint)
			CLASS_ATTR_PAINT(ioClass, iData->name, iData->flags);
		if (iData->ordered) {
			char orderStr[32];
			sprintf(orderStr, "%ld", ++order);
			CLASS_ATTR_ORDER(ioClass, iData->name, iData->flags, orderStr);
			}
		
		iData++;
		}
	}

 
