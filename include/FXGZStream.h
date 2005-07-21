/********************************************************************************
*                                                                               *
*                        G Z S t r e a m  C l a s s e s                         *
*                                                                               *
*********************************************************************************
* Copyright (C) 2002,2005 by Sander Jansen.   All Rights Reserved.              *
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
* $Id: FXGZStream.h,v 1.9 2005/01/16 16:06:06 fox Exp $                         *
********************************************************************************/
#ifdef HAVE_ZLIB_H
#ifndef FXGZSTREAM_H
#define FXGZSTREAM_H

#ifndef FXSTREAM_H
#include "FXStream.h"
#endif


namespace FX {

class FXFile;

//! \deprecated Use FX::FXGZipDevice instead
class FXAPI FXGZFileStream : public FXStream {
	FXFile *myfile;
public:

  /// Create GZIP compressed file stream
  FXDEPRECATEDEXT FXGZFileStream(const FXObject* cont=NULL);

  /// Open file stream
  FXbool open(const FXString& filename,FXStreamDirection save_or_load,unsigned long size=8192);

  /// Close file stream
  virtual FXbool close();

  /// Clean up
  virtual ~FXGZFileStream();
  };

}


#endif
#endif
