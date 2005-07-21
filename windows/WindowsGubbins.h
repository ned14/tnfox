/********************************************************************************
*                                                                               *
*                  Microsoft Windows header files include                       *
*                                                                               *
*********************************************************************************
* Copyright (C) 2002,2003 by Niall Douglas.   All Rights Reserved.              *
*   NOTE THAT I NIALL DOUGLAS DO NOT PERMIT ANY OF MY CODE USED UNDER THE GPL   *
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

#ifndef _WindowsGubbins_h_
#define _WindowsGubbins_h_

#ifndef _WINDOWS
#error This file should only ever be included on Windows builds
#endif

// We want across the board APIs
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x600

#ifndef FXWINDOWSGUBBINS_INCLUDE_EVERYTHING
// Cut out large portions
#define WIN32_LEAN_AND_MEAN
#endif
#include "windows.h"

// Define a special kind of error handler for Win32
#ifdef FXEXCEPTION_DISABLESOURCEINFO
#define FXERRGWIN(code, flags)				{ FX::FXException::int_throwWinError(0, 0, code, flags); }
#define FXERRGWINFN(code, flags, filename)	{ FX::FXException::int_throwWinError(0, 0, code, flags, filename); }
#else
#define FXERRGWIN(code, flags)				{ FX::FXException::int_throwWinError(__FILE__, __LINE__, code, flags); }
#define FXERRGWINFN(code, flags, filename)	{ FX::FXException::int_throwWinError(__FILE__, __LINE__, code, flags, filename); }
#endif
#ifdef DEBUG
#define FXERRHWIN(exp)								{ DWORD __errcode=(DWORD)(exp); if(!__errcode || FX::FXException::int_testCondition()) FXERRGWIN(GetLastError(), 0); }
#define FXERRHWINFN(exp, filename)					{ DWORD __errcode=(DWORD)(exp); if(!__errcode || FX::FXException::int_testCondition()) FXERRGWINFN(GetLastError(), 0, filename); }
#define FXERRHWIN2(exp, getlasterror)				{ DWORD __errcode=(DWORD)(exp); if(!__errcode || FX::FXException::int_testCondition()) FXERRGWIN(getlasterror, 0); }
#define FXERRHWIN2FN(exp, getlasterror, filename)	{ DWORD __errcode=(DWORD)(exp); if(!__errcode || FX::FXException::int_testCondition()) FXERRGWINFN(getlasterror, 0, filename); }
#else
#define FXERRHWIN(exp)								{ DWORD __errcode=(DWORD)(exp); if(!__errcode) FXERRGWIN(GetLastError(), 0); }
#define FXERRHWINFN(exp, filename)					{ DWORD __errcode=(DWORD)(exp); if(!__errcode) FXERRGWINFN(GetLastError(), 0, filename); }
#define FXERRHWIN2(exp, getlasterror)				{ DWORD __errcode=(DWORD)(exp); if(!__errcode) FXERRGWIN(getlasterror, 0); }
#define FXERRHWIN2FN(exp, getlasterror, filename)	{ DWORD __errcode=(DWORD)(exp); if(!__errcode) FXERRGWINFN(getlasterror, 0, filename); }
#endif
#define FXERRGCOM(code, flags) { FXERRMAKE(e, tr("COM error 0x%1 occurred").arg(code), FXEXCEPTION_OSSPECIFIC, flags); \
	FXERRH_THROW(e); }
#define FXERRHCOM(exp)	{ HRESULT __ret=(exp); if(NOERROR!=__ret) FXERRGCOM(__ret, 0); }

#endif
