/********************************************************************************
*                                                                               *
*                   U n i c o d e   T e x t   C o d e c                         *
*                                                                               *
*********************************************************************************
* Copyright (C) 2002,2004 by Lyle Johnson.   All Rights Reserved.               *
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
* $Id: FXTextCodec.cpp,v 1.8 2004/02/08 17:29:07 fox Exp $                      *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "FXStream.h"
#include "FXDict.h"
#include "FXString.h"
#include "FXTextCodec.h"
#include "FXUTF8Codec.h"

#include "FXMemDbg.h"
#if defined(DEBUG) && !defined(FXMEMDBG_DISABLE)
static const char *_fxmemdbg_current_file_ = __FILE__;
#endif

/*
  Notes:

  - IANA defined mime names for character sets are found in:

        http://www.iana.org/assignments/character-sets.
*/

/*******************************************************************************/

namespace FX {


class FXTextCodecDict : public FXDict {
  FXDECLARE(FXTextCodecDict)
private:
  FXTextCodecDict(const FXTextCodecDict&);
  FXTextCodecDict& operator=(const FXTextCodecDict&);
public:

  /// Construct a codec dictionary
  FXTextCodecDict(){}

  /// Insert named codec into dictionary
  FXTextCodec* insert(const FXchar* name,FXTextCodec* codec);

  /// Remove named codec from dictionary
  FXTextCodec* remove(const FXchar* name);

  /// Find codec by name
  FXTextCodec* find(const FXchar* name) const;
  };


FXIMPLEMENT(FXTextCodecDict,FXDict,NULL,0)


// Insert codec with name
FXTextCodec* FXTextCodecDict::insert(const FXchar* name,FXTextCodec* codec){
  return reinterpret_cast<FXTextCodec*>(FXDict::insert(name,codec));
  }


// Remove named codec
FXTextCodec* FXTextCodecDict::remove(const FXchar* name){
  return reinterpret_cast<FXTextCodec*>(FXDict::remove(name));
  }


// Find named codec
FXTextCodec* FXTextCodecDict::find(const FXchar* name) const {
  return reinterpret_cast<FXTextCodec*>(FXDict::find(name));
  }


//---------------------------------------------------------------------------


// Dictionary maps names to codecs
FXTextCodecDict* FXTextCodec::codecs=0;

/* 27th Nov 2004 ned: It bugs me to see memory leaks after program exit, so here's
a hacked in text code cleanup routine Jeroen really should implement himself by now */
static struct DeleteFXTextCodecs
{
	~DeleteFXTextCodecs()
	{
		FXTextCodec *codec;
		FXTextCodecDict *codecs=FXTextCodec::codecs;
		if(codecs)
		{
			for(int n=0; n<codecs->size(); n++)
			{
				codec=(FXTextCodec *) codecs->data(n);
				delete codec;
			}
			delete codecs; codecs=0;
		}
	}
} deleteFXTextCodecs;


// Register codec associated with this name.
FXbool FXTextCodec::registerCodec(const FXchar* name,FXTextCodec* codec){
  if(codecs==0){
    codecs=new FXTextCodecDict;
    }
  if(codecs->find(name)==0){
    codecs->insert(name,codec);
    return TRUE;
    }
  else{
    return FALSE;
    }
  }


// Return the codec associated with this name (or NULL if no match is found)
FXTextCodec* FXTextCodec::codecForName(const FXchar* name){
  return (codecs!=0)?codecs->find(name):0;
  }


//---------------------------------------------------------------------------


/**
 * Codec for all supported ISO-8859-x character sets.
 * The ISO 8859 character codes are isomorphic in the sense that code
 * positions 0-127 contain the same character as in ASCII, positions
 * 128-159 are unused (reserved for control characters) and positions
 * 160-255 are the varying part, used differently in different members
 * of the ISO 8859 family.
 */
class FXISOTextCodec : public FXTextCodec {
private:
  FXString name;
  FXint mib;
  const FXwchar* table;
protected:
  static const FXuchar SUBSTITUTE;
protected:
  FXISOTextCodec(){}
  FXbool canEncode(FXwchar c) const;
  FXuchar encode(FXwchar c) const;
  FXwchar decode(FXuchar c) const;
public:

  /// Constructor
  FXISOTextCodec(const FXString& nm,FXint m,const FXwchar* t);

  /// Convert from Unicode to this encoding
  virtual unsigned long fromUnicode(FXuchar*& dest,unsigned long m,const FXwchar*& src,unsigned long n);

  /// Convert from this encoding to Unicode
  virtual unsigned long toUnicode(FXwchar*& dest,unsigned long m,const FXuchar*& src,unsigned long n);

  // Return the IANA mime name for this codec
  virtual const FXchar* mimeName() const { return name.text(); }

  /// Return the Management Information Base (MIBenum) for the character set.
  virtual FXint mibEnum() const { return mib; }

  /// Destructor
  virtual ~FXISOTextCodec(){}
  };


/**
 * Character to substitute when we're encoding a Unicode string into this
 * character set and there's no corresponding character.
 */
const FXuchar FXISOTextCodec::SUBSTITUTE='?';


// Number of items in table
static const FXint NTABLEITEMS=255-160+1;

// Constructor
FXISOTextCodec::FXISOTextCodec(const FXString& nm,FXint m,const FXwchar* t){
  name=nm;
  mib=m;
  table=t;
  }


// Return true if can encode this character
FXbool FXISOTextCodec::canEncode(FXwchar c) const {
  register FXint i;
  if(c<0x80){
    return TRUE;
    }
  else{
    for(i=0; i<NTABLEITEMS; i++){
      if(table[i]==c) return TRUE;
      }
    }
  return FALSE;
  }


// Encode this character (i.e. convert from Unicode to this encoding)
FXuchar FXISOTextCodec::encode(FXwchar c) const {
  register FXint i;
  if(c<0x80){
    return TRUE;
    }
  else{
    for(i=0; i<NTABLEITEMS; i++){
      if(table[i]==c) return 160+i;
      }
    }
  return SUBSTITUTE;
  }


// Decode this character (i.e. convert from this encoding to Unicode)
FXwchar FXISOTextCodec::decode(FXuchar c) const {
  FXASSERT(c<0x80 || c>0x9f);
  return (c<0x80)?c:table[c-160];
  }


// Convert a sequence of wide characters from Unicode to this encoding
unsigned long FXISOTextCodec::fromUnicode(FXuchar*& dest,unsigned long m,const FXwchar*& src,unsigned long n){
  unsigned long i,j;
  FXwchar c;
  i=0;
  j=0;
  while(i<n && j<m){
    c=src[i++];
    dest[j++]=canEncode(c)?encode(c):SUBSTITUTE;
    }
  src=&src[i];
  dest=&dest[j];
  return j;
  }


// Convert a sequence of bytes in this encoding to Unicode
unsigned long FXISOTextCodec::toUnicode(FXwchar*& dest,unsigned long m,const FXuchar*& src,unsigned long n){
  unsigned long i,j;
  i=0;
  j=0;
  while(i<n && j<m){
    dest[j++]=decode(src[i++]);
    }
  src=&src[i];
  dest=&dest[j];
  return i;
  }


//---------------------------------------------------------------------------


// ISO-8859-1 (Latin1)
static const FXwchar ISO_8859_1[]={
 0x00a0,0x00a1,0x00a2,0x00a3,0x00a4,0x00a5,0x00a6,0x00a7,0x00a8,0x00a9,
 0x00aa,0x00ab,0x00ac,0x00ad,0x00ae,0x00af,0x00b0,0x00b1,0x00b2,0x00b3,
 0x00b4,0x00b5,0x00b6,0x00b7,0x00b8,0x00b9,0x00ba,0x00bb,0x00bc,0x00bd,
 0x00be,0x00bf,0x00c0,0x00c1,0x00c2,0x00c3,0x00c4,0x00c5,0x00c6,0x00c7,
 0x00c8,0x00c9,0x00ca,0x00cb,0x00cc,0x00cd,0x00ce,0x00cf,0x00d0,0x00d1,
 0x00d2,0x00d3,0x00d4,0x00d5,0x00d6,0x00d7,0x00d8,0x00d9,0x00da,0x00db,
 0x00dc,0x00dd,0x00de,0x00df,0x00e0,0x00e1,0x00e2,0x00e3,0x00e4,0x00e5,
 0x00e6,0x00e7,0x00e8,0x00e9,0x00ea,0x00eb,0x00ec,0x00ed,0x00ee,0x00ef,
 0x00f0,0x00f1,0x00f2,0x00f3,0x00f4,0x00f5,0x00f6,0x00f7,0x00f8,0x00f9,
 0x00fa,0x00fb,0x00fc,0x00fd,0x00fe,0x00ff};


// ISO-8859-2 (Latin2)
static const FXwchar ISO_8859_2[]={
 0x00a0,0x0104,0x02d8,0x0141,0x00a4,0x013d,0x015a,0x00a7,0x00a8,0x0160,
 0x015e,0x0164,0x0179,0x00ad,0x017d,0x017b,0x00b0,0x0105,0x02db,0x0142,
 0x00b4,0x013e,0x015b,0x02c7,0x00b8,0x0161,0x015f,0x0165,0x017a,0x02dd,
 0x017e,0x017c,0x0154,0x00c1,0x00c2,0x0102,0x00c4,0x0139,0x0106,0x00c7,
 0x010c,0x00c9,0x0118,0x00cb,0x011a,0x00cd,0x00ce,0x010e,0x0110,0x0143,
 0x0147,0x00d3,0x00d4,0x0150,0x00d6,0x00d7,0x0158,0x016e,0x00da,0x0170,
 0x00dc,0x00dd,0x0162,0x00df,0x0155,0x00e1,0x00e2,0x0103,0x00e4,0x013a,
 0x0107,0x00e7,0x010d,0x00e9,0x0119,0x00eb,0x011b,0x00ed,0x00ee,0x010f,
 0x0111,0x0144,0x0148,0x00f3,0x00f4,0x0151,0x00f6,0x00f7,0x0159,0x016f,
 0x00fa,0x0171,0x00fc,0x00fd,0x0163,0x02d9};


// ISO-8859-3 (Latin3)
static const FXwchar ISO_8859_3[]={
 0x00a0,0x0126,0x02d8,0x00a3,0x00a4,0x0124,0x00a7,0x00a8,0x0130,
 0x015e,0x011e,0x0134,0x00ad,0x017b,0x00b0,0x0127,0x00b2,0x00b3,
 0x00b4,0x00b5,0x0125,0x00b7,0x00b8,0x0131,0x015f,0x011f,0x0135,0x00bd,
 0x017c,0x00c0,0x00c1,0x00c2,0x00c4,0x010a,0x0108,0x00c7,
 0x00c8,0x00c9,0x00ca,0x00cb,0x00cc,0x00cd,0x00ce,0x00cf,0x00d1,
 0x00d2,0x00d3,0x00d4,0x0120,0x00d6,0x00d7,0x011c,0x00d9,0x00da,0x00db,
 0x00dc,0x016c,0x015c,0x00df,0x00e0,0x00e1,0x00e2,0x00e4,0x010b,
 0x0109,0x00e7,0x00e8,0x00e9,0x00ea,0x00eb,0x00ec,0x00ed,0x00ee,0x00ef,
 0x00f1,0x00f2,0x00f3,0x00f4,0x0121,0x00f6,0x00f7,0x011d,0x00f9,
 0x00fa,0x00fb,0x00fc,0x016d,0x015d,0x02d9};


// ISO-8859-4 (Latin4)
static const FXwchar ISO_8859_4[]={
 0x00a0,0x0104,0x0138,0x0156,0x00a4,0x0128,0x013b,0x00a7,0x00a8,0x0160,
 0x0112,0x0122,0x0166,0x00ad,0x017d,0x00af,0x00b0,0x0105,0x02db,0x0157,
 0x00b4,0x0129,0x013c,0x02c7,0x00b8,0x0161,0x0113,0x0123,0x0167,0x014a,
 0x017e,0x014b,0x0100,0x00c1,0x00c2,0x00c3,0x00c4,0x00c5,0x00c6,0x012e,
 0x010c,0x00c9,0x0118,0x00cb,0x0116,0x00cd,0x00ce,0x012a,0x0110,0x0145,
 0x014c,0x0136,0x00d4,0x00d5,0x00d6,0x00d7,0x00d8,0x0172,0x00da,0x00db,
 0x00dc,0x0168,0x016a,0x00df,0x0101,0x00e1,0x00e2,0x00e3,0x00e4,0x00e5,
 0x00e6,0x012f,0x010d,0x00e9,0x0119,0x00eb,0x0117,0x00ed,0x00ee,0x012b,
 0x0111,0x0146,0x014d,0x0137,0x00f4,0x00f5,0x00f6,0x00f7,0x00f8,0x0173,
 0x00fa,0x00fb,0x00fc,0x0169,0x016b,0x02d9};


// ISO-8859-5 (Cyrillic)
static const FXwchar ISO_8859_5[]={
 0x00a0,0x0401,0x0402,0x0403,0x0404,0x0405,0x0406,0x0407,0x0408,0x0409,
 0x040a,0x040b,0x040c,0x00ad,0x040e,0x040f,0x0410,0x0411,0x0412,0x0413,
 0x0414,0x0415,0x0416,0x0417,0x0418,0x0419,0x041a,0x041b,0x041c,0x041d,
 0x041e,0x041f,0x0420,0x0421,0x0422,0x0423,0x0424,0x0425,0x0426,0x0427,
 0x0428,0x0429,0x042a,0x042b,0x042c,0x042d,0x042e,0x042f,0x0430,0x0431,
 0x0432,0x0433,0x0434,0x0435,0x0436,0x0437,0x0438,0x0439,0x043a,0x043b,
 0x043c,0x043d,0x043e,0x043f,0x0440,0x0441,0x0442,0x0443,0x0444,0x0445,
 0x0446,0x0447,0x0448,0x0449,0x044a,0x044b,0x044c,0x044d,0x044e,0x044f,
 0x2116,0x0451,0x0452,0x0453,0x0454,0x0455,0x0456,0x0457,0x0458,0x0459,
 0x045a,0x045b,0x045c,0x00a7,0x045e,0x045f};


// ISO-8859-7 (Greek)
static const FXwchar ISO_8859_7[]={
 0x00a0,0x02bd,0x02bc,0x00a3,0x00a4,0x00a5,0x00a6,0x00a7,0x00a8,0x00a9,
 0x00aa,0x00ab,0x00ac,0x00ad,0x00ae,0x2015,0x00b0,0x00b1,0x00b2,0x00b3,
 0x0384,0x0385,0x0386,0x00b7,0x0388,0x0389,0x038a,0x00bb,0x038c,0x00bd,
 0x038e,0x038f,0x0390,0x0391,0x0392,0x0393,0x0394,0x0395,0x0396,0x0397,
 0x0398,0x0399,0x039a,0x039b,0x039c,0x039d,0x039e,0x039f,0x03a0,0x03a1,
 0x00d2,0x03a3,0x03a4,0x03a5,0x03a6,0x03a7,0x03a8,0x03a9,0x03aa,0x03ab,
 0x03ac,0x03ad,0x03ae,0x03af,0x03b0,0x03b1,0x03b2,0x03b3,0x03b4,0x03b5,
 0x03b6,0x03b7,0x03b8,0x03b9,0x03ba,0x03bb,0x03bc,0x03bd,0x03be,0x03bf,
 0x03c0,0x03c1,0x03c2,0x03c3,0x03c4,0x03c5,0x03c6,0x03c7,0x03c8,0x03c9,
 0x03ca,0x03cb,0x03cc,0x03cd,0x03ce,0x00ff};


// ISO-8859-7 (Latin5)
static const FXwchar ISO_8859_9[]={
 0x00a0,0x00a1,0x00a2,0x00a3,0x00a4,0x00a5,0x00a6,0x00a7,0x00a8,0x00a9,
 0x00aa,0x00ab,0x00ac,0x00ad,0x00ae,0x00af,0x00b0,0x00b1,0x00b2,0x00b3,
 0x00b4,0x00b5,0x00b6,0x00b7,0x00b8,0x00b9,0x00ba,0x00bb,0x00bc,0x00bd,
 0x00be,0x00bf,0x00c0,0x00c1,0x00c2,0x00c3,0x00c4,0x00c5,0x00c6,0x00c7,
 0x00c8,0x00c9,0x00ca,0x00cb,0x00cc,0x00cd,0x00ce,0x00cf,0x011e,0x00d1,
 0x00d2,0x00d3,0x00d4,0x00d5,0x00d6,0x00d7,0x00d8,0x00d9,0x00da,0x00db,
 0x00dc,0x0130,0x015e,0x00df,0x00e0,0x00e1,0x00e2,0x00e3,0x00e4,0x00e5,
 0x00e6,0x00e7,0x00e8,0x00e9,0x00ea,0x00eb,0x00ec,0x00ed,0x00ee,0x00ef,
 0x011f,0x00f1,0x00f2,0x00f3,0x00f4,0x00f5,0x00f6,0x00f7,0x00f8,0x00f9,
 0x00fa,0x00fb,0x00fc,0x0131,0x015f,0x00ff};


// ISO-8859-15 (Latin9)
static const FXwchar ISO_8859_15[]={
 0x00a0,0x00a1,0x00a2,0x00a3,0x20ac,0x00a5,0x0160,0x00a7,0x0161,0x00a9,
 0x00aa,0x00ab,0x00ac,0x00ad,0x00ae,0x00af,0x00b0,0x00b1,0x00b2,0x00b3,
 0x017d,0x00b5,0x00b6,0x00b7,0x017e,0x00b9,0x00ba,0x00bb,0x0152,0x0153,
 0x0178,0x00bf,0x00c0,0x00c1,0x00c2,0x00c3,0x00c4,0x00c5,0x00c6,0x00c7,
 0x00c8,0x00c9,0x00ca,0x00cb,0x00cc,0x00cd,0x00ce,0x00cf,0x00d0,0x00d1,
 0x00d2,0x00d3,0x00d4,0x00d5,0x00d6,0x00d7,0x00d8,0x00d9,0x00da,0x00db,
 0x00dc,0x00dd,0x00de,0x00df,0x00e0,0x00e1,0x00e2,0x00e3,0x00e4,0x00e5,
 0x00e6,0x00e7,0x00e8,0x00e9,0x00ea,0x00eb,0x00ec,0x00ed,0x00ee,0x00ef,
 0x00f0,0x00f1,0x00f2,0x00f3,0x00f4,0x00f5,0x00f6,0x00f7,0x00f8,0x00f9,
 0x00fa,0x00fb,0x00fc,0x00fd,0x00fe,0x00ff};


class FXTextCodecRegistrar {
public:
  FXTextCodecRegistrar(){
    FXTextCodec::registerCodec("ISO-8859-1",new FXISOTextCodec("ISO-8859-1",4,ISO_8859_1));
    FXTextCodec::registerCodec("ISO-8859-2",new FXISOTextCodec("ISO-8859-2",5,ISO_8859_2));
    FXTextCodec::registerCodec("ISO-8859-3",new FXISOTextCodec("ISO-8859-3",6,ISO_8859_3));
    FXTextCodec::registerCodec("ISO-8859-4",new FXISOTextCodec("ISO-8859-4",7,ISO_8859_4));
    FXTextCodec::registerCodec("ISO-8859-5",new FXISOTextCodec("ISO-8859-5",8,ISO_8859_5));
    FXTextCodec::registerCodec("ISO-8859-7",new FXISOTextCodec("ISO-8859-7",10,ISO_8859_7));
    FXTextCodec::registerCodec("ISO-8859-9",new FXISOTextCodec("ISO-8859-9",12,ISO_8859_9));
    FXTextCodec::registerCodec("ISO-8859-15",new FXISOTextCodec("ISO-8859-15",111,ISO_8859_15));
    FXTextCodec::registerCodec("UTF-8",new FXUTF8Codec);
    }
  };


static FXTextCodecRegistrar textCodecRegistrar;


}
