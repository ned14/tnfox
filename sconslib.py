#********************************************************************************
#                                                                               *
#                              TnFOX scons library                              *
#                                                                               *
#********************************************************************************
#        Copyright (C) 2003 by Niall Douglas.   All Rights Reserved.            *
#       NOTE THAT I DO NOT PERMIT ANY OF MY CODE TO BE PROMOTED TO THE GPL      *
#********************************************************************************
# This code is free software; you can redistribute it and/or modify it under    *
# the terms of the GNU Library General Public License v2.1 as published by the  *
# Free Software Foundation EXCEPT that clause 3 does not apply ie; you may not  *
# "upgrade" this code to the GPL without my prior written permission.           *
# Please consult the file "License_Addendum2.txt" accompanying this file.       *
#                                                                               *
# This code is distributed in the hope that it will be useful,                  *
# but WITHOUT ANY WARRANTY; without even the implied warranty of                *
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                          *
#********************************************************************************

import os
import sys
import string
import datetime

def libAsLA(leaf=False):
    ret=str(DLL)
    ret=ret[:string.find(ret, ".so")]+".la"
    if leaf: ret=ret[string.rfind(ret, '/')+1:]
    return ret

def VersionedSharedLibraryName(env, target, version, debug=False):
    ABIversion, interface, sourcev, backwards=version
    if ABIversion: target+="-"+ABIversion
    suffix=""
    if debug: target+="d"
    # Hack for scons misinterpreting '.' in target name
    if sys.platform=="win32":
        suffix+=".dll"
    else:
        suffix+=".so"
        suffix+=".%d.%d.%d" % (interface, sourcev, backwards)
    return target,suffix

def VersionedSharedLibrary(env, target, version, installpath, sources, debug=False, genstaticlib=False, **args):
    """Produces a versioned shared library on POSIX and Windows platforms.
    version is a tuple of (ABI string, interface rev, source rev,
    backwards compatible level (usually same as interface rev) ).
    installpath is used only on GNU and is /usr/lib or /usr/local/lib etc."""
    target=os.path.abspath(target)
    targetorig=target
    ABIversion, interface, sourcev, backwards=version
    target,suffix=VersionedSharedLibraryName(env, target, version, debug)
    libtool=sys.platform!="win32"
    DLL=env.SharedLibrary(target+suffix, sources, SHLIBSUFFIX=suffix, **args)
    basetarget=str(DLL[0])
    print "basetarget=",basetarget
    stem,ext=os.path.split(targetorig)
    mpos=string.rfind(basetarget, '-')
    lpos=string.rfind(basetarget, os.sep)
    dpos=string.find(basetarget, '.')
    sopos=string.find(basetarget, '.', dpos+1)
    vIpos=string.find(basetarget, '.', sopos+1)
    vSpos=string.find(basetarget, '.', vIpos+1)
    vBpos=string.find(basetarget, '.', vSpos+1)
    LAIname=os.path.join(stem, basetarget[lpos+1:sopos]+".la")
    def genLAIfile(target, source, env):
        print "Generating LAI file at",LAIname
        oh=file(LAIname, "wt")
        try:
            
            oh.write("# "+LAIname+" - a libtool library file\n")
            oh.write("# Generated by scons for libtool ("+datetime.datetime.now().isoformat()+")\n#\n")
            oh.write("# Please DO NOT delete this file!\n# It is necessary for linking the library.\n\n")
            oh.write("# The name that we can dlopen(3).\n")
            oh.write("dlname='"+basetarget[lpos+1:vSpos]+"'\n\n")
            oh.write("# Names of this library.\n")
            oh.write("library_names='"+basetarget[lpos+1:]+" "+basetarget[lpos+1:vSpos]+" "+basetarget[lpos+1:vIpos]+"'\n\n")
            oh.write("# The name of the static archive.\n")
            oh.write("old_library='"+basetarget[lpos+1:sopos]+".a'\n\n")
            oh.write("# Libraries that this one depends upon.\n")
            oh.write("dependency_libs='")
            for lib in env['LIBS']:
                oh.write(" -l"+lib)
            oh.write("'\n\n")
            oh.write("# Version information for libtnfox.\n")
            oh.write("current=%d\nage=%d\nrevision=%d\n\n" % (interface, backwards, sourcev))
            oh.write("# Is this an already installed library?\n")
            oh.write("installed=no\n\n")
            oh.write("# Files to dlopen/dlpreopen\ndlopen=''\ndlpreopen=''\n\n")
            oh.write("# Directory that this library needs to be installed in:\n")
            oh.write("libdir='"+installpath+"'\n")
        finally:
            oh.close()
        libsdir=os.path.join(stem, ".libs")
        try:
            os.mkdir(libsdir)
        except: pass
        LAIloc=os.path.join(libsdir, basetarget[lpos+1:sopos]+".lai")
        try:
            os.unlink(LAIloc)
        except: pass
        os.link(LAIname, LAIloc)

    def dupLibrary(target, source, env):
        target=str(target[0])
        if os.path.getsize(target)<10240: # Has libtool output into .libs?
            return
        basetarget=target[string.rfind(target, os.path.sep)+1:]
        libsbase=os.path.abspath(os.path.join(stem, ".libs"))
        libsloc=os.path.join(libsbase, basetarget)
        print "Linking to", target, "from", libsloc, "libsbase=", libsbase, "basetarget=",basetarget
        try:
            os.mkdir(libsbase)
        except: pass
        try:
            os.unlink(libsloc)
        except: pass
        os.link(target, libsloc)
    def dupLibrary2(target, source, env):
        libsbase=os.path.abspath(os.path.join(stem, ".libs"))
        try:
            os.unlink(os.path.join(libsbase, basetarget[lpos+1:vSpos]))
        except: pass
        os.symlink(basetarget[lpos+1:], os.path.join(libsbase, basetarget[lpos+1:vSpos]))
        try:
            os.unlink(os.path.join(libsbase, basetarget[lpos+1:vIpos]))
        except: pass
        os.symlink(basetarget[lpos+1:], os.path.join(libsbase, basetarget[lpos+1:vIpos]))

    if libtool or genstaticlib:
        LIB=env.StaticLibrary(target+".a", sources, LIBSUFFIX=".a", **args)
        env.Depends(DLL, LIB)
    if libtool:
        AddPostAction(LIB, Action(dupLibrary))
        AddPreAction(DLL, Action(genLAIfile))
        Clean(DLL, LAIname)
        AddPostAction(DLL, Action(dupLibrary))
        AddPostAction(DLL, Action(dupLibrary2))
        Clean(DLL, os.path.join(stem, ".libs"))
    return DLL


#********************************************************************************

def ternary(val, A, B):
    if val:
        return A
    else:
        return B

def getBase(file):
    base,ext=os.path.splitext(file)
    base,leaf=os.path.split(base)
    # If it's in a dir other than src, include that
    if len(base)>3:
        leaf=os.path.join(base[ternary(base[:3]=="src", 4, 0):], leaf)
    return leaf
    
def init(cglobals, prefixpath="", platprefix="", targetversion=0, tcommonopts=0):
    execfile(prefixpath+"config.py")    # Sets debugmode, version, libtnfoxname
    for key,value in locals().items():
        globals()[key]=value
    if targetversion==0: targetversion=tnfoxversion
    compiler=ARGUMENTS.get("compiler", None)
    global toolset
    if compiler:
        toolset=[compiler]
    if toolset:
        env=Environment(tools=toolset)
    else:
        env=Environment()
    for tool in env['TOOLS']:
        platformconfig=platprefix+"config/"+tool+".py"
        if os.path.isfile(platformconfig): break
    if not os.path.isfile(platformconfig):
        raise IOError, "No platform config file for any of "+repr(env['TOOLS'])+" was found"
    print "Using platform configuration",platformconfig,"..."
    onWindows=(env['PLATFORM']=="win32")
    if tcommonopts:
        libtcommon,libtcommonsuffix=VersionedSharedLibraryName(env, tncommonname, tncommonversioninfo, debug=debugmode)
        libtcommon2,libtcommonsuffix=VersionedSharedLibraryName(env, tncommonname+"Tn", tncommonversioninfo, debug=debugmode)
    else:
        libtnfox,libtnfoxsuffix=VersionedSharedLibraryName(env, tnfoxname, tnfoxversioninfo, debug=debugmode)
    if debugmode:
        builddir="Debug_"+tool
    else:
        builddir="Release_"+tool
    env['CPPDEFINES']=[ "FOXDLL" ]
    env['CPPPATH']=[ prefixpath+"include" ]
    env['LIBPATH']=[ ]
    env['LIBS']=[ ]
    env['CCWPOOPTS']=[ ]
    execfile(platformconfig)
    if sys.byteorder=="big": env['CPPDEFINES']+=[("FOX_BIGENDIAN",1)]
    if make64bit: env['CPPDEFINES']+=["FX_IS64BITPROCESSOR"]
    if makeSMPBuild: env['CPPDEFINES']+=["FX_SMPBUILD"]
    if inlineMutex: env['CPPDEFINES']+=["FXINLINE_MUTEX_IMPLEMENTATION"]
    if debugmode: env['CPPDEFINES']+=["FXDISABLE_GLOBALALLOCATORREPLACEMENTS"]
    if architecture=="i486":
        env['CPPDEFINES']+=[("FX_X86PROCESSOR", i486_version)]
    if tcommonopts==2: env['CPPDEFINES']+=["BUILDING_TCOMMON"]
    for key,value in locals().items():
        cglobals[key]=value

def getTnFOXSources(prefix="", getWPOfiles=False):
    filelist=os.listdir(prefix+"src")
    idx=0
    while idx<len(filelist):
        type=filelist[idx][-4:]
        if type!=ternary(getWPOfiles, ".cxx", ".cpp"):
            del filelist[idx]
        elif not onWindows and filelist[idx]=="vsscanf.cpp":
            del filelist[idx]   # Causes issues with glibc
        else: idx+=1
    return filelist
def getTnFOXIncludes(prefix=""):
    filelist=os.listdir(prefix+"include")
    idx=0
    while idx<len(filelist):
        type=filelist[idx][-2:]
        if type!=".h":
            del filelist[idx]
        else: idx+=1
    return filelist
   
def doConfTests(env, prefixpath=""):
    conf=Configure(env)
    def checkLib(conf, name, header, prefix=""):
        capsname=string.upper(name)
        if os.path.exists(prefixpath+"windows/"+prefix+"lib"+name):
            print "Found "+capsname+" library"
            conf.env['CPPDEFINES']+=["HAVE_"+capsname+"_H"]
            conf.env['CPPPATH']+=[prefixpath+"windows/"+prefix+"lib"+name]
            conf.env['LIBS']+=[prefixpath+"windows/lib"+name+"/lib"+name]
        elif conf.CheckCHeader(["stdio.h", header]):
            conf.env['CPPDEFINES']+=["HAVE_"+capsname+"_H"]
            conf.env['LIBS']+=[name]
        else:
            print capsname+" library not found, disabling support"
    checkLib(conf, "jpeg", "jpeglib.h")
    checkLib(conf, "png", "png.h")
    checkLib(conf, "tiff", "tiff.h", "libtiff/")
    if os.path.exists(prefixpath+"windows/zlib"):
        print "Found ZLib"
        conf.env['CPPDEFINES']+=["HAVE_ZLIB_H"]
        conf.env['CPPPATH']+=[prefixpath+"windows/zlib"]
        conf.env['LIBS']+=[prefixpath+"windows/zlib/zlib"]
    elif conf.CheckCHeader("zlib.h"):
        conf.env['CPPDEFINES']+=["HAVE_ZLIB_H"]
        conf.env['LIBS']+=["z"]
    else:
        print "ZLib library not found, disabling support"

    if os.path.exists(prefixpath+"../openssl"):
        print "Found OpenSSL library"
        conf.env['CPPDEFINES']+=["HAVE_OPENSSL"]
        conf.env['CPPPATH']+=[prefixpath+"../openssl/inc32"]
        conf.env['LIBPATH']+=[prefixpath+"../openssl/out32dll"]
        conf.env['LIBS']+=["libeay32", "ssleay32"]
    elif conf.CheckLibWithHeader("ssl", ["openssl/ssl.h"], "c"):
        conf.env['CPPDEFINES']+=["HAVE_OPENSSL"]
        conf.env['LIBS']+=["crypto"]
    else:
        print "OpenSSL library not found, disabling support"

    if conf.CheckLibWithHeader(ternary(onWindows, "opengl32", "GL"), ternary(onWindows, ["windows.h"], [])+["GL/gl.h"], "c", "glBegin(GL_POINTS);", True):
        conf.env['CPPDEFINES']+=["HAVE_GL_H",
                                 "SUN_OGL_NO_VERTEX_MACROS",
                                 "HPOGL_SUPPRESS_FAST_API"]
        if conf.CheckCXXHeader(ternary(onWindows, ["windows.h"], [])+["GL/glu.h"]):
            conf.env['CPPDEFINES']+=["HAVE_GLU_H"]
            conf.env['LIBS']+=[ternary(onWindows, "glu32", "GLU")]
        else:
            print "GLU library not found, disabling support"
    else:
        print "GL library not found, disabling support"

    env=conf.Finish()

def addBind(DLL):
    if onWindows:
        def runBind(target, source, env):
            stem, leaf=os.path.split(str(target[0]))
            oldpath=os.getcwd()
            try:
                os.chdir(stem)
                os.system('bind -u '+leaf)
            finally:
                os.chdir(oldpath)
        AddPostAction(DLL, Action(runBind))

def genSynopsis(headersloc, headers):
    syns=["Synopsis_syn/"+x[:string.find(x, '.')]+".syn" for x in headers]
    for idx in range(0, len(headers)):
        os.system("synopsis -p Cxx -D__GNUC__=1 -I"+headersloc+" -o "+syns[idx]+" "+headersloc+"/"+headers[idx])
    return syns

def callSynopsis(syns):
    print "Linking synopsis AST files"
    cmd="synopsis \"-Wl,comment_processors(['qt','java','summary'])\" -o Synopsis_syn/combined.syn "+string.join(syns, " ")
    #print cmd
    os.system(cmd)
    print "Generating HTML"
    os.system("synopsis -f HTML -o Synopsis Synopsis_syn/combined.syn")

    