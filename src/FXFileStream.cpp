/********************************************************************************
*                                                                               *
*                       F i l e   S t r e a m   C l a s s                       *
*                                                                               *
*********************************************************************************
* Copyright (C) 1997,2004 by Jeroen van der Zijp.   All Rights Reserved.        *
* TnFOX Extensions (C) 2003 Niall Douglas                                       *
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
* $Id: FXFileStream.cpp,v 1.19 2005/01/16 16:06:07 fox Exp $                    *
********************************************************************************/
#include "fxdefs.h"
#include "FXFileStream.h"
#include "FXFile.h"
#include "FXException.h"




/*******************************************************************************/

namespace FX {

// Initialize file stream
FXFileStream::FXFileStream(const FXObject* cont):FXStream(0, cont)
{
}

// Try open file stream
FXbool FXFileStream::open(const FXString& filename,FXStreamDirection save_or_load,unsigned long size){

  // Stream should not yet be open
  if(dir!=FXStreamDead){ fxerror("FXFileStream::open: stream is already open.\n"); }

  FXFile *d;
  FXERRHM(d=new FXFile(filename));
  setDevice(d);

  if(save_or_load==FXStreamLoad)
  {   // Open for read
	  d->open(IO_ReadOnly);
  }
  else
  {   // Open for write
	  d->open(IO_WriteOnly);
  }

  // Do the generic book-keeping
  return FXStream::open(save_or_load, size);
  }


// Close file stream
FXbool FXFileStream::close(){
  device()->close();
  return FXStream::close();
  }


// Close file stream
FXFileStream::~FXFileStream(){
  device()->close();
  delete device();
  setDevice(0);
  }

}
