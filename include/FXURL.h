/********************************************************************************
*                                                                               *
*                       U R L   M a n i p u l a t i o n                         *
*                                                                               *
*********************************************************************************
* Copyright (C) 2000,2004 by Jeroen van der Zijp.   All Rights Reserved.        *
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
* $Id: FXURL.h,v 1.10 2004/02/08 17:17:34 fox Exp $                              *
********************************************************************************/
#ifndef FXURL_H
#define FXURL_H

#include "FXString.h"

namespace FX {

namespace FXURL {       // FIXME namespace may not be needed anymore?

/// Return host name
FXString FXAPI hostname();

/// Return URL of filename
FXString FXAPI fileToURL(const FXString& file);

/// Return filename from URL, empty if url is not a local file
FXString FXAPI fileFromURL(const FXString& url);

}

}

#endif

