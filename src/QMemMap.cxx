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

#include "xincs.h"
#include "fxdefs.h"
#include "QMemMap.h"
#include "FXString.h"
#include "QFile.h"
#include "FXException.h"
#include "QThread.h"
#include "FXProcess.h"
#include "FXRollback.h"
#include "QTrans.h"
#include "FXErrCodes.h"
#include "FXPtrHold.h"
#include "FXACL.h"
#include <qsortedlist.h>
#include <qcstring.h>
#include <string.h>
#include "FXMemDbg.h"
#if defined(DEBUG) && defined(FXMEMDBG_H)
static const char *_fxmemdbg_current_file_ = __FILE__;
#endif

#ifndef USE_POSIX
#define USE_WINAPI
#include "WindowsGubbins.h"
#endif
#ifdef USE_POSIX
#include <sys/mman.h>
#ifdef __linux__
// Linux's /dev/shm doesn't have a /tmp directory defined :(
#define POSIX_SHARED_MEM_PREFIX "/TnFOX_"
#else
// BSD and other systems map shm_open() to normal open() so use /tmp
#define POSIX_SHARED_MEM_PREFIX "/tmp/TnFOX_"
#endif
#endif

namespace FX {

#ifdef USE_WINAPI
#ifndef SE_CREATE_GLOBAL_NAME			// Not present on older SDK's
#define SE_CREATE_GLOBAL_NAME			TEXT("SeCreateGlobalPrivilege")
#endif

class QMemMapInit
{	// Cached here to speed FXACL::check()
public:
	BOOL haveCreateGlobalNamePriv;
	QMemMapInit() : haveCreateGlobalNamePriv(0)
	{
		HANDLE myprocessh;
		DWORD pssize;
		PRIVILEGE_SET *ps;
		FXERRHWIN(OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY|TOKEN_DUPLICATE|TOKEN_ADJUST_PRIVILEGES, &myprocessh));
		FXRBOp unh=FXRBFunc(&CloseHandle, myprocessh);
		pssize=sizeof(PRIVILEGE_SET)+1*sizeof(LUID_AND_ATTRIBUTES);
		FXERRHM(ps=(PRIVILEGE_SET *) malloc(pssize));
		FXRBOp unps=FXRBFunc(&free, ps, (FXMemoryPool *) 0, (FXuint) 0);
		ps->PrivilegeCount=1;
		ps->Control=0;
		ps->Privilege[0].Attributes=0;
		if(LookupPrivilegeValue(0, SE_CREATE_GLOBAL_NAME, &ps->Privilege[0].Luid))
		{
			FXERRHWIN(PrivilegeCheck(myprocessh, ps, &haveCreateGlobalNamePriv));
		}
		else
			haveCreateGlobalNamePriv=1;
	}
};
static QMemMapInit qmemmapinit;
#endif

struct FXDLLLOCAL Mapping
{
	FXfval offset, len;
	bool copyOnWrite;
	void *addr, *oldaddr;
	Mapping(FXfval o, FXfval l=0, bool cow=false) : offset(o), len(l), copyOnWrite(cow), addr(0), oldaddr(0) { }
	bool operator==(const Mapping &o) const { return offset==o.offset; }
	bool operator<(const Mapping &o) const { return offset<o.offset; }
};
struct FXDLLLOCAL QMemMapPrivate : public QMutex
{
	QMemMap::Type type;
	bool myfile, creator, mappingsFailed, unique;
	QFile *file;
	int filefd;				// Either than of file (all platforms) or shared memory object (POSIX)
	FXString name;
	FXfval size;			// Size of file. Doesn't necessarily reflect file->size()
	QByteArray ungetchbuffer;
	FXACL acl;

	QSortedList<Mapping> mappings;
	FXuint pageSize;		// Page size of machine
	Mapping *cmapping;		// Mapping ioIndex is in, or zero if it isn't
	QSortedListIterator<Mapping> cmappingit;
#ifdef USE_WINAPI
	HANDLE mappingh;
	DWORD pageaccess;
#endif
#ifdef USE_POSIX
	int pageaccess;
#endif
	QMemMapPrivate(QMemMap::Type _type, QFile *f=0) : type(_type), myfile(0==f), creator(false),
		mappingsFailed(false), unique(false), file(f), filefd(0), size(0), acl(FXACL::MemMap), mappings(true),
		pageSize(FXProcess::pageSize()), cmapping(0), cmappingit(mappings),
#ifdef USE_WINAPI
		mappingh(0),
#endif
		pageaccess(0), QMutex() { }
	~QMemMapPrivate() { if(myfile) FXDELETE(file); }
	void setPrivatePerms()
	{
		acl.append(FXACL::Entry(FXACLEntity::owner(), 0, FXACL::Permissions().setAll()));
	}
	void setAllAccessPerms()
	{
		acl.append(FXACL::Entry(FXACLEntity::everything(), 0, FXACL::Permissions().setAll()));
	}
	inline Mapping *findMapping(FXfval offset, QSortedListIterator<Mapping> *outit=0) const
	{
		QSortedListIterator<Mapping> it=mappings.findClosestIter(&Mapping(offset));
		Mapping *m=it.current();
		if(!m) return 0;
		// findClosestIter() returns item /after/ if offset is bigger
		if(m->offset>offset && !it.atFirst())
		{
			--it; m=it.current();
		}
		if(outit) *outit=it;
		return (offset>=m->offset && offset<m->offset+m->len) ? m : 0;
	}
	inline void *offsetToPtr(FXfval offset, Mapping *m=0) const
	{
		if(!m) m=cmapping;
		if(!m) return 0;
		return (void *)(FXuval)((FXfval)(FXuval)m->addr+offset-m->offset);
	}
	void map()
	{	// Maps in all unmapped regions
		Mapping *m=0;
		mappingsFailed=false;
		for(QSortedListIterator<Mapping> it(mappings); (m=it.current()); ++it)
		{
			if(!m->addr)
			{
#ifdef USE_WINAPI
				assert(mappingh);
				if(!mappingh) return;
				DWORD pageacc=pageaccess;
				if(m->copyOnWrite) pageacc|=FILE_MAP_COPY;
				m->addr=MapViewOfFileEx(mappingh, pageacc, (DWORD)(m->offset>>32),
					(DWORD)(m->offset), (DWORD) m->len, m->oldaddr);
				//m->addr=0; // for debugging purposes
#ifdef DEBUG
				if(!m->addr)
				{
					DWORD code=GetLastError();
					TCHAR buffer[1024];
					FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, code, 0, buffer, sizeof(buffer)/sizeof(TCHAR), 0);
					fxmessage("NOTE: QMemMap map of %s (%x:%x) failed with %s (%d)\n",
						name.text(), (FXuint) m->offset, (FXuint) m->len, buffer, code);
				}
#endif
#endif
#ifdef USE_POSIX
				int flags=(m->copyOnWrite) ? MAP_PRIVATE : MAP_SHARED;
				if(MAP_FAILED==(m->addr=::mmap(m->oldaddr, (size_t) m->len, pageaccess,
					flags, filefd, m->offset))) m->addr=0;
#ifdef DEBUG
				if(!m->addr)
				{
					fxmessage("NOTE: QMemMap map of %s (%x:%x) failed with %s (%d)\n",
						name.text(), (FXuint) m->offset, (FXuint) m->len, strerror(errno), errno);
				}
#endif
#endif
			}
			if(!m->addr) mappingsFailed=true;
		}
	}
	void unmap(FXfval offset, FXfval amount, bool delEntries=true)
	{
		QSortedListIterator<Mapping> it=mappings.findClosestIter(&Mapping(offset));
		if(!it.atFirst()) --it;
		if(!amount) amount=1;
		for(Mapping *m=0; (m=it.current()) && m->offset<offset+amount;)
		{
			if((offset-m->offset)<amount || (offset+amount)>m->offset)
			{
				if(m->addr)
				{
#ifdef USE_WINAPI
					FXERRHWIN(UnmapViewOfFile(m->addr));
#endif
#ifdef USE_POSIX
					FXERRHIO(::munmap(m->addr, (size_t) m->len));
#endif
					m->oldaddr=m->addr;
					m->addr=0;
				}
				if(m==cmapping) cmapping=0;
				if(delEntries)
				{
					QSortedListIterator<Mapping> cit(it);
					++it;
					mappings.removeByIter(cit);
					continue;
				}
			}
			++it;
		}
		if(mappings.isEmpty()) cmappingit.toFirst();
	}
};

QMemMap::QMemMap() : p(0), QIODevice()
{
	FXRBOp unconstr=FXRBConstruct(this);
	FXERRHM(p=new QMemMapPrivate(File));
	p->setPrivatePerms();
	FXERRHM(p->file=new QFile);
	p->myfile=true;
	unconstr.dismiss();
}

QMemMap::QMemMap(const FXString &filename) : p(0), QIODevice()
{
	FXRBOp unconstr=FXRBConstruct(this);
	FXERRHM(p=new QMemMapPrivate(File));
	p->setPrivatePerms();
	FXERRHM(p->file=new QFile(filename));
	p->myfile=true;
	p->name=p->file->name();
	unconstr.dismiss();
}

QMemMap::QMemMap(QFile &file) : p(0), QIODevice()
{
	FXRBOp unconstr=FXRBConstruct(this);
	FXERRHM(p=new QMemMapPrivate(File, &file));
	p->setPrivatePerms();
	p->name=p->file->name();
	unconstr.dismiss();
}

QMemMap::QMemMap(const FXString &name, FXuval len) : p(0), QIODevice()
{
	FXRBOp unconstr=FXRBConstruct(this);
	FXERRHM(p=new QMemMapPrivate(Memory));
	p->setAllAccessPerms();
	setName(name);
	p->size=len;
	unconstr.dismiss();
}

QMemMap::~QMemMap()
{ FXEXCEPTIONDESTRUCT1 {
	close();
	FXDELETE(p);
} FXEXCEPTIONDESTRUCT2; }

const FXString &QMemMap::name() const
{
	QMtxHold h(p);
	return p->name;
}

void QMemMap::setName(const FXString &name)
{
	QMtxHold h(p);
	close();
	p->name=name;
	if(p->file) p->file->setName(name);
	p->unique=!!name.empty();
}

bool QMemMap::isUnique() const
{
	QMtxHold h(p);
	return p->unique;
}

void QMemMap::setUnique(bool v)
{
	QMtxHold h(p);
	p->unique=v;
}

QMemMap::Type QMemMap::type() const
{
	QMtxHold h(p);
	return p->type;
}

void QMemMap::setType(QMemMap::Type type)
{
	QMtxHold h(p);
	p->type=type;
}

bool QMemMap::exists() const
{
	QMtxHold h(p);
	if(File==p->type)
		return p->file->exists();
	return isOpen();
}

bool QMemMap::remove()
{
	QMtxHold h(p);
	close();
	if(File==p->type)
		return p->file->remove();
	return true;
}

FXfval QMemMap::reloadSize()
{
	QMtxHold h(p);
	if(isOpen() && File==p->type)
		return p->file->reloadSize();
	return 0;
}

FXfval QMemMap::mappableSize() const
{
	QMtxHold h(p);
	return p->size;
}

void QMemMap::maximiseMappableSize()
{
	if(!isWriteable()) FXERRGIO(QTrans::tr("QMemMap", "Not open for writing"));
	if(isOpen() && File==p->type)
	{	// NOTE TO SELF: Keep consistent with truncate()
		QMtxHold h(p);
#ifdef USE_WINAPI
		{	// Ok, close mapping and reopen
			if(p->mappingh)
			{
				p->unmap(0, p->size, false);
				FXERRHWIN(CloseHandle(p->mappingh));
				p->mappingh=0;
			}
			FXRBOp closeit=FXRBObj(*this, &QMemMap::close);
			p->size=p->file->reloadSize();
			winopen(mode());
			closeit.dismiss();
			if(p->mappingh)
				p->map();	// In theory maps everything back to where it was (hopefully)
		}
#endif
#ifdef USE_POSIX
		{	// Considerably easier this, as it should be in fact
			p->size=p->file->reloadSize();
		}
#endif
	}
}

void *QMemMap::mapIn(FXfval offset, FXfval amount, bool copyOnWrite)
{
	QMtxHold h(p);
	Mapping *m;
	if((FXfval) -1==amount) amount=p->size;
	if(!amount) return 0;	// Can't have null maps
	offset&=p->pageSize-1;	// Round down to page size
	p->unmap(offset, amount);
	bool noMappings=p->mappings.isEmpty();
	FXERRHM(m=new Mapping(offset, amount, copyOnWrite));
	FXRBOp unm=FXRBNew(m);
	p->mappings.insert(m);
	unm.dismiss();
	if(noMappings) p->cmappingit.toFirst();
#ifdef USE_WINAPI
	if(!p->mappingh) winopen(mode());
#endif
	p->map();
	setIoIndex(ioIndex);
	return m->addr;
}

void QMemMap::mapOut(FXfval offset, FXfval amount)
{
	QMtxHold h(p);
	if((FXfval) -1==amount) amount=p->size;
	p->unmap(offset, amount);
	if(p->mappingsFailed) p->map();
}

void QMemMap::mapOut(void *area)
{
	QMtxHold h(p);
	QSortedListIterator<Mapping> it(p->mappings);
	for(Mapping *m; (m=it.current()); ++it)
	{
		if(m->addr==area)
		{
			p->unmap(m->offset, m->len);
			if(p->mappingsFailed) p->map();
			break;
		}
	}
}

bool QMemMap::mappedRegion(QMemMap::MappedRegion *current, QMemMap::MappedRegion *next, FXfval offset) const
{
	bool ret=false;
	QMtxHold h(p);
	if((FXfval) -1==offset)
		offset=ioIndex;
	QSortedListIterator<Mapping> it=p->mappings.findClosestIter(&Mapping(offset));
	Mapping *m=it.current();
	if(!m)
		m=it.toLast();
	else if(m->offset>offset && !it.atFirst())
		m=--it;
	if(m && offset>=m->offset && offset<m->offset+m->len)
	{
		if(current)
			*current=MappedRegion(m->offset, m->len, m->addr);
		m=++it;
		ret=true;
	}
	else
	{
		if(current)
			*current=MappedRegion(0, 0, 0);
	}
	if(m && m->offset>offset)
	{
		if(next)
			*next=MappedRegion(m->offset, m->len, m->addr);
		ret=true;
	}
	else
	{
		if(next)
			*next=MappedRegion(0, 0, 0);
	}
	return ret;
}
void *QMemMap::mapOffset(FXfval offset) const
{
	QMtxHold h(p);
	if((FXfval) -1==offset)
	{
		if(p->cmapping)
			return p->offsetToPtr(ioIndex);
		else
			return 0;
	}
	Mapping *m=p->findMapping(offset);
	if(m)
		return p->offsetToPtr(offset, m);
	else
		return 0;
}

void QMemMap::winopen(int mode)
{
#ifdef USE_WINAPI
	HANDLE fileh=INVALID_HANDLE_VALUE;
	DWORD access=0;
	if(File==p->type) fileh=(HANDLE) _get_osfhandle(p->filefd);
	p->pageaccess|=READ_CONTROL|DELETE;
	if(mode & IO_WriteOnly)
	{
		access|=PAGE_READWRITE;
		p->pageaccess|=FILE_MAP_WRITE;
	}
	else if(mode & IO_ReadOnly)
	{
		access|=PAGE_READONLY;
		p->pageaccess|=FILE_MAP_READ;
	}
	FXfval mappingsize=p->size;
	if(INVALID_HANDLE_VALUE==fileh)
	{	
		if(!mappingsize) mappingsize=1;
		FXString name;
		do
		{
			if(p->unique)
				p->name=FXString("%1_%2").arg(FXProcess::id()).arg(rand(),0,16);
			name=FXString("Global\\"+p->name);
			if(qmemmapinit.haveCreateGlobalNamePriv)
			{
				if(mode & IO_WriteOnly)
				{
					SECURITY_ATTRIBUTES sa={ sizeof(SECURITY_ATTRIBUTES) };
					sa.lpSecurityDescriptor=p->acl.int_toWin32SecurityDescriptor();
					p->mappingh=CreateFileMapping(fileh, &sa, access,
						(DWORD)(mappingsize>>32), (DWORD) mappingsize, FXUnicodify<>(name).buffer());
					if(p->unique && ERROR_ALREADY_EXISTS==GetLastError())
					{
						CloseHandle(p->mappingh);
						continue;
					}
				}
				else
					p->mappingh=OpenFileMapping(p->pageaccess, FALSE, FXUnicodify<>(name).buffer());
			}
			else
			{	// Not allowed create Global\ objects, so try opening and if not create in Local\.
				if(!(p->mappingh=OpenFileMapping(p->pageaccess, FALSE, FXUnicodify<>(name).buffer())))
				{
					name.replace(0, 6, "Local");
					if(mode & IO_WriteOnly)
					{
						SECURITY_ATTRIBUTES sa={ sizeof(SECURITY_ATTRIBUTES) };
						sa.lpSecurityDescriptor=p->acl.int_toWin32SecurityDescriptor();
						p->mappingh=CreateFileMapping(fileh, &sa, access,
							(DWORD)(mappingsize>>32), (DWORD) mappingsize, FXUnicodify<>(name).buffer());
						if(p->unique && ERROR_ALREADY_EXISTS==GetLastError())
						{
							CloseHandle(p->mappingh);
							continue;
						}
					}
					else
						p->mappingh=OpenFileMapping(p->pageaccess, FALSE, FXUnicodify<>(name).buffer());
				}
			}
		} while(false);
		FXERRHWINFN(p->mappingh, name);
		p->acl=FXACL(p->mappingh, FXACL::MemMap);
	}
	else if(mappingsize)
	{	// If zero sized mapping, can't create
		SECURITY_ATTRIBUTES sa={ sizeof(SECURITY_ATTRIBUTES) };
		sa.lpSecurityDescriptor=p->acl.int_toWin32SecurityDescriptor();
		FXERRHWIN(p->mappingh=CreateFileMapping(fileh, &sa, access,
			(DWORD)(mappingsize>>32), (DWORD) mappingsize, NULL));
		// Interestingly, trying to read the security descriptor of an
		// unnamed section does not work :(
	}
#endif
}

inline void QMemMap::setIoIndex(FXfval newpos)
{
	if(!p->cmapping) p->cmapping=p->findMapping(newpos, &p->cmappingit);
	else if(newpos>=p->cmapping->offset+p->cmapping->len)
	{	// Optimised
		++p->cmappingit;
		p->cmapping=p->cmappingit.current();
		if(p->cmapping && newpos<p->cmapping->offset) p->cmapping=0;
	}
	else if(newpos<p->cmapping->offset)
		p->cmapping=p->findMapping(newpos, &p->cmappingit);
	ioIndex=newpos;
}

bool QMemMap::open(FXuint mode)
{
	QMtxHold h(p);
	if(isOpen())
	{	// I keep fouling myself up here, so assertion check
		if(QIODevice::mode()!=mode) FXERRGIO(QTrans::tr("QMemMap", "Device reopen has different mode"));
	}
	else
	{
		if(File==p->type)
		{
			if(p->file->isOpen()) p->file->flush();
			int filemode=mode & ~IO_Translate;
			if((filemode & IO_ReadWrite)==IO_WriteOnly) filemode|=IO_ReadOnly|IO_Truncate;	// mmaps imply reading when writing
			p->file->open(filemode);
			p->filefd=p->file->int_fileDescriptor();
			p->size=p->file->size();
			const FXACLEntity &owner=p->file->permissions().owner();
			if(FXACLEntity::everything()!=owner)
			{	// Ensure mapping owner is always file owner
				p->acl.setOwner(owner);
			}
		}
		p->pageaccess=0;
#ifdef USE_WINAPI
		winopen(mode);
#endif
#ifdef USE_POSIX
		int access=0;
		if(IO_ReadWrite==(mode & IO_ReadWrite))
		{
			access|=O_RDWR;
			p->pageaccess|=PROT_READ|PROT_WRITE;
		}
		else if(mode & IO_ReadOnly)
		{
			access|=O_RDONLY;
			p->pageaccess|=PROT_READ;
		}
		else if(mode & IO_WriteOnly)
		{
			access|=O_RDWR;
			p->pageaccess|=PROT_READ|PROT_WRITE;
		}
		if(Memory==p->type)
		{
			FXString name;
			static QMutex locallock;	// Lock to prevent race between determining name doesn't exist and creating it
			QMtxHold h(locallock);
			do
			{
				if(p->unique)
					p->name=FXString("%1_%2").arg(FXProcess::id()).arg(rand(),0,16);
				name=FXString(POSIX_SHARED_MEM_PREFIX+p->name);
				p->filefd=::shm_open(name.text(), access, S_IREAD|S_IWRITE);
				if(p->unique && -1!=p->filefd)
				{
					::close(p->filefd);
					continue;
				}
				if(-1==p->filefd)
				{
					if(mode & IO_WriteOnly)
					{
						access|=O_CREAT;
						p->creator=true;
						FXERRHIO(p->filefd=::shm_open(name.text(), access, S_IREAD|S_IWRITE));
						FXERRHOS(::fcntl(p->filefd, F_SETFD, ::fcntl(p->filefd, F_GETFD, 0)|FD_CLOEXEC));
						FXERRHIO(::ftruncate(p->filefd, p->size));
						p->acl.writeTo(p->filefd);
					}
					else { FXERRHOSFN(-1, name); }
				}
			} while(false);
		}
#endif
		setFlags((mode & IO_ModeMask)|IO_Open);
		setIoIndex(0);
		if(!(mode & IO_NoAutoUTF) && isReadable() && isTranslated())
		{	// Have a quick peek to see what kind of text it is
			FXuchar buffer[256];
			FXuval read;
			if((read=QMemMap::readBlock((char *) buffer, sizeof(buffer))))
				setUnicodeTranslation(determineUnicodeType(buffer, read));
		}
		setIoIndex((mode & IO_Append) ? p->size : 0);
	}
	return true;
}

void QMemMap::close()
{
	if(isOpen())
	{
		QMtxHold h(p);
		mapOut();	// maps out everything
#ifdef USE_WINAPI
		if(p->mappingh)
		{
			FXERRHWIN(CloseHandle(p->mappingh));
			p->mappingh=0;
		}
#endif
#ifdef USE_POSIX
		if(Memory==p->type)
		{
			if(p->creator && !(flags() & IO_DontUnlink))
			{
				QThread_DTHold dth;
				FXString name(POSIX_SHARED_MEM_PREFIX+p->name);
				FXERRHIO(::close(p->filefd));
				FXERRHIO(::shm_unlink(name.text()));
			}
		}
#endif
		p->acl=FXACL(FXACL::MemMap);
		if(File==p->type)
		{
			if(p->myfile) p->file->close();
			p->setPrivatePerms();
		}
		else
			p->setAllAccessPerms();
		p->size=0;
		ioIndex=0;
		setFlags(0);
	}
}

void QMemMap::flush()
{
	if(isOpen() && isWriteable())
	{
		QMtxHold h(p);
		QSortedListIterator<Mapping> it(p->mappings);
		for(Mapping *m; (m=it.current()); ++it)
		{
			if(m->addr)
			{
				QThread_DTHold dth;
#ifdef USE_WINAPI
				FXERRHWIN(FlushViewOfFile(m->addr, (DWORD) m->len));
#endif
#ifdef USE_POSIX
				FXERRHIO(::msync(m->addr, (size_t) m->len, MS_SYNC|MS_INVALIDATE));
#endif
			}
		}
		if(p->file) p->file->flush();
	}
}

FXfval QMemMap::size() const
{
	if(isOpen())
	{
		// QMtxHold h(p); can do without
		return (File==p->type) ? p->file->size() : p->size;
	}
	return 0;
}

void QMemMap::truncate(FXfval newsize)
{
	if(!isWriteable()) FXERRGIO(QTrans::tr("QMemMap", "Not open for writing"));
	if(isOpen())
	{	// NOTE TO SELF: Keep consistent with maximiseMappableSize()
		QMtxHold h(p);
		if(newsize<p->size)
		{
			if(mode() & IO_ShredTruncate)
				shredData(newsize);
			mapOut(newsize);
		}
#ifdef USE_WINAPI
		{	// Ok, close mapping and reopen
			if(p->mappingh)
			{
				p->unmap(0, p->size, false);
				FXERRHWIN(CloseHandle(p->mappingh));
				p->mappingh=0;
			}
			FXRBOp closeit=FXRBObj(*this, &QMemMap::close);
			if(File==p->type) p->file->truncate(newsize);
			p->size=newsize;
			winopen(mode());
			closeit.dismiss();
			p->map();	// In theory maps everything back to where it was (hopefully)
		}
#endif
#ifdef USE_POSIX
		{	// Considerably easier this, as it should be in fact
			FXERRHIO(::ftruncate(p->filefd, newsize));
			p->size=newsize;
		}
#endif
		if(ioIndex>p->size) setIoIndex(p->size);
	}
}

FXfval QMemMap::at() const
{
	if(isOpen()) return ioIndex;
	return 0;
}

bool QMemMap::at(FXfval newpos)
{
	if(isOpen() && ioIndex!=newpos)
	{
		QMtxHold h(p);
		setIoIndex(newpos);
		p->ungetchbuffer.resize(0);
		return true;
	}
	return false;
}

bool QMemMap::atEnd() const
{
	if(isOpen())
	{
		return ioIndex>=((File==p->type) ? p->file->size() : p->size);
	}
	return false;
}

const FXACL &QMemMap::permissions() const
{
	return p->acl;
}
void QMemMap::setPermissions(const FXACL &perms)
{
	if(isOpen())
	{
#ifdef USE_WINAPI
		assert(p->mappingh);
		if(p->mappingh)
			perms.writeTo(p->mappingh);
#endif
#ifdef USE_POSIX
		if(Memory==p->type)
			perms.writeTo(p->filefd);
#endif
	}
	p->acl=perms;
}
FXACL QMemMap::permissions(const FXString &name)
{
#ifdef USE_WINAPI
	FXString _name="Global\\"+name;
	return FXACL(_name, FXACL::MemMap);
#endif
#ifdef USE_POSIX
	int fd;
	FXString sname(POSIX_SHARED_MEM_PREFIX+name);
	FXERRHIO(fd=::shm_open(sname.text(), O_RDWR, 0));
	FXRBOp unfd=FXRBFunc(::close, fd);
	return FXACL(fd, FXACL::MemMap);
#endif
}
void QMemMap::setPermissions(const FXString &name, const FXACL &perms)
{
#ifdef USE_WINAPI
	FXString _name="Global\\"+name;
	perms.writeTo(_name);
#endif
#ifdef USE_POSIX
	int fd;
	FXString sname(POSIX_SHARED_MEM_PREFIX+name);
	FXERRHIO(fd=::shm_open(sname.text(), O_RDWR, 0));
	FXRBOp unfd=FXRBFunc(::close, fd);
	perms.writeTo(fd);
#endif
}

FXuval QMemMap::readBlock(char *data, FXuval maxlen)
{
	QMtxHold h(p);
	if(!QIODevice::isReadable()) FXERRGIO(QTrans::tr("QMemMap", "Not open for reading"));
	FXfval mysize=(File==p->type) ? p->file->size() : p->size;
	if(isOpen() && ioIndex<mysize && maxlen)
	{
		FXuval readed=0;
		if(!p->ungetchbuffer.isEmpty())
		{
			FXuchar *ungetchdata=p->ungetchbuffer.data();
			FXuval ungetchlen=p->ungetchbuffer.size();
			FXuval copylen=FXMIN(ungetchlen, maxlen);
			memcpy(data, ungetchdata, copylen);
			readed+=copylen; setIoIndex(ioIndex+copylen);
			if(copylen<ungetchlen) memmove(ungetchdata, ungetchdata+copylen, ungetchlen-copylen);
			p->ungetchbuffer.resize((FXuint)(ungetchlen-copylen));
		}
		while(readed<maxlen)
		{
			Mapping *m;
			while((m=p->cmapping) && m->addr)
			{	// Use section
				FXfval offset=ioIndex-m->offset;
				FXuval left=FXMIN((maxlen-readed), (FXuval)(m->len-offset));
				void *from=(void *)(FXuval)(((FXfval)(FXuval) m->addr)+offset);
				memcpy(data+readed, from, left);
				setIoIndex(ioIndex+left); readed+=left;
				if(readed==maxlen) break;
			}
			if(readed==maxlen) break;
			FXERRH(p->file, QTrans::tr("QMemMap", "Unable to read unmapped shared memory"), QMEMMAP_NOTMAPPED, 0);
			// Ok do file read
			Mapping *nextm=p->cmappingit.current(); // Next mapping still held by iterator
			FXfval tillnextsection=((nextm && nextm->addr) ? nextm->offset : mysize)-ioIndex;
			p->file->at(ioIndex);
			FXuval left=FXMIN((maxlen-readed), (FXuval)(tillnextsection));
			FXuval read=p->file->readBlock(data+readed, left);
			if(!read) break;
			setIoIndex(ioIndex+read); readed+=read;
		}
		if(isTranslated())
		{
			QByteArray temp(maxlen);
			FXuval inputlen=readed;
			FXuval output=removeCRLF(temp.data(), (FXuchar *) data, maxlen, inputlen, unicodeTranslation());
			// Adjust the file pointer to reprocess unprocessed input later
			setIoIndex(ioIndex-((FXfval) readed-(FXfval) inputlen));
			memcpy(data, temp.data(), output);
			readed=output;
		}
		return readed;
	}
	return 0;
}

FXuval QMemMap::writeBlock(const char *data, FXuval maxlen)
{
	QMtxHold h(p);
	if(!isWriteable()) FXERRGIO(QTrans::tr("QMemMap", "Not open for writing"));
	if(isOpen() && maxlen)
	{
		FXuval written=0, trmaxlen=maxlen;
		FXPtrHold<QByteArray> tmpmem;
		if(isTranslated())
		{	// Only way I could think of doing it :(
			FXERRHM(tmpmem=new QByteArray(maxlen+4+maxlen/4));
			FXuval writ=0, inptr=0, inlen=maxlen;
			for(;;)
			{
				writ+=applyCRLF(tmpmem->data()+writ, (const FXuchar *) data+inptr, tmpmem->size()-inptr, inlen, crlfFormat(), unicodeTranslation());
				if(inlen+inptr<maxlen)
				{	// Extend
					tmpmem->resize((FXuint)(tmpmem->size()+4+maxlen/4));
					inptr=inlen;
					inlen=maxlen-inptr;
				}
				else break;
			}
			data=(const char *) tmpmem->data();
			maxlen=writ;
		}
		if(!p->ungetchbuffer.isEmpty()) p->ungetchbuffer.resize(0);
		while(written<maxlen)
		{
			Mapping *m;
			while((m=p->cmapping) && m->addr)
			{	// Use section
				FXfval offset=ioIndex-m->offset;
				FXuval left=FXMIN((maxlen-written), (FXuval)(m->len-offset));
				void *to=(void *)(FXuval)(((FXfval)(FXuval) m->addr)+offset);
				memcpy(to, data+written, left);
				setIoIndex(ioIndex+left); written+=left;
				if(written==maxlen) break;
			}
			if(written==maxlen) break;
			FXERRH(p->file, QTrans::tr("QMemMap", "Unable to write unmapped shared memory"), QMEMMAP_NOTMAPPED, 0);
			// Ok do file write
			Mapping *nextm=p->cmappingit.current(); // Next mapping still held by iterator
			FXfval tillnextsection=((nextm && nextm->addr) ? nextm->offset-ioIndex : (FXfval)-1);
			p->file->at(ioIndex);
			FXuval left=FXMIN((maxlen-written), (FXuval)(tillnextsection));
			FXuval writ=p->file->writeBlock(data+written, left);
			setIoIndex(ioIndex+writ); written+=writ;
		}
		if(isRaw()) flush();
		return isTranslated() ? trmaxlen : written;
	}
	return 0;
}

FXuval QMemMap::readBlockFrom(char *data, FXuval maxlen, FXfval newpos)
{
	if(isOpen())
	{
		QMtxHold h(p);
		if(ioIndex!=newpos)
		{
			setIoIndex(newpos);
			p->ungetchbuffer.resize(0);
		}
		return readBlock(data, maxlen);
	}
	return 0;
}
FXuval QMemMap::writeBlockTo(FXfval newpos, const char *data, FXuval maxlen)
{
	if(isOpen())
	{
		QMtxHold h(p);
		if(ioIndex!=newpos)
		{
			setIoIndex(newpos);
			p->ungetchbuffer.resize(0);
		}
		return writeBlock(data, maxlen);
	}
	return 0;
}

int QMemMap::getch()
{
	QMtxHold h(p);
	if(isOpen())
	{
		FXuchar ret;
		if(p->ungetchbuffer.isEmpty() && p->cmapping)
		{
			ret=*(FXuchar *)(p->offsetToPtr(ioIndex));
			setIoIndex(ioIndex+1);
			if(isTranslated() && 13==ret)
			{
				char nextch;
				FXuval read=1;
				if(p->cmapping)
				{
					nextch=*(FXchar *)(p->offsetToPtr(ioIndex));
					setIoIndex(ioIndex+1);
				}
				else
					read=readBlock(&nextch, 1);
				if(10!=nextch)
					ungetch(nextch);
				return 10;
			}
			return ret;
		}
		if(!readBlock((char *)&ret, 1)) return -1;
		return (int) ret;
	}
	return -1;
}

int QMemMap::putch(int c)
{
	QMtxHold h(p);
	if(isOpen())
	{
		FXuchar val=(FXuchar) c;
		if(p->cmapping)
		{
			if(isTranslated() && 10==val)
			{
				FXuchar buff[2];
				FXuval inputlen=1;
				FXuval towrite=applyCRLF(buff, &val, 2, inputlen, crlfFormat(), unicodeTranslation());
				if(2==towrite)
				{
					*(FXuchar *)(p->offsetToPtr(ioIndex))=buff[0];
					setIoIndex(ioIndex+1);
					if(!p->cmapping)
					{
						if(!writeBlock((char *) &buff[1], 1)) return -1;
						else return c;
					}
					val=buff[1];
				}
				else val=buff[0];
			}
			*(FXuchar *)(p->offsetToPtr(ioIndex))=val;
			setIoIndex(ioIndex+1);
			return c;
		}
		if(!writeBlock((char *)&val, 1)) return -1;
		return c;
	}
	return -1;
}

int QMemMap::ungetch(int c)
{
	QMtxHold h(p);
	if(isOpen())
	{
		FXuval size=p->ungetchbuffer.size();
		p->ungetchbuffer.resize(size+1);
		p->ungetchbuffer[(FXuint)size]=(char) c;
		setIoIndex(ioIndex-1);
		return c;
	}
	return -1;
}

} // namespace
