/********************************************************************************
*                                                                               *
*                        G Z S t r e a m   C l a s s e s                        *
*                                                                               *
*********************************************************************************
* Copyright (C) 2002,2004 by Sander Jansen.   All Rights Reserved.              *
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
* $Id: FXGZStream.cpp,v 1.10.2.1 2005/03/18 05:36:12 fox Exp $                      *
********************************************************************************/
#ifdef FX_FOXCOMPAT

#include "fxdefs.h"
#include "FXFile.h"
#include "FXGZFileStream.h"
#include "QGZipDevice.h"
#include "FXException.h"

/*******************************************************************************/

namespace FX {

#ifdef HAVE_ZLIB_H

// Initialize file stream
FXGZFileStream::FXGZFileStream(const FXObject* cont): myfile(0), FXStream(0, cont){
  }


// Try open file stream
FXbool FXGZFileStream::open(const FXString& filename,FXStreamDirection save_or_load,unsigned long size)
{
	FXERRHM(myfile=new FXFile(filename));
	QGZipDevice *d;
	FXERRHM(d=new QGZipDevice(myfile));
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
FXbool FXGZFileStream::close(){
  device()->close();
  myfile->close();
  return FXStream::close();
  }


// Destructor
FXGZFileStream::~FXGZFileStream(){
  device()->close();
  delete device();
  setDevice(0);
  FXDELETE(myfile);
  }

#endif

}

#endif
