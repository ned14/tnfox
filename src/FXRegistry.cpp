/********************************************************************************
*                                                                               *
*                           R e g i s t r y   C l a s s                         *
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
* $Id: FXRegistry.cpp,v 1.43 2005/01/16 16:06:07 fox Exp $                      *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "FXHash.h"
#include "FXStream.h"
#include "FXObject.h"
#include "FXString.h"
#include "FXFile.h"
#include "FXStringDict.h"
#include "FXRegistry.h"

/*
  Notes:

  - Default directories for registry files where search for the FOX settings
    database is tried:

     1 $FOXDIR

     2 /etc/foxrc /usr/lib/foxrc /usr/local/lib/foxrc

     3 $PATH/foxrc

     4 ~/.foxrc

  - The latter one is writable, and also the one that overrides all other
    values.

  - Search $PATH for registry directory [$PATH/VendorKey/AppKey] or
    [$PATH/AppKey]

  - Compile-time #define for registry directories [for global settings].

  - Registry is organized as follows:

    DESKTOP             Registry file common to all FOX applications, from all vendors
    Vendor/Vendor       Registry file common to all applications from Vendor
    Vendor/Application  Registry file for Application from Vendor
    Application         Registry file for Application, if no Vendor specified

    Rationale:

      1)    When installing an application, simply copy ``seed'' registry files
            in the appropriate places; having a subdirectory Vendor prevents clobbering
            other people's registry files, even if their application has the same name.

      2)    System-wide registry files are, as a matter of principle, read-only.

      3)    System registry files are loaded first, then per-user registry files are
            loaded on top of that.

      4)    Registry files loaded later will take precedence over those loaded earlier;
            i.e. key/value pairs in a later file will override a key/value pair with the
            same key loaded earlier.

      5)    The exception to the rule is that a key/value pair will not be overridden if
            the value of the key has changed since loading it. In other words, changes
            will persist.

      6)    An application reads files in the order of system: (DESKTOP, Vendor/Vendor,
            Vendor/App), then user: (DESKTOP, Vendor/Vendor, Vendor/App).

      7)    All changed entries are written to the per-user directory.  An application
            starts by reading the registry files, runs for a while, then at the end writes
            the settings back out to the per-user registry files.
            It will ONLY write those entries which (a) have been changed, or (b) were
            previously read from the same per-user registry files.

  - Need ability to read/write DESKTOP, Vendor/Vendor, Vendor/Application registry,
    for both user as well as system-wide settings. This is primarily important for
    installation programs.

*/

#define MAXNAME   200
#define MAXVALUE  2000

#ifndef REGISTRYPATH
#ifndef WIN32
#define REGISTRYPATH   "/etc:/usr/lib:/usr/local/lib"
#else
#define REGISTRYPATH   "\\WINDOWS\\foxrc"
#endif
#endif

#define DESKTOP        "Desktop"



/*******************************************************************************/

namespace FX {

// Object implementation
FXIMPLEMENT(FXRegistry,FXSettings,NULL,0)

// Make registry object
FXRegistry::FXRegistry(const FXString& akey,const FXString& vkey):applicationkey(akey),vendorkey(vkey){
#ifndef WIN32
  ascii=TRUE;
#else
  ascii=FALSE;
#endif
  }


// Read registry
FXbool FXRegistry::read(){
  FXString dirname;
  register FXbool ok=FALSE;

#ifdef WIN32      // Either file based or system registry for WIN32

  if(ascii){

    FXTRACE((100,"Reading from file based settings database.\n"));

    // Try get location from FOXDIR environment variable
    dirname=getenv("FOXDIR");
    if(!dirname.empty()){
      FXTRACE((100,"Found registry %s in $FOXDIR.\n",dirname.text()));
      ok=readFromDir(dirname,FALSE);
      }

    // Try search along REGISTRYPATH if not specified explicitly
    if(!ok){
      dirname=FXFile::search(REGISTRYPATH,"foxrc");
      if(!dirname.empty()){
        FXTRACE((100,"Found registry %s in REGISTRYPATH.\n",dirname.text()));
        ok=readFromDir(dirname,FALSE);
        }
      }

    // Try search along PATH if still not found
    if(!ok){
      dirname=FXFile::search(FXFile::getExecPath(),"foxrc");
      if(!dirname.empty()){
        FXTRACE((100,"Found registry %s in $PATH.\n",dirname.text()));
        ok=readFromDir(dirname,FALSE);
        }
      }

    // Get path to per-user settings directory
    dirname=FXFile::getEnvironment("USERPROFILE")+PATHSEPSTRING "foxrc";

    // Then read per-user settings; overriding system-wide ones
    if(readFromDir(dirname,TRUE)) ok=TRUE;
    }

  else{

    FXTRACE((100,"Reading from registry HKEY_LOCAL_MACHINE.\n"));

    // Load system-wide resources first
    if(readFromRegistry(HKEY_LOCAL_MACHINE,FALSE)) ok=TRUE;

    FXTRACE((100,"Reading from registry HKEY_CURRENT_USER.\n"));

    // Now any modified resources for current user
    if(readFromRegistry(HKEY_CURRENT_USER,TRUE)) ok=TRUE;
    }

#else             // File based registry for UNIX

  // Try get location from FOXDIR environment variable
  dirname=getenv("FOXDIR");
  if(!dirname.empty()){
    FXTRACE((100,"Found registry %s in $FOXDIR.\n",dirname.text()));
    ok=readFromDir(dirname,FALSE);
    }

  // Try search along REGISTRYPATH if not specified explicitly
  if(!ok){
    dirname=FXFile::search(REGISTRYPATH,"foxrc");
    if(!dirname.empty()){
      FXTRACE((100,"Found registry %s in REGISTRYPATH.\n",dirname.text()));
      ok=readFromDir(dirname,FALSE);
      }
    }

  // Try search along PATH if still not found
  if(!ok){
    dirname=FXFile::search(FXFile::getExecPath(),"foxrc");
    if(!dirname.empty()){
      FXTRACE((100,"Found registry %s in $PATH.\n",dirname.text()));
      ok=readFromDir(dirname,FALSE);
      }
    }

  // Get path to per-user settings directory
  dirname=FXFile::getHomeDirectory()+PATHSEPSTRING ".foxrc";

  // Then read per-user settings; overriding system-wide ones
  if(readFromDir(dirname,TRUE)) ok=TRUE;

#endif

  return ok;
  }


// Try read registry from directory
FXbool FXRegistry::readFromDir(const FXString& dirname,FXbool mark){
  FXbool ok=FALSE;

  // Directory is empty?
  if(!dirname.empty()){

    // First try to load desktop registry
#ifndef WIN32
    if(parseFile(dirname+PATHSEPSTRING DESKTOP,FALSE)) ok=TRUE;
#else
    if(parseFile(dirname+PATHSEPSTRING DESKTOP ".ini",FALSE)) ok=TRUE;
#endif

    // Have vendor key
    if(!vendorkey.empty()){
#ifndef WIN32
      if(parseFile(dirname+PATHSEPSTRING+vendorkey+PATHSEPSTRING+vendorkey,FALSE)) ok=TRUE;
#else
      if(parseFile(dirname+PATHSEPSTRING+vendorkey+PATHSEPSTRING+vendorkey+".ini",FALSE)) ok=TRUE;
#endif
      // Have application key
      if(!applicationkey.empty()){
#ifndef WIN32
        if(parseFile(dirname+PATHSEPSTRING+vendorkey+PATHSEPSTRING+applicationkey,mark)) ok=TRUE;
#else
        if(parseFile(dirname+PATHSEPSTRING+vendorkey+PATHSEPSTRING+applicationkey+".ini",mark)) ok=TRUE;
#endif
        }
      }

    // No vendor key
    else{

      // Have application key
      if(!applicationkey.empty()){
#ifndef WIN32
        if(parseFile(dirname+PATHSEPSTRING+applicationkey,mark)) ok=TRUE;
#else
        if(parseFile(dirname+PATHSEPSTRING+applicationkey+".ini",mark)) ok=TRUE;
#endif
        }
      }
    }
  return ok;
  }


#ifdef WIN32

// Read from Windows Registry
FXbool FXRegistry::readFromRegistry(void* hRootKey,FXbool mark){
  HKEY hSoftKey,hOrgKey;
  FXbool ok=FALSE;

  // Open Software registry section
  if(RegOpenKeyEx((HKEY)hRootKey,"Software",0,KEY_READ,&hSoftKey)==ERROR_SUCCESS){

    // Read Software\Desktop
    if(readFromRegistryGroup(hSoftKey,DESKTOP)) ok=TRUE;

    // Have vendor key
    if(!vendorkey.empty()){

      // Open Vendor registry sub-section
      if(RegOpenKeyEx(hSoftKey,vendorkey.text(),0,KEY_READ,&hOrgKey)==ERROR_SUCCESS){

        // Read Software\Vendor\Vendor
        if(readFromRegistryGroup(hOrgKey,vendorkey.text())) ok=TRUE;

        // Have application key
        if(!applicationkey.empty()){

          // Read Software\Vendor\Application
          if(readFromRegistryGroup(hOrgKey,applicationkey.text(),mark)) ok=TRUE;
          }
        RegCloseKey(hOrgKey);
        }
      }

    // No vendor key
    else{

      // Have application key
      if(!applicationkey.empty()){

        // Read Software\Application
        if(readFromRegistryGroup(hSoftKey,applicationkey.text(),mark)) ok=TRUE;
        }
      }
    RegCloseKey(hSoftKey);
    }
  return ok;
  }


// Read from given group
FXbool FXRegistry::readFromRegistryGroup(void* org,const char* groupname,FXbool mark){
  FXchar section[MAXNAME],name[MAXNAME],value[MAXVALUE];
  DWORD sectionsize,sectionindex,namesize,valuesize,index,type;
  HKEY groupkey,sectionkey;
  FILETIME writetime;
  FXStringDict *group;
  if(RegOpenKeyEx((HKEY)org,groupname,0,KEY_READ,&groupkey)==ERROR_SUCCESS){
    sectionindex=0;
    sectionsize=MAXNAME;
    FXTRACE((100,"Reading registry group %s\n",groupname));
    while(RegEnumKeyEx(groupkey,sectionindex,section,&sectionsize,NULL,NULL,NULL,&writetime)==ERROR_SUCCESS){
      group=insert(section);
      FXTRACE((100,"[%s]\n",section));
      if(RegOpenKeyEx(groupkey,section,0,KEY_READ,&sectionkey)==ERROR_SUCCESS){
        index=0;
        namesize=MAXNAME;
        valuesize=MAXVALUE;
        while(RegEnumValue(sectionkey,index,name,&namesize,NULL,&type,(BYTE*)value,&valuesize)!=ERROR_NO_MORE_ITEMS){
          FXASSERT(type==REG_SZ);
          FXTRACE((100,"%s=%s\n",name,value));
          group->replace(name,value,mark);
          namesize=MAXNAME;
          valuesize=MAXVALUE;
          index++;
          }
        RegCloseKey(sectionkey);
        }
      sectionsize=MAXNAME;
      sectionindex++;
      }
    RegCloseKey(groupkey);
    return TRUE;
    }
  return FALSE;
  }


#endif



// Write registry
FXbool FXRegistry::write(){
  FXString pathname,tempname;

  // Settings have not changed
  if(!isModified()) return TRUE;

  // We can not save if no application key given
  if(!applicationkey.empty()){

#ifdef WIN32      // Either file based or system registry for WIN32

    if(ascii){

      FXTRACE((100,"Writing to file based settings database.\n"));

      // Changes written only in the per-user registry
      pathname=FXFile::getEnvironment("USERPROFILE")+PATHSEPSTRING "foxrc";

      // If this directory does not exist, make it
      if(!FXFile::exists(pathname)){
        if(!FXFile::createDirectory(pathname,0777)){
          fxwarning("%s: unable to create directory.\n",pathname.text());
          return FALSE;
          }
        }
      else{
        if(!FXFile::isDirectory(pathname)){
          fxwarning("%s: is not a directory.\n",pathname.text());
          return FALSE;
          }
        }

      // Add vendor subdirectory
      if(!vendorkey.empty()){
        pathname.append(PATHSEPSTRING+vendorkey);
        if(!FXFile::exists(pathname)){
          if(!FXFile::createDirectory(pathname,0777)){
            fxwarning("%s: unable to create directory.\n",pathname.text());
            return FALSE;
            }
          }
        else{
          if(!FXFile::isDirectory(pathname)){
            fxwarning("%s: is not a directory.\n",pathname.text());
            return FALSE;
            }
          }
        }

      // Add application key
      pathname.append(PATHSEPSTRING+applicationkey+".ini");

      // Construct temp name
      tempname.format("%s_%d",pathname.text(),fxgetpid());

      // Unparse settings into temp file first
      if(unparseFile(tempname)){

      // Rename ATOMICALLY to proper name
      if(!FXFile::move(tempname,pathname,TRUE)){
        fxwarning("Unable to save registry.\n");
        return FALSE;
        }

      modified=FALSE;
      return TRUE;
      }
    }

  else{

    FXTRACE((100,"Writing to registry HKEY_CURRENT_USER.\n"));

    // Write back modified resources for current user
    if(writeToRegistry(HKEY_CURRENT_USER)) return TRUE;
    }

#else             // File based registry for X11

    // Changes written only in the per-user registry
    pathname=FXFile::getHomeDirectory()+PATHSEPSTRING ".foxrc";

    // If this directory does not exist, make it
    if(!FXFile::exists(pathname)){
      if(!FXFile::createDirectory(pathname,0777)){
        fxwarning("%s: unable to create directory.\n",pathname.text());
        return FALSE;
        }
      }
    else{
      if(!FXFile::isDirectory(pathname)){
        fxwarning("%s: is not a directory.\n",pathname.text());
        return FALSE;
        }
      }

    // Add vendor subdirectory
    if(!vendorkey.empty()){
      pathname.append(PATHSEPSTRING+vendorkey);
      if(!FXFile::exists(pathname)){
        if(!FXFile::createDirectory(pathname,0777)){
          fxwarning("%s: unable to create directory.\n",pathname.text());
          return FALSE;
          }
        }
      else{
        if(!FXFile::isDirectory(pathname)){
          fxwarning("%s: is not a directory.\n",pathname.text());
          return FALSE;
          }
        }
      }

    // Add application key
    pathname.append(PATHSEPSTRING+applicationkey);

    // Construct temp name
    tempname.format("%s_%d",pathname.text(),fxgetpid());

    // Unparse settings into temp file first
    if(unparseFile(tempname)){

      // Rename ATOMICALLY to proper name
      if(!FXFile::move(tempname,pathname,TRUE)){
        fxwarning("Unable to save registry.\n");
        return FALSE;
        }
      setModified(FALSE);
      return TRUE;
      }

#endif

    }
  return FALSE;
  }



#ifdef WIN32

// Update current user's settings
FXbool FXRegistry::writeToRegistry(void* hRootKey){
  HKEY hSoftKey,hOrgKey;
  DWORD disp;
  FXbool ok=FALSE;

  // Open Software registry section
  if(RegOpenKeyEx((HKEY)hRootKey,"Software",0,KEY_WRITE,&hSoftKey)==ERROR_SUCCESS){

    // Have vendor key
    if(!vendorkey.empty()){

      // Open Vendor registry sub-section
      if(RegCreateKeyEx(hSoftKey,vendorkey.text(),0,REG_NONE,REG_OPTION_NON_VOLATILE,KEY_WRITE|KEY_READ,NULL,&hOrgKey,&disp)==ERROR_SUCCESS){

        // Have application key
        if(!applicationkey.empty()){

          // Write Software\Vendor\Application
          if(writeToRegistryGroup(hOrgKey,applicationkey.text())) ok=TRUE;
          }
        RegCloseKey(hOrgKey);
        }
      }

    // No vendor key
    else{

      // Have application key
      if(!applicationkey.empty()){

        // Write Software\Application
        if(writeToRegistryGroup(hSoftKey,applicationkey.text())) ok=TRUE;
        }
      }

    // Done with Software key
    RegCloseKey(hSoftKey);
    }
  return ok;
  }


// Write to registry group
FXbool FXRegistry::writeToRegistryGroup(void* org,const char* groupname){
  FXchar section[MAXNAME];
  DWORD sectionsize,sectionindex,disp;
  HKEY groupkey,sectionkey;
  FXint s,e;
  FILETIME writetime;
  FXStringDict *group;
  if(RegCreateKeyEx((HKEY)org,groupname,0,REG_NONE,REG_OPTION_NON_VOLATILE,KEY_WRITE|KEY_READ,NULL,&groupkey,&disp)==ERROR_SUCCESS){

    // First, purge all existing sections
    while(1){
      sectionindex=0;
      sectionsize=MAXNAME;
      if(RegEnumKeyEx(groupkey,sectionindex,section,&sectionsize,NULL,NULL,NULL,&writetime)!=ERROR_SUCCESS) break;
      if(RegDeleteKey(groupkey,section)!=ERROR_SUCCESS) break;
      }

    // Dump the registry, writing only marked entries
    s=first();
    while(s<size()){
      sectionkey=NULL;
      group=data(s);
      FXASSERT(group);
      for(e=group->first(); e<group->size(); e=group->next(e)){
        if(group->mark(e)){
          if(sectionkey==NULL){
            FXASSERT(key(s));
            if(RegCreateKeyEx(groupkey,key(s),0,REG_NONE,REG_OPTION_NON_VOLATILE,KEY_WRITE|KEY_READ,NULL,&sectionkey,&disp)!=ERROR_SUCCESS) goto x;
            }
          FXASSERT(group->key(e));
          FXASSERT(group->data(e));
          if(RegSetValueEx(sectionkey,group->key(e),0,REG_SZ,(BYTE*)group->data(e),strlen(group->data(e))+1)!=ERROR_SUCCESS) break;
          }
        }

      // Close this section's key (if it exists)
      if(sectionkey) RegCloseKey(sectionkey);

      // Process next registry section
x:    s=next(s);
      }
    RegCloseKey(groupkey);
    return TRUE;
    }
  return FALSE;
  }


#endif

}
