/********************************************************************************
*                                                                               *
*       P e r s i s t e n t   S t o r a g e   S t r e a m   C l a s s e s       *
*                                                                               *
*********************************************************************************
* Copyright (C) 1997,2004 by Jeroen van der Zijp.   All Rights Reserved.        *
* TnFOX Extensions (C) 2003 Niall Douglas                                       *
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
* $Id: FXStream.cpp,v 1.47 2004/04/05 14:49:33 fox Exp $                        *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXObject.h"
#include "FXIODevice.h"
#include "FXException.h"
#include "FXTrans.h"
#include "FXFile.h"
#include "FXBuffer.h"
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
FXStream::FXStream(FXIODevice *_dev, const FXObject *cont){
  parent=cont;
  table=NULL;
  ntable=0;
  no=0;
  dir=FXStreamDead;
  code=FXStreamOK;
  dev=_dev;
  swap=FOX_BIGENDIAN;
  }


// Destroy PersistentStore object
FXStream::~FXStream(){
  FXFREE(&table);
  parent=(const FXObject*)-1L;
  table=(FXStreamHashEntry*)-1L;
  }


// Grow hash table
void FXStream::grow(){
  register FXuint i,n,p,x;
  FXStreamHashEntry *h;

  // Double table size
  FXASSERT(ntable);
  n=ntable<<1;

  // Allocate new table
  if(!FXMALLOC(&h,FXStreamHashEntry,n)){ code=FXStreamAlloc; return; }

  // Rehash table when FXStreamSave
  if(dir==FXStreamSave){
    for(i=0; i<n; i++) h[i].ref=UNUSEDSLOT;
    for(i=0; i<ntable; i++){
      if(table[i].ref==UNUSEDSLOT) continue;
      p = HASH1(table[i].obj,n);
      FXASSERT(p<n);
      x = HASH2(table[i].obj,n);
      FXASSERT(1<=x && x<n);
      while(h[p].ref!=UNUSEDSLOT) p=(p+x)%n;
      h[p].ref=table[i].ref;
      h[p].obj=table[i].obj;
      }
    }

  // Simply copy over when FXStreamLoad
  else if(dir==FXStreamLoad){
    for(i=0; i<ntable; i++){
      h[i].ref=table[i].ref;
      h[i].obj=table[i].obj;
      }
    }

  // Ditch old table
  FXFREE(&table);

  // Point to new table
  table=h;
  ntable=n;
  }


// Open for save or load
FXbool FXStream::open(FXStreamDirection save_or_load,unsigned long size){
  register unsigned int i,p;
  if(save_or_load!=FXStreamSave && save_or_load!=FXStreamLoad){fxerror("FXStream::open: illegal stream direction.\n");}
  if(!dir){
    if(size<16){ fxerror("FXStream::open: size argument less than 16\n"); }

    // Allocate hash table
    if(!FXMALLOC(&table,FXStreamHashEntry,DEF_HASH_SIZE)){ code=FXStreamAlloc; return FALSE; }

    // Initialize table to empty
    for(i=0; i<DEF_HASH_SIZE; i++){ table[i].ref=UNUSEDSLOT; }

    // Set variables
    ntable=DEF_HASH_SIZE;
    dir=save_or_load;
    no=0;

    // Append container object to hash table
    if(parent){
      if(dir==FXStreamSave){
        p=HASH1(parent,ntable);
        FXASSERT(p<ntable);
        table[p].obj=(FXObject*)parent;
        table[p].ref=no;
        }
      else{
        table[no].obj=(FXObject*)parent;
        table[no].ref=no;
        }
      no++;
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
    dir=FXStreamDead;
    FXFREE(&table);
    ntable=0;
    no=0;
    return code==FXStreamOK;
    }
  return FALSE;
  }

unsigned long FXStream::getSpace() const
{
	return 102400;
}

void FXStream::setSpace(unsigned long)
{
	// Do nothing
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

void FXStream::setDevice(FXIODevice *_dev)
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
	if(len!=dev->readBlock(buffer, len)) FXERRGIO(FXTrans::tr("FXStream", "Premature EOF encountered"));
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

// Swap duplets
static inline void swap2(void *p){
  register FXuchar t;
  t=((FXuchar*)p)[0]; ((FXuchar*)p)[0]=((FXuchar*)p)[1]; ((FXuchar*)p)[1]=t;
  }


// Swap quadruplets
static inline void swap4(void *p){
  register FXuchar t;
  t=((FXuchar*)p)[0]; ((FXuchar*)p)[0]=((FXuchar*)p)[3]; ((FXuchar*)p)[3]=t;
  t=((FXuchar*)p)[1]; ((FXuchar*)p)[1]=((FXuchar*)p)[2]; ((FXuchar*)p)[2]=t;
  }


// Swap octuplets
static inline void swap8(void *p){
  register FXuchar t;
  t=((FXuchar*)p)[0]; ((FXuchar*)p)[0]=((FXuchar*)p)[7]; ((FXuchar*)p)[7]=t;
  t=((FXuchar*)p)[1]; ((FXuchar*)p)[1]=((FXuchar*)p)[6]; ((FXuchar*)p)[6]=t;
  t=((FXuchar*)p)[2]; ((FXuchar*)p)[2]=((FXuchar*)p)[5]; ((FXuchar*)p)[5]=t;
  t=((FXuchar*)p)[3]; ((FXuchar*)p)[3]=((FXuchar*)p)[4]; ((FXuchar*)p)[4]=t;
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
	if(swap){swap2(&v);}
	dev->writeBlock((char *) &v,2);
	return *this;
}

FXStream& FXStream::operator<<(const FXuint &_v)
{
	FXuint v=_v;
	if(swap){swap4(&v);}
	dev->writeBlock((char *) &v,4);
	return *this;
}

FXStream& FXStream::operator<<(const FXfloat &_v)
{
	FXfloat v=_v;
	if(swap){swap4(&v);}
	dev->writeBlock((char *) &v,4);
	return *this;
}

FXStream& FXStream::operator<<(const FXdouble &_v)
{
	FXdouble v=_v;
	if(swap){swap8(&v);}
	dev->writeBlock((char *) &v,8);
	return *this;
}

#ifdef FX_LONG
FXStream& FXStream::operator<<(const FXulong &_v)
{
	FXulong v=_v;
	if(swap){swap8(&v);}
	dev->writeBlock((char *) &v,8);
	return *this;
}
#endif

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
	  for(unsigned long i=0; i<n; i++) swap2(&_p[i]);
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
	  for(unsigned long i=0; i<n; i++) swap4(&_p[i]);
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
	  for(unsigned long i=0; i<n; i++) swap4(&_p[i]);
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
	  for(unsigned long i=0; i<n; i++) swap8(&_p[i]);
	  p=_p;
  }
  dev->writeBlock((char *) p,n<<3);
  return *this;
  }

#ifdef FX_LONG
FXStream& FXStream::save(const FXulong* p,unsigned long n){
  FXASSERT(n==0 || (n>0 && p!=NULL));
  if(swap && n)
  {
	  FXulong *_p=(FXulong *) memcpy(alloca(n*sizeof(FXulong)), p, n*sizeof(FXulong));
	  for(unsigned long i=0; i<n; i++) swap8(&_p[i]);
	  p=_p;
  }
  dev->writeBlock((char *) p,n<<3);
  return *this;
  }
#endif


/*****************************  Load Basic Types  ******************************/

FXStream& FXStream::operator>>(FXuchar& v){
  int _v=dev->getch();
  v=(FXuchar) _v;
  if(-1==_v) FXERRGIO(FXTrans::tr("FXStream", "Premature EOF encountered"));
  return *this;
  }

FXStream& FXStream::operator>>(FXushort& v){
  if(2!=dev->readBlock((char *) &v,2)) FXERRGIO(FXTrans::tr("FXStream", "Premature EOF encountered"));
  if(swap){swap2(&v);}
  return *this;
  }

FXStream& FXStream::operator>>(FXuint& v){
  if(4!=dev->readBlock((char *) &v,4)) FXERRGIO(FXTrans::tr("FXStream", "Premature EOF encountered"));
  if(swap){swap4(&v);}
  return *this;
  }

FXStream& FXStream::operator>>(FXfloat& v){
  if(4!=dev->readBlock((char *) &v,4)) FXERRGIO(FXTrans::tr("FXStream", "Premature EOF encountered"));
  if(swap){swap4(&v);}
  return *this;
  }

FXStream& FXStream::operator>>(FXdouble& v){
  if(8!=dev->readBlock((char *) &v,8)) FXERRGIO(FXTrans::tr("FXStream", "Premature EOF encountered"));
  if(swap){swap8(&v);}
  return *this;
  }

#ifdef FX_LONG
FXStream& FXStream::operator>>(FXulong& v){
  if(8!=dev->readBlock((char *) &v,8)) FXERRGIO(FXTrans::tr("FXStream", "Premature EOF encountered"));
  if(swap){swap8(&v);}
  return *this;
  }
#endif

FXStream& FXStream::operator>>(bool& v){
  int _v=dev->getch();
  v=(_v!=0);
  if(-1==_v) FXERRGIO(FXTrans::tr("FXStream", "Premature EOF encountered"));
  return *this;
  }


/************************  Load Blocks of Basic Types  *************************/

FXStream& FXStream::load(FXuchar* p,unsigned long n){
  FXASSERT(n==0 || (n>0 && p!=NULL));
  if(n!=dev->readBlock((char *) p,n)) FXERRGIO(FXTrans::tr("FXStream", "Premature EOF encountered"));
  return *this;
  }

FXStream& FXStream::load(FXushort* p,unsigned long n){
  FXASSERT(n==0 || (n>0 && p!=NULL));
  if(n<<1!=dev->readBlock((char *) p,n<<1)) FXERRGIO(FXTrans::tr("FXStream", "Premature EOF encountered"));
  if(swap&&n){do{swap2(p++);}while(--n);}
  return *this;
  }

FXStream& FXStream::load(FXuint* p,unsigned long n){
  FXASSERT(n==0 || (n>0 && p!=NULL));
  if(n<<2!=dev->readBlock((char *) p,n<<2)) FXERRGIO(FXTrans::tr("FXStream", "Premature EOF encountered"));
  if(swap&&n){do{swap4(p++);}while(--n);}
  return *this;
  }

FXStream& FXStream::load(FXfloat* p,unsigned long n){
  FXASSERT(n==0 || (n>0 && p!=NULL));
  if(n<<2!=dev->readBlock((char *) p,n<<2)) FXERRGIO(FXTrans::tr("FXStream", "Premature EOF encountered"));
  if(swap&&n){do{swap4(p++);}while(--n);}
  return *this;
  }

FXStream& FXStream::load(FXdouble* p,unsigned long n){
  FXASSERT(n==0 || (n>0 && p!=NULL));
  if(n<<3!=dev->readBlock((char *) p,n<<3)) FXERRGIO(FXTrans::tr("FXStream", "Premature EOF encountered"));
  if(swap&&n){do{swap8(p++);}while(--n);}
  return *this;
  }

#ifdef FX_LONG
FXStream& FXStream::load(FXulong* p,unsigned long n){
  FXASSERT(n==0 || (n>0 && p!=NULL));
  if(n<<3!=dev->readBlock((char *) p,n<<3)) FXERRGIO(FXTrans::tr("FXStream", "Premature EOF encountered"));
  if(swap&&n){do{swap8(p++);}while(--n);}
  return *this;
  }
#endif


/*******************************  Save Objects  ********************************/


// Save object
FXStream& FXStream::saveObject(const FXObject* v){
  const FXMetaClass *cls;
  register FXuint p,x;
  FXuint tag,esc=0;
  if(dir!=FXStreamSave){ fxerror("FXStream::saveObject: wrong stream direction.\n"); }
  if(code==FXStreamOK){
    if(v==NULL){
      tag=0;
      *this << tag;
      return *this;
      }
    p=HASH1(v,ntable);
    FXASSERT(p<ntable);
    x=HASH2(v,ntable);
    FXASSERT(1<=x && x<ntable);
    while(table[p].ref!=UNUSEDSLOT){
      if(table[p].obj==v){
        FXASSERT(table[p].ref<=no);
        tag=table[p].ref|0x80000000;
        *this << tag;
        return *this;
        }
      p = (p+x)%ntable;
      }
    table[p].obj=(FXObject*)v;
    table[p].ref=no++;
    FXASSERT(no<ntable);
    if((100*no)>=(MAX_LOAD*ntable)) grow();
    cls=v->getMetaClass();
    tag=cls->getClassNameLength();
    if(tag>MAXCLASSNAME){
      code=FXStreamFormat;                    // Class name too long
      return *this;
      }
    *this << tag;
    *this << esc;                             // Escape code for future expension; must be 0 for now
    save(cls->getClassName(),cls->getClassNameLength());
    FXTRACE((100,"saveObject(%s)\n",v->getClassName()));
    v->save(*this);
    }
  return *this;
  }


/*******************************  Load Objects  ********************************/

// Load object
FXStream& FXStream::loadObject(FXObject*& v){
  const FXMetaClass *cls;
  FXchar obnam[MAXCLASSNAME];
  FXuint tag,esc;
  if(dir!=FXStreamLoad){ fxerror("FXStream::loadObject: wrong stream direction.\n"); }
  if(code==FXStreamOK){
    *this >> tag;
    if(tag==0){
      v=NULL;
      return *this;
      }
    if(tag&0x80000000){
      tag&=0x7fffffff;
      if(tag>=no){                            // Out-of-range reference number
        code=FXStreamFormat;                  // Bad format in stream
        return *this;
        }
      if(table[tag].ref!=tag){                // We should have constructed the object already!
        code=FXStreamFormat;                  // Bad format in stream
        return *this;
        }
      FXASSERT(tag<ntable);
      v=table[tag].obj;
      FXASSERT(v);
      return *this;
      }
    if(tag>MAXCLASSNAME){                     // Out-of-range class name string
      code=FXStreamFormat;                    // Bad format in stream
      return *this;
      }
    *this >> esc;                             // Read but ignore escape code
    load(obnam,tag);
    cls=FXMetaClass::getMetaClassFromName(obnam);
    if(cls==NULL){                            // No FXMetaClass with this class name
      code=FXStreamUnknown;                   // Unknown class
      return *this;
      }
    v=cls->makeInstance();                    // Build some object!!
    if(v==NULL){
      code=FXStreamAlloc;                     // Unable to construct object
      return *this;
      }
    FXASSERT(no<ntable);
    table[no].obj=v;                          // Save pointer in table
    table[no].ref=no;                         // Save reference number also!
    no++;
    if(no>=ntable) grow();
    FXTRACE((100,"loadObject(%s)\n",v->getClassName()));
    v->load(*this);
    }
  return *this;
  }

}
