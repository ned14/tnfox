/********************************************************************************
*                                                                               *
*                            P C X   I m a g e   O b j e c t                    *
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
* $Id: FXPCXImage.h,v 1.13 2004/11/10 16:22:05 fox Exp $                        *
********************************************************************************/
#ifndef FXPCXIMAGE_H
#define FXPCXIMAGE_H

#ifndef FXIMAGE_H
#include "FXImage.h"
#endif

namespace FX {

///  PCX graphics file
class FXAPI FXPCXImage : public FXImage {
  FXDECLARE(FXPCXImage)
protected:
  FXPCXImage(){}
private:
  FXPCXImage(const FXPCXImage&);
  FXPCXImage &operator=(const FXPCXImage&);
public:
  static const FXchar *fileExt;
public:

  /// Construct image from memory stream formatted in PCX file
  FXPCXImage(FXApp* a,const void *pix=NULL,FXuint opts=0,FXint w=1,FXint h=1);

  /// Save pixels into stream in PCX file
  virtual FXbool savePixels(FXStream& store) const;

  /// Load pixels from stream in  bitmap format
  virtual FXbool loadPixels(FXStream& store);

  /// Destroy icon
  virtual ~FXPCXImage();
  };


/**
* Load an PCX (PC Paintbrush) file from a stream.
* Upon successful return, the pixel array and size are returned.
* If an error occurred, the pixel array is set to NULL.
*/
extern FXAPI FXbool fxloadPCX(FXStream& store,FXColor*& data,FXint& width,FXint& height);


/**
* Save an PCX (PC Paintbrush) file to a stream.
*/
extern FXAPI FXbool fxsavePCX(FXStream& store,const FXColor *data,FXint width,FXint height);

}

#endif
