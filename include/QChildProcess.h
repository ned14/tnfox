/********************************************************************************
*                                                                               *
*                           Child Process i/o device                            *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2007 by Niall Douglas.   All Rights Reserved.            *
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


#ifndef QCHILDPROCESS_H
#define QCHILDPROCESS_H
#include "QIODeviceS.h"
#include "FXString.h"

namespace FX {

/*! \file QChildProcess.h
\brief Defines classes used to access a child process
*/

/*! \class QChildProcess
\ingroup siodevices
\ingroup IPC
\brief A child process i/o device

This handy class allows one process to portably communicate with a child process
using the standard Unix \c stdin, \c stdout and \c stderr character streams. This
can be useful for invoking compilers and other non-TnFOX command-line based programs.

QChildProcess is a \em synchronous i/o device like FX:QBlkSocket - in other words,
reads from it block until data is available. To avoid this, check atEnd() before
reading - however generally, <i>you want it</i> to block because you will have
allocated a special thread whose sole purpose is to wait on incoming data and
asynchronously do something useful with it. One action could be to post a message
to some control in the GUI for example. You should note that if you don't empty
the output stream from the child process regularly, most operating systems will
suspend the child thread until you do - to achieve this, at a minimum you need
to call FX::QIODeviceS::waitForData() regularly.

The two readable streams, \c stdout and \c stderr, are read simultaneously and
stored internally by QChildProcess. This allows you to read say only \c stdout
and the \c stderr pipe won't get blocked up. On close() (or destruction), the
calling thread blocks until the child process exits. The child process will be forcibly
terminated if the parent process should die for any reason before close() - this
ensures zombie child processes don't multiply. If you want to remove the child
from management, call detach().

If a child process should hang, you can wait for a certain time period for it
to produce output or to exit and if not then invoke terminate().

<h4>Usage:</h4>
To use is pretty easy - simply construct one and set the path to the command you
desire. Calling open() invokes the process, and thereafter readBlock() and writeBlock()
work as expected. Calling close() will wait for the process to terminate before
returning. Should the process exit, readBlock() will start returning zero bytes
instead of blocking for more data to arrive.

Unix programs output two streams: \c stdout and \c stderr. You can switch between
which is active, or whether to merge them, as the read channel using setReadChannel().
*/

struct QChildProcessPrivate;
class FXAPIR QChildProcess : public QIODeviceS
{
	QChildProcessPrivate *p;
	QChildProcess(const QChildProcess &);
	QChildProcess &operator=(const QChildProcess &);
	virtual FXDLLLOCAL void *int_getOSHandle() const;
	FXDLLLOCAL void int_killChildI(bool);
	FXDLLLOCAL bool int_suckPipesDry(bool block=false);
public:
	//! The types of read channel
	enum ReadChannel
	{
		StdOut=1,		//!< \c stdout is the read channel
		StdErr=2,		//!< \c stderr is the read channel
		Combined=3		//!< \c A combination of \c stdout and \c stderr is the read channel
	};
public:
	QChildProcess();
	//! Constructs an instance executing \em command
	QChildProcess(const FXString &command, const FXString &args=FXString::nullStr(), ReadChannel channel=Combined);
	~QChildProcess();

	//! The command to be executed
	const FXString &command() const;
	//! Sets the command to be executed
	void setCommand(const FXString &command);
	//! The arguments for the command to be executed
	FXString arguments() const;
	//! Sets the arguments for the command to be executed
	void setArguments(const FXString &args);
	//! The working directory for the command
	FXString workingDir() const;
	//! Sets the working directory for the command to be executed
	void setWorkingDir(const FXString &dir);
	//! Returns the channel to be used for reading
	ReadChannel readChannel() const throw();
	//! Sets the channel to be used for reading
	void setReadChannel(ReadChannel channel);

	//! Returns the return code of the process (only valid after close())
	FXlong returnCode() const throw();
	//! Detaches the process
	void detach();
	//! Waits for the process to end
	bool waitForExit(FXuint waitfor=FXINFINITE);
	//! Ensures the process is dead by exit, returning true if it was terminated (not exited naturally)
	bool terminate();
private:
	//!	Disabled, throws an exception if you call it
	bool create(FXuint mode=IO_ReadWrite);
public:
	//! Executes the command.
	bool open(FXuint mode=IO_ReadWrite);
	//! Waits for the command to exit.
	void close();
	//! Flushes any data currently waiting to be transferred
	void flush();

	//! Returns the amount of data waiting to be read. May return \c ((FXfval)-1) if there is data but it's unknown how much
	FXfval size() const;

	/*! \return The number of bytes read (which may be less than requested)
	\param data Pointer to buffer to receive data
	\param maxlen Maximum number of bytes to read

	Reads a block of data from the command's output into the given buffer.
	Will wait forever until some data has been read if necessary. Is compatible
	with thread cancellation in FX::QThread on all platforms. 
	*/
	FXuval readBlock(char *data, FXuval maxlen);
	/*! \return The number of bytes written.
	\param data Pointer to buffer of data to send
	\param maxlen Number of bytes to send

	Writes a block of data from the given buffer to the command. If the command
	isn't reading in from its input quickly enough, this may block.
	*/
	FXuval writeBlock(const char *data, FXuval maxlen);

	//! Tries to unread a character. Unsupported.
	int ungetch(int);
};

} // namespace

#endif
