/********************************************************************************
*                                                                               *
*                           C h a r a c t e r   S e t s                         *
*                                                                               *
*********************************************************************************
* Copyright (C) 2000,2004 by Jeroen van der Zijp.   All Rights Reserved.        *
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
* $Id: FXCharset.h,v 1.14 2004/02/08 17:17:33 fox Exp $                         *
********************************************************************************/
#ifndef FXCHARSET_H
#define FXCHARSET_H

#include "fxdefs.h"

namespace FX {

/// A set of characters
class FXAPI FXCharset {
private:
  FXuint s[8];              // Because 8*32 is 256 characters
private:
  FXCharset(FXuint a,FXuint b,FXuint c,FXuint d,FXuint e,FXuint f,FXuint g,FXuint h){
    s[0]=a;s[1]=b;s[2]=c;s[3]=d;s[4]=e;s[5]=f;s[6]=g;s[7]=h;
    }
public:

  /// Initialize to empty set
  FXCharset(){clear();}

  /// Copy constructor
  FXCharset(const FXCharset& a){
    s[0]=a.s[0];s[1]=a.s[1];s[2]=a.s[2];s[3]=a.s[3];s[4]=a.s[4];s[5]=a.s[5];s[6]=a.s[6];s[7]=a.s[7];
    }

  /// Initialize with one character
  FXCharset(FXchar ch){
    clear(); s[((FXuchar)ch)>>5] |= (1<<(ch&31));
    }

  /// Initialize set with set of characters
  FXCharset(const FXString& characters);

  /// Convert to characters
  operator FXString();

  /// See if character ch is member of set
  FXbool has(FXchar ch) const {
    return (s[((FXuchar)ch)>>5] & (1<<(ch&31)))!=0;
    }

  /// Clear the set
  FXCharset& clear(){
    s[0]=s[1]=s[2]=s[3]=s[4]=s[5]=s[6]=s[7]=0;
    return *this;
    }

  /// Assignment of one character
  FXCharset& operator=(FXchar ch){
    clear(); s[((FXuchar)ch)>>5] |= (1<<(ch&31));
    return *this;
    }

  /// Include character ch into set
  FXCharset& operator+=(FXchar ch){
    s[((FXuchar)ch)>>5] |= (1<<(ch&31));
    return *this;
    }

  /// Exclude character ch from set
  FXCharset& operator-=(FXchar ch){
    s[((FXuchar)ch)>>5] &= ~(1<<(ch&31));
    return *this;
    }

  /// Assignment with characters
  FXCharset& operator=(const FXString& characters);

  /// Include characters into set
  FXCharset& operator+=(const FXString& characters);

  /// Exclude characters from set
  FXCharset& operator-=(const FXString& characters);

  /// Assigning one set to this one
  FXCharset& operator=(const FXCharset& a){
    s[0]=a.s[0];s[1]=a.s[1];s[2]=a.s[2];s[3]=a.s[3];s[4]=a.s[4];s[5]=a.s[5];s[6]=a.s[6];s[7]=a.s[7];
    return *this;
    }

  /// Union set with this one
  FXCharset& operator+=(const FXCharset& a){
    s[0]|=a.s[0];s[1]|=a.s[1];s[2]|=a.s[2];s[3]|=a.s[3];s[4]|=a.s[4];s[5]|=a.s[5];s[6]|=a.s[6];s[7]|=a.s[7];
    return *this;
    }

  /// Remove set from this one
  FXCharset& operator-=(const FXCharset& a){
    s[0]&=~a.s[0];s[1]&=~a.s[1];s[2]&=~a.s[2];s[3]&=~a.s[3];s[4]&=~a.s[4];s[5]&=~a.s[5];s[6]&=~a.s[6];s[7]&=~a.s[7];
    return *this;
    }

  /// Interset set with this one
  FXCharset& operator*=(const FXCharset& a){
    s[0]&=a.s[0];s[1]&=a.s[1];s[2]&=a.s[2];s[3]&=a.s[3];s[4]&=a.s[4];s[5]&=a.s[5];s[6]&=a.s[6];s[7]&=a.s[7];
    return *this;
    }

  /// Negate set
  friend FXAPI FXCharset operator-(const FXCharset& a){
    return FXCharset(~a.s[0],~a.s[1],~a.s[2],~a.s[3],~a.s[4],~a.s[5],~a.s[6],~a.s[7]);
    }

  /// Union sets a and b
  friend FXAPI FXCharset operator+(const FXCharset& a,const FXCharset& b){
    return FXCharset(a.s[0]|b.s[0],a.s[1]|b.s[1],a.s[2]|b.s[2],a.s[3]|b.s[3],a.s[4]|b.s[4],a.s[5]|b.s[5],a.s[6]|b.s[6],a.s[7]|b.s[7]);
    }

  /// Set a less b
  friend FXAPI FXCharset operator-(const FXCharset& a,const FXCharset& b){
    return FXCharset(a.s[0]&~b.s[0],a.s[1]&~b.s[1],a.s[2]&~b.s[2],a.s[3]&~b.s[3],a.s[4]&~b.s[4],a.s[5]&~b.s[5],a.s[6]&~b.s[6],a.s[7]&~b.s[7]);
    }

  /// Intersect set a and b
  friend FXAPI FXCharset operator*(const FXCharset& a,const FXCharset& b){
    return FXCharset(a.s[0]&b.s[0],a.s[1]&b.s[1],a.s[2]&b.s[2],a.s[3]&b.s[3],a.s[4]&b.s[4],a.s[5]&b.s[5],a.s[6]&b.s[6],a.s[7]&b.s[7]);
    }

  /// Equality tests
  friend FXAPI int operator==(const FXCharset& a,const FXCharset& b){
    return a.s[0]==b.s[0] && a.s[1]==b.s[1] && a.s[2]==b.s[2] && a.s[3]==b.s[3] && a.s[4]==b.s[4] && a.s[5]==b.s[5] && a.s[6]==b.s[6] && a.s[7]==b.s[7];
    }

  friend FXAPI int operator!=(const FXCharset& a,const FXCharset& b){
    return a.s[0]!=b.s[0] || a.s[1]!=b.s[1] || a.s[2]!=b.s[2] || a.s[3]!=b.s[3] || a.s[4]!=b.s[4] || a.s[5]!=b.s[5] || a.s[6]!=b.s[6] || a.s[7]!=b.s[7];
    }

  /// Save set to a stream
  friend FXAPI FXStream& operator<<(FXStream& store,const FXCharset& cs);

  /// Load set from a stream
  friend FXAPI FXStream& operator>>(FXStream& store,FXCharset& cs);

  };

}

#endif

