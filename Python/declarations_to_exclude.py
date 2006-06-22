#! /usr/bin/python
# Copyright 2004 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import os
from environment import settings
from pygccxml import declarations

to_be_excluded = [
      "::FX::FXACL::int_toWin32SecurityDescriptor"            # internal
    , "::FX::FXApp::getNextEvent"                             # Uses FXRawEvent (undefined)
    , "::FX::FXApp::dispatchEvent"                            # Uses FXRawEvent (undefined)
    , "::FX::FXDataTarget::addUpcall"                         # uses functor
    , "::FX::FXDCPrint::outf"                                 # internal
    , "::FX::FXDCPrint::outhex"                               # internal
    , "::FX::FXEventLoop::getNextEvent"                       # Uses FXRawEvent (undefined)
    , "::FX::FXEventLoop::getNextEventI"                      # Uses FXRawEvent (undefined)
    , "::FX::FXEventLoop::dispatchEvent"                      # Uses FXRawEvent (undefined)
    , "::FX::FXException::int_enableNestedExceptionFramework" # internal
    , "::FX::FXFile::info"                                    # Uses unstable struct stat
    , "::FX::FXFile::linkinfo"                                # Uses unstable struct stat
    , "::FX::FXFunctorTarget::functor"                        # uses functor
    , "::FX::FXFunctorTarget::setFunctor"                     # uses functor
    , "::FX::FXImage::setData"                                # Can't allow setting image data to external data
    , "::FX::FXIPCMsg::originalData"                          # Returns a byte array
    , "::FX::FXMat3d::operator ::FX::FXdouble const *"
    , "::FX::FXMat3f::operator ::FX::FXfloat const *"
    , "::FX::FXMat4d::operator ::FX::FXdouble const *"
    , "::FX::FXMat4f::operator ::FX::FXfloat const *"
    , "::FX::FXMessageBox::error"                             # Uses ...
    , "::FX::FXMessageBox::information"                       # Uses ...
    , "::FX::FXMessageBox::question"                          # Uses ...
    , "::FX::FXMessageBox::warning"                           # Uses ...
    , "::FX::FXMetaClass::FXMetaClass"                        # Uses function pointer
    , "::FX::FXObject::decouplePythonObject"                  # internal
    , "::FX::FXObject::FXMapEntry::func"                      # Uses function pointer
    , "::FX::FXObject::getPythonObject"                       # internal
    , "::FX::FXSettings::readFormatEntry"                     # Uses ...
    , "::FX::FXSettings::writeFormatEntry"                    # Uses ...
    , "::FX::TnFXSQLDBCursor::resultsLatch"                   # internal
    , "::FX::FXStream::readBytes"
    , "::FX::FXStream::readRawBytes"
    , "::FX::FXStream::writeBytes"
    , "::FX::FXStream::writeRawBytes"
    , "::FX::FXVec2d::operator ::FX::FXdouble const *"
    , "::FX::FXVec2f::operator ::FX::FXfloat const *"
    , "::FX::FXVec3d::operator ::FX::FXdouble const *"
    , "::FX::FXVec3f::operator ::FX::FXfloat const *"
    , "::FX::FXVec4d::operator ::FX::FXdouble const *"
    , "::FX::FXVec4f::operator ::FX::FXfloat const *"
    , "::FX::FXWindow::addColormapWindows"                    # internal
    , "::FX::FXWindow::remColormapWindows"                    # internal
    , "::FX::QHostAddress::ip6Addr"                           # Returns array
    , "::FX::QIODevice::readBlock"
    , "::FX::QIODevice::readBlockFrom"
    , "::FX::QIODevice::writeBlock"
    , "::FX::QIODevice::writeBlockTo"
    , "::FX::QIODevice::readLine"
    , "::FX::QMemArray::begin"
    , "::FX::QMemArray::data"
    , "::FX::QMemArray::end"
    , "::FX::QMemArray<unsigned char>::begin"
    , "::FX::QMemArray<unsigned char>::data"
    , "::FX::QMemArray<unsigned char>::operator unsigned char const *"
    , "::FX::QMemArray<unsigned char>::end"
    , "::FX::QPtrVector::data"
    , "::FX::QThread::addCleanupCall"                         # uses functor
    , "::FX::QThread::int_cancelWaiterHandle"                 # internal
    , "::FX::QThread::removeCleanupCall"                      # uses functor
    , "::FX::QTransString::langIdFunc"                        # uses function pointer
    , "::FX::Secure::TigerHash"
    , "::FX::Secure::TigerHashValue"                          # uses union
    , "::FX::TnFXAppEventLoop::executeRetCode"                # returns void *&
    , "::FX::strdup"
    , "::FX::fxstrdup"
    , "::FX::fxnamefromcolor"                                 # Modifies passed string
    , "::FX::fxtrace"                                         # uses ...
    , "::FX::fxmessage"                                       # uses ...
    , "::FX::fxwarning"                                       # uses ...
    , "::FX::fxerror"                                         # uses ...
]

declarations_aliases = [
    "as__scope_FX_scope_FXfloat__ptr_"
    , "as__scope_FX_scope_FXfloat_const__ptr_"
    , "as__scope_FX_scope_FXdouble__ptr_"
    , "as__scope_FX_scope_FXdouble_const__ptr_"
]

def find_deprecated():
    deprecated = {} #fname : [line ]
    for fname in os.listdir( settings.tnfox_include_path ):
        if fname == 'fxdefs.h':
            continue
        full_name = os.path.join( settings.tnfox_include_path, fname )
        if not os.path.isfile( full_name ):
            continue
        fobj = file( full_name, 'r' )
        for index, line in enumerate( fobj ):
            if 'FXDEPRECATEDEXT' in line:
                if not deprecated.has_key( fname ):
                    deprecated[ fname ] = []
                deprecated[ fname ].append( index + 1 ) #enumerate calcs from 0, while gccxml from 1
        fobj.close()
    return deprecated

deprecated = find_deprecated()

def is_deprecated( decl ):
    global deprecated
    if None is decl.location:
        return False
    file_name = os.path.split( decl.location.file_name )[1]
    return deprecated.has_key( file_name ) and decl.location.line in deprecated[ file_name ]
        
def is_excluded( decl ):
    fullname = declarations.full_name( decl )
    return fullname in to_be_excluded or is_deprecated( decl )
