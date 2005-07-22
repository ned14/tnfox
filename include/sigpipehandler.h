/********************************************************************************
*                                                                               *
*                               SIGPIPE handler                                 *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2004 by Niall Douglas.   All Rights Reserved.            *
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

#ifndef SIGPIPEHANDLER_H
#define SIGPIPEHANDLER_H

#include "fxdefs.h"
#include <signal.h>
#include "QThread.h"
#include "FXException.h"

namespace FX {

#ifdef USE_POSIX
class QIODeviceS_SignalHandler
{
	static QThreadLocalStorageBase data;
	static void handler(int)
	{
#ifdef DEBUG
		fxmessage("*** SIGPIPE received!\n");
#endif
		data.setPtr((void *) 1);
	}
public:
	QIODeviceS_SignalHandler()
	{
		if(SIG_ERR==signal(SIGPIPE, handler)) FXERRHOS(-1);
	}
	static void lockWrite()
	{
		data.setPtr(0);
	}
	static bool unlockWrite()
	{
		return data.getPtr()!=0;
	}
};
#endif

} // namespace

#endif
