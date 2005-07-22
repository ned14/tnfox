/********************************************************************************
*                                                                               *
*                       T A R G A  I m a g e   O b j e c t                      *
*                                                                               *
*********************************************************************************
* Copyright (C) 2001,2005 by Janusz Ganczarski.   All Rights Reserved.          *
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
* $Id: FXTGAImage.cpp,v 1.21 2005/01/16 16:06:07 fox Exp $                      *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "FXHash.h"
#include "QThread.h"
#include "FXStream.h"
#include "FXMemoryStream.h"
#include "FXString.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXRegistry.h"
#include "FXApp.h"
#include "FXTGAImage.h"



/*
  Notes:
*/



/*******************************************************************************/

namespace FX {


// Suggested file extension
const FXchar *FXTGAImage::fileExt="tga";


// Object implementation
FXIMPLEMENT(FXTGAImage,FXImage,NULL,0)


// Initialize
FXTGAImage::FXTGAImage(FXApp* a,const void *pix,FXuint opts,FXint w,FXint h):FXImage(a,NULL,opts,w,h){
  if(pix){
    FXMemoryStream ms;
    ms.open(FXStreamLoad,(FXuchar*)pix);
    loadPixels(ms);
    ms.close();
    }
  }


// Save pixels to stream
FXbool FXTGAImage::savePixels(FXStream& store) const {
  if(fxsaveTGA(store,data,width,height)){
    return TRUE;
    }
  return FALSE;
  }


// Load pixels from stream
FXbool FXTGAImage::loadPixels(FXStream& store){
  FXColor *pixels; FXint w,h;
  if(fxloadTGA(store,pixels,w,h)){
    setData(pixels,IMAGE_OWNED,w,h);
    return TRUE;
    }
  return FALSE;
  }


// Clean up
FXTGAImage::~FXTGAImage(){
  }

}
