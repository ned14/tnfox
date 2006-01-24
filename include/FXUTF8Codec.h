/********************************************************************************
*                                                                               *
*                      U T F - 8  T e x t   C o d e c                           *
*                                                                               *
*********************************************************************************
* Copyright (C) 2002,2005 by Lyle Johnson.   All Rights Reserved.               *
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
* $Id: FXUTF8Codec.h,v 1.7 2005/01/16 16:06:06 fox Exp $                        *
********************************************************************************/
#ifndef FXUTF8CODEC_H
#define FXUTF8CODEC_H

#ifndef FXTEXTCODEC_H
#include "FXTextCodec.h"
#endif


//////////////////////////////  UNDER DEVELOPMENT  //////////////////////////////


namespace FX {

/**
 * Codec for UTF-8
 */
class FXUTF8Codec : public FXTextCodec {
public:

  /// Constructor
  FXUTF8Codec(){}

  /**
   * Convert a sequence of wide characters from Unicode to UTF-8.
   * Reads at most n wide characters from src and writes at most m
   * bytes into dest. Returns the number of characters actually
   * written into dest.
   *
   * On exit, the src and dest pointers are updated to point to the next
   * available character (or byte) for reading (writing).
   */
  virtual unsigned long fromUnicode(FXuchar*& dest,unsigned long m,const FXwchar*& src,unsigned long n);

  /**
   * Convert a sequence of bytes in UTF-8 encoding to a sequence
   * of wide characters (Unicode). Reads at most n bytes from src and
   * writes at most m characters into dest. Returns the number of characters
   * actually read from src.
   *
   * On exit, the src and dest pointers are updated to point to the next
   * available byte (or character) for writing (reading).
   */
  virtual unsigned long toUnicode(FXwchar*& dest,unsigned long m,const FXuchar*& src,unsigned long n);

  /**
  * Return the IANA mime name for this codec; this is used for example
  * as "text/utf-8" in drag and drop protocols.
  */
  virtual const FXchar* mimeName() const;

  /**
  * Return the Management Information Base (MIBenum) for the character set.
  */
  virtual FXint mibEnum() const;

  /// Destructor
  virtual ~FXUTF8Codec(){}
  };

}

#endif

