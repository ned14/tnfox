/********************************************************************************
*                                                                               *
*                  F i l e - A s s o c i a t i o n   T a b l e                  *
*                                                                               *
*********************************************************************************
* Copyright (C) 1998,2004 by Jeroen van der Zijp.   All Rights Reserved.        *
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
* $Id: FXFileDict.h,v 1.21 2004/02/08 17:17:33 fox Exp $                        *
********************************************************************************/
#ifndef FXFILEDICT_H
#define FXFILEDICT_H

#ifndef FXDICT_H
#include "FXDict.h"
#endif

namespace FX {

class FXStream;
class FXIcon;
class FXApp;
class FXSettings;

/// Registers stuff to know about the extension
struct FXFileAssoc {
  FXString   command;           /// Command to execute
  FXString   extension;         /// Full extension name
  FXString   mimetype;          /// Mime type name
  FXIcon    *bigicon;           /// Big normal icon
  FXIcon    *bigiconopen;       /// Big open icon
  FXIcon    *miniicon;          /// Mini normal icon
  FXIcon    *miniiconopen;      /// Mini open icon
  FXDragType dragtype;          /// Registered drag type
  FXuint     flags;             /// Flags
  };


/// Icon dictionary
class FXAPI FXIconDict : public FXDict {
  FXDECLARE(FXIconDict)
private:
  FXApp        *app;            // Application object
  FXString      path;           // Where to search icons
protected:
  FXIconDict(){}
  virtual void *createData(const void*);
  virtual void deleteData(void*);
private:
  FXIconDict(const FXIconDict&);
  FXIconDict &operator=(const FXIconDict&);
public:

  /// Default icon search path
  static const FXchar *defaultIconPath;

public:

  /// Construct an icon dictionary, with given path
  FXIconDict(FXApp* a,const FXString& p=defaultIconPath);

  /// Get application
  FXApp* getApp() const { return app; }

  /// Set icon search path
  void setIconPath(const FXString& p){ path=p; }

  /// Return current icon search path
  FXString getIconPath() const { return path; }

  /// Insert unique icon loaded from filename into dictionary
  FXIcon* insert(const FXchar* name){ return (FXIcon*)FXDict::insert(name,name); }

  /// Remove icon from dictionary
  FXIcon* remove(const FXchar* name){ return (FXIcon*)FXDict::remove(name); }

  /// Find icon by name
  FXIcon* find(const FXchar* name){ return (FXIcon*)FXDict::find(name); }

  /// Save to stream
  virtual void save(FXStream& store) const;

  /// Load from stream
  virtual void load(FXStream& store);

  /// Destructor
  virtual ~FXIconDict();
  };


/**
* The File Association dictionary associates a file extension
* with a FXFileAssoc record which contains command name, mime type,
* icons, and other information about the file type.
* The Registry is used as source of the file bindings; an alternative
* Settings database may be specified however.
*/
class FXAPI FXFileDict : public FXDict {
  FXDECLARE(FXFileDict)
private:
  FXApp        *app;            // Application object
  FXSettings   *settings;       // Settings database where to get bindings
  FXIconDict   *icons;          // Icon table
protected:
  FXFileDict(){}
  virtual void *createData(const void*);
  virtual void deleteData(void*);
private:
  FXFileDict(const FXFileDict&);
  FXFileDict &operator=(const FXFileDict&);
public:

  /// Registry key used to find fallback executable icons
  static const FXchar *defaultExecBinding;

  /// Registry key used to find fallback directory icons
  static const FXchar *defaultDirBinding;

  /// Registry key used to find fallback document icons
  static const FXchar *defaultFileBinding;
public:

  /**
  * Construct a dictionary mapping file-extension to file associations,
  * using the application registry settings as a source for the bindings.
  */
  FXFileDict(FXApp* a);

  /**
  * Construct a dictionary mapping file-extension to file associations,
  * using the specified settings database as a source for the bindings.
  */
  FXFileDict(FXApp* a,FXSettings* db);

  /// Get application
  FXApp* getApp() const { return app; }

  /// Set icon search path
  void setIconPath(const FXString& path);

  /// Return current icon search path
  FXString getIconPath() const;

  /**
  * Replace file association.
  * The new association is written into the settings database under the
  * FILETYPES section; the format of the association is as follows:
  *
  * <extension> = "<command> ; <type> ; <bigicon> [ : <bigopenicon> ] ; <smallicon> [ : <smalliconopen> ] ; <mimetype>"
  *
  * Where <command> is the command used to launch the application (e.g. "xv %s &"),
  * and <type> is the file type string (e.g. "GIF Image"),
  * <bigicon> and <bigiconopen> are the large icons shown in "Icons" mode,
  * <smallicon> and <smalliconopen> are the small icons shown in "Details" mode,
  * and <mimetype> is the RFC2045 mime type of the file.
  *
  * For example:
  *
  * [FILETYPES]
  * gif="xv %s &;GIF Image;big.xpm:bigopen.xpm;mini.xpm:miniopen.xpm;image/gif"
  * /home/jeroen=";Home;home.xpm;minihome.xpm;application/x-folder"
  *
  */
  FXFileAssoc* replace(const FXchar* ext,const FXchar* str);

  /// Remove file association
  FXFileAssoc* remove(const FXchar* ext);

  /// Find file association already in dictionary
  FXFileAssoc* find(const FXchar* ext){ return (FXFileAssoc*)FXDict::find(ext); }

  /// Find file association from registry
  FXFileAssoc* associate(const FXchar* key);

  /**
  * Determine binding for the given file.
  * The default implementation tries the whole filename first,
  * then tries the extensions.
  * For example, for a file "source.tar.gz":
  *
  *  "source.tar.gz",
  *  "tar.gz",
  *  "gz"
  *
  * are tried in succession.  If no association is found the
  * key "defaultfilebinding" is tried as a fallback association.
  * A NULL is returned if no association of any kind is found.
  */
  virtual FXFileAssoc* findFileBinding(const FXchar* pathname);

  /**
  * Find directory binding from registry.
  * The default implementation tries the whole pathname first,
  * then tries successively smaller parts of the path.
  * For example, a pathname "/usr/people/jeroen":
  *
  *   "/usr/people/jeroen"
  *   "/people/jeroen"
  *   "/jeroen"
  *
  * are tried in succession.  If no bindings are found, the
  * key "defaultdirbinding" is tried as a fallback association.
  * A NULL is returned if no association of any kind is found.
  */
  virtual FXFileAssoc* findDirBinding(const FXchar* pathname);

  /**
  * Determine binding for the given executable.
  * The default implementation returns the fallback binding associated with
  * the key "defaultexecbinding".
  * A NULL is returned if no association of any kind is found.
  */
  virtual FXFileAssoc* findExecBinding(const FXchar* pathname);

  /// Save to stream
  virtual void save(FXStream& store) const;

  /// Load from stream
  virtual void load(FXStream& store);

  /// Destructor
  virtual ~FXFileDict();
  };

}

#endif

