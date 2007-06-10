/********************************************************************************
*                                                                               *
*                          U t i l i t y   F u n c t i o n s                    *
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
* $Id: fxutils.cpp,v 1.103.2.1 2005/12/13 16:57:32 fox Exp $                        *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "fxkeys.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXString.h"
#include "QThread.h"
#include "FXException.h"
#include "FXRollback.h"
#include "QTrans.h"
#include <qdict.h>
#if defined(__GNUC__) &&  __GNUC__>=3
#include <cxxabi.h>
#endif

/*
  Notes:
  - Those functions manipulating strings should perhaps become FXString type
    functions?
  - Need to pass buffer-size argument to all those fxdirpart() etc. functions
    to protect against memory overruns (make it a default argument last one
    so as to not impact anyone).
  - Revive my old malloc() replacement library to detect memory block overwrites.
*/


// Make it larger if you need
#ifndef MAXMESSAGESIZE
#define MAXMESSAGESIZE 4096
#endif


// Allows things to find TnFOX
extern "C" FXAPI void findTnFOX(void){ }


/*******************************************************************************/

namespace FX {

// Global flag which controls tracing level
unsigned int fxTraceLevel=0;


// Version number that the library has been compiled with
const FXuchar fxversion[3]={FOX_MAJOR,FOX_MINOR,FOX_LEVEL};

// TnFOX version number
const FXuchar tnfoxversion[2]={TNFOX_MAJOR, TNFOX_MINOR};


// Thread-safe, linear congruential random number generator from Knuth & Lewis.
FXuint fxrandom(FXuint& seed){
  seed=1664525UL*seed+1013904223UL;
  return seed;
  }

// These had to be defined out of line to avoid dragging FXStream.h into everything
namespace Generic
{
	FXStream &operator<<(FXStream &s, const typeInfoBase &i)
	{
		s << i.decorated << i.readable;
		return s;
	}
	FXStream &operator>>(FXStream &s, typeInfoBase &i)
	{
		s >> i.decorated >> i.readable;
		return s;
	}
}

/* Niall's Adler32 implementation using all the tricks he knows.
Reference: Adler32 algorithm details are in RFC1950 and it is non-patented (thus far)

The adler32 checksum algorithm is at least 1.5x times faster than any
CRC32 algorithm and is nearly as good when on large data sets. zlib's
implementation just happens to run optimally on x86 as fixed index
operations are quick so I've duplicated that below. On an ARM though
a Duff's Device with pointer increment would be quickest as memory
access ops can also write back an incremented pointer. My changes
below involve replacing the slow divide operation and a duff's device
for the smaller blocks - these gain you about 2-5% on MSVC7.1 which
is ~290-300Mb/sec on my machine (dual Athlon 1700).

The modulus replacement is an old assembler trick using the fact that for 
getting the modulus of 2^n divisors is simply modulus=val & (divisor-1).

We're dividing by BASE which is 65521, 15 short of 65536. Therefore:
while(a>0xffff) a=(a & 0xffff)+15*(a>>16)
or
while(a>0xffff) a=(a & 0xffff)+((a>>12)&0xfffffff0))-(a>>16)

However on modern x86, imul is very fast indeed so we use the former.
*/
#define ADLERMODULUS(a) while(a>0xffff) a=(a & 0xffff)+15*(a>>16)
#define NMAX 5552
/* NMAX is the largest n such that 255n(n+1)/2 + (n+1)(BASE-1) <= 2^32-1 */
#define ALDO1(i) p1+=buff[i]; p2+=p1;
#define ALDO2(i) ALDO1(i) ALDO1(i+1)
#define ALDO4(i) ALDO2(i) ALDO2(i+2)
#define ALDO8(i) ALDO4(i) ALDO4(i+4)
#define ALDO16   ALDO8(0) ALDO8(8)
FXuint fxadler32(FXuint c, const FXuchar *buff, FXuval len) throw()
{
	FXuint p1=c & 0xffff, p2=(c>>16) & 0xffff;
	while(len>0)
	{
		FXint chunk=(FXint)((len<NMAX) ? len : NMAX);
		len-=chunk;
		while(chunk>=16)
		{
			ALDO16
			buff+=16; chunk-=16;
		}
		if(chunk)
		{
			switch(chunk & 7)	// This in case you're interested is a Duff's Device
			{					// (an old C trick to unroll loops - modern compilers
			case 0:				// do this for you but not in debug mode!)
				while(chunk>0)
				{
					p1+=*buff++; p2+=p1;
			case 7: p1+=*buff++; p2+=p1;
			case 6: p1+=*buff++; p2+=p1;
			case 5: p1+=*buff++; p2+=p1;
			case 4: p1+=*buff++; p2+=p1;
			case 3: p1+=*buff++; p2+=p1;
			case 2: p1+=*buff++; p2+=p1;
			case 1: p1+=*buff++; p2+=p1;
					chunk-=8;
				}
			}
		}
		ADLERMODULUS(p1);
		ADLERMODULUS(p2);
	}
	return (p2<<16)|p1;
}

/// Dumps the contents of a buffer into a string in byte format (useful for debugging)
FXString fxdump8(FXuchar *buffer, FXuval len) throw()
{
	FXString ret, val;
	ret.format("Dump of %d bytes at 0x%p: ", (int) len, buffer);
	for(FXuval n=0; n<len; n++)
	{
		val.format("%.2x ", buffer[n]);
		ret+=val;
	}
	ret[ret.length()-1]='\n';
	return ret;
}

/// Dumps the contents of a buffer into a string in 32 bit word format (useful for debugging)
FXString fxdump32(FXuint *buffer, FXuval len) throw()
{
	FXString ret, val;
	ret.format("Dump of %d words at 0x%p: ", (int) len, buffer);
	for(FXuval n=0; n<len; n++)
	{
		val.format("%.8x ", buffer[n]);
		ret+=val;
	}
	ret[ret.length()-1]='\n';
	return ret;
}

/// Turns a file size into a string
FXString fxstrfval(FXfval mysize, FXint fw, char fmt, int prec)
{
	//if(mysize>=1024LL*1024*1024*1024*1024*1024*1024*1024)	// Yottabyte
	//	return QTrans::tr("QFileInfo", "%1Yb").arg((float)mysize/(1024LL*1024*1024*1024*1024*1024*1024*1024), fw, fmt, prec);
	//else if(mysize>=1024LL*1024*1024*1024*1024*1024*1024)	// Zettabyte
	//	return QTrans::tr("QFileInfo", "%1Zb").arg((float)mysize/(1024LL*1024*1024*1024*1024*1024*1024), fw, fmt, prec);
	if(mysize>=1024LL*1024*1024*1024*1024*1024)				// Exabyte
		return QTrans::tr("QFileInfo", "%1Eb").arg((float)mysize/(1024LL*1024*1024*1024*1024*1024), fw, fmt, prec);
	else if(mysize>=1024LL*1024*1024*1024*1024)				// Petabyte
		return QTrans::tr("QFileInfo", "%1Pb").arg((float)mysize/(1024LL*1024*1024*1024*1024), fw, fmt, prec);
	else if(mysize>=1024LL*1024*1024*1024)					// Terabyte
		return QTrans::tr("QFileInfo", "%1Tb").arg((float)mysize/(1024LL*1024*1024*1024), fw, fmt, prec);
	else if(mysize>=1024*1024*1024)							// Gigabyte
		return QTrans::tr("QFileInfo", "%1Gb").arg((float)mysize/(1024*1024*1024), fw, fmt, prec);
	else if(mysize>=1024*1024)								// Megabyte
		return QTrans::tr("QFileInfo", "%1Mb").arg((float)mysize/(1024*1024), fw, fmt, prec);
	else if(mysize>=1024)									// Kilobyte
		return QTrans::tr("QFileInfo", "%1Kb").arg((float)mysize/(1024), fw, fmt, prec);
	else
		return QTrans::tr("QFileInfo", "%1 bytes").arg(mysize);
}

/// A decreasing array of two's powered prime numbers (useful for adjust hash tables based on memory load)
const FXuint *fx2powerprimes(FXuint topval) throw()
{
	static const FXuint primes[]={ 4095991, 2048003, 1023991, 511997, 255989, 127997, 63997, 32003, 16001, 7993, 4001, 1999, 997, 499, 251, 127, 61, 31, 13, 7, 3, 2, 1, 1, 1, 1 };
	const FXuint *ret=primes;
	topval++;
	assert(topval<=primes[0]);
	while(*ret>topval) ret++;
	return (ret==primes) ? primes : --ret;
}


// Allocate memory
FXint fxmalloc(void** ptr,unsigned long size){
  *ptr=NULL;
  if(size!=0){
    if((*ptr=malloc(size))==NULL) return FALSE;
    }
  return TRUE;
  }


// Allocate cleaned memory
FXint fxcalloc(void** ptr,unsigned long size){
  *ptr=NULL;
  if(size!=0){
    if((*ptr=calloc(size,1))==NULL) return FALSE;
    }
  return TRUE;
  }


// Resize memory
FXint fxresize(void** ptr,unsigned long size){
  register void *p=NULL;
  if(size!=0){
    if((p=realloc(*ptr,size))==NULL) return FALSE;
    }
  else{
    if(*ptr) free(*ptr);
    }
  *ptr=p;
  return TRUE;
  }


// Allocate and initialize memory
FXint fxmemdup(void** ptr,const void* src,unsigned long size){
  *ptr=NULL;
  if(size!=0 && src!=NULL){
    if((*ptr=malloc(size))==NULL) return FALSE;
    memcpy(*ptr,src,size);
    }
  return TRUE;
  }


// String duplicate
FXchar *fxstrdup(const FXchar* str){
  register FXchar *copy;
  if(str!=NULL && (copy=(FXchar*)malloc(strlen(str)+1))!=NULL){
    strcpy(copy,str);
    return copy;
    }
  return NULL;
  }


// Free memory, resets ptr to NULL afterward
void fxfree(void** ptr){
  if(*ptr){
    free(*ptr);
    *ptr=NULL;
    }
  }


#ifdef WIN32

// Return TRUE if console application
FXbool fxisconsole(const FXchar *path){
  IMAGE_OPTIONAL_HEADER optional_header;
  IMAGE_FILE_HEADER     file_header;
  IMAGE_DOS_HEADER      dos_header;
  DWORD                 dwCoffHeaderOffset;
  DWORD                 dwNewOffset;
  DWORD                 dwMoreDosHeader[16];
  ULONG                 ulNTSignature;
  HANDLE                hImage;
  DWORD                 dwBytes;
  FXbool                flag=MAYBE;

  // Open the application file.
  hImage=CreateFile(path,GENERIC_READ,FILE_SHARE_DELETE|FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
  if(hImage!=INVALID_HANDLE_VALUE){

    // Read MS-Dos image header.
    if(ReadFile(hImage,&dos_header,sizeof(IMAGE_DOS_HEADER),&dwBytes,NULL)==0) goto x;

    // Test bytes read
    if(dwBytes!=sizeof(IMAGE_DOS_HEADER)) goto x;

    // Test signature
    if(dos_header.e_magic!=IMAGE_DOS_SIGNATURE) goto x;

    // Read more MS-Dos header.
    if(ReadFile(hImage,dwMoreDosHeader,sizeof(dwMoreDosHeader),&dwBytes,NULL)==0) goto x;

    // Test bytes read
    if(dwBytes!=sizeof(dwMoreDosHeader)) goto x;

    // Move the file pointer to get the actual COFF header.
    dwNewOffset=SetFilePointer(hImage,dos_header.e_lfanew,NULL,FILE_BEGIN);
    dwCoffHeaderOffset=dwNewOffset+sizeof(ULONG);
    if(dwCoffHeaderOffset==0xFFFFFFFF) goto x;

    // Read NT signature of the file.
    if(ReadFile(hImage,&ulNTSignature,sizeof(ULONG),&dwBytes,NULL)==0) goto x;

    // Test bytes read
    if(dwBytes!=sizeof(ULONG)) goto x;

    // Test NT signature
    if(ulNTSignature!=IMAGE_NT_SIGNATURE) goto x;

    if(ReadFile(hImage,&file_header,IMAGE_SIZEOF_FILE_HEADER,&dwBytes,NULL)==0) goto x;

    // Test bytes read
    if(dwBytes!=IMAGE_SIZEOF_FILE_HEADER) goto x;

    // Read the optional header of file.
    if(ReadFile(hImage,&optional_header,IMAGE_SIZEOF_NT_OPTIONAL_HEADER,&dwBytes,NULL)==0) goto x;

    // Test bytes read
    if(dwBytes!=IMAGE_SIZEOF_NT_OPTIONAL_HEADER) goto x;

    // Switch on systems
    switch(optional_header.Subsystem){
      case IMAGE_SUBSYSTEM_WINDOWS_GUI:     // Windows GUI (2)
      case IMAGE_SUBSYSTEM_WINDOWS_CE_GUI:  // Windows CE GUI (9)
        flag=FALSE;
        break;
      case IMAGE_SUBSYSTEM_WINDOWS_CUI:     // Windows Console (3)
      case IMAGE_SUBSYSTEM_OS2_CUI:         // OS/2 Console (5)
      case IMAGE_SUBSYSTEM_POSIX_CUI:       // Posix Console (7)
        flag=TRUE;
        break;
      case IMAGE_SUBSYSTEM_NATIVE:          // Native (1)
      case IMAGE_SUBSYSTEM_NATIVE_WINDOWS:  // Native Win9x (8)
      case IMAGE_SUBSYSTEM_UNKNOWN:         // Unknown (0)
      default:
        break;
      }
x:  CloseHandle(hImage);
    }
  return flag;
  }


#else

// Return TRUE if console application
FXbool fxisconsole(const FXchar*){
  return TRUE;
  }

#endif


// Assert failed routine
void fxassert(const char* expression,const char* filename,unsigned int lineno){
#ifndef WIN32
  fprintf(stderr,"%s:%d: FXASSERT(%s) failed.\n",filename,lineno,expression);
#else
#ifdef _WINDOWS
  char msg[MAXMESSAGESIZE];
  sprintf(msg,"%s(%d): FXASSERT(%s) failed.\n",filename,lineno,expression);
  OutputDebugString(msg);
  fprintf(stderr,"%s",msg); // if a console is available
#else
  fprintf(stderr,"%s(%d): FXASSERT(%s) failed.\n",filename,lineno,expression);
#endif
#endif
  }


// Log message to [typically] stderr
void fxmessage(const char* format,...){
#ifndef WIN32                         // Unix
  QThread_DTHold dthold;
  va_list arguments;
  va_start(arguments,format);
  vfprintf(stderr,format,arguments);
  va_end(arguments);
#else
#ifdef _WINDOWS
  char msg[MAXMESSAGESIZE];
  va_list arguments;
  va_start(arguments,format);
  vsnprintf(msg,sizeof(msg),format,arguments);
  va_end(arguments);
  OutputDebugString(msg);
  fprintf(stderr,"%s",msg); // if a console is available
#else
  va_list arguments;
  va_start(arguments,format);
  vfprintf(stderr,format,arguments);
  va_end(arguments);
#endif
#endif
  }


// Error routine
void fxerror(const char* format,...){
#ifndef WIN32
  QThread_DTHold dthold;
  va_list arguments;
  va_start(arguments,format);
  vfprintf(stderr,format,arguments);
  va_end(arguments);
#else
#ifdef _WINDOWS
  char msg[MAXMESSAGESIZE];
  va_list arguments;
  va_start(arguments,format);
  vsnprintf(msg,sizeof(msg),format,arguments);
  va_end(arguments);
  OutputDebugString(msg);
  fprintf(stderr,"%s",msg); // if a console is available
  MessageBox(NULL,msg,NULL,MB_OK|MB_ICONEXCLAMATION|MB_APPLMODAL);
  DebugBreak();
#else
  va_list arguments;
  va_start(arguments,format);
  vfprintf(stderr,format,arguments);
  va_end(arguments);
#endif
#endif
  abort();
  }


// Warning routine
void fxwarning(const char* format,...){
#ifndef WIN32
  QThread_DTHold dthold;
  va_list arguments;
  va_start(arguments,format);
  vfprintf(stderr,format,arguments);
  va_end(arguments);
#else
#ifdef _WINDOWS
  char msg[MAXMESSAGESIZE];
  va_list arguments;
  va_start(arguments,format);
  vsnprintf(msg,sizeof(msg),format,arguments);
  va_end(arguments);
  OutputDebugString(msg);
  fprintf(stderr,"%s",msg); // if a console is available
  MessageBox(NULL,msg,NULL,MB_OK|MB_ICONINFORMATION|MB_APPLMODAL);
#else
  va_list arguments;
  va_start(arguments,format);
  vfprintf(stderr,format,arguments);
  va_end(arguments);
#endif
#endif
  }


// Trace printout routine
void fxtrace(unsigned int level,const char* format,...){
  if(fxTraceLevel>level){
#ifndef WIN32
    va_list arguments;
    va_start(arguments,format);
    vfprintf(stderr,format,arguments);
    va_end(arguments);
#else
#ifdef _WINDOWS
    char msg[MAXMESSAGESIZE];
    va_list arguments;
    va_start(arguments,format);
    vsnprintf(msg,sizeof(msg),format,arguments);
    va_end(arguments);
    OutputDebugString(msg);
    fprintf(stderr,"%s",msg); // if a console is available
#else
    va_list arguments;
    va_start(arguments,format);
    vfprintf(stderr,format,arguments);
    va_end(arguments);
#endif
#endif
    }
  }


// Sleep n microseconds
void fxsleep(unsigned int n){
#ifdef WIN32
  unsigned int zzz=n/1000;
  if(zzz==0) zzz=1;
  Sleep(zzz);
#else
#ifdef __USE_POSIX199309
  struct timespec value;
  value.tv_nsec = 1000 * (n%1000000);
  value.tv_sec = n/1000000;
  nanosleep(&value,NULL);
#else
#ifndef BROKEN_SELECT
  struct ::timeval value;
  value.tv_usec = n % 1000000;
  value.tv_sec = n / 1000000;
  select(1,0,0,0,&value);
#else
  unsigned int zzz=n/1000000;
  if(zzz==0) zzz=1;
  if(zzz){
    while((zzz=sleep(zzz))>0) ;
    }
#endif
#endif
#endif
  }


// Parse accelerator from menu
FXHotKey fxparseAccel(const FXString& string){
  register FXuint code=0,mods=0;
  register FXint pos=0;

  // Parse leading space
  while(pos<string.length() && isspace((FXuchar)string[pos])) pos++;

  // Parse modifiers
  while(pos<string.length()){

    // Modifier
    if(comparecase(&string[pos],"ctl",3)==0){ mods|=CONTROLMASK; pos+=3; }
    else if(comparecase(&string[pos],"ctrl",4)==0){ mods|=CONTROLMASK; pos+=4; }
    else if(comparecase(&string[pos],"alt",3)==0){ mods|=ALTMASK; pos+=3; }
    else if(comparecase(&string[pos],"meta",4)==0){ mods|=METAMASK; pos+=4; }
    else if(comparecase(&string[pos],"shift",5)==0){ mods|=SHIFTMASK; pos+=5; }
    else break;

    // Separator
    if(string[pos]=='+' || string[pos]=='-' || isspace((FXuchar)string[pos])) pos++;
    }

  // Test for some special keys
  if(comparecase(&string[pos],"home",4)==0){
    code=KEY_Home;
    }
  else if(comparecase(&string[pos],"end",3)==0){
    code=KEY_End;
    }
  else if(comparecase(&string[pos],"pgup",4)==0){
    code=KEY_Page_Up;
    }
  else if(comparecase(&string[pos],"pgdn",4)==0){
    code=KEY_Page_Down;
    }
  else if(comparecase(&string[pos],"left",4)==0){
    code=KEY_Left;
    }
  else if(comparecase(&string[pos],"right",5)==0){
    code=KEY_Right;
    }
  else if(comparecase(&string[pos],"up",2)==0){
    code=KEY_Up;
    }
  else if(comparecase(&string[pos],"down",4)==0){
    code=KEY_Down;
    }
  else if(comparecase(&string[pos],"ins",3)==0){
    code=KEY_Insert;
    }
  else if(comparecase(&string[pos],"del",3)==0){
    code=KEY_Delete;
    }
  else if(comparecase(&string[pos],"esc",3)==0){
    code=KEY_Escape;
    }
  else if(comparecase(&string[pos],"tab",3)==0){
    code=KEY_Tab;
    }
  else if(comparecase(&string[pos],"return",6)==0){
    code=KEY_Return;
    }
  else if(comparecase(&string[pos],"enter",5)==0){
    code=KEY_Return;
    }
  else if(comparecase(&string[pos],"back",4)==0){
    code=KEY_BackSpace;
    }
  else if(comparecase(&string[pos],"spc",3)==0){
    code=KEY_space;
    }
  else if(comparecase(&string[pos],"space",5)==0){
    code=KEY_space;
    }

  // Test for function keys
  else if(tolower((FXuchar)string[pos])=='f' && isdigit((FXuchar)string[pos+1])){
    if(isdigit((FXuchar)string[pos+2])){
      code=KEY_F1+10*(string[1]-'0')+(string[2]-'0')-1;
      }
    else{
      code=KEY_F1+string[pos+1]-'1';
      }
    }

  // Text if its a single character accelerator
  else if(isprint((FXuchar)string[pos])){
    if(mods&SHIFTMASK)
      code=toupper((FXuchar)string[pos])+KEY_space-' ';
    else
      code=tolower((FXuchar)string[pos])+KEY_space-' ';
    }

  FXTRACE((110,"fxparseAccel(%s) = code=%04x mods=%04x\n",string.text(),code,mods));
  return MKUINT(code,mods);
  }


// Parse hot key from string
FXHotKey fxparseHotKey(const FXString& string){
  register FXuint code=0,mods=0;
  register FXint pos=0;
  while(pos<string.length()){
    if(string[pos]=='&'){
      if(string[pos+1]!='&'){
        if(isalnum((FXuchar)string[pos+1])){
          mods=ALTMASK;
          code=tolower((FXuchar)string[pos+1])+KEY_space-' ';
          }
        break;
        }
      pos++;
      }
    pos++;
    }
  FXTRACE((110,"fxparseHotKey(%s) = code=%04x mods=%04x\n",string.text(),code,mods));
  return MKUINT(code,mods);
  }


// Obtain hot key offset in string
FXint fxfindHotKey(const FXString& string){
  register FXint pos=0;
  register FXint n=0;
  while(pos<string.length()){
    if(string[pos]=='&'){
      if(string[pos+1]!='&'){
        return n;
        }
      pos++;
      }
    pos++;
    n++;
    }
  return -1;
  }


// Strip hot key from string
FXString fxstripHotKey(const FXString& string){
  FXString result=string;
  register FXint len=result.length();
  register FXint i,j;
  for(i=j=0; j<len; j++){
    if(result[j]=='&'){
      if(result[j+1]!='&') continue;
      j++;
      }
    result[i++]=result[j];
    }
  result.trunc(i);
  return result;
  }


// Get highlight color
FXColor makeHiliteColor(FXColor clr){
  FXuint r,g,b;
  r=FXREDVAL(clr);
  g=FXGREENVAL(clr);
  b=FXBLUEVAL(clr);
  r=FXMAX(31,r);
  g=FXMAX(31,g);
  b=FXMAX(31,b);
  r=(133*r)/100;
  g=(133*g)/100;
  b=(133*b)/100;
  r=FXMIN(255,r);
  g=FXMIN(255,g);
  b=FXMIN(255,b);
  return FXRGB(r,g,b);
  }


// Get shadow color
FXColor makeShadowColor(FXColor clr){
  FXuint r,g,b;
  r=FXREDVAL(clr);
  g=FXGREENVAL(clr);
  b=FXBLUEVAL(clr);
  r=(66*r)/100;
  g=(66*g)/100;
  b=(66*b)/100;
  return FXRGB(r,g,b);
  }


// Convert to MSDOS clipboard format; we add a end
// of string at the end of the entire buffer.
FXbool fxtoDOS(FXchar*& string,FXint& len){
  register FXint f=0,t=0,c;
  while(f<len){
    if(string[f++]=='\n') t++;
    t++;
    }
  t++;
  len=t;
  if(!FXRESIZE(&string,FXchar,len)) return FALSE;
  string[--t]='\0';
  while(0<t){
    c=string[--f];
    string[--t]=c;
    if(c=='\n') string[--t]='\r';
    }
  FXASSERT(f==0);
  FXASSERT(t==0);
  return TRUE;
  }


// Convert from MSDOS clipboard format; the length passed
// in is potentially larger than the actual string length,
// so we scan until the end of string character or the end.
FXbool fxfromDOS(FXchar*& string,FXint& len){
  register FXint f=0,t=0,c;
  while(f<len && string[f]!='\0'){
    if((c=string[f++])!='\r') string[t++]=c;
    }
  len=t;
  if(!FXRESIZE(&string,FXchar,len)) return FALSE;
  return TRUE;
  }


// Get process id
FXint fxgetpid(){
#ifndef WIN32
  return getpid();
#else
  return (int)GetCurrentProcessId();
#endif
  }


// Convert RGB to HSV
void fxrgb_to_hsv(FXfloat& h,FXfloat& s,FXfloat& v,FXfloat r,FXfloat g,FXfloat b){
  FXfloat t,delta;
  v=FXMAX3(r,g,b);
  t=FXMIN3(r,g,b);
  delta=v-t;
  if(v!=0.0)
    s=delta/v;
  else
    s=0.0;
  if(s==0.0){
    h=0.0;
    }
  else{
    if(r==v)
      h=(g-b)/delta;
    else if(g==v)
      h=2.0f+(b-r)/delta;
    else if(b==v)
      h=4.0f+(r-g)/delta;
    h=h*60.0f;
    if(h<0.0) h=h+360;
    }
  }


// Convert to RGB
void fxhsv_to_rgb(FXfloat& r,FXfloat& g,FXfloat& b,FXfloat h,FXfloat s,FXfloat v){
  FXfloat f,w,q,t;
  FXint i;
  if(s==0.0){
    r=v;
    g=v;
    b=v;
    }
  else{
    if(h==360.0) h=0.0;
    h=h/60.0f;
    i=(FXint)h;
    f=h-i;
    w=v*(1.0f-s);
    q=v*(1.0f-(s*f));
    t=v*(1.0f-(s*(1.0f-f)));
    switch(i){
      case 0: r=v; g=t; b=w; break;
      case 1: r=q; g=v; b=w; break;
      case 2: r=w; g=v; b=t; break;
      case 3: r=w; g=q; b=v; break;
      case 4: r=t; g=w; b=v; break;
      case 5: r=v; g=w; b=q; break;
      }
    }
  }


// Calculate a hash value from a string; algorithm same as in perl
FXuint fxstrhash(const FXchar* str){
  register const FXuchar *s=(const FXuchar*)str;
  register FXuint h=0;
  register FXuint c;
  while((c=*s++)!='\0'){
    h = ((h << 5) + h) ^ c;
    }
  return h;
  }


// Classify IEEE 754 floating point number
FXint fxieeefloatclass(FXfloat number){
  FXfloat num=number;
  FXASSERT(sizeof(FXfloat)==sizeof(FXuint));
  FXuint s=(*((FXuint*)&num)&0x80000000);        // Sign
  FXuint e=(*((FXuint*)&num)&0x7f800000);        // Exponent
  FXuint m=(*((FXuint*)&num)&0x007fffff);        // Mantissa
  FXint result=0;
  if(e==0x7f800000){
    if(m==0)
      result=1;     // Inf
    else
      result=2;     // NaN
    if(s)
      result=-result;
    }
  return result;
  }


// Classify IEEE 754 floating point number
FXint fxieeedoubleclass(FXdouble number){
  FXdouble num=number;
  FXASSERT(sizeof(FXdouble)==2*sizeof(FXuint));
#if FOX_BIGENDIAN
  FXuint s=(((FXuint*)&num)[0]&0x80000000);     // Sign
  FXuint e=(((FXuint*)&num)[0]&0x7ff00000);     // Exponent
  FXuint h=(((FXuint*)&num)[0]&0x000fffff);     // Mantissa high
  FXuint l=(((FXuint*)&num)[1]);                // Mantissa low
#else
  FXuint s=(((FXuint*)&num)[1]&0x80000000);     // Sign
  FXuint e=(((FXuint*)&num)[1]&0x7ff00000);     // Exponent
  FXuint h=(((FXuint*)&num)[1]&0x000fffff);     // Mantissa high
  FXuint l=(((FXuint*)&num)[0]);                // Mantissa low
#endif
  FXint result=0;
  if(e==0x7ff00000){
    if(h==0 && l==0)
      result=1;     // Inf
    else
      result=2;     // NaN
    if(s)
      result=-result;
    }
  return result;
  }


/*******************************************************************************/


#if defined(__GNUC__) && defined(__linux__) && defined(__i386__)


// Capabilities
#define CPU_HAS_TSC             0x01
#define CPU_HAS_MMX             0x02
#define CPU_HAS_MMXEX           0x04
#define CPU_HAS_SSE             0x08
#define CPU_HAS_SSE2            0x10
#define CPU_HAS_3DNOW           0x20
#define CPU_HAS_3DNOWEXT        0x40


// The CPUID instruction returns stuff in eax, ecx, edx.
#define cpuid(op,eax,ecx,edx)	\
  asm volatile ("pushl %%ebx \n\t"    	\
                "cpuid       \n\t"    	\
                "popl  %%ebx \n\t"      \
                : "=a" (eax),		\
                  "=c" (ecx),           \
                  "=d" (edx)            \
                : "a" (op)              \
                : "cc")


/*
* Find out some useful stuff about the CPU we're running on.
* We don't care about everything, but just MMX, XMMS, SSE, SSE2, 3DNOW, 3DNOWEXT,
* for the obvious reasons.
* If we're generating for Pentium or above then assume CPUID is present; otherwise,
* test if CPUID is present first using the recommended code...
*/
FXuint fxcpuid(){
  FXuint eax, ecx, edx, caps;

  // Generating code for pentium:- don't bother checking for CPUID presence.
#if !(defined(__i586__) || defined(__i686__) || defined(__athlon__) || defined(__pentium4__))

  // If EFLAGS bit 21 can be changed, we have CPUID capability.
  asm volatile ("pushfl                 \n\t"
                "popl   %0              \n\t"
                "movl   %0,%1           \n\t"
                "xorl   $0x200000,%0    \n\t"
                "pushl  %0              \n\t"
                "popfl                  \n\t"
                "pushfl                 \n\t"
                "popl   %0              \n\t"
                : "=a" (eax),
                  "=d" (edx)
                :
                : "cc");

  // Yes, we have no CPUID!
  if(eax==edx) return 0;
#endif

  // Capabilities
  caps=0;

  // Get vendor string; this also returns the highest CPUID code in eax.
  // If highest CPUID code is zero, we can't call any other CPUID functions.
  cpuid(0x00000000, eax,ecx,edx);
  if(eax){

    // AMD:   ebx="Auth" edx="enti" ecx="cAMD",
    // Intel: ebx="Genu" edx="ineI" ecx="ntel"
    // VIAC3: ebx="Cent" edx="aurH" ecx="auls"

    // Test for AMD
    if((ecx==0x444d4163) && (edx==0x69746e65)){

      // Any extended capabilities; this returns highest extended CPUID code in eax.
      cpuid(0x80000000,eax,ecx,edx);
      if(eax>0x80000000){

        // Test extended athlon capabilities
        cpuid(0x80000001,eax,ecx,edx);
        if(edx&0x08000000) caps|=CPU_HAS_MMXEX;
        if(edx&0x80000000) caps|=CPU_HAS_3DNOW;
        if(edx&0x40000000) caps|=CPU_HAS_3DNOWEXT;
        }
      }

    // Standard CPUID code 1.
    cpuid(0x00000001,eax,ecx,edx);
    if(edx&0x00000010) caps|=CPU_HAS_TSC;
    if(edx&0x00800000) caps|=CPU_HAS_MMX;
    if(edx&0x02000000) caps|=CPU_HAS_SSE;
    if(edx&0x04000000) caps|=CPU_HAS_SSE2;
    }

  // Return capabilities
  return caps;
  }
#endif


/*
* Return clock ticks from x86 TSC register [GCC/ICC x86 version].
*/
#if (defined(__GNUC__) || defined(__ICC)) && defined(__linux__) && defined(__i386__)
extern FXAPI FXlong fxgetticks();
FXlong fxgetticks(){
  FXlong value;
  asm volatile ("rdtsc" : "=A" (value));
  return value;
  }
#endif


/*
* Return clock ticks from performance counter [GCC AMD64 version].
*/
#if defined(__GNUC__) && defined(__linux__) && defined(__x86_64__)
extern FXAPI FXlong fxgetticks();
FXlong fxgetticks(){
  register FXuint a,d;
  asm volatile("rdtsc" : "=a" (a), "=d" (d));
//asm volatile("rdtscp" : "=a" (a), "=d" (d): : "%ecx");        // Serializing version (%ecx has processor id)
  return ((FXulong)a) | (((FXulong)d)<<32);
  }
#endif


/*
* Return clock ticks from performance counter.
*/
#ifdef WIN32
extern FXAPI FXlong fxgetticks();
FXlong fxgetticks(){
  FXlong value;
  QueryPerformanceCounter((LARGE_INTEGER*)&value);
  return value;
  }
#endif

#if (defined(__GNUC__) &&  __GNUC__>=3) || defined(__DMC__)
// GNU symbol demangler
const FXString &fxdemanglesymbol(const FXString &rawsymbol, bool errorIfNotFound)
{
	static QMutex lock;
	static QDict<FXString> cache(13, true);
	QMtxHold h(lock);
	FXString *ret=cache.find(rawsymbol);
	if(ret) return *ret;
	FXERRHM(ret=new FXString);
	FXRBOp dealloc=FXRBNew(ret);
	int status=0;
	char *demangled=::abi::__cxa_demangle(rawsymbol.text(), 0, 0, &status);
	if(status!=0 || !demangled)
	{
		if(errorIfNotFound)
		{
			FXERRG("Failed to demangle symbol", 0, FXERRH_ISDEBUG);
		}
		else
			return rawsymbol;
	}
	FXRBOp demangledalloc=FXRBFunc(&::free, demangled);
	ret->assign(demangled);
	cache.insert(rawsymbol, ret);
	dealloc.dismiss();
	return *ret;
}
#else
const FXString &fxdemanglesymbol(const FXString &rawsymbol)
{
	return rawsymbol;
}
#endif


}
