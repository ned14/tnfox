/********************************************************************************
*                                                                               *
*                          D i c t i o n a r y    C l a s s                     *
*                                                                               *
*********************************************************************************
* Copyright (C) 1998,2004 by Jeroen van der Zijp.   All Rights Reserved.        *
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
* $Id: FXStringDict.cpp,v 1.9 2004/09/17 07:46:22 fox Exp $                     *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXStringDict.h"


/*
  Notes:
  - String dict may be useful in many applications.
*/




/*******************************************************************************/

namespace FX {

// Object implementation
FXIMPLEMENT(FXStringDict,FXDict,NULL,0)


// Construct string dict
FXStringDict::FXStringDict(){
  FXTRACE((100,"FXStringDict::FXStringDict %p\n",this));
  }


// Create string
void *FXStringDict::createData(const void* ptr){
  return strdup((const char*)ptr);
  }


// Delete string
void FXStringDict::deleteData(void* ptr){
  free(ptr);
  }


// Destructor
FXStringDict::~FXStringDict(){
  FXTRACE((100,"FXStringDict::~FXStringDict %p\n",this));
  clear();
  }

}
