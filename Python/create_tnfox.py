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
    fx_ns.decls( lambda decl: decl.name.startswith('FXIPCMsgHolder') ).exclude()
    fx_ns.namespace( 'Pol' ).exclude()
    fx_ns.decls( files_to_exclude.is_excluded ).exclude()
    fx_ns.class_( 'QValueList<FX::Pol::knowReferrers::ReferrerEntry>').exclude()
    try:
        fx_ns.variables( 'metaClass').exclude()
    except: pass
    try:
        fx_ns.class_( 'QPtrVector<FX::Generic::BoundFunctorV>').exclude()
    except: pass
    #Niall? wrapper for this function could not be compiled
    #TnFXSQLDBStatement = fx_ns.class_( 'TnFXSQLDBStatement' )
    #TnFXSQLDBStatement.member_function( name='bind', arg_types=[None,None,None] ).exclude()

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

    # Fix up default parameter use of null strings
    for func in mb.calldefs():
        for arg in func.arguments:
            if arg.default_value == 'FX::FXString::null':
                arg.default_value = 'FX::FXString::nullStr()'

    # Insert custom code into wrappers for FXObject subclasses
    fxobject = mb.class_( 'FXObject' )
    fxobjectclasses = mb.classes( lambda decl: decl==fxobject or declarations.is_base_and_derived( fxobject, decl ) )
    for fxobjectclass in fxobjectclasses:
        # Redirect handle() to point to python dispatcher
        fxobjectclass.member_function( 'handle' ).exclude()
        fxobjectclass.add_code( 'def("handle", &::FX::'+fxobjectclass.name+'::handle, &'+fxobjectclass.name+'_wrapper::default_handle, bp::default_call_policies() )' )

        # Make it so TnFOX objects and their python mirrors can be detached
        fxobjectclass.held_type = 'std::auto_ptr<'+fxobjectclass.name+'_wrapper>'

        # Add in runtime support for detaching and msg handling
        fxobjectclass.set_constructors_body( '    FX::FXPython::int_pythonObjectCreated((int_decouplingFunctor=FX::Generic::BindObjN(*this, &FX::'+fxobjectclass.name+'::decouplePythonObject)));' )
        fxobjectclass.add_wrapper_code("""    // TnFOX runtime support
    virtual long int handle( ::FX::FXObject * sender, ::FX::FXSelector sel, void * ptr ) {
        if( bp::override func_handle = this->get_override( "handle" ) )
            return func_handle( boost::python::ptr(sender), sel, ptr );
        else
            return default_handle( sender, sel, ptr );
    }
    
    long int default_handle( ::FX::FXObject * sender, ::FX::FXSelector sel, void * ptr ) {
        long ret=0;
        if( FX::FXPython::int_FXObjectHandle( &ret, this, sender, sel, ptr ) )
            return ret;
        return FX::"""+fxobjectclass.name+"""::handle( sender, sel, ptr );
    }


    ~"""+fxobjectclass.name+"""_wrapper( ) {
        FX::FXPython::int_pythonObjectDeleted(int_decouplingFunctor);
        using namespace boost::python;
        using namespace std;
        PyObject *py_self=get_owner(*this);
        // We must be careful not to reinvoke object destruction!
        py_self->ob_refcnt=1<<16;
        {
            object me(boost::python::handle<>(borrowed(py_self)));
            auto_ptr<"""+fxobjectclass.name+"""_wrapper> &autoptr=extract<auto_ptr<"""+fxobjectclass.name+"""_wrapper> &>(me);
            autoptr.release();
        }
        py_self->ob_refcnt=0;
    }

    virtual void *getPythonObject() const {
        return (void *) get_owner(*this);
    }
    virtual void decouplePythonObject() const {
        using namespace boost::python;
        using namespace std;
        PyObject *py_self=get_owner(*this);
        object me(boost::python::handle<>(borrowed(py_self)));
        auto_ptr<"""+fxobjectclass.name+"""_wrapper> &autoptr=extract<auto_ptr<"""+fxobjectclass.name+"""_wrapper> &>(me);
        autoptr.reset(0);
    }
private:
    Generic::BoundFunctorV *int_decouplingFunctor;""")

    # Patch in custom FXApp::init() implementation
    fxapp = mb.class_( 'FXApp' )
    fxapp.member_functions( 'init' ).exclude()
    fxapp.add_code( 'def("init", &FXApp_init)' )
    fxapp.add_code( 'def("init", &FXApp_init2)' )
    fxapp.add_code( 'def("getArgv", &FXApp_getArgv)' )

    # Patch in custom FXGLTriangleMesh implementations
    fxgltrianglemesh = mb.class_( 'FXGLTriangleMesh' )
    fxgltrianglemesh.member_functions( 'getVertexBuffer' ).exclude()
    fxgltrianglemesh.add_code   ( 'def("getVertexBuffer",       &FXGLTriangleMesh_getVertexBuffer)' )
    fxgltrianglemesh.member_functions( 'getColorBuffer' ).exclude()
    fxgltrianglemesh.add_code   ( 'def("getColorBuffer",        &FXGLTriangleMesh_getColorBuffer)' )
    fxgltrianglemesh.member_functions( 'getNormalBuffer' ).exclude()
    fxgltrianglemesh.add_code   ( 'def("getNormalBuffer",       &FXGLTriangleMesh_getNormalBuffer)' )
    fxgltrianglemesh.member_functions( 'getTextureCoordBuffer' ).exclude()
    fxgltrianglemesh.add_code   ( 'def("getTextureCoordBuffer", &FXGLTriangleMesh_getTextureCoordBuffer)' )

    # Patch in custom FXGLViewer implementations
    fxglviewer = mb.class_( 'FXGLViewer' )
    fxglviewer.member_functions( 'lasso' ).exclude()
    fxglviewer.add_code   ( 'def("lasso",  &FXGLViewer_lasso, bp::return_value_policy< bp::manage_new_object, bp::default_call_policies >() )' )
    fxglviewer.member_functions( 'select' ).exclude()
    fxglviewer.add_code   ( 'def("select", &FXGLViewer_select, bp::return_value_policy< bp::manage_new_object, bp::default_call_policies >() )' )

    # Patch image & bitmap getData() functions
    getdatafuncs = mb.calldefs( lambda decl: decl.name == 'getData' and not declarations.is_void_pointer( decl.return_type ) and ('Icon' in decl.parent.name or 'Image' in decl.parent.name or 'Bitmap' in decl.parent.name) )
    getdatafuncs.exclude()
    for getdatafunc in getdatafuncs:
        getdatafunc.parent.add_code( 'def("getData", &'+getdatafunc.parent.name+'_getData)' )
    
    # Patch sort functions
    sortfuncs = mb.calldefs( lambda decl: 'SortFunc' in decl.name and 'set' in decl.name )
    for sortfunc in sortfuncs:
        sortfunc.parent.add_code( 'def("'+sortfunc.name+'", &FX::FXPython::set'+sortfunc.parent.name+'SortFunc)' )
    
    # Patch QIODevice wrapper with missing pure virtual functions
    qiodevice = mb.class_( 'QIODevice' )
    qiodevice.add_wrapper_code("""    virtual ::FX::FXuval readBlock( char * data, ::FX::FXuval maxlen ){
        bp::override func_readBlock = this->get_override( "readBlock" );
        return func_readBlock( data, maxlen );
    }

    virtual ::FX::FXuval writeBlock( char const * data, ::FX::FXuval maxlen ){
        bp::override func_writeBlock = this->get_override( "writeBlock" );
        return func_writeBlock( data, maxlen );
    }

    virtual ::FX::FXuval readBlockFrom( char * data, ::FX::FXuval maxlen, ::FX::FXfval pos ){
        bp::override func_readBlockFrom = this->get_override( "readBlockFrom" );
        return func_readBlockFrom( data, maxlen, pos );
    }

    virtual ::FX::FXuval writeBlockTo( ::FX::FXfval pos, char const * data, ::FX::FXuval maxlen ){
        bp::override func_writeBlockTo = this->get_override( "writeBlockTo" );
        return func_writeBlockTo( pos, data, maxlen );
    }
""")
    qiodevices = mb.class_( 'QIODeviceS' )
    qiodevices.add_wrapper_code("""    virtual ::FX::FXuval readBlock( char * data, ::FX::FXuval maxlen ){
        bp::override func_readBlock = this->get_override( "readBlock" );
        return func_readBlock( data, maxlen );
    }

    virtual ::FX::FXuval writeBlock( char const * data, ::FX::FXuval maxlen ){
        bp::override func_writeBlock = this->get_override( "writeBlock" );
        return func_writeBlock( data, maxlen );
    }

    virtual void *int_getOSHandle() const{
        return 0;
    }
""")


def customize_module( mb ):   
    extmodule = mb.code_creator
    extmodule.license = customization_data.license

    # Insert a custom init call
    extmodule.adopt_creator( code_creators.custom_text_t('extern void InitialiseTnFOXPython();\n\n'), len(extmodule.creators)-1)
    extmodule.body.adopt_creator( code_creators.custom_text_t('    InitialiseTnFOXPython();\n\n'), 0)

    # Remove all standard headers in favour of precompiled header
    includes = filter( lambda creator: isinstance( creator, code_creators.include_t )
                        , extmodule.creators )
    map( lambda creator: extmodule.remove_creator( creator ), includes )
    position = extmodule.last_include_index() + 1
    extmodule.adopt_creator( code_creators.namespace_using_t('::FX'), position )
    extmodule.user_defined_directories.append( settings.generated_files_dir )
    extmodule.adopt_include( code_creators.include_t( header='../common.h' ) )    
    extmodule.precompiled_header = '../common.h'
    extmodule.adopt_include( code_creators.include_t( header='../patches.cpp.h' ) )    

    # Fix bug in gccxml where default args with function as value gain an extra ()
    try:
        constr = mb.constructor( 'FXPrimaryButton', arg_types=[None]*15 )
        constr.arguments[10].default_value = '(FX::FXWindow::defaultPadding() * 4)'
        constr.arguments[11].default_value = '(FX::FXWindow::defaultPadding() * 4)'
    except: pass

    # Patch default args with enums to be number (to avoid undeclared enum problem)
    def args_declaration_wa( self ):
        args = []
        for index, arg in enumerate( self.declaration.arguments ):
            result = arg.type.decl_string + ' ' + self.argument_name(index)
            if arg.default_value:
                result += '=%s' % arg.wrapper_default_value
            args.append( result )
        if len( args ) == 1:
            return args[ 0 ]
        return ', '.join( args )

    code_creators.calldef.calldef_wrapper_t.args_declaration = args_declaration_wa 

    allfuncs = mb.calldefs()
    for func in allfuncs:
        #print type(func), type(func.parent), func
        for arg in func.arguments:
            if not arg.default_value:
                continue
            arg.wrapper_default_value = arg.default_value
            if not declarations.is_enum( arg.type ):
                continue
            enum_ = declarations.enum_declaration( arg.type )
            if isinstance( enum_.parent, declarations.namespace_t ):
                continue #global enum
            # Be lazy, and just lookup the last part
            value = arg.default_value[ arg.default_value.rfind('::')+2: ]
            arg.default_value = arg.type.declaration.values[ value ] + '/*' + arg.default_value + '*/'


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
