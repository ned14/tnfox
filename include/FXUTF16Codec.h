/********************************************************************************
*                                                                               *
*                      U T F - 1 6  T e x t   C o d e c                         *
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
* $Id: FXUTF16Codec.h,v 1.4 2005/01/16 16:06:06 fox Exp $                       *
********************************************************************************/
#ifndef FXUTF16CODEC_H
#define FXUTF16CODEC_H

#ifndef FXTEXTCODEC_H
#include "FXTextCodec.h"
#endif


//////////////////////////////  UNDER DEVELOPMENT  //////////////////////////////


namespace FX {

/**
 * Codec for UTF-16
 */
class FXUTF16Codec : public FXTextCodec {
public:

  /// Constructor
  FXUTF16Codec(){}

  /**
   * Convert a sequence of wide characters from Unicode to UTF-16.
   * Reads at most n wide characters from src and writes at most m
   * bytes into dest. Returns the number of characters actually
   * written into dest.
   *
   * On exit, the src and dest pointers are updated to point to the next
   * available character (or byte) for reading (writing).
   */
  virtual unsigned long fromUnicode(FXuchar*& dest,unsigned long m,const FXwchar*& src,unsigned long n);

  /**
   * Insert byte-order mark (BOM) into the stream
   */
  unsigned long insertBOM(FXuchar*& dest,unsigned long m);

  /**
   * Convert a sequence of bytes in UTF-16 encoding to a sequence
   * of wide characters (Unicode). Reads at most n bytes from src and
   * writes at most m characters into dest. Returns the number of characters
   * actually read from src.
   *
   * On exit, the src and dest pointers are updated to point to the next
   * available byte (or character) for writing (reading).
   */
  virtual unsigned long toUnicode(FXwchar*& dest,unsigned long m,const FXuchar*& src,unsigned long n);

  /**
  * Return the IANA mime name for this codec.
  */
  virtual const FXchar* mimeName() const;

  /**
  * Return the Management Information Base (MIBenum) for the character set.
  */
  virtual FXint mibEnum() const;

  /// Destructor
  virtual ~FXUTF16Codec(){}
  };

}

#endif

