/********************************************************************************
*                                                                               *
*                   U n i c o d e   T e x t   C o d e c                         *
*                                                                               *
*********************************************************************************
* Copyright (C) 2002,2004 by Lyle Johnson.   All Rights Reserved.               *
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
* $Id: FXTextCodec.h,v 1.7 2004/02/08 17:17:34 fox Exp $                        *
********************************************************************************/
#ifndef FXTEXTCODEC_H
#define FXTEXTCODEC_H


//////////////////////////////  UNDER DEVELOPMENT  //////////////////////////////


namespace FX {

class FXTextCodecDict;

/**
 * Abstract base class for a stateless coder/decoder.
 */
class FXTextCodec {
protected:
  static FXTextCodecDict* codecs;
protected:
  FXTextCodec(){}
public:

  /**
   * Convert a sequence of wide characters from Unicode to the specified
   * 8-bit encoding. Reads at most n wide characters from src and writes
   * at most m bytes into dest. Returns the number of characters actually
   * written into dest.
   *
   * On exit, the src and dest pointers are updated to point to the next
   * available character (or byte) for reading (writing).
   */
  virtual unsigned long fromUnicode(FXuchar*& dest,unsigned long m,const FXwchar*& src,unsigned long n) = 0;

  /**
   * Convert a sequence of bytes in some 8-bit encoding to a sequence
   * of wide characters (Unicode). Reads at most n bytes from src and
   * writes at most m characters into dest. Returns the number of characters
   * actually read from src.
   *
   * On exit, the src and dest pointers are updated to point to the next
   * available byte (or character) for writing (reading).
   */
  virtual unsigned long toUnicode(FXwchar*& dest,unsigned long m,const FXuchar*& src,unsigned long n) = 0;

  /**
   * Return the IANA mime name for this codec; this is used for example
   * as "text/utf-8" in drag and drop protocols.
   */
  virtual const FXchar* mimeName() const = 0;

  /**
   * Return the Management Information Base (MIBenum) for the character set.
   */
  virtual FXint mibEnum() const = 0;

  /**
   * Register codec with this name. Returns FALSE if a different codec
   * has already been registered with this name, otherwise TRUE.
   */
  static FXbool registerCodec(const FXchar* name,FXTextCodec* codec);

  /// Return the codec associated with this name (or NULL if no match is found)
  static FXTextCodec* codecForName(const FXchar* name);

  /// Destructor
  virtual ~FXTextCodec(){}
  };

}

#endif
