/********************************************************************************
*                                                                               *
*                    W i d e   S t r i n g   O b j e c t                        *
*                                                                               *
*********************************************************************************
* Copyright (C) 1997,2004 by Jeroen van der Zijp.   All Rights Reserved.        *
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
* $Id: FXWString.cpp,v 1.12 2004/02/08 17:29:07 fox Exp $                       *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "FXStream.h"
#include "FXWString.h"


/*
  Notes:

  - How can we find the length of a null-terminated string of FXwchars
    quickly? See places where fxwstrlen() is used below.
  - Need Unicode-aware replacements for character classification
    functions like isspace(), tolower(), toupper(), etc.
*/


// The string buffer is always rounded to a multiple of ROUNDVAL
// which must be 2^n.  Thus, small size changes will not result in any
// actual resizing of the buffer except when ROUNDVAL is exceeded.
#define ROUNDVAL    16

// Round up to nearest ROUNDVAL
#define ROUNDUP(n)  (((n)+ROUNDVAL-1)&-ROUNDVAL)

// This will come in handy
#define EMPTY       ((FXwchar*)&emptystring[1])




/*******************************************************************************/

namespace FX {


// Empty string
static const FXint emptystring[2]={0,0};


// Special NULL string
const FXwchar FXWString::null[4]={0,0,0,0};



// Numbers for hexadecimal
const FXwchar FXWString::hex[17]={'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f',0};
const FXwchar FXWString::HEX[17]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F',0};


// This is a stopgap; need some faster way to determine the length of
// a null-terminated string of FXwchars!
inline size_t fxstrlen(const FXwchar* s){
  register size_t c=0;
  while(*s++) c++;
  return c;
  }

inline void fxmemset(FXwchar* dest,FXwchar c,FXint n){
  for(register FXint i=0; i<n; i++) dest[i]=c;
  }

inline void fxmemcpy(FXwchar* dest,const FXwchar* src,size_t n){
  memcpy((void*)dest,(const void*)src,n*sizeof(FXwchar));
  }

inline void fxmemmove(FXwchar* dest,const FXwchar* src,size_t n){
  memmove((void*)dest,(const void*)src,n*sizeof(FXwchar));
  }

inline FXwchar fxtoupper(FXwchar c){
  return toupper(c);    // FIXME need unicode flavor of this
  }

inline FXwchar fxtolower(FXwchar c){
  return tolower(c);    // FIXME need unicode flavor of this
  }

inline bool fxisspace(FXwchar c){
  return isspace(c)!=0;    // FIXME need unicode flavor of this
  }


// Change the length of the string to len
void FXWString::length(FXint len){
  if(((FXint*)str)[-1]!=len){
    if(0<len){
      if(str==EMPTY)
        str=sizeof(FXint)+(FXwchar*)malloc(ROUNDUP(sizeof(FXint)+1+len)*sizeof(FXwchar));
      else
        str=sizeof(FXint)+(FXwchar*)realloc(str-sizeof(FXint),ROUNDUP(sizeof(FXint)+1+len)*sizeof(FXwchar));
      str[len]=0;
      ((FXint*)str)[-1]=len;
      }
    else if(str!=EMPTY){
      free(str-sizeof(FXint));
      str=EMPTY;
      }
    }
  }


// Simple construct
FXWString::FXWString():str(EMPTY){
  }


// Copy construct
FXWString::FXWString(const FXWString& s):str(EMPTY){
  register FXint len=s.length();
  if(0<len){
    length(len);
    fxmemcpy(str,s.str,len);
    }
  }


// Construct and init
FXWString::FXWString(const FXwchar* s):str(EMPTY){
  if(s && s[0]){
    register FXint len=fxstrlen(s);
    length(len);
    fxmemcpy(str,s,len);
    }
  }


// Construct and init with substring
FXWString::FXWString(const FXwchar* s,FXint n):str(EMPTY){
  if(0<n){
    length(n);
    fxmemcpy(str,s,n);
    }
  }


// Construct and fill with constant
FXWString::FXWString(FXwchar c,FXint n):str(EMPTY){
  if(0<n){
    length(n);
    fxmemset(str,c,n);
    }
  }


// Construct string from two parts
FXWString::FXWString(const FXwchar *s1,const FXwchar* s2):str(EMPTY){
  register FXint len1=0,len2=0,len;
  if(s1 && s1[0]){ len1=fxstrlen(s1); }
  if(s2 && s2[0]){ len2=fxstrlen(s2); }
  len=len1+len2;
  str=EMPTY;
  if(len){
    length(len);
    fxmemcpy(str,s1,len1);
    fxmemcpy(&str[len1],s2,len2);
    }
  }


// Return partition of string separated by delimiter delim
FXWString FXWString::section(FXwchar delim,FXint start,FXint num) const {
  register FXint len=length(),s,e;
  s=0;
  if(0<start){
    while(s<len){
      ++s;
      if(str[s-1]==delim && --start==0) break;
      }
    }
  e=s;
  if(0<num){
    while(e<len){
      if(str[e]==delim && --num==0) break;
      ++e;
      }
    }
  return FXWString(&str[s],e-s);
  }


// Return partition of string separated by delimiters in delim
FXWString FXWString::section(const FXwchar* delim,FXint n,FXint start,FXint num) const {
  register FXint len=length(),s,e,i;
  register FXwchar c;
  s=0;
  if(0<start){
    while(s<len){
      c=str[s++];
      i=n;
      while(--i>=0){ if(delim[i]==c && --start==0) goto a; }
      }
    }
a:e=s;
  if(0<num){
    while(e<len){
      c=str[e];
      i=n;
      while(--i>=0){ if(delim[i]==c && --num==0) goto b; }
      ++e;
      }
    }
b:return FXWString(&str[s],e-s);
  }


// Return partition of string separated by delimiters in delim
FXWString FXWString::section(const FXwchar* delim,FXint start,FXint num) const {
  return section(delim,fxstrlen(delim),start,num);
  }


// Return partition of string separated by delimiters in delim
FXWString FXWString::section(const FXWString& delim,FXint start,FXint num) const {
  return section(delim.text(),delim.length(),start,num);
  }


// Assignment
FXWString& FXWString::operator=(const FXWString& s){
  if(str!=s.str){
    register FXint len=s.length();
    if(0<len){
      length(len);
      fxmemcpy(str,s.str,len);
      }
    else{
      length(0);
      }
    }
  return *this;
  }


// Assign a string
FXWString& FXWString::operator=(const FXwchar* s){
  if(str!=s){
    if(s && s[0]){
      register FXint len=fxstrlen(s);
      length(len);
      fxmemcpy(str,s,len);
      }
    else{
      length(0);
      }
    }
  return *this;
  }


// Concatenate two FXWStrings
FXWString operator+(const FXWString& s1,const FXWString& s2){
  return FXWString(s1.str,s2.str);
  }


// Concatenate FXWString and string
FXWString operator+(const FXWString& s1,const FXwchar* s2){
  return FXWString(s1.str,s2);
  }


// Concatenate string and FXWString
FXWString operator+(const FXwchar* s1,const FXWString& s2){
  return FXWString(s1,s2.str);
  }


// Concatenate FXWString and character
FXWString operator+(const FXWString& s,FXwchar c){
  FXwchar string[2];
  string[0]=c;
  string[1]=0;
  return FXWString(s.str,string);
  }


// Concatenate character and FXWString
FXWString operator+(FXwchar c,const FXWString& s){
  FXwchar string[2];
  string[0]=c;
  string[1]=0;
  return FXWString(string,s.str);
  }


// Fill with a constant
FXWString& FXWString::fill(FXwchar c,FXint n){
  length(n);
  fxmemset(str,c,n);
  return *this;
  }


// Fill up to current length
FXWString& FXWString::fill(FXwchar c){
  fxmemset(str,c,length());
  return *this;
  }


// Assign input character to this string
FXWString& FXWString::assign(FXwchar c){
  length(1);
  str[0]=c;
  return *this;
  }


// Assign input n characters c to this string
FXWString& FXWString::assign(FXwchar c,FXint n){
  length(n);
  fxmemset(str,c,n);
  return *this;
  }


// Assign first n characters of input string to this string
FXWString& FXWString::assign(const FXwchar *s,FXint n){
  if(str!=s){
    length(n);
    fxmemcpy(str,s,n);
    }
  return *this;
  }


// Assign input string to this string
FXWString& FXWString::assign(const FXWString& s){
  assign(s.str,s.length());
  return *this;
  }


// Assign input string to this string
FXWString& FXWString::assign(const FXwchar *s){
  assign(s,fxstrlen(s));
  return *this;
  }


// Insert character at position
FXWString& FXWString::insert(FXint pos,FXwchar c){
  register FXint len=length();
  length(len+1);
  if(pos<=0){
    fxmemmove(&str[1],str,len+1);
    str[0]=c;
    }
  else if(pos>=len){
    str[len]=c;
    }
  else{
    fxmemmove(&str[pos+1],&str[pos],len-pos+1);
    str[pos]=c;
    }
  return *this;
  }


// Insert n characters c at specified position
FXWString& FXWString::insert(FXint pos,FXwchar c,FXint n){
  if(0<n){
    register FXint len=length();
    length(len+n);
    if(pos<=0){
      fxmemmove(&str[n],str,len);
      fxmemset(str,c,n);
      }
    else if(pos>=len){
      fxmemset(&str[len],c,n);
      }
    else{
      fxmemmove(&str[pos+n],&str[pos],len-pos);
      fxmemset(&str[pos],c,n);
      }
    }
  return *this;
  }



// Insert string at position
FXWString& FXWString::insert(FXint pos,const FXwchar* s,FXint n){
  if(0<n){
    register FXint len=length();
    length(len+n);
    if(pos<=0){
      fxmemmove(&str[n],str,len);
      fxmemcpy(str,s,n);
      }
    else if(pos>=len){
      fxmemcpy(&str[len],s,n);
      }
    else{
      fxmemmove(&str[pos+n],&str[pos],len-pos);
      fxmemcpy(&str[pos],s,n);
      }
    }
  return *this;
  }


// Insert string at position
FXWString& FXWString::insert(FXint pos,const FXWString& s){
  insert(pos,s.str,s.length());
  return *this;
  }


// Insert string at position
FXWString& FXWString::insert(FXint pos,const FXwchar* s){
  insert(pos,s,fxstrlen(s));
  return *this;
  }


// Add character to the end
FXWString& FXWString::append(FXwchar c){
  register FXint len=length();
  length(len+1);
  str[len]=c;
  return *this;
  }


// Append input n characters c to this string
FXWString& FXWString::append(FXwchar c,FXint n){
  if(0<n){
    register FXint len=length();
    length(len+n);
    fxmemset(&str[len],c,n);
    }
  return *this;
  }


// Add string to the end
FXWString& FXWString::append(const FXwchar *s,FXint n){
  if(0<n){
    register FXint len=length();
    length(len+n);
    fxmemcpy(&str[len],s,n);
    }
  return *this;
  }


// Add string to the end
FXWString& FXWString::append(const FXWString& s){
  append(s.str,s.length());
  return *this;
  }


// Add string to the end
FXWString& FXWString::append(const FXwchar *s){
  append(s,fxstrlen(s));
  return *this;
  }


// Append FXWString
FXWString& FXWString::operator+=(const FXWString& s){
  append(s.str,s.length());
  return *this;
  }


// Append string
FXWString& FXWString::operator+=(const FXwchar* s){
  append(s,fxstrlen(s));
  return *this;
  }


// Append character
FXWString& FXWString::operator+=(FXwchar c){
  append(c);
  return *this;
  }


// Prepend character
FXWString& FXWString::prepend(FXwchar c){
  register FXint len=length();
  length(len+1);
  fxmemmove(&str[1],str,len);
  str[0]=c;
  return *this;
  }


// Prepend string
FXWString& FXWString::prepend(const FXwchar *s,FXint n){
  if(0<n){
    register FXint len=length();
    length(len+n);
    fxmemmove(&str[n],str,len);
    fxmemcpy(str,s,n);
    }
  return *this;
  }


// Prepend string with n characters c
FXWString& FXWString::prepend(FXwchar c,FXint n){
  if(0<n){
    register FXint len=length();
    length(len+n);
    fxmemmove(&str[n],str,len);
    fxmemset(str,c,n);
    }
  return *this;
  }


// Prepend string
FXWString& FXWString::prepend(const FXWString& s){
  prepend(s.str,s.length());
  return *this;
  }


// Prepend string
FXWString& FXWString::prepend(const FXwchar *s){
  prepend(s,fxstrlen(s));
  return *this;
  }


// Replace character in string
FXWString& FXWString::replace(FXint pos,FXwchar c){
  register FXint len=length();
  if(pos<0){
    length(len+1);
    fxmemmove(&str[1],str,len);
    str[0]=c;
    }
  else if(pos>=len){
    length(len+1);
    str[len]=c;
    }
  else{
    str[pos]=c;
    }
  return *this;
  }


// Replace the m characters at pos with n characters c
FXWString& FXWString::replace(FXint pos,FXint m,FXwchar c,FXint n){
  register FXint len=length();
  if(pos+m<=0){
    if(0<n){
      length(len+n);
      fxmemmove(&str[pos+n],str,len);
      fxmemset(str,c,n);
      }
    }
  else if(len<=pos){
    if(0<n){
      length(len+n);
      fxmemset(&str[len],c,n);
      }
    }
  else{
    if(pos<0){m+=pos;pos=0;}
    if(pos+m>len){m=len-pos;}
    if(m<n){
      length(len-m+n);
      fxmemmove(&str[pos+n],&str[pos+m],len-pos-m);
      }
    else if(m>n){
      fxmemmove(&str[pos+n],&str[pos+m],len-pos-m);
      length(len-m+n);
      }
    if(0<n){
      fxmemset(&str[pos],c,n);
      }
    }
  return *this;
  }


// Replace part of string
FXWString& FXWString::replace(FXint pos,FXint m,const FXwchar *s,FXint n){
  register FXint len=length();
  if(pos+m<=0){
    if(0<n){
      length(len+n);
      fxmemmove(&str[pos+n],str,len);
      fxmemcpy(str,s,n);
      }
    }
  else if(len<=pos){
    if(0<n){
      length(len+n);
      fxmemcpy(&str[len],s,n);
      }
    }
  else{
    if(pos<0){m+=pos;pos=0;}
    if(pos+m>len){m=len-pos;}
    if(m<n){
      length(len-m+n);
      fxmemmove(&str[pos+n],&str[pos+m],len-pos-m);
      }
    else if(m>n){
      fxmemmove(&str[pos+n],&str[pos+m],len-pos-m);
      length(len-m+n);
      }
    if(0<n){
      fxmemcpy(&str[pos],s,n);
      }
    }
  return *this;
  }


// Replace part of string
FXWString& FXWString::replace(FXint pos,FXint m,const FXWString& s){
  replace(pos,m,s.str,s.length());
  return *this;
  }


// Replace part of string
FXWString& FXWString::replace(FXint pos,FXint m,const FXwchar *s){
  replace(pos,m,s,fxstrlen(s));
  return *this;
  }


// Remove section from buffer
FXWString& FXWString::remove(FXint pos,FXint n){
  if(0<n){
    register FXint len=length();
    if(pos<len && pos+n>0){
      if(pos<0){n+=pos;pos=0;}
      if(pos+n>len){n=len-pos;}
      fxmemmove(&str[pos],&str[pos+n],len-n-pos);
      length(len-n);
      }
    }
  return *this;
  }


// Return number of occurrences of ch in string
FXint FXWString::contains(FXwchar ch){
  register FXint len=length();
  register FXwchar c=ch;
  register FXint m=0;
  register FXint i=0;
  while(i<len){
    if(str[i]==c){
      m++;
      }
    i++;
    }
  return m;
  }


// Return number of occurrences of string sub in string
FXint FXWString::contains(const FXwchar* sub,FXint n){
  register FXint len=length()-n;
  register FXint m=0;
  register FXint i=0;
  while(i<=len){
    if(compare(&str[i],sub,n)==0){
      m++;
      }
    i++;
    }
  return m;
  }


// Return number of occurrences of string sub in string
FXint FXWString::contains(const FXwchar* sub){
  return contains(sub,fxstrlen(sub));
  }


// Return number of occurrences of string sub in string
FXint FXWString::contains(const FXWString& sub){
  return contains(sub.text(),sub.length());
  }


// Substitute one character by another
FXWString& FXWString::substitute(FXwchar org,FXwchar sub,FXbool all){
  register FXint len=length();
  register FXwchar c=org;
  register FXwchar s=sub;
  register FXint i=0;
  while(i<len){
    if(str[i]==c){
      str[i]=s;
      if(!all) break;
      }
    i++;
    }
  return *this;
  }


// Substitute one string by another
FXWString& FXWString::substitute(const FXwchar* org,FXint olen,const FXwchar* rep,FXint rlen,FXbool all){
  if(0<olen){
    register FXint pos=0;
    while(pos<=length()-olen){
      if(compare(&str[pos],org,olen)==0){
        replace(pos,olen,rep,rlen);
        if(!all) break;
        pos+=rlen;
        continue;
        }
      pos++;
      }
    }
  return *this;
  }


// Substitute one string by another
FXWString& FXWString::substitute(const FXwchar* org,const FXwchar* rep,FXbool all){
  return substitute(org,fxstrlen(org),rep,fxstrlen(rep),all);
  }


// Substitute one string by another
FXWString& FXWString::substitute(const FXWString& org,const FXWString& rep,FXbool all){
  return substitute(org.text(),org.length(),rep.text(),rep.length(),all);
  }


// Simplify whitespace in string
FXWString& FXWString::simplify(){
  if(str!=EMPTY){
    register FXint s=0;
    register FXint d=0;
    register FXint e=length();
    while(s<e && fxisspace((FXuchar)str[s])) s++;
    while(1){
      while(s<e && !fxisspace((FXuchar)str[s])) str[d++]=str[s++];
      while(s<e && fxisspace((FXuchar)str[s])) s++;
      if(s>=e) break;
      str[d++]=' ';
      }
    length(d);
    }
  return *this;
  }


// Remove leading and trailing whitespace
FXWString& FXWString::trim(){
  if(str!=EMPTY){
    register FXint s=0;
    register FXint e=length();
    while(0<e && fxisspace(str[e-1])) e--;
    while(s<e && fxisspace(str[s])) s++;
    fxmemmove(str,&str[s],e-s);
    length(e-s);
    }
  return *this;
  }


// Remove leading whitespace
FXWString& FXWString::trimBegin(){
  if(str!=EMPTY){
    register FXint s=0;
    register FXint e=length();
    while(s<e && fxisspace(str[s])) s++;
    fxmemmove(str,&str[s],e-s);
    length(e-s);
    }
  return *this;
  }


// Remove trailing whitespace
FXWString& FXWString::trimEnd(){
  if(str!=EMPTY){
    register FXint e=length();
    while(0<e && fxisspace(str[e-1])) e--;
    length(e);
    }
  return *this;
  }


// Truncate string
FXWString& FXWString::trunc(FXint pos){
  register FXint len=length();
  if(pos>len) pos=len;
  length(pos);
  return *this;
  }


// Clean string
FXWString& FXWString::clear(){
  length(0);
  return *this;
  }


// Get leftmost part
FXWString FXWString::left(FXint n) const {
  if(0<n){
    register FXint len=length();
    if(n>len) n=len;
    return FXWString(str,n);
    }
  return FXWString::null;
  }


// Get rightmost part
FXWString FXWString::right(FXint n) const {
  if(0<n){
    register FXint len=length();
    if(n>len) n=len;
    return FXWString(str+len-n,n);
    }
  return FXWString::null;
  }


// Get some part in the middle
FXWString FXWString::mid(FXint pos,FXint n) const {
  if(0<n){
    register FXint len=length();
    if(pos<len && pos+n>0){
      if(pos<0){n+=pos;pos=0;}
      if(pos+n>len){n=len-pos;}
      return FXWString(str+pos,n);
      }
    }
  return FXWString::null;
  }


// Return all characters before the nth occurrence of ch, searching forward
FXWString FXWString::before(FXwchar c,FXint n) const {
  register FXint len=length();
  register FXint p=0;
  if(0<n){
    while(p<len){
      if(str[p]==c && --n==0) break;
      p++;
      }
    }
  return FXWString(str,p);
  }


// Return all characters before the nth occurrence of ch, searching backward
FXWString FXWString::rbefore(FXwchar c,FXint n) const {
  register FXint p=length();
  if(0<n){
    while(0<p){
      p--;
      if(str[p]==c && --n==0) break;
      }
    }
  return FXWString(str,p);
  }


// Return all characters after the nth occurrence of ch, searching forward
FXWString FXWString::after(FXwchar c,FXint n) const {
  register FXint len=length();
  register FXint p=0;
  if(0<n){
    while(p<len){
      p++;
      if(str[p-1]==c && --n==0) break;
      }
    }
  return FXWString(&str[p],len-p);
  }


// Return all characters after the nth occurrence of ch, searching backward
FXWString FXWString::rafter(FXwchar c,FXint n) const {
  register FXint len=length();
  register FXint p=len;
  if(0<n){
    while(0<p){
      if(str[p-1]==c && --n==0) break;
      p--;
      }
    }
  return FXWString(&str[p],len-p);
  }


// Convert to lower case
FXWString& FXWString::lower(){
  register FXint len=length();
  for(register FXint i=0; i<len; i++){
    str[i]=fxtolower(str[i]);
    }
  return *this;
  }


// Convert to upper case
FXWString& FXWString::upper(){
  register FXint len=length();
  for(register FXint i=0; i<len; i++){
    str[i]=fxtoupper(str[i]);
    }
  return *this;
  }


// Compare strings
FXint compare(const FXwchar *s1,const FXwchar *s2){     // FIXME collating ?
  register FXwchar c1,c2;
  do{
    c1=*s1++;
    c2=*s2++;
    }
  while(c1 && (c1==c2));
  return c1-c2;
  }

FXint compare(const FXwchar *s1,const FXWString &s2){
  return compare(s1,s2.str);
  }

FXint compare(const FXWString &s1,const FXwchar *s2){
  return compare(s1.str,s2);
  }

FXint compare(const FXWString &s1,const FXWString &s2){
  return compare(s1.str,s2.str);
  }


// Compare strings up to n
FXint compare(const FXwchar *s1,const FXwchar *s2,FXint n){     // FIXME collating ?
  register const FXwchar *p1=s1;
  register const FXwchar *p2=s2;
  register FXwchar c1,c2;
  if(0<n){
    do{
      c1=*p1++;
      c2=*p2++;
      }
    while(--n && c1 && (c1==c2));
    return c1-c2;
    }
  return 0;
  }

FXint compare(const FXwchar *s1,const FXWString &s2,FXint n){
  return compare(s1,s2.str,n);
  }

FXint compare(const FXWString &s1,const FXwchar *s2,FXint n){
  return compare(s1.str,s2,n);
  }

FXint compare(const FXWString &s1,const FXWString &s2,FXint n){
  return compare(s1.str,s2.str,n);
  }


// Compare strings case insensitive
FXint comparecase(const FXwchar *s1,const FXwchar *s2){ // FIXME collating ?
  register const FXwchar *p1=s1;
  register const FXwchar *p2=s2;
  register FXwchar c1,c2;
  do{
    c1=fxtolower(*p1++);
    c2=fxtolower(*p2++);
    }
  while(c1 && (c1==c2));
  return c1-c2;
  }

FXint comparecase(const FXwchar *s1,const FXWString &s2){
  return comparecase(s1,s2.str);
  }

FXint comparecase(const FXWString &s1,const FXwchar *s2){
  return comparecase(s1.str,s2);
  }

FXint comparecase(const FXWString &s1,const FXWString &s2){
  return comparecase(s1.str,s2.str);
  }


// Compare strings case insensitive up to n
FXint comparecase(const FXwchar *s1,const FXwchar *s2,FXint n){ // FIXME collating ?
  register const FXwchar *p1=s1;
  register const FXwchar *p2=s2;
  register FXwchar c1,c2;
  if(0<n){
    do{
      c1=fxtolower(*p1++);
      c2=fxtolower(*p2++);
      }
    while(--n && c1 && (c1==c2));
    return c1-c2;
    }
  return 0;
  }

FXint comparecase(const FXwchar *s1,const FXWString &s2,FXint n){
  return comparecase(s1,s2.str,n);
  }

FXint comparecase(const FXWString &s1,const FXwchar *s2,FXint n){
  return comparecase(s1.str,s2,n);
  }

FXint comparecase(const FXWString &s1,const FXWString &s2,FXint n){
  return comparecase(s1.str,s2.str,n);
  }

/// Comparison operators
FXbool operator==(const FXWString &s1,const FXWString &s2){
  return compare(s1.str,s2.str)==0;
  }

FXbool operator==(const FXWString &s1,const FXwchar *s2){
  return compare(s1.str,s2)==0;
  }

FXbool operator==(const FXwchar *s1,const FXWString &s2){
  return compare(s1,s2.str)==0;
  }

FXbool operator!=(const FXWString &s1,const FXWString &s2){
  return compare(s1.str,s2.str)!=0;
  }

FXbool operator!=(const FXWString &s1,const FXwchar *s2){
  return compare(s1.str,s2)!=0;
  }

FXbool operator!=(const FXwchar *s1,const FXWString &s2){
  return compare(s1,s2.str)!=0;
  }

FXbool operator<(const FXWString &s1,const FXWString &s2){
  return compare(s1.str,s2.str)<0;
  }

FXbool operator<(const FXWString &s1,const FXwchar *s2){
  return compare(s1.str,s2)<0;
  }

FXbool operator<(const FXwchar *s1,const FXWString &s2){
  return compare(s1,s2.str)<0;
  }

FXbool operator<=(const FXWString &s1,const FXWString &s2){
  return compare(s1.str,s2.str)<=0;
  }

FXbool operator<=(const FXWString &s1,const FXwchar *s2){
  return compare(s1.str,s2)<=0;
  }

FXbool operator<=(const FXwchar *s1,const FXWString &s2){
  return compare(s1,s2.str)<=0;
  }

FXbool operator>(const FXWString &s1,const FXWString &s2){
  return compare(s1.str,s2.str)>0;
  }

FXbool operator>(const FXWString &s1,const FXwchar *s2){
  return compare(s1.str,s2)>0;
  }

FXbool operator>(const FXwchar *s1,const FXWString &s2){
  return compare(s1,s2.str)>0;
  }

FXbool operator>=(const FXWString &s1,const FXWString &s2){
  return compare(s1.str,s2.str)>=0;
  }

FXbool operator>=(const FXWString &s1,const FXwchar *s2){
  return compare(s1.str,s2)>=0;
  }

FXbool operator>=(const FXwchar *s1,const FXWString &s2){
  return compare(s1,s2.str)>=0;
  }


// Find n-th occurrence of character, searching forward; return position or -1
FXint FXWString::find(FXwchar c,FXint pos,FXint n) const {
  register FXint len=length();
  register FXint p=pos;
  register FXwchar cc=c;
  if(p<0) p=0;
  if(n<=0) return p;
  while(p<len){
    if(str[p]==cc){ if(--n==0) return p; }
    ++p;
    }
  return -1;
  }


// Find n-th occurrence of character, searching backward; return position or -1
FXint FXWString::rfind(FXwchar c,FXint pos,FXint n) const {
  register FXint len=length();
  register FXint p=pos;
  register FXwchar cc=c;
  if(p>=len) p=len-1;
  if(n<=0) return p;
  while(0<=p){
    if(str[p]==cc){ if(--n==0) return p; }
    --p;
    }
  return -1;
  }


// Find a character, searching forward; return position or -1
FXint FXWString::find(FXwchar c,FXint pos) const {
  register FXint len=length();
  register FXint p=pos;
  register FXwchar cc=c;
  if(p<0) p=0;
  while(p<len){ if(str[p]==cc){ return p; } ++p; }
  return -1;
  }


// Find a character, searching backward; return position or -1
FXint FXWString::rfind(FXwchar c,FXint pos) const {
  register FXint len=length();
  register FXint p=pos;
  register FXwchar cc=c;
  if(p>=len) p=len-1;
  while(0<=p){ if(str[p]==cc){ return p; } --p; }
  return -1;
  }


// Find a substring of length n, searching forward; return position or -1
FXint FXWString::find(const FXwchar* substr,FXint n,FXint pos) const {
  register FXint len=length();
  if(0<=pos && 0<n && n<=len){
    register FXwchar c=substr[0];
    len=len-n+1;
    while(pos<len){
      if(str[pos]==c){
        if(!compare(str+pos,substr,n)){
          return pos;
          }
        }
      pos++;
      }
    }
  return -1;
  }


// Find a substring, searching forward; return position or -1
FXint FXWString::find(const FXwchar* substr,FXint pos) const {
  return find(substr,fxstrlen(substr),pos);
  }


// Find a substring, searching forward; return position or -1
FXint FXWString::find(const FXWString &substr,FXint pos) const {
  return find(substr.text(),substr.length(),pos);
  }


// Find a substring of length n, searching backward; return position or -1
FXint FXWString::rfind(const FXwchar* substr,FXint n,FXint pos) const {
  register FXint len=length();
  if(0<=pos && 0<n && n<=len){
    register FXwchar c=substr[0];
    len-=n;
    if(pos>len) pos=len;
    while(0<=pos){
      if(str[pos]==c){
        if(!compare(str+pos,substr,n)){
          return pos;
          }
        }
      pos--;
      }
    }
  return -1;
  }


// Find a substring, searching backward; return position or -1
FXint FXWString::rfind(const FXwchar* substr,FXint pos) const {
  return rfind(substr,fxstrlen(substr),pos);
  }


// Find a substring, searching backward; return position or -1
FXint FXWString::rfind(const FXWString &substr,FXint pos) const {
  return rfind(substr.text(),substr.length(),pos);
  }


// Find first character in the set of size n, starting from pos; return position or -1
FXint FXWString::find_first_of(const FXwchar *set,FXint n,FXint pos) const {
  register FXint len=length();
  register FXint p=pos;
  if(p<0) p=0;
  while(p<len){
    register FXwchar c=str[p];
    register FXint i=n;
    while(--i>=0){ if(set[i]==c) return p; }
    p++;
    }
  return -1;
  }


// Find first character in the set, starting from pos; return position or -1
FXint FXWString::find_first_of(const FXwchar *set,FXint pos) const {
  return find_first_of(set,fxstrlen(set),pos);
  }


// Find first character in the set, starting from pos; return position or -1
FXint FXWString::find_first_of(const FXWString &set,FXint pos) const {
  return find_first_of(set.text(),set.length(),pos);
  }


// Find first character, starting from pos; return position or -1
FXint FXWString::find_first_of(FXwchar c,FXint pos) const {
  register FXint len=length();
  register FXint p=pos;
  register FXwchar cc=c;
  if(p<0) p=0;
  while(p<len){ if(str[p]==cc){ return p; } p++; }
  return -1;
  }


// Find last character in the set of size n, starting from pos; return position or -1
FXint FXWString::find_last_of(const FXwchar *set,FXint n,FXint pos) const {
  register FXint len=length();
  register FXint p=pos;
  if(p>=len) p=len-1;
  while(0<=p){
    register FXwchar c=str[p];
    register FXint i=n;
    while(--i>=0){ if(set[i]==c) return p; }
    p--;
    }
  return -1;
  }


// Find last character in the set, starting from pos; return position or -1
FXint FXWString::find_last_of(const FXwchar *set,FXint pos) const {
  return find_last_of(set,fxstrlen(set),pos);
  }


// Find last character in the set, starting from pos; return position or -1
FXint FXWString::find_last_of(const FXWString &set,FXint pos) const {
  return find_last_of(set.text(),set.length(),pos);
  }


// Find last character, starting from pos; return position or -1
FXint FXWString::find_last_of(FXwchar c,FXint pos) const {
  register FXint len=length();
  register FXint p=pos;
  register FXwchar cc=c;
  if(p>=len) p=len-1;
  while(0<=p){ if(str[p]==cc){ return p; } p--; }
  return -1;
  }



// Find first character NOT in the set of size n, starting from pos; return position or -1
FXint FXWString::find_first_not_of(const FXwchar *set,FXint n,FXint pos) const {
  register FXint len=length();
  register FXint p=pos;
  if(p<0) p=0;
  while(p<len){
    register FXwchar c=str[p];
    register FXint i=n;
    while(--i>=0){ if(set[i]==c) goto x; }
    return p;
x:  p++;
    }
  return -1;
  }


// Find first character NOT in the set, starting from pos; return position or -1
FXint FXWString::find_first_not_of(const FXwchar *set,FXint pos) const {
  return find_first_not_of(set,fxstrlen(set),pos);
  }


// Find first character NOT in the set, starting from pos; return position or -1
FXint FXWString::find_first_not_of(const FXWString &set,FXint pos) const {
  return find_first_not_of(set.text(),set.length(),pos);
  }


// Find first character NOT equal to c, starting from pos; return position or -1
FXint FXWString::find_first_not_of(FXwchar c,FXint pos) const {
  register FXint len=length();
  register FXint p=pos;
  register FXwchar cc=c;
  if(p<0) p=0;
  while(p<len){ if(str[p]!=cc){ return p; } p++; }
  return -1;
  }



// Find last character NOT in the set of size n, starting from pos; return position or -1
FXint FXWString::find_last_not_of(const FXwchar *set,FXint n,FXint pos) const {
  register FXint len=length();
  register FXint p=pos;
  if(p>=len) p=len-1;
  while(0<=p){
    register FXwchar c=str[p];
    register FXint i=n;
    while(--i>=0){ if(set[i]==c) goto x; }
    return p;
x:  p--;
    }
  return -1;
  }

// Find last character NOT in the set, starting from pos; return position or -1
FXint FXWString::find_last_not_of(const FXwchar *set,FXint pos) const {
  return find_last_not_of(set,fxstrlen(set),pos);
  }

// Find last character NOT in the set, starting from pos; return position or -1
FXint FXWString::find_last_not_of(const FXWString &set,FXint pos) const {
  return find_last_not_of(set.text(),set.length(),pos);
  }


// Find last character NOT equal to c, starting from pos; return position or -1
FXint FXWString::find_last_not_of(FXwchar c,FXint pos) const {
  register FXint len=length();
  register FXint p=pos;
  register FXwchar cc=c;
  if(p>=len) p=len-1;
  while(0<=p){ if(str[p]!=cc){ return p; } p--; }
  return -1;
  }


// Find number of occurrences of character in string
FXint FXWString::count(FXwchar c) const {
  register FXint len=length();
  register FXwchar cc=c;
  register FXint n=0;
  for(register FXint i=0; i<len; i++){
    n+=(str[i]==cc);
    }
  return n;
  }


// Get hash value
FXuint FXWString::hash() const {
  register const FXwchar *s=str;
  register FXuint h=0;
  register FXwchar c;                // This should be a very good hash function:- just 4 collisions
  while((c=*s++)!=0){                // on the webster web2 dictionary of 234936 words, and no
    h = ((h << 5) + h) ^ c;          // collisions at all on the standard dict!
    }
  return h;
  }


// Save
FXStream& operator<<(FXStream& store,const FXWString& s){
  FXint len=s.length();
  store << len;
  store.save(s.str,len);
  return store;
  }


// Load
FXStream& operator>>(FXStream& store,FXWString& s){
  FXint len;
  store >> len;
  s.length(len);
  store.load(s.str,len);
  return store;
  }


// Delete
FXWString::~FXWString(){
  length(0);
  }

}


