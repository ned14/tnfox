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

#include "xincs.h"
#include "QFileInfo.h"
#include "FXException.h"
#include "FXRollback.h"
#include "FXFile.h"
#include "FXACL.h"
#include "QTrans.h"
#ifndef USE_POSIX
#define USE_WINAPI
#include "WindowsGubbins.h"
#define S_ISREG(m) ((m & _S_IFREG)==_S_IFREG)
#define S_ISDIR(m) ((m & _S_IFDIR)==_S_IFDIR)
#define S_ISLNK(m) (false)
#endif
#include "FXMemDbg.h"
#if defined(DEBUG) && defined(FXMEMDBG_H)
static const char *_fxmemdbg_current_file_ = __FILE__;
#endif


namespace FX {


struct FXDLLLOCAL QFileInfoPrivate
{
	bool cached, exists;
	bool readable, writeable, executable;
	FXString pathname, linkedto;
	FXuint flags, hardLinks;
	FXfval size, compressedSize;
	FXTime created, lastModified, lastAccessed;
	FXACL permissions;
	QFileInfoPrivate(const FXString &path) : cached(true), exists(false), 
		readable(false), writeable(false), executable(false),
		pathname(path), flags(0), hardLinks(0), size(0), compressedSize(0), permissions() { }
};

QFileInfo::QFileInfo() : p(0)
{
	FXERRHM(p=new QFileInfoPrivate(FXString::nullStr()));
}
QFileInfo::QFileInfo(const FXString &path) : p(0)
{
	FXRBOp unconstr=FXRBConstruct(this);
	FXERRHM(p=new QFileInfoPrivate(path));
	refresh();
	unconstr.dismiss();
}
QFileInfo::QFileInfo(const FXFile &file) : p(0)
{
	FXRBOp unconstr=FXRBConstruct(this);
	FXERRHM(p=new QFileInfoPrivate(file.name()));
	refresh();
	unconstr.dismiss();
}
QFileInfo::QFileInfo(const QDir &dir, const FXString &leafname) : p(0)
{
	FXRBOp unconstr=FXRBConstruct(this);
	FXERRHM(p=new QFileInfoPrivate(dir.filePath(leafname)));
	refresh();
	unconstr.dismiss();
}
QFileInfo::QFileInfo(const QFileInfo &o) : p(0)
{
	if(o.p) { FXERRHM(p=new QFileInfoPrivate(*o.p)); }
}
QFileInfo &QFileInfo::operator=(const QFileInfo &o)
{
	*p=*o.p;
	return *this;
}
QFileInfo::~QFileInfo()
{ FXEXCEPTIONDESTRUCT1 {
	FXDELETE(p);
} FXEXCEPTIONDESTRUCT2; }
bool QFileInfo::operator<(const QFileInfo &o) const
{
	FXint ret;
	if((ret=comparecase(p->pathname, o.p->pathname))==0)
	{
		if(p->size        <o.p->size)  return true;
		if(p->lastModified<o.p->lastModified) return true;
		if(p->lastAccessed<o.p->lastAccessed) return true;
		if(p->created     <o.p->created) return true;
		if(p->hardLinks   <o.p->hardLinks) return true;
	}
	return ret<0;
}
bool QFileInfo::operator==(const QFileInfo &o) const
{
	if(comparecase(p->pathname, o.p->pathname)!=0) return false;
	if(p->size        !=o.p->size)  return false;
	if(p->lastModified!=o.p->lastModified) return false;
	if(p->lastAccessed!=o.p->lastAccessed) return false;
	if(p->created     !=o.p->created) return false;
	if(p->hardLinks   !=o.p->hardLinks) return false;
	if(p->permissions !=o.p->permissions) return false;
	return true;
}
bool QFileInfo::operator>(const QFileInfo &o) const
{
	FXint ret;
	if((ret=comparecase(p->pathname, o.p->pathname))==0)
	{
		if(p->size        >o.p->size)  return true;
		if(p->lastModified>o.p->lastModified) return true;
		if(p->lastAccessed>o.p->lastAccessed) return true;
		if(p->created     >o.p->created) return true;
		if(p->hardLinks   >o.p->hardLinks) return true;
	}
	return ret>0;
}

void QFileInfo::setFile(const FXString &path)
{
	p->pathname=path;
	refresh();
}
void QFileInfo::setFile(const FXFile &file)
{
	p->pathname=file.name();
	refresh();
}
void QFileInfo::setFile(const QDir &dir, const FXString &leafname)
{
	p->pathname=dir.path()+QDir::separator()+leafname;
	refresh();
}
bool QFileInfo::exists() const
{
	if(p->cached) return p->exists;
	return FXFile::exists(p->pathname)!=0;
}
void QFileInfo::refresh()
{
	if(p->cached)
	{
		if((p->exists=FXFile::exists(p->pathname)!=0))
		{
			p->readable=FXFile::isReadable(p->pathname)!=0;
			p->writeable=FXFile::isWritable(p->pathname)!=0;
			p->executable=FXFile::isExecutable(p->pathname)!=0;
			p->linkedto=FXFile::symlink(p->pathname);
			FXFile::readMetadata(p->pathname, &p->flags, &p->size, &p->created, &p->lastModified, &p->lastAccessed, &p->compressedSize, &p->hardLinks);
			// We may not be allowed to read the permissions, so
			// handle failure gracefully
			try
			{
				p->permissions=FXACL(p->pathname, isFile() ? FXACL::File : FXACL::Directory);
			}
			catch(FXException &)
			{	// Give it a null ACL owned by root
				p->permissions=FXACL(isFile() ? FXACL::File : FXACL::Directory, FXACLEntity::root());
			}
			// It's possible that file has been deleted during our metadata
			// scan, so make very sure
			if(!(p->exists=FXFile::exists(p->pathname)!=0))
			{
				p->readable=p->writeable=p->executable=false;
				p->linkedto=FXString::nullStr();
				p->flags=0; p->size=0;
				p->created=p->lastModified=p->lastAccessed=FXTime();
				p->compressedSize=0; p->hardLinks=0;
			}
		}
	}
}
bool QFileInfo::caching() const
{
	return p->cached;
}
void QFileInfo::setCaching(bool newon)
{
	p->cached=newon;
}
const FXString &QFileInfo::filePath() const
{
	return p->pathname;
}
FXString QFileInfo::fileName() const
{
	return FXFile::name(p->pathname);
}
FXString QFileInfo::absFilePath() const
{
	return FXFile::absolute(p->pathname);
}
FXString QFileInfo::baseName(bool complete) const
{
	FXString ret=FXFile::name(p->pathname);
	FXint dotpos=(complete) ? ret.rfind('.') : ret.find('.');
	if(-1==dotpos) dotpos=ret.length();
	else dotpos-=1;
	return ret.left(dotpos);
}
FXString QFileInfo::extension(bool complete) const
{
	FXString ret=FXFile::name(p->pathname);
	FXint dotpos=(complete) ? ret.rfind('.') : ret.find('.');
	if(-1==dotpos) dotpos=0;
	else dotpos+=1;
	return ret.mid(dotpos, ret.length());
}
FXString QFileInfo::dirPath(bool absPath) const
{
	return absPath ? FXFile::absolute(FXFile::directory(p->pathname)) : FXFile::directory(p->pathname);
}
bool QFileInfo::isReadable() const
{
	return p->cached ? p->readable : FXFile::isReadable(p->pathname)!=0;
}
bool QFileInfo::isWriteable() const
{
	return p->cached ? p->writeable : FXFile::isWritable(p->pathname)!=0;
}
bool QFileInfo::isExecutable() const
{
	return p->cached ? p->executable : FXFile::isExecutable(p->pathname)!=0;
}
FXuint QFileInfo::metaFlags() const
{
	return p->cached ? p->flags : FXFile::metaFlags(p->pathname);
}
bool QFileInfo::isHidden() const
{
	return !!((p->cached ? p->flags : FXFile::metaFlags(p->pathname)) & FXFile::IsHidden);
}
bool QFileInfo::isRelative() const
{
	return !FXFile::isAbsolute(p->pathname);
}
bool QFileInfo::convertToAbs()
{
	if(isRelative())
	{
		p->pathname=FXFile::absolute(p->pathname);
		return true;
	}
	return false;
}
bool QFileInfo::isFile() const
{
	return !!((p->cached ? p->flags : FXFile::metaFlags(p->pathname)) & FXFile::IsFile);
}
bool QFileInfo::isDir() const
{
	return !!((p->cached ? p->flags : FXFile::metaFlags(p->pathname)) & FXFile::IsDirectory);
}
bool QFileInfo::isSymLink() const
{
	return !!((p->cached ? p->flags : FXFile::metaFlags(p->pathname)) & FXFile::IsLink);
}
FXString QFileInfo::readLink() const
{
	return p->cached ? p->linkedto : FXFile::symlink(p->pathname);
}
const FXACLEntity &QFileInfo::owner() const
{
	return permissions().owner();
}
const FXACL &QFileInfo::permissions() const
{
	if(!p->cached) const_cast<QFileInfoPrivate *>(p)->permissions=FXFile::permissions(p->pathname);
	return p->permissions;
}
bool QFileInfo::permission(FXACL::Perms what) const
{
	if(!p->cached) const_cast<QFileInfoPrivate *>(p)->permissions=FXFile::permissions(p->pathname);
	return p->permissions.check(what);
}
FXString QFileInfo::permissionsAsString() const
{
	return permissions().report();
}
FXfval QFileInfo::size() const
{
	return p->cached ? p->size : FXFile::size(p->pathname);
}
FXString QFileInfo::sizeAsString() const
{
	return fxstrfval(size());
}
FXTime QFileInfo::created() const
{
	return p->cached ? p->created : FXFile::created(p->pathname);
}
FXString QFileInfo::createdAsString(const FXString &format, bool inLocalTime) const
{
	FXTime t(created());
	if(inLocalTime) t.toLocalTime();
	return t.asString(format);
}
FXTime QFileInfo::lastModified() const
{
	return p->cached ? p->lastModified : FXFile::modified(p->pathname);
}
FXString QFileInfo::lastModifiedAsString(const FXString &format, bool inLocalTime) const
{
	FXTime t(lastModified());
	if(inLocalTime) t.toLocalTime();
	return t.asString(format);
}
FXTime QFileInfo::lastRead() const
{
	return p->cached ? p->lastAccessed : FXFile::accessed(p->pathname);
}
FXString QFileInfo::lastReadAsString(const FXString &format, bool inLocalTime) const
{
	FXTime t(lastRead());
	if(inLocalTime) t.toLocalTime();
	return t.asString(format);
}

} // namespace

