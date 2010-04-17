#! /usr/bin/env python
#********************************************************************************
#                                                                               *
#                           TnFOX munged files updater                          *
#                                                                               *
#********************************************************************************
#        Copyright (C) 2002-2010 by Niall Douglas.   All Rights Reserved.       *
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
# $Id:                                                                          *
#*******************************************************************************/

myversion="0.22"
import sys
import os

if sys.version_info<(2,3,0,'alpha',0):
    sys.exit("UpdateMunged.py needs at least v2.3 of Python")
from optparse import OptionParser

parser=OptionParser(usage="""%prog -d <directory> -t <timestampfile.h> -c "<command line arguments>"
      Copyright (C) 2003-2010 by Niall Douglas.   All Rights Reserved.
  NOTE THAT I NIALL DOUGLAS DO NOT PERMIT ANY OF MY CODE USED UNDER THE GPL
*********************************************************************************
  This code is free software; you can redistribute it and/or modify it under
  the terms of the GNU Library General Public License v2.1 as published by the
  Free Software Foundation EXCEPT that clause 3 does not apply ie; you may not
  "upgrade" this code to the GPL without my prior written permission.
  Please consult the file "License_Addendum2.txt" accompanying this file.

""", version="%prog v"+myversion)
parser.add_option("-d", "--directory", default=".",
                  help="Path to the directory of sources")
parser.add_option("-t", "--timestampfile", default=None,
                  help="Path to the timestamp file")
parser.add_option("-s", "--gitrevisionheader", default=None,
                  help="Path to a header file for writing the current GIT revision into")
parser.add_option("-c", "--commandline", default="",
                  help="The command line arguments to pass to CppMunge.py")
parser.add_option("-v", "--verbose", action="store_true", default=False,
                  help="Enable verbose mode")
args=sys.argv;
#args=['D:\\Tornado\\TClient\\TnFOX\\UpdatedMunged.py', '-d', 'src', '-c', '"-m -f 4 -c include\FXErrCodes.h -t TnFOXTrans.txt"']
#print args
(options, args)=parser.parse_args(args)


verbose=options.verbose
print "TnFOX Munge file updater v"+myversion
myloc=os.path.dirname(sys.argv[0])
if options.timestampfile:
    timestampfile=options.timestampfile
else:
    timestampfile=os.path.join(myloc, "UpdateMunged.timestamp")
try:
    lastmungedmtime=os.path.getmtime(timestampfile)
except os.error:
    lastmungedmtime=0
if options.gitrevisionheader:
    if "/home/ned" in os.path.abspath(myloc):
        print "Skipping as on Niall's machine"
    else:
        fh=file(options.gitrevisionheader, "wt")
        try:
            fh.write('#define TNFOX_GIT_DESCRIPTION "')
            try:
                (childinh, childh)=os.popen4("git describe")
                line=childh.readline()
                if line:
                    fh.write(line[:-1]+'"\n#define TNFOX_GIT_REVISION 0x')
                    # We want the GIT SHA
                    idx=line.find('-g')
                    assert idx>=0;
                    line=line[idx+2:]
                    fh.write(line)
                    print "\nThis is built from GIT revision",line
                childinh.close()
                childh.close()
            except:
                fh.write('<error>"\n#define TNFOX_GIT_REVISION -1\n')
                print "\nCalling git describe FAILED due to",sys.exc_info()
        finally:
            fh.close()

filelistsrc=os.listdir(options.directory)
filelist=[]
for srcfile in filelistsrc:
    name,ext=os.path.splitext(srcfile)
    if ext==".cxx":
        sourcepath=os.path.normpath(os.path.join(options.directory, srcfile))
        filelist.append((sourcepath, os.path.getmtime(sourcepath)))

def isNewer(item):
    global lastmungedmtime
    sourcepath, sourcemtime=item
    #print lastmungedmtime, sourcemtime, sourcepath
    return sourcemtime>lastmungedmtime

updatelist=filter(isNewer, filelist)
if 0==len(updatelist):
    print "\nNo files need updating"
else:
    if sys.platform=="win32": cppmungeexec="CppMunge.py"
    else: cppmungeexec="python CppMunge.py"
    cppmungeloc=os.path.normpath(os.path.join(myloc, cppmungeexec))
    if ' ' in cppmungeloc: cppmungeloc='"'+cppmungeloc+'"';
    commandoptions=options.commandline
    for item in updatelist:
        sourcepath, sourcemtime=item
        command=cppmungeloc+" "+commandoptions+" -s "+sourcepath
        print "\n"+command
        if "/home/ned" in os.path.abspath(cppmungeloc):
            print "Skipping as on Niall's machine"
        else:
            (childinh, childh)=os.popen4(command)
            while 1:
                line=childh.readline()
                if not line: break
                print "  "+line,
            childinh.close()
            childh.close()
    fh=file(timestampfile, "wt")
    fh.write("//foo")
    fh.close()

