/********************************************************************************
*                                                                               *
*                         J P E G   I c o n   O b j e c t                       *
*                                                                               *
*********************************************************************************
* Copyright (C) 2000,2004 by David Tyree.   All Rights Reserved.                *
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
* $Id: FXJPGIcon.cpp,v 1.20 2004/11/10 16:22:05 fox Exp $                       *
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
#include "FXJPGIcon.h"


/*
  Notes:
  - Requires JPEG library.
*/



/*******************************************************************************/

namespace FX {


// Suggested file extension
const FXchar *FXJPGIcon::fileExt="jpg";


// Object implementation
FXIMPLEMENT(FXJPGIcon,FXIcon,NULL,0)


#ifdef HAVE_JPEG_H
const FXbool FXJPGIcon::supported=TRUE;
#else
const FXbool FXJPGIcon::supported=FALSE;
#endif


// Initialize
FXJPGIcon::FXJPGIcon(FXApp* a,const void *pix,FXColor clr,FXuint opts,FXint w,FXint h):
  FXIcon(a,NULL,clr,opts,w,h){
  quality=75;
  if(pix){
    FXMemoryStream ms;
    ms.open(FXStreamLoad,(FXuchar*)pix);
    loadPixels(ms);
    ms.close();
    }
  }


// Save pixels only
FXbool FXJPGIcon::savePixels(FXStream& store) const {
  if(!fxsaveJPG(store,data,width,height,quality)) return FALSE;
  return TRUE;
  }


// Load pixels only
FXbool FXJPGIcon::loadPixels(FXStream& store){
  if(options&IMAGE_OWNED){ FXFREE(&data); }
  if(!fxloadJPG(store,data,width,height,quality)) return FALSE;
  if(options&IMAGE_ALPHAGUESS) transp=guesstransp();
  options|=IMAGE_OWNED;
  return TRUE;
  }


// Clean up
FXJPGIcon::~FXJPGIcon(){
  }

}
