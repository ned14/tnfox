/********************************************************************************
*                                                                               *
*                         B Z S t r e a m   C l a s s e s                       *
*                                                                               *
*********************************************************************************
* Copyright (C) 1999,2005 by Lyle Johnson. All Rights Reserved.                 *
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
* $Id: FXBZStream.h,v 1.8 2005/01/16 16:06:06 fox Exp $                         *
********************************************************************************/
#ifdef HAVE_BZ2LIB_H
#ifndef FXBZSTREAM_H
#define FXBZSTREAM_H

#ifndef FXSTREAM_H
#include "FXStream.h"
#endif


namespace FX {


/// BZIP2 compressed file stream
class FXAPI FXBZFileStream : public FXStream {
private:
  void *file;
  void *bzfile;
protected:
  virtual unsigned long writeBuffer(unsigned long count);
  virtual unsigned long readBuffer(unsigned long count);
public:

  /// Create BZIP2 file stream
  FXBZFileStream(const FXObject* cont=NULL);

  /// Open file stream
  FXbool open(const FXString& filename,FXStreamDirection save_or_load,unsigned long size=8192);

  /// Close file stream
  virtual FXbool close();

  /// Get position
  FXlong position() const { return FXStream::position(); }

  /// Move to position
  virtual FXbool position(FXlong,FXWhence){ return FALSE; }

  /// Save single items to stream
  FXBZFileStream& operator<<(const FXuchar& v){ FXStream::operator<<(v); return *this; }
  FXBZFileStream& operator<<(const FXchar& v){ FXStream::operator<<(v); return *this; }
  FXBZFileStream& operator<<(const FXushort& v){ FXStream::operator<<(v); return *this; }
  FXBZFileStream& operator<<(const FXshort& v){ FXStream::operator<<(v); return *this; }
  FXBZFileStream& operator<<(const FXuint& v){ FXStream::operator<<(v); return *this; }
  FXBZFileStream& operator<<(const FXint& v){ FXStream::operator<<(v); return *this; }
  FXBZFileStream& operator<<(const FXfloat& v){ FXStream::operator<<(v); return *this; }
  FXBZFileStream& operator<<(const FXdouble& v){ FXStream::operator<<(v); return *this; }
  FXBZFileStream& operator<<(const FXlong& v){ FXStream::operator<<(v); return *this; }
  FXBZFileStream& operator<<(const FXulong& v){ FXStream::operator<<(v); return *this; }

  /// Save arrays of items to stream
  FXBZFileStream& save(const FXuchar* p,unsigned long n){ FXStream::save(p,n); return *this; }
  FXBZFileStream& save(const FXchar* p,unsigned long n){ FXStream::save(p,n); return *this; }
  FXBZFileStream& save(const FXushort* p,unsigned long n){ FXStream::save(p,n); return *this; }
  FXBZFileStream& save(const FXshort* p,unsigned long n){ FXStream::save(p,n); return *this; }
  FXBZFileStream& save(const FXuint* p,unsigned long n){ FXStream::save(p,n); return *this; }
  FXBZFileStream& save(const FXint* p,unsigned long n){ FXStream::save(p,n); return *this; }
  FXBZFileStream& save(const FXfloat* p,unsigned long n){ FXStream::save(p,n); return *this; }
  FXBZFileStream& save(const FXdouble* p,unsigned long n){ FXStream::save(p,n); return *this; }
  FXBZFileStream& save(const FXlong* p,unsigned long n){ FXStream::save(p,n); return *this; }
  FXBZFileStream& save(const FXulong* p,unsigned long n){ FXStream::save(p,n); return *this; }

  /// Load single items from stream
  FXBZFileStream& operator>>(FXuchar& v){ FXStream::operator>>(v); return *this; }
  FXBZFileStream& operator>>(FXchar& v){ FXStream::operator>>(v); return *this; }
  FXBZFileStream& operator>>(FXushort& v){ FXStream::operator>>(v); return *this; }
  FXBZFileStream& operator>>(FXshort& v){ FXStream::operator>>(v); return *this; }
  FXBZFileStream& operator>>(FXuint& v){ FXStream::operator>>(v); return *this; }
  FXBZFileStream& operator>>(FXint& v){ FXStream::operator>>(v); return *this; }
  FXBZFileStream& operator>>(FXfloat& v){ FXStream::operator>>(v); return *this; }
  FXBZFileStream& operator>>(FXdouble& v){ FXStream::operator>>(v); return *this; }
  FXBZFileStream& operator>>(FXlong& v){ FXStream::operator>>(v); return *this; }
  FXBZFileStream& operator>>(FXulong& v){ FXStream::operator>>(v); return *this; }

  /// Load arrays of items from stream
  FXBZFileStream& load(FXuchar* p,unsigned long n){ FXStream::load(p,n); return *this; }
  FXBZFileStream& load(FXchar* p,unsigned long n){ FXStream::load(p,n); return *this; }
  FXBZFileStream& load(FXushort* p,unsigned long n){ FXStream::load(p,n); return *this; }
  FXBZFileStream& load(FXshort* p,unsigned long n){ FXStream::load(p,n); return *this; }
  FXBZFileStream& load(FXuint* p,unsigned long n){ FXStream::load(p,n); return *this; }
  FXBZFileStream& load(FXint* p,unsigned long n){ FXStream::load(p,n); return *this; }
  FXBZFileStream& load(FXfloat* p,unsigned long n){ FXStream::load(p,n); return *this; }
  FXBZFileStream& load(FXdouble* p,unsigned long n){ FXStream::load(p,n); return *this; }
  FXBZFileStream& load(FXlong* p,unsigned long n){ FXStream::load(p,n); return *this; }
  FXBZFileStream& load(FXulong* p,unsigned long n){ FXStream::load(p,n); return *this; }

  /// Save object
  FXBZFileStream& saveObject(const FXObject* v){ FXStream::saveObject(v); return *this; }

  /// Load object
  FXBZFileStream& loadObject(FXObject*& v){ FXStream::loadObject(v); return *this; }

  /// Clean up
  virtual ~FXBZFileStream();
  };


}

#endif
#endif

