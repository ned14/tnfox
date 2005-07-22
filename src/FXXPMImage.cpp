/********************************************************************************
*                                                                               *
*                            X P M   I m a g e   O b j e c t                    *
*                                                                               *
*********************************************************************************
* Copyright (C) 2000,2005 by Jeroen van der Zijp.   All Rights Reserved.        *
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
* $Id: FXXPMImage.cpp,v 1.28 2005/01/16 16:06:07 fox Exp $                      *
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
#include "FXXPMImage.h"



/*
  Notes:
*/




/*******************************************************************************/

namespace FX {


// Suggested file extension
const FXchar *FXXPMImage::fileExt="xpm";


// Object implementation
FXIMPLEMENT(FXXPMImage,FXImage,NULL,0)


// Initialize
FXXPMImage::FXXPMImage(FXApp* a,const FXchar **pix,FXuint opts,FXint w,FXint h):FXImage(a,NULL,opts,w,h){
  if(pix){
    fxloadXPM(pix,data,width,height);
    options|=IMAGE_OWNED;
    }
  }


// Save pixel data only
FXbool FXXPMImage::savePixels(FXStream& store) const {
  if(fxsaveXPM(store,data,width,height)){
    return TRUE;
    }
  return FALSE;
  }


// Load pixel data only
FXbool FXXPMImage::loadPixels(FXStream& store){
  FXColor *pixels; FXint w,h;
  if(fxloadXPM(store,pixels,w,h)){
    setData(pixels,IMAGE_OWNED,w,h);
    return TRUE;
    }
  return FALSE;
  }


// Clean up
FXXPMImage::~FXXPMImage(){
  }

}
