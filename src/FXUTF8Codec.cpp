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
* $Id: FXUTF8Codec.cpp,v 1.13 2005/01/16 16:06:07 fox Exp $                     *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "FXTextCodec.h"
#include "FXUTF8Codec.h"


/*
  Notes:
  - Some code and algorithms lifted from Roman Czyborra's page on
    Unicode Transformation Formats (http://czyborra.com/utf).

  - From Markus Kuhn's "UTF-8 and Unicode FAQ":
    For security reasons, a UTF-8 decoder must not accept UTF-8 sequences
    that are longer than necessary to encode a character. For example, the
    character U+000A (line feed) must be accepted from a UTF-8 stream only in
    the form 0x0A, but not in any of the following five possible overlong forms:

        0xC0 0x8A
        0xE0 0x80 0x8A
        0xF0 0x80 0x80 0x8A
        0xF8 0x80 0x80 0x80 0x8A
        0xFC 0x80 0x80 0x80 0x80 0x8A

    Any overlong UTF-8 sequence could be abused to bypass UTF-8 substring
    tests that look only for the shortest possible encoding. All overlong UTF-8
    sequences start with one of the following byte patterns:

        1100000x (10xxxxxx)
        11100000 100xxxxx (10xxxxxx)
        11110000 1000xxxx (10xxxxxx 10xxxxxx)
        11111000 10000xxx (10xxxxxx 10xxxxxx 10xxxxxx)
        11111100 100000xx (10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx)

    Also note that the code positions U+D800 to U+DFFF (UTF-16 surrogates) as
    well as U+FFFE and U+FFFF must not occur in normal UTF-8 or UCS-4 data.
    UTF-8 decoders should treat them like malformed or overlong sequences for
    safety reasons.
  - The decoder should replace a malformed sequence with U+FFFD
  - See also: RFC 2279.
  - See RFC-1759 for printer MIB Enums.
*/

/*******************************************************************************/

namespace FX {


// Replace malformed sequences with this character
static const FXwchar MALFORMED=0xfffd;


// How many bytes is it going to take to encode this Unicode character in UTF-8?
inline FXint count(FXwchar c){
  if(c<0x80) return 1;
  if(c<0x800) return 2;
  if(c<0x10000) return 3;
  if(c<0x200000) return 4;
  if(c<0x4000000) return 5;
  return 6;
  }


// How many bytes total should a UTF-8 sequence with this starting byte take?
inline FXint count(FXuchar c){
  if((c&0xfc)==0xfc) return 6;
  if((c&0xf8)==0xf8) return 5;
  if((c&0xf0)==0xf0) return 4;
  if((c&0xe0)==0xe0) return 3;
  if((c&0xc0)==0xc0) return 2;
  return 1;
  }


/**
 * Returns true if this is a valid starting byte for a UTF-8 sequence.
 *
 * UCS characters U+0000 to U+007F (i.e. ASCII) are encoded simply as bytes
 * 0x00 to 0x7F, so that files and strings which contain only 7-bit ASCII
 * characters will have the same encoding under both ASCII and UTF-8. So
 * this defines one range of bytes which are valid starting bytes for a
 * UTF-8 sequence.
 *
 * The first byte of a multibyte sequence that represents a non-ASCII character
 * is always in the range 0xC0 to 0xFD, and it indicates how many bytes follow
 * for this character. So, this gives our other range of bytes which are valid
 * starting bytes for a UTF-8 sequence.
 */
inline bool validstart(FXuchar c){
  return (c<0x80 || (0xc0<=c && c<=0xfd));
  }


/**
 * All bytes after the first byte in a multibyte sequence must be in the
 * range 0x80 to 0xBF. This function returns true if the input byte falls
 * in that range, otherwise it returns false.
 */
inline bool validfollow(FXuchar c){
  return (0x7f<c && c<0xc0);
  }


// Detect overlong UTF-8 sequences (see notes above)
inline bool overlongp(FXuchar c0){
  return ((c0>>1)==0x60);
  }


inline bool overlongp(FXuchar c0,FXuchar c1){
  switch(c0){
    case 0xe0:
      return (c1>>5)==0x04;
    case 0xf0:
      return (c1>>4)==0x08;
    case 0xf8:
      return (c1>>3)==0x10;
    case 0xfc:
      return (c1>>2)==0x20;
    default:
      return false;
    }
  }


// Detect UTF-16 surrogates or U+FFFE or U+FFFF (see notes above)
inline bool abnormalp(FXwchar c){
  return (c>=0xd800 && c<=0xdfff) || c==0xfffe || c==0xffff;
  }


/// Convert to UTF8; return the number of bytes written to dest
unsigned long FXUTF8Codec::fromUnicode(FXuchar*& dest,unsigned long m,const FXwchar*& src,unsigned long n){
  FXASSERT(src);
  FXASSERT(dest);
  FXwchar c;
  unsigned long i,j;
  i=0;
  j=0;
  while(i<n && j<m){
    c=src[i];
    if(j+count(c)>m){
      break;
      }
    i++;
    if(c<0x80){
      dest[j++]=(FXuchar)c;
      }
    else if(c<0x800){
      dest[j++]=(FXuchar)(0xc0 | c>>6);
      dest[j++]=(FXuchar)(0x80 | c    & 0x3f);
      }
    else if(c<0x10000){
      dest[j++]=(FXuchar)(0xe0 | c>>12);
      dest[j++]=(FXuchar)(0x80 | c>>6 & 0x3f);
      dest[j++]=(FXuchar)(0x80 | c    & 0x3f);
      }
    else if(c<0x200000){
      dest[j++]=(FXuchar)(0xf0 | c>>18);
      dest[j++]=(FXuchar)(0x80 | c>>12 & 0x3f);
      dest[j++]=(FXuchar)(0x80 | c>>6  & 0x3f);
      dest[j++]=(FXuchar)(0x80 | c     & 0x3f);
      }
    else if(c<0x4000000){
      dest[j++]=(FXuchar)(0xf8 | c>>24);
      dest[j++]=(FXuchar)(0x80 | c>>18 & 0x3f);
      dest[j++]=(FXuchar)(0x80 | c>>12 & 0x3f);
      dest[j++]=(FXuchar)(0x80 | c>>6  & 0x3f);
      dest[j++]=(FXuchar)(0x80 | c     & 0x3f);
      }
    else{
      dest[j++]=(FXuchar)(0xfc | c>>30);
      dest[j++]=(FXuchar)(0x80 | c>>24 & 0x3f);
      dest[j++]=(FXuchar)(0x80 | c>>18 & 0x3f);
      dest[j++]=(FXuchar)(0x80 | c>>12 & 0x3f);
      dest[j++]=(FXuchar)(0x80 | c>>6  & 0x3f);
      dest[j++]=(FXuchar)(0x80 | c     & 0x3f);
      }
    }
  src=&src[i];
  dest=&dest[j];
  return j;
  }


/// Convert a sequence of bytes from UTF-8; return the number of bytes read from src
unsigned long FXUTF8Codec::toUnicode(FXwchar*& dest,unsigned long m,const FXuchar*& src,unsigned long n){
  FXASSERT(src);
  FXASSERT(dest);
  FXuchar c0,c1,c2,c3,c4,c5;
  unsigned long i,j;

  i=0;
  j=0;
  while(i<n && j<m){
    /**
     * Sequence must begin properly. If it doesn't, replace this character
     * with the malformed sequence indicator and skip ahead to the next byte.
     */
    if(!validstart(src[i])){
      dest[j++]=MALFORMED;
      i++;
      continue;
      }

    // Are enough characters present to complete this sequence?
    if(i+count(src[i])>n){
      break;
      }

    c0=src[i++];
    if((c0&0xfc)==0xfc){
      c1=src[i++];
      if(!validfollow(c1)){
        i--;
        dest[j++]=MALFORMED;
        continue;
        }
      c2=src[i++];
      if(!validfollow(c2)){
        i--;
        dest[j++]=MALFORMED;
        continue;
        }
      c3=src[i++];
      if(!validfollow(c3)){
        i--;
        dest[j++]=MALFORMED;
        continue;
        }
      c4=src[i++];
      if(!validfollow(c4)){
        i--;
        dest[j++]=MALFORMED;
        continue;
        }
      c5=src[i++];
      if(!validfollow(c5)){
        i--;
        dest[j++]=MALFORMED;
        continue;
        }
      if(overlongp(c0,c1)){
        dest[j++]=MALFORMED;
        continue;
        }
      FXTRACE((200,"  c0,c1,c2,c3,c4,c5=0x%04x,0x%04x,0x%04x,0x%04x,0x%04x,0x%04x\n",c0,c1,c2,c3,c4,c5));
      dest[j++]=((c0&0x01)<<30)|((c1&0x3f)<<24)|((c2&0x3f)<<18)|((c3&0x3f)<<12)|((c4&0x3f)<<6)|(c5&0x3f);
      }
    else if((c0&0xf8)==0xf8){
      c1=src[i++];
      if(!validfollow(c1)){
        i--;
        dest[j++]=MALFORMED;
        continue;
        }
      c2=src[i++];
      if(!validfollow(c2)){
        i--;
        dest[j++]=MALFORMED;
        continue;
        }
      c3=src[i++];
      if(!validfollow(c3)){
        i--;
        dest[j++]=MALFORMED;
        continue;
        }
      c4=src[i++];
      if(!validfollow(c4)){
        i--;
        dest[j++]=MALFORMED;
        continue;
        }
      if(overlongp(c0,c1)){
        dest[j++]=MALFORMED;
        continue;
        }
      FXTRACE((200,"  c0,c1,c2,c3,c4=0x%04x,0x%04x,0x%04x,0x%04x,0x%04x\n",c0,c1,c2,c3,c4));
      dest[j++]=((c0&0x03)<<24)|((c1&0x3f)<<18)|((c2&0x3f)<<12)|((c3&0x3f)<<6)|(c4&0x3f);
      }
    else if((c0&0xf0)==0xf0){
      c1=src[i++];
      if(!validfollow(c1)){
        i--;
        dest[j++]=MALFORMED;
        continue;
        }
      c2=src[i++];
      if(!validfollow(c2)){
        i--;
        dest[j++]=MALFORMED;
        continue;
        }
      c3=src[i++];
      if(!validfollow(c3)){
        i--;
        dest[j++]=MALFORMED;
        continue;
        }
      if(overlongp(c0,c1)){
        dest[j++]=MALFORMED;
        continue;
        }
      FXTRACE((200,"  c0,c1,c2,c3=0x%04x,0x%04x,0x%04x,0x%04x\n",c0,c1,c2,c3));
      dest[j++]=((c0&0x07)<<18)|((c1&0x3f)<<12)|((c2&0x3f)<<6)|(c3&0x3f);
      }
    else if((c0&0xe0)==0xe0){
      c1=src[i++];
      if(!validfollow(c1)){
        i--;
        dest[j++]=MALFORMED;
        continue;
        }
      c2=src[i++];
      if(!validfollow(c2)){
        i--;
        dest[j++]=MALFORMED;
        continue;
        }
      if(overlongp(c0,c1)){
        dest[j++]=MALFORMED;
        continue;
        }
      FXTRACE((200,"  c0,c1,c2=0x%04x,0x%04x,0x%04x\n",c0,c1,c2));
      dest[j++]=((c0&0x0f)<<12)|((c1&0x3f)<<6)|(c2&0x3f);
      }
    else if((c0&0xc0)==0xc0){
      c1=src[i++];
      if(!validfollow(c1)){
        i--;
        dest[j++]=MALFORMED;
        continue;
        }
      if(overlongp(c0)){
        dest[j++]=MALFORMED;
        continue;
        }
      FXTRACE((200,"  c0,c1=0x%04x,0x%04x\n",c0,c1));
      dest[j++]=((c0&0x1f)<<6)|(c1&0x3f);
      }
    else{
      FXASSERT(c0<0x80);
      FXTRACE((200,"  c0=0x%04x\n",c0));
      dest[j++]=c0;
      }
    if(abnormalp(dest[j-1])) dest[j-1]=MALFORMED;
    }
  src=&src[i];
  dest=&dest[j];
  return i;
  }


// Return the IANA mime name for this codec
const FXchar* FXUTF8Codec::mimeName() const {
  return "UTF-8";
  }


// Return code for UTF-8
FXint FXUTF8Codec::mibEnum() const {
  return 106;
  }


}

