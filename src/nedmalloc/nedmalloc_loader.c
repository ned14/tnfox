/* Generic Windows Process Patch Loader. Intended for patching nedmalloc in to
replace the MSVCRT allocator in any arbitrary process but could be used for
anything.
(C) 2009 Niall Douglas

Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#include "nedmalloc.h"
#include <stdio.h>
#include <stdlib.h>
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <tchar.h>
#include <psapi.h>
#include "winpatcher_errorh.h"


/* WINBASEAPI HMODULE WINAPI LoadLibraryW(LPCWSTR lpLibFileName) */
typedef HMODULE (WINAPI *LoadLibraryWaddr_t)(LPCWSTR);
typedef HANDLE (WINAPI *GetCurrentThreadaddr_t)(void);
typedef DWORD (WINAPI *SuspendThreadaddr_t)(HANDLE);
typedef int (WINAPI *WinMainCRTStartupaddr_t)(void);
typedef struct InjectionData_t
{
	LoadLibraryWaddr_t LoadLibraryWaddr;
	GetCurrentThreadaddr_t GetCurrentThreadaddr;
	SuspendThreadaddr_t SuspendThreadaddr;
	WinMainCRTStartupaddr_t oldStartupAddr;
	HMODULE moduleh;
	TCHAR dllname[_MAX_PATH+2];
} InjectionData;
static InjectionData injectiondata;

/* Be very, VERY careful in what you do here: make ABSOLUTELY sure
that this code can work correctly in a separate process. Do NOT
use strings or ANY form of static data.
*/
int __fastcall InjectedRoutineEP(InjectionData *p)
{
	WinMainCRTStartupaddr_t oldStartupAddr=p->oldStartupAddr;
	p->moduleh=p->LoadLibraryWaddr(p->dllname);
	/* Pause for debugging */
	p->SuspendThreadaddr(p->GetCurrentThreadaddr());
	return oldStartupAddr();
}
DWORD WINAPI InjectedRoutineRT(InjectionData *p)
{
	p->moduleh=p->LoadLibraryWaddr(p->dllname);
	return 0;
}

static DWORD GetImageMachineType(void *Base)
{
	IMAGE_DOS_HEADER *dosheader=(IMAGE_DOS_HEADER *) Base;
	IMAGE_NT_HEADERS *peheader=0;
	size_t offset=0;
	if(dosheader->e_magic==*(USHORT *)"MZ")
		peheader=(IMAGE_NT_HEADERS *)((char *)dosheader+dosheader->e_lfanew);
	else
		peheader=(IMAGE_NT_HEADERS *) dosheader;
	if(peheader->Signature!=IMAGE_NT_SIGNATURE)
		return 0;
	return peheader->FileHeader.Machine;
}

int main(int argc, char *argv[])
{
	CONTEXT oldcontext={0}, context={0};	/* Keep first to maintain alignment */
	Status ret={SUCCESS};
	STARTUPINFO si={sizeof(STARTUPINFO)};
	PROCESS_INFORMATION pi={0};
	HANDLE childh=0;
	TCHAR commandline[32769], *mylocation, *executable, *newcommandline, s, *q;
#ifdef _DEBUG
	printf("nedmalloc Generic Process Patcher v1.00 (" __DATE__ ")\n"
		   "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n");
#endif
	if(argc<2)
	{
		fprintf(stderr, "Must specify an executable to load!\n");
		return -1;
	}
	/* Pull the unicode command line */
	_tcscpy_s(commandline, sizeof(commandline)/sizeof(TCHAR), GetCommandLine());
	(_tcschr(commandline, 0))[1]=0;
	for(newcommandline=commandline, s=0; *newcommandline && (s || *newcommandline!=' '); newcommandline++)
		if('"'==newcommandline[0] && '\\'!=newcommandline[-1]) s=!s;
	*newcommandline=0;
	mylocation=commandline;
	if('"'==mylocation[0] && '"'==newcommandline[-1])
	{
		mylocation++;
		newcommandline[-1]=0;
	}
	*_tcsrchr(mylocation, '\\')=0;
	executable=++newcommandline;
	for(s=0; *newcommandline && (s || *newcommandline!=' '); newcommandline++)
		if('"'==newcommandline[0] && '\\'!=newcommandline[-1]) s=!s;
	*newcommandline++=0;
	/* Fix up executable if needs be */
	q=_tcschr(executable, 0);
	if('"'==executable[0] && '"'==q[-1])
	{
		executable++;
		q[-1]=0;
	}
#ifdef _DEBUG
	_tprintf(__T("Process to run is: %s\nCommand line is: %s\n\n"), executable, newcommandline);
#endif

	if(!CreateProcess(executable, newcommandline, NULL, NULL, FALSE,
		CREATE_SUSPENDED, NULL, NULL, &si, &pi))
		{ MKSTATUSWIN(ret);	goto badexit; }
	__try
	{	/* The handles returned by CreateProcess are too restricted
		We need more power! */
		HANDLE processh=pi.hProcess, threadh=pi.hThread;
		size_t *entrypoint=0, *firstparam=0, *stack=0;
		void *code=0, *data=0;

		/*if(!DuplicateHandle(GetCurrentProcess(), pi.hProcess, GetCurrentProcess(), &processh,
			STANDARD_RIGHTS_REQUIRED|PROCESS_ALL_ACCESS, FALSE, 0))
		{ MKSTATUSWIN(ret);	goto badexit; }
		if(!DuplicateHandle(GetCurrentProcess(), pi.hThread, GetCurrentProcess(), &threadh,
			STANDARD_RIGHTS_REQUIRED|THREAD_ALL_ACCESS, FALSE, 0))
		{ MKSTATUSWIN(ret);	goto badexit; }
		*/

#ifdef _DEBUG
		_tprintf(__T("Process handle: %p (%u), thread handle: %p (%u)\n"),
			pi.hProcess, pi.dwProcessId, pi.hThread, pi.dwThreadId);
#endif
		/* Make sure that the process is the same type as us */
		{
			HMODULE list[4096], us, them;
			DWORD needed, usmachinetype, themmachinetype;
			char *buffer=(char *) list;
			if(!EnumProcessModules(GetCurrentProcess(), list, 4096, &needed))
			{ MKSTATUSWIN(ret);	goto badexit; }
			us=list[0];
			memset(list, 0, sizeof(list));
			if(!EnumProcessModules(processh, list, 4096, &needed))
			{ MKSTATUSWIN(ret);	goto badexit; }
			them=list[0];
			memset(list, 0, sizeof(list));
			if(!ReadProcessMemory(GetCurrentProcess(), us, buffer, 4096, NULL))
			{ MKSTATUSWIN(ret);	goto badexit; }
			usmachinetype=GetImageMachineType(buffer);
			memset(list, 0, sizeof(list));
			if(!ReadProcessMemory(processh, them, buffer, 4096, NULL))
			{ MKSTATUSWIN(ret);	goto badexit; }
			themmachinetype=GetImageMachineType(buffer);
			if(usmachinetype!=themmachinetype)
			{
				MKSTATUS(ret, ERROR);
				_tcscpy_s(ret.msg, sizeof(ret.msg)/sizeof(TCHAR), __T("Incompatible machine type"));
				MessageBox(NULL, __T("The child process does not have the same machine type as me. Terminating process"), __T("Error"), MB_OK);
				goto badexit;
			}
		}
#if 0
		oldcontext.ContextFlags = context.ContextFlags = CONTEXT_ALL;
		if(!GetThreadContext(threadh, &oldcontext))
		{ MKSTATUSWIN(ret);	goto badexit; }
		context=oldcontext;
#if defined(_M_IX86)
		entrypoint=&context.Eip;
		firstparam=&context.Ecx;
		stack=&context.Esp;
#elif defined(_M_X64)
		entrypoint=&context.Rip;
		firstparam=&context.Rcx;
		stack=&context.Rsp;
#else
#error Unsupported architecture
#endif
#ifdef _DEBUG
		_tprintf(__T("Program starts running at %p\n"), *entrypoint);
#endif
#endif
		/* Set up injection data. We make use of the fact that
		the current implementation of ASLR keeps an identical map
		between processes */
		_tcscpy_s(injectiondata.dllname, sizeof(injectiondata.dllname)/sizeof(TCHAR), mylocation);
		_tcscat_s(injectiondata.dllname, sizeof(injectiondata.dllname)/sizeof(TCHAR), __T("\\nedmalloc.dll"));
		if(INVALID_FILE_ATTRIBUTES==GetFileAttributes(injectiondata.dllname))
		{
			GetCurrentDirectory(sizeof(injectiondata.dllname), injectiondata.dllname);
			_tcscat_s(injectiondata.dllname, sizeof(injectiondata.dllname)/sizeof(TCHAR), __T("\\nedmalloc.dll"));
			if(INVALID_FILE_ATTRIBUTES==GetFileAttributes(injectiondata.dllname))
			{ MKSTATUSWIN(ret);	goto badexit; }
		}
		if(!(injectiondata.LoadLibraryWaddr=(LoadLibraryWaddr_t) GetProcAddress(GetModuleHandle(__T("kernel32.dll")), sizeof(TCHAR)==1 ? "LoadLibraryA": "LoadLibraryW")))
		{ MKSTATUSWIN(ret);	goto badexit; }
		if(!(injectiondata.GetCurrentThreadaddr=(GetCurrentThreadaddr_t) GetProcAddress(GetModuleHandle(__T("kernel32.dll")), "GetCurrentThread")))
		{ MKSTATUSWIN(ret);	goto badexit; }
		if(!(injectiondata.SuspendThreadaddr=(SuspendThreadaddr_t) GetProcAddress(GetModuleHandle(__T("kernel32.dll")), "SuspendThread")))
		{ MKSTATUSWIN(ret);	goto badexit; }
		/*injectiondata.oldStartupAddr=(WinMainCRTStartupaddr_t) *entrypoint;*/
		/* Allocate me some memory in the process for some code and
		for some data and copy it in */
		if(!(code=VirtualAllocEx(processh, 0, 4096, MEM_TOP_DOWN|MEM_COMMIT, PAGE_EXECUTE_READWRITE)))
		{ MKSTATUSWIN(ret);	goto badexit; }
		if(!(data=VirtualAllocEx(processh, 0, sizeof(injectiondata), MEM_TOP_DOWN|MEM_COMMIT, PAGE_READWRITE)))
		{ MKSTATUSWIN(ret);	goto badexit; }
		if(!WriteProcessMemory(processh, code, InjectedRoutineRT, 4096, NULL))
		{ MKSTATUSWIN(ret);	goto badexit; }
		if(!WriteProcessMemory(processh, data, &injectiondata, sizeof(injectiondata), NULL))
		{ MKSTATUSWIN(ret);	goto badexit; }
#ifdef _DEBUG
		_tprintf(__T("Injected code segment at %p and data segment at %p\n"), code, data);
#endif
#if 0	/* This is the "reset PE entry point to my code" trick */
		/* Now reset the context to point at my code instead */
		*entrypoint=(size_t) code;
		*firstparam=(size_t) data;	/* For __fastcall/x64 only */
		if(0) /* __stdcall only */
		{	/* Knock the stack down one for the data pointer */
			*stack-=sizeof(size_t);
			/* Write the data pointer onto the stack */
			if(!WriteProcessMemory(processh, (void *) *firstparam, &data, sizeof(void *), NULL))
			{ MKSTATUSWIN(ret);	goto badexit; }
		}
		if(!SetThreadContext(threadh, &context))
		{ MKSTATUSWIN(ret);	goto badexit; }
#else	/* This is the "create remote thread before process starts" trick */
		{
			HANDLE childthreadh=0;
			DWORD threadid=0;
			if(!(childthreadh=CreateRemoteThread(processh, NULL, 0, code, data, 0, &threadid)))
			{ MKSTATUSWIN(ret);	goto badexit; }
#ifdef _DEBUG
			_tprintf(__T("Remote thread launched with threadid=%d\n"), threadid);
#endif
			if(WAIT_TIMEOUT==WaitForSingleObject(childthreadh, 5000))
			{
				MKSTATUS(ret, ERROR);
				_tcscpy_s(ret.msg, sizeof(ret.msg)/sizeof(TCHAR), __T("Failed to inject"));
				MessageBox(NULL, __T("Failed to inject DLL. This happens sometimes"), __T("Error"), MB_OK);
				goto badexit;
			}
			CloseHandle(childthreadh);
		}
#endif
		/* Rock and roll */
		if(!ResumeThread(threadh))
		{ MKSTATUSWIN(ret);	goto badexit; }
		do
		{
			Sleep(100);
			if(!ReadProcessMemory(processh, data, &injectiondata, sizeof(injectiondata), NULL))
			{ MKSTATUSWIN(ret);	goto badexit; }
		} while(!injectiondata.moduleh);
#ifdef _DEBUG
		_tprintf(__T("Injected code has loaded DLL to %p\n"), injectiondata.moduleh);
#endif
		/* The DLL is injected. Now restart the process init */
		/*if(!SetThreadContext(threadh, &oldcontext))
		{ MKSTATUSWIN(ret);	goto badexit; }*/
		CloseHandle(pi.hProcess);
		pi.hProcess=0;
	}
	__finally
	{
		if(pi.hProcess)
		{
#ifdef _DEBUG
			_tprintf(__T("Terminating process %p\n"), pi.hProcess);
#endif
			TerminateProcess(pi.hProcess, 1);
			CloseHandle(pi.hProcess);
			pi.hProcess=0;
		}
	}
	return 0;
badexit:
	{
		TCHAR buffer[4096], *p;
		const char *s;
		_stprintf_s(buffer, sizeof(buffer), __T("Error code %d (%s) at "), ret.code, ret.msg);
		for(p=_tcschr(buffer, 0), s=ret.sourcefile; *s; p++, s++)
			*p=*s;
		_stprintf_s(p, sizeof(buffer), __T(":%d\n"), ret.sourcelineno);
		MessageBox(NULL, buffer, __T("Error"), MB_OK);
	}
	return -1;
}
