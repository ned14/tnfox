#! /usr/bin/python
# Copyright 2004 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import sys, os

class settings:    
    module_name='TnFOX'
    boost_path = ''
    boost_libs_path = ''
    gccxml_path = '' 
    pygccxml_path = ''
    pyplusplus_path = ''
    tnfox_path = ''
    tnfox_include_path = ''
    tnfox_python_include_path = ''
    tnfox_libs_path = ''
    python_libs_path = ''
    python_include_path = ''
    working_dir = ''
    generated_files_dir = ''
    unittests_dir = ''
    xml_files = ''
    defined_symbols = [ "FXDISABLE_GLOBALALLOCATORREPLACEMENTS"
                        , "FX_INCLUDE_ABSOLUTELY_EVERYTHING"
                        , "FOXPYTHONDLL_EXPORTS"
                        , "FX_NO_GLOBAL_NAMESPACE" ]
    # For debugging purposes to get far smaller bindings, you can define FX_DISABLEGUI
    #defined_symbols.append("FX_DISABLEGUI=1")
    
    if 'big'==sys.byteorder:
        defined_symbols.append("FOX_BIGENDIAN=1")
    else:
        defined_symbols.append("FOX_BIGENDIAN=0")
    if 'win32'==sys.platform or 'win64'==sys.platform:
        defined_symbols.append("WIN32")
    defined_symbols_gccxml = defined_symbols + ["__GCCXML__"
                        , "_NATIVE_WCHAR_T_DEFINED=1" ]
            
    def setup_environment():
        sys.path.append( settings.pygccxml_path )
        sys.path.append( settings.pyplusplus_path )
    setup_environment = staticmethod(setup_environment)
   
rootdir=os.path.normpath(os.getcwd()+'/../..')
settings.boost_path = rootdir+'/boost'
settings.boost_libs_path = rootdir+'/boost/libs'
#settings.gccxml_path = ''
#settings.pygccxml_path = ''
#settings.pyplusplus_path = ''
settings.python_include_path = os.getenv('PYTHON_INCLUDE')
settings.python_libs_path = os.getenv('PYTHON_ROOT')+'/libs'
settings.tnfox_path = rootdir+'/TnFOX'
settings.tnfox_include_path = rootdir+'/TnFOX/include'
settings.tnfox_libs_path = rootdir+'/TnFOX/lib'
settings.working_dir = os.getcwd()
settings.generated_files_dir = rootdir+'/TnFOX/Python/generated'
settings.unittests_dir = rootdir+'/TnFOX/Python/unittests'

settings.setup_environment()
