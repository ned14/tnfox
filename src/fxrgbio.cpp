/********************************************************************************
*                                                                               *
*                    I R I S   R G B   I n p u t / O u t p u t                  *
*                                                                               *
*********************************************************************************
* Copyright (C) 2002,2004 by Jeroen van der Zijp.   All Rights Reserved.        *
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
* $Id: fxrgbio.cpp,v 1.19 2004/09/17 07:46:22 fox Exp $                         *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "FXHash.h"
#include "FXStream.h"



/*
  Notes:
  - Need to implement RLE compression some time.
  - Bad data may core reader.
*/



/*******************************************************************************/

namespace FX {


extern FXAPI FXbool fxloadRGB(FXStream& store,FXColor*& data,FXint& width,FXint& height);
extern FXAPI FXbool fxsaveRGB(FXStream& store,const FXColor *data,FXint width,FXint height);


// Read from MSB order
static FXuint read32(FXStream& store){
  FXuchar c1,c2,c3,c4;
  store >> c1 >> c2 >> c3 >> c4;
  return ((FXuint)c4) | (((FXuint)c3)<<8) | (((FXuint)c2)<<16) | (((FXuint)c1)<<24);
  }


// Read from MSB order
static FXushort read16(FXStream& store){
  FXuchar c1,c2;
  store >> c1 >> c2;
  return ((FXuint)c2) | (((FXuint)c1)<<8);
  }


// Read table
static void readtab(FXStream& store,FXuint* tab,FXint n){
  for(register FXint i=0; i<n; i++){ tab[i]=read32(store); }
  }


// RLE decompress
static void expandrow(FXuchar* optr,FXuchar *iptr){   // FIXME bad data could blow past array!!
  unsigned char pixel, count;
  while(1){
    pixel=*iptr++;
    count=pixel&0x7f;
    if(count==0) return;
    if(pixel&0x80){   // Literal bytes
      while(count--){
	*optr=*iptr++;
	optr+=4;
        }
      }
    else{             // Repeated bytes
      pixel=*iptr++;
      while(count--){
	*optr=pixel;
	optr+=4;
        }
      }
    }
  }


// Load image from stream
FXbool fxloadRGB(FXStream& store,FXColor*& data,FXint& width,FXint& height){
  register FXint i,j,c,nchannels,dimension,tablen,magic,t,sub;
  FXfval base, start;
  FXuchar  storage;
  FXuchar  bpc;
  FXuchar  temp[4096];
  FXuint  *starttab;
  FXuint  *lengthtab;
  FXint    total;
  FXuchar *array;

  // Null out
  data=NULL;
  width=0;
  height=0;

  // Where the image format starts
  base=store.position();

  // Load header
  magic=read16(store);  // MAGIC (2)

  FXTRACE((50,"fxloadRGB: magic=%d\n",magic));

  // Check magic number
  if(magic!=474) return FALSE;

  // Load rest of header
  store >> storage;     // STORAGE (1)
  store >> bpc;         // BPC (1)

  FXTRACE((50,"fxloadRGB: bpc=%d storage=%d\n",bpc,storage));

  // Check the bpc; only grok 1 byte/channel!
  if(bpc!=1) return FALSE;

  dimension=read16(store);  // DIMENSION (2)
  width=read16(store);      // XSIZE (2)
  height=read16(store);     // YSIZE (2)
  nchannels=read16(store);  // ZSIZE (2)

  // Don't grok anything other than RGB!
  if(nchannels!=3) return FALSE;

  read32(store);            // PIXMIN (4)
  read32(store);            // PIXMAX (4)
  read32(store);            // DUMMY (4)
  store.load(temp,80);      // IMAGENAME (80)
  read32(store);            // COLORMAP (4)
  store.load(temp,404);     // DUMMY (404)

  FXTRACE((50,"fxloadRGB: width=%d height=%d nchannels=%d dimension=%d storage=%d bpc=%d\n",width,height,nchannels,dimension,storage,bpc));

  // Make room for image
  if(!FXMALLOC(&data,FXColor,width*height)) return FALSE;

  // RLE compressed
  if(storage){
    tablen=height*3;

    // Allocate line tables
    FXMALLOC(&starttab,FXuint,tablen*2);
    if(!starttab) return FALSE;
    lengthtab=&starttab[tablen];

    // Read line tables
    readtab(store,starttab,tablen);
    readtab(store,lengthtab,tablen);

    // Where the RLE chunks start
    start=store.position();

    // Substract this amount to get offset from chunk start
    sub=(FXint)(start-base);

    total=0;

    // Fix up the line table & figure space for RLE chunks
    // Intelligent RGB writers (not ours ;-)) may re-use RLE
    // chunks for more than 1 line...
    for(i=0; i<tablen; i++){
      starttab[i]-=sub;
      t=starttab[i]+lengthtab[i];
      if(t>total) total=t;
      }

    FXTRACE((1,"total=%d start=%d base=%d\n",total,start,base));

    // Make room for the compressed lines
    FXMALLOC(&array,FXuchar,total);
    if(!array){ FXFREE(&starttab); return FALSE; }

    // Load all RLE chunks
    store.load(array,total);
    for(c=0; c<3; c++){
      for(j=height-1; j>=0; j--){
        expandrow(((FXuchar*)(data+j*width))+c,&array[starttab[height-1-j+c*height]]);
        }
      }

    // Free RLE chunks
    FXFREE(&array);

    // Free line tables
    FXFREE(&starttab);
    }

  // NON compressed
  else{
    for(c=0; c<3; c++){
      for(j=height-1; j>=0; j--){
        store.load(temp,width);
        for(i=0; i<width; i++) ((FXuchar*)(data+j*width+i))[c]=temp[i];
        }
      }
    }

  // Set alpha
  for(i=0; i<width*height; i++) ((FXuchar*)(data+i))[3]=255;

  // Return TRUE if OK
  return TRUE;
  }


/*******************************************************************************/

// This always writes MSB
static void write16(FXStream& store,FXushort i){
  FXuchar c1,c2;
  c1=(i>>8)&0xff;
  c2=i&0xff;
  store << c1 << c2;
  }


// Write 32 bit thing MSB
static void write32(FXStream& store,FXuint i){
  FXuchar c1,c2,c3,c4;
  c1=(i>>24)&0xff;
  c2=(i>>16)&0xff;
  c3=(i>>8)&0xff;
  c4=i&0xff;
  store << c1 << c2 << c3 << c4;
  }


// Save a bmp file to a stream
FXbool fxsaveRGB(FXStream& store,const FXColor *data,FXint width,FXint height){
  register FXint i,j,c;
  FXuchar  storage=0;       // FIXME NON compressed, for now
  FXuchar  bpc=1;
  FXuchar  temp[4096];

  // Must make sense
  if(!data || width<=0 || height<=0) return FALSE;

  // Save header
  write16(store,474);       // MAGIC (2)
  store << storage;         // STORAGE (1)
  store << bpc;             // BPC (1)
  write16(store,3);         // DIMENSION (2)
  write16(store,width);     // XSIZE (2)
  write16(store,height);    // YSIZE (2)
  write16(store,3);         // ZSIZE (2)
  write32(store,0);         // PIXMIN (4)
  write32(store,255);       // PIXMAX (4)
  write32(store,0);         // DUMMY (4)
  memset(temp,0,80);        // Clean it
  memcpy(temp,"Name",4);    // Write name
  store.save(temp,80);      // IMAGENAME (80)
  write32(store,0);         // COLORMAP (4)
  memset(temp,0,404);       // Clean it
  store.save(temp,404);     // DUMMY (404)

  // Write pixels
  for(c=0; c<3; c++){
    for(j=height-1; j>=0; j--){
      for(i=0; i<width; i++) temp[i]=((FXuchar*)(data+j*width+i))[c];
      store.save(temp,width);
      }
    }
  return TRUE;
  }

}
