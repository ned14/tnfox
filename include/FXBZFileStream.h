#ifdef FX_FOXCOMPAT

/********************************************************************************
*                                                                               *
*                      B Z F i l e S t r e a m   C l a s s e s                  *
*                                                                               *
*********************************************************************************
* Copyright (C) 1999,2006 by Lyle Johnson. All Rights Reserved.                 *
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
* $Id: FXBZFileStream.h,v 1.5.2.1 2007/09/28 16:42:19 fox Exp $                     *
********************************************************************************/
#ifdef HAVE_BZ2LIB_H
#ifndef FXBZFILESTREAM_H
#define FXBZFILESTREAM_H

#ifndef FXSTREAM_H
#include "FXStream.h"
#endif


namespace FX {

class QFile;

//! \deprecated Use FX::QBZip2Device instead
class FXAPI FXBZFileStream : public FXStream {
	QFile *myfile;
public:

  /// Create BZIP2 file stream
  FXDEPRECATEDEXT FXBZFileStream(const FXObject* cont=NULL);

  /// Open file stream
  bool open(const FXString& filename,FXStreamDirection save_or_load,FXuval size=8192);

  /// Flush buffer
  virtual bool flush();

  /// Close file stream
  virtual bool close();

  /// Clean up
  virtual ~FXBZFileStream();
  };

}


#endif
#endif
#endif
