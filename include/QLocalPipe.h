/********************************************************************************
*                                                                               *
*                            Local pipe i/o device                              *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2003-2006 by Niall Douglas.   All Rights Reserved.       *
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


#ifndef QLOCALPIPE_H
#define QLOCALPIPE_H
#include "QIODeviceS.h"

namespace FX {

/*! \file QLocalPipe.h
\brief Defines classes used to provide a process local pipe
*/

/*! \class QLocalPipe
\ingroup siodevices
\brief A process-local pipe

This is a synchronous pipe i/o device operated completely within the local
process and is intended as the primary method for threads to communicate
with each other. It is extremely efficient, threadsafe and has no effective
pipe buffer length as it uses the freestore.

See FX::QPipe for more information about how such objects work. One
difference that there is no create(), instead there is an
otherEnd() method which returns an instance of QLocalPipe reflecting
the other end of the local pipe.

Note that on some platforms FX::QLocalPipe can appear to be slower than
a system pipe (the i/o test shows this). I can't explain this except to
say that if you set the granularity to 512Kb you get the same speed but
at 1Mb sequential performance suddenly doubles - so I'm assuming that
the MSVC6 STL \c <vector> implementation is crossing over some boundary
where it reallocates every time. One thing my tests \b do confirm is that
QLocalPipe has a \em much lower latency than FX::QPipe which means
anything working without a sliding window will benefit a lot.
*/

struct QLocalPipePrivate;
class FXAPIR QLocalPipe : public QIODeviceS
{
	friend struct QLocalPipePrivate;
	QLocalPipePrivate *p;
	bool creator;
	QLocalPipe &operator=(const QLocalPipe &);
	virtual void *int_getOSHandle() const;
public:
	//! Constructs a process-local pipe
	QLocalPipe();
	//! The copy is the other end of the pipe ie; effectively the same as clientEnd()
	QLocalPipe(const QLocalPipe &o);
	~QLocalPipe();

	//! Returns the granularity of memory allocation. The default is 64Kb.
	FXuval granularity() const;
	//! Sets the granularity of memory allocation. Set only when the device is closed.
	void setGranularity(FXuval newval);
	//!	Returns an instance of this local pipe reflecting the client end of the local pipe
	QLocalPipe clientEnd() const { return *this; }
	//! Opens the local pipe for usage
	bool create(FXuint mode=IO_ReadWrite) { return open(mode); }
	//! \overload
	bool open(FXuint mode=IO_ReadWrite);
	//! Closes the connection. Note that any data still waiting to be read is lost (use flush() before if you don't want this)
	void close();
	//! Flushes any data currently waiting to be transferred
	void flush();

	//! Returns true if no more data to be read
	bool atEnd() const;
	//! Returns the amount of data waiting to be read
	FXfval size() const;
	//! Does nothing as pipes can't be truncated
	void truncate(FXfval size);
	//! Returns 0 because pipes don't have a current file pointer
	FXfval at() const;
	//! Returns false because you can't set the current file pointer on a pipe
	bool at(FXfval newpos);

	/*! \return The number of bytes read (which may be less than requested)
	\param data Pointer to buffer to receive data
	\param maxlen Maximum number of bytes to read

	Reads a block of data from the pipe into the given buffer. Will wait forever
	until requested amount of data has been read if necessary. Is compatible
	with thread cancellation in FX::QThread on all platforms. 
	*/
	FXuval readBlock(char *data, FXuval maxlen);
	/*! \return The number of bytes written.
	\param data Pointer to buffer of data to send
	\param maxlen Number of bytes to send

	Writes a block of data from the given buffer to the pipe. This is instantaneous.
	*/
	FXuval writeBlock(const char *data, FXuval maxlen);

	//! Tries to unread a character. Unsupported for pipes.
	int ungetch(int);
};

} // namespace

#endif
