/********************************************************************************
*                                                                               *
*                      U T F - 1 6  T e x t   C o d e c                         *
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
* $Id: FXUTF16Codec.cpp,v 1.4 2004/02/08 17:29:07 fox Exp $                     *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "FXTextCodec.h"
#include "FXUTF16Codec.h"


/*
  Notes:
  - Some code and algorithms lifted from Roman Czyborra's page on
    Unicode Transformation Formats (http://czyborra.com/utf).
  - What about UTF-16LE (little-endian) and UTF-16BE (big-endian)?
*/

/*******************************************************************************/

namespace FX {


// Convert to UTF-16; return the number of bytes written to dest
unsigned long FXUTF16Codec::fromUnicode(FXuchar*& dest,unsigned long m,const FXwchar*& src,unsigned long n){
  FXASSERT(src);
  FXASSERT(dest);
  FXwchar c;
  unsigned long i,j;
  i=0;
  j=0;
  while(i<n && j+1<m){
    c=src[i++];
    if(c>0xffff){
      dest[j++]=(FXuchar)(0xd7c0+(c>>10));
      dest[j++]=(FXuchar)(0xdc00 | c & 0x3ff);
      }
    else{
      dest[j++]=(FXuchar)(c>>8);
      dest[j++]=(FXuchar)(c&0xff);
      }
    }
  src=&src[i];
  dest=&dest[j];
  return j;
  }


// UTF-16 texts should start with U+FEFF (zero width no-break space) as a byte-order mark
static const FXwchar BOM=0xfeff;


// Insert byte-order mark (BOM) into the stream
unsigned long FXUTF16Codec::insertBOM(FXuchar*& dest,unsigned long m){
  if(m>1){
    *dest++=(FXuchar)(BOM>>8);
    *dest++=(FXuchar)(BOM&0xff);
    return 2;
    }
  return 0;
  }


// Convert a sequence of bytes from UTF-16; return the number of bytes read from src
unsigned long FXUTF16Codec::toUnicode(FXwchar*& dest,unsigned long m,const FXuchar*& src,unsigned long n){
  FXASSERT(src);
  FXASSERT(dest);
  return 0;
  }


// Return the IANA mime name for this codec
const FXchar* FXUTF16Codec::mimeName() const {
  return "UTF-16";
  }


// Return code for UTF-16
FXint FXUTF16Codec::mibEnum() const {
  return 1015;
  }


}

