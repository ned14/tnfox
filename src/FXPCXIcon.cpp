/********************************************************************************
*                                                                               *
*                        P C X   I c o n   O b j e c t                          *
*                                                                               *
*********************************************************************************
* Copyright (C) 2001,2004 by Janusz Ganczarski.   All Rights Reserved.          *
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
* $Id: FXPCXIcon.cpp,v 1.20 2004/11/10 16:22:05 fox Exp $                       *
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
#include "FXId.h"
#include "FXDrawable.h"
#include "FXImage.h"
#include "FXIcon.h"
#include "FXPCXIcon.h"


/*
  Notes:
  - PCX does not support alpha in the file format.
  - You can also let the system guess a transparancy color based on the corners.
  - If that doesn't work, you can force a specific transparency color.
*/



/*******************************************************************************/

namespace FX {


// Suggested file extension
const FXchar *FXPCXIcon::fileExt="pcx";


// Object implementation
FXIMPLEMENT(FXPCXIcon,FXIcon,NULL,0)


// Initialize nicely
FXPCXIcon::FXPCXIcon(FXApp* a,const void *pix,FXColor clr,FXuint opts,FXint w,FXint h):
  FXIcon(a,NULL,clr,opts,w,h){
  if(pix){
    FXMemoryStream ms;
    ms.open(FXStreamLoad,(FXuchar*)pix);
    loadPixels(ms);
    ms.close();
    }
  }

// Save object to stream
FXbool FXPCXIcon::savePixels(FXStream& store) const {
  if(!fxsavePCX(store,data,width,height)) return FALSE;
  return TRUE;
  }


// Load object from stream
FXbool FXPCXIcon::loadPixels(FXStream& store){
  if(options&IMAGE_OWNED){ FXFREE(&data); }
  if(!fxloadPCX(store,data,width,height)) return FALSE;
  if(options&IMAGE_ALPHAGUESS) transp=guesstransp();
  options|=IMAGE_OWNED;
  return TRUE;
  }


// Clean up
FXPCXIcon::~FXPCXIcon(){
  }

}

