/********************************************************************************
*                                                                               *
*                     I R I S   R G B   I m a g e   O b j e c t                 *
*                                                                               *
*********************************************************************************
* Copyright (C) 2002,2004 by Jeroen van der Zijp.   All Rights Reserved.        *
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
* $Id: FXRGBImage.cpp,v 1.20 2004/11/10 16:22:05 fox Exp $                      *
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
#include "FXRGBImage.h"



/*
  Notes:
*/



/*******************************************************************************/

namespace FX {


// Suggested file extension
const FXchar FXRGBImage::fileExt[]="rgb";


// Object implementation
FXIMPLEMENT(FXRGBImage,FXImage,NULL,0)


// Initialize
FXRGBImage::FXRGBImage(FXApp* a,const void *pix,FXuint opts,FXint w,FXint h):
  FXImage(a,NULL,opts,w,h){
  if(pix){
    FXMemoryStream ms;
    ms.open(FXStreamLoad,(FXuchar*)pix);
    fxloadRGB(ms,data,width,height);
    options|=IMAGE_OWNED;
    ms.close();
    }
  }


// Save pixel data only
FXbool FXRGBImage::savePixels(FXStream& store) const {
  if(!fxsaveRGB(store,data,width,height)) return FALSE;
  return TRUE;
  }


// Load pixel data only
FXbool FXRGBImage::loadPixels(FXStream& store){
  if(options&IMAGE_OWNED){ FXFREE(&data); }
  if(!fxloadRGB(store,data,width,height)) return FALSE;
  options|=IMAGE_OWNED;
  return TRUE;
  }


// Clean up
FXRGBImage::~FXRGBImage(){
  }

}

