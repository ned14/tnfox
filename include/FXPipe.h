/********************************************************************************
*                                                                               *
*                            Named pipe i/o device                              *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2002,2003 by Niall Douglas.   All Rights Reserved.       *
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


#ifndef FXPIPE_H
#define FXPIPE_H
#include "FXIODeviceS.h"

namespace FX {

/*! \file FXPipe.h
\brief Defines classes used to provide a portable named pipe
*/

class FXString;

/*! \class FXPipe
\ingroup siodevices
\brief A named pipe i/o device

This is one of the oldest pieces of code in my extensions to FOX - it was one
of the first classes written for Tornado originally and it is a testament to
how API compatible with Qt TnFOX is that very little work had to be done to port it.
In fact, the below docs are almost identical too :)

FXPipe is a \em synchronous i/o device like FX:FXBlkSocket - in other words,
reads from it block until data is available. To avoid this, check atEnd() before
reading - however generally, <i>you want it</i> to block because you will have
allocated a special thread whose sole purpose is to wait on incoming data and
asynchronously do something useful with it. One action could be to post a message
to some control in the GUI for example but more likely you'll combine it with
FX::FXIPCMsgMonitorBase.

Reads and writes are threadsafe, but you should avoid more than two threads working
with the same pipe (the same as more than two processes) because transfers occur
in no more than maxAtomicLength() (usually 4096 bytes, but it is system dependent) -
so two threads writing at the same time will cause the read data to be mixed
blocks of data - which you probably don't want. If you do use a pipe which has
more than two users, ensure all data transfers are less than maxAtomicLength(). If
you want a pipe with less restrictions and only for intra-process communication,
see FX::FXLocalPipe. If you need less restrictions across multiple processes, see
FX::FXBlkSocket (though this is inefficient).

Default security is for FX::FXACLEntity::everything() to have full access. This
makes sense as named pipes are generally for inter-process communication -
if however it's a private pipe for communication with a known process it makes
sense to restrict access to the user running that process at least.
Note that until the pipe is opened, permissions()
returns what will be applied to the pipe on open() rather than the pipe
itself - if you want the latter, use the static method.

Like FX::FXMemMap, the pipe name is deleted by its creator on POSIX
only (on Windows it lasts until the last thing referring to it closes). If
you don't want this, specify IO_DontUnlink in the flags to open().

<h4>Usage:</h4>
To use is pretty easy - simply construct one, set its name and have your intended
server process call create(). Then have your client process call open(). Reads
block (wait indefinitely) until someone connects and sends some data. If you
set the \c isDeepPipe parameter on creation, extra deep buffers are set if supported
by the host operating system.

If a pipe should break unexpectedly, a special informational exception FX::FXConnectionLostException
is thrown which has a code of FXEXCEPTION_CONNECTIONLOST. You may or may not at this
stage wish to call reset(). For anything more than non-trivial usage, you will
need to trap and handle this in order to be handling errors correctly.

If you merely want some unique pipe for communication, use
setUnique(). This chooses a randomised name and will retry rerandomised names
until it finds one available during create(). Thereafter you can send its unique
name (retrieved by name()) to the other process to open(). Note that reset()
may rerandomise the name if it something else has claimed that name.

\warning On both POSIX and Win32, the read side of a pipe is operated in non-blocking mode
(to prevent blocks on open or breaking waitForData() on POSIX and to emulate thread
cancellation on Win32). You shouldn't ever notice this.

\warning A major portability difference is that on Win32/64, named pipes are fundamentally
broken in various minor ways plus must cope with thread cancellation. I originally spent
many months getting this code to work around all the problems and make them behave like
on Unix as much as possible - however it's possible I've missed some area. Please look at
the source of FXPipe to get some idea of the problems :(

\warning Due to issues with cancelling overlapped i/o on Windows, close() the pipe from the
same thread you last performed a read operation. Failure to do this results in free space
corruption in the heap which may or may not have the CRT library throw an assertion when
validating the MSVC heap. This bug took me two days to find, so I hope that you won't suffer the same!
*/

struct FXPipePrivate;
class FXAPIR FXPipe : public FXIODeviceS
{
	FXPipePrivate *p;
	bool creator, anonymous;
	FXPipe(const FXPipe &);
	FXPipe &operator=(const FXPipe &);
	virtual FXDLLLOCAL void *int_getOSHandle() const;
	friend class FXIPCChannel;
	void FXDLLLOCAL int_hack_makeWriteNonblocking() const;
public:
	FXPipe();
	/*! \param name Name you wish this pipe to refer to. If null, the pipe is set as anonymous
	\param isDeepPipe True if this pipe has extra-deep buffers (useful when shifting data rapidly is paramount)

	Constructs a pipe referring to \em name on the local machine
	*/
	FXPipe(const FXString &name, bool isDeepPipe=false);
	~FXPipe();

	//! The name of the pipe
	const FXString &name() const;
	//! Sets the name of the pipe. Closes the previous pipe name if open before changing the name. Also unsets anonymous.
	void setName(const FXString &name);
	//! Returns true if this pipe is unique
	bool isUnique() const { return anonymous; }
	//! Set if you want a unique pipe
	void setUnique(bool a) { anonymous=a; }

	//!	Creates the named pipe so that others can connect to it
	bool create(FXuint mode=IO_ReadWrite);
	//! Opens an existing named pipe.
	bool open(FXuint mode=IO_ReadWrite);
	//! Closes the connection
	void close();
	//! Flushes any data currently waiting to be transferred
	void flush();
	//! Resets the pipe back to original just opened state
	bool reset();

	//! Returns true if no more data to be read
	bool atEnd() const;
	//! Returns the amount of data waiting to be read. May return \c ((FXfval)-1) if there is data but it's unknown how much
	FXfval size() const;
	//! Does nothing as pipes can't be truncated
	void truncate(FXfval size);
	//! Returns 0 because pipes don't have a current file pointer
	FXfval at() const;
	//! Returns false because you can't set the current file pointer on a pipe
	bool at(FXfval newpos);
	virtual const FXACL &permissions() const;
	virtual void setPermissions(const FXACL &perms);
	//! Returns the permissions for the pipe section called \em name
	static FXACL permissions(const FXString &name);
	//! Sets the permissions for the pipe section called \em name
	static void setPermissions(const FXString &name, const FXACL &perms);

	/*! \return The number of bytes read (which may be less than requested)
	\param data Pointer to buffer to receive data
	\param maxlen Maximum number of bytes to read

	Reads a block of data from the pipe into the given buffer. Will wait forever
	until requested amount of data has been read if necessary. Is compatible
	with thread cancellation in FX::FXThread on all platforms. 
	*/
	FXuval readBlock(char *data, FXuval maxlen);
	/*! \return The number of bytes written.
	\param data Pointer to buffer of data to send
	\param len Number of bytes to send

	Writes a block of data from the given buffer to the pipe. Normally this
	will be very fast because pipes have large (4k) queues but if the queue
	is full then the call will block until there is sufficient space before
	returning.
	*/
	FXuval writeBlock(const char *data, FXuval maxlen);

	//! Tries to unread a character. Unsupported for pipes.
	int ungetch(int);
public:
	/*! Returns the maximum atomic pipe operation data length. This is the maximum
	amount of data when can be moved before the move splits into more than one
	transfer (and thus becomes less efficient)
	*/
	FXuval maxAtomicLength();
};

} // namespace

#endif
