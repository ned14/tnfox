#! /usr/bin/env python
#********************************************************************************
#                                                                               *
#                        TnFOX Python bindings generator                        *
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
import tempfile
import string
import threading

# Set to where the headers point
headerspath="../include"
# Set to where boost lives
boostpath="../../boost"
# Set to the number of processors this machine has
processors=1

# List of files to be excluded
excludelist=["FXArray.h", "FXDict.h",
             "FXErrCodes.h", "FXGenericTools.h", "FXHash.h",
             "FXLRUCache.h", "FXMemDbg.h", "FXMemoryPool.h", "FXPolicies.h",
             "FXPtrHold.h", "FXRefedObject.h", "FXRollback.h", "FXSecure.h",
             "FXString.h", "FXStringDict.h",
             "FXTextCodec.h", "FXURL.h", "FXUTF16Codec.h", "FXUTF32Codec.h",
             "FXUTF8Codec.h", "FXWString.h"]
# AllFromHeader() broken bug
excludelist+=["FXDLL.h", "FXElement.h", "FXHandedInterface.h", "FXIPC.h", "FXMDIButton.h"]
# Pyste and/or psyco bug?
excludelist+=["FXBZStream.h", "FXGZStream.h"]
# TnFOX deprecated classes from FOX
depreclist=["FXFileStream.h", "FXMemoryStream.h"]

# List of files with special includes
includelist={"FXApp.h"          : [ "CArrays.h" ],
             "FXBitmap.h"       : [ "CArrays.h" ],
             "FXBuffer.h"       : [ "qcstring.h" ],
             "FXException.h"    : [ "CArrays.h" ],
             "FXGLCanvas.h"     : [ "FXGLVisual.h" ],
             "FXGLContext.h"    : [ "FXGLVisual.h" ],
             "FXGLObject.h"     : [ "FXGLViewer.h" ],
             "FXGLTriangleMesh.h" : [ "CArrays.h" ],
             "FXGLViewer.h"     : [ "CArrays.h", "FXGLObject.h", "FXGLVisual.h" ],
             "FXImage.h"        : [ "CArrays.h" ],
             "FXMat4d.h"        : [ "FXQuatd.h" ],
             "FXMat4f.h"        : [ "FXQuatf.h" ],
             "FXObjectList.h"   : [ "CArrays.h" ],
             "FXProcess.h"      : [ "CArrays.h", "qvaluelist.h" ],
             "FXRanged.h"       : [ "FXSphered.h", "FXVec4d.h" ],
             "FXRangef.h"       : [ "FXSpheref.h", "FXVec3f.h", "FXVec4d.h", "FXVec4f.h" ],
             "FXSphered.h"      : [ "FXSpheref.h", "FXVec4d.h" ],
             "FXSpheref.h"      : [ "FXRangef.h", "FXVec4d.h", "FXVec4f.h" ],
             "FXTrans.h"        : [ "CArrays.h", "qvaluelist.h" ]
             }

# List of files with special additions
appendlist={"FXObject.h" : """cclass=Class('FX::FXMetaClass', 'FXObject.h')
FXObject.applyFXMetaClass(globals(), cclass)
""" }

# List of files with alternative instructions
instructlist={  "FXDLL.h" : "awaiting AllFromHeader()",
                "FXElement.h" : "awaiting AllFromHeader()",
                "FXMDIButton.h" : "awaiting AllFromHeader()"}

# Files to be floated to top to avoid dependency issues
floattoplist=[]

    
def mysystem(command):
    #os.system(command)
    #return
    print "\n"+command
    (childinh, childh)=os.popen4(command)
    while 1:
        line=childh.readline()
        if not line: break
        print "  "+line,

# Automatically scan the headers directory and pull all starting with "FX" in
module="FOX"
mypath=os.path.dirname(sys.argv[0])
filelist=os.listdir(os.path.join(mypath, headerspath))
idx=0
while idx<len(filelist):
    if filelist[idx]=="TnFOXDocs.h": module="TnFOX"
    if filelist[idx][0:2]!="FX" or filelist[idx] in excludelist: # or filelist[idx]<"FXN" or filelist[idx]>"FXM":
	del filelist[idx]
    else: idx+=1
if module=="TnFOX":
    idx=0
    while idx<len(filelist):
        if filelist[idx] in depreclist:
            del filelist[idx]
        else: idx+=1
idx=0
while idx<len(filelist):
    if filelist[idx] in floattoplist:
        temp=filelist[idx]
        del filelist[idx]
        filelist.insert(0, temp)
    idx+=1

#print "Files to be processed:",filelist
#filelist=[ "FXGLTriangleMesh.h" ]
#filelist=["FXThread.h"]
if not os.path.isdir("Policies"):
    os.mkdir("Policies")
modules=[]
try:
    for filename in filelist:
        base,ext=os.path.splitext(filename)
        outh=file(base+".pyste", "wt")
        outh.write("from Policies import "+base+"\n")
        # outh.write("Include('"+filename+"')\n")
        outh.write("PCHInclude('common.h')\n")
        if includelist.has_key(filename):
            for includefile in includelist[filename]:
                outh.write("Include('"+includefile+"')\n")
        # outh.write("cclass=AllFromHeader('"+filename+"')\n")
        if instructlist.has_key(filename):
            outh.write(instructlist[filename])
        else:
            # outh.write("TnFOXPolicies.apply"+base+"(cclass."+base+")\n")
            outh.write("baseclass="+base+".base"+base+"()\n")
            outh.write("if baseclass: Import(baseclass+'.pyste')\n")
            outh.write("cclass=Class('FX::"+base+"', '"+filename+"')\n")
            outh.write(base+".apply"+base+"(globals(), cclass)\n")
        outh.write("try: "+base+".customise(globals())\nexcept: pass\n")
        if appendlist.has_key(filename):
            outh.write(appendlist[filename])
        outh.close()
        modules+=[base]
finally:
    inith=file("Policies/__init__.py", "wt")
    inith.write("__all__="+repr(modules)+"\n");
    inith.close()
print "Generated pyste files"

herepath=os.getcwd()
boostpath=os.path.abspath(boostpath)
pystepath=os.path.abspath(boostpath+"/libs/python/pyste/src/Pyste/pyste.py")+" "
args="--module="+module+" --multiple --out "+herepath+" --pyste-ns "+module
args+=" -I "+herepath+" -I "+boostpath+" -I "+headerspath
if sys.platform=="win32": args+=" -DWIN32 "
else: pystepath="python "+pystepath
def callpyste(args, files):
    if len(files)>1024:
        while 1:
            temploc=tempfile.mktemp()
            fh=open(temploc, "wt")
            if fh: break
        try:
            fh.write(files+"\n")
            fh.close()
            mysystem(pystepath+args+" --file-list="+temploc)
        finally:
            os.remove(temploc)
    else:
        mysystem(pystepath+args+" "+string.replace(files, "\n", " "))

updatelist=["_regconvs.pyste"]
pystelist=""
for filename in filelist:
    filepath=os.path.join(headerspath, filename)
    base,ext=os.path.splitext(filename)
    pystelist+=base+".pyste\n"
    try:
        lastupdate=os.path.getmtime("_"+base+".cpp")
    except os.error:
        lastupdate=0
    if os.path.getmtime(filepath)>lastupdate:
        updatelist+=[base+".pyste"]
pystelist="_regconvs.pyste\n"+pystelist
print "Generating main ..."
callpyste("--generate-main "+args, pystelist)
print "\nHeader files requiring update:"
print updatelist

def processList(updatelist):
    for filepath in updatelist:
        callpyste(args, filepath)
if len(updatelist):
    threadh=[]
    portion=int((0.5+len(updatelist))/processors)
    if portion==0: portion=1
    n=0
    for t in range(0, processors):
        if n>len(updatelist): break
        threadh.append(threading.Thread(target=processList(updatelist[n:n+portion])))
        n+=portion
        threadh[-1].start()
    for t in range(0, processors):
        threadh[t].join()
print "Done!"
