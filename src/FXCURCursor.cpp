/********************************************************************************
*                                                                               *
*                        C U R   C u r s o r    O b j e c t                     *
*                                                                               *
*********************************************************************************
* Copyright (C) 2001,2004 by Sander Jansen.   All Rights Reserved.              *
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
* $Id: FXCURCursor.cpp,v 1.16 2004/01/14 14:21:28 fox Exp $                     *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "FXStream.h"
#include "FXMemoryStream.h"
#include "FXString.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXHash.h"
#include "FXApp.h"
#include "FXCURCursor.h"


/*
 Notes:
  - Tossed old code now that FXCursor has an RGBA representation.
*/





/*******************************************************************************/

namespace FX {


// Object implementation
FXIMPLEMENT(FXCURCursor,FXCursor,NULL,0)


// Constructor
FXCURCursor::FXCURCursor(FXApp* a,const void *pix):FXCursor(a,NULL,0,0,0,0){
  if(pix){
    FXMemoryStream ms;
    ms.open(FXStreamLoad,(FXuchar*)pix);
    fxloadICO(ms,data,width,height,hotx,hoty);
    options|=CURSOR_OWNED;
    ms.close();
    }
  }



// Save pixel data only, in CUR format
FXbool FXCURCursor::savePixels(FXStream& store) const {
  if(!fxsaveICO(store,data,width,height,hotx,hoty)) return FALSE;
  return TRUE;
  }


// Load cursor mask and image
FXbool FXCURCursor::loadPixels(FXStream & store){
  if(options&CURSOR_OWNED){FXFREE(&data);}
  if(!fxloadICO(store,data,width,height,hotx,hoty)) return FALSE;
  options|=CURSOR_OWNED;
  return TRUE;
  }


// Destroy
FXCURCursor::~FXCURCursor(){
  }

}
