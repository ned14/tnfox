/********************************************************************************
*                                                                               *
*                 F i l e  - A s s o c i a t i o n   T a b l e                  *
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
* $Id: FXFileDict.cpp,v 1.55 2005/02/05 07:40:03 fox Exp $                      *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "FXHash.h"
#include "FXThread.h"
#include "FXStream.h"
#include "FXFileStream.h"
#include "FXString.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXFile.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXApp.h"
#include "FXIcon.h"
#include "FXIconDict.h"
#include "FXFileDict.h"


/*
  Notes:

  - FXFileDict needs additional fields, e.g. a print command.
  - The associate member function should be virtual so we can overload it.
  - FXFileDict is solely responsible for determining mime-type, and
    duplicate code in FXDirList and FXFileList is eliminated.
  - We will use two different techniques:

      - For directories, we will match "/usr/people/jeroen", then
        try "/people/jeroen", and finally, try "/jeroen" to determine
        directory bindings (Note we pass a "/" in front so we won't
        match a file binding when looking for directories.

        This means we can in many cases keep the same icon bindings
        even if some directory tree of a project is moved around in
        the file system.

      - For files, we will try to match the whole name, then try
        the extensions.

      - We will try to match "defaultdirbinding" to determine directory type,
        and "defaultfilebinding" for a file, and "defaultexecbinding", for an
        executable to allow bindings to be set for broad categories.

  - We should look into using the mime-database and content-based
    file type detection [I'm not a big fan of this, but it may sometimes
    be necessary].

  - Refer to RFC 2045, 2046, 2047, 2048, and 2077.
    The Internet media type registry is at:
    ftp://ftp.iana.org/in-notes/iana/assignments/media-types/

  - We should at least organize things so that enough info is passed in
    so we can read a fragment of the content and do it (so that means
    the full pathname, and perhaps some flags).

  - The registry format has been extended; it now is:

    command string ';' extension string ';' bigicon [ ':' bigiconopen ] ';' miniicon [ ':' miniiconopen ] ';' mimetype

  - An empty binding like:

      ext=""

    Can be used to override a global binding with an empty one, i.e. it is
    as if no binding exists; you will get the default or fallback in this case.

  - Obtaining icons for files, folders, executable, etc. under Windows:

    1) Get icon sizes:
        w = GetSystemMetrics(SM_CXSMICON);
        h = GetSystemMetrics(SM_CYSMICON);

    2) Default icons for folder:

        // Get key
        RegOpenKeyEx(HKEY_CLASSES_ROOT,"folder\\DefaultIcon",0,KEY_READ,&key);

        // To get data
        RegQueryValueEx(key,subKey,0,0,(LPBYTE)buf,&bufsz);

        // Obtain HICON extract it
        result=ExtractIconEx(filename,iconindex,arrayofbigicons,arrayofsmallicons,nicons);

    3) Default icon(s) for file:

        // Extract from shell32.dll
        ExtractIconEx("shell32.dll",0,NULL,&smallicon,1);

    4) Default exe icon(s):

        // Extract from shell32.dll
        ExtractIconExA("shell32.dll",2,NULL,&smallicon,1);

    5) Other executables:

        // Extract from executable:
        ExtractIconEx("absolutepathofexe",indexoflast,NULL,&smallicon,1);

    6) Documents:

        // Key on ".bmp" (extension with . in front)
        RegOpenKeyEx(HKEY_CLASSES_ROOT,".bmp",0,KEY_READ,&key);

        // Obtain value using:
        RegQueryValueEx(key,subKey,0,0,(LPBYTE)buf,&bufsz);

        // Key on file type and look for default icon:
        RegOpenKeyEx(HKEY_CLASSES_ROOT,"PaintPicture\\DefaultIcon",0,KEY_READ,&keyoficon);

        // Obtain value using:
        RegQueryValueEx(key,subKey,0,0,(LPBYTE)buf,&bufsz);

        // String contains program pathname, and icon index.  Use:
        ExtractIconEx("absolutepathofprogram",index,NULL,&smallicon,1);
*/


#define COMMANDLEN   256
#define EXTENSIONLEN 128
#define MIMETYPELEN  64
#define ICONNAMELEN  256




/*******************************************************************************/

namespace FX {


// These registry keys are used for default bindings.
const FXchar *FXFileDict::defaultExecBinding="defaultexecbinding";
const FXchar *FXFileDict::defaultDirBinding="defaultdirbinding";
const FXchar *FXFileDict::defaultFileBinding="defaultfilebinding";


// Object implementation
FXIMPLEMENT(FXFileDict,FXDict,NULL,0)


// Construct an file-extension association table
FXFileDict::FXFileDict(FXApp* app):settings(&app->reg()){
  FXTRACE((100,"FXFileDict::FXFileDict\n"));
  icons=new FXIconDict(app,settings->readStringEntry("SETTINGS","iconpath",FXIconDict::defaultIconPath));
  }


// Construct an file-extension association table, and alternative settings database
FXFileDict::FXFileDict(FXApp* app,FXSettings* db):settings(db){
  FXTRACE((100,"FXFileDict::FXFileDict\n"));
  icons=new FXIconDict(app,settings->readStringEntry("SETTINGS","iconpath",FXIconDict::defaultIconPath));
  }


// Create new association from extension
void *FXFileDict::createData(const void* ptr){
  register const FXchar *p=(const FXchar*)ptr;
  register FXchar *q;
  FXchar command[COMMANDLEN];
  FXchar extension[EXTENSIONLEN];
  FXchar mimetype[MIMETYPELEN];
  FXchar bigname[ICONNAMELEN];
  FXchar bignameopen[ICONNAMELEN];
  FXchar mininame[ICONNAMELEN];
  FXchar mininameopen[ICONNAMELEN];
  FXFileAssoc *fileassoc;

  FXTRACE((300,"FXFileDict: adding association: %s\n",(FXchar*)ptr));

  // Make association record
  fileassoc=new FXFileAssoc;

  // Parse command
  for(q=command; *p && *p!=';' && q<command+COMMANDLEN-1; *q++=*p++); *q='\0';

  // Skip section separator
  if(*p==';') p++;

  // Parse extension type
  for(q=extension; *p && *p!=';' && q<extension+EXTENSIONLEN-1; *q++=*p++); *q='\0';

  // Skip section separator
  if(*p==';') p++;

  // Parse big icon name
  for(q=bigname; *p && *p!=';' && *p!=':' && q<bigname+ICONNAMELEN-1; *q++=*p++); *q='\0';

  // Skip icon separator
  if(*p==':') p++;

  // Parse big open icon name
  for(q=bignameopen; *p && *p!=';' && q<bignameopen+ICONNAMELEN-1; *q++=*p++); *q='\0';

  // Skip section separator
  if(*p==';') p++;

  // Parse mini icon name
  for(q=mininame; *p && *p!=';' && *p!=':' && q<mininame+ICONNAMELEN-1; *q++=*p++); *q='\0';

  // Skip icon separator
  if(*p==':') p++;

  // Parse mini open icon name
  for(q=mininameopen; *p && *p!=';' && q<mininameopen+ICONNAMELEN-1; *q++=*p++); *q='\0';

  // Skip section separator
  if(*p==';') p++;

  // Parse mime type
  for(q=mimetype; *p && *p!=';' && q<mimetype+MIMETYPELEN-1; *q++=*p++); *q='\0';

  FXTRACE((300,"FXFileDict: command=\"%s\" extension=\"%s\" mimetype=\"%s\" big=\"%s\" bigopen=\"%s\" mini=\"%s\" miniopen=\"%s\"\n",command,extension,mimetype,bigname,bignameopen,mininame,mininameopen));

  // Initialize association data
  fileassoc->command=command;
  fileassoc->extension=extension;
  fileassoc->bigicon=NULL;
  fileassoc->miniicon=NULL;
  fileassoc->bigiconopen=NULL;
  fileassoc->miniiconopen=NULL;
  fileassoc->mimetype=mimetype;
  fileassoc->dragtype=0;
  fileassoc->flags=0;

  // Insert icons into icon dictionary
  if(bigname[0]){ fileassoc->bigicon=fileassoc->bigiconopen=icons->insert(bigname); }
  if(mininame[0]){ fileassoc->miniicon=fileassoc->miniiconopen=icons->insert(mininame); }

  // Add open icons also; we will fall back on the regular icons in needed
  if(bignameopen[0]){ fileassoc->bigiconopen=icons->insert(bignameopen); }
  if(mininameopen[0]){ fileassoc->miniiconopen=icons->insert(mininameopen); }

  // Return the binding
  return fileassoc;
  }


// Delete association
void FXFileDict::deleteData(void* ptr){
  delete ((FXFileAssoc*)ptr);
  }


// Set icon search path
void FXFileDict::setIconPath(const FXString& path){

  // Replace iconpath setting in registry
  settings->writeStringEntry("SETTINGS","iconpath",path.text());

  // Change it in icon dictionary
  icons->setIconPath(path);
  }


// Return current icon search path
const FXString& FXFileDict::getIconPath() const {
  return icons->getIconPath();
  }


// Replace or add file association
FXFileAssoc* FXFileDict::replace(const FXchar* ext,const FXchar* str){

  // Replace entry in registry
  settings->writeStringEntry("FILETYPES",ext,str);

  // Replace record
  return (FXFileAssoc*)FXDict::replace(ext,str);
  }


// Remove file association
FXFileAssoc* FXFileDict::remove(const FXchar* ext){

  // Delete registry entry for this type
  settings->deleteEntry("FILETYPES",ext);

  // Remove record
  FXDict::remove(ext);

  return NULL;
  }


// Find file association
FXFileAssoc* FXFileDict::associate(const FXchar* key){
  register const FXchar *association;
  register FXFileAssoc* record;
  if(key && key[0]){

    FXTRACE((300,"FXFileDict: trying key: %s\n",key));

    // See if we have an existing record already
    if((record=find(key))!=NULL) return record;

    // See if this entry is known in FILETYPES
    association=settings->readStringEntry("FILETYPES",key,FXString::null);

    // If not an empty string, make a record for it now
    if(association[0]) return (FXFileAssoc*)FXDict::insert(key,association);
    }
  return NULL;
  }



// Find file association from registry
FXFileAssoc* FXFileDict::findFileBinding(const FXchar* pathname){
  register const FXchar *filename=pathname;
  register const FXchar *p=pathname;
  register FXFileAssoc* record;
  FXTRACE((300,"FXFileDict: searching file binding for: %s\n",pathname));
  while(*p){ if(ISPATHSEP(*p)){ filename=p+1; } p++; }
  record=associate(filename);
  if(record) return record;
  filename=strchr(filename,'.');
  while(filename){
    record=associate(filename+1);
    if(record) return record;
    filename=strchr(filename+1,'.');
    }
  return associate(defaultFileBinding);
  }


// Find directory association from registry
FXFileAssoc* FXFileDict::findDirBinding(const FXchar* pathname){
  register const FXchar* path=pathname;
  register FXFileAssoc* record;
  FXTRACE((300,"FXFileDict: searching dir binding for: %s\n",pathname));
  while(*path){
    record=associate(path);
    if(record) return record;
    path++;
    while(*path && !ISPATHSEP(*path)) path++;
    }
  return associate(defaultDirBinding);
  }


// Find executable association from registry
FXFileAssoc* FXFileDict::findExecBinding(const FXchar* pathname){
  FXTRACE((300,"FXFileDict: searching exec binding for: %s\n",pathname));
  return associate(defaultExecBinding);
  }


// Save data
void FXFileDict::save(FXStream& store) const {
  FXDict::save(store);
  store << settings;
  store << icons;
  }


// Load data
void FXFileDict::load(FXStream& store){
  FXDict::load(store);
  store >> settings;
  store >> icons;
  }


// Destructor
FXFileDict::~FXFileDict(){
  FXTRACE((100,"FXFileDict::~FXFileDict\n"));
  delete icons;
  icons=(FXIconDict*)-1L;
  clear();
  }

}
