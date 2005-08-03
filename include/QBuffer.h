/********************************************************************************
*                                                                               *
*                        i/o device working with memory                         *
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

#ifndef QBUFFER_H
#define QBUFFER_H

#include "QIODevice.h"

/*! \file QBuffer.h
\brief Defines classes used for accessing memory like a file
*/

namespace FX {

class QByteArray;

/*! \class QBuffer
\ingroup fiodevices
\brief An i/o device accessing memory (Qt compatible)

There's little more to say than that this class works with a QByteArray to
provide an i/o device working directly with memory. It auto-extends the array
if needed etc. It's also thread-safe so multiple threads can use the i/o
device at once.

Note that QBuffer ignores CR/LF translation (IO_Translate). Anything dealing
with files in TnFOX does though, so the conversion can be done then.

\note If you don't set a buffer, an internal one is created for you which is
deleted on destruction or setBuffer(). If you set your own buffer, it is never
deleted by QBuffer.

<h3>Differences from QBuffer</h3>
Since there is no reference counted sharing in TnFOX, buffer() and setBuffer()
return and take references.
*/
struct QBufferPrivate;
class FXAPIR QBuffer : public QIODevice
{
	QBufferPrivate *p;
	QBuffer(const QBuffer &);
	QBuffer &operator=(const QBuffer &);
public:
	//! Constructs a new instance using an internal QByteArray of length \em len
	explicit QBuffer(FXuval len=0);
	//! Constructs a new instance using \em buffer as the memory array
	explicit QBuffer(QByteArray &buffer);
	~QBuffer();
	//! Returns the byte array being addressed by this device.
	QByteArray &buffer() const;
	//! Sets the byte array being addressed by this device. Closes the old buffer first.
	void setBuffer(QByteArray &buffer);

	virtual bool open(FXuint mode);
	virtual void close();
	virtual void flush();
	virtual FXfval size() const;
	virtual void truncate(FXfval size);
	virtual FXuval readBlock(char *data, FXuval maxlen);
	virtual FXuval writeBlock(const char *data, FXuval maxlen);
	virtual FXuval readBlockFrom(char *data, FXuval maxlen, FXfval pos);
	virtual FXuval writeBlockTo(FXfval pos, const char *data, FXuval maxlen);
	virtual int getch();
	virtual int putch(int c);
	virtual int ungetch(int c);
	friend FXAPI FXStream &operator<<(FXStream &s, const QBuffer &i);
	friend FXAPI FXStream &operator>>(FXStream &s, QBuffer &i);
};

/*! Appends the contents of a buffer to stream \em s.
\note This is an optimised overload of QIODevice's which is also thread-safe
*/
FXAPI FXStream &operator<<(FXStream &s, const QBuffer &i);
/*! Reads all available contents of the stream \em s to a buffer, replacing its current contents.
\note This is an optimised overload of QIODevice's which is also thread-safe
*/
FXAPI FXStream &operator>>(FXStream &s, QBuffer &i);

}
#endif
