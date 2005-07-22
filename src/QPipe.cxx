/********************************************************************************
*                                                                               *
*                            Named pipe i/o device                              *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2002,2003 by Niall Douglas.   All Rights Reserved.       *
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
#include "QPipe.h"
#include "FXString.h"
#include "QThread.h"
#include "FXException.h"
#include "QTrans.h"
#include "FXProcess.h"
#include "FXFile.h"
#include "FXRollback.h"
#include "FXACL.h"
#include <stdlib.h>
#include <assert.h>
#include "sigpipehandler.h"

#ifndef USE_POSIX
#define USE_WINAPI
#include "WindowsGubbins.h"

// Define this to the atomic pipe operation maximum. Make it 4096 bytes for compatibility
// with GNU/Linux.
#define WINMAXATOMICLEN 4096
#define WINDEEPPIPELEN  65536

#endif
#ifdef USE_POSIX
#include <sys/poll.h>
#endif

#include "FXMemDbg.h"
#if defined(DEBUG) && !defined(FXMEMDBG_DISABLE)
static const char *_fxmemdbg_current_file_ = __FILE__;
#endif

namespace FX {

struct FXDLLLOCAL QPipePrivate : public QMutex
{
	FXString pipename;
	FXACL acl;
	FXint bufferLength;
#ifdef USE_WINAPI
	bool connected;
	int inprogress;			// =0 not in progress, =1 connecting =2 reading =3 just opened
	HANDLE readh, writeh;
	OVERLAPPED ol;
	QPipePrivate(bool deepPipe) : acl(FXACL::Pipe), bufferLength(deepPipe ? WINDEEPPIPELEN : WINMAXATOMICLEN), readh(0), writeh(0), QMutex() { memset(&ol, 0, sizeof(ol)); }
#endif
#ifdef USE_POSIX
	int readh, writeh;
	QPipePrivate(bool deepPipe) : acl(FXACL::Pipe),
#ifdef __FreeBSD__
		// PIPE_BUF lies on FreeBSD :(
		bufferLength(16384),	// =PIPE_SIZE from sys/pipe.h, could even go to 64Kb (BIG_PIPE_SIZE)
#else
		bufferLength(PIPE_BUF),
#endif
		readh(0), writeh(0), QMutex() { }
#endif
	void makePerms()
	{
		acl.append(FXACL::Entry(FXACLEntity::everything(), 0, FXACL::Permissions().setAll()));
	}
};

void *QPipe::int_getOSHandle() const
{
#ifdef USE_WINAPI
	return (void *) p->ol.hEvent;
#endif
#ifdef USE_POSIX
	return (void *) p->readh;
#endif
}

void QPipe::int_hack_makeWriteNonblocking() const
{
#ifdef USE_POSIX
	if(p->writeh)
	{
		::fcntl(p->writeh, F_SETFL, O_NONBLOCK);
	}
#endif
}

QPipe::QPipe() : p(0), creator(false), QIODeviceS()
{
	FXRBOp unconstr=FXRBConstruct(this);
	FXERRHM(p=new QPipePrivate(false));
	p->makePerms();
	unconstr.dismiss();
}

QPipe::QPipe(const FXString &name, bool isDeepPipe) : p(0), creator(false), QIODeviceS()
{
	FXRBOp unconstr=FXRBConstruct(this);
	FXERRHM(p=new QPipePrivate(isDeepPipe));
	p->makePerms();
	if(name.empty())
		setUnique(true);
	else
		setName(name);
	unconstr.dismiss();
}

QPipe::~QPipe()
{ FXEXCEPTIONDESTRUCT1 {
	close();
	FXDELETE(p);
} FXEXCEPTIONDESTRUCT2; }

const FXString &QPipe::name() const
{
	return p->pipename;
}

void QPipe::setName(const FXString &name)
{
	close();
	p->pipename=name;
	setUnique(false);
}


static inline FXString makeFullPath(const FXString &src)
{
#ifdef USE_WINAPI
	return "\\\\.\\pipe\\TnFOX\\"+src;
#endif
#ifdef USE_POSIX
	return "/tmp/TnFOX_"+src;
#endif
}

/* ned 23rd October 2002
Had some really really weird behaviour - it seems if one thread fills a duplex pipe's write buffer
and then another thread tries writing more when the reading thread has not yet read any data,
the second write hangs and so do any other subsequent writes. Best of all, the read doesn't return either!

I found mention of this bug in google groups for NT *3.5* and their solution was to separate a
duplex pipe into two seperate ones, each dedicated to one direction. And yes, that works fine.

It appears duplex pipes are broken on Windows NT all versions :(
*/
bool QPipe::create(FXuint mode)
{
	FXMtxHold h(p);
	close();
	FXString fullname;
	for(;;)
	{
		if(anonymous && p->pipename.empty())
		{
			p->pipename=FXString("%1_%2").arg(FXProcess::id()).arg(rand(),0,16);
		}
		fullname=makeFullPath(p->pipename);
#ifdef USE_WINAPI
		HANDLE ret;
		SECURITY_ATTRIBUTES sa={ sizeof(SECURITY_ATTRIBUTES) };
		sa.lpSecurityDescriptor=p->acl.int_toWin32SecurityDescriptor();
		if(mode & IO_WriteOnly)
		{
			FXString writename(fullname+'w');
			/* BTW the +24 on the buffer length is an undocumented "feature" of NT pipes
			where the actual buffer size used is -24 because the internal header size
			isn't included! */
			ret=(p->writeh=CreateNamedPipe(writename.text(),
				PIPE_ACCESS_OUTBOUND|WRITE_DAC|WRITE_OWNER,
				PIPE_TYPE_BYTE, 1, p->bufferLength+24, 0, 
				INFINITE, &sa));
			if(INVALID_HANDLE_VALUE==ret)
			{
				if(GetLastError()==ERROR_ALREADY_EXISTS && anonymous)
					continue;
				FXERRHWINFN(0, writename);
			}
		}
		if(mode & IO_ReadOnly)
		{
			FXString readname(fullname+'r');
			ret=(p->readh=CreateNamedPipe(readname.text(),
				PIPE_ACCESS_INBOUND|WRITE_DAC|WRITE_OWNER|FILE_FLAG_OVERLAPPED,
				PIPE_TYPE_BYTE, 1, 0, p->bufferLength+24, 
				INFINITE, &sa));
			if(INVALID_HANDLE_VALUE==ret)
			{
				if(GetLastError()==ERROR_ALREADY_EXISTS && anonymous)
					continue;
				FXERRHWINFN(0, readname);
			}
		}
		p->connected=false;
		p->inprogress=0;
		FXERRHWIN(INVALID_HANDLE_VALUE!=(p->ol.hEvent=CreateEvent(NULL, TRUE, FALSE, NULL)));
#endif
#ifdef USE_POSIX
		if(mode & IO_ReadOnly)
		{
			FXString readname(fullname+'r');
#ifdef HAVE_WIDEUNISTD
			if(-1==mkfifo(readname.utext(), S_IREAD|S_IWRITE)) { if(EEXIST==errno) { if(anonymous) continue; } else FXERRHOSFN(-1, readname); }
#else
			if(-1==mkfifo(readname.text(), S_IREAD|S_IWRITE)) { if(EEXIST==errno) { if(anonymous) continue; } else FXERRHOSFN(-1, readname); }
#endif
			p->acl.writeTo(readname);
#ifdef HAVE_WIDEUNISTD
			FXERRHOSFN(p->readh=::wopen(readname.utext(), O_RDONLY|O_NONBLOCK, 0), readname);
#else
			FXERRHOSFN(p->readh=::open(readname.text(), O_RDONLY|O_NONBLOCK, 0), readname);
#endif
		}
		if(mode & IO_WriteOnly)
		{
			FXString writename(fullname+'w');
#ifdef HAVE_WIDEUNISTD
			if(-1==mkfifo(writename.utext(), S_IREAD|S_IWRITE)) { if(EEXIST==errno) { if(anonymous) continue; } else FXERRHOSFN(-1, writename); }
#else
			if(-1==mkfifo(writename.text(), S_IREAD|S_IWRITE)) { if(EEXIST==errno) { if(anonymous) continue; } else FXERRHOSFN(-1, writename); }
#endif
			p->acl.writeTo(writename);
		}
#endif
		break;
	}
	setFlags((mode & IO_ModeMask)|IO_Open);
	creator=true;
	return true;
}

bool QPipe::open(FXuint mode)
{
	FXMtxHold h(p);
	if(isOpen())
	{	// I keep fouling myself up here, so assertion check
		if(QIODevice::mode()!=mode) FXERRGIO(QTrans::tr("QPipe", "Device reopen has different mode"));
	}
	else
	{
		FXString fullname=makeFullPath(p->pipename);
		bool doneACL=false;
#ifdef USE_WINAPI
		if(mode & IO_ReadOnly)
		{
			FXString readname(fullname+'w');
			h.unlock();
			FXERRHWINFN(WaitNamedPipe(readname.text(), NMPWAIT_USE_DEFAULT_WAIT), readname);
			h.relock();
			FXERRHWINFN(INVALID_HANDLE_VALUE!=(p->readh=CreateFile(readname.text(),
				GENERIC_READ,
				FILE_SHARE_DELETE|FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OVERLAPPED, NULL)), readname);
			p->acl=FXACL(p->readh, FXACL::Pipe); doneACL=true;
		}
		if(mode & IO_WriteOnly)
		{
			FXString writename(fullname+'r');
			h.unlock();
			FXERRHWINFN(WaitNamedPipe(writename.text(), NMPWAIT_USE_DEFAULT_WAIT), writename);
			h.relock();
			FXERRHWINFN(INVALID_HANDLE_VALUE!=(p->writeh=CreateFile(writename.text(),
				GENERIC_WRITE,
				FILE_SHARE_DELETE|FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)), writename);
			if(!doneACL) p->acl=FXACL(p->writeh, FXACL::Pipe);
		}
		p->connected=true;
		p->inprogress=0;
		FXERRHWIN(INVALID_HANDLE_VALUE!=(p->ol.hEvent=CreateEvent(NULL, TRUE, FALSE, NULL)));
		if(mode & IO_ReadOnly)
		{	// Monitor inbound (start a read reading no data) so event object is correct
			DWORD ret=ReadFile(p->readh, 0, 0, 0, &p->ol);
			assert(ret==0 && GetLastError()==ERROR_IO_PENDING);
			p->inprogress=3;
		}
#endif
#ifdef USE_POSIX
		if(mode & IO_ReadOnly)
		{
			FXString readname(fullname+'w');
			if(!FXFile::exists(readname)) FXERRGNF(QTrans::tr("QPipe", "Pipe not found"), 0);
			p->acl=FXACL(readname, FXACL::Pipe); doneACL=true;
#ifdef HAVE_WIDEUNISTD
			FXERRHOSFN(p->readh=::wopen(readname.utext(), O_RDONLY|O_NONBLOCK, 0), readname);
#else
			FXERRHOSFN(p->readh=::open(readname.text(), O_RDONLY|O_NONBLOCK, 0), readname);
#endif
		}
		if(mode & IO_WriteOnly)
		{
			FXString writename(fullname+'r');
			if(!FXFile::exists(writename)) FXERRGNF(QTrans::tr("QPipe", "Pipe not found"), 0);
			if(!doneACL) p->acl=FXACL(writename, FXACL::Pipe);
		}
#endif
		setFlags((mode & IO_ModeMask)|IO_Open);
		creator=false;
	}
	return true;
}

/* ned 8th November 2002: Took two days nailing this bug. Essentially, if you perform
an overlapped i/o read (which you do if you created, not opened the pipe) which returns
on timeout and then you close it, Windows rather stupidly doesn't cancel pending i/o
operations for that thread and hence ReadFile ends up writing over the heap which
on heap validation throws a CRT exception. Even if you CancelIo() the handle, it
doesn't help because a thread can only cancel what it itself started.

Thanks to Microsoft for a completely wank system! :(
*/
void QPipe::close()
{
	if(p)
	{
		FXMtxHold h(p);
		QThread_DTHold dth;
#ifdef USE_WINAPI
		if(p->connected)
		{
			if(p->writeh && !FlushFileBuffers(p->writeh))
			{
				if(ERROR_PIPE_NOT_CONNECTED!=GetLastError())
				{
					FXERRHWIN(0);
				}
			}
			if(p->readh)
			{
				if(p->inprogress) FXERRHWIN(CancelIo(p->readh));
			}
			if(creator)
			{
				if(p->readh) FXERRHWIN(DisconnectNamedPipe(p->readh));
				if(p->writeh && p->readh!=p->writeh) FXERRHWIN(DisconnectNamedPipe(p->writeh));
			}
			p->connected=false;
		}
		if(p->readh)
		{
			FXERRHWIN(CloseHandle(p->readh));
			p->readh=0;
		}
		if(p->writeh && p->readh!=p->writeh)
		{
			FXERRHWIN(CloseHandle(p->writeh));
			p->writeh=0;
		}
		if(p->ol.hEvent)
		{
			FXERRHWIN(CloseHandle(p->ol.hEvent));
			p->ol.hEvent=0;
		}
#endif
#ifdef USE_POSIX
		if(p->readh)
		{
			FXERRHIO(::close(p->readh));
			p->readh=0;
		}
		if(p->writeh)
		{
			FXERRHIO(::close(p->writeh));
			p->writeh=0;
		}
		if(creator && !(flags() & IO_DontUnlink))
		{
			FXString fullname=makeFullPath(p->pipename);
#ifdef HAVE_WIDEUNISTD
			if(flags() & IO_ReadOnly) ::wunlink(fullname.utext()+'r');
			if(flags() & IO_WriteOnly) ::wunlink(fullname.utext()+'w');
#else
			if(flags() & IO_ReadOnly) ::unlink(fullname.text()+'r');
			if(flags() & IO_WriteOnly) ::unlink(fullname.text()+'w');
#endif
		}
#endif
		p->acl=FXACL(FXACL::Pipe);
		p->makePerms();
		setFlags(0);
	}
}

void QPipe::flush()
{
	FXMtxHold h(p);
	if(isOpen() && isWriteable())
	{
		QThread_DTHold dth;
#ifdef USE_WINAPI
		if(p->connected)
			FXERRHWIN(FlushFileBuffers(p->writeh));
#endif
#ifdef USE_POSIX
		// This is supposed to work, but Linux keeps giving an error :(
		//FXERRHIO(::fsync(p->writeh));
		::fsync(p->writeh);
#endif
	}
}

bool QPipe::reset()
{
	FXuint flgs=flags();
	close();
#ifdef USE_WINAPI
	/* Ok, one problem with Windows is that the name persists until the last
	thing has closed it. This causes create() to fail */
	if(creator)
	{
		FXString fullname=makeFullPath(p->pipename);
		FXString readname(fullname+'r'), writename(fullname+'w');
		for(;;)
		{
			BOOL ret;
			DWORD readcode, writecode;
			ret=WaitNamedPipe(readname.text(), 100);
			assert(ret==0);
			readcode=GetLastError();
			ret=WaitNamedPipe(writename.text(), 100);
			assert(ret==0);
			writecode=GetLastError();
			if(ERROR_FILE_NOT_FOUND==readcode && ERROR_FILE_NOT_FOUND==writecode) break;
			Sleep(1);
		}
	}
#endif
	return (creator) ? create(flgs) : open(flgs);
}

FXfval QPipe::size() const
{
	FXMtxHold h(p);
	FXfval waiting=0;
	if(!isReadable()) return 0;
#ifdef USE_WINAPI
	if(!p->connected) return 0;
	DWORD dwaiting=0;
	FXERRHWIN(PeekNamedPipe(p->readh, NULL, 0, NULL, &dwaiting, NULL));
	waiting=(FXfval) dwaiting;
#endif
#ifdef USE_POSIX
	QThread_DTHold dth;
	struct pollfd pfd={0};
	pfd.fd=p->readh;
	pfd.events=POLLIN;
	FXERRHIO(::poll(&pfd, 1, 0));
	if(pfd.revents & POLLIN) waiting=(FXfval) -1; // No idea how much, but there is some
#endif
	return waiting;
}

void QPipe::truncate(FXfval) { }
FXfval QPipe::at() const { return 0; }
bool QPipe::at(FXfval) { return false; }
bool QPipe::atEnd() const { return size()==0; }

const FXACL &QPipe::permissions() const
{
	return p->acl;
}
void QPipe::setPermissions(const FXACL &perms)
{
	if(isOpen()) { perms.writeTo(p->readh); perms.writeTo(p->writeh); }
	p->acl=perms;
}
FXACL QPipe::permissions(const FXString &name)
{
	return FXACL(makeFullPath(name), FXACL::Pipe);
}
void QPipe::setPermissions(const FXString &name, const FXACL &perms)
{
	perms.writeTo(makeFullPath(name));
}

FXuval QPipe::readBlock(char *data, FXuval maxlen)
{
	FXMtxHold h(p);
	if(!QIODevice::isReadable()) FXERRGIO(QTrans::tr("QPipe", "Not open for reading"));
	if(isOpen())
	{
		FXuval readed=0;
#ifdef USE_WINAPI
		// BTW Yes, it does require all this code to make it work properly :(
		bool errhandle=false;
		BOOL ret;
		DWORD bread=0;
		if(!p->connected && creator)
		{	// Wait till someone connects to me OR pipe becomes available
			if(!p->inprogress)
			{
				if(!ConnectNamedPipe(p->readh, &p->ol))
				{
					DWORD errcode=GetLastError();
					if(ERROR_PIPE_CONNECTED==errcode)
						p->connected=true;
					else if(ERROR_IO_PENDING==errcode)
						p->inprogress=1;
					else
						errhandle=true;
				}
				else
				{
					p->connected=true;
				}
			}
		}
		if(p->connected)
		{
			if(!p->inprogress)
			{
doread:
				//u32 before=Support_GetMsCount();
				if(!(ret=ReadFile(p->readh, data, (DWORD) maxlen, &bread, &p->ol)))
				{
					//QString temp("Thread 0x%1 ReadFile returned after %2 ms with i/o in progress=%3\n");
					//tDebug(temp.arg(Support_GetThreadId(),0, 16).arg(Support_GetMsCount()-before).arg(ERROR_IO_PENDING==GetLastError()));
					if(ERROR_IO_PENDING==GetLastError())
						p->inprogress=2;
					else
						errhandle=true;
				}
				/*else 
				{
					QString t="Thread 0x%1 Read immediately %2 bytes from %3 after %4 ms\n";
					t=t.arg(Support_GetThreadId(),0, 16).arg(bread).arg(QString((QChar *) fi->path, wcslen(fi->path))).arg(Support_GetMsCount()-before);
					//if(bread) t+=dump8((char *) buffer, bread);
					tDebug(t);
				}*/
			}
		}
		if(p->inprogress)
		{
			//u32 before=Support_GetMsCount();
			h.unlock();
			HANDLE hs[2]; hs[0]=p->ol.hEvent; hs[1]=QThread::int_cancelWaiterHandle();
			ret=WaitForMultipleObjects(2, hs, FALSE, INFINITE);
			h.relock();
			//QString temp("Thread 0x%1 WaitForSingleObject returned after %2 ms with data=%3\n");
			//tDebug(temp.arg(Support_GetThreadId(),0, 16).arg(Support_GetMsCount()-before).arg(WAIT_TIMEOUT!=ret));
			if(WAIT_OBJECT_0+1==ret)
			{
				FXERRHWIN(CancelIo(p->readh));
				h.unlock();
				QThread::current()->checkForTerminate();
			}
			else if(WAIT_OBJECT_0!=ret) errhandle=true;
			else if(!(ret=GetOverlappedResult(p->readh, &p->ol, &bread, FALSE)))
				errhandle=true;
			/*else 
			{
				QString t="Thread 0x%1 Read overlapped %2 bytes from %3 at %4\n";
				t=t.arg(Support_GetThreadId(),0, 16).arg(bread).arg(QString((QChar *) fi->path, wcslen(fi->path))).arg(Support_GetMsCount());
				//if(bread) t+=dump8((char *) buffer, bread);
				tDebug(t);
			}*/
			if(3==p->inprogress)
			{	// Nothing will have been read
				p->inprogress=0;
				goto doread;
			}
			p->inprogress=0;
		}
		if(errhandle)
		{
			DWORD errcode=GetLastError();
			assert(errcode!=ERROR_IO_PENDING);
			if(ERROR_NO_DATA==errcode || ERROR_BROKEN_PIPE==errcode || ERROR_PIPE_NOT_CONNECTED==errcode
				|| ERROR_OPERATION_ABORTED==errcode)
			{	// Client disconnected suddenly
				if(creator)
				{
					ret=DisconnectNamedPipe(p->readh);
					if(!ret)
					{
						if(ERROR_PIPE_NOT_CONNECTED!=GetLastError()
							&& ERROR_INVALID_FUNCTION!=GetLastError())
						{
							FXERRHWIN(0);
						}
					}
				}
				p->connected=false;
			}
			FXERRGWIN(errcode, 0);
		}
		readed=(FXuval) bread;
#endif
#ifdef USE_POSIX
		fd_set rfds, efds;
		FD_ZERO(&rfds);
		FD_ZERO(&efds);
		FD_SET(p->readh, &rfds);
		FD_SET(p->readh, &efds);
		h.unlock();
		FXERRHIO(::select(p->readh+1, &rfds, 0, &efds, NULL));
		if(FD_ISSET(p->readh, &efds))	// error occurred (eg; widowed pipe)
			return 0;
		assert(FD_ISSET(p->readh, &rfds));
		readed=::read(p->readh, data, maxlen);
		h.relock();
		FXERRHIO(readed);
#endif
		return readed;
	}
	return 0;
}

FXuval QPipe::writeBlock(const char *data, FXuval maxlen)
{
	FXMtxHold h(p);
	if(!isWriteable()) FXERRGIO(QTrans::tr("QPipe", "Not open for writing"));
	if(isOpen())
	{
		FXuval written;
#ifdef USE_WINAPI
		DWORD bwritten=0;
		h.unlock();
		BOOL ret=WriteFile(p->writeh, data, (DWORD) maxlen, &bwritten, NULL);
		DWORD getlasterror=GetLastError();
		QThread::current()->checkForTerminate();
		h.relock();
		FXERRHWIN2(ret, getlasterror);
		written=(FXuval) bwritten;
#endif
#ifdef USE_POSIX
		if(!p->writeh)
		{
			FXString writename(makeFullPath(p->pipename)+((creator) ? 'w' : 'r'));
			h.unlock();
#ifdef HAVE_WIDEUNISTD
			p->writeh=::wopen(writename.utext(), O_WRONLY, 0);
#else
			p->writeh=::open(writename.text(), O_WRONLY, 0);
#endif
			h.relock();
			FXERRHOSFN(p->writeh, writename);
		}
		QIODeviceS_SignalHandler::lockWrite();
		h.unlock();
		written=::write(p->writeh, data, maxlen);
		h.relock();
		FXERRHIO(written);
		if(QIODeviceS_SignalHandler::unlockWrite())		// Nasty this
			FXERRGCONLOST("Broken pipe", 0);
#endif
		if(isRaw()) flush();
		return written;
	}
	return 0;
}

int QPipe::ungetch(int c)
{
	return -1;
}

FXuval QPipe::maxAtomicLength()
{
	return p->bufferLength;
}

}
