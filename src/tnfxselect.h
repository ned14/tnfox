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
	using namespace FX;
	// For some unfathomable reason, Apple have not made select() always a thread cancellation
	// point despite the POSIX standard requiring it. This thoroughly breaks most parts
	// of TnFOX which *requires* it to be a cancellation point as it should be
	//
	// With assistance of QThread, this cancellable select relies on a magic waiter pipe
	// to get signalled when cancellation is wanted.

	int ret, h=0;
	if(readfds)
	{
		h=(int)(FXuval) QThread::int_cancelWaiterHandle();
		FD_SET(h, readfds);
		if(h>=nfds) nfds=h+1;
	}
	ret=::select(nfds, readfds, writefds, exceptfds, timeout);
	if(ret>0 && readfds && FD_ISSET(h, readfds))
		QThread::current()->checkForTerminate();
	return ret;
#endif
}
