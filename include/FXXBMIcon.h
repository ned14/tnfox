/********************************************************************************
*                                                                               *
*                        X B M   I c o n   O b j e c t                          *
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
* $Id: FXXBMIcon.h,v 1.5 2004/02/08 17:17:34 fox Exp $                          *
********************************************************************************/
#ifndef FXXBMICON_H
#define FXXBMICON_H

#ifndef FXICON_H
#include "FXIcon.h"
#endif

namespace FX {


/// X Bitmap icon
class FXAPI FXXBMIcon : public FXIcon {
  FXDECLARE(FXXBMIcon)
protected:
  FXXBMIcon(){}
private:
  FXXBMIcon(const FXXBMIcon&);
  FXXBMIcon &operator=(const FXXBMIcon&);
public:

  /// Construct icon from memory stream formatted in X Bitmap format
  FXXBMIcon(FXApp* a,const FXuchar *pixels=NULL,const FXuchar *mask=NULL,FXColor clr=FXRGB(192,192,192),FXuint opts=0,FXint w=1,FXint h=1);

  /// Save pixels into stream in X Bitmap format
  virtual FXbool savePixels(FXStream& store) const;

  /// Load pixels from stream in X Bitmap format
  virtual FXbool loadPixels(FXStream& store);

  /// Destroy icon
  virtual ~FXXBMIcon();
  };


#ifndef FXLOADXBM
#define FXLOADXBM

/**
* Load an XBM (X Bitmap) from pixel array and mask array.
* Upon successful return, the pixel array and size are returned.
* If an error occurred, the pixel array is set to NULL.
*/
extern FXAPI FXbool fxloadXBM(FXColor*& data,const FXuchar *pix,const FXuchar *msk,FXint width,FXint height);


/**
* Load an XBM (X Bitmap) file from a stream.
* Upon successful return, the pixel array and size, and hot-spot are returned.
* If an error occurred, the pixel array is set to NULL.
*/
extern FXAPI FXbool fxloadXBM(FXStream& store,FXColor*& data,FXint& width,FXint& height,FXint& hotx,FXint& hoty);


/**
* Save an XBM (X Bitmap) file to a stream; if the parameters hotx and hoty are set
* to -1, no hotspot location is saved.
*/
extern FXAPI FXbool fxsaveXBM(FXStream& store,const FXColor *data,FXint width,FXint height,FXint hotx=-1,FXint hoty=-1);

/**
* Save a PostScript file to a stream; format the picture to the maximal
* size that fits within the given margins of the indicated paper size.
*/
extern FXAPI FXbool fxsavePS(FXStream& store,const FXColor *data,FXint width,FXint height,FXint paperw=612,FXint paperh=792,FXint margin=35,FXbool color=TRUE);

#endif

}

#endif
