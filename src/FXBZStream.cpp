/********************************************************************************
*                                                                               *
*                         B Z S t r e a m   C l a s s e s                       *
*                                                                               *
*********************************************************************************
* Copyright (C) 1999,2004 by Lyle Johnson. All Rights Reserved.                 *
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
* $Id: FXBZStream.cpp,v 1.8 2004/09/19 20:06:05 fox Exp $                       *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "FXHash.h"
#include "FXThread.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXObject.h"
#include "FXBZStream.h"

#ifdef HAVE_BZ2LIB_H
#include <bzlib.h>

/*
  Notes:
  - Very basic compressed file I/O only.
  - Updated for new stream classes 2003/07/08.
*/


#define BLOCKSIZE100K 1         // Block size x 100,000 bytes
#define VERBOSITY     0         // For tracing in bzip library
#define WORKFACTOR    0         // See bzip2 documentation


/*******************************************************************************/


namespace FX {


// Initialize file stream
FXBZFileStream::FXBZFileStream(const FXObject* cont):FXStream(cont){
  file=NULL;
  bzfile=NULL;
  }


// Save to a file
unsigned long FXBZFileStream::writeBuffer(unsigned long){
  register long m; int bzerror;
  if(dir!=FXStreamSave){fxerror("FXBZFileStream::writeBuffer: wrong stream direction.\n");}
  FXASSERT(begptr<=rdptr);
  FXASSERT(rdptr<=wrptr);
  FXASSERT(wrptr<=endptr);
  if(code==FXStreamOK){
    m=wrptr-rdptr;
    BZ2_bzWrite(&bzerror,(BZFILE*)bzfile,(void*)rdptr,m);
    if(bzerror!=BZ_OK){
      code=FXStreamFull;
      return endptr-wrptr;
      }
    rdptr=begptr;
    wrptr=begptr;
    return endptr-wrptr;
    }
  return 0;
  }


// Load from file
unsigned long FXBZFileStream::readBuffer(unsigned long){
  register long m,n; int bzerror;
  if(dir!=FXStreamLoad){fxerror("FXFileStream::readBuffer: wrong stream direction.\n");}
  FXASSERT(begptr<=rdptr);
  FXASSERT(rdptr<=wrptr);
  FXASSERT(wrptr<=endptr);
  if(code==FXStreamOK){
    m=wrptr-rdptr;
    if(m){memmove(begptr,rdptr,m);}
    rdptr=begptr;
    wrptr=begptr+m;
    n=BZ2_bzRead(&bzerror,(BZFILE*)bzfile,wrptr,endptr-wrptr);
    if(bzerror!=BZ_OK){
      if(bzerror!=BZ_STREAM_END){
        code=FXStreamFormat;
        return wrptr-rdptr;
        }
      code=FXStreamEnd;
      }
    wrptr+=n;
    return wrptr-rdptr;
    }
  return 0;
  }


// Try open file stream
FXbool FXBZFileStream::open(const FXString& filename,FXStreamDirection save_or_load,unsigned long size){
  int bzerror;
  if(save_or_load!=FXStreamSave && save_or_load!=FXStreamLoad){fxerror("FXFileStream::open: illegal stream direction.\n");}
  if(!dir){
    if(save_or_load==FXStreamLoad){
      file=fopen(filename.text(),"rb");
      if(file==NULL){
        code=FXStreamNoRead;
        return FALSE;
        }
      bzfile=BZ2_bzReadOpen(&bzerror,(FILE*)file,0,VERBOSITY,NULL,0);
      if(bzerror!=BZ_OK){
        BZ2_bzReadClose(&bzerror,(BZFILE*)bzfile);
        fclose((FILE*)file);
        code=FXStreamNoRead;
        return FALSE;
        }
      }
    else if(save_or_load==FXStreamSave){
      file=fopen(filename.text(),"wb");
      if(file==NULL){
        code=FXStreamNoWrite;
        return FALSE;
        }
      bzfile=BZ2_bzWriteOpen(&bzerror,(FILE*)file,BLOCKSIZE100K,VERBOSITY,WORKFACTOR);
      if(bzerror!=BZ_OK){
        BZ2_bzWriteClose(&bzerror,(BZFILE*)bzfile,0,NULL,NULL);
        fclose((FILE*)file);
        code=FXStreamNoWrite;
        return FALSE;
        }
      }
    return FXStream::open(save_or_load,size);
    }
  return FALSE;
  }


// Close file stream
FXbool FXBZFileStream::close(){
  int bzerror;
  if(dir){
    if(dir==FXStreamLoad){
      BZ2_bzReadClose(&bzerror,(BZFILE*)bzfile);
      }
    else{
      flush();
      BZ2_bzWriteClose(&bzerror,(BZFILE*)bzfile,0,NULL,NULL);
      }
    fclose((FILE*)file);
    return FXStream::close();
    }
  return FALSE;
  }


// Destructor
FXBZFileStream::~FXBZFileStream(){
  close();
  }

}

#endif
