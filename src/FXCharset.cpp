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
* $Id: FXCharset.cpp,v 1.17 2004/02/08 17:29:06 fox Exp $                       *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "FXStream.h"
#include "FXObject.h"
#include "FXString.h"
#include "FXCharset.h"



/*******************************************************************************/

namespace FX {

// Initialize set with set of characters
FXCharset::FXCharset(const FXString& characters){
  register const FXchar* ptr=characters.text();
  clear(); while(*ptr){ *this+=*ptr++; }
  }


// Convert to characters
FXCharset::operator FXString(){
  register FXuint i=0,ch=1; FXchar buffer[256];
  do{ if(has((FXchar)ch)) buffer[i++]=ch; }while(++ch!=256); buffer[i]='\0';
  return FXString(buffer);
  }


// Assignment with characters
FXCharset& FXCharset::operator=(const FXString& characters){
  register const FXchar* ptr=characters.text();
  clear(); while(*ptr){ *this+=*ptr++; }
  return *this;
  }


// Include characters into set
FXCharset& FXCharset::operator+=(const FXString& characters){
  register const FXchar* ptr=characters.text();
  while(*ptr){ *this+=*ptr++; }
  return *this;
  }


// Exclude characters from set
FXCharset& FXCharset::operator-=(const FXString& characters){
  register const FXchar* ptr=characters.text();
  while(*ptr){ *this-=*ptr++; }
  return *this;
  }


// Saving
FXStream& operator<<(FXStream& store,const FXCharset& cs){
  store << cs.s[0] << cs.s[1] << cs.s[2] << cs.s[3] << cs.s[4] << cs.s[5] << cs.s[6] << cs.s[7];
  return store;
  }


// Loading
FXStream& operator>>(FXStream& store,FXCharset& cs){
  store >> cs.s[0] >> cs.s[1] >> cs.s[2] >> cs.s[3] >> cs.s[4] >> cs.s[5] >> cs.s[6] >> cs.s[7];;
  return store;
  }

}
