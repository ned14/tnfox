/********************************************************************************
*                                                                               *
*                           FOX compatibility layer                             *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2006 by Niall Douglas.   All Rights Reserved.            *
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

#include "QThread.h"
#include "FXException.h"

/* When calling TnFOX code as part of implementing the FOX emulations, we must
catch all exceptions thrown as FOX code does not return exceptions. It's up to
the user to ensure the appropriate return or error status is set.

Furthermore we also disable thread cancellation as code written for FOX will
misbehave
*/
#define FXEXCEPTION_FOXCOMPAT1 QThread_DTHold _int_disable_termination; FXERRH_TRY {
#define FXEXCEPTION_FOXCOMPAT2 } FXERRH_CATCH(FX::FXException & /*_int_caught_e*/) { /*FXERRH_REPORT(FX::FXApp::instance(), _int_caught_e);*/ } FXERRH_ENDTRY

namespace FX
{
	static FXuint fxconvertfxiomode(FXuint mode)
	{
		FXuint ret=0;
		if(mode & FXIO::ReadOnly) ret|=IO_ReadOnly;
		if(mode & FXIO::WriteOnly) ret|=IO_WriteOnly;
		if(mode & FXIO::Append) ret|=IO_Append;
		if(mode & FXIO::Truncate) ret|=IO_Truncate;
		if((mode & FXIO::WriteOnly) && !(mode & FXIO::Create))
			fxwarning("fxconvertfxiomode(): FXIO::Create not specified\n");
		if(mode & FXIO::Exclusive)
			fxwarning("fxconvertfxiomode(): FXIO::Exclusive not supported\n");
		if(mode & FXIO::NonBlocking)
			fxwarning("fxconvertfxiomode(): FXIO::NonBlocking not supported\n");
		return ret;
	}
}
