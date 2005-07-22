/********************************************************************************
*                                                                               *
*                          Filter device applying .gz                           *
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

#ifndef QGZIPDEVICE_H
#define QGZIPDEVICE_H

#include "QIODevice.h"

namespace FX {

/*! \file QGZipDevice.h
\brief Defines classes used in translating .gz files
*/

/*! \class QGZipDevice
\ingroup fiodevices
\brief Provides a filter i/o device which transparently translates .gz files

This is a useful little class which shows the power of the TnFOX i/o class
structure. Quite simply, it takes a QIODevice which accesses the .gz file
and provides the decompressed version to anything using it. You can use
it for reading or writing.

You can especially combine this with QTrans translation files to markedly
reduce their size and loading time. Simply attach the i/o device accessing
the translation file to an instance of this class using setGZData(). Then
read from this instance instead.

<h3>Implementation notes:</h3>
Since the process of inflation and deflation is slow, the class internally
decompresses to a QBuffer on open() and all work is done to and from this
buffer. Only on close() or flush() is the data in the buffer gzipped back
to the gzdata source.

The source if not already open is opened on open() - if being reopened
it does not reset the file pointer so ensure it's at the right place.
After open() it leaves the file pointer pointing after the .gz data.

On close() or flush(), the file pointer is first set to zero and after
writing a truncate() is issued to the source to remove any extraneous data.
If opened as read only or write only, the device behaves correctly and
doesn't issue anything incorrect (or at least it shouldn't!). QGZipDevice
never closes the source when you close() it.

\note You need the zlib library available for this to work. If it's missing
an exception is generated if you try to use it.

This class uses the zlib library and hacked together parts of its sample code
(C) 1995-1998 Jean-loup Gailly and Mark Adler
*/
struct QGZipDevicePrivate;
class FXAPIR QGZipDevice : public QIODevice
{
	QGZipDevicePrivate *p;
	QGZipDevice(const QGZipDevice &);
	QGZipDevice &operator=(const QGZipDevice &);
public:
	QGZipDevice(QIODevice *gzdata=0);
	~QGZipDevice();
	//! Returns the device being used as .gz source
	QIODevice *GZData() const;
	//! Sets the device being usied as .gz source
	void setGZData(QIODevice *gzdata);

	virtual bool open(FXuint mode);
	virtual void close();
	virtual void flush();
	virtual FXfval size() const;
	virtual void truncate(FXfval size);
	virtual FXfval at() const;
	virtual bool at(FXfval newpos);
	virtual bool atEnd() const;
	virtual FXuval readBlock(char *data, FXuval maxlen);
	virtual FXuval writeBlock(const char *data, FXuval maxlen);
	virtual FXuval readBlockFrom(char *data, FXuval maxlen, FXfval pos);
	virtual FXuval writeBlockTo(FXfval pos, const char *data, FXuval maxlen);
	virtual int getch();
	virtual int putch(int c);
	virtual int ungetch(int c);
};

} // namespace

#endif
