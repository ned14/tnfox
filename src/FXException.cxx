/********************************************************************************
*                                                                               *
*                     E x c e p t i o n  H a n d l i n g                        *
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

#include <qptrlist.h>
#include <qptrvector.h>
#include "FXException.h"
#if !defined(FXEXCEPTION_DISABLESOURCEINFO)
#if defined(WIN32) && defined(_MSC_VER)
#define WIN32_LEAN_AND_MEAN
#include "Windows.h"
#include "Dbghelp.h"
#include "psapi.h"
#elif defined(__GNUC__)
#include "execinfo.h"
#endif
#endif
#include "QTrans.h"
#include "QThread.h"
#include "FXStream.h"
#include "FXRollback.h"
#include "FXMemoryPool.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "FXMemDbg.h"
#if defined(DEBUG) && defined(FXMEMDBG_H)
static const char *_fxmemdbg_current_file_ = __FILE__;
#endif

namespace FX {

struct FXDLLLOCAL FXExceptionPrivate
{
  int uniqueId;		// zero if exception invalid
  FXString message;
  FXuint code;
  FXuint flags;
  const char *srcfilename;
  int srclineno;
  FXulong threadId;
  mutable FXString *reporttxt;
  QPtrVector<FXExceptionPrivate> nestedlist;
#ifndef FXEXCEPTION_DISABLESOURCEINFO
#define FXEXCEPTION_STACKBACKTRACEDEPTH 16
	struct
	{
		void *pc;
		char module[64];
		char functname[256];
		char file[96];
		int lineno;
	} stack[FXEXCEPTION_STACKBACKTRACEDEPTH];
#endif
  int stacklevel;
  FXExceptionPrivate() : uniqueId(0), reporttxt(0), nestedlist(true) { }
};
//static TThreadLocalStorageBase exceptionCount;
static FXAtomicInt creationCnt;
struct FXException_TIB
{
	struct LevelEntry
	{
		const char *srcfile;						// Where the FXERRH_TRY block is in the source
		int lineno;
		int nestingCount;							// How many FXEXCEPTIONDESTRUCT's we are from this FXERRH_TRY block
		int uniqueId;								// Id of primary exception being thrown
		QPtrList<FXExceptionPrivate> currentExceptions;	// A list of known copies of the primary exception currently being thrown
		LevelEntry(const char *_srcfile, int _lineno) : srcfile(_srcfile), lineno(_lineno), nestingCount(0), uniqueId(0) { }
	};
	QPtrList<LevelEntry> stack;					// One of these exist per nesting level of FXERRH_TRY blocks
	struct GlobalErrorCreationCount
	{
		FXint master;
		FXint latch;
		FXint count;
		GlobalErrorCreationCount() : master(0), latch(0), count(0) {}
	} GECC;
	FXException_TIB() : stack(true) { }
};
static bool mytibenabled;
static QThreadLocalStorage<FXException_TIB> mytib;
static bool GlobalPause;

static void DestroyTIB()
{
	delete mytib;
	mytib=(FXException_TIB *)((FXuval)-1);
}
static inline bool CheckTIB(FXException_TIB **_mytib=0)
{
	QThread *c=QThread::current();
	if(!mytibenabled || !c) return false;
	FXException_TIB *tib=mytib;
	if(!tib)
	{
		FXERRHM(mytib=tib=new FXException_TIB);
		c->addCleanupCall(Generic::BindFuncN(DestroyTIB), true);
	}
	if(_mytib) *_mytib=tib;
	return tib!=0 && tib!=(FXException_TIB *)((FXuval)-1);
}

static inline int GetCreationCnt()
{
	int ret;
	if(!(ret=++creationCnt)) ret=++creationCnt;
	return creationCnt;
}

void FXException::int_enableNestedExceptionFramework(bool yes)
{
	mytibenabled=yes;
}

#if defined(WIN32) && defined(_MSC_VER)
#ifndef FXEXCEPTION_DISABLESOURCEINFO
#define COPY_STRING(d, s, maxlen) { size_t len=strlen(s); len=(len>maxlen) ? maxlen-1 : len; memcpy(d, s, len); d[len]=0; }

static int ExceptionFilter(unsigned int code, struct _EXCEPTION_POINTERS *ep, CONTEXT *ct)
{
	*ct=*ep->ContextRecord;
	return EXCEPTION_EXECUTE_HANDLER;
}

static DWORD64 __stdcall GetModBase(HANDLE hProcess, DWORD64 dwAddr)
{
	DWORD64 modulebase;
	// Try to get the module base if already loaded, otherwise load the module
	modulebase=SymGetModuleBase64(hProcess, dwAddr);
    if(modulebase)
        return modulebase;
    else
    {
        MEMORY_BASIC_INFORMATION stMBI ;
        if ( 0 != VirtualQueryEx ( hProcess, (LPCVOID)dwAddr, &stMBI, sizeof(stMBI)))
        {
            DWORD dwPathLen=0, dwNameLen=0 ;
            TCHAR szFile[ MAX_PATH ], szModuleName[ MAX_PATH ] ;
			MODULEINFO mi={0};
            dwPathLen = GetModuleFileName ( (HMODULE) stMBI.AllocationBase , szFile, MAX_PATH );
            dwNameLen = GetModuleBaseName (hProcess, (HMODULE) stMBI.AllocationBase , szModuleName, MAX_PATH );
			for(int n=dwNameLen; n>0; n--)
			{
				if(szModuleName[n]=='.')
				{
					szModuleName[n]=0;
					break;
				}
			}
			if(!GetModuleInformation(hProcess, (HMODULE) stMBI.AllocationBase, &mi, sizeof(mi)))
			{
				//fxmessage("WARNING: GetModuleInformation() returned error code %d\n", GetLastError());
			}
			if(!SymLoadModule64 ( hProcess, NULL, (PSTR)( (dwPathLen) ? szFile : 0), (PSTR)( (dwNameLen) ? szModuleName : 0),
				(DWORD64) mi.lpBaseOfDll, mi.SizeOfImage))
			{
				//fxmessage("WARNING: SymLoadModule64() returned error code %d\n", GetLastError());
			}
			//fxmessage("%s, %p, %x, %x\n", szFile, mi.lpBaseOfDll, mi.SizeOfImage, (DWORD) mi.lpBaseOfDll+mi.SizeOfImage);
			modulebase=SymGetModuleBase64(hProcess, dwAddr);
			return modulebase;
        }
    }
    return 0;
}

static HANDLE myprocess;
static void DeinitSym()
{
	if(myprocess)
	{
		SymCleanup(myprocess);
		CloseHandle(myprocess);
		myprocess=0;
	}
}

void FXException::doStackWalk() throw()
{
	int i,i2;
	HANDLE mythread=(HANDLE) GetCurrentThread();
	STACKFRAME64 sf={ 0 };
	CONTEXT ct={ 0 };
	if(!myprocess)
	{
		DWORD symopts;
		DuplicateHandle(GetCurrentProcess(), GetCurrentProcess(), GetCurrentProcess(), &myprocess, 0, FALSE, DUPLICATE_SAME_ACCESS);
		symopts=SymGetOptions();
		SymSetOptions(symopts /*| SYMOPT_DEFERRED_LOADS*/ | SYMOPT_LOAD_LINES);
		SymInitialize(myprocess, NULL, TRUE);
		atexit(DeinitSym);
	}
	ct.ContextFlags=CONTEXT_FULL;

	// Use RtlCaptureContext() if we have it as it saves an exception throw
	static VOID (WINAPI *RtlCaptureContextAddr)(PCONTEXT)=(VOID (WINAPI *)(PCONTEXT)) -1;
	if((VOID (WINAPI *)(PCONTEXT)) -1==RtlCaptureContextAddr)
		RtlCaptureContextAddr=(VOID (WINAPI *)(PCONTEXT)) GetProcAddress(GetModuleHandle("kernel32"), "RtlCaptureContext");
	if(RtlCaptureContextAddr)
		RtlCaptureContextAddr(&ct);
	else
	{	// This is nasty, but it works
		__try
		{
			int *foo=0;
			*foo=78;
		}
		__except (ExceptionFilter(GetExceptionCode(), GetExceptionInformation(), &ct))
		{
		}
	}

	sf.AddrPC.Mode=sf.AddrStack.Mode=sf.AddrFrame.Mode=AddrModeFlat;
#ifndef _M_AMD64
	sf.AddrPC.Offset   =ct.Eip;
	sf.AddrStack.Offset=ct.Esp;
	sf.AddrFrame.Offset=ct.Ebp;
#else
	sf.AddrPC.Offset   =ct.Rip;
	sf.AddrStack.Offset=ct.Rsp;
	sf.AddrFrame.Offset=ct.Rbp; // maybe Rdi?
#endif
	for(i2=0; i2<FXEXCEPTION_STACKBACKTRACEDEPTH; i2++)
	{
		IMAGEHLP_MODULE64 ihm={ sizeof(IMAGEHLP_MODULE64) };
		char temp[MAX_PATH+sizeof(IMAGEHLP_SYMBOL64)];
		IMAGEHLP_SYMBOL64 *ihs;
		IMAGEHLP_LINE64 ihl={ sizeof(IMAGEHLP_LINE64) };
		DWORD64 offset;
		if(!StackWalk64(
#ifndef _M_AMD64
			IMAGE_FILE_MACHINE_I386,
#else
			IMAGE_FILE_MACHINE_AMD64,
#endif
			myprocess, mythread, &sf, &ct, NULL, SymFunctionTableAccess64, GetModBase, NULL))
			break;
		if(0==sf.AddrPC.Offset)
			break;
		i=i2;
		if(i)	// Skip first entry relating to this function
		{
			p->stack[i-1].pc=(void *) sf.AddrPC.Offset;
			if(SymGetModuleInfo64(myprocess, sf.AddrPC.Offset, &ihm))
			{
				char *leaf;
				leaf=strrchr(ihm.ImageName, '\\');
				if(!leaf) leaf=ihm.ImageName-1;
				COPY_STRING(p->stack[i-1].module, leaf+1, sizeof(p->stack[i-1].module));
			}
			else strcpy(p->stack[i-1].module, "<unknown>");
			//fxmessage("WARNING: SymGetModuleInfo64() returned error code %d\n", GetLastError());
			memset(temp, 0, MAX_PATH+sizeof(IMAGEHLP_SYMBOL64));
			ihs=(IMAGEHLP_SYMBOL64 *) temp;
			ihs->SizeOfStruct=sizeof(IMAGEHLP_SYMBOL64);
			ihs->Address=sf.AddrPC.Offset;
			ihs->MaxNameLength=MAX_PATH;

			if(SymGetSymFromAddr64(myprocess, sf.AddrPC.Offset, &offset, ihs))
			{
				COPY_STRING(p->stack[i-1].functname, ihs->Name, sizeof(p->stack[i-1].functname));
				if(strlen(p->stack[i-1].functname)<sizeof(p->stack[i-1].functname)-8)
				{
					sprintf(strchr(p->stack[i-1].functname, 0), " +0x%x", offset);
				}
			}
			else
				strcpy(p->stack[i-1].functname, "<unknown>");
			DWORD lineoffset=0;
			if(SymGetLineFromAddr64(myprocess, sf.AddrPC.Offset, &lineoffset, &ihl))
			{
				char *leaf;
				p->stack[i-1].lineno=ihl.LineNumber;

				leaf=strrchr(ihl.FileName, '\\');
				if(!leaf) leaf=ihl.FileName-1;
				COPY_STRING(p->stack[i-1].file, leaf+1, sizeof(p->stack[i-1].file));
			}
			else
				strcpy(p->stack[i-1].file, "<unknown>");
		}
	}
}
#endif
#endif

void FXException::init(const char *_filename, int _lineno, const FXString &msg, FXuint _code, FXuint _flags)
{
#ifdef DEBUG
	if(FXEXCEPTION_INTTHREADCANCEL!=_code)
	{	// Purely for breakpointing
		int a=1;
	}
	if(GlobalPause)
	{	// Also breakpointing
		int a=1;
	}
	if(FXEXCEPTION_NOMEMORY==_code)
	{
		int a=1;
	}
#endif
	FXRBOp unconstr=FXRBConstruct(this);
	FXERRHM(p=new FXExceptionPrivate);
	p->message=msg;
	p->code=_code;
	p->flags=_flags;
	p->srcfilename=_filename;
	p->srclineno=_lineno;
	p->threadId=QThread::id();
	p->reporttxt=0;
	p->uniqueId=GetCreationCnt();
#if defined(DEBUG) || defined(BUILDING_TCOMMON)
	fxmessage("FXException id %d created, '%s' at line %d in %s thread %d\n", p->uniqueId, msg.text(), _lineno, _filename, (FXuint) p->threadId);
#endif
#ifndef FXEXCEPTION_DISABLESOURCEINFO
	memset(p->stack, 0, sizeof(p->stack));
#if defined(WIN32) && defined(_MSC_VER)
	static QMutex symlock;
	if(!(p->flags & FXERRH_ISINFORMATIONAL))
	{
		QMtxHold lockh(symlock);
		doStackWalk();
	}
#elif defined(__GNUC__)
	if(!(p->flags & FXERRH_ISINFORMATIONAL))
	{
		void *backtr[FXEXCEPTION_STACKBACKTRACEDEPTH];
		size_t size;
		char **strings;
		size=backtrace(backtr, FXEXCEPTION_STACKBACKTRACEDEPTH);
		strings=backtrace_symbols(backtr, size);
		for(size_t i2=0; i2<size; i2++)
		{	// Format can be <file path>(<mangled symbol>+0x<offset>) [<pc>]
			// or can be <file path> [<pc>]
			int start=0, end=strlen(strings[i2]), which=0;
			for(int idx=0; idx<end; idx++)
			{
				if(0==which && (' '==strings[i2][idx] || '('==strings[i2][idx]))
				{
					int len=FXMIN(idx-start, sizeof(p->stack[i2].file));
					memcpy(p->stack[i2].file, strings[i2]+start, len);
					p->stack[i2].file[len]=0;
					which=(' '==strings[i2][idx]) ? 2 : 1;
					start=idx+1;
				}
				else if(1==which && ')'==strings[i2][idx])
				{
					FXString functname(strings[i2]+start, idx-start);
					FXint offset=functname.rfind("+0x");
					FXString rawsymbol(functname.left(offset));
					FXString symbol(rawsymbol.length() ? fxdemanglesymbol(rawsymbol, false) : rawsymbol);
					symbol.append(functname.mid(offset));
					int len=FXMIN(symbol.length(), sizeof(p->stack[i2].functname));
					memcpy(p->stack[i2].functname, symbol.text(), len);
					p->stack[i2].functname[len]=0;
					which=2;
				}
				else if(2==which && '['==strings[i2][idx])
				{
					start=idx+1;
					which=3;
				}
				else if(3==which && ']'==strings[i2][idx])
				{
					FXString v(strings[i2]+start+2, idx-start-2);
					p->stack[i2].pc=(void *)(FXuval)v.toULong(0, 16);
				}
			}
		}
		free(strings);
	}
#endif
#endif
	p->stacklevel=-1;
	if(p->flags & FXERRH_ISFOXEXCEPTION)
	{	// If it's a FOX exception, exit the application now as there's no way
		// FOX can be in a continuable state. If you don't like this, think about
		// how Jeroen has implemented exceptions and you'll realise there is no
		// other choice.
		fxerror("%s\n", report().text());
		abort();
	}
	unconstr.dismiss();
}

#if 0
FXException FXException::copy() const
	: p->uniqueId(o.p->uniqueId), p->message(o.p->message), p->code(o.p->code), p->flags(o.p->flags),
	p->srcfilename(o.p->srcfilename), p->srclineno(o.p->srclineno), p->threadId(QThread::id()), p->reporttxt(0), nestedlist(0),
	stacklevel(o.stacklevel)
{
	FXRBOp unconstr=FXRBConstruct(this);
	if(o.p->reporttxt)
	{
		FXERRHM(p->reporttxt=new FXString(*o.p->reporttxt));
	}
	if(o.nestedlist)
	{
		FXERRHM(nestedlist=new QValueList<FXException>(*o.nestedlist));
	}
#ifndef FXEXCEPTION_DISABLESOURCEINFO
	for(int n=0; n<FXEXCEPTION_STACKBACKTRACEDEPTH; n++)
		p->stack[n]=o.p->stack[n];
#endif
	FXException_TIB *tib=0;
	if(CheckTIB(&tib))
	{
		FXException_TIB::LevelEntry *le=tib->stack.at(stacklevel);
		if(stacklevel>=0 && le->p->uniqueId==p->uniqueId)
		{
			//fxmessage("Thread %u creation of copy of primary exception=%p, id=%d from stack level %d\n", (FXuint) QThread::id(), this, p->uniqueId, stacklevel);
			le->currentExceptions.append(this);
		}
	}
	unconstr.dismiss();
}
#endif

FXException &FXException::operator=(FXException &o)
{
	FXDELETE(p);
	p=o.p;
	o.p=0;
	return *this;
}

FXException FXException::copy() const
{
	FXException ret(p->srcfilename, p->srclineno, p->message, p->code, p->flags);
	ret.p->uniqueId=p->uniqueId; ret.p->threadId=p->threadId;
	if(p->reporttxt)
	{
		FXERRHM(ret.p->reporttxt=new FXString(*p->reporttxt));
	}
	FXExceptionPrivate *ne;
	for(QPtrVectorIterator<FXExceptionPrivate> it(p->nestedlist); (ne=it.current()); ++it)
	{
		FXExceptionPrivate *_e;
		FXERRHM(_e=new FXExceptionPrivate(*ne));
		FXRBOp une=FXRBNew(_e);
		ret.p->nestedlist.append(_e);
		une.dismiss();
	}
#ifndef FXEXCEPTION_DISABLESOURCEINFO
	for(int n=0; n<FXEXCEPTION_STACKBACKTRACEDEPTH; n++)
		ret.p->stack[n]=p->stack[n];
#endif
	ret.p->stacklevel=p->stacklevel;
	FXException_TIB *tib=mytib;
	if(ret.p->stacklevel>=0 && tib->stack.at(ret.p->stacklevel)->uniqueId==ret.p->uniqueId)
	{
		//fxmessage("Thread %u creation of copy of primary exception=%p, id=%d from stack level %d\n", (FXuint) QThread::id(), this, p->uniqueId, stacklevel);
		tib->stack.at(ret.p->stacklevel)->currentExceptions.append(ret.p);
	}
	return ret;
}

FXException::~FXException()
{ FXEXCEPTIONDESTRUCT1 {
	if(p)
	{
		if(p->uniqueId)
		{
			FXException_TIB *tib=0;
			if(CheckTIB(&tib) && p->stacklevel>=0)
			{
				assert(p->stacklevel<(int) tib->stack.count());
				FXException_TIB::LevelEntry *le=tib->stack.at(p->stacklevel);
				if(le && le->uniqueId==p->uniqueId)
				{
					//fxmessage("Thread %u destruction of copy of primary exception=%p, id=%d from stack level %d (%d remaining)\n", (FXuint) QThread::id(), this, p->uniqueId, stacklevel, le->currentExceptions.count()-1);
					le->currentExceptions.removeRef(p);
					if(le->currentExceptions.isEmpty()) le->uniqueId=0;
				}
			}
			p->uniqueId=0;
		}
		FXDELETE(p->reporttxt);
		FXDELETE(p);
	}
} FXEXCEPTIONDESTRUCT2; }

bool FXException::isValid() const throw()
{
	return p->uniqueId!=0;
}

bool FXException::isFatal() const throw()
{
	return p->flags & FXERRH_ISFATAL;
}

void FXException::setFatal(bool _fatal)
{
	if(_fatal)
		p->flags|=FXERRH_ISFATAL;
	else
		p->flags&=~FXERRH_ISFATAL;
}

void FXException::sourceInfo(const char **FXRESTRICT file, int *FXRESTRICT lineno) const throw()
{
	if(file) *file=p->srcfilename;
	if(lineno) *lineno=p->srclineno;
}

const FXString &FXException::message() const throw()
{
	return p->message;
}

void FXException::setMessage(const FXString &msg)
{
	p->message=msg;
	if(isValid() && p->reporttxt)
	{
		FXDELETE(p->reporttxt);
	}
}

FXuint FXException::code() const throw()
{
	return p->code;
}

FXuint FXException::flags() const throw()
{
	return p->flags;
}

FXulong FXException::threadId() const throw()
{
	return p->threadId;
}

const FXString &FXException::report() const
{
	if(isValid())
	{
		if(!p->reporttxt)
		{
			if(!p->srcfilename || (p->flags & FXERRH_ISINFORMATIONAL)!=0)
			{
				if(p->flags & (FXERRH_ISFATAL|FXERRH_ISFOXEXCEPTION))
					p->reporttxt=new FXString(QTrans::tr("FXException", "FATAL ERROR: %1 (code 0x%2)").arg(p->message).arg(p->code,0,16));
				else if(p->flags & FXERRH_ISFROMOTHER)
					p->reporttxt=new FXString(QTrans::tr("FXException", "From other end of IPC channel: %1 (code 0x%2)").arg(p->message).arg(p->code,0,16));
				else
					p->reporttxt=new FXString(QTrans::tr("FXException", "%1 (code 0x%2)").arg(p->message).arg(p->code,0,16));
			}
			else
			{
				if(p->flags & (FXERRH_ISFATAL|FXERRH_ISFOXEXCEPTION))
					p->reporttxt=new FXString(QTrans::tr("FXException", "FATAL ERROR: %1 (code 0x%2 file %3 line %4 thread %5)").arg(p->message).arg(p->code,0,16).arg(p->srcfilename).arg(p->srclineno).arg(p->threadId));
				else if(p->flags & FXERRH_ISFROMOTHER)
					p->reporttxt=new FXString(QTrans::tr("FXException", "From other end of IPC channel: %1 (code 0x%2 file %3 line %4 thread %5)").arg(p->message).arg(p->code,0,16).arg(p->srcfilename).arg(p->srclineno).arg(p->threadId));
				else
					p->reporttxt=new FXString(QTrans::tr("FXException", "%1 (code 0x%2 file %3 line %4 thread %5)").arg(p->message).arg(p->code,0,16).arg(p->srcfilename).arg(p->srclineno).arg(p->threadId));
			}
#ifndef FXEXCEPTION_DISABLESOURCEINFO
			if(!(p->flags & FXERRH_ISINFORMATIONAL))
			{
				int i;
				p->reporttxt->append(QTrans::tr("FXException", "\nStack backtrace:\n"));
				FXString templ(QTrans::tr("FXException", "0x%1:%2%3\n                                 (file %4 line no %5)\n"));
				for(i=0; i<FXEXCEPTION_STACKBACKTRACEDEPTH; i++)
				{
					if(!p->stack[i].pc) break;
					{
						FXString line(templ);
						line.arg((FXuval) p->stack[i].pc, -(int)(2*sizeof(void *)), 16).arg(FXString(p->stack[i].module), -21).arg(FXString(p->stack[i].functname));
						line.arg(p->stack[i].file, -25).arg(p->stack[i].lineno);
						p->reporttxt->append(line);
					}
				}
				if(FXEXCEPTION_STACKBACKTRACEDEPTH==i)
					p->reporttxt->append(QTrans::tr("FXException", "<backtrace may continue ...>"));
				else
					p->reporttxt->append(QTrans::tr("FXException", "<backtrace ends>"));
			}
#endif
			if(nestedLen())
			{
				p->reporttxt->append(QTrans::tr("FXException", "\nFurthermore, %1 errors occurred during the handling of the above exception:\n").arg(nestedLen()));
				for(int n=0; n<nestedLen(); n++)
				{
					p->reporttxt->append(FXString::number(n+1)+": "+nested(n).report()+"\n");
				}
			}
		}
		return *p->reporttxt;
	}
	static FXString foo;
	return foo;
}

bool FXException::isPrimary() const
{
	return p->stacklevel>=0 && mytib->stack.at(p->stacklevel)->uniqueId==p->uniqueId;
}

FXint FXException::nestedLen() const
{
	return (FXint) p->nestedlist.size();
}

FXException FXException::nested(FXint idx) const
{
	FXException ret;
	FXDELETE(ret.p);
	FXExceptionPrivate *rp=p->nestedlist.at(idx);
	if(rp)
	{
		FXERRHM(ret.p=new FXExceptionPrivate(*rp));
		ret.p->nestedlist.setAutoDelete(false);
	}
	return ret;
}

void FXException::int_setThrownException(FXException &e)
{
	FXException_TIB *tib=0;
	if(CheckTIB(&tib))
	{
		if(e.code()!=FXEXCEPTION_INTTHREADCANCEL)
		{
			assert(tib);
			// Set this exception's stack level to the current level
			e.p->stacklevel=tib->stack.count()-1;
			if(e.p->stacklevel<0)
			{	// Oh dear, there's no FXERRH_TRY above me
				e.setFatal(true);
				fxerror("NO FXERRH_TRY ABOVE %s\n", e.report().text());
				assert(0);
			}
			else
			{
				FXException_TIB::LevelEntry *le=tib->stack.getLast();
				//if(8==e.p->uniqueId)
				//{
				//	int a=1;
				//}
				if(!le->uniqueId)
				{
					assert(le->currentExceptions.isEmpty());
					//fxmessage("Thread %u setting primary exception=%p, id=%u in stack level %d\n", (FXuint) QThread::id(), &e, e.p->uniqueId, e.stacklevel);
					le->currentExceptions.append(e.p);
					le->uniqueId=e.p->uniqueId;
				}
				else if(le->uniqueId==e.p->uniqueId)
				{	// Same exception rethrown
					//fxmessage("Thread %u rethrowing primary exception=%p, id=%u in stack level %d\n", (FXuint) QThread::id(), &e, e.p->uniqueId, e.stacklevel);
					le->currentExceptions.append(e.p);
				}
				else
				{	// New exception thrown during handling of primary exception (bad)
					FXExceptionPrivate *ee;
					for(QPtrListIterator<FXExceptionPrivate> it(le->currentExceptions); (ee=it.current()); ++it)
					{
						FXExceptionPrivate *_e;
						FXERRHM(_e=new FXExceptionPrivate(*e.p));
						FXRBOp une=FXRBNew(_e);
						ee->flags|=FXERRH_HASNESTED|FXERRH_ISFATAL;
						ee->nestedlist.append(_e);
						une.dismiss();
						assert(e.p->nestedlist.isEmpty());
					}
					//fxmessage("Thread %u throwing nested exception=%p, id=%d into primary exception id=%d in stack level %d, nestingCount=%d\n",
					//	(FXuint) QThread::id(), &e, e.p->uniqueId, le->p->uniqueId, e.stacklevel, le->nestingCount);
					assert(le->nestingCount>0);
				}
			}
		}
	}
	else
	{	// We're in a state without static data, so likelihood is there's no catch handlers
		e.setFatal(true);
		fxerror("DURING STATIC DATA INITIALISATION/DESTRUCTION %s\n", e.report().text());
	}
}

void FXException::int_enterTryHandler(const char *srcfile, int lineno)
{	// Called by FXERRH_TRY before each iteration of the tryable code
	FXException_TIB *tib=0;
	if(CheckTIB(&tib))
	{
		FXException_TIB::LevelEntry *le;
		FXERRHM(le=new FXException_TIB::LevelEntry(srcfile, lineno));
		FXRBOp unle=FXRBNew(le);
		tib->stack.append(le);
		unle.dismiss();
		//fxmessage("Thread %u entering try handler stack level %d (file %s line %d)\n",
		//	(FXuint) QThread::id(), tib->stack.count()-1, srcfile, lineno);
	}
}

void FXException::int_exitTryHandler() throw()
{	// Called by FXERRH_TRY at the end of each iteration of the tryable code
	FXException_TIB *tib=0;
	if(CheckTIB(&tib))
	{
		assert(tib);
		FXException_TIB::LevelEntry *le=tib->stack.getLast();
		FXuint stackcount=tib->stack.count();
		if(le && !le->currentExceptions.isEmpty())
		{
			if(stackcount>0)
			{	// The only exceptions left are those being thrown or rethrown so transfer upwards
				FXException_TIB::LevelEntry *ple=tib->stack.at(stackcount-2);
				FXExceptionPrivate *currenterror=le->currentExceptions.getFirst();
				if(ple->uniqueId)
				{	// Enter as nested
					FXExceptionPrivate *ee;
					for(QPtrListIterator<FXExceptionPrivate> it(ple->currentExceptions); (ee=it.current()); ++it)
					{
						FXExceptionPrivate *_e;
						try
						{
							_e=new FXExceptionPrivate(*currenterror);
						}
						catch(...)
						{
							std::terminate();
						}
						FXRBOp une=FXRBNew(_e);
						ee->flags|=FXERRH_HASNESTED|FXERRH_ISFATAL;
						ee->nestedlist.append(_e);
						une.dismiss();
					}
					//fxmessage("Thread %u reentering exception %d as nested into stack level %d\n", le->currentExceptions.getFirst()->p->uniqueId, (FXuint) QThread::id(), stackcount-1);
				}
				else
				{	// Enter as primary
					ple->currentExceptions=le->currentExceptions;
					ple->uniqueId=currenterror->uniqueId;
					//fxmessage("Thread %u reentering exception %d as primary into stack level %d\n", le->currentExceptions.getFirst()->p->uniqueId, (FXuint) QThread::id(), stackcount-1);
				}
				currenterror->stacklevel=stackcount-2;
			}
		}
		//fxmessage("Thread %u exiting try handler stack level %d\n", (FXuint) QThread::id(), stackcount-1);
		tib->stack.removeLast();
	}
}

void FXException::int_incDestructorCnt()
{
	FXException_TIB *tib=0;
	if(CheckTIB(&tib))
	{
		assert(tib);
		FXException_TIB::LevelEntry *le=tib->stack.getLast();
		//assert(tib->nestingCount>=0);
		if(le) le->nestingCount++;
	}
}

bool FXException::int_nestedException(FXException &e)
{	// Returns true if exception should be rethrown
	FXException_TIB *tib=0;
	if(CheckTIB(&tib))
	{
		assert(tib);
		FXException_TIB::LevelEntry *le=tib->stack.getLast();
		//fxmessage("Thread %u destructor caught exception %d in stack level %d, throwing already=%d, nestingCount=%d\n",
		//	(FXuint) QThread::id(), e.p->uniqueId, e.stacklevel, !!(le->currentExceptions.getFirst()->p->flags & FXERRH_HASNESTED), le->nestingCount);
		if(!(le->currentExceptions.getFirst()->flags & FXERRH_HASNESTED))
		{	// If just one exception being thrown, always throw upwards
			return true;
		}
		else
		{	// Must never throw during handling of throw (as it would call terminate()).
			// This unfortunately doesn't mean quite correct behaviour :(
			return (le->nestingCount>1) ? true : false;
		}
	}
	return true;
}

void FXException::int_decDestructorCnt()
{
	FXException_TIB *tib=0;
	if(CheckTIB(&tib))
	{
		assert(tib);
		FXException_TIB::LevelEntry *le=tib->stack.getLast();
		if(le) le->nestingCount--;
		//assert(tib->nestingCount>=0);
	}
}

bool FXException::int_testCondition()
{
#ifdef DEBUG
	FXException_TIB *tib=mytib;
	if(!tib || tib==(FXException_TIB *)((FXuval)-1)) return false;
	if(!tib->GECC.master) return false;
	if(tib->GECC.count++>=tib->GECC.latch)
	{
		setGlobalErrorCreationCount(tib->GECC.master);
		return true;
	}
#endif
	return false;
}

FXint FXException::setGlobalErrorCreationCount(FXint no)
{
#ifdef DEBUG
	FXException_TIB *tib=0;
	if(CheckTIB(&tib))
	{
		FXint ret=tib->GECC.master;
		tib->GECC.master=no;
		tib->GECC.latch=(no<0) ? ((FXint) (rand()*abs(no)/RAND_MAX)) : abs(no);
		tib->GECC.count=0;
		return ret;
	}
#endif
	return 0;
}

bool FXException::setConstructionBreak(bool v)
{
	bool t=GlobalPause;
	GlobalPause=v;
	return t;
}

void FXException::int_throwOSError(const char *file, int lineno, int code, FXuint flags, const FXString &filename)
{
	FXString errstr(strerror(code));
	errstr.append(" ("+FXString::number(code)+")");
	if(ENOENT==code || ENOTDIR==code)
	{
		errstr=QTrans::tr("FXException", "File '%1' not found [Host OS Error: %2]").arg(filename).arg(errstr);
		FXNotFoundException e(file, lineno, errstr, flags);
		FXERRH_THROW(e);
	}
	else if(EACCES==code)
	{
		errstr=QTrans::tr("FXException", "Access to '%1' denied [Host OS Error: %2]").arg(filename).arg(errstr);
		FXNoPermissionException e(errstr, flags);
		FXERRH_THROW(e);
	}
	else
	{
		FXException e(file, lineno, errstr, FXEXCEPTION_OSSPECIFIC, flags);
		FXERRH_THROW(e);
	}
}
void FXException::int_throwWinError(const char *file, int lineno, FXuint code, FXuint flags, const FXString &filename)
{
#ifdef WIN32
	if(ERROR_SUCCESS==code)
	{
#ifdef DEBUG
		fxmessage("WARNING: Win32 reported error when there was no error!\n");
#endif
		return;
	}
	DWORD len;
	TCHAR buffer[1024];
	len=FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, code, 0, buffer, sizeof(buffer)/sizeof(TCHAR), 0);
	// Remove annoying CRLF at end of message sometimes
	while(10==buffer[len-1])
	{
		buffer[len-1]=0;
		len--;
		if(13==buffer[len-1])
		{
			buffer[len-1]=0;
			len--;
		}
	}
	FXString errstr(buffer, len);
	errstr.append(" ("+FXString::number(code)+")");
	if(ERROR_FILE_NOT_FOUND==code || ERROR_PATH_NOT_FOUND==code)
	{
		errstr=QTrans::tr("FXException", "File '%1' not found [Host OS Error: %2]").arg(filename).arg(errstr);
		FXNotFoundException e(file, lineno, errstr, flags);
		FXERRH_THROW(e);
	}
	else if(ERROR_ACCESS_DENIED==code || ERROR_EA_ACCESS_DENIED==code)
	{
		errstr=QTrans::tr("FXException", "Access to '%1' denied [Host OS Error: %2]").arg(filename).arg(errstr);
		FXNoPermissionException e(errstr, flags);
		FXERRH_THROW(e);
	}
	else if(ERROR_NO_DATA==code || ERROR_BROKEN_PIPE==code || ERROR_PIPE_NOT_CONNECTED==code || ERROR_PIPE_LISTENING==code)
	{
		FXConnectionLostException e(errstr, flags);
		FXERRH_THROW(e);
	}
	else
	{
		FXException e(file, lineno, errstr, FXEXCEPTION_OSSPECIFIC, flags);
		FXERRH_THROW(e);
	}
#endif
}


FXStream &operator<<(FXStream &s, const FXException &e)
{
	s << (FXchar) e.isValid();
	if(e.isValid())
	{
		s << e.report() << e.code() << e.flags();
		s << e.p->nestedlist.size();
		FXExceptionPrivate *_e;
		for(QPtrVectorIterator<FXExceptionPrivate> it(e.p->nestedlist); (_e=it.current()); ++it)
		{
			s << _e->message << _e->code << _e->flags;
		}
	}
	return s;
}
FXStream &operator>>(FXStream &s, FXException &e)
{
	FXchar valid=0;
	s >> valid;
	if(valid)
	{
		e.p->uniqueId=-1;
		s >> e.p->message >> e.p->code >> e.p->flags;
		FXuint sz;
		s >> sz;
		e.p->nestedlist.clear();
		for(FXuint n=0; n<sz; n++)
		{
			FXExceptionPrivate *_e;
			FXERRHM(_e=new FXExceptionPrivate);
			FXRBOp une=FXRBNew(_e);
			s >> _e->message >> _e->code >> _e->flags;
			e.p->nestedlist.append(_e);
			une.dismiss();
		}
	}
	return s;
}

} // namespace
