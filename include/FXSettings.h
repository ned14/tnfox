/********************************************************************************
*                                                                               *
*                           S e t t i n g s   C l a s s                         *
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
* $Id: FXSettings.h,v 1.22 2004/09/26 18:02:28 fox Exp $                        *
********************************************************************************/
#ifndef FXSETTINGS_H
#define FXSETTINGS_H

#ifndef FXDICT_H
#include "FXDict.h"
#endif

namespace FX {


class FXStringDict;


/**
* The Settings class manages a key-value database.  This is normally used as
* part of Registry, but can also be used separately in applications that need 
* to maintain a key-value database in a file of their own.
*/
class FXAPI FXSettings : public FXDict {
  FXDECLARE(FXSettings)
protected:
  FXbool modified;      // Changed settings
protected:
  virtual void *createData(const void*);
  virtual void deleteData(void*);
  FXbool parseValue(FXchar* value,const FXchar* buffer);
  FXbool unparseValue(FXchar* buffer,const FXchar* value);
  FXStringDict* insert(const FXchar* ky){ return (FXStringDict*)FXDict::insert(ky,NULL); }
  FXStringDict* replace(const FXchar* ky,FXStringDict* section){ return (FXStringDict*)FXDict::replace(ky,section,TRUE); }
  FXStringDict* remove(const FXchar* ky){ return (FXStringDict*)FXDict::remove(ky); }
private:
  FXSettings(const FXSettings&);
  FXSettings &operator=(const FXSettings&);
public:

  /// Construct settings database.
  FXSettings();

  /// Parse a file containing a settings database.
  FXbool parseFile(const FXString& filename,FXbool mark);

  /// Unparse settings database into given file.
  FXbool unparseFile(const FXString& filename);

  /// Obtain the string dictionary for the given section
  FXStringDict* data(FXuint pos) const { return (FXStringDict*)FXDict::data(pos); }

  /// Find string dictionary for the given section
  FXStringDict* find(const FXchar *section) const { return (FXStringDict*)FXDict::find(section); }

  /// Read a formatted registry entry, using scanf-style format
  FXint readFormatEntry(const FXchar *section,const FXchar *key,const FXchar *fmt,...) FX_SCANF(4,5) ;

  /// Read a string registry entry; if no value is found, the default value def is returned
  const FXchar *readStringEntry(const FXchar *section,const FXchar *key,const FXchar *def=NULL);

  /// Read a integer registry entry; if no value is found, the default value def is returned
  FXint readIntEntry(const FXchar *section,const FXchar *key,FXint def=0);

  /// Read a unsigned integer registry entry; if no value is found, the default value def is returned
  FXuint readUnsignedEntry(const FXchar *section,const FXchar *key,FXuint def=0);

  /// Read a double-precision floating point registry entry; if no value is found, the default value def is returned
  FXdouble readRealEntry(const FXchar *section,const FXchar *key,FXdouble def=0.0);

  /// Read a color value registry entry; if no value is found, the default value def is returned
  FXColor readColorEntry(const FXchar *section,const FXchar *key,FXColor def=0);

  /// Write a formatted registry entry, using printf-style format
  FXint writeFormatEntry(const FXchar *section,const FXchar *key,const FXchar *fmt,...) FX_PRINTF(4,5) ;

  /// Write a string registry entry
  FXbool writeStringEntry(const FXchar *section,const FXchar *key,const FXchar *val);

  /// Write a integer registry entry
  FXbool writeIntEntry(const FXchar *section,const FXchar *key,FXint val);

  /// Write a unsigned integer registry entry
  FXbool writeUnsignedEntry(const FXchar *section,const FXchar *key,FXuint val);

  /// Write a double-precision floating point registry entry
  FXbool writeRealEntry(const FXchar *section,const FXchar *key,FXdouble val);

  /// Write a color value entry
  FXbool writeColorEntry(const FXchar *section,const FXchar *key,FXColor val);

  /// Delete a registry entry
  FXbool deleteEntry(const FXchar *section,const FXchar *key);

  /// See if entry exists
  FXbool existingEntry(const FXchar *section,const FXchar *key);

  /// Delete section
  FXbool deleteSection(const FXchar *section);

  /// See if section exists
  FXbool existingSection(const FXchar *section);

  /// Clear all sections
  FXbool clear();

  /// Mark as changed
  void setModified(FXbool mdfy=TRUE){ modified=mdfy; }

  /// Is it modified
  FXbool isModified() const { return modified; }

  /// Cleanup
  virtual ~FXSettings();
  };


namespace FXSettingsHelpers
{
	/*! Helper function which makes the variable and FXSetting setting consistent
	based upon \em save */
	template<typename type> inline void rwSetting(bool save, FXSettings &settings, const FXchar *section, const FXchar *name, type &val, const type &defval);
	template<> inline void rwSetting<bool>(bool save, FXSettings &settings, const FXchar *section, const FXchar *name, bool &val, const bool &defval)
	{
		save ? settings.writeIntEntry(section, name, val)
			: (val=(0!=settings.readIntEntry(section, name, defval)));
	}
	template<> inline void rwSetting<FXint>(bool save, FXSettings &settings, const FXchar *section, const FXchar *name, FXint &val, const FXint &defval)
	{
		save ? settings.writeIntEntry(section, name, val)
			: (val=settings.readIntEntry(section, name, defval));
	}
	template<> inline void rwSetting<FXuint>(bool save, FXSettings &settings, const FXchar *section, const FXchar *name, FXuint &val, const FXuint &defval)
	{
		save ? settings.writeUnsignedEntry(section, name, val)
			: (val=settings.readUnsignedEntry(section, name, defval));
	}
	/* GCC won't accept that FXColor is different to a FXuint :(
	template<> inline void rwSetting<FXColor(bool save, FXSettings &settings, const FXchar *section, const FXchar *name, FXColor &val, const FXColor &defval)
	{
		save ? settings.writeColorEntry(section, name, val)
			: (val=settings.readColorEntry(section, name, defval));
	}*/
	template<> inline void rwSetting<double>(bool save, FXSettings &settings, const FXchar *section, const FXchar *name, double &val, const double &defval)
	{
		save ? settings.writeRealEntry(section, name, val)
			: (val=settings.readRealEntry(section, name, defval));
	}
	template<> inline void rwSetting<FXString>(bool save, FXSettings &settings, const FXchar *section, const FXchar *name, FXString &val, const FXString &defval)
	{
		save ? settings.writeStringEntry(section, name, val.text())
			: (val=settings.readStringEntry(section, name, defval.text()), true);
	}
	// Ambiguity buster
	template<typename type> inline void rwSetting(bool save, FXSettings &settings, const FXchar *section, const FXchar *name, type &val, int defval)
	{
		rwSetting<type>(save, settings, section, name, val, (type) defval);
	}
}

}

#endif

