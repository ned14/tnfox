/********************************************************************************
*                                                                               *
*                            X P M   I m a g e   O b j e c t                    *
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
* $Id: FXXPMImage.cpp,v 1.26 2004/11/10 16:22:05 fox Exp $                      *
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
FXXPMImage::FXXPMImage(FXApp* a,const FXchar **pix,FXuint opts,FXint w,FXint h):
  FXImage(a,NULL,opts,w,h){
  if(pix){
    fxloadXPM(pix,data,width,height);
    options|=IMAGE_OWNED;
    }
  }


// Save pixel data only
FXbool FXXPMImage::savePixels(FXStream& store) const {
  if(!fxsaveXPM(store,data,width,height)) return FALSE;
  return TRUE;
  }


// Load pixel data only
FXbool FXXPMImage::loadPixels(FXStream& store){
  if(options&IMAGE_OWNED){FXFREE(&data);}
  if(!fxloadXPM(store,data,width,height)) return FALSE;
  options|=IMAGE_OWNED;
  return TRUE;
  }


// Clean up
FXXPMImage::~FXXPMImage(){
  }

}
