/* Generic Windows Process Patcher. Intended for patching nedmalloc in to
replace the MSVCRT allocator but could be used for anything.
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
#include "embedded_printf.h"
#include <stdlib.h>
#include <assert.h>
#ifdef WIN32
#include <tchar.h>
#include <process.h>
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <psapi.h>
#include "winpatcher_errorh.h"


/* TODO list:

* Patch GetProcAddress to return the patched address
*/

#if defined(__cplusplus)
#if !defined(NO_WINPATCHER_NAMESPACE)
namespace winpatcher {
#else
extern "C" {
#endif
#endif

/* If you want to learn lots more about the vagueries of Win32 PE dynamic linking,
see http://msdn.microsoft.com/en-us/magazine/cc301808.aspx. Excerpt from that:

The anchor of the imports data is the IMAGE_IMPORT_DESCRIPTOR structure. The DataDirectory
entry for imports points to an array of these structures. There's one IMAGE_IMPORT_DESCRIPTOR
for each imported executable. The end of the IMAGE_IMPORT_DESCRIPTOR array is indicated by an
entry with fields all set to 0. Figure 5 shows the contents of an IMAGE_IMPORT_DESCRIPTOR.

Each IMAGE_IMPORT_DESCRIPTOR typically points to two essentially identical arrays. These
arrays have been called by several names, but the two most common names are the Import Address
Table (IAT) and the Import Name Table (INT).

Both arrays have elements of type IMAGE_THUNK_DATA, which is a pointer-sized union. Each
IMAGE_THUNK_DATA element corresponds to one imported function from the executable. The ends of
both arrays are indicated by an IMAGE_THUNK_DATA element with a value of zero. The
IMAGE_THUNK_DATA union is a DWORD with these interpretations:

DWORD Function;       // Memory address of the imported function
DWORD Ordinal;        // Ordinal value of imported API
DWORD AddressOfData;  // RVA to an IMAGE_IMPORT_BY_NAME with
                      // the imported API name
DWORD ForwarderString;// RVA to a forwarder string

The IMAGE_THUNK_DATA structures within the IAT lead a dual-purpose life. In the executable
file, they contain either the ordinal of the imported API or an RVA to an IMAGE_IMPORT_BY_NAME
structure. The IMAGE_IMPORT_BY_NAME structure is just a WORD, followed by a string naming the
imported API. The WORD value is a "hint" to the loader as to what the ordinal of the imported
API might be. When the loader brings in the executable, it overwrites each IAT entry with the
actual address of the imported function. This a key point to understand before proceeding. I
highly recommend reading Russell Osterlund's article in this issue which describes the steps
that the Windows loader takes.

Before the executable is loaded, is there a way you can tell if an IMAGE_THUNK_DATA structure
contains an import ordinal, as opposed to an RVA to an IMAGE_IMPORT_BY_NAME structure? The key
is the high bit of the IMAGE_THUNK_DATA value. If set, the bottom 31 bits (or 63 bits for a 64-bit
executable) is treated as an ordinal value. If the high bit isn't set, the IMAGE_THUNK_ DATA value
is an RVA to the IMAGE_IMPORT_BY_NAME.

The other array, the INT, is essentially identical to the IAT. It's also an array of
IMAGE_THUNK_DATA structures. The key difference is that the INT isn't overwritten by the loader
when brought into memory. Why have two parallel arrays for each set of APIs imported from a DLL?
The answer is in a concept called binding. When the binding process rewrites the IAT in the file
(I'll describe this process later), some way of getting the original information needs to remain.
The INT, which is a duplicate copy of the information, is just the ticket.
*/


typedef struct ModuleListItem_t
{
	const char *into;
	const char *from;	/* zero means this module */
} ModuleListItem;

typedef struct SymbolListItem_t
{
	struct Replace_t
	{
		const char *name;
		HMODULE moduleBase;
		char moduleName[_MAX_PATH+2];
		PROC addr;
	} replace;
	const ModuleListItem *modules;	/* zero means wherever the replace symbol lives */
	struct With_t
	{
		const char *name;
		PROC addr;
	} with;
} SymbolListItem;


/* Little helper function to return the module base (a HMODULE)
given some address within that module. Assumes that the NT kernel
maps an entire module at once with one TLB entry */
static HMODULE ModuleFromAddress(void *addr) THROWSPEC
{
	MEMORY_BASIC_INFORMATION mbi={0};
	return ((VirtualQuery(addr, &mbi, sizeof(mbi)) != 0) ? (HMODULE) mbi.AllocationBase : NULL);
}

/* Little helper function to deindirect the PE DLL linking mechanism
This is architecture dependent */
static PROC DeindirectAddress(PROC _addr) THROWSPEC
{
	unsigned char *addr=(unsigned char *) *_addr;
#if defined(_M_IX86)
	if(0xe9==addr[0])
	{	/* We're seeing a jmp rel32 inserted under Edit & Continue */
		unsigned int offset=*(unsigned int *)(addr+1);
		addr+=offset+5;
	}
	if(0xff==addr[0] && 0x25==addr[1])
	{	/* This is a jmp ptr, so dword[2:6] is where to load the address from */
		addr=(char *)(**(unsigned int **)(addr+2));
	}
#elif defined(_M_X64)
	if(0xff==addr[0] && 0x25==addr[1])
	{	/* This is a jmp qword ptr, so dword[2:6] is the offset to where to load the address from */
		unsigned int offset=*(unsigned int *)(addr+2);
		addr+=offset+6;
		addr=(char *)(*(size_t *)(addr));
	}
#endif
	return (PROC) addr;
}

/* Little helper function for sending stuff to the debugger output
seeing as fprintf et al are completely unavailable to us */
static void putc(void *p, char c) THROWSPEC { *(*((char **)p))++ = c; }
static HANDLE debugfile=INVALID_HANDLE_VALUE;
static void DebugPrint(const char *fmt, ...) THROWSPEC
{
#if defined(_DEBUG) && defined(USE_DEBUGGER_OUTPUT)
	char buffer[16384];
	char *s=buffer;
	HANDLE stdouth=GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD len;
	DWORD written=0;

	va_list va;
	va_start(va,fmt);
	tfp_format(&s,putc,fmt,va);
	putc(&s,0);
	va_end(va);
	len=(DWORD)(strchr(buffer, 0)-buffer);
	OutputDebugStringA(buffer);
	if(stdouth && stdouth!=INVALID_HANDLE_VALUE)
		WriteFile(stdouth, buffer, len, &written, NULL);
#if 1
	if(INVALID_HANDLE_VALUE==debugfile)
	{
		debugfile=CreateFile(__T("C:\\nedmalloc.log"), GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, 
			NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	}
	if(INVALID_HANDLE_VALUE!=debugfile)
		WriteFile(debugfile, buffer, len, &written, NULL);
#endif
#endif
}

/* Our own implementation of DbgHelp's ImageDirectoryEntryToData()
DbgHelp unfortunately calls malloc() during its DllMain which is a
big no-no in our situation */
PVOID MyImageDirectoryEntryToData(PVOID Base, BOOLEAN MappedAsImage, USHORT DirectoryEntry, PULONG Size )
{
	IMAGE_DOS_HEADER *dosheader=(IMAGE_DOS_HEADER *) Base;
	IMAGE_NT_HEADERS *peheader=0;
	void *ret=0;
	size_t offset=0;
	if(Size) *Size=0;
	if(dosheader->e_magic==*(USHORT *)"MZ")
		peheader=(IMAGE_NT_HEADERS *)((char *)dosheader+dosheader->e_lfanew);
	else
		peheader=(IMAGE_NT_HEADERS *) dosheader;
	if(peheader->Signature!=IMAGE_NT_SIGNATURE)
	{
		SetLastError(ERROR_INVALID_DATA);
		return 0;
	}
	offset=peheader->OptionalHeader.DataDirectory[DirectoryEntry].VirtualAddress;
	if(offset)
	{
		ret=(void *)((char *) Base+offset);
		if(Size) *Size=peheader->OptionalHeader.DataDirectory[DirectoryEntry].Size;
	}
	return ret;
}


/* Modifies the import table of a loaded module
     Returns: The number of entries modified
     moduleBase: Where the PE module is living in memory
     importModuleName: Name of the PE module whose exports we wish to modify
	 fnToReplace: Address of function to replace
	 fnNew: Replacement address
*/
static Status ModifyModuleImportTableForI(HMODULE moduleBase, const char *importModuleName, SymbolListItem *sli, int patchin) THROWSPEC
{
	Status ret = { SUCCESS };
	ULONG size;
	PIMAGE_THUNK_DATA thunk = 0;
	PIMAGE_IMPORT_DESCRIPTOR desc = 0;
	PROC replaceaddr = patchin ? sli->replace.addr : sli->with.addr, withaddr = patchin ? sli->with.addr : sli->replace.addr;

	/* Find the import table of the module loaded at hmodCaller */
	desc = (PIMAGE_IMPORT_DESCRIPTOR) MyImageDirectoryEntryToData(moduleBase, TRUE, IMAGE_DIRECTORY_ENTRY_IMPORT, &size);
	if (!desc)
		return MKSTATUS(ret, SUCCESS);  /* This module has no import section */

	/* Find the import descriptor containing references to the module we want. */
	for (; desc->Name; desc++) {
		PSTR modname = (PSTR)((PBYTE) moduleBase + desc->Name);
		if (0==lstrcmpiA(modname, importModuleName)) 
			break;
	}
	if (!desc->Name)
		return MKSTATUS(ret, SUCCESS);	/* Module we want not found */

	/* Get the import address table (IAT) for the functions imported from our wanted module */
	thunk = (PIMAGE_THUNK_DATA)((PBYTE) moduleBase + desc->FirstThunk);

	/* Find and replace current function address with new function address */
	for (; thunk->u1.Function; thunk++) {
		/* Get the address of the function address */
		PROC *fn = (PROC *) &thunk->u1.Function;

		/* Is this the function we're looking for? */
		BOOL found = (*fn == replaceaddr);

		if (found) {
			/* The addresses match; change the import section address. */
			MEMORY_BASIC_INFORMATION mbi={0};
#if defined(_DEBUG)
			{
				char moduleBaseName[_MAX_PATH+2];
				GetModuleBaseNameA(GetCurrentProcess(), moduleBase, moduleBaseName, sizeof(moduleBaseName)-1);
				DebugPrint("Winpatcher: Replacing function pointer %p (%s:%s) with %p (%s) at %p in module %p (%s)\n",
					*fn, importModuleName, sli->replace.name, withaddr, sli->with.name, fn, moduleBase, moduleBaseName);
			}
#endif
			/*if(!WriteProcessMemory(GetCurrentProcess(), fn, &withaddr, sizeof(withaddr), NULL))
				return MKSTATUSWIN(ret);*/
			if(!VirtualQuery(fn, &mbi, sizeof(mbi)))
				return MKSTATUSWIN(ret);
			if(!(mbi.Protect & PAGE_READWRITE))
			{
#if defined(_DEBUG)
				DebugPrint("Winpatcher: Setting PAGE_WRITECOPY on module %p, region %p length %u\n", moduleBase, mbi.BaseAddress, mbi.RegionSize);
#endif
				if(!VirtualProtect(mbi.BaseAddress, mbi.RegionSize, PAGE_WRITECOPY, &mbi.Protect))
					return MKSTATUSWIN(ret);
			}
			*fn=withaddr;
			return MKSTATUS(ret, SUCCESS+1);
		}
	}
	return MKSTATUS(ret, SUCCESS);	/* Function we want not found */
}

/* Modifies all symbols in the import table of a loaded module
     Returns: The number of entries modified
     moduleBase: The PE module to patch
*/
static Status ModifyModuleImportTableFor(HMODULE moduleBase, SymbolListItem *sli, int patchin) THROWSPEC
{
	Status ret={SUCCESS};
	int count=0;
	for(; sli->replace.name; sli++, count++)
	{
		if(sli->modules)
		{
			const ModuleListItem *module;
			for(module=sli->modules; module->into; module++)
			{
				sli->replace.moduleBase=0;
				if(!GetModuleHandleExA(0, module->into, &sli->replace.moduleBase))
				{	/* Not loaded so move to next one */
					ret=MKSTATUSWIN(ret);
					continue;
				}
				if(!(sli->replace.addr=GetProcAddress(sli->replace.moduleBase, sli->replace.name)))
					return MKSTATUSWIN(ret);
				if(!module->from)
				{
					if(!sli->with.addr)
						abort();	/* If we are not specifying the module it must be a local symbol */
				}
				else
				{
					HMODULE withBase=0;
					if(!GetModuleHandleExA(0, module->from, &withBase))
						return MKSTATUSWIN(ret);
					if(!(sli->with.addr=GetProcAddress(withBase, sli->with.name)))
						return MKSTATUSWIN(ret);
				}
				if((ret=ModifyModuleImportTableForI(moduleBase, module->into, sli, patchin), ret.code)<0)
					return ret;
			}
		}
		else
		{
			if(!sli->replace.moduleBase)
			{
				if(!sli->replace.addr || !sli->with.addr)
					abort();	/* If you are not specifying modules you must specify symbols */
				sli->replace.addr=DeindirectAddress(sli->replace.addr);
				sli->replace.moduleBase=ModuleFromAddress(sli->replace.addr);
				if(!GetModuleBaseNameA(GetCurrentProcess(), sli->replace.moduleBase, sli->replace.moduleName, sizeof(sli->replace.moduleName)-1))
					return MKSTATUSWIN(ret);
			}
			if((ret=ModifyModuleImportTableForI(moduleBase, sli->replace.moduleName, sli, patchin), ret.code)<0)
				return ret;
		}
	}
	return MKSTATUS(ret, count);
}

/* Modifies all symbols in all loaded modules
     Returns: The number of entries modified
*/
NEDMALLOCEXTSPEC Status WinPatcher(SymbolListItem *symbollist, int patchin) THROWSPEC;
Status WinPatcher(SymbolListItem *symbollist, int patchin) THROWSPEC
{
	int count=0;
	Status ret={SUCCESS};
	HMODULE myModuleBase=ModuleFromAddress(WinPatcher), *module=0, modulelist[4096];
	DWORD modulelistlen=sizeof(modulelist), modulelistlenneeded=0;

	if(!EnumProcessModules(GetCurrentProcess(), modulelist, modulelistlen, &modulelistlenneeded))
		return MKSTATUSWIN(ret);
	for(module=modulelist; module<modulelist+(modulelistlenneeded/sizeof(HMODULE)); module++)
	{
		char moduleBaseName[_MAX_PATH+2];
		GetModuleBaseNameA(GetCurrentProcess(), *module, moduleBaseName, sizeof(moduleBaseName)-1);
		if(*module==myModuleBase)
			continue;	/* Not us or we'd break our patch table */
#if defined(_DEBUG)
		DebugPrint("Winpatcher: Scanning module %p (%s) for things to %s ...\n", *module, moduleBaseName, patchin ? "patch" : "depatch");
#endif
		if((ret=ModifyModuleImportTableFor(*module, symbollist, patchin), ret.code)<0)
			return ret;
		count+=ret.code;
	}
	return MKSTATUS(ret, count);
}












NEDMALLOCEXTSPEC int PatchInNedmallocDLL(void) THROWSPEC;
NEDMALLOCEXTSPEC int DepatchInNedmallocDLL(void) THROWSPEC;
/* A LoadLibrary() wrapper */
static HMODULE WINAPI LoadLibraryA_winpatcher(LPCSTR lpLibFileName)
{
	HMODULE ret=LoadLibraryA(lpLibFileName);
#if defined(_DEBUG)
	DebugPrint("Winpatcher: LoadLibraryA intercepted\n");
#endif
#ifdef REPLACE_SYSTEM_ALLOCATOR
	PatchInNedmallocDLL();
#endif
	return ret;
}
static HMODULE WINAPI LoadLibraryW_winpatcher(LPCWSTR lpLibFileName)
{
	HMODULE ret=LoadLibraryW(lpLibFileName);
#if defined(_DEBUG)
	DebugPrint("Winpatcher: LoadLibraryW intercepted\n");
#endif
#ifdef REPLACE_SYSTEM_ALLOCATOR
	PatchInNedmallocDLL();
#endif
	return ret;
}
/* The patch table: replace the specified symbols in the specified modules with the
   specified replacements. Format is:

   <--            what to replace               -->  <--  in what  -->  <--          replacements            -->
   {{ "<linker symbol>", 0, "", 0|<function addr> }, 0|<which modules>, { "<linker symbol>", <function addr> } },

   If you specify <which modules> then <function addr> is overwritten with whatever
   GetProcAddress() returns for <linker symbol>. On the hand, and usually much more
   usefully, leaving <which modules> at zero and writing in whatever the dynamic
   linker resolves for <function addr> lets the patcher look up which modules the
   dynamic linker used and to patch that instead. This helps when the implementing
   module is not constant, but it does require that the enclosing DLL is using the
   same version as everything else in the process.

   Note that not specifying modules introduces x86 or x64 dependent code and specific
   assumptions about how MSVC implements the PE image spec. The only fully portable
   method is to specify modules, plus specifying modules covers executables not built
   using the same version of MSVCRT.
*/
static const ModuleListItem modules[]={
	/* Release and Debug MSVC6 CRTs */
	{ "MSVCRT.DLL", 0 }, { "MSVCRTD.DLL", 0 },
	/* Release and Debug MSVC7.0 CRTs */
	{ "MSVCR70.DLL", 0 }, { "MSVCR70D.DLL", 0 },
	/* Release and Debug MSVC7.1 CRTs */
	{ "MSVCR71.DLL", 0 }, { "MSVCR71D.DLL", 0 },
	/* Release and Debug MSVC8 CRTs */
	{ "MSVCR80.DLL", 0 }, { "MSVCR80D.DLL", 0 },
	/* Release and Debug MSVC9 CRTs */
	{ "MSVCR90.DLL", 0 }, { "MSVCR90D.DLL", 0 },
	{ 0, 0 }
};
static const ModuleListItem kernelmodule[]={
	{ "KERNEL32.DLL", 0 },
	{ 0, 0 }
};
static SymbolListItem nedmallocpatchtable[]={
	{ { "malloc",  0, "", 0/*(PROC) malloc */ }, modules, { "nedmalloc",  (PROC) nedmalloc  } },
	{ { "calloc",  0, "", 0/*(PROC) calloc */ }, modules, { "nedcalloc",  (PROC) nedcalloc  } },
	{ { "realloc", 0, "", 0/*(PROC) realloc*/ }, modules, { "nedrealloc", (PROC) nedrealloc } },
	{ { "free",    0, "", 0/*(PROC) free   */ }, modules, { "nedfree",    (PROC) nedfree    } },
#ifdef REPLACE_SYSTEM_ALLOCATOR
	{ { "LoadLibraryA", 0, "", 0 }, kernelmodule, { "LoadLibraryA_winpatcher", (PROC) LoadLibraryA_winpatcher } },
	{ { "LoadLibraryW", 0, "", 0 }, kernelmodule, { "LoadLibraryW_winpatcher", (PROC) LoadLibraryW_winpatcher } },
#endif
	{ { 0, 0, "", 0 }, 0, { 0, 0 } }
};
int PatchInNedmallocDLL(void) THROWSPEC
{
	Status ret={SUCCESS};
	ret=WinPatcher(nedmallocpatchtable, 1);
	if(ret.code<0)
	{
#if defined(_DEBUG)
		DebugPrint("Winpatcher: DLL Process Attach Failed with error code %d (%s) at %s:%d\n", ret.code, ret.msg, ret.sourcefile, ret.sourcelineno);
#endif
		return FALSE;
	}
	return TRUE;
}
int DepatchInNedmallocDLL(void) THROWSPEC
{
	Status ret={SUCCESS};
	ret=WinPatcher(nedmallocpatchtable, 0);
	if(ret.code<0)
	{
#if defined(_DEBUG)
		DebugPrint("Winpatcher: DLL Process Detach Failed with error code %d (%s) at %s:%d\n", ret.code, ret.msg, ret.sourcefile, ret.sourcelineno);
#endif
		return FALSE;
	}
	return TRUE;
}

/* The DLL entry function for nedmalloc. This is called by the dynamic linker
before absolutely everything else - including the CRT */
BOOL WINAPI
_DllMainCRTStartup(
        HANDLE  hDllHandle,
        DWORD   dwReason,
        LPVOID  lpreserved
        );
//#pragma optimize("", off)
/* We split DllPreMainCRTStartup to avoid an annoying bug on the x64 compiler in /O2
whereby it inserts a security cookie check before we've initialised support for it, thus
provoking a failure */
static __declspec(noinline) BOOL DllPreMainCRTStartup2(HMODULE myModuleBase, DWORD dllcode, LPVOID *isTheDynamicLinker)
{
	BOOL ret=TRUE;
	if(DLL_PROCESS_ATTACH==dllcode)
	{
#ifdef REPLACE_SYSTEM_ALLOCATOR
		if(!PatchInNedmallocDLL())
			return FALSE;
#endif
	}
	/* Invoke the CRT's handler which does atexit() etc */
	ret=_DllMainCRTStartup(myModuleBase, dllcode, isTheDynamicLinker);
	if(DLL_THREAD_DETACH==dllcode)
	{	/* Destroy the thread cache for the system pool at least */
		neddisablethreadcache(0);
	}
	else if(DLL_PROCESS_DETACH==dllcode)
	{
#ifdef REPLACE_SYSTEM_ALLOCATOR
		if(!DepatchInNedmallocDLL())
			return FALSE;
#endif
	}
	return ret;
}

BOOL APIENTRY DllPreMainCRTStartup(HMODULE myModuleBase, DWORD dllcode, LPVOID *isTheDynamicLinker)
{
	if(DLL_PROCESS_ATTACH==dllcode)
		__security_init_cookie();	/* For /GS support */
	return DllPreMainCRTStartup2(myModuleBase, dllcode, isTheDynamicLinker);
}

#if defined(__cplusplus)
}
#endif
#endif
