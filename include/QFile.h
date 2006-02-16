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
internal buffering as it is expected it will only be used rarely.

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

\note I folded FOX's QFile namespace functions into this QFile as static
methods. This should maintain compatibility with FOX applications

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

#if 0
// Fix this up into the parts FOX now has later

public:


	// These are directly copied from FOX

	/// Get current user name
	static FXString  getCurrentUserName();

	/// Return value of environment variable name
	static FXString  getEnvironment(const FXString& name);

	/// Return the home directory for the current user.
	static FXString  getHomeDirectory();

	/*! Return the home directory for a given user.
	\deprecated This call doesn't work correctly, use FX::FXACLEntity::homeDirectory()
	instead */
	static FXDEPRECATEDEXT FXString  getUserDirectory(const FXString& user);

	/// Return temporary directory.
	static FXString  getTempDirectory();

	/// Set the current working directory
	static FXbool  setCurrentDirectory(const FXString& path);

	/// Get the current working directory
	static FXString  getCurrentDirectory();

	/// Set the current drive (for Win32 systems)
	static FXbool  setCurrentDrive(const FXString& prefix);

	/// Return the current drive (for Win32 systems)
	static FXString  getCurrentDrive();

	/// Get executable path
	static FXString  getExecPath();

	/**
	* Return the directory part of the path name.
	* Note that directory("/bla/bla/") is "/bla/bla" and NOT "/bla".
	* However, directory("/bla/bla") is "/bla" as we expect!
	*/
	static FXString  directory(const FXString& file);

	/**
	* Return name and extension part of the path name.
	* Note that name("/bla/bla/") is "" and NOT "bla".
	* However, name("/bla/bla") is "bla" as we expect!
	*/
	static FXString  name(const FXString& file);

	/// Return file title, i.e. document name only
	static FXString  title(const FXString& file);

	/// Return extension part of the file name
	static FXString  extension(const FXString& file);

	/// Return file name less the extension
	static FXString  stripExtension(const FXString& file);

	/// Return the drive letter prefixing this file name (if any).
	static FXString  drive(const FXString& file);

	/// Perform tilde or environment variable expansion
	static FXString  expand(const FXString& file);

	/**
	* Simplify a file path; the path will remain relative if it was relative,
	* or absolute if it was absolute.  Also, a trailing "/" will be preserved
	* as this is important in other functions.
	* For example, simplify("..//aaa/./bbb//../c/") becomes "../aaa/c/".
	*/
	static FXString  simplify(const FXString& file);

	/// Return absolute path from current directory and file name
	static FXString  absolute(const FXString& file);

	/// Return absolute path from base directory and file name
	static FXString  absolute(const FXString& base,const FXString& file);

	/// Return relative path of file to the current directory
	static FXString  relative(const FXString& file);

	/// Return relative path of file to given base directory
	static FXString  relative(const FXString& base,const FXString& file);

	/**
	* Return root of absolute path; on Unix, this is just "/". On
	* Windows, this is "\\" or "C:\".  Returns the empty string
	* if the given path is not absolute.
	*/
	static FXString  root(const FXString& file);

	/// Enquote filename to make safe for shell
	static FXString  enquote(const FXString& file,FXbool forcequotes=FALSE);

	/// Dequote filename to get original again
	static FXString  dequote(const FXString& file);

	/**
	* Generate unique filename of the form pathnameXXX.ext, where
	* pathname.ext is the original input file, and XXX is a number,
	* possibly empty, that makes the file unique.
	*/
	static FXString  unique(const FXString& file);

	/// Search path list for this file, return full path name for first occurrence
	static FXString  search(const FXString& pathlist,const FXString& file);

	/// Return path to directory above input directory name
	static FXString  upLevel(const FXString& file);

	/// Return true if file name is absolute
	static FXbool  isAbsolute(const FXString& file);

	/// Return true if input directory is a top-level directory
	static FXbool  isTopDirectory(const FXString& file);

	/// Return true if input path is a file name. Implemented as readMetadata()
	static FXbool  isFile(const FXString& file);

	/// Return true if input path is a link. Implemented as readMetadata()
	static FXbool  isLink(const FXString& file);

	/// Return true if input path is a directory. Implemented as readMetadata()
	static FXbool  isDirectory(const FXString& file);

	/// Return true if input path is a file share
	static FXbool  isShare(const FXString& file);

	/// Return true if file is readable
	static FXbool  isReadable(const FXString& file);

	/// Return true if file is writable
	static FXbool  isWritable(const FXString& file);

	/// Return true if file is executable
	static FXbool  isExecutable(const FXString& file);

	/// Return true if owner has read-write-execute permissions
	static FXDEPRECATEDEXT FXbool  isOwnerReadWriteExecute(const FXString& file);

	/// Return true if owner has read permissions
	static FXDEPRECATEDEXT FXbool  isOwnerReadable(const FXString& file);

	/// Return true if owner has write permissions
	static FXDEPRECATEDEXT FXbool  isOwnerWritable(const FXString& file);

	/// Return true if owner has execute permissions
	static FXDEPRECATEDEXT FXbool  isOwnerExecutable(const FXString& file);

	/// Return true if group has read-write-execute permissions
	static FXDEPRECATEDEXT FXbool  isGroupReadWriteExecute(const FXString& file);

	/// Return true if group has read permissions
	static FXDEPRECATEDEXT FXbool  isGroupReadable(const FXString& file);

	/// Return true if group has write permissions
	static FXDEPRECATEDEXT FXbool  isGroupWritable(const FXString& file);

	/// Return true if group has execute permissions
	static FXDEPRECATEDEXT FXbool  isGroupExecutable(const FXString& file);

	/// Return true if others have read-write-execute permissions
	static FXDEPRECATEDEXT FXbool  isOtherReadWriteExecute(const FXString& file);

	/// Return true if others have read permissions
	static FXDEPRECATEDEXT FXbool  isOtherReadable(const FXString& file);

	/// Return true if others have write permissions
	static FXDEPRECATEDEXT FXbool  isOtherWritable(const FXString& file);

	/// Return true if others have execute permissions
	static FXDEPRECATEDEXT FXbool  isOtherExecutable(const FXString& file);

	/// Return true if the file sets the user id on execution
	static FXDEPRECATEDEXT FXbool  isSetUid(const FXString& file);

	/// Return true if the file sets the group id on execution
	static FXDEPRECATEDEXT FXbool  isSetGid(const FXString& file);

	/// Return true if the file has the sticky bit set
	static FXDEPRECATEDEXT FXbool  isSetSticky(const FXString& file);

	/// \deprecated Use permissions() instead
	static FXDEPRECATEDEXT FXString  owner(FXuint uid);

	/// \deprecated Use permissions() instead
	static FXDEPRECATEDEXT FXString  owner(const FXString& file);

	/// \deprecated Use permissions() instead
	static FXDEPRECATEDEXT FXString  group(FXuint gid);

	/// \deprecated Use permissions() instead
	static FXDEPRECATEDEXT FXString  group(const FXString& file);

	/// \deprecated Use permissions() instead
	static FXDEPRECATEDEXT FXString  permissions(FXuint mode);

	/// Return file size in bytes. Implemented as readMetadata()
	static FXfval  size(const FXString& file);

/**
* Return last modified time for this file, on filesystems
* where this is supported.  This is the time when any data
* in the file was last modified. Implemented as readMetadata()
*/
	static FXTime  modified(const FXString& file);

/**
* Return last accessed time for this file, on filesystems
* where this is supported. Implemented as readMetadata()
*/
	static FXTime  accessed(const FXString& file);

/**
* Return created time for this file, on filesystems
* where this is supported.  This is also the time when
* ownership, permissions, links, and other meta-data may
* have changed. Implemented as readMetadata()
*/
	static FXTime  created(const FXString& file);


	/// Match filenames using *, ?, [^a-z], and so on
	static FXbool  match(const FXString& pattern,const FXString& file,FXuint flags=(FILEMATCH_NOESCAPE|FILEMATCH_FILE_NAME));

	/**
	* List files in a given directory.
	* Returns the number of files in the string-array list which matched the
	* pattern or satisfied the flag conditions.
	*/
	static FXint  listFiles(FXString*& filelist,const FXString& path,const FXString& pattern="*",FXuint flags=LIST_MATCH_ALL);

	/// Return current time
	static FXDEPRECATEDEXT FXTime  now() { return FXTime::now(); }

	/// Convert file time to date-string
	static FXDEPRECATEDEXT FXString  time(FXTime filetime) { return filetime.toLocalTime().asString("%m/%d/%Y %H:%M:%S"); }

	/**
	* Convert file time to date-string as per strftime.
	* Format characters supported by most systems are:
	*
	*  %a %A %b %B %c %d %H %I %j %m %M %p %S %U %w %W %x %X %y %Y %Z %%
	*
	* Some systems support additional conversions.
	*/
	static FXDEPRECATEDEXT FXString  time(const FXchar *format,FXTime filetime) { return filetime.toLocalTime().asString(format); }

	/// Return file info as reported by system stat() function
	static FXbool  info(const FXString& file,struct stat& inf);

	/// Return file info as reported by system lstat() function
	static FXbool  linkinfo(const FXString& file,struct stat& inf);

	/// Return true if file exists
	static FXbool  exists(const FXString& file);

	/// Return true if files are identical
	static FXbool  identical(const FXString& file1,const FXString& file2);

	/// Return the mode flags for this file
	static FXuint  mode(const FXString& file);

	/// Change the mode flags for this file
	static FXbool  mode(const FXString& file,FXuint mode);

	/// Create new directory
	static FXbool  createDirectory(const FXString& path,FXuint mode);

	/// Create new (empty) file
	static FXbool  createFile(const FXString& file,FXuint mode);

	/**
	* Concatenate srcfile1 and srcfile2 to a dstfile.
	* If overwrite is true, then the operation fails if dstfile already exists.
	* srcfile1 and srcfile2 should not be the same as dstfile.
	*/
	static FXbool  concatenate(const FXString& srcfile1,const FXString& srcfile2,const FXString& dstfile,FXbool overwrite=FALSE);

	/// Remove file or directory, recursively.
	static FXbool  remove(const FXString& file);

	/// Copy file or directory, recursively
	static FXbool  copy(const FXString& srcfile,const FXString& dstfile,FXbool overwrite=FALSE);

	/// Rename or move file or directory
	static FXbool  move(const FXString& srcfile,const FXString& dstfile,FXbool overwrite=FALSE);

	/// Link file
	static FXbool  link(const FXString& srcfile,const FXString& dstfile,FXbool overwrite=FALSE);

	/// Symbolic link file
	static FXbool  symlink(const FXString& srcfile,const FXString& dstfile,FXbool overwrite=FALSE);

	/// Read symbolic link
	static FXString  symlink(const FXString& file);

};
#endif

}

#endif
