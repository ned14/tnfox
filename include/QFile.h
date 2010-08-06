/********************************************************************************
*                                                                               *
*                                File i/o device                                *
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

#ifndef QFile_H
#define QFile_H

#include "QIODevice.h"
#include "FXTime.h"

/*! \file QFile.h
\brief Defines items and classes used for accessing files
*/

/// Declared as "C" so as to not clash tag-names
extern "C" { struct stat; }

namespace FX {

/*! \class QFile
\ingroup fiodevices
\brief An i/o device accessing the filing system directly (Qt compatible)

Not much to say about this - it works as you'd expect. It's also thread-safe
so multiple threads can read and write from it (though the current file pointer
is the same for both, so it probably doesn't help you much).

Most likely you'll prefer to use FX::QMemMap all the time as it offers
superior performance and facilities in most cases. Indeed, QFile does no
internal buffering as it is expected it will only be used rarely, mostly
for small files with little i/o performed to them.

Like all file type i/o classes, QFile can perform automatic CR/LF translation
as well as UTF-8 to UTF-16 and UTF-32 conversion. If you enable \c IO_Translate,
the file data is probed and its unicode type determined such that the file
unicode type is transparently converted into UTF-8 and back into its original
form. This allows your code to work exclusively in UTF-8 using the standard
FX::FXString functions. You can set the type of output using
setUnicodeTranslation().

For speed, QFile maintains its own record of file length which it manages.
This normally isn't a problem, but when multiple QFile's are working on the
same file (either in-process or across processes) then the internal count
can become desynchronised with the actual length. If you want to reset the
length, call reloadSize().

One major difference is default security, especially on NT. QFile like
all TnFOX sets very conservative permissions on all things it creates -
see FX::FXACL::default_(). Note that until the file is opened, permissions()
returns what will be applied to the file on open() rather than the file
itself - if you want the latter, use the static method.

\warning Try not to call atEnd() too much. Because of limitations in POSIX
it works by reading a byte which if successful means not EOF and a ungetch()
or moving the file pointer back!
*/
class QFilePrivate;
class QMemMap;
class FXAPIR QFile : public QIODevice
{
	QFilePrivate *p;
	QFile(const QFile &);
	QFile &operator=(const QFile &);
	struct WantStdioType
	{
		typedef int foo;
	};
	struct WantLightQFile { };
	QFile(WantStdioType);
	QFile(const FXString &name, WantLightQFile);
	friend class QMemMap;
	friend class FXProcess;
	FXDLLLOCAL int int_fileDescriptor() const;
public:
	QFile();
	//! Constructs a new instance, setting the file name to be used to \em name
	QFile(const FXString &name);
	~QFile();
	//! Returns the file name being addressed by this device
	const FXString &name() const;
	//! Sets the file name being addressed by this device. Closes the old file first.
	void setName(const FXString &name);
	//! Returns true if the file name points to an existing file
	bool exists() const;
	//! Deletes the file name, closing the file first if open. Returns false if file doesn't exist
	bool remove();
	//! Reloads the size of the file. See description above
	FXfval reloadSize();
	/*! Returns an QIODevice referring to stdin/stdout. This is somewhat of a special device
	in that it can't be closed, doesn't have a size and reads from it can block.
	\sa QPipe
	*/
	static QIODevice &stdio(bool applyCRLFTranslation=false);

	using QIODevice::mode;
	virtual bool open(FXuint mode);
	virtual void close();
	virtual void flush();
	virtual FXfval size() const;
	virtual void truncate(FXfval size);
	virtual FXfval at() const;
	virtual bool at(FXfval newpos);
	virtual bool atEnd() const;
	virtual const FXACL &permissions() const;
	virtual void setPermissions(const FXACL &perms);
	//! Returns the permissions for the file at the specified path
	static FXACL permissions(const FXString &path);
	//! Sets the permissions for the file at the specified path
	static void setPermissions(const FXString &path, const FXACL &perms);
	virtual FXuval readBlock(char *data, FXuval maxlen);
	virtual FXuval writeBlock(const char *data, FXuval maxlen);
	virtual FXuval readBlockFrom(char *data, FXuval maxlen, FXfval pos);
	virtual FXuval writeBlockTo(FXfval pos, const char *data, FXuval maxlen);
	virtual int ungetch(int c);
};

}

#endif
