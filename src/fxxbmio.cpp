/********************************************************************************
*                                                                               *
*                          X B M   I n p u t / O u t p u t                      *
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
* $Id: fxxbmio.cpp,v 1.10 2004/04/08 16:24:48 fox Exp $                          *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "FXStream.h"
#include "fxpriv.h"



/*
  Notes:
  - Support for black and white images.
  - When writing image, dither output to bilevel.
  - Hot spot allows using these routines for cursor images.
  - Should we load (or save) image name?
*/




/*******************************************************************************/

namespace FX {


extern FXAPI FXbool fxloadXBM(FXColor*& data,const FXuchar *pix,const FXuchar *msk,FXint width,FXint height);
extern FXAPI FXbool fxloadXBM(FXStream& store,FXColor*& data,FXint& width,FXint& height,FXint& hotx,FXint& hoty);
extern FXAPI FXbool fxsaveXBM(FXStream& store,const FXColor *data,FXint width,FXint height,FXint hotx=-1,FXint hoty=-1);


// Little helper
static void readline(FXStream& store,FXchar* buffer,FXuint size){
  register FXuint i=0;
  while(!store.atEnd() && i<size){
    store >> buffer[i];
    if(buffer[i]=='\r') continue;
    if(buffer[i]=='\n') break;
    i++;
    }
  buffer[i]=0;
  }


// Load alpha XBM image from pixels and mask
FXbool fxloadXBM(FXColor*& data,const FXuchar *pixels,const FXuchar *mask,FXint width,FXint height){
  register FXint x,y,byt,bit,row;
  data=NULL;
  if(pixels && mask && 0<width && 0<height){
    if(FXCALLOC(&data,FXColor,width*height)){
      row=(width+7)>>3;
      for(y=0; y<height; y++){
        for(x=0; x<width; x++){
          byt=y*row+(x>>3);
          bit=1<<(x&7);
          if(mask[byt]&bit){
            data[y*width+x]|=FXRGBA(0,0,0,255);                                 // Opaque
            if(!(pixels[byt]&bit)) data[y*width+x]|=FXRGBA(255,255,255,0);      // White
            }
          }
        }
      return TRUE;
      }
    }
  return FALSE;
  }


// Load image from stream
FXbool fxloadXBM(FXStream& store,FXColor*& data,FXint& width,FXint& height,FXint& hotx,FXint& hoty){
  const FXColor colormap[2]={FXRGB(255,255,255),FXRGB(0,0,0)};
  FXchar buffer[1024],name[255],ch;
  FXint  value,i,j;
  FXColor *pp;

  // Null out
  data=NULL;
  width=0;
  height=0;
  hotx=-1;
  hoty=-1;

  // Read header
  while(!store.atEnd()){

    // Read buffer
    readline(store,buffer,sizeof(buffer));
    while(strstr(buffer,"/*") && !store.atEnd()){
      readline(store,buffer,sizeof(buffer));
      }

    // Recognize #define
    if(sscanf(buffer,"#define %s %d",name,&value)==2){
      if(strstr(name,"width")) width=value;
      else if(strstr(name,"height")) height=value;
      else if(strstr(name,"x_hot")) hotx=value;
      else if(strstr(name,"y_hot")) hoty=value;
      continue;
      }

    // Recognize declaration
    if(sscanf(buffer,"static unsigned char %s = {",name)==1) break;
    if(sscanf(buffer,"static char %s = {", name)==1) break;
    }

  // Test sensible width, height
  if(width<=0 || height<=0) return FALSE;

  // Allocate image to return
  if(!FXCALLOC(&data,FXColor,width*height)){
    return FALSE;
    }

  // Load image; note we will read a whole number of bytes
  // for each line, the last byte may not be completely used
  for(j=0,pp=data; j<height; j++){
    for(i=0; i<width; i++){
      if((i&7)==0){
        value=0;
        while(!store.atEnd()){
          store >> ch;
          if(ch!='0') continue;
          store >> ch;
          if(ch!='x' && ch!='X') continue;
          break;
          }
        while(!store.atEnd()){
          store >> ch;
          if(!isxdigit((FXuchar)ch)) break;
          value=value*16+(('a'<=ch) ? ch-'a'+10 : ('A'<=ch) ? ch-'A'+10 : ch-'0');
          }
        }
      *pp++=colormap[value&1];
      value>>=1;
      }
    }

  // We got the image, but we're not done yet; need to read few more bytes
  // the number of bytes read here must match the number of bytes written
  // by fxsaveXBM() so that the stream won't get out of sync
  while(!store.atEnd()){
    store >> ch;
    if(ch=='\n') break;
    }

  return TRUE;
  }


/*******************************************************************************/



// Save image to a stream
FXbool fxsaveXBM(FXStream& store,const FXColor *data,FXint width,FXint height,FXint hotx,FXint hoty){
  static const FXint dither[4][4]={{0,32768, 8192,40960},{49152,16384,57344,24576},{12288,45056,4096,36864},{61440,28672,53248,20480}};
  register const FXuchar *ptr=(const FXuchar*)data;
  register FXint bit,code,count,x,y,n;
  const char name[]="image";
  FXchar buffer[128];

  // Write width
  n=sprintf(buffer,"#define %s_width %d\n",name,width);
  store.save(buffer,n);

  // Write height
  n=sprintf(buffer,"#define %s_height %d\n",name,height);
  store.save(buffer,n);

  // Write hot spot
  if(0<=hotx && 0<=hoty){
    n=sprintf(buffer,"#define %s_x_hot %d\n",name,hotx);
    store.save(buffer,n);
    n=sprintf(buffer,"#define %s_y_hot %d\n",name,hoty);
    store.save(buffer,n);
    }

  // Write declaration
  n=sprintf(buffer,"static char %s_bits[] = {",name);
  store.save(buffer,n);

  // Write pixels
  for(y=count=0; y<height; y++){
    for(x=code=0,bit=1; x<width; x++){
      if((ptr[0]*77+ptr[1]*151+ptr[2]*29)<dither[y&3][x&3]) code|=bit;
      bit<<=1;
      if(bit==256 || x==width-1){
        if(count){
          store.save(",",1);
          }
        if(count%12==0){
          store.save("\n   ",3);
          }
        n=sprintf(buffer," 0x%02x",code);
        store.save(buffer,n);
        bit=1;
        code=0;
        count++;
        }
      ptr+=4;
      }
    }

  // Wrap it up
  store.save("};\n",3);

  return TRUE;
  }

}

