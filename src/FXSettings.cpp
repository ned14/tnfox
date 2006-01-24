/********************************************************************************
*                                                                               *
*                           S e t t i n g s   C l a s s                         *
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
* $Id: FXSettings.cpp,v 1.33.2.3 2005/11/05 11:12:03 fox Exp $                      *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXStringDict.h"
#include "FXSettings.h"
#include "FXApp.h"

/*
  Notes:

  - Format for settings database file:

    [Section Key]
    EntryKey=string-with-no-spaces
    EntryKey="string\nwith a\nnewline in it\n"
    EntryKey=" string with leading and trailing spaces and \"embedded\" in it  "
    EntryKey=string with no leading or trailing spaces

  - EntryKey may is of the form "ali baba", "ali-baba", "ali_baba", or "ali.baba".

  - Leading/trailing spaces are NOT part of the EntryKey.

  - FXSectionDict should go; FXSettings should simply derive from FXDict.

  - Escape sequences now allow octal (\377) as well as hex (\xff) codes.

  - EntryKey format should be like values.
*/

#define MAXBUFFER 2000
#define MAXNAME   200
#define MAXVALUE  2000



/*******************************************************************************/

namespace FX {

// Object implementation
FXIMPLEMENT(FXSettings,FXDict,NULL,0)


// Make registry object
FXSettings::FXSettings(){
  modified=FALSE;
  }


// Create data
void *FXSettings::createData(const void*){
  return new FXStringDict;
  }


// Delete data
void FXSettings::deleteData(void* ptr){
  delete ((FXStringDict*)ptr);
  }


// Parse filename
FXbool FXSettings::parseFile(const FXString& filename,FXbool mark){
  FXLockHold applock(FXApp::instance());
  FXchar buffer[MAXBUFFER],value[MAXVALUE];
  register FXStringDict *group=NULL;
  register FXchar *name,*ptr,*p;
  register FXint lineno=1;
  FILE *file;
  file=fopen(filename.text(),"r");
  if(file){
    FXTRACE((100,"Reading settings file: %s\n",filename.text()));

    // Parse one line at a time
    while(fgets(buffer,MAXBUFFER,file)!=NULL){

      // Parse buffer
      ptr=buffer;

      // Skip leading spaces
      while(*ptr && isspace((FXuchar)*ptr)) ptr++;

      // Test for comments
      if(*ptr=='#' || *ptr==';' || *ptr=='\0') goto next;

      // Parse section name
      if(*ptr=='['){
        for(name=++ptr; *ptr && *ptr!=']'; ptr++){
          if((FXuchar)*ptr<' '){
            fxwarning("%s:%d: illegal section name.\n",filename.text(),lineno);
            goto next;
            }
          }

        // End
        *ptr='\0';

        // Add new section dict
        group=insert(name);
        }

      // Parse key name
      else{

        // Should have a group
        if(!group){
          fxwarning("%s:%d: settings entry should follow a section.\n",filename.text(),lineno);
          goto next;
          }

        // Transfer key, checking validity
        for(name=ptr; *ptr && *ptr!='='; ptr++){
          if((FXuchar)*ptr<' '){
            fxwarning("%s:%d: illegal key name.\n",filename.text(),lineno);
            goto next;
            }
          }

        // Should be a '='
        if(*ptr!='='){
          fxwarning("%s:%d: expected '=' to follow key.\n",filename.text(),lineno);
          goto next;
          }

        // Remove trailing spaces
        for(p=ptr; name<p && *(p-1)==' '; p--);

        // End
        *p='\0';

        ptr++;

        // Skip more spaces
        while(*ptr && isspace((FXuchar)*ptr)) ptr++;

        // Parse value
        if(!parseValue(value,ptr)){
          fxwarning("%s:%d: error parsing value.\n",filename.text(),lineno);
          goto next;
          }

        // Add entry to current section
        group->replace(name,value,mark);
        }

      // Next line
next: lineno++;
      }

    // Done
    fclose(file);
    return TRUE;
    }
  return FALSE;
  }


// Parse value
FXbool FXSettings::parseValue(FXchar* value,const FXchar* buffer){
  register const FXchar *ptr=buffer;
  register FXchar *out=value;
  register FXuint v,c;

  // Was quoted string; copy verbatim
  if(*ptr=='"'){
    ptr++;
    while(*ptr){
      switch(*ptr){
        case '\\':
          ptr++;
          switch(*ptr){
            case 'n':
              *out++='\n';
              break;
            case 'r':
              *out++='\r';
              break;
            case 'b':
              *out++='\b';
              break;
            case 'v':
              *out++='\v';
              break;
            case 'a':
              *out++='\a';
              break;
            case 'f':
              *out++='\f';
              break;
            case 't':
              *out++='\t';
              break;
            case '\\':
              *out++='\\';
              break;
            case '"':
              *out++='"';
              break;
            case '\'':
              *out++='\'';
              break;
            case 'x':
              v='x';
              if(isxdigit((FXuchar)*(ptr+1))){
                c=*++ptr;
                v=('a'<=c)?(c-'a'+10):('A'<=c)?(c-'A'+10):(c-'0');
                if(isxdigit((FXuchar)*(ptr+1))){
                  c=*++ptr;
                  v=(v<<4)+(('a'<=c)?(c-'a'+10):('A'<=c)?(c-'A'+10):(c-'0'));
                  }
                }
              *out++=v;
              break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
              v=*ptr-'0';
              if('0'<=*(ptr+1) && *(ptr+1)<='7'){
                v=v*8+*++ptr-'0';
                if('0'<=*(ptr+1) && *(ptr+1)<='7'){
                  v=v*8+*++ptr-'0';
                  }
                }
              *out++=v;
            default:
              *out++=*ptr;
              break;
            }
          break;
        case '"':
          *out='\0';
          return TRUE;
        default:
          *out++=*ptr;
          break;
        }
      ptr++;
      }
    *value='\0';
    return FALSE;
    }

  // String not starting or ending with spaces
  else{

    // Copy as much as we can
    while(*ptr && isprint((FXuchar)*ptr)){
      *out++=*ptr++;
      }

    // Strip spaces at the end
    while(value<out && *(out-1)==' ') --out;

    // Terminate
    *out='\0';
    }
  return TRUE;
  }



// Unparse registry file
FXbool FXSettings::unparseFile(const FXString& filename){
  FXLockHold applock(FXApp::instance());
  FXchar buffer[MAXVALUE];
  FXStringDict *group;
  FILE *file;
  FXint s,e;
  FXbool sec,mrk;
  file=fopen(filename.text(),"w");
  if(file){
    FXTRACE((100,"Writing settings file: %s\n",filename.text()));

    // Loop over all sections
    for(s=first(); s<size(); s=next(s)){

      // Get group
      group=data(s);
      FXASSERT(group);
      sec=FALSE;

      // Loop over all entries
      for(e=group->first(); e<group->size(); e=group->next(e)){
        mrk=group->mark(e);

        // Write section name if not written yet
        if(mrk && !sec){
          FXASSERT(key(s));
          fputc('[',file);
          fputs(key(s),file);
          fputc(']',file);
          fputc('\n',file);
          sec=TRUE;
          }

        // Only write marked entries
        if(mrk){
          FXASSERT(group->key(e));
          FXASSERT(group->data(e));

          // Write key name
          fputs(group->key(e),file);
          fputc('=',file);

          // Write quoted value
          if(unparseValue(buffer,group->data(e))){
            fputc('"',file);
            fputs(buffer,file);
            fputc('"',file);
            }

          // Write unquoted
          else{
            fputs(buffer,file);
            }

          // End of line
          fputc('\n',file);
          }
        }

      // Blank line after end
      if(sec){
        fputc('\n',file);
        }
      }
    fclose(file);
    return TRUE;
    }
  return FALSE;
  }


// Unparse value by quoting strings; return TRUE if quote needed
FXbool FXSettings::unparseValue(FXchar* buffer,const FXchar* value){
  register FXbool mustquote=FALSE;
  register FXchar *ptr=buffer;
  register FXuchar v;
  FXASSERT(value);
  while((v=*value++) && ptr<&buffer[MAXVALUE-5]){
    switch(v){
      case '\n':
        *ptr++='\\';
        *ptr++='n';
        mustquote=TRUE;
        break;
      case '\r':
        *ptr++='\\';
        *ptr++='r';
        mustquote=TRUE;
        break;
      case '\b':
        *ptr++='\\';
        *ptr++='b';
        mustquote=TRUE;
        break;
      case '\v':
        *ptr++='\\';
        *ptr++='v';
        mustquote=TRUE;
        break;
      case '\a':
        *ptr++='\\';
        *ptr++='a';
        mustquote=TRUE;
        break;
      case '\f':
        *ptr++='\\';
        *ptr++='f';
        mustquote=TRUE;
        break;
      case '\t':
        *ptr++='\\';
        *ptr++='t';
        mustquote=TRUE;
        break;
      case '\\':
        *ptr++='\\';
        *ptr++='\\';
        mustquote=TRUE;
        break;
      case '"':
        *ptr++='\\';
        *ptr++='"';
        mustquote=TRUE;
        break;
      case '\'':
        *ptr++='\\';
        *ptr++='\'';
        mustquote=TRUE;
        break;
      case ' ':
        if((ptr==buffer) || (*value=='\0')) mustquote=TRUE;
        *ptr++=' ';
        break;
      default:
        if(v<0x20 || 0x7f<v){
          *ptr++='\\';
          *ptr++='x';
          *ptr++=FXString::HEX[v>>4];
          *ptr++=FXString::HEX[v&15];
          mustquote=TRUE;
          }
        else{
          *ptr++=v;
          }
        break;
      }
    }
  FXASSERT(ptr<&buffer[MAXVALUE]);
  *ptr='\0';
  return mustquote;
  }


// Furnish our own version if we have to
#ifndef HAVE_VSSCANF
extern "C" int vsscanf(const char* str, const char* format, va_list arg_ptr);
#endif


// Read a formatted registry entry
FXint FXSettings::readFormatEntry(const FXchar *section,const FXchar *key,const FXchar *fmt,...){
  FXLockHold applock(FXApp::instance());
  if(!section || !section[0]){ fxerror("FXSettings::readFormatEntry: bad section argument.\n"); }
  if(!key || !key[0]){ fxerror("FXSettings::readFormatEntry: bad key argument.\n"); }
  if(!fmt){ fxerror("FXSettings::readFormatEntry: bad fmt argument.\n"); }
  FXStringDict *group=find(section);
  va_list args;
  va_start(args,fmt);
  FXint result=0;
  if(group){
    const char *value=group->find(key);
    if(value){
      result=vsscanf((char*)value,fmt,args);    // Cast needed for HP-UX 11, which has wrong prototype for vsscanf
      }
    }
  va_end(args);
  return result;
  }


// Read a string-valued registry entry
const FXchar *FXSettings::readStringEntry(const FXchar *section,const FXchar *key,const FXchar *def){
  FXLockHold applock(FXApp::instance());
  if(!section || !section[0]){ fxerror("FXSettings::readStringEntry: bad section argument.\n"); }
  if(!key || !key[0]){ fxerror("FXSettings::readStringEntry: bad key argument.\n"); }
  FXStringDict *group=find(section);
  if(group){
    const char *value=group->find(key);
    if(value) return value;
    }
  return def;
  }


// Read a int-valued registry entry
FXint FXSettings::readIntEntry(const FXchar *section,const FXchar *key,FXint def){
  FXLockHold applock(FXApp::instance());
  if(!section || !section[0]){ fxerror("FXSettings::readIntEntry: bad section argument.\n"); }
  if(!key || !key[0]){ fxerror("FXSettings::readIntEntry: bad key argument.\n"); }
  FXStringDict *group=find(section);
  if(group){
    const char *value=group->find(key);
    if(value){
      FXint ivalue;
      if(value[0]=='0' && (value[1]=='x' || value[1]=='X')){
        if(sscanf(value+2,"%x",&ivalue)) return ivalue;
        }
      else{
        if(sscanf(value,"%d",&ivalue)==1) return ivalue;
        }
      }
    }
  return def;
  }


// Read a unsigned int-valued registry entry
FXuint FXSettings::readUnsignedEntry(const FXchar *section,const FXchar *key,FXuint def){
  FXLockHold applock(FXApp::instance());
  if(!section || !section[0]){ fxerror("FXSettings::readUnsignedEntry: bad section argument.\n"); }
  if(!key || !key[0]){ fxerror("FXSettings::readUnsignedEntry: bad key argument.\n"); }
  FXStringDict *group=find(section);
  if(group){
    const char *value=group->find(key);
    if(value){
      FXuint ivalue;
      if(value[0]=='0' && (value[1]=='x' || value[1]=='X')){
        if(sscanf(value+2,"%x",&ivalue)) return ivalue;
        }
      else{
        if(sscanf(value,"%u",&ivalue)==1) return ivalue;
        }
      }
    }
  return def;
  }


// Read a double-valued registry entry
FXdouble FXSettings::readRealEntry(const FXchar *section,const FXchar *key,FXdouble def){
  FXLockHold applock(FXApp::instance());
  if(!section || !section[0]){ fxerror("FXSettings::readRealEntry: bad section argument.\n"); }
  if(!key || !key[0]){ fxerror("FXSettings::readRealEntry: bad key argument.\n"); }
  FXStringDict *group=find(section);
  if(group){
    const char *value=group->find(key);
    if(value){
      FXdouble dvalue;
      if(sscanf(value,"%lf",&dvalue)==1) return dvalue;
      }
    }
  return def;
  }


// Read a color registry entry
FXColor FXSettings::readColorEntry(const FXchar *section,const FXchar *key,FXColor def){
  FXLockHold applock(FXApp::instance());
  if(!section || !section[0]){ fxerror("FXSettings::readColorEntry: bad section argument.\n"); }
  if(!key || !key[0]){ fxerror("FXSettings::readColorEntry: bad key argument.\n"); }
  FXStringDict *group=find(section);
  if(group){
    const char *value=group->find(key);
    if(value){
      return fxcolorfromname(value);
      }
    }
  return def;
  }


// Write a formatted registry entry
FXint FXSettings::writeFormatEntry(const FXchar *section,const FXchar *key,const FXchar *fmt,...){
  FXLockHold applock(FXApp::instance());
  if(!section || !section[0]){ fxerror("FXSettings::writeFormatEntry: bad section argument.\n"); }
  if(!key || !key[0]){ fxerror("FXSettings::writeFormatEntry: bad key argument.\n"); }
  if(!fmt){ fxerror("FXSettings::writeFormatEntry: bad fmt argument.\n"); }
  FXStringDict *group=insert(section);
  va_list args;
  va_start(args,fmt);
  FXint result=0;
  if(group){
    FXchar buffer[2048];
#if defined(WIN32) || defined(HAVE_VSNPRINTF)
    result=vsnprintf(buffer,sizeof(buffer),fmt,args);
#else
    result=vsprintf(buffer,fmt,args);
#endif
    group->replace(key,buffer,TRUE);
    modified=TRUE;
    }
  va_end(args);
  return result;
  }


// Write a string-valued registry entry
FXbool FXSettings::writeStringEntry(const FXchar *section,const FXchar *key,const FXchar *val){
  FXLockHold applock(FXApp::instance());
  if(!section || !section[0]){ fxerror("FXSettings::writeStringEntry: bad section argument.\n"); }
  if(!key || !key[0]){ fxerror("FXSettings::writeStringEntry: bad key argument.\n"); }
  FXStringDict *group=insert(section);
  if(group){
    group->replace(key,val,TRUE);
    modified=TRUE;
    return TRUE;
    }
  return FALSE;
  }


// Write a int-valued registry entry
FXbool FXSettings::writeIntEntry(const FXchar *section,const FXchar *key,FXint val){
  FXLockHold applock(FXApp::instance());
  if(!section || !section[0]){ fxerror("FXSettings::writeIntEntry: bad section argument.\n"); }
  if(!key || !key[0]){ fxerror("FXSettings::writeIntEntry: bad key argument.\n"); }
  FXStringDict *group=insert(section);
  if(group){
    FXchar buffer[32];
    sprintf(buffer,"%d",val);
    group->replace(key,buffer,TRUE);
    modified=TRUE;
    return TRUE;
    }
  return FALSE;
  }


// Write a unsigned int-valued registry entry
FXbool FXSettings::writeUnsignedEntry(const FXchar *section,const FXchar *key,FXuint val){
  FXLockHold applock(FXApp::instance());
  if(!section || !section[0]){ fxerror("FXSettings::writeUnsignedEntry: bad section argument.\n"); }
  if(!key || !key[0]){ fxerror("FXSettings::writeUnsignedEntry: bad key argument.\n"); }
  FXStringDict *group=insert(section);
  if(group){
    FXchar buffer[32];
    sprintf(buffer,"%u",val);
    group->replace(key,buffer,TRUE);
    modified=TRUE;
    return TRUE;
    }
  return FALSE;
  }


// Write a double-valued registry entry
FXbool FXSettings::writeRealEntry(const FXchar *section,const FXchar *key,FXdouble val){
  FXLockHold applock(FXApp::instance());
  if(!section || !section[0]){ fxerror("FXSettings::writeRealEntry: bad section argument.\n"); }
  if(!key || !key[0]){ fxerror("FXSettings::writeRealEntry: bad key argument.\n"); }
  FXStringDict *group=insert(section);
  if(group){
    FXchar buffer[64];
    sprintf(buffer,"%.16g",val);
    group->replace(key,buffer,TRUE);
    modified=TRUE;
    return TRUE;
    }
  return FALSE;
  }


// Write a color registry entry
FXbool FXSettings::writeColorEntry(const FXchar *section,const FXchar *key,FXColor val){
  FXLockHold applock(FXApp::instance());
  if(!section || !section[0]){ fxerror("FXSettings::writeColorEntry: bad section argument.\n"); }
  if(!key || !key[0]){ fxerror("FXSettings::writeColorEntry: bad key argument.\n"); }
  FXStringDict *group=insert(section);
  if(group){
    FXchar buffer[64];
    group->replace(key,fxnamefromcolor(buffer,val),TRUE);
    modified=TRUE;
    return TRUE;
    }
  return FALSE;
  }


// Delete a registry entry
FXbool FXSettings::deleteEntry(const FXchar *section,const FXchar *key){
  FXLockHold applock(FXApp::instance());
  if(!section || !section[0]){ fxerror("FXSettings::deleteEntry: bad section argument.\n"); }
  if(!key || !key[0]){ fxerror("FXSettings::deleteEntry: bad key argument.\n"); }
  FXStringDict *group=insert(section);
  if(group){
    group->remove(key);
    modified=TRUE;
    return TRUE;
    }
  return FALSE;
  }


// Delete section
FXbool FXSettings::deleteSection(const FXchar *section){
  FXLockHold applock(FXApp::instance());
  if(!section || !section[0]){ fxerror("FXSettings::deleteSection: bad section argument.\n"); }
  remove(section);
  modified=TRUE;
  return TRUE;
  }


// Clear all sections
FXbool FXSettings::clear(){
  FXLockHold applock(FXApp::instance());
  FXDict::clear();
  modified=TRUE;
  return TRUE;
  }


// See if section exists
FXbool FXSettings::existingSection(const FXchar *section){
  FXLockHold applock(FXApp::instance());
  if(!section || !section[0]){ fxerror("FXSettings::existingSection: bad section argument.\n"); }
  return find(section)!=NULL;
  }


// See if entry exists
FXbool FXSettings::existingEntry(const FXchar *section,const FXchar *key){
  FXLockHold applock(FXApp::instance());
  if(!section || !section[0]){ fxerror("FXSettings::existingEntry: bad section argument.\n"); }
  if(!key || !key[0]){ fxerror("FXSettings::existingEntry: bad key argument.\n"); }
  FXStringDict *group=find(section);
  return group && group->find(key)!=NULL;
  }


// Clean up
FXSettings::~FXSettings(){
  clear();
  }

}
