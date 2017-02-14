/*
	File:		TrialPeriodUtils.h

	Contains:	Library functions for use by the Litter Power Package.

	Written by:	Peter Castine

	Copyright:	© 2001-2006 Peter Castine

	Change History (most recent first):
	

*/

/******************************************************************************************
 ******************************************************************************************/

#pragma once
#ifndef TRIALPERIODUTILS_H
#define TRIALPERIODUTILS_H

#pragma mark ¥ Include Files

// We only need this for checking syntax,
// it will normally be #included before this file
#ifndef LITTERLIB_H
	#include "LitterLib.h"				// Need for gModDate
#endif


#pragma mark ¥ Constants

	// Trial period ends on 4:15PM, LITTER_TRIALPERIOD days after
	// the times specified by the arguments
#ifdef __GNUC__
	#define kDaySeconds		86400
	#define kTrialOffset	(LITTER_TRIALPERIOD * kDaySeconds + kDaySeconds / 2)
#else
	#ifdef LITTER_TRIALPERIOD
	const UInt32	kDaySeconds		= 86400,
					kTrialOffset	= LITTER_TRIALPERIOD * kDaySeconds + kDaySeconds / 2;
	#endif
#endif

#pragma mark ¥ Macros

	// #define TRIALPERIOD to zero for final versions
	// For beta, alpha, and development phases that go out of house, make
	// sure to #define it to a reasonable number of days
	// __TRIALPERIOD__ is typically #defined in a Prefix file or (with GCC)
	// as a command-line option.
#ifdef LITTER_TRIALPERIOD
	#define LITTER_CHECKTIMEOUT(X)	Boolean timeup = LitterTimeBomb(X)
	#define LITTER_TIMEBOMB			if (!timeup)
#else																	// __TRIALPERIOD__
	#define LITTER_TRIALPERIOD	0
	#define LITTER_CHECKTIMEOUT(X)
	#define LITTER_TIMEBOMB
#endif




#pragma mark -
#pragma mark ¥ Inline Functions

#if LITTER_TRIALPERIOD

	static inline short ParseMonth(const char iMonth[])
		{
		short result;
		
		switch (iMonth[0]) {
		case 'J':					// January, June, or July
			result = (iMonth[1] == 'a')
						? 1
						: (iMonth[2] != 'n') + 6;
			break;
		
		case 'F':					// February
			result = 2;
			break;
		
		case 'M':					// March, May
			result = (iMonth[2] == 'r') ? 3 : 5;
			break;
		
		case 'A':					// April, August
			result = (iMonth[1] == 'p') ? 4 : 8;
			break;
			
		case 'S':					// September
			result = 9;
			break;
			
		case 'O':					// October
			result = 10;
			break;
			
		case 'N':					// November
			result = 11;
			break;
			
		default:					// must be December
			result = 12;
			break;
			}
		
		return result;
		}
	
	
	#ifdef WIN_VERSION								// Code for Windows
	#pragma longlong on
	
	static inline UInt32 GetTriggerTime(short iYear, short iMonth, short iDay)
		{
		SYSTEMTIME		trigST;
		FILETIME		trigFT;
		UInt32			nowSecs = XObjGetDateTime(),
						trigSecs,
						fileSecs;
		
		// Get trigger time
		trigST.wYear			= iYear;
		trigST.wMonth			= iMonth;
		trigST.wDayOfWeek		= 0;				// Dummy value
		trigST.wDay				= iDay;
		trigST.wHour			= 4;
		trigST.wMinute			= 15;
		trigST.wSecond			= 0;
		trigST.wMilliseconds	= 0;
		
		SystemTimeToFileTime(&trigST, &trigFT);
		trigSecs = SecondsFromMacEpoch(&trigFT);
		
		// Check to make sure the __DATE__ macro resolved to something
		// within 24 hours of the file time stored on the OS
		// We leave this much leeway in case the system adjusts for different
		// time zones
		fileSecs = SecondsFromMacEpoch(&gModDate);
		if (fileSecs - kDaySeconds < trigSecs && trigSecs < fileSecs + kDaySeconds) {
			return trigSecs + kTrialOffset;
			}
		else return 0;
		}
	
	#pragma longlong reset
	#else											// Code for Mac OS
	
	
	static inline UInt32 GetTriggerTime(short iYear, short iMonth, short iDay)
		{
		DateTimeRec	trigDTR;
		UInt32		trigSecs;
		
		trigDTR.year		= iYear;
		trigDTR.month		= iMonth;
		trigDTR.day			= iDay;
		trigDTR.hour		= 4;
		trigDTR.minute		= 15;
		trigDTR.second		= 0;
		trigDTR.dayOfWeek	= 0;				// Dummy value
		
		DateToSeconds(&trigDTR, &trigSecs);
		
		// Check to make sure the __DATE__ macro resolved to something
		// within 24 hours of the file time stored on the OS
		// We leave this much leeway in case the system adjusts for different
		// time zones
		// On Mac OS I haven't yet made sure that gModDate is initialized before
		// the call to GetTriggerTime(). Need to fix !! ??
		if (gModDate == 0
				|| (gModDate - kDaySeconds < trigSecs && trigSecs < gModDate + kDaySeconds)) {
			return trigSecs + kTrialOffset;
			}
		else return 0;
		}
	
	
	#endif
	
	static inline Boolean
	LitterTimeBomb(
		const char	iClassName[])
		
		{
		char		month[32];			// Far more than is needed
		long		day, year;
		UInt32		now,
					trigger;
			
		// Quick check if trigger has been activated
		if (LITTER_TRIALPERIOD <= 0)
			return false;
		
		// Calculate trigger, based on build time and LITTER_TRIALPERIOD macro in seconds
		sscanf(__DATE__, "%s%ld%ld", month, &day, &year);
		trigger = GetTriggerTime(year, ParseMonth(month), day);
		now = XObjGetDateTime();
		
		if (now >= trigger) {
			post("=============================================================");
			post("%s: the trial period for this copy has expired", iClassName);
			post("  Some functions may be disabled");
			post("  Please contact 4-15 Music & Technlogy for a release version");
			post("  <mailto:4-15@kagi.com>");
			post("  <http://order.kagi.com/cgi-bin/store.cgi?storeID=GSX>");
			post("=============================================================");
			
			return true;
			}
		else {
			UInt32 daysLeft = (trigger - now) / 86400;
			
			if (daysLeft > 0)
				 post("%s: the trial period for this copy ends in %ld day%s",
						iClassName, daysLeft, (daysLeft > 1) ? "s" : "");
			else post("%s: the trial period for this copy ends %s",
						(trigger - now) <= 58500 ? "today" : "tomorrow afternoon");
			
			return false;
			}
		}

#else
	// LITTER_TRIALPERIOD macro must be 0
static inline Boolean LitterTimeBomb(const char iClassName[])
		{
		#pragma unused(iClassName)
		
		return false;
		}
#endif















#endif		// TRIALPERIODUTILS_H