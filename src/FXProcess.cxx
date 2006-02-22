/********************************************************************************
*                                                                               *
*                         P r o c e s s   S u p p o r t                         *
*                                                                               *
*********************************************************************************
* Copyright (C) 2002,2003 by Niall Douglas.   All Rights Reserved.              *
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

#include "fxdefs.h"
#include "fxver.h"
#include "FXProcess.h"
#include "FXException.h"
#include "FXPtrHold.h"
#include "FXErrCodes.h"
#include "QFile.h"
#include "QBuffer.h"
#include "FXStream.h"
#include "QTrans.h"
#include "QThread.h"
#include "FXACL.h"
#include "FXApp.h"
#include "FXRollback.h"
#include "FXTime.h"
#include "FXPath.h"
#include "FXStat.h"
#include <stdlib.h>
#include <qptrlist.h>
#include <qvaluelist.h>
#include <qsortedlist.h>
#include <qmemarray.h>
#include <assert.h>

// Decide which API to use
#ifndef USE_POSIX
 #define USE_WINAPI
 #include "WindowsGubbins.h"
 #include "psapi.h"
 #include "DbgHelp.h"
 #include <sys/timeb.h>
 #if defined(_M_IX86)
  // MSVC does give us some idea of what we are compiling
  #if _M_IX86>=800
   #define ARCHITECTURE "x64"
  #elif _M_IX86>=600
   #define ARCHITECTURE "i686"
  #elif _M_IX86>=500
   #define ARCHITECTURE "i586"
  #elif _M_IX86>=400
   #define ARCHITECTURE "i486"
  #else
   #error Unknown x86 architecture
  #endif
 #elif defined(_M_ALPHA)
  #define ARCHITECTURE "Alpha"
 #elif defined(_M_IA64)
  #define ARCHITECTURE "ia64"
 #else
  #define ARCHITECTURE "i486"
 #endif
#else
 #include <sys/time.h>
 #include <unistd.h>
 #include <errno.h>
 #include <signal.h>
 #include <dlfcn.h>
 #include <sys/mman.h>
 #ifdef __linux__
  #include <sys/fsuid.h>
  #include <sys/mount.h>
 #endif
 #ifdef __FreeBSD__
  #include <sys/sysctl.h>
  #include <sys/vmmeter.h>
  #include <sys/param.h>
  #include <sys/mount.h>
 #endif
 // GCC lets us know what it's compiling
 #if defined(__x86_64__)
  #define ARCHITECTURE "x64"
 #elif defined(__i686__) || defined(__athlon__) || defined(__pentiumpro__) || defined(__pentium4__)
  #define ARCHITECTURE "i686"
 #elif defined(__i586__) || defined(__k6__)
  #define ARCHITECTURE "i586"
 #elif defined(__i386__) || defined(__i486__)
  #define ARCHITECTURE "i486"
 #elif defined(__ia64__)
  #define ARCHITECTURE "ia64"
 #elif defined(__alpha__)
  #define ARCHITECTURE "Alpha"
 #elif defined(__mips__)
  #define ARCHITECTURE "MIPS"
 #elif defined(__hppa__)
  #define ARCHITECTURE "PA_RISC"
 #elif defined(__powerpc__)
  #define ARCHITECTURE "PPC"
 #elif defined(__sparc__)
  #define ARCHITECTURE "SPARC"
 #else
  #error Unknown architecture
 #endif
#endif
#include "FXMemDbg.h"
#if defined(DEBUG) && defined(FXMEMDBG_H)
static const char *_fxmemdbg_current_file_ = __FILE__;
#endif

#ifdef DEBUG
#define FXPROCESS_POOLTHREADS 1
#else
#define FXPROCESS_POOLTHREADS 4
#endif

namespace FX {

/* FXTime implementation, see FXTime.h */
#ifdef WIN32
static QMutex gmtimelock;
#endif
struct tm *FXTime::as_tm(struct tm *buf, bool inLocalTime) const
{
	time_t tmp=as_time_t();
	if(inLocalTime==isLocalTime) {}
	else if(inLocalTime && !isLocalTime)
		tmp+=(time_t)(localTimeDiff()/micsPerSecond);
	else if(!inLocalTime && isLocalTime)
		tmp-=(time_t)(localTimeDiff()/micsPerSecond);
#if defined(USE_POSIX) && defined(FOX_THREAD_SAFE)
	return inLocalTime ? localtime_r(&tmp, buf) : gmtime_r(&tmp, buf);
#else
#ifdef WIN32
	QMtxHold h(gmtimelock);
#endif
	struct tm *src=inLocalTime ? localtime(&tmp) : gmtime(&tmp);
	assert(src);
	FXERRH(src, "Failed to convert to struct tm", 0, FXERRH_ISDEBUG);
	*buf=*src;
	return buf;
#endif
}
FXTime &FXTime::set_tm(struct tm *buf, bool isInLocalTime)
{
	time_t temp=mktime(buf);
	assert(temp>=0);
	FXERRH(temp>=0, "Supplied struct tm structure is bad", 0, FXERRH_ISDEBUG);
	set_time_t(temp);
	if(isInLocalTime==isLocalTime) {}
	else if(isInLocalTime && !isLocalTime)
		value-=localTimeDiff();
	else if(!isInLocalTime && isLocalTime)
		value+=localTimeDiff();
	return *this;
}

FXString FXTime::asString(const FXString &fmt) const
{
	FXString ret(fmt);
	FXint insertcnt=0;
	while(-1!=ret.find("%F"))
		ret.substitute("%F", "%%"+FXString::number(++insertcnt), FALSE);
	if(!isLocalTime)
		ret.substitute("%Z", "UTC");

	FXchar buffer[512];
	struct tm tmresult;
	FXint len=(FXint) strftime(buffer, sizeof(buffer), ret.text(), as_tm(&tmresult, isLocalTime));
	ret.assign(buffer, len);

	if(insertcnt)
	{
		FXString fractstr(FXString::number((value % micsPerSecond)/(double) micsPerSecond, 'f', 6).mid(2));
		while(insertcnt--)
			ret.arg(fractstr);
	}
	return ret;
}

FXTime FXTime::now(bool inLocalTime)
{
#ifdef WIN32
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	if(inLocalTime)
	{
		FILETIME t=ft;
		FileTimeToLocalFileTime(&t, &ft);
	}
	FXTime ret(0, inLocalTime);
	FXTIMEFROMFILETIME(ret, ft);
	return ret;
#else
	struct timeval tv;
	FXERRHOS(gettimeofday((::timeval *) &tv, 0));
	FXTime ret((time_t)tv.tv_sec);
	ret.value+=tv.tv_usec;
	if(inLocalTime) ret.toLocalTime();
	return ret;
#endif
}
FXlong FXTime::localTimeDiff()
{
#ifdef WIN32
	FILETIME i, o;
	i.dwHighDateTime=1000;
	i.dwLowDateTime=0;
	FileTimeToLocalFileTime(&i, &o);
	FXulong _i=((FXulong)i.dwHighDateTime<<32)|(FXulong)i.dwLowDateTime, _o=((FXulong)o.dwHighDateTime<<32)|(FXulong)o.dwLowDateTime;
	return (FXlong)(_o-_i);
#else
	time_t tmp=1<<28;
	struct tm buf1, buf2;
#ifdef FOX_THREAD_SAFE
	localtime_r(&tmp, &buf1);
	gmtime_r(&tmp, &buf2);
#else
	buf1=*localtime(&tmp);
	buf2=*gmtime(&tmp);
#endif
	// buf1 has local time, buf2 UTC
	// mktime undoes localtime, so buf2 becomes minus local diff
	return mktime(&buf1)-mktime(&buf2);
#endif
}

FXStream &operator<<(FXStream &s, const FXTime &i)
{
	if(i.isLocalTime)
	{
		s << (FXuchar) 1;
		FXTime tmp(i.value-FXTime::localTimeDiff());
		s << i.value;
	}
	else
		s << (FXuchar) 0 << i.value;
	return s;
}
FXStream &operator>>(FXStream &s, FXTime &i)
{
	FXuchar l; s >> l;
	if(l)
	{
		i.isLocalTime=true;
		s >> i.value;
		i.value+=FXTime::localTimeDiff();
	}
	else
	{
		i.isLocalTime=false;
		s >> i.value;
	}
	return s;
}




static FXProcess *myprocess;
static QList<FXProcess_StaticInitBase> *SIlist;
static QList<FXProcess_StaticDepend> *SIdepend;
static struct ExitUpcalls
{
	QMutex lock;
	QPtrList<FXProcess::FatalExitUpcallSpec> upcalls;
	ExitUpcalls() : upcalls(true) { }
} *exitupcalls;

static void RunFatalExitCalls(bool fatal)
{
	if(exitupcalls)
	{
		QMtxHold h(exitupcalls->lock);
		for(QPtrListIterator<FXProcess::FatalExitUpcallSpec> it(exitupcalls->upcalls); it.current(); ++it)
		{
			try
			{
				(*it.current())(fatal);
			} catch(...)
			{	// Sink all exceptions
			}
		}
		h.unlock();
		FXDELETE(exitupcalls);
	}
}
static void atexitRunFatalExitCalls()
{
	RunFatalExitCalls(false);
}
#ifdef USE_WINAPI
static BOOL WINAPI ConsoleCtrlHandler(DWORD type)
{
	switch(type)
	{
	case CTRL_CLOSE_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
		{	// Only thing we can really do is ask FXApp to exit
#ifndef FX_DISABLEGUI
			FXEventLoop *pel=FXApp::getPrimaryEventLoop();
			pel->postAsyncMessage(FXApp::instance(), FXSEL(SEL_COMMAND, FXApp::ID_QUIT));
#else
			FXProcess::exit(0);
#endif
			return TRUE;
		}
	}
	return FALSE;
}
static LONG MyUnhandledExceptionFilter(EXCEPTION_POINTERS *ei)
{
	RunFatalExitCalls(true);
	FXString codestr("Unknown");
	switch(ei->ExceptionRecord->ExceptionCode)
	{
	case EXCEPTION_ACCESS_VIOLATION:
		{
			codestr.format("Access violation %s 0x%x",
				ei->ExceptionRecord->ExceptionInformation[0]==1 ? "writing" : "reading",
				ei->ExceptionRecord->ExceptionInformation[1]);
			break;
		}
	case EXCEPTION_DATATYPE_MISALIGNMENT:
		codestr="Misaligned data access";
		break;
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:
	case EXCEPTION_INT_DIVIDE_BY_ZERO:
		codestr="Divide by zero";
		break;
	case EXCEPTION_ILLEGAL_INSTRUCTION:
		codestr="Illegal instruction";
		break;
	case EXCEPTION_IN_PAGE_ERROR:
		codestr="In page error";
		break;
	case EXCEPTION_PRIV_INSTRUCTION:
		codestr="Attempt to execute privileged instruction";
		break;
	case EXCEPTION_STACK_OVERFLOW:
		codestr="Stack overflow";
		break;
	}
	// Don't translate as want to do as little as possible
	fprintf(stderr, "\nUnhandled Exception 0x%x (%s)\n    at address 0x%p - immediate exit!\n",
		ei->ExceptionRecord->ExceptionCode, codestr.text(), ei->ExceptionRecord->ExceptionAddress);
	fflush(stderr);
	return EXCEPTION_CONTINUE_SEARCH;
}
#endif
#ifdef USE_POSIX
static void MyUnhandledSignalHandler(int sig, siginfo_t *si, void *data)
{
	RunFatalExitCalls(true);
	FXString codestr("Unknown");
	switch(sig)
	{
	case SIGABRT:
		codestr="Abnormal termination";
		break;
	case SIGALRM:
		codestr="Timeout alarm";
		break;
	case SIGBUS:
		{
			const char *detail="Unknown";
#ifdef __linux__
			switch(si->si_code)
			{
			case BUS_ADRALN:
				detail="Invalid address alignment";
				break;
			case BUS_ADRERR:
				detail="Non-existant physical address";
				break;
			case BUS_OBJERR:
				detail="Object specific hardware error";
				break;
			}
#endif
			codestr.format("Bus error [%s]", detail);
			break;
		}
	case SIGFPE:
		{
			const char *detail="Unknown";
			switch(si->si_code)
			{
			case FPE_INTDIV:
			case FPE_FLTDIV:
				detail="Divide by zero";
				break;
			}
			codestr.format("Floating-point exception [%s]", detail);
			break;
		}
	case SIGHUP:
		codestr="Hang up";
		break;
	case SIGILL:
		{
			const char *detail="Unknown";
#ifdef __linux__
			switch(si->si_code)
			{
			case ILL_ILLOPC:
				detail="Illegal opcode";
				break;
			case ILL_ILLOPN:
				detail="Illegal operand";
				break;
			case ILL_ILLADR:
				detail="Illegal addressing mode";
				break;
			case ILL_ILLTRP:
				detail="Illegal trap";
				break;
			case ILL_PRVOPC:
				detail="Privileged opcode";
				break;
			case ILL_PRVREG:
				detail="Privileged register";
				break;
			case ILL_COPROC:
				detail="Coprocessor error";
				break;
			case ILL_BADSTK:
				detail="Internal stack error";
				break;
			}
#endif
			codestr.format("Illegal instruction [%s]", detail);
			break;
		}
	case SIGINT:
		codestr="User interrupt";
		break;
	case SIGKILL:
		codestr="Terminated";
		break;
	case SIGPIPE:
		codestr="Broken pipe";
		break;
	case SIGPROF:
		codestr="Profiling timer alarm";
		break;
	case SIGQUIT:
		codestr="Quit";
		break;
	case SIGSEGV:
		{
			const char *detail="Unknown";
#ifdef __linux__
			switch(si->si_code)
			{
			case SEGV_MAPERR:
				detail="Address not mapped to object";
				break;
			case SEGV_ACCERR:
				detail="Invalid permissions for mapped object";
				break;
			}
#endif
			codestr.format("Segmentation fault [%s]", detail);
			break;
		}
	case SIGSTOP:
		codestr="Stop process";
		break;
	case SIGSYS:
		codestr="Invalid system call";
		break;
	case SIGTERM:
		codestr="User terminate";
		break;
	case SIGTRAP:
		codestr="Trace trap";
		break;
	case SIGXCPU:
		codestr="CPU time limit exceeded";
		break;
	case SIGXFSZ:
		codestr="File size limit exceeded";
		break;
	}
	// Don't translate as want to do as little as possible
	fprintf(stderr, "\nUnhandled Signal 0x%x (%s)\n    at address %p - immediate exit!\n",
		sig, codestr.text(), si->si_addr);
	fflush(stderr);
	_exit(EXIT_FAILURE);
}
#endif

struct FXDLLLOCAL FXProcessPrivate : public QMutex
{
	struct args_t
	{
		int argc;
		char **argv;
	} argscopy;
    struct Overrides_t
    {
        FXfloat memory;
        FXfloat processor;
        FXfloat discio;
        Overrides_t() : memory(-1), processor(-1), discio(-1) { }
    } overrides;
	QThreadPool *threadpool;
	FXProcess::UserHandedness handedness;
	FXuint screenScale;
	FXint maxScreenWidth, maxScreenHeight;
	FXProcessPrivate() : threadpool(0), handedness(FXProcess::UNKNOWN_HANDED),
		screenScale(100), maxScreenWidth(0x7fffffff), maxScreenHeight(0x7fffffff) { }
};

FXProcess::FXProcess() : p(0)
{
	int argc=0;

	char *argv[]={ "" };
	init(argc, argv);
}

FXProcess::FXProcess(int &argc, char *argv[]) : p(0)
{
	init(argc, argv);
}

void FXProcess::init(int &argc, char *argv[])
{
	FXERRH_TRY
	{
		bool inHelpMode=false;
#if defined(_MSC_VER) && defined(_DEBUG) && !defined(FXMEMDBG_DISABLE)
		_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF|_CRTDBG_LEAK_CHECK_DF|_CRTDBG_CHECK_EVERY_1024_DF);
		//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF|_CRTDBG_LEAK_CHECK_DF|_CRTDBG_CHECK_EVERY_128_DF);
		//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF|_CRTDBG_LEAK_CHECK_DF|_CRTDBG_CHECK_EVERY_16_DF);
		//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF|_CRTDBG_LEAK_CHECK_DF|_CRTDBG_CHECK_ALWAYS_DF);
#endif
#ifdef USE_WINAPI
		SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
		SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER) MyUnhandledExceptionFilter);
		SetErrorMode(SEM_FAILCRITICALERRORS);
#endif
#ifdef USE_POSIX
		{	// Point all process terminate signals to our handler.
			struct sigaction act;
			act.sa_sigaction=MyUnhandledSignalHandler;
			sigemptyset(&act.sa_mask);
			act.sa_flags=SA_SIGINFO;
			static int sigs[]={ SIGABRT, SIGALRM, SIGBUS, SIGFPE, SIGHUP, SIGILL, SIGINT, /*SIGKILL,*/ SIGPIPE,
				SIGPROF, SIGQUIT, SIGSEGV, /*SIGSTOP,*/ SIGSYS, SIGTERM, /*SIGTRAP,*/ SIGXCPU, SIGXFSZ };
			for(uint n=0; n<sizeof(sigs)/sizeof(int); n++)
			{
				sigaction(sigs[n], &act, NULL);
			}
		}
#ifdef __linux__
		// Set file access impersonation to real user id (not effective as per default)
		setfsuid(getuid());
		setfsgid(getgid());
#endif
		/*{
			uid_t uid, euid, suid;
			getresuid(&uid, &euid, &suid);
			fxmessage("uid=%d, euid=%d, suid=%d\n", uid, euid, suid);
		}*/
		{	// Ensure /proc is working
			FXString myprocloc=FXString("/proc/%1").arg(id());
			if(!FXStat::exists(myprocloc))
			{
				fxerror("The /proc pseudo-filing system must be installed and mounted\nfor this application to run\n");
				::exit(1);
			}
		}
#endif
		atexit(atexitRunFatalExitCalls);
		FXERRHM(p=new FXProcessPrivate);
		myprocess=this;
		p->argscopy.argc=0;
		p->argscopy.argv=0;
		p->threadpool=0;
		QBuffer txtholder;
		txtholder.open(IO_ReadWrite);
		FXStream stxtholder(&txtholder);
		if(runPendingStaticInits(argc, argv, stxtholder)) inHelpMode=true;
		p->argscopy.argc=argc;
		p->argscopy.argv=argv;
		QIODevice &stdio=QFile::stdio(true);
		FXStream sstdio(&stdio);
		for(int argi=0; argi<argc; argi++)
		{
			if(0==strcmp(argv[argi], "-help"))
			{
				inHelpMode=true;
				QTransString temp2=QTrans::tr("FXProcess", "%1 based on the TnFOX portable library v%2.%3\n   (derived from FOX v%4.%5.%6) (built: %7 %8)\n");
				temp2.arg(FXPath::name(FXProcess::execpath()).text()).arg(TNFOX_MAJOR).arg(TNFOX_MINOR);
				temp2.arg(FOX_MAJOR).arg(FOX_MINOR).arg(FOX_LEVEL).arg(FXString(__TIME__)).arg(FXString(__DATE__));
				FXString temp(temp2);
				sstdio << temp.text();
				temp=QTrans::tr("FXProcess", "TnFOX (http://www.nedprod.com/TnFOX/) is heavily based upon the FOX portable\n"); sstdio << temp.text();
				temp=QTrans::tr("FXProcess", "GUI toolkit (http://www.fox-toolkit.org/) and all rights are reserved\n\n"); sstdio << temp.text();
				temp=QTrans::tr("FXProcess", "  -sysinfo               : Show information about the system & environment\n"); sstdio << temp.text();
				temp=QTrans::tr("FXProcess", "  -fxhanded=<left|right> : Overrides the handedness of the user\n"); sstdio << temp.text();
				temp=QTrans::tr("FXProcess", "  -fxmemoryfull=<fpno>   : Overrides the memory full setting\n"); sstdio << temp.text();
				temp=QTrans::tr("FXProcess", "  -fxscreenscale=<%>     : Overrides the window layout scaling factor\n"); sstdio << temp.text();
				temp=QTrans::tr("FXProcess", "  -fxscreensize=w,h      : Constrains the screen (debug only)\n"); sstdio << temp.text();
				break;
			}
			else if(0==strcmp(argv[argi], "-sysinfo"))
			{
				inHelpMode=true;
				FXString temp;
				temp=QTrans::tr("FXProcess", "TnFOX system information:\n\n"); sstdio << temp.text();
				temp=QTrans::tr("FXProcess", "The host OS is: %1\n").arg(FXProcess::hostOSDescription()); sstdio << temp.text();
				temp=QTrans::tr("FXProcess", "The millisecond count is %1, process id is %2 and this machine has %3 processors\n")
					.arg(FXProcess::getMsCount()).arg(FXProcess::id()).arg(FXProcess::noOfProcessors());
				sstdio << temp.text();
				temp=QTrans::tr("FXProcess", "System load: processor=%1, memory=%2, disc i/o=%3 with %4 VAS free\n")
					.arg(FXProcess::hostOSProcessorLoad(), 0, 'f', 2).arg(FXProcess::hostOSMemoryLoad(), 0, 'f', 2)
					.arg(FXProcess::hostOSDiscIOLoad(FXProcess::execpath()), 0, 'f', 2)
					.arg(fxstrfval(FXProcess::virtualAddrSpaceLeft()));
				sstdio << temp.text();
				temp=QTrans::tr("FXProcess", "This process was created by the user %1 (group %2)\n")
					.arg(FXACLEntity::currentUser().asString()).arg(FXACLEntity::currentUser().group().asString());
				sstdio << temp.text();
				if(LEFT_HANDED==userHandedness())
					temp=QTrans::tr("FXProcess", "    who will experience a left-handed interface\n");
				else
					temp=QTrans::tr("FXProcess", "    who will experience a right-handed interface\n");
				sstdio << temp.text();
				temp=QTrans::tr("FXProcess", "There are the following files mapped into this process' memory:\n");
				sstdio << temp.text();
				QValueList<MappedFileInfo> list=FXProcess::mappedFiles();
				for(QValueList<MappedFileInfo>::iterator it=list.begin(); it!=list.end(); ++it)
				{
					static const int addrwidth=sizeof(void *)*2;
					QTransString temp2(QTrans::tr("FXProcess", "  %1 to %2 %3%4%5%6 %7\n"));
					temp2.arg((FXulong) (*it).startaddr,-addrwidth,16).arg((FXulong) (*it).endaddr,-addrwidth,16);
					temp2.arg(((*it).read) ? 'r' : '-').arg(((*it).write) ? 'w' : '-').arg(((*it).execute) ? 'x' : '-').arg(((*it).copyonwrite) ? 'c' : '-');
					temp2.arg((*it).path);
					temp=temp2;
					sstdio << temp.text();
				}
			}
			else if(0==strncmp(argv[argi], "-fxhanded=", 10))
			{
				FXString s(argv[argi]+10); s.lower();
				if(s=="left")
					overrideUserHandedness(LEFT_HANDED);
				else if(s=="right")
					overrideUserHandedness(RIGHT_HANDED);
				else
				{
					FXString temp=QTrans::tr("FXProcess", "Unknown option '%1' passed to %2\n").arg(s).arg("-fxhanded");
					fxwarning("%s\n", temp.text());
				}
			}
			else if(0==strncmp(argv[argi], "-fxmemoryfull=", 14))
			{
				FXString s(argv[argi]+14);
				bool ok;
				double val=s.toDouble(&ok);
				if(ok)
					overrideFreeResources((FXfloat) val);
				else
				{
					FXString temp=QTrans::tr("FXProcess", "Unknown option '%1' passed to %2\n").arg(s).arg("-fxmemoryfull");
					fxwarning("%s\n", temp.text());
				}
			}
			else if(0==strncmp(argv[argi], "-fxscreenscale=", 15))
			{
				FXString s(argv[argi]+15);
				bool ok;
				FXuint val=s.toUInt(&ok);
				if(ok)
					overrideScreenScale(val);
				else
				{
					FXString temp=QTrans::tr("FXProcess", "Unknown option '%1' passed to %2\n").arg(s).arg("-fxscreenscale");
					fxwarning("%s\n", temp.text());
				}
			}
			else if(0==strncmp(argv[argi], "-fxscreensize=", 14))
			{
				FXString s(argv[argi]+14);
				bool ok, bad=false;
				FXuint x=s.toUInt(&ok), cpos=s.find(',');
				if(ok && cpos>0)
				{
					FXuint y=s.mid(cpos+1,255).toUInt(&ok);
					if(ok) { p->maxScreenWidth=(FXint) x; p->maxScreenHeight=(FXint) y; }
					else bad=true;
				}
				else bad=true;
				if(bad)
				{
					FXString temp=QTrans::tr("FXProcess", "Unknown option '%1' passed to %2\n").arg(s).arg("-fxscreensize");
					fxwarning("%s\n", temp.text());
				}
			}
		}
		if(inHelpMode)
		{
			sstdio << txtholder;
			goto doexit;
		}
		FXException::int_enableNestedExceptionFramework();
	}
	FXERRH_CATCH(FXException &e)
	{
		fxerror("%s\n",e.report().text());
		exit(1);
	}
	FXERRH_ENDTRY
	return;
doexit:
	exit(0);
}

FXProcess::~FXProcess()
{ FXEXCEPTIONDESTRUCT1 {
	destroy();
} FXEXCEPTIONDESTRUCT2; }

void FXProcess::destroy()
{
	FXException::int_enableNestedExceptionFramework(false);
	if(SIlist)
	{
		FXProcess_StaticInitBase *i;
		for(QListIterator<FXProcess_StaticInitBase> it(*SIlist); (i=it.current()); ++it)
		{
			if(i->done)
			{
#ifdef DEBUG
				fxmessage("Destroying static init %s\n", Generic::typeInfoBase(typeid(*i)).name().text());
#endif
				i->destroy();
				i->done=false;
			}
		}
	}
	FXDELETE(p->threadpool);
	FXDELETE(p);
	myprocess=0;
}

bool FXProcess::runPendingStaticInits(int &argc, char *argv[], FXStream &txtout)
{
	bool quitNow=false;
	if(SIlist)
	{
		if(SIdepend)
		{	// Assemble dependencies
			// TODO: Convert SIlist to hash table first
			QListIterator<FXProcess_StaticDepend> it(*SIdepend);
			for(; it.current(); ++it)
			{
				const char *onwhatname=(*it)->onwhat;
				QListIterator<FXProcess_StaticInitBase> it2(*SIlist);
				for(; it2.current(); ++it2)
				{
					if(0==strcmp((*it2)->name(), onwhatname)) break;
				}
				if(!it2.current()) FXERRG(FXString("Dependency at line %1 of file %2 unknown").arg((*it)->lineno).arg((*it)->module), FXPROCESS_UNKNOWNDEP, FXERRH_ISDEBUG);
				FXProcess_StaticInitBase *var=(*it2);
				if(!var->dependencies)
				{
					FXERRHM(var->dependencies=new QList<FXProcess_StaticInitBase>);
				}
				QList<FXProcess_StaticInitBase> *deplist=(QList<FXProcess_StaticInitBase> *) var->dependencies;
				deplist->append((*it)->var);
			}
			FXDELETE(SIdepend);
		}
		{	// Reorder list
			FXPtrHold<QList<FXProcess_StaticInitBase> > newSIlist;
			FXERRHM(newSIlist=new QList<FXProcess_StaticInitBase>);
			for(QListIterator<FXProcess_StaticInitBase> o(*SIlist); o.current();)
			{
				FXProcess_StaticInitBase *var=(*o);
				if(!var->dependencies)
				{
					newSIlist->append(var);
					QListIterator<FXProcess_StaticInitBase> co=o; ++o;
					SIlist->removeByIter(co);
				}
				else ++o;
			}
			while(SIlist->count())
			{
				bool movement=false;
				for(QListIterator<FXProcess_StaticInitBase> o(*SIlist); o.current(); )
				{
					FXProcess_StaticInitBase *var=(*o);
					QList<FXProcess_StaticInitBase> *deplist=(QList<FXProcess_StaticInitBase> *) var->dependencies;
					assert(deplist);
					QListIterator<FXProcess_StaticInitBase> d(*deplist);
					for(; d.current(); ++d)
					{
						if(-1==newSIlist->findRef(*d)) break;
					}
					++o;
					if(!d.current())
					{	// Found all dependencies already done, so move it
						newSIlist->append(var);
						SIlist->removeRef(var);
						movement=true;
					}
				}
				if(!movement)
				{
					FXString msg("FXProcess found unresolvable dependencies in the following list:\n");
					for(QListIterator<FXProcess_StaticInitBase> o(*SIlist); o.current(); ++o)
					{
						FXProcess_StaticInitBase *var=(*o);
						msg+=FXString(var->name())+" (depends on";
						QList<FXProcess_StaticInitBase> *deplist=(QList<FXProcess_StaticInitBase> *) var->dependencies;
						for(QListIterator<FXProcess_StaticInitBase> d(*deplist); d.current(); ++d)
						{
							msg+=FXString(" ")+(*d)->name();
						}
						msg+=")\n";
					}
					FXERRG(msg, FXPROCESS_UNRESOLVEDDEPS, FXERRH_ISDEBUG);
				}
			}
			FXDELETE(SIlist);
			SIlist=newSIlist;
			newSIlist=0;
		}
		// Move "TComponentManager" to before everything else
		QListIterator<FXProcess_StaticInitBase> it(*SIlist);
		for(; it.current(); ++it)
		{
			if(0==strcmp((*it)->myname, "TComponentManager"))
			{
				SIlist->append(*it);
				SIlist->removeRef(*it);
				break;
			}
		}
		// Move "FXPrimaryThread" to before everything else
		for(it.toFirst(); it.current(); ++it)
		{
			if(0==strcmp((*it)->myname, "FXPrimaryThread"))
			{
				SIlist->append(*it);
				SIlist->removeRef(*it);
				break;
			}
		}
		for(it.toLast(); it.current(); --it)
		{
			if(!(*it)->done)
			{
				if((*it)->create(argc, argv, txtout)) quitNow=true;
				(*it)->done=true;
			}
		}
	}
	return quitNow;
}

void FXProcess::exit(FXint code)
{
	instance()->destroy();
	::exit(code);
}

FXProcess *FXProcess::instance()
{
	return myprocess;
}

FXuint FXProcess::getMsCount()
{
#ifdef USE_WINAPI
	return GetTickCount();
#endif
#ifdef USE_POSIX
	struct timeval tv;
	FXERRHOS(gettimeofday((::timeval *) &tv, 0));
	return (tv.tv_sec*1000)+(tv.tv_usec/1000);
#endif
}

void FXProcess::getTimeOfDay(struct timeval *tv)
{
#ifdef USE_WINAPI
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	FXulong time=((FXulong)ft.dwHighDateTime<<32)|(FXulong)ft.dwLowDateTime;
	tv->tv_sec=(long)(time/(100*1000000));
	tv->tv_usec=(long)((time % (100*1000000))/10);
#endif
#ifdef USE_POSIX
	FXERRHOS(gettimeofday((::timeval *) tv, 0));
#endif
}

FXulong FXProcess::getNsCount()
{
#ifdef USE_WINAPI
	static LARGE_INTEGER ticksPerSec;
	static double scalefactor;
	// It's faster to use the FPU than a 128 bit MulDiv implementation
	// though this would change if SSE2 became more prevalent
	if(!scalefactor)
	{
		if(QueryPerformanceFrequency(&ticksPerSec))
			scalefactor=ticksPerSec.QuadPart/1000000000.0;
		else
			scalefactor=1;
	}
	LARGE_INTEGER val;
	if(!QueryPerformanceCounter(&val))
		return (FXulong) GetTickCount() * 1000000;
	return (FXulong) (val.QuadPart/scalefactor);
#endif
#ifdef USE_POSIX
	// It's only as good as POSIX can make it, which is pretty good on most
	// x86 based platforms which have a RTDSC
	struct timeval tv;
	FXERRHOS(gettimeofday((::timeval *) &tv, 0));
	return ((FXulong) tv.tv_sec*1000000000LL)+tv.tv_usec*1000;
#endif
}

FXuint FXProcess::id()
{
#ifdef USE_WINAPI
	return GetCurrentProcessId();
#endif
#ifdef USE_POSIX
	return getpid();
#endif
}

FXuint FXProcess::noOfProcessors()
{
#ifdef USE_WINAPI
	SYSTEM_INFO si={0};
	GetSystemInfo(&si);
	return si.dwNumberOfProcessors;
#endif
#ifdef USE_POSIX
#ifdef __linux__
	// I can't believe that reading in a text file is the way to do this! :(
	static FXuint processors;
	if(processors) return processors;
	FILE *ih=0;
	ih=fopen("/proc/cpuinfo", "rt");
	if(!ih) FXERRGOS(errno, 0);
	char buffer[4096];
	int len=fread(buffer, 1, 4096, ih);
	buffer[len]=0;
	fclose(ih);
	FXString cpuinfo(buffer);
	processors=cpuinfo.contains("processor");
	return processors;
#endif
#ifdef __FreeBSD__
	int command[2]={CTL_HW, HW_NCPU };
	int processors;
	size_t processorslen=sizeof(processors);
	FXERRHOS(sysctl(command, 2, &processors, &processorslen, NULL, 0));
	return processors;
#endif
#endif
}

FXuint FXProcess::pageSize()
{
#ifdef USE_WINAPI
	SYSTEM_INFO si={0};
	GetSystemInfo(&si);
	return si.dwPageSize;
#endif
#ifdef USE_POSIX
	return ::getpagesize();
#endif
}

QValueList<FXProcess::MappedFileInfo> FXProcess::mappedFiles()
{
	QValueList<MappedFileInfo> list;
	MappedFileInfo bi;
#ifdef USE_WINAPI
	TCHAR rawbuffer[4096];
	MEMORY_BASIC_INFORMATION mbi;
	HMODULE handles[1024];
	DWORD needed;
	FXERRHWIN(EnumProcessModules(GetCurrentProcess(), handles, sizeof(handles), &needed));
	for(DWORD n=0; n<needed/sizeof(HMODULE); n++)
	{
		MODULEINFO mi={0};
		FXERRHWIN(GetModuleInformation(GetCurrentProcess(), handles[n], &mi, sizeof(mi)));
		bi.startaddr=(FXuval) mi.lpBaseOfDll;
		bi.length=(FXuval) mi.SizeOfImage;
		bi.endaddr=bi.startaddr+bi.length;
		VirtualQuery(mi.lpBaseOfDll, &mbi, sizeof(mbi));
		bi.read=(mbi.AllocationProtect & (PAGE_READONLY|PAGE_EXECUTE_READ))!=0;
		bi.write=(mbi.AllocationProtect & (PAGE_READWRITE|PAGE_EXECUTE_READWRITE))!=0;
		if(bi.write) bi.read=true;
		bi.execute=(mbi.AllocationProtect & (PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY))!=0;
		if(bi.execute) bi.read=true;
		bi.copyonwrite=(mbi.AllocationProtect & (PAGE_WRITECOPY|PAGE_EXECUTE_WRITECOPY))!=0;
		bi.offset=0;
		FXERRHWIN(GetModuleFileNameEx(GetCurrentProcess(), handles[n], rawbuffer, sizeof(rawbuffer)));
		bi.path=rawbuffer;
		list.append(bi);
	}
	// Unfortunately the above is only shared libraries and doesn't include mapped sections
	FXuval addr=0;
	while(VirtualQuery((LPVOID) addr, &mbi, sizeof(mbi)))
	{
		if(MEM_FREE!=mbi.State)
		{
			rawbuffer[GetMappedFileName(GetCurrentProcess(), mbi.AllocationBase, rawbuffer, sizeof(rawbuffer))]=0;
			if(rawbuffer[0])
			{
				bi.startaddr=addr;
				bi.length=(FXuval) mbi.RegionSize;
				bi.endaddr=bi.startaddr+bi.length;
				bi.read=(mbi.AllocationProtect & (PAGE_READONLY|PAGE_EXECUTE_READ))!=0;
				bi.write=(mbi.AllocationProtect & (PAGE_READWRITE|PAGE_EXECUTE_READWRITE))!=0;
				if(bi.write) bi.read=true;
				bi.execute=(mbi.AllocationProtect & (PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY))!=0;
				if(bi.execute) bi.read=true;
				bi.copyonwrite=(mbi.AllocationProtect & (PAGE_WRITECOPY|PAGE_EXECUTE_WRITECOPY))!=0;
				bi.offset=0;
				bi.path=rawbuffer;
				list.append(bi);
			}
		}
		addr+=mbi.RegionSize;
	}
	// Ok sort the list
	list.sort();
#endif
#ifdef USE_POSIX
#ifdef __linux__
	FXString procpath=FXString("/proc/%1/maps").arg(FXProcess::id());
	QFile fh(procpath, QFile::WantLightQFile());
	fh.open(IO_ReadOnly|IO_Translate);
	char rawbuffer[4096];
	while(fh.readLine(rawbuffer, sizeof(rawbuffer)))
	{	// Format is hexstart-_hexend_ rwxp hexofset dd:dd _inode     /path...
		unsigned long startaddr, endaddr, offset;
		char _r, _w, _x, _p;
		int t1, t2;
		unsigned int pid;
		char *path=rawbuffer+2048;
		int len=sscanf(rawbuffer, "%lx-%lx %c%c%c%c %lx %d:%d %u %s", &startaddr, &endaddr,
			&_r, &_w, &_x, &_p, &offset, &t1, &t2, &pid, path);
		bi.startaddr=(FXulong) startaddr;
		bi.endaddr=(FXulong) endaddr;
		bi.length=bi.endaddr-bi.startaddr;
		bi.read=('r'==_r);
		bi.write=('w'==_w);
		bi.execute=('x'==_x);
		bi.copyonwrite=('p'==_p);
		bi.offset=(FXulong) offset;
		bi.path=path;
		bi.path.trim();
		if(!list.empty())
		{	// Linux doesn't say RAM sections belong to DLL
			MappedFileInfo &bbi=list.back();
			if(bbi.endaddr==bi.startaddr && bi.path.empty()
				&& bi.read==bbi.read && bi.write==bbi.write && bi.execute==bbi.execute && bi.copyonwrite==bbi.copyonwrite)
			{	// Collapse
				bbi.endaddr=bi.endaddr;
				continue;
			}
		}
		list.append(bi);
	}
	fh.close();
#endif
#ifdef __FreeBSD__
	FXString procpath=FXString("/proc/%1/map").arg(FXProcess::id());
	// FreeBSD doesn't like incremental reads according to kernel sources
	char rawbuffer[131072 /* WARNING: This value derived by experimentation */ ];
	char *ptr=rawbuffer, *end;
	{
		QFile fh(procpath, QFile::WantLightQFile());
		if(!fh.exists()) // Probably /proc isn't mounted - let FXProcess take care of that
			return list;
		fh.open(IO_ReadOnly|IO_Translate);
		FXuval read=fh.readBlock(rawbuffer, sizeof(rawbuffer)-2);
		assert(read>0 && read<sizeof(rawbuffer)-2);
		end=ptr+read; end[0]=10; end[1]=0;
	}
	// Format is 0xstart 0xend resident privresident [0x]obj rwx refcnt shadcnt 0xflags NCOW|COW NNC|NC type path
	// (or from kernel sources) "0x%lx 0x%lx %d %d %p %s%s%s %d %d 0x%x %s %s %s %s\n"
	while(ptr<end)
	{	// sscanf seems to have thread safety issues on FreeBSD :( - so we parse manually
		long start, end, objptr; char r,w,x; int refcnt, shadcnt, flags;
		char cow[8], nc[8], type[32], *sp; bool ok;
		
		// Start
		assert('0'==ptr[0] && 'x'==ptr[1]);
		if((sp=strchr(ptr, ' '))) *sp=0; assert(sp);
		bi.startaddr=FXString(ptr+2, sp).toULong(&ok, 16); assert(ok);
		if(sp) ptr=sp+1;
		
		// End
		assert('0'==ptr[0] && 'x'==ptr[1]);
		if((sp=strchr(ptr, ' '))) *sp=0; assert(sp);
		bi.endaddr=FXString(ptr+2, sp).toULong(&ok, 16); assert(ok);
		if(sp) ptr=sp+1;
		
		bi.length=bi.endaddr-bi.startaddr;
		sp=strchr(ptr, ' '); assert(sp); if(sp) ptr=sp+1;	// resident
		sp=strchr(ptr, ' '); assert(sp); if(sp) ptr=sp+1;	// privresident
		sp=strchr(ptr, ' '); assert(sp); if(sp) ptr=sp+1;	// obj
		
		// rwx
		bi.read='r'==ptr[0];
		bi.write='w'==ptr[1];
		bi.execute='x'==ptr[2];
		ptr+=4;
		
		sp=strchr(ptr, ' '); assert(sp); if(sp) ptr=sp+1;	// refcnt
		sp=strchr(ptr, ' '); assert(sp); if(sp) ptr=sp+1;	// shadcnt
		sp=strchr(ptr, ' '); assert(sp); if(sp) ptr=sp+1;	// flags
		
		// COW
		//fxmessage("Processing %s\n", FXString(ptr, 32).text());
		assert(!strncmp("NCOW", ptr, 4) || !strncmp("COW", ptr, 3));
		bi.copyonwrite='C'==ptr[0];
		sp=strchr(ptr, ' '); assert(sp); if(sp) ptr=sp+1;
		
		sp=strchr(ptr, ' '); assert(sp); if(sp) ptr=sp+1;	// NNC
		sp=strchr(ptr, ' '); assert(sp); if(sp) ptr=sp+1;	// type
		bi.offset=0;
		char *nl=strchr(ptr, 10);
		if('-'==ptr[0])
			bi.path.truncate(0);
		else
			bi.path.assign(ptr, nl-ptr);
		list.append(bi);
		if(nl)
			ptr=nl+1;
		else
			break;
	}
	assert(end-ptr<2);
#endif
#endif
	return list;
}

const FXString &FXProcess::execpath()
{
	static FXString myexecpath;
	if(!myexecpath.empty()) return myexecpath;
#ifdef USE_WINAPI
	// Luckily we have a ready made API on Win32
	TCHAR buffer[4096];
	FXERRHWIN(GetModuleFileName(NULL, buffer, sizeof(buffer)/sizeof(TCHAR)));
	myexecpath=buffer;
#endif
#ifdef USE_POSIX
	/* This is a real pain in the hole to get right always. I tried looking up
	main() but that causes more problems so best I could come up with was a
	heuristic for each supported platform :( */
#ifdef __linux__
#if defined(__LP64__) || defined(_LP64) || (_MIPS_SZLONG == 64) || (__WORDSIZE == 64)
	static const FXuval addr=0x00400000;
#else
	static const FXuval addr=0x08000000;
#endif
#endif
#ifdef __FreeBSD__
	static const FXuval addr=0x08000000;
#endif
	FXuval diff=(FXuval) -1;
	QValueList<MappedFileInfo> list=mappedFiles();
	for(QValueList<MappedFileInfo>::iterator it=list.begin(); it!=list.end(); ++it)
	{
		if((*it).startaddr-addr<diff)
		{
			myexecpath=(*it).path;
			diff=(*it).startaddr-addr;
		}
	}
#endif
	return myexecpath;
}

FXString FXProcess::dllPath(void *_addr, void **dllstart, void **dllend)
{
	FXuval addr=(FXuval) _addr;
	QValueList<MappedFileInfo> list=mappedFiles();
	for(QValueList<MappedFileInfo>::iterator it=list.begin(); it!=list.end(); ++it)
	{
		MappedFileInfo &mfi=*it;
		if(addr>=mfi.startaddr && addr<mfi.endaddr)
		{
			if(dllstart) *dllstart=(void *) mfi.startaddr;
			if(dllend) *dllend=(void *) mfi.endaddr;
			return mfi.path;
		}
	}
	return FXString();
}

FXProcess::dllHandle FXProcess::dllLoad(const FXString &path)
{
	bool firstLoad=true;
	dllHandle h;
#ifdef USE_WINAPI
	/* Rather annoyingly, LoadLibraryEx() won't try file extensions for us like LoadLibrary()
	so we must do this ourselves */
	FXString path_=path;
	FXUnicodify<> path__(path_);
	// Need GetModuleHandleEx (>win2k only) to avoid race condition
	static BOOL (WINAPI *pGetModuleHandleEx)(DWORD dwFlags, LPCTSTR lpModuleName, HMODULE* phModule);
	if(!pGetModuleHandleEx)
	{
		pGetModuleHandleEx=(BOOL (WINAPI *)(DWORD dwFlags, LPCTSTR lpModuleName, HMODULE* phModule))
			GetProcAddress(
#ifdef UNICODE
			GetModuleHandle(L"kernel32"), "GetModuleHandleExW");
#else
			GetModuleHandle("kernel32"), "GetModuleHandleExA");
#endif
	}
	if((pGetModuleHandleEx && (firstLoad=!pGetModuleHandleEx(0, path__.buffer(), (HMODULE *) &h.h)))
		|| (!pGetModuleHandleEx && (firstLoad=!GetModuleHandle(path__.buffer()), true)))
	{	// Failed to inc ref count of already loaded module
		if(!(h.h=(void *) LoadLibraryEx(path__.buffer(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH)))
		{
			if(ERROR_MOD_NOT_FOUND!=GetLastError()) { FXERRHWIN(0); }
			path_=path+".dll";
			if(!(h.h=(void *) LoadLibraryEx(FXUnicodify<>(path_).buffer(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH)))
			{
				if(ERROR_MOD_NOT_FOUND!=GetLastError()) { FXERRHWIN(0); }
				path_=path+".exe";
				if(!(h.h=(void *) LoadLibraryEx(FXUnicodify<>(path_).buffer(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH)))
				{
					FXERRHWIN(0);
				}
			}
		}
	}
#endif
#ifdef USE_POSIX
	/* We have a minor problem in that dlopen() doesn't search the
	same way as LoadLibrary() and thus code which works on NT doesn't
	on POSIX. Thus we emulate the Windows behaviour. Another minor
	problem is that libraries can have "lib" on their front :( */
	FXString path_=path;
	bool hasLib=path_.left(3)=="lib", hasSO=path_.right(3)==".so";
	while(!FXStat::exists(path_) && !FXPath::isAbsolute(path_))
	{
		if(!hasSO)  { path_+=".so";			if(FXStat::exists(path_)) break; }
		if(!hasLib) { path_="lib"+path_;	if(FXStat::exists(path_)) break; }
		// Try directory where my executable is
		FXString inexecpath=FXPath::directory(execpath())+PATHSEPSTRING;
		path_=path;
		if(FXStat::exists(inexecpath+path_)) { path_=inexecpath+path_; break; }
		if(!hasSO)  { path_+=".so";			if(FXStat::exists(inexecpath+path_)) { path_=inexecpath+path_; break; } }
		if(!hasLib) { path_="lib"+path_;	if(FXStat::exists(inexecpath+path_)) { path_=inexecpath+path_; break; } }
		// Try current directory
		inexecpath="." PATHSEPSTRING;
		path_=path;
		if(FXStat::exists(inexecpath+path_)) { path_=inexecpath+path_; break; }
		if(!hasSO)  { path_+=".so";			if(FXStat::exists(inexecpath+path_)) { path_=inexecpath+path_; break; } }
		if(!hasLib) { path_="lib"+path_;	if(FXStat::exists(inexecpath+path_)) { path_=inexecpath+path_; break; } }
		break;
	}
	path_=FXPath::absolute(path_);
	QValueList<MappedFileInfo> list=mappedFiles();
	for(QValueList<MappedFileInfo>::iterator it=list.begin(); it!=list.end(); ++it)
	{
		MappedFileInfo &mfi=*it;
		if(mfi.path==path_)
		{
			firstLoad=false;
			break;
		}
	}
#ifdef DEBUG
	fxmessage("dlopening %s (firstLoad=%d) ...\n", path_.text(), firstLoad);
#endif
	// Ignore if it's already loaded
	if(firstLoad)
	{
		if(!(h.h=dlopen(path_.text(), RTLD_NOW | RTLD_GLOBAL)))
		{
			FXERRG(FXString(dlerror()), FXPROCESS_DLERROR, 0);
		}
	}
#endif
	if(firstLoad)
	{
		QBuffer txtholder;
		txtholder.open(IO_ReadWrite);
		FXStream stxtholder(&txtholder);
		myprocess->runPendingStaticInits(myprocess->p->argscopy.argc, myprocess->p->argscopy.argv, stxtholder);
	}
	return h;
}

void *FXProcess::dllResolveBase(const FXProcess::dllHandle &h, const char *apiname)
{
	void *addr;
#ifdef USE_WINAPI
	FXERRHWIN(addr=GetProcAddress((HMODULE) h.h, apiname));
#endif
#ifdef USE_POSIX
	dlerror(); // clear the state
	if(!(addr=dlsym(h.h, apiname)))
	{
		FXERRG(FXString(dlerror()), FXPROCESS_DLERROR, 0);
	}
#endif
	return addr;
}

void FXProcess::dllUnload(FXProcess::dllHandle &h)
{
#ifdef USE_WINAPI
	FXERRHWIN(FreeLibrary((HMODULE) h.h));
#endif
#ifdef USE_POSIX
	if(dlclose(h.h))
	{
		FXERRG(FXString(dlerror()), FXPROCESS_DLERROR, 0);
	}
#endif
	h.h=0;
}

FXString FXProcess::hostOS()
{
#ifdef USE_WINAPI
	LPVOID execbase;
	HANDLE myprocess=GetCurrentProcess();
	// Not a lot of docs on how to do this :(
	HMODULE list[4096];
	DWORD needed;
	FXERRHWIN(EnumProcessModules(myprocess, list, 4096, &needed));
	execbase=list[0];	// First is always exe
	FXString desc;

	PIMAGE_NT_HEADERS headers;
	headers=ImageNtHeader(execbase);
	if     (IMAGE_FILE_MACHINE_I386   ==headers->FileHeader.Machine) desc="Win32/" ARCHITECTURE;
	else if(IMAGE_FILE_MACHINE_ALPHA  ==headers->FileHeader.Machine) desc="Win32/Alpha";
	else if(IMAGE_FILE_MACHINE_POWERPC==headers->FileHeader.Machine) desc="Win32/PowerPC";
#ifdef IMAGE_FILE_MACHINE_IA64
	else if(IMAGE_FILE_MACHINE_IA64   ==headers->FileHeader.Machine) desc="Win64/IA64";
#endif
#ifdef IMAGE_FILE_MACHINE_AMD64
	else if(IMAGE_FILE_MACHINE_AMD64  ==headers->FileHeader.Machine) desc="Win64/AMD64";
#endif
	else desc="unknown";
	return desc;
#endif
#ifdef USE_POSIX
#ifdef __linux__
	static FXString desc;
	if(!desc.empty()) return desc;
	
	FILE *ih=0;
	ih=fopen("/proc/sys/kernel/ostype", "rt");
	if(!ih) FXERRGOS(errno, 0);
	char buffer[64];
	int len=fread(buffer, 1, 64, ih);
	buffer[len-1]=0;
	fclose(ih);
	desc=buffer;
	return desc+"/"+ARCHITECTURE;
#endif
#ifdef __FreeBSD__
	int command[2]={ CTL_KERN, KERN_OSTYPE };
	char type[256];
	size_t typelen=sizeof(type);
	FXERRHOS(sysctl(command, 2, type, &typelen, NULL, 0));
	return FXString(type)+"/"+ARCHITECTURE;
#endif
#endif
}

FXString FXProcess::hostOSDescription()
{
#ifdef USE_WINAPI
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	FXString desc;
#ifdef PROCESSOR_ARCHITECTURE_IA64
	if(si.wProcessorArchitecture>=PROCESSOR_ARCHITECTURE_IA64)
		desc="Win64 (";
	else
#endif
		desc="Win32 (";
	OSVERSIONINFO ovi={ sizeof(OSVERSIONINFO) };
	FXERRHWIN(GetVersionEx(&ovi));
	if     (VER_PLATFORM_WIN32s       ==ovi.dwPlatformId) desc+="DOS";
	else if(VER_PLATFORM_WIN32_WINDOWS==ovi.dwPlatformId) desc+="9x";
	else if(VER_PLATFORM_WIN32_NT     ==ovi.dwPlatformId) desc+="NT";
	else desc+="unknown";
	desc+=" [%1] kernel) version %2.%3, ";
	desc.arg(ovi.szCSDVersion);
	desc.arg((FXint) ovi.dwMajorVersion).arg((FXint) ovi.dwMinorVersion);

	if     (PROCESSOR_ARCHITECTURE_INTEL==si.wProcessorArchitecture) desc+=ARCHITECTURE;
	else if(PROCESSOR_ARCHITECTURE_ALPHA==si.wProcessorArchitecture) desc+="Alpha";
	else if(PROCESSOR_ARCHITECTURE_PPC  ==si.wProcessorArchitecture) desc+="PowerPC";
#ifdef PROCESSOR_ARCHITECTURE_IA64
	else if(PROCESSOR_ARCHITECTURE_IA64 ==si.wProcessorArchitecture) desc+="IA64";
#endif
#ifdef PROCESSOR_ARCHITECTURE_AMD64
	else if(PROCESSOR_ARCHITECTURE_AMD64==si.wProcessorArchitecture) desc+="AMD64";
#endif
	else desc+="unknown";
	desc+=" architecture";
	return desc;
#endif
#ifdef USE_POSIX
	FXString desc="POSIX.2 (";
#ifdef __linux__
	FILE *ih=0;
	char buffer[256];
	int len;
	
	ih=fopen("/proc/sys/kernel/ostype", "rt");
	if(!ih) FXERRGOS(errno, 0);
	len=fread(buffer, 1, 64, ih);
	buffer[len-1]=0;
	fclose(ih);
	desc+=buffer;
	ih=fopen("/proc/sys/kernel/osrelease", "rt");
	if(!ih) FXERRGOS(errno, 0);
	len=fread(buffer, 1, 64, ih);
	buffer[len-1]=0;
	fclose(ih);
	desc+=" ["; desc+=buffer;
#endif
#ifdef __FreeBSD__
	int command[2]={ CTL_KERN, KERN_OSTYPE };
	char buffer[256];
	size_t bufflen=sizeof(buffer);
	FXERRHOS(sysctl(command, 2, buffer, &bufflen, NULL, 0));
	desc+=buffer;
	command[1]=KERN_OSRELEASE; bufflen=sizeof(buffer);
	FXERRHOS(sysctl(command, 2, buffer, &bufflen, NULL, 0));
	desc+=" ["; desc+=buffer;
#endif
	desc+="] kernel) version %1";
	desc.arg((FXulong) sysconf(_SC_2_VERSION));
	desc+=", " ARCHITECTURE " architecture";
	return desc;
#endif
}

FXfloat FXProcess::hostOSProcessorLoad()
{
    if(myprocess->p->overrides.processor>=0) return myprocess->p->overrides.processor;
#ifdef USE_WINAPI
	return 0;
#endif
#ifdef USE_POSIX
	return 0;
#endif
}

#ifdef USE_POSIX
#ifdef __linux__
static inline FXulong readMemVal(const FXString &buffer)
{
	FXulong ret=buffer.toULong();
	FXint sp=buffer.find(' ');
	switch(buffer[sp+1])
	{
	case 'k':
		ret*=1024;
		break;
	case 'm':
		ret*=1024*1024;
		break;
	default:
		assert(0);
	}
	return ret;
}
#endif
#endif
FXfloat FXProcess::hostOSMemoryLoad(FXuval *totalPhysMem)
{
	if(!myprocess) return 0;
    if(myprocess->p->overrides.memory>=0) return myprocess->p->overrides.memory;
	static QMutex lock;
	static FXuint lastcheck;
	static FXfloat lastret;
	QMtxHold h(lock);
	FXuint now=FXProcess::getMsCount();
	if((now-lastcheck)<60*1000/* one min */) return lastret;
#ifdef USE_WINAPI
	MEMORYSTATUSEX ms={ sizeof(MEMORYSTATUSEX) };
	FXERRHWIN(GlobalMemoryStatusEx(&ms));
	// ullTotalPhys    = physical mem size
	// ullTotalPageFile= swap file size + physical mem size
	// ullAvailPageFile= swap file free + physical mem free
	//DWORDLONG swapSize=ms.ullTotalPageFile-ms.ullTotalPhys;
	// Unfortunately, NT doesn't include mapped files in its virtual memory calculations
	// so we must reduce the value by some amount and hope for the best
	DWORDLONG committed=ms.ullTotalPageFile-ms.ullAvailPageFile;
	// committed       = total address space allocated
	// Kernel takes about 50Mb of physical, I guess a further 10% in disc buffers on NTFS systems
	// Therefore after virtual address space allocation breaks 85% of physical memory we're full
	lastret=(FXfloat)(committed/(ms.ullTotalPhys*0.85));
	if(totalPhysMem)
	{
		*totalPhysMem=(FXuval) ms.ullTotalPhys;
		if(*totalPhysMem<ms.ullTotalPhys) *totalPhysMem=0xffffffff;		// work around Win32 under Win64 problem
	}
#endif
#ifdef USE_POSIX
#ifdef __linux__
	QFile fh("/proc/meminfo", QFile::WantLightQFile());
	fh.open(IO_ReadOnly|IO_Translate);
	char rawbuffer[4096];
	rawbuffer[fh.readBlock(rawbuffer, sizeof(rawbuffer)+1)]=0;
	fh.close();
	FXString buffer(rawbuffer);
	/* Format is:
	...
	MemTotal:       257076 kB
	MemFree:          4652 kB
	...
	Buffers:         13860 kB
	...
	Cached:          97116 kB
	...
	SwapTotal:      523992 kB
	SwapFree:       523992 kB
	*/
	/* Buffers appears to be part of Cached on Linux. Cached appears to be memory
	maps - therefore real application data in physical memory is Used-Buffers.
	Therefore total virtual address space allocation is that plus swap file used */
	FXulong totalPhys, physFree, Buffers, totalSwap, swapFree;
	FXint memidx, buffersidx, swapidx, sp;
	memidx=buffer.find("MemTotal:"); buffersidx=buffer.find("Buffers:");
	swapidx=buffer.find("SwapTotal:");
	assert(memidx>=0); assert(buffersidx>=0); assert(swapidx>=0);

	memidx+=9; while(' '==buffer[memidx]) memidx++; sp=buffer.find('\n', memidx);
	totalPhys=readMemVal(buffer.mid(memidx, sp-memidx));
	memidx=sp+1; assert(!strncmp("MemFree:", &buffer[memidx], 8)); memidx+=8;
	while(' '==buffer[memidx]) memidx++; sp=buffer.find('\n', memidx);
	physFree=readMemVal(buffer.mid(memidx, sp-memidx));

	buffersidx+=8; while(' '==buffer[buffersidx]) buffersidx++; sp=buffer.find('\n', buffersidx);
	Buffers=readMemVal(buffer.mid(buffersidx, sp-buffersidx));

	swapidx+=10; while(' '==buffer[swapidx]) swapidx++; sp=buffer.find('\n', swapidx);
	totalSwap=readMemVal(buffer.mid(swapidx, sp-swapidx));
	swapidx=sp+1; assert(!strncmp("SwapFree:", &buffer[swapidx], 9)); swapidx+=9;
	while(' '==buffer[swapidx]) swapidx++; sp=buffer.find('\n', swapidx);
	swapFree=readMemVal(buffer.mid(swapidx, sp-swapidx));

	// Since we get more info with Linux, this value is probably near spot on
	lastret=static_cast<FXfloat>((totalPhys-physFree)-Buffers+(totalSwap-swapFree))/totalPhys;
	if(totalPhysMem)
	{
		*totalPhysMem=(FXuval) totalPhys;
		if(*totalPhysMem<totalPhys) *totalPhysMem=0xffffffff;
	}
#endif
#ifdef __FreeBSD__
	// Ahh, this is the perfect solution! :)
	int command1[2]={ CTL_VM, 1 /*VM_TOTAL*/ }, command2[2]={ CTL_HW, HW_PHYSMEM };
	char vm[256]; long pm;
	size_t vmlen=sizeof(vm), pmlen=sizeof(pm);
	FXERRHOS(sysctl(command1, 2, vm, &vmlen, NULL, 0));
	FXERRHOS(sysctl(command2, 2, &pm, &pmlen, NULL, 0));
	struct vmtotal *vt=(struct vmtotal *) vm;
	// It's not documented that t_avm is in pages
	lastret=static_cast<FXfloat>(((FXuval) vt->t_avm)*getpagesize())/pm;
	if(totalPhysMem)
		*totalPhysMem=(FXuval) pm;
#endif
#endif
	lastcheck=now;
	return lastret;
}

FXfloat FXProcess::hostOSDiscIOLoad(const FXString &path)
{
    if(myprocess->p->overrides.discio>=0) return myprocess->p->overrides.discio;
#ifdef USE_WINAPI
	// Simply PhysicalDisk/% Disk Time
	return 0;
#endif
#ifdef USE_POSIX
	return 0;
#endif
}

void FXProcess::overrideFreeResources(FXfloat memory, FXfloat processor, FXfloat discio)
{
	QMtxHold h(myprocess->p);
    myprocess->p->overrides.memory=memory;
    myprocess->p->overrides.processor=processor;
    myprocess->p->overrides.discio=discio;
}

FXuval FXProcess::virtualAddrSpaceLeft(FXuval chunk)
{
	FXuval pagesize=pageSize();
	if(chunk<pagesize) chunk=pagesize;
	/* Some notes:
	Linux 2.4		Linux 2.6 x86	Linux 2.6 x64			FreeBSD 5.2		NT 5.0
	0xbfffffff		0xffffffff		0xffffffffffffffff		0xbfffffff		0x7fffffff
	|| stacks		|| stacks		|| stacks				|| stacks		|| DLLs
	\/				\/				\/						\/				\/
	/\ anon maps	0xf7050000		0x0000007fc0000000		/\ anon maps	/\ anon maps
	|| DLLs			|| anon maps	|| anon maps			|| DLLs			|| sbrk()
	0x40000000		\/				\/						0x28240000		0x00300000
	/\				/\				0x0000002a95556000		/\				0x00130000
	|| sbrk()		|| sbrk()		|| sbrk()				|| sbrk()		|| stacks
	0x08000000		0x08000000		0x0000000000400000		0x08000000		\/

	* On Linux 2.6 DLLs live between 0x00111000-0x00f18000 and 0x0350d000-0x03d0000
	growing upwars. Furthermore anon maps are always high growing downards. Some
	144Mb is reserved for stacks.
	* On NT 5.0 main program image usually lives at 0x00400000. Between
	0x00130000-0x00300000 appear to get allocated to system DLLs and resources.
*/
#ifdef USE_WINAPI
	void *map1, *map2;
	map1=VirtualAlloc(NULL, chunk, MEM_RESERVE, PAGE_NOACCESS);
	if(!map1) return 0;
	map2=VirtualAlloc(NULL, chunk, MEM_RESERVE|MEM_TOP_DOWN, PAGE_NOACCESS);
	VirtualFree(map1, 0, MEM_RELEASE);
	if(!map2) return 0;
	VirtualFree(map2, 0, MEM_RELEASE);
	return chunk+(FXuval) map2-(FXuval) map1;
#endif
#ifdef USE_POSIX
	FXuval stackaddr; stackaddr=((FXuval) &stackaddr) & ~(0x10000-1);	// Round down to Mb boundary
	// On all supported POSIX implementations stacks live at the highest address
	// but on Linux 2.6 the end of mapping space is 144Mb lower
	int flags=MAP_PRIVATE|MAP_NORESERVE;	// Don't actually allocate memory
	// MAP_AUTORESRV;
#ifdef __linux__
	flags|=MAP_ANONYMOUS;
#endif
#ifdef __FreeBSD__
	flags|=MAP_ANON;
#endif
	// Some POSIX systems allocate memory downwards rather than upwards (eg; x86 Linux 2.6)
	void *map1=::mmap(0, chunk, PROT_NONE, flags, -1, 0);
	void *map2=::mmap(0, chunk, PROT_NONE, flags, -1, 0);
	if(map1) ::munmap(map1, chunk);
	if(map2) ::munmap(map2, chunk);
	if(!map1 || !map2) return 0;
	if(map2>map1)
	{	// Maps grow upwards
		return stackaddr-(FXuval) map1;
	}
	else
	{	// Maps grow downwards
		return ((FXuval)map1)+chunk-(FXuval)sbrk(0);
	}
#endif
}

#ifdef USE_POSIX
static inline bool isTerminator(const char p) throw()
{
	return !p || ','==p || ' '==p || '\t'==p;
}
static inline FXProcess::MountablePartition readfstabentry(const char *&t)
{
	// Form is /dev/hda6            /                    reiserfs   acl,user_xattr        1 1
	FXProcess::MountablePartition ret;
	if('#'!=*t)
	{
		char devname[64], mountpoint[260], fstype[32], opts[64];
		sscanf(t, "%s %s %s %s", devname, mountpoint, fstype, opts);
		if(*devname)
			ret.name=devname;
		if(*mountpoint)
			ret.location=mountpoint;
		if(*fstype)
			ret.fstype=fstype;
		if(*opts)
		{
			const char *user=strstr(opts, "user"), *ro=strstr(opts, "ro");
			ret.mounted=false;
			ret.mountable=(!getuid() || (user && isTerminator(user[-1]) && isTerminator(user[4])));
			ret.readWrite=(ro && isTerminator(ro[-1]) && isTerminator(ro[2]));
		}
	}
	for(; *t && 10!=*t; ++t);
	while(10==*t) ++t;
	return ret;
}
#endif
QValueList<FXProcess::MountablePartition> FXProcess::mountablePartitions()
{
	QValueList<MountablePartition> ret;
#ifdef USE_WINAPI
	HANDLE fvh;
	TCHAR volume[MAX_PATH];
	FXERRHWIN(INVALID_HANDLE_VALUE!=(fvh=FindFirstVolume(volume, sizeof(volume)/sizeof(TCHAR))));
	FXRBOp unfvh=FXRBFunc(&FindVolumeClose, fvh);
	MountablePartition mp;
	mp.mountable=(FXACLEntity::currentUser()==FXACLEntity::root());
	do
	{
		DWORD volumeflags;
		TCHAR fstype[64];
		mp.name=volume;
		if(!GetVolumeInformation(volume, NULL, 0, NULL, NULL, &volumeflags, fstype, sizeof(fstype)/sizeof(TCHAR)))
		{
			if(ERROR_NOT_READY!=GetLastError())
				FXERRHWIN(0);
			// Unreadable, so assume it's removable
			volumeflags=0;
			fstype[0]=0;
		}
		mp.fstype=fstype;
		mp.readWrite=!(volumeflags & FILE_READ_ONLY_VOLUME);

		HANDLE fmph;
		TCHAR mountpoint[MAX_PATH];
		if(INVALID_HANDLE_VALUE==(fmph=FindFirstVolumeMountPoint(volume, mountpoint, sizeof(mountpoint)/sizeof(TCHAR))))
		{	// Not mounted on any path, or we are not administrator
			mp.location=FXString::nullStr();
			mp.mounted=false;
			mp.driveLetter=0;
			ret.push_back(mp);
		}
		else
		{
			FXRBOp unfmph=FXRBFunc(&FindVolumeMountPointClose, fmph);
			do
			{
				mp.location=mountpoint;
				mp.mounted=true;
				if(3==mp.location.length())
					mp.driveLetter=mp.location[0];
				else
					mp.driveLetter=0;
				ret.push_back(mp);
			} while(FindNextVolumeMountPoint(fmph, mountpoint, sizeof(mountpoint)/sizeof(TCHAR)));
			FXERRHWIN(ERROR_NO_MORE_FILES==GetLastError());
		}
	} while(FindNextVolume(fvh, volume, sizeof(volume)/sizeof(TCHAR)));
	FXERRHWIN(ERROR_NO_MORE_FILES==GetLastError());

	// Now rather oddly the above code does NOT enumerate MS-DOS mount points ie; the most common type
	DWORD drives=GetLogicalDrives();
	TCHAR drive[4];
	drive[0]='A'; drive[1]=':'; drive[2]='\\'; drive[3]=0;
	for(; drives; ++drive[0], drives>>=1)
	{
		if(drives & 1)
		{
			TCHAR volname[MAX_PATH];
			FXERRHWIN(GetVolumeNameForVolumeMountPoint(drive, volname, sizeof(volname)/sizeof(TCHAR)));
			bool found=false;
			for(QValueList<MountablePartition>::iterator it=ret.begin(); it!=ret.end(); ++it)
			{
				if(!compare(it->name, volname))
				{
					it->location=drive;
					it->mounted=true;
					it->driveLetter=(char) drive[0];
					found=true;
					break;
				}
			}
			if(!found)
			{
				mp.name=volname;
				mp.location=drive;
				mp.fstype=FXString::nullStr();
				mp.mounted=true;
				mp.readWrite=true;
				mp.driveLetter=(char) drive[0];
				ret.push_back(mp);
			}
		}
	}
	return ret;
#endif
#ifdef USE_POSIX
	FXString fstab, mtab;
	{
		QFile l("/etc/fstab", QFile::WantLightQFile());
		l.open(IO_ReadOnly);
		fstab.length((FXuint) l.size());
		l.readBlock((char *) fstab.text(), fstab.length());
		// FreeBSD doesn't maintain /etc/mtab, and doesn't appear to have an equivalent :(
#ifndef __FreeBSD__
		QFile m("/etc/mtab", QFile::WantLightQFile());
		m.open(IO_ReadOnly);
		mtab.length((FXuint) m.size());
		m.readBlock((char *) mtab.text(), mtab.length());
#endif
	}
	const char *fstabp=fstab.text(), *mtabp=mtab.text();
	MountablePartition mp;
	while(*fstabp)
	{
		mp=readfstabentry(fstabp);
		if(!mp.name.empty())
			ret.push_back(mp);
	}
#ifndef __FreeBSD__
	while(*mtabp)
	{
		bool found=false;
		mp=readfstabentry(mtabp);
		for(QValueList<MountablePartition>::iterator it=ret.begin(); it!=ret.end(); ++it)
		{
			if(it->name==mp.name)
			{
				it->mounted=true;
				it->readWrite=mp.readWrite;
				found=true;
				break;
			}
		}
		if(!found)
			ret.push_back(mp);
	}
#endif
	// Attempt to guess drive letters
	char driveLetter='C';
	for(QValueList<MountablePartition>::iterator it=ret.begin(); it!=ret.end(); ++it)
	{
		MountablePartition &mp=*it;
		mp.driveLetter=0;
#ifdef __FreeBSD__
		mp.mounted=true;	// Mark all as mounted on FreeBSD
#endif
		if(!comparecase(mp.fstype, "vfat") || !comparecase(mp.fstype, "msdosfs") || !comparecase(mp.fstype, "ntfs"))
		{
			FXString location(mp.location); location.upper();
			if(-1!=location.find("WIN"))
				mp.driveLetter=driveLetter++;
			for(const char *t=location.text(); *t; t++)
			{
				if(' '==*t || '/'==*t || '('==*t)
				{
					if(t[1]>='C' && t[1]<='Z' && (!t[2] || '/'==t[2] || ' '==t[2] || ')'==t[2]))
					{
						mp.driveLetter=t[1];
						if(t[1]>=driveLetter) driveLetter=t[1]+1;
						break;
					}
				}
			}
		}
	}
	return ret;
#endif
}
void FXProcess::mountPartition(const FXString &partitionName, const FXString &location, const FXString &fstype, bool readWrite)
{
#ifdef USE_WINAPI
	TCHAR volname[MAX_PATH];
	FXERRHWIN(GetVolumeNameForVolumeMountPoint(FXUnicodify<>(partitionName).buffer(), volname, sizeof(volname)/sizeof(TCHAR)));
	FXERRHWIN(SetVolumeMountPoint(FXUnicodify<>(location).buffer(), volname));
#endif
#ifdef USE_POSIX
#ifdef __linux__
	unsigned long flags=readWrite ? 0 : MS_RDONLY;
	FXERRHOS(mount(partitionName.text(), location.text(), fstype.text(), flags, "defaults"));
#endif
#ifdef __FreeBSD__
	int flags=readWrite ? 0 : MNT_RDONLY;
	FXERRHOS(mount(fstype.text(), location.text(), flags, (void *) "defaults"));
#endif
#endif
}
void FXProcess::unmountPartition(const FXString &location)
{
#ifdef USE_WINAPI
	FXERRHWIN(DeleteVolumeMountPoint(FXUnicodify<>(location).buffer()));
#endif
#ifdef USE_POSIX
#ifdef __linux__
	FXERRHOS(umount(location.text()));
#endif
#ifdef __FreeBSD__
	FXERRHOS(unmount(location.text(), 0));
#endif
#endif
}



QThreadPool &FXProcess::threadPool()
{
	if(!myprocess->p->threadpool)
	{
		QMtxHold h(myprocess->p);
		if(!myprocess->p->threadpool)
		{
			FXERRHM(myprocess->p->threadpool=new QThreadPool(FXPROCESS_POOLTHREADS));
		}
	}
	return *myprocess->p->threadpool;
}

FXProcess::UserHandedness FXProcess::userHandedness()
{
	QMtxHold h(myprocess->p);
	if(UNKNOWN_HANDED==myprocess->p->handedness)
	{
#ifdef USE_WINAPI
		HKEY regkey;
		FXERRHWIN(ERROR_SUCCESS==RegOpenKeyEx(HKEY_CURRENT_USER, FXUnicodify<>(FXString("Control Panel\\Mouse")).buffer(),
			0, KEY_READ, &regkey));
		FXRBOp unregkey=FXRBFunc(RegCloseKey, regkey);
		DWORD type, len=16;
		char buffer[16];
		FXERRHWIN(ERROR_SUCCESS==RegQueryValueEx(regkey, FXUnicodify<>(FXString("SwapMouseButtons")).buffer(), NULL,
			&type, (LPBYTE) buffer, &len));
		myprocess->p->handedness='1'==buffer[0] ? LEFT_HANDED : RIGHT_HANDED;
#endif
#ifdef USE_POSIX
		// Hard coded for now
		myprocess->p->handedness=RIGHT_HANDED;
#endif
	}
	return myprocess->p->handedness;
}
void FXProcess::overrideUserHandedness(FXProcess::UserHandedness val)
{
	myprocess->p->handedness=val;
}

FXuint FXProcess::screenScale(FXuint value)
{
	return (myprocess->p->screenScale*value)/100;
}
void FXProcess::overrideScreenScale(FXuint percent)
{
	myprocess->p->screenScale=percent;
}
void FXProcess::overrideScreenSize(FXint w, FXint h)
{
	myprocess->p->maxScreenWidth=w;
	myprocess->p->maxScreenHeight=h;
}

void FXProcess::addFatalExitUpcall(FXProcess::FatalExitUpcallSpec upcallv)
{
	FatalExitUpcallSpec *cu;
	FXERRHM(cu=new FatalExitUpcallSpec(upcallv));
	FXRBOp unnew=FXRBNew(cu);
	if(!exitupcalls)
	{
		FXERRHM(exitupcalls=new ExitUpcalls);
	}
	QMtxHold h(exitupcalls->lock);
	exitupcalls->upcalls.append(cu);
	unnew.dismiss();
}
bool FXProcess::removeFatalExitUpcall(FXProcess::FatalExitUpcallSpec upcallv)
{
	QMtxHold h(exitupcalls->lock);
	FatalExitUpcallSpec *cu;
	for(QPtrListIterator<FatalExitUpcallSpec> it(exitupcalls->upcalls); (cu=it.current()); ++it)
	{
		if(*cu==upcallv)
		{
			exitupcalls->upcalls.removeByIter(it);
			return true;
		}
	}
	return false;
}



static struct LockedMem : public QMutex
{
	FXuval pageSizeM1;
	struct MemLockEntry
	{
		void *addr;
		FXuval len;
		void *startp, *endp;
		MemLockEntry(void *_addr, FXuval _len, FXuval _startp, FXuval _endp)
			: addr(_addr), len(_len), startp((void *) _startp), endp((void *) _endp) { }
	};
	QValueList<MemLockEntry> lockedRegions;
	QSortedList<void> lockedPages;
	LockedMem() : pageSizeM1(FXProcess::pageSize()-1) { }
} *lockedMem;

QMemArray<void *> FXProcess::lockedPages()
{
	QMemArray<void *> ret;
	if(!lockedMem) return ret;
	QMtxHold h(lockedMem);
	void *page;
	for(QSortedListIterator<void> it(lockedMem->lockedPages); (page=it.current()); ++it)
	{
		FXuint size=ret.size();
		ret.resize(size+1);
		ret[size]=page;
	}
	return ret;
}

void FXProcess::int_addStaticInit(FXProcess_StaticInitBase *o)
{
	if(!SIlist)
	{
		FXERRHM(SIlist=new QList<FXProcess_StaticInitBase>);
	}
	SIlist->append(o);
}

void FXProcess::int_removeStaticInit(FXProcess_StaticInitBase *o)
{
	if(SIlist)
	{
		SIlist->removeRef(o);
		if(SIlist->isEmpty()) FXDELETE(SIlist);
	}
}

void FXProcess::int_addStaticDepend(FXProcess_StaticDepend *d)
{
	if(!SIdepend)
	{
		FXERRHM(SIdepend=new QList<FXProcess_StaticDepend>);
	}
	SIdepend->append(d);
}

void *FXProcess::int_lockMem(void *addr, FXuval len)
{
	if(!lockedMem)
	{
		FXERRHM(lockedMem=new LockedMem);
	}
	LockedMem *p=lockedMem;
	QMtxHold h(p);
	for(QValueList<LockedMem::MemLockEntry>::iterator it=p->lockedRegions.begin(); ; ++it)
	{
		bool done=(p->lockedRegions.end()==it);
		const LockedMem::MemLockEntry &mle=!done ? *it : LockedMem::MemLockEntry(0, 0, 0, 0);
		if(done || it->addr>addr)
		{	// Found where to insert
			FXuval start=(FXuval) addr, end=((FXuval) addr)+len;
			start&=~p->pageSizeM1;
			end=(end+p->pageSizeM1) & ~p->pageSizeM1;
			QSortedListIterator<void> pit=p->lockedPages.findIter((void *) start);
			for(void *page=(void *) start; page<(void *) end; page=FXOFFSETPTR(page, p->pageSizeM1+1))
			{
				for(;pit.current() && pit.current()<page; ++pit);
				if(pit.current()!=page)
				{	// Lock (likely to fail if not root/administrator)
					static bool hadError;
					bool wasError=false;
#ifdef USE_WINAPI
					if(!VirtualLock(page, p->pageSizeM1)) wasError=true;
#endif
#ifdef USE_POSIX
					if(-1==mlock(page, p->pageSizeM1)) wasError=true;
#endif
					if(wasError && !hadError)
					{
						hadError=true;
						fxwarning("WARNING: Unable to lock memory pages, does this process have sufficient privilege?\n");
					}
					p->lockedPages.insert(page);
				}
			}
			it=p->lockedRegions.insert(it, LockedMem::MemLockEntry(addr, len, start, end));
			return &(*it);
		}
		if(done) break;
	}
	// Should never reach here
	assert(0);
	return 0;
}
void FXProcess::int_unlockMem(void *handle)
{
	LockedMem *p=lockedMem;
	QMtxHold h(p);
	void *prevend=0;
	for(QValueList<LockedMem::MemLockEntry>::iterator it=p->lockedRegions.begin(); it!=p->lockedRegions.end(); ++it)
	{
		LockedMem::MemLockEntry &mle=*it;
		if(&(*it)==handle)
		{
			QValueList<LockedMem::MemLockEntry>::iterator hit=it;
			bool atEnd=(++it==p->lockedRegions.end());
			const LockedMem::MemLockEntry &mlea=!atEnd ? *it : LockedMem::MemLockEntry(0, 0, 0, 0);
			void *start=mle.startp;
			if(!atEnd) for(; start<=prevend; start=FXOFFSETPTR(start, p->pageSizeM1+1));
			void *end=mle.endp;
			if(!atEnd) for(; end>=mlea.startp; end=FXOFFSETPTR(end, -((FXival) p->pageSizeM1+1)));
			if(end>=start)
			{
				FXuval len=(FXuval)end-(FXuval)start;
				len=(len+p->pageSizeM1) & ~p->pageSizeM1;
				for(FXuval n=0; n<len; n+=p->pageSizeM1+1)
				{	// Don't error check in case page has been deleted
#ifdef USE_WINAPI
					VirtualUnlock(FXOFFSETPTR(start, n), p->pageSizeM1);
#endif
#ifdef USE_POSIX
					munlock(FXOFFSETPTR(start, n), p->pageSizeM1);
#endif
				}
				QSortedListIterator<void> it=p->lockedPages.findIter(start);
				for(FXuval n=0; n<len; n+=p->pageSizeM1+1)
				{
					assert(it.current()==FXOFFSETPTR(start, n));
					QSortedListIterator<void> pit(it); ++it;
					p->lockedPages.removeByIter(pit);
				}
			}
			p->lockedRegions.erase(hit);
			if(p->lockedRegions.empty() && !myprocess)
			{
				h.unlock();
				FXDELETE(lockedMem);
			}
			return;
		}
		prevend=mle.endp;
	}
	// Should never reach here
	assert(0);
}

bool FXProcess::int_constrainScreen(FXint &x, FXint &y, FXint &w, FXint &h)
{
	bool ret=false;
	if(w>myprocess->p->maxScreenWidth)  { w=myprocess->p->maxScreenWidth; ret=true; }
	if(h>myprocess->p->maxScreenHeight) { h=myprocess->p->maxScreenHeight; ret=true; }
	if(x+w>myprocess->p->maxScreenWidth) { x=myprocess->p->maxScreenWidth-w; ret=true; }
	if(y+h>myprocess->p->maxScreenHeight) { y=myprocess->p->maxScreenHeight-h; ret=true; }
	return ret;
}

bool FXProcess::int_constrainScreen(FXint &w, FXint &h)
{
	bool ret=false;
	if(w>myprocess->p->maxScreenWidth)  { w=myprocess->p->maxScreenWidth; ret=true; }
	if(h>myprocess->p->maxScreenHeight) { h=myprocess->p->maxScreenHeight; ret=true; }
	return ret;
}

//*****************************************************************************************

FXProcess_StaticInitBase::~FXProcess_StaticInitBase()
{ FXEXCEPTIONDESTRUCT1 {
	QList<FXProcess_StaticInitBase> *deplist=(QList<FXProcess_StaticInitBase> *) dependencies;
	delete deplist;
	dependencies=0;
} FXEXCEPTIONDESTRUCT2; }

}
