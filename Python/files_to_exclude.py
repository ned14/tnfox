#! /usr/bin/python
# Copyright 2004 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import os

files = [ 
      "FXArray.h"
    , "FXDict.h"
    , "FXDLL.h"
    , "FXElement.h"
    , "FXErrCodes.h"
    , "FXFileStream.h"
    , "FXFunctorTarget.h"
    , "FXGenericTools.h"
    , "FXHash.h"
    , "FXIconDict.h"
    , "FXLRUCache.h"
    , "FXMemDbg.h"
    , "FXMemoryPool.h"
    , "FXMemoryStream.h"
    , "FXPolicies.h"
    , "FXPtrHold.h"
    , "FXRefedObject.h"
    , "FXRollback.h"
    , "FXStringDict.h"
    , "FXString.h"
    , "FXTextCodec.h"
    , "FXURL.h"
    , "FXUTF16Codec.h"
    , "FXUTF32Codec.h"
    , "FXUTF8Codec.h"
    , "FXWString.h"
    , "QThread.h"

    # Temporary until pyplusplus gets fixed
    , "FXWinLinks.h"
]

def is_excluded( decl ):
    global files
    if None is decl.location:
        return False
    file_name = os.path.split( decl.location.file_name )[1]
    return file_name in files
