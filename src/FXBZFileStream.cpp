/********************************************************************************
*                                                                               *
*                      B Z F i l e S t r e a m   C l a s s e s                  *
*                                                                               *
*********************************************************************************
* Copyright (C) 1999,2006 by Lyle Johnson. All Rights Reserved.                 *
* TnFOX Extensions (C) 2006 Niall Douglas                                       *
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
* $Id: FXBZFileStream.cpp,v 1.5 2006/01/22 17:58:18 fox Exp $                   *
********************************************************************************/
#ifdef FX_FOXCOMPAT

#include "fxdefs.h"
#include "QFile.h"
#include "FXBZFileStream.h"
#include "QBZip2Device.h"
#include "FXException.h"

/*******************************************************************************/

namespace FX {

#ifdef HAVE_BZ2LIB_H

// Initialize file stream
FXBZFileStream::FXBZFileStream(const FXObject* cont): myfile(0), FXStream(0, cont){
  }


// Try open file stream
bool FXBZFileStream::open(const FXString& filename,FXStreamDirection save_or_load,FXuval size)
{
	FXERRHM(myfile=new QFile(filename));
	QBZip2Device *d;
	FXERRHM(d=new QBZip2Device(myfile));
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


// Flush buffer
bool FXBZFileStream::flush(){
  bool status;
  int action=ac;
  if(ac!=BZ_FINISH) ac=BZ_FLUSH;
  status=FXStream::flush();
  ac=action;
  return status;
  }

// Close file stream
bool FXBZFileStream::close(){
  device()->close();
  myfile->close();
  return FXStream::close();
  }


// Destructor
FXBZFileStream::~FXBZFileStream(){
  device()->close();
  delete device();
  setDevice(0);
  FXDELETE(myfile);
  }

#endif

}

#endif
