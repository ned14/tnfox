/********************************************************************************
*                                                                               *
*                        B M P   I c o n   O b j e c t                          *
*                                                                               *
*********************************************************************************
* Copyright (C) 1997,2004 by Jeroen van der Zijp.   All Rights Reserved.        *
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
* $Id: FXBMPIcon.h,v 1.15 2004/02/08 17:17:33 fox Exp $                         *
********************************************************************************/
#ifndef FXBMPICON_H
#define FXBMPICON_H

#ifndef FXICON_H
#include "FXIcon.h"
#endif

namespace FX {


/// Microsoft Bitmap icon
class FXAPI FXBMPIcon : public FXIcon {
  FXDECLARE(FXBMPIcon)
protected:
  FXBMPIcon(){}
private:
  FXBMPIcon(const FXBMPIcon&);
  FXBMPIcon &operator=(const FXBMPIcon&);
public:

  /// Construct icon from memory stream formatted in Microsoft BMP format
  FXBMPIcon(FXApp* a,const void *pix=NULL,FXColor clr=FXRGB(192,192,192),FXuint opts=0,FXint w=1,FXint h=1);

  /// Save pixels into stream in Microsoft bitmap format
  virtual FXbool savePixels(FXStream& store) const;

  /// Load pixels from stream in Microsoft bitmap format
  virtual FXbool loadPixels(FXStream& store);

  /// Destroy icon
  virtual ~FXBMPIcon();
  };


/**
* Load an BMP (Microsoft Bitmap) file from a stream.
* Upon successful return, the pixel array and size are returned.
* If an error occurred, the pixel array is set to NULL.
*/
extern FXAPI FXbool fxloadBMP(FXStream& store,FXColor*& data,FXint& width,FXint& height);


/**
* Save an BMP (Microsoft Bitmap) file to a stream.
*/
extern FXAPI FXbool fxsaveBMP(FXStream& store,const FXColor *data,FXint width,FXint height);

}

#endif
