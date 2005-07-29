/********************************************************************************
*                                                                               *
*       P e r s i s t e n t   S t o r a g e   S t r e a m   C l a s s e s       *
*                                                                               *
*********************************************************************************
* Copyright (C) 1997,2005 by Jeroen van der Zijp.   All Rights Reserved.        *
* TnFOX Extensions (C) 2003, 2004 Niall Douglas                                 *
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
* $Id: FXStream.cpp,v 1.53 2004/11/08 15:35:10 fox Exp $                        *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXHash.h"
#include "FXObject.h"
#include "QIODevice.h"
#include "FXException.h"
#include "QTrans.h"
#include "FXFile.h"
#include "QBuffer.h"
#include <qcstring.h>

#ifdef _MSC_VER
#include <malloc.h>
#define alloca _alloca
#endif


#define MAXCLASSNAME       256          // Maximum class name length
#define DEF_HASH_SIZE      32           // Initial table size (MUST be power of 2)
#define MAX_LOAD           80           // Maximum hash table load factor (%)
#define FUDGE              5            // Fudge for hash table size
#define UNUSEDSLOT         0xffffffff   // Unsused slot marker
#define CLASSIDFLAG        0x80000000   // Marks it as a class ID

#define HASH1(x,n) (((FXuint)(FXuval)(x)*13)%(n))         // Number [0..n-1]
#define HASH2(x,n) (1|(((FXuint)(FXuval)(x)*17)%((n)-1))) // Number [1..n-2]




/*******************************************************************************/

namespace FX {


// Create PersistentStore object
FXStream::FXStream(QIODevice *_dev, const FXObject *cont){
  parent=cont;
  begptr=NULL;
  endptr=NULL;
  wrptr=NULL;
  rdptr=NULL;
  pos=0;
  dir=FXStreamDead;
  code=FXStreamOK;
  seq=0x80000000;
  swap=FALSE;
  owns=FALSE;

  // TnFOX stuff
  hash=0;
  dev=_dev;
  }

// Create PersistentStore object
FXStream::FXStream(const FXObject *cont){
  parent=cont;
  begptr=NULL;
  endptr=NULL;
  wrptr=NULL;
  rdptr=NULL;
  pos=0L;
  dir=FXStreamDead;
  code=FXStreamOK;
  seq=0x80000000;
  swap=FALSE;
  owns=FALSE;

  // TnFOX stuff
  hash=0;
  dev=0;
  }

// Destroy PersistentStore object
FXStream::~FXStream(){
  if(owns){FXFREE(&begptr);}
  parent=(FXObject*)-1L;
  begptr=(FXuchar*)-1L;
  endptr=(FXuchar*)-1L;
  wrptr=(FXuchar*)-1L;
  rdptr=(FXuchar*)-1L;

  // TnFOX stuff
  FXDELETE(hash);
  dev=(QIODevice *)-1L;
  }


void FXStream::setError(FXStreamStatus err)
{
	code=err;
}

unsigned long FXStream::getSpace() const
{
	return 102400;	// dummy value to keep code happy
}


void FXStream::setSpace(unsigned long)
{
	// Do nothing
}


// Open for save or load
FXbool FXStream::open(FXStreamDirection save_or_load,unsigned long size,FXuchar* data){
  if(save_or_load!=FXStreamSave && save_or_load!=FXStreamLoad){fxerror("FXStream::open: illegal stream direction.\n");}
  if(!dir){

    // Use external buffer space
    if(data){
      begptr=data;
      if(size==ULONG_MAX)
        endptr=((FXuchar*)NULL)-1;
      else
        endptr=begptr+size;
      wrptr=begptr;
      rdptr=begptr;
      owns=FALSE;
      }

    // Use internal buffer space
    else{
      if(!FXCALLOC(&begptr,FXuchar,size)){ code=FXStreamAlloc; return FALSE; }
      endptr=begptr+size;
      wrptr=begptr;
      rdptr=begptr;
      owns=TRUE;
      }

    // Reset variables
	FXERRHM(hash=new FXHash);
    hash->clear();
    dir=save_or_load;
    seq=0x80000000;
    pos=0;

    // Append container object to hash table
    if(parent){
      addObject(parent);
      }

    // So far, so good
    code=FXStreamOK;

    return TRUE;
    }
  return FALSE;
  }


// Close store; return TRUE if no errors have been encountered
FXbool FXStream::close(){
  if(dir){
    hash->clear();
	FXDELETE(hash);
    dir=FXStreamDead;
    if(owns){FXFREE(&begptr);}
    begptr=NULL;
    wrptr=NULL;
    rdptr=NULL;
    endptr=NULL;
    owns=FALSE;
    return code==FXStreamOK;
    }
  return FALSE;
  }


// Flush buffer
FXbool FXStream::flush(){
  // Does nothing as underlying i/o devices do the buffering
  return code==FXStreamOK;
  }


FXulong FXStream::position() const
{
	return dev->at();
}

// Move to position
FXbool FXStream::position(FXfval newpos,FXWhence whence)
{
	if(FXFromCurrent==whence) newpos+=dev->at();
	else if(FXFromEnd==whence) newpos=dev->size()-newpos;
	return dev->at(newpos);
}

void FXStream::setDevice(QIODevice *_dev)
{
	dev=_dev;
}

bool FXStream::atEnd() const
{
	return dev->atEnd();
}

FXStream &FXStream::readBytes(char *&s, FXuint &l)
{
	*this >> l;
	FXERRHM(s=new char[l]);
	return readRawBytes(s, l);
}

FXStream &FXStream::readRawBytes(char *buffer, FXuval len)
{
	if(len!=dev->readBlock(buffer, len)) FXERRGIO(QTrans::tr("FXStream", "Premature EOF encountered"));
	return *this;
}

FXStream &FXStream::writeBytes(const char *s, FXuint l)
{
	return writeRawBytes(s, l);
}

FXStream &FXStream::writeRawBytes(const char *buffer, FXuval len)
{
	dev->writeBlock(buffer, len);
	return *this;
}

FXfval FXStream::rewind(FXint amount)
{
	FXfval c=dev->at();
	if(amount>0 && c<(FXfval) amount) c=0; else c-=amount;
	dev->at(c);
	return c;
}


/******************************  Save Basic Types  *****************************/

FXStream& FXStream::operator<<(const FXuchar &v)
{
  dev->putch(v);
  return *this;
}

FXStream& FXStream::operator<<(const FXushort &_v)
{
	FXushort v=_v;
	if(swap){fxendianswap2(&v);}
	dev->writeBlock((char *) &v,2);
	return *this;
}

FXStream& FXStream::operator<<(const FXuint &_v)
{
	FXuint v=_v;
	if(swap){fxendianswap4(&v);}
	dev->writeBlock((char *) &v,4);
	return *this;
}

FXStream& FXStream::operator<<(const FXfloat &_v)
{
	FXfloat v=_v;
	if(swap){fxendianswap4(&v);}
	dev->writeBlock((char *) &v,4);
	return *this;
}

FXStream& FXStream::operator<<(const FXdouble &_v)
{
	FXdouble v=_v;
	if(swap){fxendianswap8(&v);}
	dev->writeBlock((char *) &v,8);
	return *this;
}

FXStream& FXStream::operator<<(const FXulong &_v)
{
	FXulong v=_v;
	if(swap){fxendianswap8(&v);}
	dev->writeBlock((char *) &v,8);
	return *this;
}

FXStream& FXStream::operator<<(const char *str)
{
	dev->writeBlock(str,strlen(str));
	return *this;
}

FXStream& FXStream::operator<<(const bool &_v)
{
	dev->putch(_v);
	return *this;
}


/************************  Save Blocks of Basic Types  *************************/

FXStream& FXStream::save(const FXuchar* p,unsigned long n){
  FXASSERT(n==0 || (n>0 && p!=NULL));
  dev->writeBlock((char *) p,n);
  return *this;
  }

FXStream& FXStream::save(const FXushort* p,unsigned long n){
  FXASSERT(n==0 || (n>0 && p!=NULL));
  if(swap && n)
  {
	  FXushort *_p=(FXushort *) memcpy(alloca(n*sizeof(FXushort)), p, n*sizeof(FXushort));
	  for(unsigned long i=0; i<n; i++) fxendianswap2(&_p[i]);
	  p=_p;
  }
  dev->writeBlock((char *) p,n<<1);
  return *this;
  }

FXStream& FXStream::save(const FXuint* p,unsigned long n){
  FXASSERT(n==0 || (n>0 && p!=NULL));
  if(swap && n)
  {
	  FXuint *_p=(FXuint *) memcpy(alloca(n*sizeof(FXuint)), p, n*sizeof(FXuint));
	  for(unsigned long i=0; i<n; i++) fxendianswap4(&_p[i]);
	  p=_p;
  }
  dev->writeBlock((char *) p,n<<2);
  return *this;
  }

FXStream& FXStream::save(const FXfloat* p,unsigned long n){
  FXASSERT(n==0 || (n>0 && p!=NULL));
  if(swap && n)
  {
	  FXfloat *_p=(FXfloat *) memcpy(alloca(n*sizeof(FXfloat)), p, n*sizeof(FXfloat));
	  for(unsigned long i=0; i<n; i++) fxendianswap4(&_p[i]);
	  p=_p;
  }
  dev->writeBlock((char *) p,n<<2);
  return *this;
  }

FXStream& FXStream::save(const FXdouble* p,unsigned long n){
  FXASSERT(n==0 || (n>0 && p!=NULL));
  if(swap && n)
  {
	  FXdouble *_p=(FXdouble *) memcpy(alloca(n*sizeof(FXdouble)), p, n*sizeof(FXdouble));
	  for(unsigned long i=0; i<n; i++) fxendianswap8(&_p[i]);
	  p=_p;
  }
  dev->writeBlock((char *) p,n<<3);
  return *this;
  }

FXStream& FXStream::save(const FXulong* p,unsigned long n){
  FXASSERT(n==0 || (n>0 && p!=NULL));
  if(swap && n)
  {
	  FXulong *_p=(FXulong *) memcpy(alloca(n*sizeof(FXulong)), p, n*sizeof(FXulong));
	  for(unsigned long i=0; i<n; i++) fxendianswap8(&_p[i]);
	  p=_p;
  }
  dev->writeBlock((char *) p,n<<3);
  return *this;
  }


/*****************************  Load Basic Types  ******************************/

FXStream& FXStream::operator>>(FXuchar& v){
  int _v=dev->getch();
  v=(FXuchar) _v;
  if(-1==_v) FXERRGIO(QTrans::tr("FXStream", "Premature EOF encountered"));
  return *this;
  }

FXStream& FXStream::operator>>(FXushort& v){
  if(2!=dev->readBlock((char *) &v,2)) FXERRGIO(QTrans::tr("FXStream", "Premature EOF encountered"));
  if(swap){fxendianswap2(&v);}
  return *this;
  }

FXStream& FXStream::operator>>(FXuint& v){
  if(4!=dev->readBlock((char *) &v,4)) FXERRGIO(QTrans::tr("FXStream", "Premature EOF encountered"));
  if(swap){fxendianswap4(&v);}
  return *this;
  }

FXStream& FXStream::operator>>(FXfloat& v){
  if(4!=dev->readBlock((char *) &v,4)) FXERRGIO(QTrans::tr("FXStream", "Premature EOF encountered"));
  if(swap){fxendianswap4(&v);}
  return *this;
  }

FXStream& FXStream::operator>>(FXdouble& v){
  if(8!=dev->readBlock((char *) &v,8)) FXERRGIO(QTrans::tr("FXStream", "Premature EOF encountered"));
  if(swap){fxendianswap8(&v);}
  return *this;
  }

FXStream& FXStream::operator>>(FXulong& v){
  if(8!=dev->readBlock((char *) &v,8)) FXERRGIO(QTrans::tr("FXStream", "Premature EOF encountered"));
  if(swap){fxendianswap8(&v);}
  return *this;
  }

FXStream& FXStream::operator>>(bool& v){
  int _v=dev->getch();
  v=(_v!=0);
  if(-1==_v) FXERRGIO(QTrans::tr("FXStream", "Premature EOF encountered"));
  return *this;
  }


/************************  Load Blocks of Basic Types  *************************/

FXStream& FXStream::load(FXuchar* p,unsigned long n){
  FXASSERT(n==0 || (n>0 && p!=NULL));
  if(n!=dev->readBlock((char *) p,n)) FXERRGIO(QTrans::tr("FXStream", "Premature EOF encountered"));
  return *this;
  }

FXStream& FXStream::load(FXushort* p,unsigned long n){
  FXASSERT(n==0 || (n>0 && p!=NULL));
  if(n<<1!=dev->readBlock((char *) p,n<<1)) FXERRGIO(QTrans::tr("FXStream", "Premature EOF encountered"));
  if(swap&&n){do{fxendianswap2(p++);}while(--n);}
  return *this;
  }

FXStream& FXStream::load(FXuint* p,unsigned long n){
  FXASSERT(n==0 || (n>0 && p!=NULL));
  if(n<<2!=dev->readBlock((char *) p,n<<2)) FXERRGIO(QTrans::tr("FXStream", "Premature EOF encountered"));
  if(swap&&n){do{fxendianswap4(p++);}while(--n);}
  return *this;
  }

FXStream& FXStream::load(FXfloat* p,unsigned long n){
  FXASSERT(n==0 || (n>0 && p!=NULL));
  if(n<<2!=dev->readBlock((char *) p,n<<2)) FXERRGIO(QTrans::tr("FXStream", "Premature EOF encountered"));
  if(swap&&n){do{fxendianswap4(p++);}while(--n);}
  return *this;
  }

FXStream& FXStream::load(FXdouble* p,unsigned long n){
  FXASSERT(n==0 || (n>0 && p!=NULL));
  if(n<<3!=dev->readBlock((char *) p,n<<3)) FXERRGIO(QTrans::tr("FXStream", "Premature EOF encountered"));
  if(swap&&n){do{fxendianswap8(p++);}while(--n);}
  return *this;
  }

FXStream& FXStream::load(FXulong* p,unsigned long n){
  FXASSERT(n==0 || (n>0 && p!=NULL));
  if(n<<3!=dev->readBlock((char *) p,n<<3)) FXERRGIO(QTrans::tr("FXStream", "Premature EOF encountered"));
  if(swap&&n){do{fxendianswap8(p++);}while(--n);}
  return *this;
  }


/*********************************  Add Object  ********************************/


// Add object without saving or loading
FXStream& FXStream::addObject(const FXObject* v){
  if(dir==FXStreamSave){
    hash->insert((void*)v,(void*)(FXuval)seq++);
    }
  else if(dir==FXStreamLoad){
    hash->insert((void*)(FXuval)seq++,(void*)v);
    }
  return *this;
  }


/********************************  Save Object  ********************************/

// Save object
FXStream& FXStream::saveObject(const FXObject* v){
  register const FXMetaClass *cls;
  register const FXchar *name;
  FXuint tag,zero=0;
  if(dir!=FXStreamSave){ fxerror("FXStream::saveObject: wrong stream direction.\n"); }
  if(code==FXStreamOK){
    if(v==NULL){                                // Its a NULL
      *this << zero;
      return *this;
      }
    tag=(FXuint)(FXuval)hash->find((void*)v);    // Already in table
    if(tag){
      *this << tag;
      return *this;
      }
    hash->insert((void*)v,(void*)(FXuval)seq++); // Add to table
    cls=v->getMetaClass();
    name=cls->getClassName();
    tag=strlen(name)+1;
    if(tag>MAXCLASSNAME){                       // Class name too long
      code=FXStreamFormat;
      return *this;
      }
    *this << tag;                               // Save tag
    *this << zero;
    save(name,tag);
    FXTRACE((100,"saveObject(%s)\n",v->getClassName()));
    v->save(*this);
    }
  return *this;
  }


/*******************************  Load Object  *********************************/

// Load object
FXStream& FXStream::loadObject(FXObject*& v){
  register const FXMetaClass *cls;
  FXchar name[MAXCLASSNAME+1];
  FXuint tag,esc;
  if(dir!=FXStreamLoad){ fxerror("FXStream::loadObject: wrong stream direction.\n"); }
  if(code==FXStreamOK){
    *this >> tag;
    if(tag==0){                                 // Was a NULL
      v=NULL;
      return *this;
      }
    if(tag>=0x80000000){
      v=(FXObject*)hash->find((void*)(FXuval)tag);
      if(!v){
        code=FXStreamFormat;                    // Bad format in stream
        }
      return *this;
      }
    if(tag>MAXCLASSNAME){                       // Class name too long
      code=FXStreamFormat;                      // Bad format in stream
      return *this;
      }
    *this >> esc;                               // Read escape code
    if(esc!=0){                                 // Escape code is wrong
      code=FXStreamFormat;                      // Bad format in stream
      return *this;
      }
    load(name,tag);                             // Load name
    cls=FXMetaClass::getMetaClassFromName(name);
    if(cls==NULL){                              // No FXMetaClass with this class name
      code=FXStreamUnknown;                     // Unknown class
      return *this;
      }
    v=cls->makeInstance();                      // Build some object!!
    hash->insert((void*)(FXuval)seq++,(void*)v); // Add to table
    FXTRACE((100,"loadObject(%s)\n",v->getClassName()));
    v->load(*this);
    }
  return *this;
  }

}
