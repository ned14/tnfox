/********************************************************************************
*                                                                               *
*                          Information about a file                             *
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

#ifndef QFILEINFO_H
#define QFILEINFO_H

#include "FXString.h"
#include "QDir.h"
#include "FXACL.h"

namespace FX {

/*! \file QFileInfo.h
\brief Defines classes used to detail a file entry
*/

class FXFile;

/*! \class QFileInfo
\brief Provides detailed information about an entry in the file system (Qt compatible)

While most of what is offered by this class can be done manually via the static methods
in FX::FXFile, it can be useful to have a container knowing everything there is to know
about an entry in a file system, particularly for purposes of comparison.

Like Qt's QFileInfo, this class also caches its information by default and thus after
construction or \c refresh(), queries are far quicker (this is the default). API
compatibility with QFileInfo has been mostly
maintained - however, the old system of permissions has been replaced with an ACL based
one so that NT file permissions are available. On Linux or BSD, suitable entries are
created to reflect the much simpler POSIX security model though it could be easily
extended to reflect the ACL security implemented by SE-Linux. For these reasons, I
recommend you use QFileInfo over FXFile directly (it's just as efficient at worst as
the original FOX code does a \c stat() even where on Win32 there's a separate faster
call - except for \c isFile() and \c isDirectory()). Note that if FX::FXACL throws an
exception when reading the file entry's security info (this could happen if the file
entry does not give you permission to read its ACL), QFileInfo will simply create an
empty ACL belonging to FX::FXACLEntity::root() for that file entry.

If you wish to enumerate the contents of a directory, please see FX::QDir. The
default operators <, > and == compare by case insensitive name.
*/
struct QFileInfoPrivate;
class FXAPIR QFileInfo
{
	QFileInfoPrivate *p;
public:
	//! Constructs a new instance
	QFileInfo();
	//! Constructs a new instance detailing \em path
	QFileInfo(const FXString &path);
	//! Constructs a new instance detailing the file used by the FXFile
	QFileInfo(const FXFile &file);
	//! Constructs a new instance detailing the file \em leafname within directory \em dir
	QFileInfo(const QDir &dir, const FXString &leafname);
	QFileInfo(const QFileInfo &o);
	QFileInfo &operator=(const QFileInfo &o);
	~QFileInfo();
	bool operator<(const QFileInfo &o) const;
	bool operator==(const QFileInfo &o) const;
	bool operator!=(const QFileInfo &o) const { return !(*this==o); }
	bool operator>(const QFileInfo &o) const;

	//! Sets the detail to use \em path
	void setFile(const FXString &path);
	//! Sets the detail to use \em file
	void setFile(const FXFile &file);
	//! Sets the detail to use \em leafname in \em dir
	void setFile(const QDir &dir, const FXString &leafname);
	//! Returns true if the filing system entry exists
	bool exists() const;
	//! Refreshes the information if caching is on
	void refresh();
	//! True if caching is enabled
	bool caching() const;
	//! Sets if caching is enabled
	void setCaching(bool newon);
	//! Returns the path + leafname of the filing system entry
	const FXString &filePath() const;
	//! Returns the leafname of the entry
	FXString fileName() const;
	//! Returns the absolute path of the entry
	FXString absFilePath() const;
	/*! Returns the base name of the entry (name before the '.').
	If \em complete is true, returns up to last dot else returns up
	to first dot */
	FXString baseName(bool complete=false) const;
	/*! Returns the extension of the entry (name after the '.').
	If \em complete is true, returns from first dot else returns from
	last dot */
	FXString extension(bool complete=true) const;
	//! Returns the entry's path, an absolute path if \em absPath is true
	FXString dirPath(bool absPath=false) const;
	//! Returns the entry's path as a FX::QDir
	QDir dir(bool absPath=false) const { return QDir(absPath ? absFilePath() : fileName()); }
	//! Returns true if the entry is readable
	bool isReadable() const;
	//! Returns true if the entry is writeable
    bool isWriteable() const;
	//! \overload
	bool isWritable() const { return isWriteable(); }
	//! Returns true if the entry is an executable
    bool isExecutable() const;
	//! Returns true if the entry is hidden
	bool isHidden() const;
	//! Returns true if the entry's path is relative
    bool isRelative() const;
	//! Converts the entry's path to an absolute one
	bool convertToAbs();
	//! Returns true if the entry is a file
    bool isFile() const;
	//! Returns true if the entry is a directory
	bool isDir() const;
	//! Returns true if the entry is a symbolic link
	bool isSymLink() const;
	//! Returns what the entry (when a symbolic link) points towards
	FXString readLink() const;
	//! Returns the owner of this entry (alias for permissions().owner())
	const FXACLEntity &owner() const;
	//! Returns the permissions for the entry
	const FXACL &permissions() const;
	//! Returns true if the file permits \em what (alias for permissions().check())
	bool permission(FXACL::Perms what) const;
	//! Returns as a string the permissions for the entry
	FXString permissionsAsString() const;
	//! Returns the size of the entry
	FXfval size() const;
	//! Returns the size of the entry as a string (eg; 1.2Mb)
	FXString sizeAsString() const;
	//! Returns the datestamp of when the entry was created (Windows only).
	FXTime created() const;
	//! Returns the datestamp formatted as a string (uses a \c strftime format)
	FXString createdAsString(const FXString &format="%Y/%b/%d %H:%M:%S") const;
	//! Returns the datestamp of when the entry's data was last modified
    FXTime lastModified() const;
	//! Returns the datestamp formatted as a string (uses a \c strftime format)
	FXString lastModifiedAsString(const FXString &format="%Y/%b/%d %H:%M:%S") const;
	//! Returns the datestamp of when the entry was last accessed
	FXTime lastRead() const;
	//! Returns the datestamp formatted as a string (uses a \c strftime format)
	FXString lastReadAsString(const FXString &format="%Y/%b/%d %H:%M:%S") const;
	/*! Returns the datestamp of when the entry was last changed (POSIX only).
	In case you're wondering what the difference from lastModified() is,
	last changed is updated when anything is changed. lastModified() is
	updated only when the data is modified and lastRead() when the data
	was last opened for access.
	*/
	FXTime lastChanged() const;
	//! Returns the datestamp formatted as a string (uses a \c strftime format)
	FXString lastChangedAsString(const FXString &format="%Y/%b/%d %H:%M:%S") const;
	//! Returns true if the path is hidden
	static bool isHidden(const FXString &path);
};

} // namespace

#endif
