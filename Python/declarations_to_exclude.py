#! /usr/bin/python
# Copyright 2004 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import os
from environment import settings
from pygccxml import declarations

to_be_excluded = [
      "::FX::FXACL::int_toWin32SecurityDescriptor"
    , "::FX::FXApp::dispatchEvent"
    , "::FX::FXApp::getDisplay"
    , "::FX::FXApp::getNextEvent"
    , "::FX::FXComboBox::getSortFunc"
    , "::FX::FXComboBox::setSortFunc"
    , "::FX::FXDataTarget::addUpcall"
    , "::FX::FXDC::context"
    , "::FX::FXDCPrint::outf"
    , "::FX::FXDCPrint::outhex"
    , "::FX::FXException::int_enableNestedExceptionFramework"
    , "::FX::FXFile::info"
    , "::FX::FXFile::linkinfo"
    , "::FX::FXFunctorTarget::functor"
    , "::FX::FXFunctorTarget::setFunctor"
    , "::FX::FXGLCanvas::getContext"
    , "::FX::FXGLViewer::getZSortFunc"
    , "::FX::FXGLViewer::setZSortFunc"
    , "::FX::FXIconList::getSortFunc"
    , "::FX::FXIconList::setSortFunc"
    , "::FX::FXId::id"
    , "::FX::FXImage::setData"
    , "::FX::FXImage::getData" #Niall?
    , "::FX::FXIPCMsg::originalData"
    , "::FX::FXListBox::getSortFunc"
    , "::FX::FXListBox::setSortFunc"
    , "::FX::FXList::getSortFunc"
    , "::FX::FXList::setSortFunc"
    , "::FX::FXMessageBox::error"
    , "::FX::FXMessageBox::information"
    , "::FX::FXMessageBox::question"
    , "::FX::FXMessageBox::warning"
    , "::FX::FXMetaClass::FXMetaClass"
    , "::FX::FXMetaClass::search"
    , "::FX::FXObject::decouplePythonObject"
    , "::FX::FXObject::FXMapEntry::func"
    , "::FX::FXObject::getPythonObject"
    , "::FX::FXObject::metaClass"
    , "::FX::FXProcess::dllResolveBase"
    , "::FX::FXSettings::readFormatEntry"
    , "::FX::FXSettings::writeFormatEntry"
    , "::FX::FXSQLDBCursor::resultsLatch"
    , "::FX::FXStream::readBytes"
    , "::FX::FXStream::readRawBytes"
    , "::FX::FXStream::writeBytes"
    , "::FX::FXStream::writeRawBytes"
    , "::FX::FXTreeListBox::getSortFunc"
    , "::FX::FXTreeListBox::setSortFunc"
    , "::FX::FXTreeList::getSortFunc"
    , "::FX::FXTreeList::setSortFunc"
    , "::FX::FXWindow::addColormapWindows"
    , "::FX::FXWindow::GetClass"
    , "::FX::FXWindow::GetDC"
    , "::FX::FXWindow::ReleaseDC"
    , "::FX::FXWindow::remColormapWindows"
    , "::FX::QHostAddress::ip6Addr"
    , "::FX::QIODevice::readLine"
    , "::FX::QMemArray::begin"
    , "::FX::QMemArray::data"
    , "::FX::QMemArray::end"
    , "::FX::QMemArray<unsigned char>::begin"
    , "::FX::QMemArray<unsigned char>::data"
    , "::FX::QMemArray<unsigned char>::end"
    , "::FX::QMemMap::mapOffset"
    , "::FX::QPtrVector::data"
    , "::FX::QThread::addCleanupCall"
    , "::FX::QThread::int_cancelWaiterHandle"
    , "::FX::QThread::removeCleanupCall"
    , "::FX::QTransString::langIdFunc"
    , "::FX::strdup"
    , "::FX::fxtrace"
    , "::FX::fxmessage"
    , "::FX::fxwarning"
    , "::FX::fxerror"
    
    #Will be replaced with custom function
    , "::FX::FXApp::getArgv" 
    , "::FX::FXBitmap::getData"
    , "::FX::FXGLTriangleMesh::getVertexBuffer"
    , "::FX::FXGLTriangleMesh::getColorBuffer"
    , "::FX::FXGLTriangleMesh::getNormalBuffer"
    , "::FX::FXGLTriangleMesh::getTextureCoordBuffer"
    , "::FX::FXObjectList::list"
    , "::FX::FXBMPIcon::getData"
    , "::FX::FXBMPImage::getData"
    , "::FX::FXGIFIcon::getData"
    , "::FX::FXGIFImage::getData"
    , "::FX::FXICOIcon::getData"
    , "::FX::FXICOImage::getData"
    , "::FX::FXIcon::getData"
    , "::FX::FXIFFIcon::getData"
    , "::FX::FXIFFImage::getData"
    , "::FX::FXImage::getData"
    , "::FX::FXJPGIcon::getData"
    , "::FX::FXJPGImage::getData"
    , "::FX::FXPCXIcon::getData"
    , "::FX::FXPCXImage::getData"
    , "::FX::FXPNGIcon::getData"
    , "::FX::FXPNGImage::getData"
    , "::FX::FXPPMIcon::getData"
    , "::FX::FXPPMImage::getData"
    , "::FX::FXRASIcon::getData"
    , "::FX::FXRASImage::getData"
    , "::FX::FXRGBIcon::getData"
    , "::FX::FXRGBImage::getData"
    , "::FX::FXTGAIcon::getData"
    , "::FX::FXTGAImage::getData"
    , "::FX::FXTIFIcon::getData"
    , "::FX::FXTIFImage::getData"
    , "::FX::FXXBMIcon::getData"
    , "::FX::FXXBMImage::getData"
    , "::FX::FXXPMIcon::getData"
    , "::FX::FXXPMImage::getData"
    #Niall?
    , "::FX::FXEventLoop"
    , "::FX::TnFXAppEventLoop::executeRetCode" 
    , "::FX::QIODevice"
    , "::FX::QIODeviceS"
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
    return declarations.full_name( decl ) in to_be_excluded or is_deprecated( decl )
