/********************************************************************************
*                                                                               *
*                        P P M   I c o n   O b j e c t                          *
*                                                                               *
*********************************************************************************
* Copyright (C) 2003,2004 by Jeroen van der Zijp.   All Rights Reserved.        *
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
* $Id: FXPPMIcon.h,v 1.4 2004/11/10 16:22:05 fox Exp $                          *
********************************************************************************/
#ifndef FXPPMICON_H
#define FXPPMICON_H

#ifndef FXICON_H
#include "FXIcon.h"
#endif

namespace FX {


/// Portable Pixmap icon
class FXAPI FXPPMIcon : public FXIcon {
  FXDECLARE(FXPPMIcon)
protected:
  FXPPMIcon(){}
private:
  FXPPMIcon(const FXPPMIcon&);
  FXPPMIcon &operator=(const FXPPMIcon&);
public:
  static const FXchar *fileExt;
public:

  /// Construct icon from memory stream formatted in Portable Pixmap format
  FXPPMIcon(FXApp* a,const void *pix=NULL,FXColor clr=FXRGB(192,192,192),FXuint opts=0,FXint w=1,FXint h=1);

  /// Save pixels into stream in Portable Pixmap format
  virtual FXbool savePixels(FXStream& store) const;

  /// Load pixels from stream in Portable Pixmap format
  virtual FXbool loadPixels(FXStream& store);

  /// Destroy icon
  virtual ~FXPPMIcon();
  };


/**
* Load an PPM (Portable Pixmap Format) file from a stream.
* Upon successful return, the pixel array and size are returned.
* If an error occurred, the pixel array is set to NULL.
*/
extern FXAPI FXbool fxloadPPM(FXStream& store,FXColor*& data,FXint& width,FXint& height);


/**
* Save an PPM (Portable Pixmap Format) file to a stream.
*/
extern FXAPI FXbool fxsavePPM(FXStream& store,const FXColor *data,FXint width,FXint height);

}

#endif
