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

#include <qlist.h>
#include <qvaluelist.h>
#include "FXException.h"
#if defined(WIN32) && defined(_MSC_VER)
#if !defined(FXEXCEPTION_DISABLESOURCEINFO)
#define WIN32_LEAN_AND_MEAN
#include "Windows.h"
#include "Dbghelp.h"
#endif
#endif
#include "FXTrans.h"
#include "FXThread.h"
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
//static TThreadLocalStorageBase exceptionCount;
static int creationCnt;
struct FXException_TIB
{
	int nestingCount;	// How far away we are from a real (ie; non-wrapper) try...catch block
	int uniqueId;		// The id of the primary exception currently being thrown
	QList<FXException> currentException;	// A list of known copies of the primary exception currently being thrown
	struct GlobalErrorCreationCount
	{
		FXint master;
		FXint latch;
		FXint count;
		GlobalErrorCreationCount() : master(0), latch(0), count(0) {}
	} GECC;
	FXException_TIB() : nestingCount(0), uniqueId(0) { }
};
static bool mytibenabled;
static FXThreadLocalStorage<FXException_TIB> mytib;

static void DestroyTIB()
{
	delete mytib;
	mytib=(FXException_TIB *)((FXuval)-1);
}
static inline bool CheckTIB()
{
	FXThread *c=FXThread::current();
	if(!mytibenabled || !c) return false;
	if(!mytib)
	{
		FXERRHM(mytib=new FXException_TIB);
		c->addCleanupCall(Generic::BindFuncN(DestroyTIB), true);
	}
	FXException_TIB *tib=mytib;
	return tib!=0 && tib!=(FXException_TIB *)((FXuval)-1);
}

static inline int GetCreationCnt()
{
	if(0==++creationCnt) creationCnt++;
	return creationCnt;
}

void FXException::int_enableNestedExceptionFramework(bool yes)
{
	mytibenabled=yes;
}

#if defined(WIN32) && defined(_MSC_VER)
#ifndef FXEXCEPTION_DISABLESOURCEINFO
#define COPY_STRING(d, s, maxlen) { int len=strlen(s); len=(len>maxlen) ? maxlen-1 : len; memcpy(d, s, len); d[len]=0; }

static DWORD __stdcall GetModBase(HANDLE hProcess, DWORD dwAddr)
{
	DWORD modulebase;
	modulebase=SymGetModuleBase(hProcess, dwAddr);
    if(modulebase)
        return modulebase ;
    else
    {
        MEMORY_BASIC_INFORMATION stMBI ;
        if ( 0 != VirtualQueryEx ( hProcess, (LPCVOID)dwAddr, &stMBI, sizeof ( stMBI )))
        {
            DWORD dwNameLen = 0 ;
            TCHAR szFile[ MAX_PATH ] ;
            HANDLE hFile = NULL ;
            dwNameLen = GetModuleFileName ( (HINSTANCE) stMBI.AllocationBase , szFile, MAX_PATH );
            if ( 0 != dwNameLen )
                hFile = CreateFile ( szFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
            SymLoadModule ( hProcess, hFile, (PSTR)( (dwNameLen) ? szFile : 0), NULL, (DWORD)stMBI.AllocationBase, 0);
			if(hFile) CloseHandle(hFile);
            return (DWORD) stMBI.AllocationBase;
        }
    }
    return 0;
}
#endif
#endif


void FXException::init(const char *_filename, int _lineno, const FXString &msg, FXuint __code, FXuint __flags)
{
#ifdef DEBUG
	if(FXEXCEPTION_INTTHREADCANCEL!=__code)
	{	// Purely for breakpointing
		int a=1;
	}
#endif
#ifdef BUILDING_TCOMMON
	fxmessage("FXException created, '%s' at line %d in %s thread %d\n", msg.text(), _lineno, _filename, FXThread::id());
#endif
	FXRBOp unconstr=FXRBConstruct(this);
	_message=msg;
	_code=__code;
	_flags=__flags;
	srcfilename=_filename;
	srclineno=_lineno;
	reporttxt=0;
	uniqueId=GetCreationCnt();
#if defined(WIN32) && defined(_MSC_VER)
	memset(stack, 0, sizeof(stack));
#ifndef FXEXCEPTION_DISABLESOURCEINFO
	static FXMutex symlock;
	if(!(_flags & FXERRH_ISINFORMATIONAL))
	{
		FXMtxHold lockh(symlock);
		int i,i2;
		HANDLE myprocess;
		HANDLE mythread=(HANDLE) GetCurrentThread();
		STACKFRAME sf={ 0 };
		CONTEXT ct={ 0 };
		DWORD symopts;
		DuplicateHandle(GetCurrentProcess(), GetCurrentProcess(), GetCurrentProcess(), &myprocess, 0, FALSE, DUPLICATE_SAME_ACCESS);
		symopts=SymGetOptions();
		SymSetOptions(symopts | SYMOPT_LOAD_LINES);
		SymInitialize(myprocess, NULL, FALSE);
		ct.ContextFlags=CONTEXT_FULL;
		GetThreadContext(mythread, &ct);
		sf.AddrPC.Mode=sf.AddrStack.Mode=sf.AddrFrame.Mode=AddrModeFlat;
		sf.AddrPC.Offset   =ct.Eip;
		sf.AddrStack.Offset=ct.Esp;
		sf.AddrFrame.Offset=ct.Ebp;
		for(i2=0; i2<FXEXCEPTION_STACKBACKTRACEDEPTH; i2++)
		{
			IMAGEHLP_MODULE ihm={ sizeof(IMAGEHLP_MODULE) };
			char temp[MAX_PATH+sizeof(IMAGEHLP_SYMBOL)];
			IMAGEHLP_SYMBOL *ihs;
			IMAGEHLP_LINE ihl={ sizeof(IMAGEHLP_LINE) };
			DWORD offset;
			if(!StackWalk(IMAGE_FILE_MACHINE_I386, myprocess, mythread, &sf, &ct, NULL,
				SymFunctionTableAccess, GetModBase, NULL))
				break;
			if(0==sf.AddrPC.Offset)
				break;
			i=i2;
			if(i)	// Skip first entry relating to GetThreadContext()
			{
				stack[i-1].pc=(void *) sf.AddrPC.Offset;
				if(SymGetModuleInfo(myprocess, sf.AddrPC.Offset, &ihm))
				{
					char *leaf;
					leaf=strrchr(ihm.ImageName, '\\');
					if(!leaf) leaf=ihm.ImageName-1;
					COPY_STRING(stack[i-1].module, leaf+1, sizeof(stack[i-1].module));
				}
				else strcpy(stack[i-1].module, "<unknown>");
				memset(temp, 0, MAX_PATH+sizeof(IMAGEHLP_SYMBOL));
				ihs=(IMAGEHLP_SYMBOL *) temp;
				ihs->SizeOfStruct=sizeof(IMAGEHLP_SYMBOL);
				ihs->Address=sf.AddrPC.Offset;
				ihs->MaxNameLength=MAX_PATH;

				if(SymGetSymFromAddr(myprocess, sf.AddrPC.Offset, &offset, ihs))
				{
					COPY_STRING(stack[i-1].functname, ihs->Name, sizeof(stack[i-1].functname));
					if(strlen(stack[i-1].functname)<sizeof(stack[i-1].functname)-8)
					{
						sprintf(strchr(stack[i-1].functname, 0), " +0x%x", offset);
					}
				}
				else
					strcpy(stack[i-1].functname, "<unknown>");
				if(SymGetLineFromAddr(myprocess, sf.AddrPC.Offset, &offset, &ihl))
				{
					char *leaf;
					stack[i-1].lineno=ihl.LineNumber;

					leaf=strrchr(ihl.FileName, '\\');
					if(!leaf) leaf=ihl.FileName-1;
					COPY_STRING(stack[i-1].file, leaf+1, sizeof(stack[i-1].file));
				}
				else
					strcpy(stack[i-1].file, "<unknown>");
			}
		}
		SymCleanup(myprocess);
		CloseHandle(myprocess);
	}
#endif
#endif
	if(!CheckTIB())
	{	// We're in a state without static data, so likelihood is there's no catch handlers
		setFatal(true);
		fxerror("DURING STATIC DATA INITIALISATION/DESTRUCTION %s\n", report().text());
	}
	unconstr.dismiss();
}

FXException::FXException(const FXException &o)
	: uniqueId(o.uniqueId), _message(o._message), _code(o._code), _flags(o._flags),
	srcfilename(o.srcfilename), srclineno(o.srclineno), _threadId(FXThread::id()), reporttxt(0), nestedlist(0)
{
	FXRBOp unconstr=FXRBConstruct(this);
	if(o.reporttxt)
	{
		FXERRHM(reporttxt=new FXString(*o.reporttxt));
	}
	if(o.nestedlist)
	{
		FXERRHM(nestedlist=new QValueList<FXException>(*o.nestedlist));
	}
#ifdef WIN32
#ifndef FXEXCEPTION_DISABLESOURCEINFO
	for(int n=0; n<FXEXCEPTION_STACKBACKTRACEDEPTH; n++)
		stack[n]=o.stack[n];
#endif
#endif
	if(CheckTIB())
	{
		if(mytib->uniqueId==uniqueId)
		{
			//printf("Copy construction of primary exception. Original=%x, Copy=%x\n", &o, this);
			mytib->currentException.append(this);
		}
	}
	unconstr.dismiss();
}

FXException &FXException::operator=(const FXException &o)
{
	if(mytib->uniqueId==uniqueId)
	{
		mytib->currentException.removeRef(this);
	}
	uniqueId=o.uniqueId; _message=o._message; _code=o._code; _flags=o._flags;
	srcfilename=o.srcfilename; srclineno=o.srclineno; _threadId=o._threadId;
	FXDELETE(reporttxt);
	FXDELETE(nestedlist);
	if(o.reporttxt)
	{
		FXERRHM(reporttxt=new FXString(*o.reporttxt));
	}
	if(o.nestedlist)
	{
		FXERRHM(nestedlist=new QValueList<FXException>(*o.nestedlist));
	}
#ifdef WIN32
#ifndef FXEXCEPTION_DISABLESOURCEINFO
	for(int n=0; n<FXEXCEPTION_STACKBACKTRACEDEPTH; n++)
		stack[n]=o.stack[n];
#endif
#endif
	if(mytib->uniqueId==uniqueId)
	{
		mytib->currentException.append(this);
	}
	return *this;
}

FXException::~FXException()
{ FXEXCEPTIONDESTRUCT1 {
	if(uniqueId && CheckTIB())
	{
		if(mytib->uniqueId==uniqueId)
		{
			//printf("Destruction of copy of primary exception=%x\n", this);
			mytib->currentException.removeRef(this);
		}
		uniqueId=0;
	}
	FXDELETE(reporttxt);
	FXDELETE(nestedlist);
} FXEXCEPTIONDESTRUCT2; }

void FXException::setFatal(bool _fatal)
{
	if(_fatal)
		_flags|=FXERRH_ISFATAL;
	else
		_flags&=~FXERRH_ISFATAL;
}

void FXException::setMessage(const FXString &msg)
{
	_message=msg;
	if(isValid() && reporttxt)
	{
		FXDELETE(reporttxt);
	}
}

const FXString &FXException::report() const
{
	if(isValid())
	{
		if(!reporttxt)
		{
			if(!srcfilename || (_flags & FXERRH_ISINFORMATIONAL)!=0)
			{
				if(_flags & FXERRH_ISFATAL)
					reporttxt=new FXString(FXTrans::tr("FXException", "FATAL ERROR: %1 (code 0x%2)").arg(_message).arg(_code,0,16));
				else if(_flags & FXERRH_ISFROMOTHER)
					reporttxt=new FXString(FXTrans::tr("FXException", "From other end of IPC channel: %1 (code 0x%2)").arg(_message).arg(_code,0,16));
				else
					reporttxt=new FXString(FXTrans::tr("FXException", "%1 (code 0x%2)").arg(_message).arg(_code,0,16));
			}
			else
			{
				if(_flags & FXERRH_ISFATAL)
					reporttxt=new FXString(FXTrans::tr("FXException", "FATAL ERROR: %1 (code 0x%2 file %3 line %4 thread %5)").arg(_message).arg(_code,0,16).arg(srcfilename).arg(srclineno).arg(_threadId));
				else if(_flags & FXERRH_ISFROMOTHER)
					reporttxt=new FXString(FXTrans::tr("FXException", "From other end of IPC channel: %1 (code 0x%2 file %3 line %4 thread %5)").arg(_message).arg(_code,0,16).arg(srcfilename).arg(srclineno).arg(_threadId));
				else
					reporttxt=new FXString(FXTrans::tr("FXException", "%1 (code 0x%2 file %3 line %4 thread %5)").arg(_message).arg(_code,0,16).arg(srcfilename).arg(srclineno).arg(_threadId));
			}
#if defined(WIN32) && defined(_MSC_VER)
#ifndef FXEXCEPTION_DISABLESOURCEINFO
			if(!(_flags & FXERRH_ISINFORMATIONAL))
			{
				int i;
				reporttxt->append(FXTrans::tr("FXException", "\nStack backtrace:\n"));
				FXString templ(FXTrans::tr("FXException", "0x%1:%2%3\n                                 (file %4 line no %5)\n"));
				for(i=0; i<FXEXCEPTION_STACKBACKTRACEDEPTH; i++)
				{
					if(!stack[i].pc) break;
					{
						FXString line(templ);
						line.arg((FXulong) stack[i].pc, -8, 16).arg(FXString(stack[i].module), -21).arg(FXString(stack[i].functname));
						line.arg(stack[i].file, -25).arg(stack[i].lineno);
						reporttxt->append(line);
					}
				}
				if(FXEXCEPTION_STACKBACKTRACEDEPTH==i)
					reporttxt->append(FXTrans::tr("FXException", "<backtrace may continue ...>"));
				else
					reporttxt->append(FXTrans::tr("FXException", "<backtrace ends>"));
			}
#endif
#endif
			if(nestedLen())
			{
				reporttxt->append(FXTrans::tr("FXException", "\nFurthermore, %1 errors occurred during the handling of the above exception:\n").arg(nestedLen()));
				for(int n=0; n<nestedLen(); n++)
				{
					reporttxt->append(FXString::number(n+1)+": "+nested(n).report()+"\n");
				}
			}
		}
		return *reporttxt;
	}
	static FXString foo;
	return foo;
}

bool FXException::isPrimary() const
{
	return mytib->uniqueId==uniqueId;
}

FXint FXException::nestedLen() const
{
	if(!nestedlist) return 0;
	return nestedlist->size();
}

FXException &FXException::nested(FXint idx) const
{
	if(!nestedlist)
	{
		static FXException null;
		return null;
	}
	return *nestedlist->at(idx);
}

void FXException::int_setThrownException(FXException &e)
{
	if(CheckTIB())
	{
		if(!mytib->uniqueId && e.code()!=FXEXCEPTION_INTTHREADCANCEL)
		{
			assert(mytib->currentException.isEmpty());
			//printf("Setting primary exception=%x\n", &e);
			mytib->currentException.append(&e);
			mytib->uniqueId=e.uniqueId;
		}
	}
}

void FXException::int_resetThrownException()
{
	if(CheckTIB())
	{
		if(mytib->uniqueId)
		{
			mytib->currentException.clear();
			mytib->uniqueId=0;
		}
		mytib->nestingCount=0;
	}
}

void FXException::int_incDestructorCnt()
{
	if(CheckTIB())
	{
		//assert(mytib->nestingCount>=0);
		mytib->nestingCount++;
	}
}

bool FXException::int_nestedException(FXException &e, bool amAuto)
{	// Returns true if exception should be rethrown
	if(CheckTIB())
	{
		if(mytib->uniqueId)
		{	// Modify all copies of primary exception
			for(QListIterator<FXException> it(mytib->currentException); it.current(); ++it)
			{
				if(!(*it)->nestedlist)
				{
					FXERRHM((*it)->nestedlist=new QValueList<FXException>);
					(*it)->_flags|=FXERRH_HASNESTED;
				}
				if(amAuto) (*it)->setFatal(true);
				(*it)->nestedlist->push_back(e);
			}
			return (mytib->nestingCount>1) ? true : false;
		}
	}
	return true;
}

void FXException::int_decDestructorCnt()
{
	if(CheckTIB())
	{
		mytib->nestingCount--;
		//assert(mytib->nestingCount>=0);
	}
}

bool FXException::int_testCondition()
{
#ifdef DEBUG
	if(!mytib || mytib==(FXException_TIB *)((FXuval)-1)) return false;
	if(!mytib->GECC.master) return false;
	if(mytib->GECC.count++>=mytib->GECC.latch)
	{
		setGlobalErrorCreationCount(mytib->GECC.master);
		return true;
	}
#endif
	return false;
}

FXint FXException::setGlobalErrorCreationCount(FXint no)
{
#ifdef DEBUG
	if(CheckTIB())
	{
		FXint ret=mytib->GECC.master;
		mytib->GECC.master=no;
		mytib->GECC.latch=(no<0) ? ((FXint) (rand()*abs(no)/RAND_MAX)) : abs(no);
		mytib->GECC.count=0;
		return ret;
	}
#endif
	return 0;
}

void FXException::int_throwOSError(const char *file, int lineno, int code, FXuint flags)
{
	if(ENOENT==code || ENOTDIR==code)
	{
		FXNotFoundException e(file, lineno, strerror(code), flags);
		FXERRH_THROW(e);
	}
	else if(EACCES==code)
	{
		FXNoPermissionException e(strerror(code), flags);
		FXERRH_THROW(e);
	}
	else
	{
		FXException e(file, lineno, strerror(code), FXEXCEPTION_OSSPECIFIC, flags);
		FXERRH_THROW(e);
	}
}
void FXException::int_throwWinError(const char *file, int lineno, FXuint code, FXuint flags)
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
	if(10==buffer[len-1]) buffer[len-1]=0;
	if(13==buffer[len-2]) buffer[len-2]=0;
	if(ERROR_FILE_NOT_FOUND==code || ERROR_PATH_NOT_FOUND==code)
	{
		FXNotFoundException e(file, lineno, buffer, flags);
		FXERRH_THROW(e);
	}
	else if(ERROR_ACCESS_DENIED==code || ERROR_EA_ACCESS_DENIED==code)
	{
		FXNoPermissionException e(buffer, flags);
		FXERRH_THROW(e);
	}
	else
	{
		FXException e(file, lineno, buffer, FXEXCEPTION_OSSPECIFIC, flags);
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
		s << *e.nestedlist;
	}
	return s;
}
FXStream &operator>>(FXStream &s, FXException &e)
{
	FXchar valid=0;
	s >> valid;
	if(valid)
	{
		e.uniqueId=-1;
		s >> e._message >> e._code >> e._flags;
		if(!e.nestedlist) FXERRHM(e.nestedlist=new QValueList<FXException>);
		s >> *e.nestedlist;
	}
	return s;
}

} // namespace
