/********************************************************************************
*                                                                               *
*                            G I F   I m a g e   O b j e c t                    *
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
* $Id: FXGIFImage.cpp,v 1.25 2004/02/08 17:29:06 fox Exp $                      *
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
#include "FXGIFImage.h"



/*
  Notes:
  - Only free image if owned!
*/




/*******************************************************************************/

namespace FX {

// Object implementation
FXIMPLEMENT(FXGIFImage,FXImage,NULL,0)


// Initialize
FXGIFImage::FXGIFImage(FXApp* a,const void *pix,FXuint opts,FXint w,FXint h):
  FXImage(a,NULL,opts,w,h){
  if(pix){
    FXMemoryStream ms;
    ms.open(FXStreamLoad,(FXuchar*)pix);
    fxloadGIF(ms,data,width,height);
    options|=IMAGE_OWNED;
    ms.close();
    }
  }


// Save object to stream
FXbool FXGIFImage::savePixels(FXStream& store) const {
  if(!fxsaveGIF(store,data,width,height)) return FALSE;
  return TRUE;
  }


// Load object from stream
FXbool FXGIFImage::loadPixels(FXStream& store){
  if(options&IMAGE_OWNED){FXFREE(&data);}
  if(!fxloadGIF(store,data,width,height)) return FALSE;
  options|=IMAGE_OWNED;
  return TRUE;
  }


// Clean up
FXGIFImage::~FXGIFImage(){
  }

}
