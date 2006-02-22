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

#include "FXWinLinks.h"
#include "QFileInfo.h"
#include "FXFile.h"
#include "QTrans.h"
#include "FXRollback.h"
#include "fxver.h"
#include "FXErrCodes.h"
#ifndef USE_POSIX
 #define USE_WINAPI
 #include "WindowsGubbins.h"
 #include "WinIoCtl.h"
#endif
#include "FXMemDbg.h"
#if defined(DEBUG) && defined(FXMEMDBG_H)
static const char *_fxmemdbg_current_file_ = __FILE__;
#endif

#define PRINT_DEBUG

namespace FX {

FXStream &operator<<(FXStream &s, const FXWinShellLink::Header &i)
{
	s << i.length;
	s.writeRawBytes(i.guid, sizeof(i.guid));
	s << *(FXuint *)&i.flags << *(FXuint *)&i.fileattribs << i.creation << i.modified << i.lastAccess;
	s << i.filelength << i.iconno << (FXuint) i.showWnd << i.hotkey;
	s << i.unknown1 << i.unknown2;
	return s;
}
FXStream &operator>>(FXStream &s, FXWinShellLink::Header &i)
{
	s >> i.length;
	s.readRawBytes(i.guid, sizeof(i.guid));
	s >> *(FXuint *)&i.flags >> *(FXuint *)&i.fileattribs >> i.creation >> i.modified >> i.lastAccess;
	FXuint showWnd;
	s >> i.filelength >> i.iconno >> showWnd >> i.hotkey;
	i.showWnd=(FXWinShellLink::Header::ShowWnd) showWnd;
	s >> i.unknown1 >> i.unknown2;
	return s;
}



FXStream &operator<<(FXStream &s, const FXWinShellLink::ItemIdListTag &i)
{
	FXfval mypos=s.device()->at();
	s << i.length;
	// Push a My Computer
	s << (FXushort) 0x14;
	static const FXuchar mycomputerguid[]={ 0x1f, 0x50, 0xe0, 0x4f, 0xd0, 0x20, 0xea, 0x3a,
		0x69, 0x10, 0xa2, 0xd8, 0x08, 0x00, 0x2b, 0x30,
		0x30, 0x9d }, zeros[]={ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	s.writeRawBytes(mycomputerguid, 18);
	// Push the drive
	s << (FXushort) 0x19 << (FXuchar) 0x2f;
	s.writeRawBytes(i.path1, 3);
	s.writeRawBytes(zeros, 8);
	s.writeRawBytes(zeros, 8);
	s.writeRawBytes(zeros, 3);
	// Push path elements
	const FXnchar *p1=i.path2+2, *p2;
	FXString originalPath(i.originalPath);
	originalPath.substitute('/', '\\');
	FXint originalPathOffset=originalPath.find(i.path1+2);
	assert(originalPathOffset>=0);
#ifdef USE_POSIX
	originalPath.substitute('\\', '/');
#endif
	do
	{
		for(p2=p1+1; *p2 && '\\'!=*p2; ++p2);
		FXfval itempos=s.device()->at();
		s << (FXushort) 0;	// length
		QFileInfo fi(originalPath.left((FXint)(originalPathOffset-2+(p2-i.path2))));
		assert(fi.exists());
		if(fi.isDir())
			s << (FXushort) 0x31;
		else
			s << (FXushort) 0x32;
		s << (FXuint) fi.size();
		s << (FXuint) 0xffffffff;	// created
		s << (FXushort)(fi.isDir() ? 0x10 : 0x20);
		{	// Generate short name
			FXString shortname(i.path1+(p1+1-i.path2), (FXint)(p2-p1-1));
#if 0
			if(fi.isDir())
			{
				if(shortname.length()>12)
					shortname.replace(10, shortname.length()-10, "~1");
			}
			else
			{
				FXint comma=shortname.rfind('.');
				if(comma>=0 && comma>8)
					shortname.replace(6, comma-6, "~1");
				if(shortname.length()>12)
					shortname.truncate(12);
			}
#endif
			int cnt=0;
			s.writeRawBytes(shortname.text(), shortname.length()+1);
			if((shortname.length()+1)&1)
				s << (FXuchar) 0;	// pas
		}
		FXfval itempos2=s.device()->at();
#if 0	// WinXP only
		s << (FXushort) 0;	// length
		s << (FXushort) 0x3 << (FXushort) 0x4 << (FXushort) 0xbeef;
		s << (FXuint) 0xffffffff;	// modified
		s << (FXuint) 0xffffffff;	// lastRead
		s << (FXuint) 0x14;
		s.save(p1+1, (FXint)(p2-p1-1));
		s << (FXushort) 0;	// terminator
		s << (FXushort) 0;	// unknown
#endif

		FXushort length1=(FXushort)(s.device()->at()-itempos), length2=(FXushort)(s.device()->at()-itempos2);
		s.device()->at(itempos2);
		s << (FXushort) length2;
		s.device()->at(itempos);
		s << (FXushort) length1;
		s.device()->at(itempos+length1);
	} while(*(p1=p2));
	s << (FXushort) 0;	// terminator?
	FXushort length=(FXushort)(s.device()->at()-mypos-2);
	s.device()->at(mypos);
	s << length;
	s.device()->at(mypos+length+2);
	return s;
}
FXStream &operator>>(FXStream &s, FXWinShellLink::ItemIdListTag &i)
{
	s >> i.length;
	FXfval mypos=s.device()->at();	// this goes AFTER
	// Next consists of tags with a FXushort length
	int n=0;
	while(s.device()->at()-mypos<i.length)
	{
		FXfval tagpos=s.device()->at();
		FXushort taglen;
		s >> taglen;
		if(!taglen) break;
#ifdef PRINT_DEBUG
		fxmessage("ItemIdListTag no %d offset 0x%lx, len %u\n", n, (long) tagpos, taglen);
#endif
		if(!n)
		{	// Is "My Computer" guid
		}
		else if(1==n)
		{	// Is drive specifier
			s >> *i.path1;
			assert(0x23==(*i.path1 & 0x23));
			s.readRawBytes(i.path1, 22);
			*strchr(i.path1, '\\')=0;
			for(int q=0; q<22; q++)
				i.path2[q]=i.path1[q];
		}
		else
		{	// Is path item
			FXushort type1, type2;
			FXuint filelen;
			s >> type1 >> filelen;
			assert(0x31==type1 || 0x32==type1);
			FXuint created, modified, accessed;
			s >> created;
			s >> type2;
			assert(0x10==type2 || 0x20==type2);
			char *p1=strchr(i.path1, 0);
			*p1++='\\';
			FXuint sncnt=0;
			while((s >> *p1, sncnt++, *p1++));
			if(sncnt&1) s >> *p1; // Round up to nearest two multiple
			if(s.device()->at()-tagpos>taglen)
			{	// This is only on WinXP
				FXushort unicodelen;
				FXushort unicodeheader1, unicodeheader2, unicodeheader3;
				s >> unicodelen >> unicodeheader1 >> unicodeheader2 >> unicodeheader3;
				assert(0x3==unicodeheader1);
				assert(0x4==unicodeheader2);
				assert(0xbeef==unicodeheader3);
				s >> modified >> accessed;
				FXuint unicodeheader4; s >> unicodeheader4;
				assert(0x14==unicodeheader4);
				FXnchar *p2=i.path2+sncnt;
				*p2++='\\';
				while((s >> *((FXushort *) p2), *p2))
#if FOX_MAJOR>1 || FOX_MINOR>=6
					p1+=nc2utfs(p1, (FXnchar *) p2, sizeof(FXnchar));
#else
					*p1++=(char) *p2++;
#endif
				*p1=0; *p2=0;
				FXushort unknown4; s >> unknown4;
				/*WIN32_FILE_ATTRIBUTE_DATA fad={0};
				GetFileAttributesEx(i.path1, GetFileExInfoStandard, &fad);
				fxmessage("%s\n", FXString("Path '%1'\nhas unknowns 0x%2, 0x%3, 0x%4, 0x%5\n0x%6, 0x%7, 0x%8").arg(FXString(i.path1))
					.arg(unknown1, 0, 16).arg(unknown2, 0, 16).arg(unknown3, 0, 16).arg(unknown4, 0, 16)
					.arg(*(FXulong *)&fad.ftCreationTime).arg(*(FXulong *)&fad.ftLastWriteTime).arg(*(FXulong *)&fad.ftLastAccessTime).text());
				fxmessage("%lf, %lf, %lf\n", (double) fad.ftCreationTime.dwHighDateTime/unknown1, (double) fad.ftLastWriteTime.dwHighDateTime/unknown2, (double) fad.ftLastAccessTime.dwHighDateTime/unknown3);*/
			}
		}
		s.device()->at(tagpos+taglen);
		n++;
	}
#ifdef PRINT_DEBUG
	fxmessage("ItemIdListTag was %u long, seeking to %u\n", (FXuint) i.length, (FXuint)(mypos+i.length));
#endif
	s.device()->at(mypos+i.length);
	return s;
}


FXStream &operator<<(FXStream &s, const FXWinShellLink::FileLocationTag::LocalVolume &i)
{
	FXfval mypos=s.device()->at();
	s << i.length << (FXuint) i.type << i.serialNo;
	FXuint vlOffset=0x10; s << vlOffset;
	s.writeRawBytes(i.volumeLabel, strlen(i.volumeLabel)+1);
	FXuint length=(FXuint)(s.device()->at()-mypos);
	s.device()->at(mypos);
	s << length;
	s.device()->at(mypos+length);
	return s;
}
FXStream &operator>>(FXStream &s, FXWinShellLink::FileLocationTag::LocalVolume &i)
{
	FXfval mypos=s.device()->at();
	FXuint type;
	s >> i.length >> type >> i.serialNo;
	i.type=(FXWinShellLink::FileLocationTag::LocalVolume::Type) type;
	FXuint vlOffset; s >> vlOffset;
	s.device()->at(mypos+vlOffset);
	s.readRawBytes(i.volumeLabel, i.length-vlOffset);
	i.volumeLabel[i.length-vlOffset]=0;
	s.device()->at(mypos+i.length);
	return s;
}


FXStream &operator<<(FXStream &s, const FXWinShellLink::FileLocationTag::NetworkVolume &i)
{
	FXfval mypos=s.device()->at();
	s << i.length << (FXuint) i.type;
	FXuint nsOffset=0x14; s << nsOffset;
	s << (FXuint) 0 << (FXuint) 0x20000;
	s.writeRawBytes(i.shareName, strlen(i.shareName)+1);
	FXuint length=(FXuint)(s.device()->at()-mypos);
	s.device()->at(mypos);
	s << length;
	s.device()->at(mypos+length);
	return s;
}
FXStream &operator>>(FXStream &s, FXWinShellLink::FileLocationTag::NetworkVolume &i)
{
	FXfval mypos=s.device()->at();
	FXuint type;
	s >> i.length >> type;
	i.type=type;
	FXuint nsOffset; s >> nsOffset;
	// Ignore the next two words
	s.device()->at(mypos+nsOffset);
	s.readRawBytes(i.shareName, i.length-nsOffset);
	i.shareName[i.length-nsOffset]=0;
	s.device()->at(mypos+i.length);
	return s;
}


FXStream &operator<<(FXStream &s, const FXWinShellLink::FileLocationTag &i)
{
	FXfval mypos=s.device()->at();
	s << i.length << i.firstOffset << *(FXuint *)&i.flags;
	// Ok all offsets start at firstOffset
	FXuint lvOffset=i.firstOffset, bpOffset=0, nvOffset=0, rpOffset=0;
	s << lvOffset << bpOffset << nvOffset << rpOffset;
	if(i.flags.onLocalVolume)
	{
		s << i.localVolume;
		bpOffset=(FXuint)(s.device()->at()-mypos);
		s.writeRawBytes(i.basePath, strlen(i.basePath)+1);
	}
	if(i.flags.onNetworkShare)
	{
		nvOffset=(FXuint)(s.device()->at()-mypos);
		s << i.networkVolume;
	}
	rpOffset=(FXuint)(s.device()->at()-mypos);
	s.writeRawBytes(i.remainingPath, strlen(i.remainingPath)+1);
	FXuint length=(FXuint)(s.device()->at()-mypos);
	// Ok rewrite the offsets we've calculated
	s.device()->at(mypos);
	s << length << i.firstOffset << *(FXuint *)&i.flags;
	s << lvOffset << bpOffset << nvOffset << rpOffset;
	s.device()->at(mypos+length);
	return s;
}
FXStream &operator>>(FXStream &s, FXWinShellLink::FileLocationTag &i)
{
	FXfval mypos=s.device()->at();
	s >> i.length;
	if(i.length>sizeof(FXuint))
	{
		s >> i.firstOffset >> *(FXuint *)&i.flags;
		/* Next comes three offsets:
				Local Volume Table	(=garbage if bit 0 clear)
				Base pathname		(=garbage if bit 0 clear)
				Network volume Table(=garbage if bit 1 clear)
				Remaining pathname	(always valid)
		*/
		FXuint lvOffset, bpOffset, nvOffset, rpOffset;
		s >> lvOffset >> bpOffset >> nvOffset >> rpOffset;
#ifdef PRINT_DEBUG
		fxmessage("mypos=%u, lvOffset=%u, bpOffset=%u, nvOffset=%u, rpOffset=%u\n", (FXuint) mypos, lvOffset, bpOffset, nvOffset, rpOffset);
#endif
		if(i.flags.onLocalVolume)
		{
			s.device()->at(mypos+lvOffset);
			s >> i.localVolume;
			s.device()->at(mypos+bpOffset);
			s.readRawBytes(i.basePath, rpOffset-bpOffset);
			i.basePath[rpOffset-bpOffset]=0;
		}
		if(i.flags.onNetworkShare)
		{
			s.device()->at(mypos+nvOffset);
			s >> i.networkVolume;
		}
		s.device()->at(mypos+rpOffset);
		s.readRawBytes(i.remainingPath, i.length-rpOffset);
		i.remainingPath[i.length-rpOffset]=0;
	}
	s.device()->at(mypos+i.length);
	return s;
}


FXStream &operator<<(FXStream &s, const FXWinShellLink::StringTag &i)
{
	FXushort len;
	for(len=0; i.string[len]; len++);
	s << len;
	s.save((FXushort *) i.string, len);
	return s;
}
FXStream &operator>>(FXStream &s, FXWinShellLink::StringTag &i)
{
	s >> i.length;
#ifdef PRINT_DEBUG
	fxmessage("StringTag: mypos=%u, length=%u\n", (FXuint)(s.device()->at()-2), (FXuint) i.length);
#endif
	s.load((FXushort *) i.string, i.length);
	i.string[i.length]=0;
	return s;
}


FXStream &operator>>(FXStream &s, FXWinShellLink &i)
{
	s >> i.header;
	if(i.header.flags.hasItemIdList)
		s >> i.itemIdList;
	s >> i.fileLocation;
	if(i.header.flags.hasDescription)
		s >> i.description;
	if(i.header.flags.hasRelativePath)
		s >> i.relativePath;
	if(i.header.flags.hasWorkingDir)
		s >> i.workingDir;
	if(i.header.flags.hasCmdLineArgs)
		s >> i.cmdLineArgs;
	if(i.header.flags.hasCustomIcon)
		s >> i.customIcon;
	return s;
}

void FXWinShellLink::write(FXStream s, const QFileInfo &whereTo)
{
	header.flags.hasItemIdList=true;
	header.flags.pointsToFileOrDir=true;
	header.flags.useWorkingDir=true;
	header.fileattribs.isReadOnly=whereTo.isWriteable();
	header.fileattribs.isHidden=whereTo.isHidden();
	header.fileattribs.isDir=whereTo.isDir();
	header.fileattribs.isModified=whereTo.isFile();		// only files have archive bit set
	header.creation=(whereTo.created().value-FXTime::mics1stJan1601)*10;
	header.modified=(whereTo.lastModified().value-FXTime::mics1stJan1601)*10;
	header.lastAccess=(whereTo.lastRead().value-FXTime::mics1stJan1601)*10;
	header.filelength=(FXuint) whereTo.size();

	FXString convertedPath(whereTo.filePath());
#ifdef USE_POSIX
	{
		bool done=false;
		QValueList<FXProcess::MountablePartition> mounts=FXProcess::mountablePartitions();
		for(QValueList<FXProcess::MountablePartition>::const_iterator it=mounts.begin(); it!=mounts.end(); ++it)
		{
			if(it->mounted && it->driveLetter)
			{
				if(!compare(it->location, convertedPath, it->location.length()))
				{
					FXString ins(&it->driveLetter, 1); ins.append(':');
					convertedPath.replace(0, it->location.length(), ins);
					done=true;
				}
			}
		}
		convertedPath.substitute('/', '\\');
		FXERRH(done, QTrans::tr("FXWinShellLink", "Couldn't find drive name translation for '%1'").arg(whereTo.filePath()), FXWINSHELLLINK_NODRIVECONVERSION, 0);
	}
#endif

	if(header.flags.hasItemIdList)
	{
#if FOX_MAJOR>1 || FOX_MINOR>=6
		memcpy(itemIdList.path1, convertedPath.text(), convertedPath.length()+1);
		utf2ncs(itemIdList.path2, convertedPath.text(), convertedPath.length()+1);
#else
		for(int n=0; n<whereTo.filePath().length()+1; n++)
		{
			itemIdList.path1[n]=convertedPath[n];
			itemIdList.path2[n]=convertedPath[n];
		}
#endif
		memcpy(itemIdList.originalPath, whereTo.filePath().text(), whereTo.filePath().length()+1);
	}
	if('\\'==convertedPath[0] && '\\'==convertedPath[1])
	{
		fileLocation.flags.onNetworkShare=true;
		memcpy(fileLocation.networkVolume.shareName, convertedPath.text(), convertedPath.length()+1);
	}
	else
	{
		fileLocation.flags.onLocalVolume=true;
		fileLocation.localVolume.type=FXWinShellLink::FileLocationTag::LocalVolume::Fixed;	// should really interrogate
		// Volume serial no & label is hard to know on POSIX
		fileLocation.localVolume.serialNo=0xffffffff;
		memcpy(fileLocation.basePath, convertedPath.text(), convertedPath.length()+1);
	}
	{	// Always seems to have a relative path
		FXString _relativePath(".."+convertedPath.mid(convertedPath.find('\\')));
#if FOX_MAJOR>1 || FOX_MINOR>=6
		utf2ncs(relativePath.string, _relativePath.text(), _relativePath.length()+1);
#else
		for(int n=0; n<_relativePath.length()+1; n++)
			relativePath.string[n]=_relativePath[n];
#endif
	}
	if(whereTo.isFile())
	{	// Has a working directory if it's a file
		FXString _workingDir(FXFile::directory(convertedPath));
#if FOX_MAJOR>1 || FOX_MINOR>=6
		utf2ncs(workingDir.string, _workingDir.text(), _workingDir.length()+1);
#else
		for(int n=0; n<_workingDir.length()+1; n++)
			workingDir.string[n]=_workingDir[n];
#endif
	}
	header.flags.hasDescription =(description.string[0]!=0);
	header.flags.hasRelativePath=(relativePath.string[0]!=0);
	header.flags.hasWorkingDir  =(workingDir.string[0]!=0);
	header.flags.hasCmdLineArgs =(cmdLineArgs.string[0]!=0);
	header.flags.hasCustomIcon  =(customIcon.string[0]!=0);

	s << header;
	if(header.flags.hasItemIdList)
		s << itemIdList;
	s << fileLocation;
	if(header.flags.hasDescription)
		s << description;
	if(header.flags.hasRelativePath)
		s << relativePath;
	if(header.flags.hasWorkingDir)
		s << workingDir;
	if(header.flags.hasCmdLineArgs)
		s << cmdLineArgs;
	if(header.flags.hasCustomIcon)
		s << customIcon;
}

FXString FXWinShellLink::read(const FXString &path, bool doDriveConversion)
{
	FXFile h(path);
	FXStream s(&h);
	h.open(IO_ReadOnly);
	FXWinShellLink sl;
	s >> sl;
	FXString ret;
	if(sl.header.flags.hasItemIdList)
	{	// We prefer this as it's in unicode
#ifdef UNICODE
		ret=sl.itemIdList.path2;
#else
		ret=sl.itemIdList.path1;
#endif
	}
	else
	{
		ret=(sl.fileLocation.flags.onLocalVolume ? sl.fileLocation.basePath : sl.fileLocation.networkVolume.shareName);
		if(sl.fileLocation.remainingPath[0])
			ret+="\\"+FXString(sl.fileLocation.remainingPath);
	}
#ifdef PRINT_DEBUG
	fxmessage("Shell link '%s' points to '%s'\n\n", path.text(), ret.text());
#endif
#ifdef USE_POSIX
	if(doDriveConversion)
	{
		bool done=false;
		ret.substitute('\\', '/');
		QValueList<FXProcess::MountablePartition> mounts=FXProcess::mountablePartitions();
		for(QValueList<FXProcess::MountablePartition>::const_iterator it=mounts.begin(); it!=mounts.end(); ++it)
		{
			if(it->mounted && it->driveLetter)
			{
				if(it->driveLetter==toupper(ret[0]))
				{
					ret.replace(0, 2, it->location);
					done=true;
					break;
				}
			}
		}
		FXERRH(done, QTrans::tr("FXWinShellLink", "Couldn't find drive name translation for '%1'").arg(ret), FXWINSHELLLINK_NODRIVECONVERSION, 0);
	}
#endif
	return ret;
}

void FXWinShellLink::write(const FXString &path, const QFileInfo &whereTo)
{
	FXERRH(whereTo.exists(), QTrans::tr("FXWinShellLink", "Destination '%1' must exist").arg(whereTo.filePath()), FXEXCEPTION_NOTFOUND, 0);
	FXFile h(path);
	FXStream s(&h);
	h.open(IO_WriteOnly);
	FXWinShellLink().write(s, whereTo);
}

//*******************************************************************************

#ifdef USE_WINAPI
// Seeing as this isn't defined in the Platform SDK ...
struct REPARSE_TAG_MOUNT_POINT
{
	WORD SubstituteNameOffset;
	WORD SubstituteNameLength;
	WORD PrintNameOffset;
	WORD PrintNameLength;
	WCHAR Path[1];
};
#endif

bool FXWinJunctionPoint::test(const FXString &path)
{
	return !!(FXFile::metaFlags(path) & FXFile::IsLink);
}

FXString FXWinJunctionPoint::read(const FXString &_path)
{
	FXERRH(test(_path), QTrans::tr("FXWinJunctionPoint", "'%1' is not a junction point").arg(_path), FXWINJUNCTIONPOINT_INVALID, 0);
#ifdef USE_WINAPI
	HANDLE h;
	FXERRHWINFN(INVALID_HANDLE_VALUE!=(h=CreateFile(FXUnicodify<>(_path, true).buffer(), GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT, 0)), _path);
	{
		FXRBOp unh=FXRBFunc(&CloseHandle, h);
		char buffer[1024];
		REPARSE_GUID_DATA_BUFFER *rgdb=(REPARSE_GUID_DATA_BUFFER *) buffer;
		REPARSE_TAG_MOUNT_POINT *rtmp=(REPARSE_TAG_MOUNT_POINT *) (buffer+8);
		DWORD bytes;
		FXERRHWIN(DeviceIoControl(h, FSCTL_GET_REPARSE_POINT, NULL, 0, (LPVOID) rgdb, sizeof(buffer), &bytes, 0));
		FXERRH(IO_REPARSE_TAG_MOUNT_POINT==rgdb->ReparseTag, QTrans::tr("FXWinJunctionPoint", "'%1' is not a junction point").arg(_path), FXWINJUNCTIONPOINT_INVALID, 0);
		WCHAR *ret=rtmp->Path;
		if('\\'==ret[0] && '?'==ret[1] && '?'==ret[2] && '\\'==ret[3])
			ret+=4;
#ifdef UNICODE
		return FXString((FXnchar *) ret, rtmp->SubstituteNameLength);
#else
		FXString r;
		for(; *ret; ret++)
			r.append((FXchar) *ret);
		return r;
#endif
	}
#endif
#ifdef USE_POSIX
	FXString ret=FXFile::symlink(_path);
	if('/'!=ret[0])
	{	// Partial path
		ret=FXFile::absolute(FXFile::directory(_path), ret);
	}
	return ret;
#endif
}

void FXWinJunctionPoint::write(const FXString &_path, const QFileInfo &whereTo)
{
#ifdef USE_WINAPI
	// Here comes a significant amount of pain in the ass :(
	//   - why doesn't MS have an API for this???
	TCHAR to[MAX_PATH];
	if('\\'==whereTo.filePath()[0] && '?'==whereTo.filePath()[1])
	{
#ifdef UNICODE
		utf2ncs(to, whereTo.filePath().text(), whereTo.filePath().length()+1);
#else
		memcpy(to, whereTo.filePath().text(), whereTo.filePath().length()+1);
#endif
	}
	else
	{	// Need to prepend with \??\ if it's a drive letter path
		LPTSTR foo;
		to[0]=to[3]='\\'; to[1]=to[2]='?';
		FXERRHWIN(GetFullPathName(FXUnicodify<>(whereTo.filePath(), true).buffer(), (sizeof(to)/sizeof(TCHAR))-4, to+4, &foo));
	}
	WORD tolen=0;
	for(TCHAR *to2=to; *to2; ++tolen, ++to2);
	FXUnicodify<> path(_path, true);
	if(!CreateDirectory(path.buffer(), NULL) && ERROR_ALREADY_EXISTS!=GetLastError())
	{
		FXERRHWINFN(0, _path);
	}
	FXRBOp uncreatedir=FXRBFunc(&RemoveDirectory, path.buffer());

	// Ok generate a suitable NTFS reparse structure
	char buffer[1024];
#ifdef DEBUG
	memset(buffer, 0xff, sizeof(buffer));
#endif
	REPARSE_GUID_DATA_BUFFER *rgdb=(REPARSE_GUID_DATA_BUFFER *) buffer;
	REPARSE_TAG_MOUNT_POINT *rtmp=(REPARSE_TAG_MOUNT_POINT *) (buffer+8);
	rgdb->ReparseTag=IO_REPARSE_TAG_MOUNT_POINT;
	rgdb->ReparseDataLength=tolen*sizeof(WCHAR)+12;
	rgdb->Reserved=0;
	rtmp->SubstituteNameOffset=0;
	rtmp->SubstituteNameLength=tolen*sizeof(WCHAR);
	rtmp->PrintNameOffset=tolen*sizeof(WCHAR)+2;
	rtmp->PrintNameLength=0;
#ifdef UNICODE
	memcpy(rtmp->Path, to, (tolen+1)*sizeof(TCHAR));
#else
	for(int n=0; n<tolen+1; n++)
		rtmp->Path[n]=to[n];
#endif

	// Now open the directory we made and turn it into a reparse point
	HANDLE h;
	FXERRHWINFN(INVALID_HANDLE_VALUE!=(h=CreateFile(path.buffer(), GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT, 0)), _path);
	{	// Mark as compressed (to make it blue)
		FXRBOp unh=FXRBFunc(&CloseHandle, h);
		USHORT par=COMPRESSION_FORMAT_DEFAULT;
		DWORD bytes;
		FXERRHWIN(DeviceIoControl(h, FSCTL_SET_COMPRESSION, (LPVOID) &par, sizeof(par), NULL, 0, &bytes, NULL));
		// Set the reparse info
		FXERRHWIN(DeviceIoControl(h, FSCTL_SET_REPARSE_POINT, (LPVOID) rgdb, rgdb->ReparseDataLength+8, NULL, 0, &bytes, NULL));
	}
	// Mark as system so you'll get a warning from Explorer if you try deleting it
	FXERRHWINFN(SetFileAttributes(path.buffer(), FILE_ATTRIBUTE_NOT_CONTENT_INDEXED|FILE_ATTRIBUTE_READONLY|FILE_ATTRIBUTE_SYSTEM), _path);
	uncreatedir.dismiss();
#endif
#ifdef USE_POSIX
	FXERRH(FXFile::symlink(whereTo.filePath(), _path, true), QTrans::tr("FXWinJunctionPoint", "Failed to link '%1' to '%2'").arg(whereTo.filePath()).arg(_path), FXEXCEPTION_NOTFOUND, 0);
#endif
}

void FXWinJunctionPoint::remove(const FXString &_path)
{
#ifdef USE_WINAPI
	FXUnicodify<> path(_path, true);
	FXERRHWINFN(SetFileAttributes(path.buffer(), FILE_ATTRIBUTE_NORMAL), _path);
	FXERRHWINFN(RemoveDirectory(path.buffer()), _path);
#endif
#ifdef USE_POSIX
	FXERRHOSFN(unlink(_path.text()), _path);
#endif
}

} // namespace
