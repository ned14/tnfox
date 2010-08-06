/********************************************************************************
*                                                                               *
*                            Filing system monitor                              *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2003-2008 by Niall Douglas.   All Rights Reserved.       *
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
class QFileInfo;

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
as so to avoid polling.

As of v0.87 when TnFOX was ported to Apple MacOS X, the problem emerged that there
was no FAM implementation for MacOS X. This meant having to write an implementation
based on BSD kqueue's which aren't entirely up to the job - in particular, only
creations and deletions within a directory are reported and at that point, and
only that point, will size and metadata changes etc. be found and reported. kqueue's
\b can provide all the functionality we need, but they require an open file handle
on each and every file in a directory being monitored which quickly causes the
process to run out of available file handles.

As of v0.88, libgamin (the supposedly better replacement for FAM) was hanging
yet again and this time I couldn't work around it. Thus as of v0.88, on Linux
FXFSMonitor is implemented using the kernel inotify interface available in
2.6.13 onwards. On BSD, it still tries to use libfam/libgamin if it can due
to the missing kqueue functionality, but finally we are free of FAM dependency!!!

As Tn especially does a lot of file system monitoring, it has been designed
to work around system limits - the limit of 64 maximum waitable objects on
Windows has been worked around using multiple threads and FAM can scale to
1024 watches per process. This should be sufficient for most users.

When the contents of a directory change, this class will upcall a supplied
FX::Generic::Functor. Your functor will be called via the process-wide thread pool
as provided by FX::FXProcess so ensure your handler is thread safe. Ordering
of event upcalls is in the order that the operating system reports them,
though you should note that thread pools can be unreliable here if the host OS
scheduler deems it so - use the event index in the changes structure.

Note that on Windows, for each 62 paths you monitor a thread must run. Upcalls
are issued via the process-wide thread pool available through FX::FXProcess::threadPool().
Note also that any relative paths you specify are converted to absolute paths
at the time of addition so if the current directory changes, you will need
to specify something different to remove the monitor.

<h3>Usage:</h3>
PLEASE NOTE that monitoring of certain parts of the file system is not supported
by all operating systems eg; some Unices won't let you monitor a mounted VFAT
or NFS path which would be indistinguishable from normal filing system paths.
In this situation, FXFSMonitor will throw an exception during add()
which you will need to catch and handle appropriately. Therefore, ALWAYS wrap
FXFSMonitor::add() with a FXERRH_THROW etc.

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
void Obj::handler(FXFSMonitor::Change change, const QFileInfo &oldfi, const QFileInfo &newfi);
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
		FXulong eventNo;			//!< Non zero event number index
		FXuint modified	: 1;		//!< When an entry is modified
		FXuint created	: 1;		//!< When an entry is created
		FXuint deleted	: 1;		//!< When an entry is deleted
		FXuint renamed	: 1;		//!< When an entry is renamed
		FXuint attrib	: 1;		//!< When the attributes of an entry are changed 
		FXuint security	: 1;		//!< When the security of an entry is changed
		Change() : eventNo(0), modified(false), created(false), deleted(false), renamed(false), attrib(false), security(false) { }
		Change(int) : eventNo(0), modified(true), created(true), deleted(true), renamed(true), attrib(true), security(true) { }
		operator FXuint() const throw()
		{
			struct Change_ { FXulong eventNo; FXuint flags; } *me=(Change_ *) this;
			return me->flags;
		}
		//! Sets the modified bit
		Change &setModified(bool v=true) throw()	{ modified=v; return *this; }
		//! Sets the created bit
		Change &setCreated(bool v=true) throw()		{ created=v; return *this; }
		//! Sets the deleted bit
		Change &setDeleted(bool v=true) throw()		{ deleted=v; return *this; }
		//! Sets the renamed bit
		Change &setRenamed(bool v=true) throw()		{ renamed=v; return *this; }
		//! Sets the attrib bit
		Change &setAttrib(bool v=true) throw()		{ attrib=v; return *this; }
		//! Sets the security bit
		Change &setSecurity(bool v=true) throw()	{ security=v; return *this; }
	};
	typedef Generic::TL::create<void, Change, QFileInfo, QFileInfo>::value ChangeHandlerPars;
	//! Defines the type of functor change handlers are
	typedef Generic::Functor<ChangeHandlerPars> ChangeHandler;
	//! Adds a monitor of a path on the filing system
	static void add(const FXString &path, ChangeHandler handler);
	//! Removes a monitor of a path. Cancels any pending handler invocations.
	static bool remove(const FXString &path, ChangeHandler handler);
};

} // namespace

#endif
