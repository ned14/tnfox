/********************************************************************************
*                                                                               *
*                            I c o n   S o u r c e                              *
*                                                                               *
*********************************************************************************
* Copyright (C) 2005 by Jeroen van der Zijp.   All Rights Reserved.             *
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
* $Id: FXIconSource.cpp,v 1.12 2005/02/08 06:12:35 fox Exp $                    *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "FXHash.h"
#include "FXThread.h"
#include "FXStream.h"
#include "FXFileStream.h"
#include "FXMemoryStream.h"
#include "FXString.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXFile.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXApp.h"
#include "FXIcon.h"
#include "FXImage.h"
#include "FXIconSource.h"

// Built-in icon formats
#include "FXBMPIcon.h"
#include "FXGIFIcon.h"
#include "FXICOIcon.h"
#include "FXPCXIcon.h"
#include "FXPPMIcon.h"
#include "FXRGBIcon.h"
#include "FXTGAIcon.h"
#include "FXXBMIcon.h"
#include "FXXPMIcon.h"

// Built-in image formats
#include "FXBMPImage.h"
#include "FXGIFImage.h"
#include "FXICOImage.h"
#include "FXPCXImage.h"
#include "FXPPMImage.h"
#include "FXRGBImage.h"
#include "FXTGAImage.h"
#include "FXXBMImage.h"
#include "FXXPMImage.h"

// Formats requiring external libraries
#ifndef CORE_IMAGE_FORMATS
#ifdef HAVE_JPEG_H
#include "FXJPGIcon.h"
#include "FXJPGImage.h"
#endif
#ifdef HAVE_PNG_H
#include "FXPNGIcon.h"
#include "FXPNGImage.h"
#endif
#ifdef HAVE_TIFF_H
#include "FXTIFIcon.h"
#include "FXTIFImage.h"
#endif
#endif


/*
  Notes:
  - Either load an icon from a file, or load from already open stream.
  - Future versions will take advantage of the new fxcheckXXX() functions
    to predicates to determine from the data itself what the type is;
    currently this is quite unreliable due to more extensive analysis
    needed of the image file format recognition.
*/


using namespace FX;

/*******************************************************************************/

namespace FX {


FXIMPLEMENT(FXIconSource,FXObject,NULL,0)


// Initialize icon source
FXIconSource::FXIconSource(FXApp* a):app(a){
  FXTRACE((100,"FXIconSource::FXIconSource\n"));
  }


// Scale image or icon to size
FXImage* FXIconSource::scaleToSize(FXImage *image,FXint size,FXint qual) const {
  if(image){
    if((image->getWidth()>size) || (image->getHeight()>size)){
      if(image->getWidth()>image->getHeight()){
        image->scale(size,(size*image->getHeight())/image->getWidth(),qual);
        }
      else{
        image->scale((size*image->getWidth())/image->getHeight(),size,qual);
        }
      }
    }
  return image;
  }


// Load from file
FXIcon *FXIconSource::loadIcon(const FXString& filename,const FXString& type) const {
  FXIcon *icon=NULL;
  FXTRACE((150,"FXIconSource loadIcon(%s)\n",filename.text()));
  if(!filename.empty()){
    FXFileStream store;
    if(store.open(filename,FXStreamLoad,65536)){
      if(type.empty()){
        icon=loadIcon(store,FXFile::extension(filename));
        }
      else{
        icon=loadIcon(store,type);
        }
      store.close();
      }
    }
  return icon;
  }


// Load from data array
FXIcon *FXIconSource::loadIcon(const void *pixels,const FXString& type) const {
  FXIcon *icon=NULL;
  if(pixels){
    FXMemoryStream store;
    store.open(FXStreamLoad,(FXuchar*)pixels);
    icon=loadIcon(store,type);
    store.close();
    }
  return icon;
  }


// Load from already open stream
FXIcon *FXIconSource::loadIcon(FXStream& store,const FXString& type) const {
  FXIcon *icon=NULL;
  if(comparecase(FXBMPIcon::fileExt,type)==0){
    icon=new FXBMPIcon(app);
    }
  else if(comparecase(FXGIFIcon::fileExt,type)==0){
    icon=new FXGIFIcon(app);
    }
  else if(comparecase(FXICOIcon::fileExt,type)==0){
    icon=new FXICOIcon(app);
    }
  else if(comparecase(FXPCXIcon::fileExt,type)==0){
    icon=new FXPCXIcon(app);
    }
  else if(comparecase(FXPPMIcon::fileExt,type)==0){
    icon=new FXPPMIcon(app);
    }
  else if(comparecase(FXRGBIcon::fileExt,type)==0){
    icon=new FXRGBIcon(app);
    }
  else if(comparecase(FXTGAIcon::fileExt,type)==0){
    icon=new FXTGAIcon(app);
    }
  else if(comparecase(FXXBMIcon::fileExt,type)==0){
    icon=new FXXBMIcon(app);
    }
  else if(comparecase(FXXPMIcon::fileExt,type)==0){
    icon=new FXXPMIcon(app);
    }
#ifndef CORE_IMAGE_FORMATS
#ifdef HAVE_JPEG_H
  else if(comparecase(FXJPGIcon::fileExt,type)==0){
    icon=new FXJPGIcon(app);
    }
#endif
#ifdef HAVE_PNG_H
  else if(comparecase(FXPNGIcon::fileExt,type)==0){
    icon=new FXPNGIcon(app);
    }
#endif
#ifdef HAVE_TIFF_H
  else if(comparecase(FXTIFIcon::fileExt,type)==0){
    icon=new FXTIFIcon(app);
    }
#endif
#endif
  if(icon){
    if(icon->loadPixels(store)) return icon;
    delete icon;
    }
  return NULL;
  }


// Load from file
FXImage *FXIconSource::loadImage(const FXString& filename,const FXString& type) const {
  FXImage *image=NULL;
  FXTRACE((150,"FXIconSource loadImage(%s)\n",filename.text()));
  if(!filename.empty()){
    FXFileStream store;
    if(store.open(filename,FXStreamLoad,65536)){
      if(type.empty()){
        image=loadImage(store,FXFile::extension(filename));
        }
      else{
        image=loadImage(store,type);
        }
      store.close();
      }
    }
  return image;
  }


// Load from data array
FXIcon *FXIconSource::loadImage(const void *pixels,const FXString& type) const {
  FXIcon *icon=NULL;
  if(pixels){
    FXMemoryStream store;
    store.open(FXStreamLoad,(FXuchar*)pixels);
    icon=loadIcon(store,type);
    store.close();
    }
  return icon;
  }


// Load from already open stream
FXImage *FXIconSource::loadImage(FXStream& store,const FXString& type) const {
  FXImage *image=NULL;
  if(comparecase(FXBMPImage::fileExt,type)==0){
    image=new FXBMPImage(app);
    }
  else if(comparecase(FXGIFImage::fileExt,type)==0){
    image=new FXGIFImage(app);
    }
  else if(comparecase(FXICOImage::fileExt,type)==0){
    image=new FXICOImage(app);
    }
  else if(comparecase(FXPCXImage::fileExt,type)==0){
    image=new FXPCXImage(app);
    }
  else if(comparecase(FXPPMImage::fileExt,type)==0){
    image=new FXPPMImage(app);
    }
  else if(comparecase(FXRGBImage::fileExt,type)==0){
    image=new FXRGBImage(app);
    }
  else if(comparecase(FXTGAImage::fileExt,type)==0){
    image=new FXTGAImage(app);
    }
  else if(comparecase(FXXBMImage::fileExt,type)==0){
    image=new FXXBMImage(app);
    }
  else if(comparecase(FXXPMImage::fileExt,type)==0){
    image=new FXXPMImage(app);
    }
#ifndef CORE_IMAGE_FORMATS
#ifdef HAVE_JPEG_H
  else if(comparecase(FXJPGImage::fileExt,type)==0){
    image=new FXJPGImage(app);
    }
#endif
#ifdef HAVE_PNG_H
  else if(comparecase(FXPNGImage::fileExt,type)==0){
    image=new FXPNGImage(app);
    }
#endif
#ifdef HAVE_TIFF_H
  else if(comparecase(FXTIFImage::fileExt,type)==0){
    image=new FXTIFImage(app);
    }
#endif
#endif
  if(image){
    if(image->loadPixels(store)) return image;
    delete image;
    }
  return NULL;
  }


// Load icon and scale it such that its dimensions does not exceed given size
FXIcon *FXIconSource::loadScaledIcon(const FXString& filename,FXint size,FXint qual,const FXString& type) const {
  return (FXIcon*)scaleToSize(loadIcon(filename,type),size,qual);
  }


// Load from data array
FXIcon *FXIconSource::loadScaledIcon(const void *pixels,FXint size,FXint qual,const FXString& type) const {
  return (FXIcon*)scaleToSize(loadIcon(pixels,type),size,qual);
  }


// Load icon and scale it such that its dimensions does not exceed given size
FXIcon *FXIconSource::loadScaledIcon(FXStream& store,FXint size,FXint qual,const FXString& type) const {
  return (FXIcon*)scaleToSize(loadIcon(store,type),size,qual);
  }


// Load image and scale it such that its dimensions does not exceed given size
FXImage *FXIconSource::loadScaledImage(const FXString& filename,FXint size,FXint qual,const FXString& type) const {
  return scaleToSize(loadImage(filename,type),size,qual);
  }


// Load from data array
FXImage *FXIconSource::loadScaledImage(const void *pixels,FXint size,FXint qual,const FXString& type) const {
  return (FXImage*)scaleToSize(loadImage(pixels,type),size,qual);
  }


// Load image and scale it such that its dimensions does not exceed given size
FXImage *FXIconSource::loadScaledImage(FXStream& store,FXint size,FXint qual,const FXString& type) const {
  return scaleToSize(loadImage(store,type),size,qual);
  }


// Save to stream
void FXIconSource::save(FXStream& store) const {
  FXObject::save(store);
  store << app;
  }


// Load from stream
void FXIconSource::load(FXStream& store){
  FXObject::load(store);
  store >> app;
  }


// Delete
FXIconSource::~FXIconSource(){
  FXTRACE((100,"FXIconSource::~FXIconSource\n"));
  app=(FXApp*)-1L;
  }



}
