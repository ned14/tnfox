/********************************************************************************
*                                                                               *
*                                 Windows Links                                 *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2005 by Niall Douglas.   All Rights Reserved.            *
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

#ifndef FXWINLINKS_H
#define FXWINLINKS_H

#include "FXString.h"

namespace FX {

/*! \file FXWinLinks.h
\brief Defines classes used in manipulating Windows Links
*/

class QFileInfo;

/*! \struct FXWinShellLink
\brief A POSIX-compatible Windows Shell Link

In the process of adding node linking to Tn, I thought it would be nice for Tn
to be able to understand and indeed generate Windows Shell Links (aka. Shortcuts)
to better aid interoperability when running on Windows machines.

However, then I thought that often I am running inside Linux and have most of my
files on both Windows and Linux living on a FAT32 drive. Surely many other people
would be in a similar situation, so if Linux Tn could also work with Windows
shortcuts then that would be really useful.

Thus began the painful process of reverse engineering! I was greatly aided in this
process by "The Windows Shortcut File Format" by Jesse Hager
(http://www.i2s-lab.com/Papers/The_Windows_Shortcut_File_Format.pdf) which proved
almost entirely correct. It turned out however that it did not contain quite enough
information for generating link files which the Windows explorer would actually
read, so a day's worth of comparing binary views of various link files later and
we have this class.

More or less, everything is covered. It's only been tested on Windows XP and
really only with linking to files and directories - not to network shares or
anything else really. Some fields I couldn't figure out - for example, each
pathname part in the item id list has its created, modified & last accessed
stored with it but not in FILETIME format nor in \c time_t, and worse still
they seem to oscillate in a way such that later timestamps can have lower
values (I'm guessing they've lost a high part of a FILETIME). And there are a
few other fields I've just had to leave empty as well as totally omitting the
distributed link tracker id at the end of the file, but it doesn't seem to matter.

<h3>Drive name translation:</h3>
On POSIX, drive name translation is performed via \c /etc/mtab so you can
use this class and in theory, windows paths will become POSIX ones and vice
versa. There is however some trickery involved here as drive letter assignment
can be totally arbitrary on NT and we don't have access to the Windows
registry, so here is the rule:

Does the mounted partition (of type vfat/msdosfs or ntfs) have a mount
name containing "win" or "windows" or the letter 'c' to 'z' alone, in brackets or
separated from the rest of the mount name by a space? If so, consider it
a candidate by matching the base part of the target path with the file system
of the mount. If the process fails, an exception is thrown.

One could go scanning superblocks of partitions and generating MSDOS drive
letter assignments, but this author routinely overrides that and I suspect so
do most people with dual boot (I have quad boot!) setups. The worst situation
is when you have two Windows installations eg; x86 and x64 and you decided in
your infinite wisdom to make the system drive on both C: - this means shortcuts
mean different destinations depending on which system you are running, and
you'd be right that this is lunacy (I tried it many years ago - it was more
hassle than having a D system drive).

Most people will mount windows partitions in fairly logical places on POSIX,
so the above scheme should work on most systems. It is unfortunately one
of those things which cannot be guaranteed however and maybe someday I should
add some mechanism for specifying a per-user mapping like via FX::FXRegistry.

\sa FX::FXProcess::
*/
struct FXAPI FXWinShellLink
{
	struct FXAPI Header
	{
		FXuint length;
		char guid[16];
		struct Flags
		{
			FXuint hasItemIdList : 1;
			FXuint pointsToFileOrDir : 1;
			FXuint hasDescription : 1;
			FXuint hasRelativePath : 1;
			FXuint hasWorkingDir : 1;
			FXuint hasCmdLineArgs : 1;
			FXuint hasCustomIcon : 1;
			FXuint useWorkingDir : 1;		// Seems to need to be set to enable working dir
			FXuint unused : 24;
		} flags;
		struct FileAttribs
		{	// = return from GetFileAttributes()
			FXuint isReadOnly : 1;
			FXuint isHidden : 1;
			FXuint isSystem : 1;
			FXuint isVolumeLabel : 1;
			FXuint isDir : 1;
			FXuint isModified : 1;	// =archive bit set, ie; is a file normally
			FXuint isEncrypted : 1;
			FXuint isNormal : 1;	// Doesn't seem to get set
			FXuint isTemporary : 1;
			FXuint isSparse : 1;
			FXuint hasReparsePoint : 1;
			FXuint isCompressed : 1;
			FXuint isOffline : 1;
			FXuint unused : 19;
		} fileattribs;		// in GetFileAttributes() format
		FXulong creation, modified, lastAccess;	// in FILETIME format
		FXuint filelength;
		FXuint iconno;
		enum ShowWnd
		{
			HIDE=0,
			NORMAL,
			SHOWMINIMIZED,
			SHOWMAXIMIZED
		};
		ShowWnd showWnd;
		FXuint hotkey;
		FXuint unknown1, unknown2;

		Header()
		{
			memset(this, 0, sizeof(*this));
			static const char linkguid[]={
				0x01, 0x14, 0x02, 0x00,
				0x00, 0x00,
				0x00, 0x00,
				(char) 0xc0, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x46 };
			length=0x4c;
			memcpy(guid, linkguid, sizeof(guid));
			showWnd=NORMAL;
		}

		friend FXAPI FXStream &operator<<(FXStream &s, const Header &i);
		friend FXAPI FXStream &operator>>(FXStream &s, Header &i);
	} header;
	/* Had to reverse engineer this myself!
	First item is guid of what I assume means "My Computer"
	  +0: length of item (always 0x14=20 bytes)
	  +2: Always 18 bytes:
		0x1f, 0x50, 0xe0, 0x4f, 0xd0, 0x20, 0xea, 0x3a,
		0x69, 0x10, 0xa2, 0xd8, 0x08, 0x00, 0x2b, 0x30,
		0x30, 0x9d
	Next item is drive:
	  +0: length of item (always 0x19=25 bytes)
	  +2: byte 0x2f
	  +3: Drive in ASCII for 22 bytes
	Next comes path items:
	  +0: length of overall item
	  +2: short 0x31 for dir, 0x32 for file
	  +4: int size of file (=0 for dir)
	  +8: int created		d:\windows=0x63893326, c:\windows=0x052D3321, c:\downloads=0x48853328
	  +12: GetFileAttributes() for entry
	  +14: Null terminated ASCII short filename version (padded to two byte multiple)
	 On WinXP only:
	  +n: length of this item
	  +n+2: short 0x3
	  +n+4: short 0x4
	  +n+6: short 0xbeef
	  +n+8: int modified	d:\windows=0x8de5328c, c:\windows=0x63c43297, c:\downloads=0x76e8331a
	  +n+12: int lastRead	d:\windows=0x49843328, c:\windows=0x41fa3328, c:\downloads=0x48853328
	  +n+16: int 0x14
	  +n+20: Null terminated Unicode version
	  +n+m: Two bytes (unknown)
	*/
	struct FXAPI ItemIdListTag
	{
		FXushort length;
		char path1[260];			// In ASCII
		FXushort path2[260];		// In unicode
		ItemIdListTag()
		{
			memset(this, 0, sizeof(*this));
		}
		friend FXAPI FXStream &operator<<(FXStream &s, const ItemIdListTag &i);
		friend FXAPI FXStream &operator>>(FXStream &s, ItemIdListTag &i);
		char originalPath[260];		// [not in lnk file] Used so code knows the non-decoded path
	} itemIdList;
	struct FXAPI FileLocationTag
	{
		FXuint length;			// to end of whole tag
		FXuint firstOffset;		// to end of this tag header
		struct FXAPI Flags
		{
			FXuint onLocalVolume : 1;
			FXuint onNetworkShare : 1;
			FXuint unused : 30;
		} flags;
		struct FXAPI LocalVolume
		{
			FXuint length;
			enum Type
			{
				Unknown=0,
				NoRoot,
				Removable,	// ie; floppy, usb drive etc.
				Fixed,		// ie; hard disc
				Remote,		// ie; network share
				CDROM,
				RamDrive
			};
			Type type;
			FXuint serialNo;
			char volumeLabel[64];
			LocalVolume()
			{
				memset(this, 0, sizeof(*this));
			}
			friend FXAPI FXStream &operator<<(FXStream &s, const LocalVolume &i);
			friend FXAPI FXStream &operator>>(FXStream &s, LocalVolume &i);
		} localVolume;
		char basePath[260];
		struct FXAPI NetworkVolume
		{
			FXuint length;
			FXuint type;
			FXchar shareName[260];
			NetworkVolume()
			{
				memset(this, 0, sizeof(*this));
				type=0x2;
			}
			friend FXAPI FXStream &operator<<(FXStream &s, const NetworkVolume &i);
			friend FXAPI FXStream &operator>>(FXStream &s, NetworkVolume &i);
		} networkVolume;
		char remainingPath[64];

		FileLocationTag()
		{
			memset(this, 0, sizeof(*this));
			firstOffset=0x1c;
		}
		friend FXAPI FXStream &operator<<(FXStream &s, const FileLocationTag &i);
		friend FXAPI FXStream &operator>>(FXStream &s, FileLocationTag &i);
	} fileLocation;
	struct FXAPI StringTag
	{
		FXushort length;			// in characters
		FXushort string[260];		// Unicode string
		StringTag() { memset(this, 0, sizeof(*this)); }
		friend FXAPI FXStream &operator<<(FXStream &s, const StringTag &i);
		friend FXAPI FXStream &operator>>(FXStream &s, StringTag &i);
	};
	StringTag description;
	StringTag relativePath;
	StringTag workingDir;
	StringTag cmdLineArgs;
	StringTag customIcon;
	friend FXAPI FXStream &operator>>(FXStream &s, FXWinShellLink &i);

	/*! Sets up and writes a shell link file to the specified FX::FXStream
	linking to \em whereTo */
	void write(FXStream s, const QFileInfo &whereTo);

	/*! Reads what the shell link file pointed to by \em path points to,
	optionally returning the targeted path after host OS drive conversion */
	static FXString read(const FXString &path, bool doDriveConversion=true);
	/*! Writes a shell link file to \em path, always performing host OS drive
	conversion when needed */
	static void write(const FXString &path, const QFileInfo &whereTo);

};


/*! \struct FXWinJunctionPoint
\brief A POSIX-compatible Windows NTFS junction point

Since NTFS 5, Windows lets you mount any directory tree under a special NTFS-only
entity called a <i>junction point</i>. This is basically a magic directory which
tells the NT kernel to indirect to some other specified path - much like a
symbolic link, except that it can only link from a directory to another directory.

As it happens, most POSIX implementations have just the same ability via a
variation of the \c mount command which allows you to bind a directory tree to
appear at some other directory location. Unfortunately, this is a root only
operation so FXWinJunctionPoint simply emulates the Windows functionality using
a normal symbolic link.

\warning The Windows Shell (Explorer) does NOT support NTFS junction points -
it thinks they are normal directories and if you delete them via the recycle
bin, Explorer happily deletes everything your junction point pointed to <b>when
the Recycle Bin is emptied</b> which usually wasn't what you wanted! You can
Shift-Delete (delete immediately) to delete just the junction point alone.

Because of this caveat, this implementation marks junction points as compressed
so that their name appears in blue in Explorer to remind you that the junction
point isn't a normal directory. Inform your users if you intend to use them in
your code.
*/
struct FXAPI FXWinJunctionPoint
{
private:
	FXWinJunctionPoint();
	~FXWinJunctionPoint();
public:
	//! Returns true if the specified path is a NTFS junction point
	static bool test(const FXString &path);
	//! Reads what a NTFS junction point points to
	static FXString read(const FXString &path);
	//! Writes a NTFS junction point
	static void write(const FXString &path, const QFileInfo &whereTo);
	//! Deletes a NTFS junction point
	static void remove(const FXString &path);
};

} // namespace

#endif
