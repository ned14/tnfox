/********************************************************************************
*                                                                               *
*                            X B M   I m a g e   O b j e c t                    *
*                                                                               *
*********************************************************************************
* Copyright (C) 2003,2005 by Jeroen van der Zijp.   All Rights Reserved.        *
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
* $Id: FXXBMImage.cpp,v 1.10 2005/01/16 16:06:07 fox Exp $                       *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "FXHash.h"
#include "FXThread.h"
#include "FXStream.h"
#include "FXMemoryStream.h"
#include "FXString.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXApp.h"
#include "FXId.h"
#include "FXDrawable.h"
#include "FXImage.h"
#include "FXXBMImage.h"



/*
  Notes:
*/



/*******************************************************************************/

namespace FX {


// Suggested file extension
const FXchar *FXXBMImage::fileExt="xbm";


// Object implementation
FXIMPLEMENT(FXXBMImage,FXImage,NULL,0)


// Initialize
FXXBMImage::FXXBMImage(FXApp* a,const FXuchar *pixels,const FXuchar *mask,FXuint opts,FXint w,FXint h):FXImage(a,NULL,opts,w,h){
  if(pixels && mask){
    fxloadXBM(data,pixels,mask,w,h);
    options|=IMAGE_OWNED;
    }
  }


// Save pixel data only
FXbool FXXBMImage::savePixels(FXStream& store) const {
  if(fxsaveXBM(store,data,width,height,-1,-1)){
    return TRUE;
    }
  return FALSE;
  }


// Load pixel data only
FXbool FXXBMImage::loadPixels(FXStream& store){
  FXColor *pixels; FXint w,h,hotx,hoty;
  if(fxloadXBM(store,pixels,w,h,hotx,hoty)){
    setData(pixels,IMAGE_OWNED,w,h);
    return TRUE;
    }
  return FALSE;
  }


// Clean up
FXXBMImage::~FXXBMImage(){
  }

}
