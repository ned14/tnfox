/********************************************************************************
*                                                                               *
*                       F i l e   S t r e a m   C l a s s                       *
*                                                                               *
*********************************************************************************
* Copyright (C) 1997,2006 by Jeroen van der Zijp.   All Rights Reserved.        *
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
* $Id: FXFileStream.h,v 1.15 2006/01/22 17:58:01 fox Exp $                      *
********************************************************************************/
#ifndef FXFILESTREAM_H
#define FXFILESTREAM_H

#ifndef FXSTREAM_H
#include "FXStream.h"
#endif

namespace FX {


/// File Store Definition
class FXAPI FXFileStream : public FXStream {
protected:
  FXFile file;
protected:
  virtual FXuval writeBuffer(FXuval count);
  virtual FXuval readBuffer(FXuval count);
public:

  /// Create file store
  FXDEPRECATEDEXT FXFileStream(const FXObject* cont=NULL);

  /**
  * Open binary data file stream; allocate a buffer of the given size
  * for the file I/O; the buffer must be at least 16 bytes.
  */
  bool open(const FXString& filename,FXStreamDirection save_or_load,FXuval size=8192);

  /// Close file store
  virtual bool close();

  /// Get position
  FXlong position() const { return FXStream::position(); }

  /// Move to position
  virtual bool position(FXlong offset,FXWhence whence=FXFromStart);

  /// Destructor
  virtual ~FXFileStream();
  };

}

#endif
