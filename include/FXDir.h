/********************************************************************************
*                                                                               *
*                        Information about a directory                          *
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

#ifndef FXDIR_H
#define FXDIR_H

#include "FXString.h"
#include "qstringlist.h"
#undef mkdir

namespace FX {

/*! \file FXDir.h
\brief Defines classes used to detail a directory entry
*/

class FXFileInfo;
typedef QValueList<FXFileInfo> QFileInfoList;

/*! \class FXDir
\brief Provides detailed information about a directory in the file system (Qt compatible)

This class permits you to enumerate the contents of a directory in a very flexible
fashion. It works in tandem with FX::FXFileInfo to provide the ultimate in both
directory enumeration and comparing before & after snapshots of directory contents.

\warning The wildcard mechanism used by FXDir is not the same as QDir. QDir uses a
space-separated set of *.x as a file open dialog would use. As Tn has disposed
permanently of file open dialogs, FOX-style wildcards were far more useful to me - sorry!

FOX-style wildcards are as follows: ? matches a single char, * matches many, [...]
specifies any one of what's between the square brackets. Ranges work, so [1-4] matches
any one of 1,2,3,4. [!1-4] or [^1-4] would be any char not 1,2,3,4. Lists of matchable
expressions can be delimited using | or , so *.cpp|*.h,*.cxx means anything with
the extensions cpp,h or cxx. Brackets nest expressions, so foo.(a,b,c) matches
foo.a, foo.b and foo.c. You can escape any of these special characters with \

You should (like QDir) try to avoid fancy filtering as it's slow - often getting
the full list and copying the items you want into a new list based upon a speculative
retrieval of FXFileInfo's can be faster than
running a regular expression match on a subset filter though care has been taken
to avoid quadratic behaviour (unlike Qt, which goes real slow on large directories).
In particular, setting FilterSpec or SortSpec options which require fetching a
FXFileInfo for each item can be particularly slow. Note that all expression matches
are case insensitive - this can cause too much data to be returned on POSIX, but
also means consistent behaviour between Windows and POSIX.

FXDir like QDir performs a certain amount of caching - the list of names are cached.
If a list of FX::FXFileInfo's is requested, the cached list of names are used to
generate the list of FX::FXFileInfo's - which may mean an incomplete listing. If
the directory enumerated has vanished altogether, then a null pointer is returned.
The static method extractChanges() is additional functionality over Qt.

Note that as with all TnFOX functionality, if something fails it throws an exception.
Thus many of the boolean returns are always true.
*/
struct FXDirPrivate;
class FXAPIR FXDir
{
	FXDirPrivate *p;
public:
	/*! Combine bitwise to specify what to include in the enumeration. Not
	setting any of Readable, Writeable nor Executable causes all three to
	become set (fastest)
	*/
	enum FilterSpec {
		Dirs      =0x001,	//!< Include directories
		Files     =0x002,	//!< Include files
		//Drives    =0x004,	//!< Include drives
		NoSymLinks=0x008,	//!< Don't include symbolic links
		All       =0x007,	//!< Include everything except hidden (fastest)
		TypeMask  =0x00f,

		Readable  =0x010,	//!< Include readable items
		Writeable =0x020,	//!< Include writeable items
		Writable  =0x020,
		Executable=0x040,	//!< Include executable items
		RWEMask   =0x070,

		//Modified  =0x080,
		Hidden    =0x100,	//!< Include hidden items (fastest)
		//System    =0x200,
		AccessMask=0x3f0,

		DefaultFilter=0xffffffff
	};
	//! Combine bitwise to specify how to sort the enumeration
    enum SortSpec
	{
		Name      =0x00,	//!< By name
		Time      =0x01,	//!< By modification time
		Size      =0x02,	//!< By size
		Unsorted  =0x03,	//!< Unsorted (fastest)
		SortByMask=0x03,

		DirsFirst =0x04,	//!< Directories first
		Reversed  =0x08,	//!< Reversed
		IgnoreCase=0x10,	//!< Ignore case

		DefaultSort=0xffffffff
	};
	//! Constructs a new instance
	FXDir();
	/*! Constructs a new instance enumerating items matching regular expression
	\em regex in the directory \em path, sorted by \em sortBy and filtered by \em filter */
	FXDir(const FXString &path, const FXString &regex=FXString::nullStr(), int sortBy=Name|IgnoreCase, int filter=All);
	FXDir(const FXDir &o);
	FXDir &operator=(const FXDir &o);
	FXDir &operator=(const FXString &path);
	~FXDir();
	//! Note that if the fileinfo list has not been generated in both, it is not compared
	bool operator==(const FXDir &o) const;
	//! \overload
	bool operator!=(const FXDir &o) const;

	//! Sets the path being enumerated
	void setPath(const FXString &path);
	//! Returns the path being enumerated
	const FXString &path() const;
	//! Returns the absolute path being enumerated
	FXString absPath() const;
	//! Returns the path being enumerated simplified (links followed and ..'s removed)
	FXString canonicalPath() const;
	//! Returns the name of the directory being enumerated
	FXString dirName() const;
	//! Returns path of \em file if it were in this directory
	FXString filePath(const FXString &file, bool acceptAbs=true) const;
	//! Returns absolute path of \em file if it were in this directory
	FXString absFilePath(const FXString &file, bool acceptAbs=true) const;
	//! Returns the same path but with '\' instead of '/' if running on Windows
	static FXString convertSeparators(const FXString &path);
	//! Change the directory, returning true if the new location exists
	bool cd(const FXString &name, bool acceptAbs=true);
	//! Changes the directory up one level, returning true if the new location exists
	bool cdUp();

	//! Returns the regular expression filtering the enumeration
	const FXString &nameFilter() const;
	//! Sets the regular expression filtering the enumeration
	void setNameFilter(const FXString &regex);
	//! Returns the filter setting
	FilterSpec filter() const;
	//! Sets the filter setting
	void setFilter(int filter);
	//! Returns the sorting setting
	SortSpec sorting() const;
	//! Sets the sorting setting
	void setSorting(int sorting);
	//! Returns true if all directories are enumerated irrespective of filter
	bool matchAllDirs() const;
	//! Sets if all directories are enumerated irrespective of filter
	void setMatchAllDirs(bool matchAll);

	//! Returns how many items are in the enumeration
	FXuint count() const;
	//! Returns the item at index \em idx
	const FXString &operator[](int idx) const;
	//! Returns a string list containing all the items in the enumeration which match \em regex, \em filter and \em sorting
	QStringList entryList(const FXString &regex, int filter=DefaultFilter, int sorting=DefaultSort);
	//! Returns a string list containing all the items in the enumeration which match \em filter and \em sorting
	QStringList entryList(int filter=DefaultFilter, int sorting=DefaultSort) { return entryList(FXString::nullStr(), filter, sorting); }
	/*! Returns a list of all the items in the enumeration like entryList()
	but as FX::FXFileInfo's. Needless to say, this call is not as fast as entryList()
	and it returns zero if the directory does not exist */
	const QFileInfoList *entryInfoList(const FXString &regex, int filter=DefaultFilter, int sorting=DefaultSort);
	//! \overload
	const QFileInfoList *entryInfoList(int filter=DefaultFilter, int sorting=DefaultSort) { return entryInfoList(FXString::nullStr(), filter, sorting); }
	/*! Returns a string list detailing all the drives on the system.
	Will be '/' for POSIX, 'A:/', 'B:/' etc. for Windows. Note that this call
	breaks API compatibility with QDir.
	*/
	static QStringList drives();
	//! Refreshes the enumeration
	void refresh();

	//! Creates a directory within this directory
	bool mkdir(const FXString &leaf, bool acceptAbs=true);
	//! Deletes a directory within this directory. Can delete a directory tree.
	bool rmdir(const FXString &leaf, bool acceptAbs=true);
	//! Deletes a file within this directory
	bool remove(const FXString &leaf, bool acceptAbs=true);
	//! Renames a file within this directory. Can handle renaming across filing systems.
	bool rename(const FXString &src, const FXString &dest, bool acceptAbs=true);
	//! Returns true if \em leaf is in this directory
	bool exists(const FXString &leaf, bool acceptAbs=true);

	//! Returns true if the directory is readable
	bool isReadable() const;
	//! Returns true if the directory exists
	bool exists() const;
	//! Returns true if the directory is the root directory of a drive
	bool isRoot() const;
	//! Returns true if the path is relative
	bool isRelative() const;
	//! Converts the path to an absolute one
	void convertToAbs();

	//! Returns the directory separator character. Same as PATHSEPSTRING
	static FXString separator();
	//! Returns the current directory for this process
	static FXDir current();
	//! Returns the home directory of the current user
	static FXDir home();
	//! Returns the root directory of the system (on Windows, where WINNT lives)
	static FXDir root();
	//! Returns the current directory path
	static FXString currentDirPath();
	//! Returns the home directory path
	static FXString homeDirPath();
	//! Returns the root directory path
	static FXString rootDirPath();
	//! Returns true if the file name matches the specified regular expression
	static bool match(const FXString &filter, const FXString &filename);
	//! Returns the simplified form of the path
	static FXString cleanDirPath(const FXString &path);
	//! Returns true if the path is relative
	static bool isRelativePath(const FXString &path);
	/*! Returns a list consisting of those filenames in A and B which
	are either not of the same name in both or one or more of their characteristics
	has changed. Needs to run entryInfoList() on both so if the older of the
	two does not contain cached FXFileInfo records, they will have to be reread -
	which means the characteristics will match */
	static QStringList extractChanges(const FXDir &A, const FXDir &B);
};

} // namespace

#endif
