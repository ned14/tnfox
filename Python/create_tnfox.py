#! /usr/bin/python
# Copyright 2004 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import os, sys
import time
from environment import settings

from pygccxml import parser
from pygccxml import declarations
from pyplusplus import code_creators
from pyplusplus import module_creator
from pyplusplus import file_writers
from pyplusplus import utils as pypp_utils
from pyplusplus import decl_wrappers
from pyplusplus import module_builder

import aliases
import color_array
import call_policies
import files_to_exclude
import customization_data
import declarations_to_exclude

def filter_decls(mb):
    mb.global_ns.exclude()
    fx_ns = mb.namespace( 'FX' )
    fx_ns.include()
    fx_ns.decls( declarations_to_exclude.is_excluded ).exclude()
    #fx_ns.decls( lambda decl: decl.name.startswith('FXIPCMsg') ).exclude()
    fx_ns.decls( lambda decl: decl.name.startswith('FXIPCMsgHolder') ).exclude()
    fx_ns.namespace( 'Pol' ).exclude()
    fx_ns.decls( files_to_exclude.is_excluded ).exclude()
    fx_ns.class_( 'QValueList<FX::Pol::knowReferrers::ReferrerEntry>').exclude()
    fx_ns.variables( 'metaClass').exclude()
    try:
        fx_ns.class_( 'QPtrVector<FX::Generic::BoundFunctorV>').exclude()
    except: pass
    #Niall? wrapper for this function could not be compiled
    #FXSQLDBStatement = fx_ns.class_( 'FXSQLDBStatement' )
    #FXSQLDBStatement.member_function( name='bind', arg_types=[None,None,None] ).exclude()

    for func in fx_ns.calldefs():
        #I want to exclude all functions that returns pointer to pointer
        #and returns pointer to fundamental type
        if declarations.is_pointer( func.return_type ):
            temp = declarations.remove_pointer( func.return_type )
            if declarations.is_fundamental( temp ) and not declarations.is_const(temp):
                func.exclude()
            temp = declarations.remove_cv( func.return_type )
            temp = declarations.remove_pointer( temp )
            if declarations.is_pointer( temp ):
                func.exclude()
                
    #decls = fx_ns.decls( lambda decl: decl.alias in declarations_to_exclude.declarations_aliases )
    #decls.exclude()

def set_call_policies(mb):

    #for func in mb.calldefs():
    #    if "QMemArray" in func.name:
    #        print "HERE!", func.name, func.parent.name, [x.name for x in func.arguments]
    #sys.exit(0)

    #first of all call policies defined within data base
    for fname, call_pol in call_policies.db.items():
        #print fname
        try:
            if fname.startswith( '::FX::FX' ):
                mb.member_functions( fname ).call_policies = call_pol
            else:
                mb.calldefs( fname ).call_policies = call_pol
        except:
            print "ERROR, skipping! was:",sys.exc_info()[0],sys.exc_info()[1]
       
    try:
        copy_funcs = mb.calldefs( lambda decl: 'FXGL' in decl.parent.name and decl.name == 'copy' )
        copy_funcs.call_policies = decl_wrappers.return_value_policy( decl_wrappers.manage_new_object )
    except:
        print "ERROR, skipping! was:",sys.exc_info()[0],sys.exc_info()[1]
    
    try:
        take_funcs = mb.calldefs( lambda decl: 'QPtrVector<' in decl.parent.name \
                                  and decl.name == 'take' \
                                  and declarations.is_pointer( decl.return_type ) )
        # Set reference_existing object only on the overload not returning a bool
        take_funcs.call_policies = decl_wrappers.return_value_policy( decl_wrappers.reference_existing_object )
    except:
        print "ERROR, skipping! was:",sys.exc_info()[0],sys.exc_info()[1]

    try:
        mb.calldefs( 'manufacture' ).call_policies \
            = decl_wrappers.return_value_policy( decl_wrappers.manage_new_object )
        mb.calldefs( 'getMetaClass' ).call_policies \
            = decl_wrappers.return_value_policy( decl_wrappers.reference_existing_object )
    except:
        print "ERROR, skipping! was:",sys.exc_info()[0],sys.exc_info()[1]

    #third calculated
    return_by_value = decl_wrappers.return_value_policy( decl_wrappers.return_by_value )
    return_internal_ref = decl_wrappers.return_internal_reference()
    const_t = declarations.const_t
    pointer_t = declarations.pointer_t
    #~ system_wide = {
          #~ pointer_t( declarations.char_t() ) : return_by_value
        #~ , pointer_t( declarations.wchar_t() ) : return_by_value
        #~ #used in 3/4 d/f mat/vec classes
        #~ , pointer_t( declarations.float_t() ) : return_internal_ref
        #~ , pointer_t( const_t( declarations.float_t() ) ) : return_internal_ref
        #~ , pointer_t( declarations.double_t() ) : return_internal_ref
        #~ , pointer_t( const_t( declarations.double_t() ) ) : return_internal_ref
    #~ }

    #~ for type_, policy in system_wide.items():
        #~ mb.calldefs( return_type=type_ ).call_policies = policy

    for name in ['::FX::FXVec4d', '::FX::FXVec4f', '::FX::FXVec3d', '::FX::FXVec3f', '::FX::FXVec2d', '::FX::FXVec2f', '::FX::QMemArray<unsigned char>']:
        try:
            mb.casting_operators( name ).call_policies = return_internal_ref
        except:
            print "ERROR, skipping! was:",sys.exc_info()[0],sys.exc_info()[1]

    return None

def customize_decls( mb ):
    classes = mb.classes()
    classes.always_expose_using_scope = True #better error reporting from compiler
    classes.redefine_operators = True #redefine all operators found in base classes
    for class_ in classes:
        if class_.name in aliases.db:
            class_.alias = aliases.db[ class_.name ]
            class_.wrapper_alias = class_.alias + '_wrapper'

    for func in mb.calldefs():
        for arg in func.arguments:
            if arg.default_value == 'FX::FXString::null':
                arg.default_value = 'FX::FXString::nullStr()'

def customize_module( mb ):   
    extmodule = mb.code_creator
    extmodule.license = customization_data.license
        
    includes = filter( lambda creator: isinstance( creator, code_creators.include_t )
                        , extmodule.creators )
    includes = includes[2:] #all includes except boost\python.hpp and __array_1.pypp.hpp
    map( lambda creator: extmodule.remove_creator( creator ), includes )
    extmodule.adopt_include( code_creators.include_t( header="fx.h" ) )
    position = extmodule.last_include_index() + 1
    extmodule.adopt_creator( code_creators.namespace_using_t('::FX'), position )
    extmodule.user_defined_directories.append( settings.generated_files_dir )
   
    mb.calldefs().use_keywords = False

def create_module():
    parser_config = parser.config_t( )

    fx_xml = os.path.join( settings.xml_files, 'fx.xml' )
    mb = module_builder.module_builder_t( [ parser.create_cached_source_fc( 'fx.h', fx_xml ) ]
                                          , gccxml_path=settings.gccxml_path
                                          , include_paths=[settings.boost_path, settings.tnfox_include_path]
                                          , define_symbols=settings.defined_symbols_gccxml )
    mb.run_query_optimizer()
    print 'filtering declarations'
    filter_decls( mb )
    print 'filtering declarations - done'
    print 'set call policies'
    set_call_policies( mb )
    print 'set call policies - done'
    print 'customize declarations'
    customize_decls( mb )
    print 'customize declarations - done'
    print 'creating module'
    mb.build_code_creator(settings.module_name )
    print 'creating module - done'
    print 'customizing module'
    customize_module( mb )
    print 'customizing module - done'
    return mb

def write_module( mb ):
    print 'writing module to files'
    assert isinstance( mb, module_builder.module_builder_t )
    if not os.path.exists( settings.generated_files_dir ):
        os.mkdir( settings.generated_files_dir )
    start_time = time.clock()      
    file_writers.write_file( os.path.join( settings.generated_files_dir, color_array.file_name )
                             , color_array.create_file_content())
    mb.split_module( settings.generated_files_dir )
    print 'time taken : ', time.clock() - start_time, ' seconds'
    print 'writing module to files - done'

def export():
    mb = create_module()
    write_module( mb )
    
if __name__ == '__main__':    
    export()
