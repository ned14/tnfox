/********************************************************************************
*                                                                               *
*                          T I F F  I c o n   O b j e c t                       *
*                                                                               *
*********************************************************************************
* Copyright (C) 2001,2004 Eric Gillet.   All Rights Reserved.                   *
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
* $Id: FXTIFIcon.cpp,v 1.22 2004/11/10 16:22:05 fox Exp $                       *
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
#include "FXTIFIcon.h"


/*
  Notes:
  - FXTIFIcon has an alpha channel.
*/



/*******************************************************************************/

namespace FX {


// Suggested file extension
const FXchar FXTIFIcon::fileExt[]="tif";


// Object implementation
FXIMPLEMENT(FXTIFIcon,FXIcon,NULL,0)


#ifdef HAVE_TIFF_H
const FXbool FXTIFIcon::supported=TRUE;
#else
const FXbool FXTIFIcon::supported=FALSE;
#endif


// Initialize
FXTIFIcon::FXTIFIcon(FXApp* a,const void *pix,FXColor clr,FXuint opts,FXint w,FXint h):
  FXIcon(a,NULL,clr,opts,w,h){
  codec=0;
  if(pix){
    FXMemoryStream ms;
    ms.open(FXStreamLoad,(FXuchar*)pix);
    loadPixels(ms);
    ms.close();
    }
  }


// Save pixels only
FXbool FXTIFIcon::savePixels(FXStream& store) const {
  if(!fxsaveTIF(store,data,width,height,codec)) return FALSE;
  return TRUE;
  }


// Load pixels only
FXbool FXTIFIcon::loadPixels(FXStream& store){
  if(options&IMAGE_OWNED){ FXFREE(&data); }
  if(!fxloadTIF(store,data,width,height,codec)) return FALSE;
  if(options&IMAGE_ALPHAGUESS) transp=guesstransp();
  options|=IMAGE_OWNED;
  return TRUE;
  }


// Clean up
FXTIFIcon::~FXTIFIcon(){
  }

}
