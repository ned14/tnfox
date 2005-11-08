/********************************************************************************
*                                                                               *
*                           Mapped memory i/o device                            *
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


#ifndef QMEMMAP_H
#define QMEMMAP_H
#include "QIODevice.h"

namespace FX {

/*! \file QMemMap.h
\brief Defines classes used to access mapped memory
*/

class FXString;
class FXFile;

/*! \class QMemMap
\ingroup fiodevices
\brief An i/o device accessing mapped memory

Mapped memory is conceptually a portion of a disc-based file indirectly accessed
as though it were a region of memory. All modern operating systems support this
facility, yet astonishingly little use is made of it in most computer programs.
Quite possibly this is because it is harder to use than it should be which is
a shame because it makes available the very best file access caching and buffering
to your program completely transparently.

QMemMap addresses this by providing transparent usage of mapped memory and
portably too. There are no issues with accidentally running off the end of
the mapped section nor off the start. Use as shared memory between processes
is also ridiculously easy - you simply name the device the same in both processes
and voilá, you're now working with the same patch of memory! Remember to
synchronise multiple process access - the most portable is via msgs using a pipe,
however if there is low contention in access it can be done via FX::QShrdMemMutex.

You can map in an existing FX::FXFile device and indeed internally QMemMap creates
one if you don't specify a FX::FXFile but do a filename. You should note that as a
result, QMemMap can lose track of the real file length under the same conditions
as FX::FXFile for which reloadSize() is also provided. If you choose shared memory
(QMemMap::Memory) rather than a filename (QMemMap::File) then the file used is kept in
memory as much as possible rather than being flushed to disc as soon as possible.

If you wish to convert a FX::QBuffer into a QMemMap, simply create an FX::FXStream
refering to the QMemMap and do \code
FXStream ds((QMemMap) x);
ds << (QBuffer) y;
\endcode

QMemMap is heavily optimised including custom getch() and putch() especially for
mostly sequential transfers (where at() is not moving more than FXProcess::pageSize()
at a time). Also file changes moving forwards are better than backwards. In ideal
conditions, not much more than a \c memcpy() is being performed so the bigger
the chunks you give it, the better.

Default security is for FX::FXACLEntity::everything() to have full access when
the type is mapped memory - file maps deny everything access except the local
process. This makes sense as mapped memory is generally for inter-process
communication - if however it's a private region for communication with a
known process it makes sense to restrict access to the user running that
process at least. Note that until the map is opened, permissions()
returns what will be applied to the map on open() rather than the map
itself - if you want the latter, use the static method. Note also that the
map's security is not linked in any way with that of any file underlying it -
you must set them separately. Note that on POSIX, the underlying file's
permissions are those for the map and setting them here has no effect -
however, if it's shared memory then the permissions \em do have an effect.

Like FX::QPipe, the shared memory name is deleted by its creator on POSIX
only (on Windows it lasts until the last thing referring to it closes). If
you don't want this, specify IO_DontUnlink in the flags to open().

<h4>Usage:</h4>
Simply instantiate, set filename or name and open(). Once open, use mapIn() to
map in a specified section and mapOut() to map it out. On 32 bit architectures,
files bigger than ~1Gb on some operating systems will fail to map in so be aware
of the consequences of the defaults in mapIn() which map everything in. Failures to map in due to address
space exhaustion cause mapIn() to return zero rather than an exception - however,
the mapping remains on the books and may get mapped in silently the next time
something is mapped out. See FX::FXProcess::virtualAddrSpaceLeft().

This is done this way because the other important facility QMemMap offers is
transparently using the mapped section(s) when the file pointer is within
one and doing normal readBlock() and writeBlock() with the FX::FXFile when
outside. Thus you can afford to map in the most commonly used portion(s) of your
file and let the rest happen via buffered i/o as usual.
  
\warning You \em really do not want to read or write parts of a file which have
been mapped in using FX::FXFile directly. This inevitably causes data corruption.
To avoid this, open() calls flush() on its FX::FXFile if it's already open
so therefore after this point, all i/o on the file should be done via QMemMap.

Indeed unless you need absolutely critical sustained sequential reads or writes,
I'd recommend doing all your file i/o via FX::QMemMap instead of via FX::FXFile
and indeed I've made the non static methods exactly the same.
Memory mapped files are demand-read often with intelligent prefetching as read
accesses are performed from its mapped section. Writes almost always remain in
the RAM backing the mapped section until the system is idle (ie; not doing more
reads) so you get the maximum performance from your hardware. Even better when
free memory is tight, the operating system will write out dirty pages sooner
rather than later thus getting the best of all possible worlds. On my development
system running Win2k I get a three-fold speed increase copying one file to another
over FXFile and that's even with NTFS's excellent caching of buffered i/o.

\note I've found that memory mapped file i/o is considerably faster on Linux when
using ReiserFS rather than ext3.

Extending the file size with truncate() works as expected - however depending
on the host OS, it can require recreating the map which implies deletion and
recreation of all mapped sections (which may move position). Shortening the file
requires wholly unmapping any mapped sections which straddle
between the old length and the new. Any sections lost this way
must be remapped by you if you want them to (this could not
be done automatically because the mapping address which you may be relying
upon could change). There is a difference between writing off the end of the file
and truncate()-ing it - only the latter extends the ability to map the new data.

Lastly the mapOffset() method lets you see if any arbitrary file ptr offset
maps into the currently mapped sections. It returns the address of the
corresponding location in memory or zero if that section is not mapped.

\note Other minor thing is that if you access memory directly using the
return from mapIn() no character translation has been performed (IO_Translate)

\note There is a bug on WinNT whereby if you map the same file in for write
access using two or more QMemMap's, the second and thereafter will never allow
any mapping whatsoever. Either share the one QMemMap, or accept lower
performance. This problem does not affect multiple read access instances or a
write with many reads - only two or more write access instances.

\note Another misfeature on WinNT is, when under Terminal Services or the second
or later non-administrator user to log on to a WinXP machine, the \c SeCreateGlobalPrivilege
is not given - this means TnFOX can no longer create shared memory that all
processes can see. In this situation, TnFOX tries to open the global version
of the shared memory name which if it fails, it then creates it in the local
namespace. This behaviour can be unexpected especially as to most users of WinXP
one login session appears identical to a normal one.
*/

struct QMemMapPrivate;
class FXAPIR QMemMap : public QIODevice
{
	QMemMapPrivate *p;
	QMemMap(const QMemMap &);
	QMemMap &operator=(const QMemMap &);
	void winopen(int mode);
	inline void setIoIndex(FXfval offset);
public:
	//! The types of mapped memory there are
	enum Type
	{
		File=0,			//!< The name refers to a file based on disc
		Memory			//!< The name refers to a name of shared memory
	};
	//! A mapped region within the map
	struct MappedRegion
	{
		FXfval offset;	//!< Offset into the file
		FXfval length;	//!< Length of mapped region
		FXfval end;		//!< End offset into the file (=offset+length)
		void *addr;		//!< Address of mapped region
		MappedRegion() { }
		MappedRegion(FXfval _offset, FXfval _length, void *_addr) : offset(_offset), length(_length), end(_offset+_length), addr(_addr) { }
		//! Returns true if structure is null
		bool operator!() const throw() { return !offset && !length && !addr; }
	};
	QMemMap();
	//! Constructs an instance operating on file \em filename
	QMemMap(const FXString &filename);
	//! Constructs an instance using an external FX::FXFile
	QMemMap(FXFile &file);
	//! Constructs an instance working with named shared memory of length \em len. If name is empty, sets to be unique
	QMemMap(const FXString &name, FXuval len);
	~QMemMap();

	//! Returns the filename or shared memory name being addressed by this device
	const FXString &name() const;
	//! Sets the filename or shared memory name being addressed by this device. Closes the old file first.
	void setName(const FXString &name);
	//! Returns true if the shared memory name is unique
	bool isUnique() const;
	//! Sets if the shared memory name is unique
	void setUnique(bool v);
	//! Returns the type of the memory map
	Type type() const;
	//! Sets whether to interpret the name as a filename or shared memory.
	void setType(Type type);
	//! Returns true if the file name points to an existing file
	bool exists() const;
	//! Deletes the file name, closing the file first if open. Returns false if file doesn't exist
	bool remove();
	//! Reloads the size of the file. See description above
	FXfval reloadSize();
	//! Returns the current mappable extent of the file (which may be shorter than the file length)
	FXfval mappableSize() const;
	//! Extends the mappable extent of the file to the current file length
	void maximiseMappableSize();

	/*! \return Where in memory the section was mapped to. Zero if could not be mapped
	(eg; ran out of address space which is common on 32 bit architectures).
	\param offset From where in the file to map (should be zero for shared memory).
	Must be a multiple of FXProcess::pageSize().
	\param amount How much to map in. (FXfval) -1 maps in the length of the file (bear in mind
	mapping in large files can exhaust available address space).
	\param copyOnWrite True if writes to the map should not be written to the file. Effectively
	disables shared memory.

	Maps the specified section of the file into memory. If there is insufficient address space
	free to fit it all in, fails silently and returns zero. Note that if this map overlaps
	any existing map, that map is removed first.
	*/
	void *mapIn(FXfval offset=0, FXfval amount=(FXfval) -1, bool copyOnWrite=false);
	/*! Maps out the specified section. If this cross-sects an existing section(s), those
	sections are mapped out in their entirity (because remaps may move location).
	*/
	void mapOut(FXfval offset=0, FXfval amount=(FXfval) -1);
	/*! \overload
	Maps out a section based on the address returned from a previous mapIn().
	*/
	void mapOut(void *area);
	/*! Returns a description of the mapped region containing the specified offset plus
	that of the region following, returning zero in all members if that offset is not within
	a region which has been mapped. The default offset is the current file pointer. Returns
	false if no region exists between the specified offset and the end of the file. */
	bool mappedRegion(MappedRegion *current, MappedRegion *next=0, FXfval offset=(FXfval) -1) const;
	/*! Returns the address within memory of where the specified offset into the file
	is mapped (which by default is the current file pointer). Returns zero if that
	place is not mapped into memory.
	*/
	void *mapOffset(FXfval offset=(FXfval) -1) const;

	virtual bool open(FXuint mode);
	virtual void close();
	//! Flushes all open mapped sections to disc, then any underlying file
	virtual void flush();
	virtual FXfval size() const;
	/*! Sets a new length of file, unmapping any sections containing the part being removed
	\note This is not especially a fast call as the mapped sections must be recreated. On the
	other hand, it is the only way to make newly written data off the end mappable.
	*/
	virtual void truncate(FXfval size);
	virtual FXfval at() const;
	virtual bool at(FXfval newpos);
	virtual bool atEnd() const;
	virtual const FXACL &permissions() const;
	virtual void setPermissions(const FXACL &perms);
	//! Returns the permissions for the memory mapped section called \em name
	static FXACL permissions(const FXString &name);
	//! Sets the permissions for the memory mapped section called \em name
	static void setPermissions(const FXString &name, const FXACL &perms);
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
