/********************************************************************************
*                                                                               *
*                           R e g i s t r y   C l a s s                         *
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
* $Id: FXRegistry.h,v 1.26 2004/02/08 17:17:34 fox Exp $                        *
********************************************************************************/
#ifndef FXREGISTRY_H
#define FXREGISTRY_H

#ifndef FXSETTINGS_H
#include "FXSettings.h"
#endif

namespace FX {


/**
* The registry maintains a database of persistent settings for an application,
* or suite of applications.
*/
class FXAPIR FXRegistry : public FXSettings {
  FXDECLARE(FXRegistry)
protected:
  FXString applicationkey;  // Application key
  FXString vendorkey;       // Vendor key
  FXbool   ascii;           // ASCII file-based registry
protected:
  FXbool readFromDir(const FXString& dirname,FXbool mark);
#ifdef WIN32
  FXbool readFromRegistry(void* hRootKey,FXbool mark);
  FXbool writeToRegistry(void* hRootKey);
  FXbool readFromRegistryGroup(void* org,const char* groupname,FXbool mark=FALSE);
  FXbool writeToRegistryGroup(void* org,const char* groupname);
#endif
private:
  FXRegistry(const FXRegistry&);
  FXRegistry &operator=(const FXRegistry&);
public:

  /**
  * Construct registry object; akey and vkey must be string constants.
  * Regular applications SHOULD set a vendor key!
  */
  FXRegistry(const FXString& akey=FXString::null,const FXString& vkey=FXString::null);

  /// Read registry
  FXbool read();

  /// Write registry
  FXbool write();

  /// Return application key
  FXString getAppKey() const { return applicationkey; }

  /// Return vendor key
  FXString getVendorKey() const { return vendorkey; }

  /**
  * Set ASCII mode; under MS-Windows, this will switch the system to a
  * file-based registry system, instead of using the System Registry API.
  */
  void setAsciiMode(FXbool asciiMode){ ascii=asciiMode; }

  /// Get ASCII mode
  FXbool getAsciiMode() const { return ascii; }
  };

}

#endif

