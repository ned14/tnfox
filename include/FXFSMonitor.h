/********************************************************************************
*                                                                               *
*                            Filing system monitor                              *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2003 by Niall Douglas.   All Rights Reserved.            *
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

#ifndef FXFSMONITOR_H
#define FXFSMONITOR_H

#include "FXGenericTools.h"

namespace FX {

/*! \file FXFSMonitor.h
\brief Defines classes used in monitoring the file system
*/

class FXString;
class FXFileInfo;

/*! \class FXFSMonitor
\brief Provides portable file system monitoring for changes

Yet more code ported from Tornado - and this one was tricky to get right originally!
Monitoring the file system has traditionally been a right arse with all the
mainstream systems not doing better than you rereading a directory listing every
few seconds. Conversely, operating systems such as Acorn RISC-OS issued a
system upcall which you could hook into and thus know when anything at all
changed. By the way, Tn improves on RISC-OS even then some.

Until v0.72, this class was implemented using the Linux-only \c dmonitor kernel
facility but as BSD support was required, it was rewritten to use SGI's FAM
daemon which all modern Unices should provide. FAM is significantly easier
to use than doing it manually and also can use kernel monitoring facilities
as so to avoiding polling - currently FreeBSD's \c kqueue is not quite up to
the job but it would be easy to extend. Whatever the case, it's better for one
thing to poll a directory that lots of things.

As Tn especially does a lot of file system monitoring, it has been designed
to work around system limits - the limit of 64 maximum waitable objects on
Windows has been worked around using multiple threads and FAM can scale to
1024 watches per process. This should be sufficient for most users.

Each path
When the contents of a directory change, this class will upcall a supplied
FX::Generic::Functor. Your functor will be called via the process-wide thread pool
as provided by FX::FXProcess so ensure your handler is thread safe.

Note that on Windows, for each 62 paths you monitor a thread must run. Upcalls
are issued via the process-wide thread pool available through FX::FXProcess::threadPool().
Note also that any relative paths you specify are converted to absolute paths
at the time of addition so if the current directory changes, you will need
to specify something different to remove the monitor.

<h3>Usage:</h3>
The upcall is issued with what has changed (FX::FXFSMonitor::Change) and both
the previous and latest states of the path. Because POSIX
doesn't provide a mechanism for the OS to tell you what file has been renamed
from what to whatever else (and Windows' support for this is plain nasty), I
elected to replace the old Tornado code here with a manual rename detection
mechanism which relies on renamed entries not changing their size, creation
date, last modified date nor last changed date. If any of these do change
or there are many entries with the same size, creation date, modification date
and last changed date then the code attempts to return a deletion and creation
of a new entry. The most which will happen then is inefficiency.

You should declare your handler as follows:
\code
void Obj::handler(FXFSMonitor::Change change, const FXFileInfo &oldfi, const FXFileInfo &newfi);
\endcode
This ensures optimum passing of parameters. To install:
\code
FXFSMonitor::add("/tmp/foo", FXFSMonitor::ChangeHandler(&Obj::handler));
\endcode
*/
class FXAPIR FXFSMonitor
{
public:
	//! Specifies what to monitor and what has changed
	struct Change
	{
		bool modified;		//!< When an entry is modified
		bool created;		//!< When an entry is created
		bool deleted;		//!< When an entry is deleted
		bool renamed;		//!< When an entry is renamed
		bool attrib;		//!< When the attributes of an entry are changed 
		bool security;		//!< When the security of an entry is changed
		Change() : modified(false), created(false), deleted(false), renamed(false), attrib(false), security(false) { }
		Change(int) : modified(true), created(true), deleted(true), renamed(true), attrib(true), security(true) { }
	};
	typedef Generic::TL::create<void, Change, FXFileInfo, FXFileInfo>::value ChangeHandlerPars;
	//! Defines the type of functor change handlers are
	typedef Generic::Functor<ChangeHandlerPars> ChangeHandler;
	//! Adds a monitor of a path on the filing system
	static void add(const FXString &path, ChangeHandler handler);
	//! Removes a monitor of a path. Cancels any pending handler invocations.
	static bool remove(const FXString &path, ChangeHandler handler);
};

} // namespace

#endif
