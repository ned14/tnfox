/********************************************************************************
*                                                                               *
*             D y n a m i c   L i n k   L i b r a r y   S u p p o r t           *
*                                                                               *
*********************************************************************************
* Copyright (C) 2002,2004 by Jeroen van der Zijp.   All Rights Reserved.        *
*********************************************************************************
* This library is free software; you can redistribute it and/or                 *
* modify it under the terms of the GNU Lesser General Public                    *
* License as published by the Free Software Foundation; either                  *
* version 2.1 of the License, or (at your option) any later version.            *
*                                                                               *
* This library is distributed in the hope that it will be useful,               *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU             *
* Lesser General Public License for more details.                               *
*                                                                               *
* You should have received a copy of the GNU Lesser General Public              *
* License along with this library; if not, write to the Free Software           *
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.    *
*********************************************************************************
* $Id: FXDLL.cpp,v 1.11 2004/02/08 17:29:06 fox Exp $                            *
********************************************************************************/
#ifndef BUILDING_TCOMMON

#include "FXProcess.h"


#ifndef RTLD_GLOBAL
#define RTLD_GLOBAL 0           // Does not exist on DEC
#endif




/*******************************************************************************/

namespace FX {


// Open DLL and return dllhandle to it
void* fxdllOpen(const FXchar *dllname)
{
	return (void *) new FXProcess::dllHandle(FXProcess::dllLoad(dllname));
}

// Close DLL of given dllhandle
void fxdllClose(void* dllhandle)
{
	FXProcess::dllHandle *h=(FXProcess::dllHandle *) dllhandle;
	delete h;
}


// Return address of the given symbol in library dllhandle
void* fxdllSymbol(void* dllhandle,const FXchar* dllsymbol)
{
	if(dllhandle && dllsymbol)
	{
		FXProcess::dllHandle *h=(FXProcess::dllHandle *) dllhandle;
		return FXProcess::dllResolveBase(*h, dllsymbol);
	}
	return NULL;
}

}

#endif
