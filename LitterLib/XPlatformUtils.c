/*
	File:		XPlatformUtils.c

	Contains:	Cross-platform wrappers.

	Written by:	Peter Castine

	Copyright:	© 2006 Peter Castine

	Change History (most recent first):

*/


/******************************************************************************************

	PREFACE: Historically Max/MSP/Jitter and all Litter Power objects were MacOS based
			 and Litter Power objects made free use of the Mac API. With the introduction
			 of Windows support Litter Power has made use of wrapper functions to access
			 this information. Where there are differences in data formats or units (for
			 instance, the Win32 API measures system 'uptime' in milliseconds while Mac OS
			 measures in 'ticks' of 1/60 second), the Windows units are converted to the
			 match Mac OS conventions.
			 
			 The cross-platform function names are prefixed 'XObj' (cross-platform object).
			 It seemed like a good idea at the time.

 ******************************************************************************************/

#pragma mark • Include Files

#include "XPlatformUtils.h"



#pragma mark • Constants



#pragma mark • Type Definitions



#pragma mark • Static Variables



#pragma mark • Initialize Global Variables



#pragma mark • Function Prototypes

#pragma mark -
/*****************************  I M P L E M E N T A T I O N  ******************************/

#pragma mark • Private & Inline Functions



#pragma mark -
#pragma mark • Cross-Platform Utilities

/******************************************************************************************
 *
 *	XObjGetModDate(iRefNum)
 *	
 *	All an external knows at main() time is its File Reference Number (returned by
 *	CurResFile()... Max guarantees that the external is the current resource at this time.)
 *	
 *	This is enough to grab the modification date, which is a useful bit of information that
 *	one can give the user.
 *
 *	The Mac version talks directly to the Mac OS API because that used to be the only way
 *	to get this information (some code appropriated from the MoreFiles library from Mac
 *	Developer Tech Support). The Windows version can only run on Max 4.3 or later and makes
 *	use of Max API calls added between Max 4.1 and 4.3.
 *	
 ******************************************************************************************/

#ifdef WIN_VERSION

unsigned long
XObjGetModDate(
	const char* iClassName)
	
	{
	char			fName[256];
	short			err,
					fPath;
	long			fType;
	unsigned long	modDate = 0;
			
	strcpy(fName, iClassName);
	strcat(fName, ".mxe");
	err = locatefile_extended(fName, &fPath, &fType, NULL, 0);
	
	if (err == noErr)
		path_getmoddate(fPath, &modDate);
	return modDate;
	}


#else

	static OSErr FileGetLocation(
		short		iRefNum,
		short*		oVRefNum,
		long*		oParDirID,
		StringPtr	oFileName)

		{
		FCBPBRec	pb;
		OSErr		errCode = noErr;

		pb.ioNamePtr	= oFileName;
		pb.ioVRefNum	= 0;
		pb.ioRefNum		= iRefNum;
		pb.ioFCBIndx	= 0;
		
		errCode = PBGetFCBInfoSync(&pb);
		if (errCode != noErr)
			goto punt;
			
		*oVRefNum	= pb.ioFCBVRefNum;
		*oParDirID	= pb.ioFCBParID;
		
	punt:	
		return errCode;
		}

	static OSErr FileGetDates(
		short				iVRefNum,
		long				iDirID,
		ConstStr255Param	iFileName,
		long*				oCrDate,
		long*				oModDate)
		
		{
		HParamBlockRec	pb;
		OSErr			errCode;
		
		pb.fileParam.ioVRefNum		= iVRefNum;
		pb.fileParam.ioFVersNum		= 0;
		pb.fileParam.ioFDirIndex	= 0;
		pb.fileParam.ioNamePtr		= (StringPtr) iFileName;
		pb.fileParam.ioDirID		= iDirID;
		
		errCode = PBHGetFInfoSync(&pb);
		if (errCode != noErr)
			goto punt;
		
		*oCrDate	= pb.fileParam.ioFlCrDat;
		*oModDate	= pb.fileParam.ioFlMdDat;
		
	punt:	
		return errCode;
		}



unsigned long
XObjGetModDate(
	short	iRefNum)	// File Reference Number returned by CurResFile() in main
	
	{
	short	errCode	= noErr,
			vRefNum;
	long	parDirID,
			crDate,
			modDate;
	Str255	fileName;
	
	errCode = FileGetLocation(iRefNum, &vRefNum, &parDirID, fileName);
	if (errCode != noErr)
		goto exit;			// Cheesy exception handling

	errCode = FileGetDates(vRefNum, parDirID, fileName, &crDate, &modDate);
							// Even cheesier exception handling
	
exit:
	return (errCode == noErr) ? modDate : 0;
	}

#endif
