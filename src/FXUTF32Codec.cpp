/********************************************************************************
*                                                                               *
*                      U T F - 3 2  T e x t   C o d e c                         *
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
* $Id: FXUTF32Codec.cpp,v 1.3 2004/02/08 17:29:07 fox Exp $                     *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "FXTextCodec.h"
#include "FXUTF32Codec.h"


/*
  Notes:
  - Some code and alogorithms lifted from Roman Czyborra's page on
    Unicode Transformation Formats (http://czyborra.com/utf).
  - What about UTF-32BE (big-endian) and UTF-32LE (little-endian)?
*/

/*******************************************************************************/

namespace FX {


/// Convert to UTF-32; return the number of bytes written to dest
unsigned long FXUTF32Codec::fromUnicode(FXuchar*& dest,unsigned long m,const FXwchar*& src,unsigned long n){
  FXASSERT(src);
  FXASSERT(dest);
  return 0;
  }


/// Convert a sequence of bytes from UTF-32; return the number of bytes read from src
unsigned long FXUTF32Codec::toUnicode(FXwchar*& dest,unsigned long m,const FXuchar*& src,unsigned long n){
  FXASSERT(src);
  FXASSERT(dest);
  return 0;
  }


// Return the IANA mime name for this codec
const FXchar* FXUTF32Codec::mimeName() const {
  return "UTF-32";
  }


// Return code for UTF-32
FXint FXUTF32Codec::mibEnum() const {
  return 1017;
  }


}

