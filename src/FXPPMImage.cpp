/********************************************************************************
*                                                                               *
*                            P P M   I m a g e   O b j e c t                    *
*                                                                               *
*********************************************************************************
* Copyright (C) 1998,2005 by Jeroen van der Zijp.   All Rights Reserved.        *
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
* $Id: FXPPMImage.cpp,v 1.9 2005/01/16 16:06:07 fox Exp $                       *
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
#include "FXPPMImage.h"



/*
  Notes:
  - PPM does not support alpha in the file format.
*/



/*******************************************************************************/

namespace FX {


// Suggested file extension
const FXchar *FXPPMImage::fileExt="ppm";


// Object implementation
FXIMPLEMENT(FXPPMImage,FXImage,NULL,0)


// Initialize
FXPPMImage::FXPPMImage(FXApp* a,const void *pix,FXuint opts,FXint w,FXint h):FXImage(a,NULL,opts,w,h){
  if(pix){
    FXMemoryStream ms;
    ms.open(FXStreamLoad,(FXuchar*)pix);
    loadPixels(ms);
    ms.close();
    }
  }


// Save pixel data only
FXbool FXPPMImage::savePixels(FXStream& store) const {
  if(fxsavePPM(store,data,width,height)){
    return TRUE;
    }
  return FALSE;
  }


// Load pixel data only
FXbool FXPPMImage::loadPixels(FXStream& store){
  FXColor *pixels; FXint w,h;
  if(fxloadPPM(store,pixels,w,h)){
    setData(pixels,IMAGE_OWNED,w,h);
    return TRUE;
    }
  return FALSE;
  }


// Clean up
FXPPMImage::~FXPPMImage(){
  }

}
