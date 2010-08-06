#! /usr/bin/python
# Copyright 2004 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import os
from environment import settings
from pyplusplus import code_creators

file_name = "image_data_iterators.pypp.hpp"

classes = [
      "::FX::FXBMPIcon"
    , "::FX::FXBMPImage"
    , "::FX::FXGIFIcon"
    , "::FX::FXGIFImage"
    , "::FX::FXICOIcon"
    , "::FX::FXICOImage"
    , "::FX::FXIcon"
    , "::FX::FXIFFIcon"
    , "::FX::FXIFFImage"
    , "::FX::FXImage"
    , "::FX::FXJPGIcon"
    , "::FX::FXJPGImage"
    , "::FX::FXPCXIcon"
    , "::FX::FXPCXImage"
    , "::FX::FXPNGIcon"
    , "::FX::FXPNGImage"
    , "::FX::FXPPMIcon"
    , "::FX::FXPPMImage"
    , "::FX::FXRASIcon"
    , "::FX::FXRASImage"
    , "::FX::FXRGBIcon"
    , "::FX::FXRGBImage"
    , "::FX::FXTGAIcon"
    , "::FX::FXTGAImage"
    , "::FX::FXTIFIcon"
    , "::FX::FXTIFImage"
    , "::FX::FXXBMIcon"
    , "::FX::FXXBMImage"
    , "::FX::FXXPMIcon"
    , "::FX::FXXPMImage"
]

def create_file_content():
    content = []
    content.append( '#ifndef __image_data_iterators_pypp_hpp__' )
    content.append( '#define __image_data_iterators_pypp_hpp__' )
    content.append( '' )
    content.append( '#include "fx.h"' )
    content.append( '' )
    content.append( '#include "CArrays.h"' )
    content.append( '' )
    content.append( 'namespace FX{ ' )
    content.append( '' )
    for class_ in classes:
        content.append( "DEFINE_MAKECARRAYITER(%(class_name)s, ::FX::FXColor, getData, (), (c.getWidth()*c.getHeight()))"
                        % dict( class_name=class_[6:] ) )
        content.append( '' )
    content.append( '}' )        
    content.append( '#endif//__image_data_iterators_pypp_hpp__' )
    return os.linesep.join( content )

class register_color_array_t( code_creators.custom_text_t ):
    TMPL = "boost::python::def( 'getData', ,&::%(class_name)s_getData );"
    def __init__( self, class_name ):
        code_creators.custom_text_t.__init__( self, self.TMPL % dict( class_name=class_name ) )
        self.works_on_instance = False
