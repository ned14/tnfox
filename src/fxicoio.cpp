/********************************************************************************
*                                                                               *
*                          I C O   I n p u t / O u t p u t                      *
*                                                                               *
*********************************************************************************
* Author: Janusz Ganczarski (POWER)   Email: JanuszG@enter.net.pl               *
* Based on fxbmpio.cpp (FOX) and ico2xpm.c (Copyright (C) 1998 Philippe Martin) *
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
* $Id: fxicoio.cpp,v 1.25 2004/09/17 07:46:22 fox Exp $                         *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "FXHash.h"
#include "FXStream.h"

/*
  Notes:
  - When writing icons, maybe round width/height up to the nearest allowed value.
  - Maybe see if image can be writting with fewer bit/pixel.
  - All padding is zero because the width is 16, 32, or 64.
  - Partially transparent pixels are considered opaque, to prevent loss of
    data.
  - There is no OS/2 ICO format AFAIK.
  - Need to have a pass, save as 32-bit when alpha present.
*/


#define BIH_RGB         0       // biCompression values
#define BIH_RLE8        1
#define BIH_RLE4        2
#define BIH_BITFIELDS   3



/*******************************************************************************/

namespace FX {

extern FXAPI FXbool fxloadICO(FXStream& store,FXColor*& data,FXint& width,FXint& height,FXint& xspot,FXint& yspot);
extern FXAPI FXbool fxsaveICO(FXStream& store,const FXColor *data,FXint width,FXint height,FXint xspot=-1,FXint yspot=-1);



static inline FXuint read16(FXStream& store){
  FXuchar c1,c2;
  store >> c1 >> c2;
  return ((FXuint)c1) | (((FXuint)c2)<<8);
  }


static inline FXuint read32(FXStream& store){
  FXuchar c1,c2,c3,c4;
  store >> c1 >> c2 >> c3 >> c4;
  return ((FXuint)c1) | (((FXuint)c2)<<8) | (((FXuint)c3)<<16) | (((FXuint)c4)<<24);
  }


// Load ICO image from stream
FXbool fxloadICO(FXStream& store,FXColor*& data,FXint& width,FXint& height,FXint& xspot,FXint& yspot){
  FXfval base,header;
  FXColor  colormap[256],*pp;
  FXshort  idReserved;
  FXshort  idType;
  FXshort  idCount;
  FXuchar  bWidth;
  FXuchar  bHeight;
  FXuchar  bColorCount;
  FXuchar  bReserved;
  FXint    dwBytesInRes;
  FXint    dwImageOffset;
  FXint    biSize;
  FXint    biWidth;
  FXint    biHeight;
  FXint    biCompression;
  FXint    biSizeImage;
  FXint    biXPelsPerMeter;
  FXint    biYPelsPerMeter;
  FXint    biClrUsed;
  FXint    biClrImportant;
  FXshort  biPlanes;
  FXshort  biBitCount;
  FXint    i,j,maxpixels,colormaplen,pad;
  FXuchar  c1;
  FXushort rgb16;

  // Null out
  data=NULL;
  width=0;
  height=0;

  // Start of icon file header
  base=store.position();

  // IconHeader
  idReserved=read16(store);     // Must be 0
  idType=read16(store);         // ICO=1, CUR=2
  idCount=read16(store);        // Number of images

  FXTRACE((1,"fxloadICO: idReserved=%d idType=%d idCount=%d\n",idReserved,idType,idCount));

  // Check
  if(idReserved!=0 || (idType!=1 && idType!=2) || idCount<1) return FALSE;

  // IconDirectoryEntry
  store >> bWidth;
  store >> bHeight;
  store >> bColorCount;
  store >> bReserved;
  xspot = read16(store);
  yspot = read16(store);
  dwBytesInRes=read32(store);
  dwImageOffset=read32(store);

  FXTRACE((1,"fxloadICO: bWidth=%d bHeight=%d bColorCount=%d bReserved=%d xspot=%d yspot=%d dwBytesInRes=%d dwImageOffset=%d\n",bWidth,bHeight,bColorCount,bReserved,xspot,yspot,dwBytesInRes,dwImageOffset));

  // Only certain color counts allowed; bColorCount=0 means 256 colors supposedly
  if(bColorCount!=0 && bColorCount!=2 && bColorCount!=4 && bColorCount!=8 && bColorCount!=16) return FALSE;

  // Jump to BitmapInfoHeader
  store.position(base+dwImageOffset);

  // Start of bitmap info header
  header=store.position();

  // BitmapInfoHeader
  biSize=read32(store);
  biWidth=read32(store);
  biHeight=read32(store)/2;     // Huh?
  biPlanes=read16(store);
  biBitCount=read16(store);
  biCompression=read32(store);
  biSizeImage=read32(store);
  biXPelsPerMeter=read32(store);
  biYPelsPerMeter=read32(store);
  biClrUsed=read32(store);
  biClrImportant=read32(store);

  FXTRACE((1,"fxloadICO: biSize=%d biWidth=%d biHeight=%d biPlanes=%d biBitCount=%d biCompression=%d biSizeImage=%d biClrUsed=%d biClrImportant=%d\n",biSize,biWidth,biHeight,biPlanes,biBitCount,biCompression,biSizeImage,biClrUsed,biClrImportant));

  // Check for supported depths
  if(biBitCount!=1 && biBitCount!=4 && biBitCount!=8 && biBitCount!=16 && biBitCount!=24 && biBitCount!=32) return FALSE;

  // Check for supported compression methods
  if(biCompression!=BIH_RGB) return FALSE;

  // Skip ahead to colormap
  store.position(header+biSize);

  // load up colormap, if any
  colormaplen=0;
  if(biBitCount<=8){
    colormaplen = biClrUsed ? biClrUsed : 1 << biBitCount;
    for(i=0; i<colormaplen; i++){
      store >> c1; ((FXuchar*)(colormap+i))[2]=c1;      // Blue
      store >> c1; ((FXuchar*)(colormap+i))[1]=c1;      // Green
      store >> c1; ((FXuchar*)(colormap+i))[0]=c1;      // Red
      store >> c1; ((FXuchar*)(colormap+i))[3]=255;     // Opaque
      }
    }

  // Total number of pixels
  maxpixels=biWidth*biHeight;

  // Allocate memory
  if(!FXMALLOC(&data,FXColor,maxpixels)){ return FALSE; }

  // Width and height
  width=biWidth;
  height=biHeight;

  // 1-bit/pixel
  if(biBitCount==1){
    for(i=biHeight-1; i>=0; i--){
      pp=data+i*biWidth;
      for(j=0; j<biWidth; j++){
        if((j&7)==0){ store >> c1; }
        *pp++=colormap[(c1&0x80)>>7];
        c1<<=1;
        }
      }
    }

  // 4-bit/pixel
  else if(biBitCount==4){
    for(i=biHeight-1; i>=0; i--){
      pp=data+i*biWidth;
      for(j=0; j<biWidth; j++){
        if((j&1)==0){ store >> c1; }
        *pp++=colormap[(c1&0xf0)>>4];
        c1<<=4;
        }
      }
    }

  // 8-bit/pixel
  else if(biBitCount==8){
    for(i=biHeight-1; i>=0; i--){
      pp=data+i*biWidth;
      for(j=0; j<biWidth; j++){
        store >> c1;
        *pp++=colormap[c1];
        }
      }
    }

  // 16-bit/pixel
  else if(biBitCount==16){
    pad=(4-((biWidth*2)&3))&3;
    for(i=biHeight-1; i>=0; i--){
      pp=data+i*biWidth;
      for(j=0; j<biWidth; j++){
        rgb16=read16(store);
        ((FXuchar*)pp)[0]=((rgb16>>7)&0xf8)+((rgb16>>12)&0x7);  // Red
        ((FXuchar*)pp)[1]=((rgb16>>2)&0xf8)+((rgb16>> 7)&0x7);  // Green
        ((FXuchar*)pp)[2]=((rgb16<<3)&0xf8)+((rgb16>> 2)&0x7);  // Blue
        ((FXuchar*)pp)[3]=255;                                  // Alpha
        pp++;
        }
      store.position(pad,FXFromCurrent);
      }
    }

  // 24-bit/pixel
  else if(biBitCount==24){
    pad=(4-((biWidth*3)&3))&3;
    for(i=biHeight-1; i>=0; i--){
      pp=data+i*biWidth;
      for(j=0; j<biWidth; j++){
        store >> ((FXuchar*)pp)[2];             // Blue
        store >> ((FXuchar*)pp)[1];             // Green
        store >> ((FXuchar*)pp)[0];             // Red
        ((FXuchar*)pp)[3]=255;                  // Alpha
        pp++;
        }
      store.position(pad,FXFromCurrent);
      }
    }

  // 32-bit/pixel
  else{
    for(i=biHeight-1; i>=0; i--){
      pp=data+i*biWidth;
      for(j=0; j<biWidth; j++){
        store >> ((FXuchar*)pp)[2];     // Blue
        store >> ((FXuchar*)pp)[1];     // Green
        store >> ((FXuchar*)pp)[0];     // Red
        store >> ((FXuchar*)pp)[3];     // Alpha
        pp++;
        }
      }
    }

  // Read 1-bit alpha data if no alpha channel, and skip otherwise.
  // We need to skip instead of just quit reading because the image
  // may be embedded in a larger stream and we need the byte-count to
  // remain correct for the format.
  if(biBitCount==32){
    store.position(store.position()+height*(width>>3));
    }
  else{
    pad=(4-((width+7)/8))&3;
    for(i=height-1; i>=0; i--){
      pp=data+i*width;
      for(j=0; j<width; j++){
        if((j&7)==0){ store >> c1; }
        ((FXuchar*)pp)[3]=~-((c1&0x80)>>7);       // Groovy!
        c1<<=1;
        pp++;
        }
      store.position(pad,FXFromCurrent);
      }
    }

  FXTRACE((1,"OK\n"));
  return TRUE;
  }

/*******************************************************************************/


static inline void write16(FXStream& store,FXuint i){
  FXuchar c1,c2;
  c1=i&0xff;
  c2=(i>>8)&0xff;
  store << c1 << c2;
  }


static inline void write32(FXStream& store,FXuint i){
  FXuchar c1,c2,c3,c4;
  c1=i&0xff;
  c2=(i>>8)&0xff;
  c3=(i>>16)&0xff;
  c4=(i>>24)&0xff;
  store << c1 << c2 << c3 << c4;
  }


// Save a ICO file to a stream
FXbool fxsaveICO(FXStream& store,const FXColor *data,FXint width,FXint height,FXint xspot,FXint yspot){
  const FXint    biSize=40;
  const FXshort  biPlanes=1;
  const FXint    biCompression=BIH_RGB;
  const FXint    biXPelsPerMeter=0;
  const FXint    biYPelsPerMeter=0;
  const FXint    biClrUsed=0;
  const FXint    biClrImportant=0;
  const FXshort  idReserved=0;
  const FXshort  idCount=1;
  const FXuchar  bReserved=0;
  const FXuchar  bColorCount=0;
  const FXint    dwImageOffset=22;
  const FXColor *pp;
  const FXuchar  padding[3]={0,0,0};
  FXshort        biBitCount=24;
  FXshort        idType=2;
  FXint          biSizeImage=width*height*3;
  FXint          dwBytesInRes=biSize+biSizeImage+height*(width>>3);
  FXuchar        bWidth=(FXuchar)width;
  FXuchar        bHeight=(FXuchar)height;
  FXint          i,j,pad;
  FXuchar        c,bit;

  // Must make sense
  if(!data || width<=0 || height<=0) return FALSE;

  // Quick pass to see if alpha<255 anywhere
  for(i=width*height-1; i>=0; i--){
    if(((FXuchar*)(data+i))[3]<255){ biBitCount=32; break; }
    }

  // If no hot-spot given, save as an icon instead of a cursor
  if(xspot<0 || yspot<0){ xspot=yspot=0; idType=1; }

  // IconHeader
  write16(store,idReserved);    // Must be zero
  write16(store,idType);        // Must be 1 or 2
  write16(store,idCount);       // Only one icon

  // IconDirectoryEntry
  store << bWidth;
  store << bHeight;
  store << bColorCount;         // 0 for > 8bit/pixel
  store << bReserved;
  write16(store,xspot);
  write16(store,yspot);
  write32(store,dwBytesInRes);  // Total number of bytes in images (including palette data)
  write32(store,dwImageOffset); // Location of image from the beginning of file

  // BitmapInfoHeader
  write32(store,biSize);
  write32(store,width);
  write32(store,height+height);
  write16(store,biPlanes);
  write16(store,biBitCount);
  write32(store,biCompression);
  write32(store,biSizeImage);
  write32(store,biXPelsPerMeter);
  write32(store,biYPelsPerMeter);
  write32(store,biClrUsed);
  write32(store,biClrImportant);

  // Write 24-bit rgb data
  if(biBitCount==24){
    pad=(4-((width*3)&3))&3;
    for(i=height-1; i>=0; i--){
      pp=data+i*width;
      for(j=0; j<width; j++){
        store << ((FXuchar*)pp)[2];
        store << ((FXuchar*)pp)[1];
        store << ((FXuchar*)pp)[0];
        pp++;
        }
      store.save(padding,pad);
      }
    }

  // 32-bit/pixel
  else{
    for(i=height-1; i>=0; i--){
      pp=data+i*width;
      for(j=0; j<width; j++){
        store << ((FXuchar*)pp)[2];
        store << ((FXuchar*)pp)[1];
        store << ((FXuchar*)pp)[0];
        store << ((FXuchar*)pp)[3];
        pp++;
        }
      }
    }

  // Write 1-bit alpha data
  pad=(4-((width+7)/8))&3;
  for(i=height-1; i>=0; i--){
    pp=data+i*width;
    for(j=c=0,bit=0x80; j<width; j++){
      if(((FXuchar*)pp)[3]==0) c|=bit;  // Only transparent if FULLY transparent!
      bit>>=1;
      if(bit==0){
        store << c;
        bit=0x80;
        c=0;
        }
      pp++;
      }
    }
  store.save(padding,pad);

  return TRUE;
  }

}
