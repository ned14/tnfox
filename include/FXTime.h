/********************************************************************************
*                                                                               *
*                         P r o c e s s   S u p p o r t                         *
*                                                                               *
*********************************************************************************
* Copyright (C) 2005 by Niall Douglas.   All Rights Reserved.                   *
*       NOTE THAT I DO NOT PERMIT ANY OF MY CODE TO BE PROMOTED TO THE GPL      *
*********************************************************************************
* This code is free software; you can redistribute it and/or modify it under    *
* the terms of the GNU Library General Public License v2.1 as published by the  *
* Free Software Foundation EXCEPT that clause 3 does not apply ie; you may not  *
* "upgrade" this code to the GPL without my prior written permission.           *
* Please consult the file "License_Addendum2.txt" accompanying this file.       *
*                                                                               *
* This code is distributed in the hope that it will be useful,                  *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                          *
*********************************************************************************
* $Id:                                                                          *
********************************************************************************/

#ifndef _FXTime_h_
#define _FXTime_h_

#include "FXString.h"
#include <time.h>

namespace FX {

/*! \file FXTime.h
\brief Defines classes used in working with time
*/

/*! \struct FXTime
\brief Holds the number of elapsed microseconds since 1st January year 0 UTC

Given that there are 1,000,000 microseconds in a second and roughly
365.24119241192411924119241192412 days in a year, there are roughly
31,556,839,024,390 microseconds in a year. This means that an unsigned 64 bit
container can hold the latest date of 20th February, year 584,556 which is
probably enough.

FXTime knows when it is in local time and correctly serialises itself such
that timezone translation correctly occurs.
*/
struct FXAPI FXTime		// NOTE: Defined in FXProcess.cxx
{
	//! The number of microseconds per second
	static const FXulong micsPerSecond=1000000;
	//! The number of microseconds per year
	static const FXulong micsPerYear=31556839024390LL;
	//! The number of microseconds that was midnight 1st January 1601 UTC
	static const FXulong mics1stJan1601=50522499278048780LL;
	//! The number of microseconds that was midnight 1st January 1970 UTC
	static const FXulong mics1stJan1970=62166972878048780LL;

	//! Whether this time is in local time
	bool isLocalTime;
	//! The number of elapsed microseconds since midnight 1st January year 0 UTC
	FXulong value;

	//! Constructs an instance
	explicit FXTime(FXulong _value=0, bool _isLocalTime=false) : isLocalTime(_isLocalTime), value(_value) { }
	//! Constructs an instance from a \c time_t
	explicit FXTime(time_t ctime) : isLocalTime(false), value(0) { set_time_t(ctime); }
	bool operator!() const throw() { return !value; }
	bool operator==(const FXTime &o) const throw() { return isLocalTime==o.isLocalTime && value==o.value; }
	bool operator!=(const FXTime &o) const throw() { return isLocalTime!=o.isLocalTime || value!=o.value; }
	bool operator<(const FXTime &o) const throw() { return isLocalTime<o.isLocalTime || value<o.value; }
	bool operator>(const FXTime &o) const throw() { return isLocalTime>o.isLocalTime || value>o.value; }

	friend FXTime operator+(const FXTime &a, const FXTime &b) throw() { return FXTime(a.value+b.value); }
	friend FXTime operator+(const FXTime &a, FXulong v) throw() { return FXTime(a.value+v); }
	friend FXTime operator-(const FXTime &a, const FXTime &b) throw() { return FXTime(a.value-b.value); }
	friend FXTime operator-(const FXTime &a, FXulong v) throw() { return FXTime(a.value-v); }
	FXTime &operator+=(const FXTime &b) throw() { value+=b.value; return *this; }
	FXTime &operator+=(FXulong v) throw() { value+=v; return *this; }
	FXTime &operator-=(const FXTime &b) throw() { value-=b.value; return *this; }
	FXTime &operator-=(FXulong v) throw() { value-=v; return *this; }

	//! Returns the value as a \c time_t
	time_t as_time_t() const throw()
	{
		return (time_t)(value ? (value-mics1stJan1970)/micsPerSecond : 0);
	}
	//! Sets the value from a \c time_t
	FXTime &set_time_t(time_t v) throw()
	{
		value=v ? mics1stJan1970+v*micsPerSecond : 0;
		return *this;
	}
	//! Returns the value as a \c struct \c tm
	struct tm *as_tm(struct tm *buf, bool inLocalTime=false) const;
	//! Sets the value from a \c struct \c tm
	FXTime &set_tm(struct tm *buf, bool isInLocalTime=false);

	/*! Returns the time as a string of format \em fmt.
	The formatting inserts are as for strftime in the C library except that
	%F means microsecond fraction of the form 999999 or 000001 which would
	mean 0.999999 or 0.000001 of a second */
	FXString asString(const FXString &fmt="%Y/%m/%d %H:%M:%S.%F %Z") const;
	//! Converts time to local time if it isn't already
	FXTime &toLocalTime() { if(!isLocalTime) { if(value) value+=localTimeDiff(); isLocalTime=true; } return *this; }
	//! Converts time from local time to UTC if it isn't already
	FXTime &toUTC() { if(isLocalTime) { if(value) value-=localTimeDiff(); isLocalTime=false; } return *this; }

	//! Returns this present moment
	static FXTime now(bool inLocalTime=false);
	//! Returns how much to add or subtract to convert UTC to local time
	static FXlong localTimeDiff();

	friend FXAPI FXStream &operator<<(FXStream &s, const FXTime &i);
	friend FXAPI FXStream &operator>>(FXStream &s, FXTime &i);
};
#define FXTIMETOFILETIME(filetime, time) { FXulong temp=((time).value-FXTime::mics1stJan1601)*10; (filetime).dwHighDateTime=(DWORD)(temp>>32); (filetime).dwLowDateTime=(DWORD) temp; }
#define FXTIMEFROMFILETIME(time, filetime) { (time).value=(((FXulong) (filetime).dwHighDateTime<<32)|(FXulong) (filetime).dwLowDateTime)/10; (time).value+=FXTime::mics1stJan1601; }

} // namespace

#endif


