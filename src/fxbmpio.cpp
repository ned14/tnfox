/********************************************************************************
*                                                                               *
*                          B M P   I n p u t / O u t p u t                      *
*                                                                               *
*********************************************************************************
* Copyright (C) 1998,2005 by Jeroen van der Zijp.   All Rights Reserved.        *
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
* $Id: fxbmpio.cpp,v 1.50 2005/01/16 16:06:07 fox Exp $                         *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "FXHash.h"
#include "FXStream.h"



/*
  Notes:
  - Writer should use fxezquantize() and if the number of colors is less than
    256, use 8bpp RLE compressed output; if less that 4, use 4bpp RLE compressed
    output, else if less than 2, use monochrome.
  - Writer should do this only when no loss of fidelity occurs.
  - Find documentation on 32-bpp bitmap.
  - When new FXStream is here, update reader/writer for byte-swapped i/o.
  - Need to have a pass, save as 32-bit when alpha present.
  - Need to have checks in RLE decoder for out-of-bounds checking.
  - To map from 5-bit to 8-bit, we use value*8+floor(value/4) which
    is almost the same as the correct value*8.225806.
*/

#define BIH_RGB         0       // biCompression values
#define BIH_RLE8        1
#define BIH_RLE4        2
#define BIH_BITFIELDS   3

#define OS2_OLD         12      // biSize values
#define WIN_NEW         40
#define OS2_NEW         64




/*******************************************************************************/

namespace FX {


extern FXAPI FXbool fxcheckBMP(FXStream& store);
extern FXAPI FXbool fxloadBMP(FXStream& store,FXColor*& data,FXint& width,FXint& height);
extern FXAPI FXbool fxsaveBMP(FXStream& store,const FXColor *data,FXint width,FXint height);



static inline FXuint read32(FXStream& store){
  FXuchar c1,c2,c3,c4;
  store >> c1 >> c2 >> c3 >> c4;
  return ((FXuint)c1) | (((FXuint)c2)<<8) | (((FXuint)c3)<<16) | (((FXuint)c4)<<24);
  }


static inline FXuint read16(FXStream& store){
  FXuchar c1,c2;
  store >> c1 >> c2;
  return ((FXuint)c1) | (((FXuint)c2)<<8);
  }


// Check if stream contains a BMP
FXbool fxcheckBMP(FXStream& store){
  FXuchar signature[2];
  store.load(signature,2);
  store.position(-2,FXFromCurrent);
  return signature[0]=='B' && signature[1]=='M';
  }


// Load image from stream
FXbool fxloadBMP(FXStream& store,FXColor*& data,FXint& width,FXint& height){
  FXint biXPelsPerMeter,biYPelsPerMeter,biClrUsed,biClrImportant,biCompression,biSize;
  FXint biSizeImage,bfOffBits,i,j,x,y,maxpixels,colormaplen,padw,pad;
  FXushort biBitCount,biPlanes,biWidth,biHeight,rgb16;
  FXColor colormap[256],*pp;
  FXuchar padding[3],c1,c2;
  FXlong base,header;

  // Null out
  data=NULL;
  width=0;
  height=0;

  // Start of bitmap file header
  base=store.position();

  // Check signature
  store >> c1 >> c2;
  if(c1!='B' || c2!='M') return FALSE;

  // Get size and offset
  read32(store);
  read16(store);
  read16(store);
  bfOffBits=read32(store);

  // Start of bitmap info header
  header=store.position();

  // Read bitmap info header
  biSize=read32(store);
  if(biSize==OS2_OLD){                  // Old format
    biWidth         = read16(store);
    biHeight        = read16(store);
    biPlanes        = read16(store);
    biBitCount      = read16(store);
    biCompression   = BIH_RGB;
    biSizeImage     = (((biPlanes*biBitCount*biWidth)+31)/32)*4*biHeight;
    biXPelsPerMeter = 0;
    biYPelsPerMeter = 0;
    biClrUsed       = 0;
    biClrImportant  = 0;
    }
  else{                                 // New format
    biWidth         = read32(store);
    biHeight        = read32(store);
    biPlanes        = read16(store);
    biBitCount      = read16(store);
    biCompression   = read32(store);
    biSizeImage     = read32(store);
    biXPelsPerMeter = read32(store);
    biYPelsPerMeter = read32(store);
    biClrUsed       = read32(store);
    biClrImportant  = read32(store);
    }

//  FXTRACE((1,"fxloadBMP: biWidth=%d biHeight=%d biPlanes=%d biBitCount=%d biCompression=%d biClrUsed=%d biClrImportant=%d\n",biWidth,biHeight,biPlanes,biBitCount,biCompression,biClrUsed,biClrImportant));

  // Ought to be 1
  if(biPlanes!=1) return FALSE;

  // Check for supported depths
  if(biBitCount!=1 && biBitCount!=4 && biBitCount!=8 && biBitCount!=16 && biBitCount!=24 && biBitCount!=32) return FALSE;

  // Check for supported compression methods
  if(biCompression!=BIH_RGB && biCompression!=BIH_RLE4 && biCompression!=BIH_RLE8 && biCompression!=BIH_BITFIELDS) return FALSE;

  // Skip ahead to colormap
  store.position(header+biSize);

  // Load up colormap, if needed
  colormaplen=0;
  if(biBitCount<=8){
    colormaplen = biClrUsed ? biClrUsed : 1<<biBitCount;
    FXASSERT(colormaplen<=256);
    for(i=0; i<colormaplen; i++){
      store >> c1;                    ((FXuchar*)(colormap+i))[2]=c1;      // Blue
      store >> c1;                    ((FXuchar*)(colormap+i))[1]=c1;      // Green
      store >> c1;                    ((FXuchar*)(colormap+i))[0]=c1;      // Red
      if(biSize!=OS2_OLD){store>>c1;} ((FXuchar*)(colormap+i))[3]=255;     // Opaque
      }
    }

  // Jump to start of actual bitmap data
  if(biSize!=OS2_OLD){
    store.position(base+bfOffBits);
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
    padw=(biWidth+31)&~31;
    for(i=biHeight-1; i>=0; i--){
      pp=data+i*biWidth;
      for(j=0; j<padw; j++){
        if((j&7)==0){ store >> c1; }
        if(j<biWidth){ *pp++=colormap[(c1&0x80)>>7]; c1<<=1; }
        }
      }
    }

  // 4-bit/pixel
  else if(biBitCount==4){
    if(biCompression==BIH_RGB){               // Read uncompressed data
      padw=(biWidth+7)&~7;
      for(i=biHeight-1; i>=0; i--){
        pp=data+i*biWidth;
        for(j=0; j<padw; j++){
          if((j&1)==0){ store >> c1; }
          if(j<biWidth){ *pp++=colormap[(c1&0xf0)>>4]; c1<<=4; }
          }
        }
      }
    else{                                     // Read RLE4 compressed data
      x=y=0;
      pp=data+(biHeight-1)*biWidth;
      while(y<biHeight){
        store >> c2;
        if(c2){                                // Encoded mode
          store >> c1;
          for(i=0; i<c2; i++,x++){
            *pp++=colormap[(i&1)?(c1&0x0f):((c1>>4)&0x0f)];
            }
          }
        else{                                 // Escape codes
          store >> c2;
          if(c2==0){                          // End of line
            x=0;
            y++;
            pp=data+(biHeight-y-1)*biWidth;
            }
          else if(c2==0x01){                  // End of pic8
            break;
            }
          else if(c2==0x02){                  // Delta
            store >> c2; x+=c2;
            store >> c2; y+=c2;
            pp=data+x+(biHeight-y-1)*biWidth;
            }
          else{                               // Absolute mode
            for(i=0; i<c2; i++,x++){
              if((i&1)==0){ store >> c1; }
              *pp++=colormap[(i&1)?(c1&0x0f):((c1>>4)&0x0f)];
              }
            if(((c2&3)==1) || ((c2&3)==2)) store >> c1;       // Read pad byte
            }
          }
        }
      }
    }

  // 8-bit/pixel
  else if(biBitCount==8){
    if(biCompression==BIH_RGB){               // Read uncompressed data
      padw=(biWidth+3)&~3;
      for(i=biHeight-1; i>=0; i--){
        pp=data+i*biWidth;
        for(j=0; j<padw; j++){
          store >> c1;
          if(j<biWidth) *pp++=colormap[c1];
          }
        }
      }
    else{                                     // Read RLE8 compressed data
      x=y=0;
      pp=data+(biHeight-1)*biWidth;
      while(y<biHeight){
        store >> c2;
        if(c2){                                // Encoded mode
          store >> c1;
          for(i=0; i<c2; i++,x++){
            *pp++=colormap[c1];
            }
          }
        else{                                 // Escape codes
          store >> c2;
          if(c2==0x00){                        // End of line
            x=0;
            y++;
            pp=data+(biHeight-y-1)*biWidth;
            }
          else if(c2==0x01){                   // End of pic8
            break;
            }
          else if(c2==0x02){                   // delta
            store >> c2; x+=c2;
            store >> c2; y+=c2;
            pp=data+x+(biHeight-y-1)*biWidth;
            }
          else{                               // Absolute mode
            for(i=0; i<c2; i++,x++){
              store >> c1;
              *pp++=colormap[c1];
              }
            if(c2&1) store >> c1;              // Odd length run: read an extra pad byte
            }
          }
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
        ((FXuchar*)pp)[0]=((rgb16>>7)&0xf8)+((rgb16>>12)&7);  // Red
        ((FXuchar*)pp)[1]=((rgb16>>2)&0xf8)+((rgb16>> 7)&7);  // Green
        ((FXuchar*)pp)[2]=((rgb16<<3)&0xf8)+((rgb16>> 2)&7);  // Blue
        ((FXuchar*)pp)[3]=255;                                // Alpha
        pp++;
        }
      store.load(padding,pad);
      }
    }

  // 24-bit/pixel
  else if(biBitCount==24){
    pad=(4-((biWidth*3)&3))&3;
    for(i=biHeight-1; i>=0; i--){
      pp=data+i*biWidth;
      for(j=0; j<biWidth; j++){
        store >> ((FXuchar*)pp)[2];     // Blue
        store >> ((FXuchar*)pp)[1];     // Green
        store >> ((FXuchar*)pp)[0];     // Red
        ((FXuchar*)pp)[3]=255;          // Alpha
        pp++;
        }
      store.load(padding,pad);
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

  return TRUE;
  }


/*******************************************************************************/


static inline void write16(FXStream& store,FXushort i){
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


// Save a bmp file to a stream
FXbool fxsaveBMP(FXStream& store,const FXColor *data,FXint width,FXint height){
  FXint biSizeImage,bfSize,bfOffBits,bperlin,i,j,pad;
  const FXshort biPlanes=1;
  const FXshort bfReserved=0;
  const FXint biXPelsPerMeter=75*39;
  const FXint biYPelsPerMeter=75*39;
  const FXint biClrUsed=0;
  const FXint biClrImportant=0;
  const FXint biCompression=BIH_RGB;
  const FXint biSize=40;
  const FXint bfHeader=14;
  const FXuchar padding[3]={0,0,0};
  const FXColor *pp;
  FXshort biBitCount=24;

  // Must make sense
  if(!data || width<=0 || height<=0) return FALSE;

  // Quick pass to see if alpha<255 anywhere
  for(i=width*height-1; i>=0; i--){
    if(((FXuchar*)(data+i))[3]<255){ biBitCount=32; break; }
    }

  // Number of bytes written per line
  bperlin=((width*biBitCount+31)/32)*4;

  // Size of raw image data
  biSizeImage=bperlin*height;

  // Offset to image data
  bfOffBits=bfHeader+biSize+biClrUsed*4;

  // Compute file size// size of bitmap file
  bfSize=bfHeader+biSize+biClrUsed*4+biSizeImage;

  // BitmapFileHeader
  store << 'B';                         // Magic number
  store << 'M';
  write32(store,bfSize);                // bfSize: size of file
  write16(store,bfReserved);            // bfReserved1
  write16(store,bfReserved);            // bfReserved2
  write32(store,bfOffBits);             // bfOffBits

  // BitmapInfoHeader
  write32(store,biSize);                // biSize: size of bitmap info header
  write32(store,width);                 // biWidth
  write32(store,height);                // biHeight
  write16(store,biPlanes);              // biPlanes:  must be '1'
  write16(store,biBitCount);            // biBitCount (1,4,8,24, or 32)
  write32(store,biCompression);         // biCompression:  BIH_RGB, BIH_RLE8, BIH_RLE4, or BIH_BITFIELDS
  write32(store,biSizeImage);           // biSizeImage:  size of raw image data
  write32(store,biXPelsPerMeter);       // biXPelsPerMeter: (75dpi * 39" per meter)
  write32(store,biYPelsPerMeter);       // biYPelsPerMeter: (75dpi * 39" per meter)
  write32(store,biClrUsed);             // biClrUsed
  write32(store,biClrImportant);        // biClrImportant

  // 24-bit/pixel
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

  return TRUE;
  }

}
