/********************************************************************************
*                                                                               *
*                         Thread cancellable select()                           *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2006 by Niall Douglas.   All Rights Reserved.            *
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

static inline int tnfxselect(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)
{
#ifndef __APPLE__
	return ::select(nfds, readfds, writefds, exceptfds, timeout);
#else
	// For some unfathomable reason, Apple have not made select() always a thread cancellation
	// point despite the POSIX standard requiring it. This thoroughly breaks most parts
	// of TnFOX which *requires* it to be a cancellation point as it should be
	//
	// This emulation simply uses polling to emulate the same. It's rotten, it's very
	// inefficient, I dislike it strongly but I can't see an alternative :(

	int ret;
	struct timeval tv;
	using namespace FX;
	FXulong waitfor=0, end=0;
	if(timeout)
	{
		waitfor=timeout->tv_sec*1000000000ULL+timeout->tv_usec*1000;
		end=FXProcess::getNsCount()+waitfor;
	}
	do
	{
		FXulong now=timeout ? FXProcess::getNsCount() : 0;
		FXlong diff=timeout ? (FXlong)(end-now) : 10000000000ULL;
		if(diff<0)
			return 0;
		else if(diff<250000000)
		{
			tv.tv_sec =diff/1000000000;
			tv.tv_usec=((diff/1000) % 1000000);
		}
		else
		{
			tv.tv_sec=0;
			tv.tv_usec=250000000;
		}
		QThread::current()->checkForTerminate();
	} while(!(ret=::select(nfds, readfds, writefds, exceptfds, &tv)));
	return ret;
#endif
}
