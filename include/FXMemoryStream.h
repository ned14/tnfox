/********************************************************************************
*                                                                               *
*                   M e m o r y   S t r e a m   C l a s s e s                   *
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
* $Id: FXMemoryStream.h,v 1.11 2006/01/22 17:58:06 fox Exp $                    *
********************************************************************************/
#ifndef FXMEMORYSTREAM_H
#define FXMEMORYSTREAM_H

#ifndef FXSTREAM_H
#include "FXStream.h"
#endif

namespace FX {

/*! \class FXMemoryStream
\deprecated For compatibility with FOX code only
*/
class QBuffer;
class FXAPI FXMemoryStream : public FXStream {
  QBuffer *buffer;
  bool    owns;         // Owns the data array
public:

  /// Create memory store
  FXDEPRECATEDEXT FXMemoryStream(const FXObject* cont=NULL);

  /// Open file store
  bool open(FXStreamDirection save_or_load,FXuchar* data);

  /// Open memory store
  bool open(FXStreamDirection save_or_load,FXuval size,FXuchar* data);

  /// Take buffer away from stream
  void takeBuffer(FXuchar*& data,FXuval& size);

  /// Give buffer to stream
  void giveBuffer(FXuchar *data,FXuval size);

  /// Close memory store
  virtual bool close();

  };

}

#endif
