/********************************************************************************
*                                                                               *
*                       F i l e   S t r e a m   C l a s s                       *
*                                                                               *
*********************************************************************************
* Copyright (C) 1997,2005 by Jeroen van der Zijp.   All Rights Reserved.        *
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
* $Id: FXFileStream.h,v 1.11 2005/02/08 03:23:28 fox Exp $                      *
********************************************************************************/
#ifndef FXFILESTREAM_H
#define FXFILESTREAM_H

#ifndef FXSTREAM_H
#include "FXStream.h"
#endif

namespace FX {

/*! \class FXFileStream
\deprecated For compatibility with FOX code only
*/
class FXAPI FXFileStream : public FXStream {
public:

  /// Create file store
  FXFileStream(const FXObject* cont=NULL);

  /**
  * Open binary data file stream; allocate a buffer of the given size
  * for the file I/O; the buffer must be at least 16 bytes.
  */
  FXbool open(const FXString& filename,FXStreamDirection save_or_load,unsigned long size=8192);

  /// Close file store
  virtual FXbool close();

  /// Destructor
  virtual ~FXFileStream();
  };

}

#endif
