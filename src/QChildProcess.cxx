/********************************************************************************
*                                                                               *
*                           Child Process i/o device                            *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2007 by Niall Douglas.   All Rights Reserved.            *
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
#include "QChildProcess.h"
#include "QPipe.h"
#include "QBuffer.h"
#include "QThread.h"
#include "FXException.h"
#include "QTrans.h"
#include "FXProcess.h"
#include "FXRollback.h"
#include <qcstring.h>
#include <qvaluelist.h>
#include <assert.h>

#ifndef USE_POSIX
#define USE_WINAPI
#include "WindowsGubbins.h"
#endif

#include "FXMemDbg.h"
#if defined(DEBUG) && !defined(FXMEMDBG_DISABLE)
static const char *_fxmemdbg_current_file_ = __FILE__;
#endif

namespace FX {

struct QChildProcessPrivate : public QMutex
{
	FXString command, arguments;
	QChildProcess::ReadChannel readChannel;
	FXuchar connectionBroken : 2;
	QBuffer outlog, errlog;
	QPipe childinout[2], childerr[2];		// [0] is our end (client), [1] child end (server)
	FXlong retcode;
	bool detached;
#ifdef USE_WINAPI
	HANDLE childinouth[2], childerrh[2];	// child end handles
	HANDLE childh;
	HANDLE alwayssignalled;
#endif
#ifdef USE_POSIX
	int childinouth[2], childerrh[2];		// child end handles
	pid_t childh;
	int alwayssignalled;
#endif

	QChildProcessPrivate() : readChannel(QChildProcess::Combined)
	{
		init();
	}
	QChildProcessPrivate(const FXString &_command, const FXString &_args, QChildProcess::ReadChannel _readChannel)
		: command(_command), arguments(_args), readChannel(_readChannel)
	{
		init();
	}
	void init()
	{
		connectionBroken=0;
		retcode=0;
		detached=false;
		memset(childinouth, 0, sizeof(childinouth)*2);
		childh=0;
		alwayssignalled=0;
	}
};

void *QChildProcess::int_getOSHandle() const
{
	if(isOpen())
	{
		if((QChildProcess::StdOut & p->readChannel) && p->outlog.size())
		{	// Return something which is always signalled
			return (void *)(FXuval) p->alwayssignalled;
		}
		if((QChildProcess::StdErr & p->readChannel) && p->errlog.size())
		{	// Return something which is always signalled
			return (void *)(FXuval) p->alwayssignalled;
		}
		if(3==p->readChannel)
			return !p->childerr[0].atEnd() ? p->childerr[0].int_getOSHandle() : p->childinout[0].int_getOSHandle();
		else if(QChildProcess::StdOut & p->readChannel)
			return p->childinout[0].int_getOSHandle();
		else
			return p->childerr[0].int_getOSHandle();
	}
	return 0;
}

void QChildProcess::int_killChildI(bool)
{
	terminate();
}

QChildProcess::QChildProcess() : p(0)
{
	FXERRHM(p=new QChildProcessPrivate);
}

QChildProcess::QChildProcess(const FXString &command, const FXString &args, ReadChannel channel) : p(0)
{
	FXERRHM(p=new QChildProcessPrivate(command, args, channel));
}

QChildProcess::~QChildProcess()
{ FXEXCEPTIONDESTRUCT1 {
	close();
	FXDELETE(p);
} FXEXCEPTIONDESTRUCT2; }

const FXString &QChildProcess::command() const
{
	QMtxHold h(p);
	return p->command;
}

void QChildProcess::setCommand(const FXString &command)
{
	QMtxHold h(p);
	p->command=command;
}

const FXString &QChildProcess::arguments() const
{
	QMtxHold h(p);
	return p->arguments;
}

void QChildProcess::setArguments(const FXString &args)
{
	QMtxHold h(p);
	p->arguments=args;
}

QChildProcess::ReadChannel QChildProcess::readChannel() const throw()
{
	return p->readChannel;
}

void QChildProcess::setReadChannel(ReadChannel channel)
{
	p->readChannel=channel;
}

FXlong QChildProcess::returnCode() const throw()
{
	return p->retcode;
}

void QChildProcess::detach()
{
	if(!p->detached && isOpen())
	{
		FXProcess::FatalExitUpcallSpec kc(this, &QChildProcess::int_killChildI);
		FXProcess::removeFatalExitUpcall(kc);
		p->detached=true;
	}
}

bool QChildProcess::waitForExit(FXuint waitfor)
{
	QMtxHold h(p);
#ifdef USE_WINAPI
	if(p->childh)
	{
		h.unlock();
		HANDLE hs[2]; hs[0]=p->childh; hs[1]=QThread::int_cancelWaiterHandle();
		DWORD ret=WaitForMultipleObjects(2, hs, FALSE, FXINFINITE==waitfor ? INFINITE : waitfor);
		if(WAIT_TIMEOUT==ret)
		{
			return false;
		}
		else if(WAIT_OBJECT_0+1==ret)
		{
			QThread::current()->checkForTerminate();
		}
		else if(WAIT_OBJECT_0!=ret)
		{
			FXERRHWIN(0);
		}
		h.relock();
		DWORD retcode=0;
		FXERRHWIN(GetExitCodeProcess(p->childh, &retcode));
		p->retcode=retcode;
		FXERRHWIN(CloseHandle(p->childh));
		p->childh=0;
	}
#endif
#ifdef USE_POSIX
	if(p->childh)
	{
		int retcode=-1;
		h.unlock();
		if(FXINFINITE==waitfor)
		{
			FXERRHOS(waitpid(p->childh, &retcode, 0));
		}
		else
		{
			FXuint start=FXProcess::getMsCount();
			do
			{
				int ret=waitpid(p->childh, &retcode, WNOHANG);
				if(!ret)
					QThread::msleep(1);
				else if(ret>0)
					goto success;
				else
					FXERRHOS(ret);
			} while(FXProcess::getMsCount()-start<waitfor);
			return false;
		}
success:
		h.relock();
		p->retcode=retcode;
		p->childh=0;
	}
#endif
	return true;
}

bool QChildProcess::terminate()
{
	if(waitForExit(0)) return false;
	QMtxHold h(p);
#ifdef USE_WINAPI
	FXERRHWIN(TerminateProcess(p->childh, -666));
	p->retcode=-666;
	p->childh=0;
#endif
#ifdef USE_POSIX
	FXERRHOS(::kill(p->childh, SIGTERM));
	if(!waitForExit(3000))
	{
		FXERRHOS(::kill(p->childh, SIGKILL));
	}
#endif
	return true;
}

bool QChildProcess::create(FXuint mode)
{
	FXERRGIO(QTrans::tr("QChildProcess", "QChildProcess::create() not possible!"));
	return false;
}

bool QChildProcess::open(FXuint mode)
{
	QMtxHold h(p);
	if(isOpen())
	{	// I keep fouling myself up here, so assertion check
		if(QIODevice::mode()!=mode) FXERRGIO(QTrans::tr("QChildProcess", "Device reopen has different mode"));
	}
	else
	{
		FXuint invmode=mode & ~IO_ReadWrite;
		if(mode & IO_ReadOnly) invmode|=IO_WriteOnly;
		if(mode & IO_WriteOnly) invmode|=IO_ReadOnly;

		/* For some undocumented reason, Windows MUST have the server end of a pipe
		be the thing it writes to when redirecting standard i/o. It is also VERY
		finickety about things being *created* with inheritability set. */
		p->childinout[1].setUnique(true);
		p->childinout[1].int_hack_makeHandlesInheritable();
		p->childinout[1].create(invmode);
		p->childinout[0].setName(p->childinout[1].name());
		p->childinout[0].open(mode);
		// Wake up the write ends
		if(p->childinout[1].isWriteable())
			p->childinout[1].writeBlock(0, 0);
		p->childinout[1].int_getOSHandles((void **) p->childinouth);
		if(mode & IO_ReadOnly)
		{
			p->childerr[1].setUnique(true);
			p->childerr[1].int_hack_makeHandlesInheritable();
			p->childerr[1].create(invmode & ~IO_ReadOnly);
			p->childerr[0].setName(p->childerr[1].name());
			p->childerr[0].open(mode & ~IO_WriteOnly);
			// Wake up the write end
			p->childerr[1].writeBlock(0, 0);
			p->childerr[1].int_getOSHandles((void **) p->childerrh);
		}

#ifdef USE_WINAPI
		STARTUPINFO si={sizeof(STARTUPINFO)};
		PROCESS_INFORMATION pi;
		FXString cmd("\""+p->command+"\" "+p->arguments);
		si.dwFlags=STARTF_USESTDHANDLES;
		si.hStdInput=p->childinouth[0];
		si.hStdOutput=p->childinouth[1];
		si.hStdError=p->childerrh[1];
		FXERRHWINFN(CreateProcess(NULL, (LPTSTR) FXUnicodify<>(cmd).buffer(), NULL, NULL, TRUE,
			CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi), p->command);
		p->childh=pi.hProcess;
		FXERRHWIN(CloseHandle(pi.hThread));
		FXERRHWIN(p->alwayssignalled=CreateEvent(NULL, TRUE, TRUE, NULL));
#endif
#ifdef USE_POSIX
		// fork() and execp().
		pid_t childh;
		QValueList<FXString> args;
		int start=0;
		bool inquote=false;
		args.push_back(p->arguments.mid(p->command.rfind('/')+1));
		for(int idx=0; idx<=p->arguments.length(); idx++)
		{
			if('\''==p->arguments[idx] && '\\'!=p->arguments[idx-1])
				inquote=!inquote;
			if(p->arguments.length()==idx || (!inquote && ' '==p->arguments[idx]))
			{
				args.push_back(p->arguments.mid(start, idx-start));
				start=idx+1;
			}
		}
		QMemArray<const char *> args_(args.size()+1);
		start=0;
		for(QValueList<FXString>::const_iterator it=args.begin(); it!=args.end(); ++it)
			args_[start++]=it->text();
		FXERRHOS(childh=fork());
		if(!childh)
		{	// This being the child process. As we call dup2() we had to use fork() instead of vfork()
			int ret=0;
			if(ret>=0) ret=dup2(p->childinouth[0],  STDIN_FILENO);
			if(ret>=0) ret=dup2(p->childinouth[1], STDOUT_FILENO);
			if(ret>=0) ret=dup2(p->childerrh[1],   STDERR_FILENO);
			// Don't need to shut these as they all have FD_CLOEXEC set on them by QPipe
			//if(ret>=0) ret=::close(p->childinouth[0]);
			//if(ret>=0) ret=::close(p->childinouth[1]);
			//if(ret>=0) ret=::close(p->childerrh[0]);
			//if(ret>=0) ret=::close(p->childerrh[1]);
			if(ret>=0) ret=execvp(p->command.text(), (char *const *) args_.data());
			if(ret<0)
				::_exit(-666);
			// Never returns
		}
		p->childh=childh;
		int hs[2];
		FXERRHOS(::pipe(hs));
		p->alwayssignalled=hs[0];
		FXERRHOS(::fcntl(p->alwayssignalled, F_SETFD, ::fcntl(p->alwayssignalled, F_GETFD, 0)|FD_CLOEXEC));
		FXERRHOS(::write(hs[1], hs, 1));
		FXERRHOS(::close(hs[1]));
#endif
		// Don't need the child end of the pipe anymore
		p->childinout[1].close();
		p->childerr[1].close();
		p->connectionBroken=0;
		if(mode & IO_ReadOnly)
		{
			p->outlog.open(IO_ReadWrite);
			p->errlog.open(IO_ReadWrite);
		}
		setFlags((mode & IO_ModeMask)|IO_Open);
		p->detached=false;
		FXProcess::FatalExitUpcallSpec kc(this, &QChildProcess::int_killChildI);
		FXProcess::addFatalExitUpcall(kc);
	}
	return true;
}

void QChildProcess::close()
{
	if(p)
	{
		flush();
		if(!p->detached)
		{
			waitForExit();
			detach();
		}
		QMtxHold h(p);
		p->childinout[0].close();
		p->childinout[1].close();
		p->childerr[0].close();
		p->childerr[1].close();
		if(p->outlog.isOpen())
		{
			p->outlog.truncate(0);
			p->outlog.close();
		}
		if(p->errlog.isOpen())
		{
			p->errlog.truncate(0);
			p->errlog.close();
		}
		memset(p->childinouth, 0, sizeof(p->childinouth)*2);
		if(p->alwayssignalled)
		{
#ifdef USE_WINAPI
			FXERRHWIN(CloseHandle(p->alwayssignalled));
#endif
#ifdef USE_POSIX
			FXERRHOS(::close(p->alwayssignalled));
#endif
			p->alwayssignalled=0;
		}
		setFlags(0);
	}
}

void QChildProcess::flush()
{
	if(isOpen() && isWriteable())
	{
		p->childinout[0].flush();
	}
}

FXfval QChildProcess::size() const
{
	QMtxHold h(p);
	if(!isReadable()) return 0;
	if(QChildProcess::StdOut==p->readChannel)
		return p->outlog.size()+p->childinout[0].size();
	else if(QChildProcess::StdErr==p->readChannel)
		return p->errlog.size()+p->childerr[0].size();
	else
	{
		return p->outlog.size()+p->childinout[0].size()+p->errlog.size()+p->childerr[0].size();
	}
}


FXuval QChildProcess::readBlock(char *data, FXuval maxlen)
{
	QMtxHold h(p);
	if(!QIODevice::isReadable()) FXERRGIO(QTrans::tr("QChildProcess", "Not open for reading"));
	if(isOpen())
	{
		FXuval read, readed=0;
		do
		{
			if(StdOut & p->readChannel)
			{
				QByteArray &b=p->outlog.buffer();
				if((read=p->outlog.readBlockFrom(data, maxlen, 0)))
				{
					memmove(b.data(), b.data()+p->outlog.at(), (size_t)(b.size()-p->outlog.at()));
					b.resize((FXuint)(b.size()-p->outlog.at()));
					p->outlog.at(0);
					data+=read;
					maxlen-=read;
					readed+=read;
				}
			}
			if(StdErr & p->readChannel)
			{
				QByteArray &b=p->errlog.buffer();
				if((read=p->errlog.readBlockFrom(data, maxlen, 0)))
				{
					memmove(b.data(), b.data()+p->errlog.at(), (size_t)(b.size()-p->errlog.at()));
					b.resize((FXuint)(b.size()-p->errlog.at()));
					p->errlog.at(0);
					data+=read;
					maxlen-=read;
					readed+=read;
				}
			}
			if(3==p->connectionBroken) break;
			// Do a speculative read if we already have stuff to return, otherwise block
			if(maxlen)
			{
				h.unlock();
				QIODeviceS *devs[2], *ready[3];
				devs[0]=&p->childinout[0];
				devs[1]=&p->childerr[0];
				ready[2]=0;
				if(QIODeviceS::waitForData(ready, 2, devs, readed ? 0 : FXINFINITE))
				{
					for(QIODeviceS **dev=ready; *dev; dev++)
					{
						if(*dev==&p->childinout[0] && !(p->connectionBroken & 1))
						{
							FXERRH_TRY
							{
								do
								{
									char buffer[4096];
									if(!(read=p->childinout[0].readBlock(buffer, sizeof(buffer))))
									{
										p->connectionBroken|=1;
										break;
									}
									h.relock();
									p->outlog.writeBlock(buffer, read);
									h.unlock();
								} while(!p->childinout[0].atEnd());
							}
							FXERRH_CATCH(FXConnectionLostException &)
							{	// Sink this error - QPipe will return zero bytes read from now on
								p->connectionBroken|=1;
							}
							FXERRH_ENDTRY
						}
						else if(*dev==&p->childerr[0] && !(p->connectionBroken & 2))
						{
							FXERRH_TRY
							{
								do
								{
									char buffer[4096];
									if(!(read=p->childerr[0].readBlock(buffer, sizeof(buffer))))
									{
										p->connectionBroken|=2;
										break;
									}
									h.relock();
									p->errlog.writeBlock(buffer, read);
									h.unlock();
								} while(!p->childerr[0].atEnd());
							}
							FXERRH_CATCH(FXConnectionLostException &)
							{	// Sink this error - QPipe will return zero bytes read from now on
								p->connectionBroken|=2;
							}
							FXERRH_ENDTRY
						}
					}
				}
				h.relock();
			}
		} while(!readed);
		return readed;
	}
	return 0;
}

FXuval QChildProcess::writeBlock(const char *data, FXuval maxlen)
{
	QMtxHold h(p);
	if(!isWriteable()) FXERRGIO(QTrans::tr("QChildProcess", "Not open for writing"));
	if(isOpen())
	{
		FXuval written=0;
		if(!p->connectionBroken)
		{
			FXERRH_TRY
			{
				h.unlock();
				do
				{
					FXuval writ=p->childinout[0].writeBlock(data, maxlen);
					data+=writ;
					maxlen-=writ;
					written+=writ;
				} while(maxlen);
				h.relock();
			}
			FXERRH_CATCH(FXConnectionLostException &)
			{	// Sink this error - QPipe will return zero bytes read from now on
				p->connectionBroken=3;
			}
			FXERRH_ENDTRY
		}
		return written;
	}
	return 0;
}

int QChildProcess::ungetch(int)
{
	return -1;
}


} // namespace

