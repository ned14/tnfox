/********************************************************************************
*                                                                               *
*                    Debug monitoring of memory allocation                      *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2002-2006 by Niall Douglas.   All Rights Reserved.       *
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

/*! \file FXMemDbg.h
\brief Redefines the memory allocation routines

Most libraries don't bother with trapping memory leaks - not so with TnFOX! Of
course this is no substitute for valgrind on Linux, but it's good enough to catch
most leaks. 

<h3>Usage:</h3>
If you wish to trap memory leaks, include this code after all other #include's
in your C++ file but before the main body:
\code
#include <FXMemDbg.h>
#if defined(DEBUG) && !defined(FXMEMDBG_DISABLE)
static const char *_fxmemdbg_current_file_ = __FILE__;
#endif
\endcode
In debug mode only, before your program quits, you'll get a list of all unfreed
memory blocks and where they were allocated if the allocation occurred within
a file covered by the code declaration above.

If you need to undo the allocator macro definitions, simply wrap the relevant
code with #include <FXMemDbg.h> e.g.
\code
#include <FXMemDbg.h>
#if defined(DEBUG) && !defined(FXMEMDBG_DISABLE)
static const char *_fxmemdbg_current_file_ = __FILE__;
#endif
... redefinitions are turned on ...
#include <FXMemDbg.h>
// Redefinitions are now turned off
obj = new(heap, 32) AlignedObject;
#include <FXMemDbg.h>
// Redefinitions are now turned on again
\endcode
FXMemDbg.h is a pure C preprocessor file and hence can be included anywhere safely.

\note If you wish to disable this facility, define FXMEMDBG_DISABLE. Note that
it is disabled when compiled as release anyway.
*/

#undef new
#undef malloc
#undef calloc
#undef realloc
#undef free

#ifdef FXMEMDBG_H
#undef FXMEMDBG_H
#else
#define FXMEMDBG_H

#if defined(DEBUG) && !defined(FXMEMDBG_DISABLE)
#define new new(_fxmemdbg_current_file_, __FUNCTION__, __LINE__, 0, 0)
//#define new(...)		new          (_fxmemdbg_current_file_, __FUNCTION__, __LINE__, __VA_ARGS__)
#define malloc(...)		malloc_dbg   (_fxmemdbg_current_file_, __FUNCTION__, __LINE__, __VA_ARGS__)
#define calloc(...)		calloc_dbg   (_fxmemdbg_current_file_, __FUNCTION__, __LINE__, __VA_ARGS__)
#define realloc(...)	realloc_dbg  (_fxmemdbg_current_file_, __FUNCTION__, __LINE__, __VA_ARGS__)

#endif

#endif
