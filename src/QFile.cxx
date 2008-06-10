/********************************************************************************
*                                                                               *
*      F i l e   I n f o r m a t i o n   a n d   M a n i p u l a t i o n        *
*                                                                               *
*********************************************************************************
* Copyright (C) 2000,2005 by Jeroen van der Zijp.   All Rights Reserved.        *
*********************************************************************************
* Contributed by: Sean Hubbell & Niall Douglas                                  *
*********************************************************************************
* This library is free software; you can redistribute it and/or                 *
* modify it under the terms of the GNU Lesser General Public                    *
* License as published by the Free Software Foundation; either                  *
* version 2.1 of the License, or (at your option) any later version.            *
*                                                                               *
* This library is distributed in the hope that it will be useful,               *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU             *
* Lesser General Public License for more details.                               *
*                                                                               *
* You should have received a copy of the GNU Lesser General Public              *
* License along with this library; if not, write to the Free Software           *
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.    *
*********************************************************************************
* $Id: QFile.cpp,v 1.182 2004/11/30 03:16:56 fox Exp $                         *
********************************************************************************/
#include "xincs.h"
#include "qcstring.h"
#include "fxver.h"
#include "fxdefs.h"
#include "FXStream.h"
#include "FXString.h"
#include "QFile.h"
#ifdef WIN32
#include <shellapi.h>
#else
#include <sys/time.h>
#endif
#include "FXException.h"
#include "QThread.h"
#include "QTrans.h"
#include "FXPtrHold.h"
#include "FXRollback.h"
#include "FXACL.h"
#include "FXWinLinks.h"
#include "FXStat.h"
#include "FXDir.h"
#include "FXMemDbg.h"
#if defined(DEBUG) && defined(FXMEMDBG_H)
static const char *_fxmemdbg_current_file_ = __FILE__;
#endif


#ifdef WIN32
#include "WindowsGubbins.h"
// Replacement ftruncate() (as there's no 64 bit version of _chsize())
extern "C" static int ftruncate(int fh, FX::FXfval _newsize)
{
	intptr_t _h=_get_osfhandle(fh);
	if(-1==_h) return -1;
	HANDLE h=(HANDLE) _h;
	LARGE_INTEGER current={0}, newsize;
	current.LowPart=SetFilePointer(h, 0, &current.HighPart, FILE_CURRENT);
	if((DWORD) -1==current.LowPart) goto err;
	newsize.QuadPart=_newsize;
	if((DWORD) -1==SetFilePointer(h, newsize.LowPart, &newsize.HighPart, FILE_BEGIN)) goto err;
	if(0==SetEndOfFile(h)) goto err;
	if((DWORD) -1==SetFilePointer(h, current.LowPart, &current.HighPart, FILE_BEGIN)) goto err;
	return 0;
err:
	errno=EIO;	// Emulate POSIX
	return -1;
}
#endif


/*******************************************************************************/

namespace FX {

class FXDLLLOCAL QFilePrivate : public QMutex
{
public:
	FXString filename;
	bool amStdio;
	int handle;
	FXfval size;
	enum LastOp
	{
		NoOp=0,
		Read=1,
		Write=2
	} lastop;
	QByteArray ungetchbuffer;
	bool doacl;
	FXACL *acl;
	QFilePrivate(bool _amStdio, bool _doacl) : amStdio(_amStdio), handle(0), size(0), lastop(NoOp), doacl(_doacl), acl(0)
	{
	}
	~QFilePrivate()
	{
		FXDELETE(acl);
	}
};
static FXPtrHold<QFile> stdiofile;

int QFile::int_fileDescriptor() const
{
	return p->handle;
}

QFile::QFile() : p(0), QIODevice()
{
	FXERRHM(p=new QFilePrivate(false, true));
}

QFile::QFile(WantStdioType) : p(0), QIODevice()
{	// Special QFile talking to stdin/stdout
	FXERRHM(p=new QFilePrivate(true, true));
	p->handle=fileno(stdout);
	setFlags(IO_ReadWrite|IO_Append|IO_Truncate|IO_Open);
}

QFile::QFile(const FXString &name, WantLightQFile) : p(0), QIODevice()
{	// Special QFile for a "light" instance. This instance is much faster to create
	// and destroy but does not implement ACL's and so is private
	FXRBOp unconstr=FXRBConstruct(this);
	FXERRHM(p=new QFilePrivate(false, false));
	p->filename=name;
	unconstr.dismiss();
}

QFile::QFile(const FXString &name) : p(0), QIODevice()
{
	FXRBOp unconstr=FXRBConstruct(this);
	FXERRHM(p=new QFilePrivate(false, true));
	p->filename=name;
	unconstr.dismiss();
}

QFile::~QFile()
{ FXEXCEPTIONDESTRUCT1 {
	close();
	FXDELETE(p);
} FXEXCEPTIONDESTRUCT2; }

const FXString &QFile::name() const
{
	// QMtxHold h(p);	can do without
	return p->filename;
}

void QFile::setName(const FXString &name)
{
	QMtxHold h(p);
	if(!p->amStdio)
	{
		if(isOpen()) close();
		p->filename=name;
	}
}

bool QFile::exists() const
{
	QMtxHold h(p);
	if(p->amStdio) return true;
	return FXStat::exists(p->filename)!=0;
}

bool QFile::remove()
{
	QMtxHold h(p);
	if(p->amStdio) return false;
	close();
	return FXDir::remove(p->filename)!=0;
}

FXfval QFile::reloadSize()
{
	QMtxHold h(p);
	if(isOpen())
	{
#ifdef WIN32
		DWORD high;
		return (p->size=(GetFileSize((HANDLE) _get_osfhandle(p->handle), &high)|(((FXfval) high)<<32)));
#else
		// I can't see any alternative to a full stat
		struct ::stat s={0};
		::fstat(p->handle, &s);
		return (p->size=(FXfval) s.st_size);
#endif
	}
	return 0;
}

QIODevice &QFile::stdio(bool applyCRLFTranslation)
{
	if(!stdiofile)
	{
		FXERRHM(stdiofile=new QFile(WantStdioType()));
	}
	if(applyCRLFTranslation)
		stdiofile->setMode(stdiofile->mode()|IO_Translate);
	else
		stdiofile->setMode(stdiofile->mode() & ~IO_Translate);
	return *stdiofile;
}

bool QFile::open(FXuint mode)
{
	QMtxHold h(p);
	if(isOpen())
	{	// I keep fouling myself up here, so assertion check
		if(QIODevice::mode()!=mode) FXERRGIO(QTrans::tr("QFile", "Device reopen has different mode"));
	}
	else
	{
		QThread_DTHold dth;
		int access=0;
		if(IO_ReadWrite==(mode & IO_ReadWrite)) access|=O_RDWR|O_CREAT;
		else
		{
			if(mode & IO_ReadOnly)  access|=O_RDONLY;
			if(mode & IO_WriteOnly) access|=O_WRONLY|O_CREAT|O_TRUNC;
		}
		if(mode & IO_Append) access|=O_APPEND;
		if(mode & IO_Truncate) access|=O_TRUNC;
		if(!(access & O_CREAT) && !exists()) FXERRGNF(QTrans::tr("QFile", "File '%1' not found").arg(p->filename), 0);
		if(p->doacl)
		{
			if(!p->acl)
			{	// If we're not writing, the default ACL of want everything
				// demands too much causing a fault on default NTFS perms
				FXERRHM(p->acl=new FXACL(FXACL::default_(FXACL::File, !(mode & IO_WriteOnly))));
			}
		}
#ifdef WIN32
		access|=O_BINARY;
		// Annoyingly, you must create a file handle with WRITE_DAC permission to
		// be able to write the ACL so a bit of a workaround here
		HANDLE h;
		SECURITY_ATTRIBUTES sa={ sizeof(SECURITY_ATTRIBUTES) };
		SECURITY_ATTRIBUTES *psa=&sa;
		DWORD accessw=0, creation=0;
		if((O_RDONLY & access)==O_RDONLY) accessw|=STANDARD_RIGHTS_READ|GENERIC_READ;
		if((O_WRONLY & access)==O_WRONLY) accessw|=STANDARD_RIGHTS_ALL|GENERIC_WRITE;
		if((O_RDWR & access)==O_RDWR) accessw|=STANDARD_RIGHTS_ALL|GENERIC_READ|GENERIC_WRITE;

		if(p->acl)
			sa.lpSecurityDescriptor=(SECURITY_DESCRIPTOR *) p->acl->int_toWin32SecurityDescriptor();
		else
			psa=0;
		switch((O_CREAT|O_TRUNC) & access)
		{
		case O_CREAT:
            creation|=OPEN_ALWAYS; break;
		case O_TRUNC:
			creation|=TRUNCATE_EXISTING; break;
		case O_CREAT|O_TRUNC:
			creation|=CREATE_ALWAYS; break;
		default:
			creation|=OPEN_EXISTING; break;
		}
		FXERRHWINFN(INVALID_HANDLE_VALUE!=(h=CreateFile(FXUnicodify<>(p->filename, true).buffer(), accessw, FILE_SHARE_DELETE|FILE_SHARE_READ|FILE_SHARE_WRITE, psa,
			creation, FILE_ATTRIBUTE_NORMAL, NULL)), p->filename);
		FXERRHIO(p->handle=_open_osfhandle((intptr_t) h, access));
#endif
#ifdef USE_POSIX
		FXERRHOSFN(p->handle=::open(p->filename.text(), access, S_IREAD|S_IWRITE), p->filename);
		FXERRHOS(::fcntl(p->handle, F_SETFD, ::fcntl(p->handle, F_GETFD, 0)|FD_CLOEXEC));
#endif
		if(access & O_CREAT)
		{	// Set the perms
			if(p->acl) p->acl->writeTo(p->handle);
		}
		if(p->acl) *p->acl=FXACL(p->handle, FXACL::File);
		setFlags((mode & IO_ModeMask)|IO_Open);
		p->lastop=QFilePrivate::NoOp;
		reloadSize();
		ioIndex=0;
		if(!(mode & IO_NoAutoUTF) && isReadable() && isTranslated())
		{	// Have a quick peek to see what kind of text it is
			FXuchar buffer[1024];
			FXuval read;
			if((read=QFile::readBlock((char *) buffer, sizeof(buffer))))
				setUnicodeTranslation(determineUnicodeType(buffer, read));
		}
		QFile::at((mode & IO_Append) ? p->size : 0);
	}
	return true;
}

void QFile::close()
{
	QMtxHold h(p);
	if(isOpen() && !p->amStdio)
	{
		QThread_DTHold dth;
		FXERRHIO(::close(p->handle));
		p->handle=0;
		p->size=0;
		if(p->acl)
		{	// Reset to default ACL
			FXDELETE(p->acl);
		}
		ioIndex=0;
		setFlags(0);
	}
}

void QFile::flush()
{
	QMtxHold h(p);
	if(isOpen() && isWriteable())
	{
		QThread_DTHold dth;
#ifdef WIN32
		FXERRHWIN(FlushFileBuffers((HANDLE) _get_osfhandle(p->handle)));
		//FXERRHIO(::_commit(p->handle));
#endif
#ifdef USE_POSIX
		FXERRHIO(::fsync(p->handle));
#endif
		p->lastop=QFilePrivate::NoOp;
	}
}

FXfval QFile::size() const
{
	// QMtxHold h(p); can do without
	if(isOpen() && !p->amStdio)
		return p->size;
	else
		return 0;
}

void QFile::truncate(FXfval size)
{
	QMtxHold h(p);
	if(!isWriteable()) FXERRGIO(QTrans::tr("QFile", "Not open for writing"));
	if(isOpen() && !p->amStdio)
	{
		QThread_DTHold dth;
		if((mode() & IO_ShredTruncate) && size<p->size)
			shredData(size);
		if(ioIndex>size) at(size);
		FXERRHIO(::ftruncate(p->handle, size));
		p->size=size;
	}
}

FXfval QFile::at() const
{
	QMtxHold h(p);
	return p->amStdio ? 0 : ioIndex;
}

bool QFile::at(FXfval newpos)
{
	QMtxHold h(p);
	if(isOpen() && ioIndex!=newpos && !p->amStdio)
	{
		QThread_DTHold dth;
#ifdef WIN32
		LARGE_INTEGER _newpos; _newpos.QuadPart=newpos;
		FXERRHWIN(SetFilePointerEx((HANDLE) _get_osfhandle(p->handle), _newpos, NULL, FILE_BEGIN));
#else
		FXERRHIO(::lseek(p->handle, newpos, SEEK_SET));
#endif
		ioIndex=newpos;
		if(ioIndex>p->size) p->size=ioIndex;
		p->ungetchbuffer.resize(0);
		return true;
	}
	return false;
}

bool QFile::atEnd() const
{
	QMtxHold h(p);
	if(!isOpen()) return true;
	// Unfortunately GNU/Linux doesn't have eof() and MSVC6's _eof() doesn't like file pointers > 4Gb!
	// You also need to stay compatible with when the input is stdin
	QFile *me=(QFile *) this;
	int c=me->getch();
	if(-1==c) return true;
	if(p->amStdio)
		me->ungetch(c);
	else
		me->at(ioIndex-1);
	return false;
}

const FXACL &QFile::permissions() const
{
	if(p->acl)
		return *p->acl;
	static FXACL def(FXACL::default_(FXACL::File, false));
	return def;
}
void QFile::setPermissions(const FXACL &perms)
{
	if(!p->acl) FXERRHM(p->acl=new FXACL);
	if(isOpen()) perms.writeTo(p->handle);
	*p->acl=perms;
}
FXACL QFile::permissions(const FXString &path)
{
	return FXACL(path, FXStat::isFile(path) ? FXACL::File : FXACL::Directory);
}
void QFile::setPermissions(const FXString &path, const FXACL &perms)
{
	perms.writeTo(path);
}

FXuval QFile::readBlock(char *data, FXuval maxlen)
{
	QMtxHold h(p);
	if(!QIODevice::isReadable()) FXERRGIO(QTrans::tr("QFile", "Not open for reading"));
	if(isOpen() && maxlen)
	{
		QThread_DTHold dth;
		FXuval readed=0;
#ifdef USE_POSIX
		if(QFilePrivate::Write==p->lastop && !p->amStdio)
		{
			FXERRHIO(::lseek(p->handle, 0, SEEK_CUR));
		}
#endif
		if(!p->ungetchbuffer.isEmpty())
		{
			FXuchar *ungetchdata=p->ungetchbuffer.data();
			FXuval ungetchlen=p->ungetchbuffer.size();
			FXuval copylen=FXMIN(ungetchlen, maxlen);
			memcpy(data, ungetchdata, copylen);
			readed+=copylen; ioIndex+=copylen;
			if(copylen<ungetchlen) memmove(ungetchdata, ungetchdata+copylen, ungetchlen-copylen);
			p->ungetchbuffer.resize((FXuint)(ungetchlen-copylen));
		}
		int ioreaded;
#ifdef WIN32
		FXERRHWIN(ReadFile((HANDLE) _get_osfhandle(p->handle), data+readed, (DWORD)(maxlen-readed), (LPDWORD) &ioreaded, NULL));
#else
		FXERRHIO(ioreaded=::read((p->amStdio) ? fileno(stdin) : p->handle, data+readed, maxlen-readed));
#endif
		ioIndex+=ioreaded; readed+=ioreaded;
		p->lastop=QFilePrivate::Read;
		if(isTranslated())
		{
			QByteArray temp(maxlen);
			FXuval inputlen=readed;
			FXuval output=removeCRLF(temp.data(), (FXuchar *) data, maxlen, inputlen, unicodeTranslation());
			// Adjust the file pointer to reprocess unprocessed input later
			QFile::at(ioIndex-(readed-inputlen));
			memcpy(data, temp.data(), output);
			readed=output;
		}
		return readed;
	}
	return 0;
}

FXuval QFile::writeBlock(const char *data, FXuval maxlen)
{
	QMtxHold h(p);
	if(!isWriteable()) FXERRGIO(QTrans::tr("QFile", "Not open for writing"));
	if(isOpen())
	{
		QThread_DTHold dth;
		FXuval written=0;
#ifdef USE_POSIX
		if(QFilePrivate::Read==p->lastop && !p->amStdio)
		{
			FXERRHIO(::lseek(p->handle, 0, SEEK_CUR));
			p->ungetchbuffer.resize(0);
		}
#endif
		if(isTranslated())
		{	// GNU/Linux open() doesn't support text translation
			FXuchar buffer[16384];
			FXuval writeidx;
			for(writeidx=0; writeidx<maxlen;)
			{
				FXuval inputlen=maxlen-writeidx;
#ifdef WIN32
				DWORD __written;
				FXERRHWIN(WriteFile((HANDLE) _get_osfhandle(p->handle), buffer, (DWORD) applyCRLF(buffer, (FXuchar *) &data[writeidx], sizeof(buffer), inputlen, crlfFormat(), unicodeTranslation()), (LPDWORD) &__written, NULL));
				written=__written;
#else
				FXERRHIO(written=::write(p->handle, buffer, applyCRLF(buffer, (FXuchar *) &data[writeidx], sizeof(buffer), inputlen, crlfFormat(), unicodeTranslation())));
#endif
				writeidx+=inputlen;
				ioIndex+=written;
			}
			written=writeidx;
		}
		else
		{
#ifdef WIN32
			DWORD __written;
			FXERRHWIN(WriteFile((HANDLE) _get_osfhandle(p->handle), data, (DWORD) maxlen, (LPDWORD) &__written, NULL));
			written=__written;
#else
			FXERRHIO(written=::write(p->handle, data, maxlen));
#endif
			ioIndex+=written;
		}
		if(ioIndex>p->size) p->size=ioIndex;
		p->lastop=QFilePrivate::Write;
		if(isRaw()) flush();
		return written;
	}
	return 0;
}

FXuval QFile::readBlockFrom(char *data, FXuval maxlen, FXfval pos)
{
	QMtxHold h(p);
	at(pos);
	return readBlock(data, maxlen);
}
FXuval QFile::writeBlockTo(FXfval pos, const char *data, FXuval maxlen)
{
	QMtxHold h(p);
	at(pos);
	return writeBlock(data, maxlen);
}

int QFile::ungetch(int c)
{
	QMtxHold h(p);
	if(isOpen())
	{
		uint size=p->ungetchbuffer.size();
		p->ungetchbuffer.resize(size+1);
		p->ungetchbuffer[size]=(char) c;
		ioIndex-=1;
		return c;
	}
	return -1;
}

}

