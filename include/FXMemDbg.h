/********************************************************************************
*                                                                               *
*                    Debug monitoring of memory allocation                      *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2002, 2003 by Niall Douglas.   All Rights Reserved.      *
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

#ifndef FXMEMDBG_H
#define FXMEMDBG_H

/*! \file FXMemDbg.h
\brief Redefines the memory allocation routines on MSVC

GNU/Linux has the extremely useful valgrind so if you're on that platform I
suggest you go use that. If you're on MacOS X or FreeBSD, you're probably the
least supported by me (but only because I don't have access to such a system).

MSVC, to Microsoft's credit, actually comes with plenty of memory leak and
corruption detection code in its debug heap - it's just not fully enabled by
default. This of course comes with the price
of the debug heap not behaving like the release one (which famously in MSVC5 (or
was it v4?) realloc() just plain didn't work) but nevertheless, with a small
amount of extra work from you you can get most of the functionality of valgrind
with less than a 5% loss in performance.

<h3>Usage:</h3>
If you wish to trap memory leaks, include this code after all other #includes
in your C++ file but before the main body:
\code
#include <FXMemDbg.h>
#if defined(DEBUG) && !defined(FXMEMDBG_DISABLE)
static const char *_fxmemdbg_current_file_ = __FILE__;
#endif
\endcode
In debug mode only, before your program quits, you'll get a list of all unfreed
memory blocks and where they were allocated. The ones with source file names
are your ones or ones within TnFOX itself. All extension code by me uses FXMemDbg
and it's easy to modify fxdefs.h to have FXMALLOC() use it too.

Lastly, MSVC's debug heap runs a heap validation check every 1024 allocations
and frees and this will usually catch memory corruption, though somewhat belatedly.
You can force a check while trying to localise the cause during debugging using
FXMEMDBG_TESTHEAP. I'd also look into _CrtSetDbgFlag() if you want more enhanced
control (FXProcess sets it for the application).

\note If you wish to disable this facility, define FXMEMDBG_DISABLE. Note that
it is disabled when compiled as release anyway.
*/

#if defined(_MSC_VER) && defined(_DEBUG) && !defined(FXMEMDBG_DISABLE)
// Set up memory tracking (emulates MFC with the MSVC CRT lib)
#include <crtdbg.h>

#define _FXMEMDBG_NEW_ new(1, _fxmemdbg_current_file_, __LINE__)
#define _FXMEMDBG_MALLOC_(x) _malloc_dbg(x, 1, _fxmemdbg_current_file_, __LINE__)

#define new _FXMEMDBG_NEW_
#define malloc(x) _FXMEMDBG_MALLOC_(x)

//! When running under MSVC's debug heap, performs a heap scan and validation
#define FXMEMDBG_TESTHEAP { _ASSERTE(_CrtCheckMemory()); }

#else

#define FXMEMDBG_TESTHEAP

#define FXMEMDBG_DISABLE

#undef FXMEMDBG_H

#endif

#endif
