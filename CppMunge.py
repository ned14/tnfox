#! /usr/bin/env python
#********************************************************************************
#                                                                               *
#                         TnFOX C++ source file munger                          *
#                                                                               *
#********************************************************************************
#        Copyright (C) 2002-2005 by Niall Douglas.   All Rights Reserved.       *
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

myversion="0.93"
import sys
import string
import os
import time
import atexit

if sys.version_info<(2,3,0,'alpha',0):
    sys.exit("CppMunge.py needs at least v2.3 of Python")
from optparse import OptionParser

parser=OptionParser(usage="""%prog [-f <flags>] [-m] [-t <file.txt>] [-c <header.h>] [-n <source.cpp>] [-s <source.cxx>
      Copyright (C) 2002-2005 by Niall Douglas.   All Rights Reserved.
       NOTE THAT I DO NOT PERMIT ANY OF MY CODE TO BE PROMOTED TO THE GPL
********************************************************************************
 This code is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version
 EXCEPT that clause 3 of the LGPL does not apply ie; you may not
 "upgrade" this code to the GPL without my prior written permission.
 Please consult the file "License_Addendum2.txt" accompanying this file.
""", version="%prog v"+myversion)
parser.add_option("-m", "--msvc", action="store_true", default=False,
                  help="Causes warnings to be generated in MSVC rather than GNU format")
parser.add_option("-s", "--source", default=None,
                  help="Specify source file")
parser.add_option("-c", "--codes", default="ErrCodes.h",
                  help="Specify file to receive error codes (default is 'ErrCodes.h'))")
parser.add_option("-t", "--text", default=None,
                  help="Specify file to receive user visible text literals")                  
parser.add_option("-f", "--flags", type="int", default=0,
                  help="Specify flags affecting processing which include:\n"
                       "bit 0: Disable destructor catches\n"
                       "bit 1: Disable exception code extraction\n"
                       "bit 2: Disable automatic exception code insertion")
parser.add_option("-v", "--verbose", action="store_true", default=False,
                  help="Enable verbose mode")
args=sys.argv;
#args=['D:\\Tornado\\TClient\\TnFOX\\CppMunge.py', '-v', '-m', '-f', '4', '-c', 'include\\FXErrCodes.h', '-t', 'TnFOXTrans.txt', '-s', 'src\\QBlkSocket.cxx']
#print args
(options, args)=parser.parse_args(args)


verbose=options.verbose
print "TnFOX C++ Munger v"+myversion+"\n"
try:
    inh=file(options.source, "rU")
except IOError, (errno, strerror):
    sys.exit("Input I/O error(%s): %s" % (errno, strerror))
if verbose: print "Source",options.source,"opened"
(inputpath,inputname)=os.path.split(options.source)
if not (options.flags & 2):
    while True:
        try:
            codesh=file(options.codes, "rU")
        except IOError, (errno, strerror):
            if 2==errno:    # missing
                try:
                    codesh=file(options.codes, "wt")
                except IOError, (errno, strerror):
                    sys.exit("Error codes header file I/O error(%s): %s" % (errno, strerror))
                codesh.close()
        else:
            break

def printError(filename, lineno, msg):
    if options.msvc:
        sys.stderr.write(filename+"("+str(lineno)+") : "+msg+"\n")
    else:
        sys.stderr.write("\""+filename+"\", line "+str(lineno)+": "+msg+"\n")
def myfind(source, substrs, startidx=0, nobrackets=False):
    instring=False; inbracketcnt=0; firsts=""
    for c in substrs:
        firsts+=c[0]
    for idx in range(startidx, len(source)):
        #c=source[idx]
        if source[idx]=='"' and source[idx-1]!='\\': instring=not instring
        if not instring:
            if source[idx]=='(': inbracketcnt+=1
            if source[idx]==')': inbracketcnt-=1
            if nobrackets: inbracketcnt=0
            if inbracketcnt<=0 and source[idx] in firsts:
                for substr in substrs:
                    if source[idx:idx+len(substr)]==substr: return idx
    return -1


# ************************* Error Codes stuff ***********************
def prepline(line):
     return string.strip(line)

class ErrCodes(dict):
    # Of macro mapping to int
    def __init__(self, inh):
        global inputname
        self.highest=0
        self.__filetext=""
        self.altered=False
        indata=False
        yes=False
        for oldline in inh:
            line=string.strip(oldline)
            if line=="// Codes for "+inputname: yes=True
            if line[:6]=="// END": indata=False
            if not yes and indata: self.__filetext+=oldline
            if line[:8]=="// BEGIN": indata=True
            if yes and line=="// End codes for "+inputname: yes=False
            if yes and line[0:7]=="#define":
                line=line[8:]
                i=string.index(line, " ")
                key=line[0:i]
                data=eval(line[i+1:])
                if data>self.highest: self.highest=data
                self[key]=data
        if self.highest==0:
            # Use a hash of the filename as base
            hashcode=0
            for c in inputname:
                hashcode=(2654435769^(hashcode+ord(c)))<<1
            hashcode=(hashcode & 0x00ffffff)<<8
            self.highest=int(hashcode)
        else:
            self.highest+=1
    def write(self, outh):
        global inputname, options
        (mypath, myname)=os.path.split(options.codes)
        myheadername=string.upper(string.replace(myname, ".", "_"))
        outh.write("/* "+myname+"\n"
                   "AUTOMATICALLY GENERATED BY CPPMUNGE - CHANGES WILL BE LOST!\n"
                   "*/\n\n"
                   "#ifndef "+myheadername+"\n"
                   "#define "+myheadername+"\n\n"
                   "// BEGIN\n")
        outh.write(self.__filetext);
        outh.write("// Codes for "+inputname+"\n")
        keydata=self.items()
        def sortfunct(a, b):
            ka,da=a; kb,db=b; return cmp(da,db)
        keydata.sort(sortfunct)
        for key,data in keydata:
            outh.write("#define "+key+" "+hex(data)+"\n")
        outh.write("// End codes for "+inputname+"\n")
        outh.write("// END\n\n#endif\n")
    def process(self, baseline):
	line=prepline(baseline)
        sidx=string.find(line, "FXERRG(")
        if(sidx==-1):
            sidx=string.find(line, "FXERRH(")
            if sidx>=0:
                c=myfind(line, [","], sidx+7)
                line=line[c+1:]
        else:
            line=line[sidx+7:]
        if sidx>=0:
            c=myfind(line, [","])
            line=line[c+1:]
            c=myfind(line, [","])
            line=string.strip(line[:c])
            if verbose: print "Found use of error macro",line,"at line",line
            if string.upper(line)==line and not self.has_key(line):
                if line=="0":
                    global options
                    global linecnt
                    printError(options.source, linecnt, "WARNING: Exception code not specified")
                elif line[:12]=="FXEXCEPTION_":
                    pass
                else:
                    self[line]=self.highest
                    if verbose: print "Added new error code",line," as code",hex(self.highest)
                    self.highest+=1
                    self.altered=True

# Need first scan of codes to determine highest code number
if not (options.flags & 2):
    errcodes=ErrCodes(codesh)
    codesh.close()
if not (options.flags & 4):
    sys.stderr.write("WARNING: Automatic exception code insertion not implemented yet\n")




# ************************* Text translation stuff ***********************    
from TextLiterals import TextLiterals
textliterals=TextLiterals(options.text, inputname)






infile=[]           # A list of lines
infilechanged=False # Becomes true if source file is changed
bracketcnt=0        # The count of {}'s
nmspbracketcnt=0    # Number of namespace nestings
linecnt=0           # The count of lines
classpending=False  # Class beginning expected
currentclass=None   # The current class
classbracketidx=0   # bracketcnt when class began
currentmethod=None  # The current method
moddestructdisabled=False # For TERRH_NOTHROW
extracterrorcodesdisabled=False
def deathDump():
    if linecnt>0:
        printError(options.source, linecnt, "PROGRAM FAILED!!!")
atexit.register(deathDump)
# Process lines till eof
for line in inh:
    linecnt+=1
    sline=prepline(line)
    bracketdiff=string.count(sline, "{")-string.count(sline,"}")
    if bracketdiff==1 and "namespace" in sline:
        nmspbracketcnt+=1
        bracketdiff=0
        if verbose: print "Namespace encountered at line",linecnt
        #line=line[:-1]+" // Namespace begins\n"
    bracketcnt+=bracketdiff
    if bracketcnt<0 and nmspbracketcnt>0:
        nmspbracketcnt+=bracketcnt
        bracketcnt=0
        if verbose: print "Namespace ends at line",linecnt
        #line=line[:-1]+" // Namespace ends\n"
    if bracketcnt<classbracketidx:
        # Modify the end of the destructor
        if currentclass and not moddestructdisabled and not (options.flags & 1):
            if "~"+currentclass==currentmethod and -1==string.find(line, "} FXEXCEPTIONDESTRUCT2; "):
                line="} FXEXCEPTIONDESTRUCT2; "+line
                infilechanged=True
        #line=line[:-1]+" // Method "+currentclass+"::"+currentmethod+" ends\n"
        currentclass=None
        currentmethod=None
        classbracketidx=0
        moddestructdisabled=False
        if verbose: print "Method ended at line",linecnt
    brackpos=string.find(sline, "(")
    if bracketcnt==0 and brackpos>=0:
        sline=string.strip(sline[0:brackpos])
        # Test for non-definition implementation
        brackpos=string.rfind(sline, "::")
        if brackpos>=0:
            spacepos=string.rfind(sline, " ", 0, brackpos)
            sline=sline[spacepos+1:]
            brackpos=string.find(sline, "::")
            currentclass=sline[0:brackpos]
            currentmethod=sline[brackpos+2:]
            if verbose: print "Line",linecnt,": class",currentclass,"method",currentmethod
            classpending=True
    if classpending:
        if string.find(line, "throw()")!=-1 or string.find(line, "FXERRH_NODESTRUCTORMOD")!=-1:
            moddestructdisabled=True
        if string.count(sline, "{")>0:
            classbracketidx=bracketcnt
            classpending=False
        # Modify the start of the destructor
        if not moddestructdisabled and not (options.flags & 1):
            if "~"+currentclass==currentmethod:
                if bracketcnt==1 and string.find(line, "{")>=0:
                    if -1==string.find(line, "{ FXEXCEPTIONDESTRUCT1 {"):
                        line=line[:-1]+" FXEXCEPTIONDESTRUCT1 {\n"
                        infilechanged=True
                        print "Modified destructor for class",currentclass,"at line",linecnt
    if string.find(line, "CPPMUNGE_NOEXTRACTERRORCODES")!=-1: extracterrorcodesdisabled=True
    if string.find(line, "CPPMUNGE_EXTRACTERRORCODES")!=-1: extracterrorcodesdisabled=False
    #if not (options.flags & 4):
    #    mod_throws(line)
    if not (options.flags & 2) and not extracterrorcodesdisabled:
        errcodes.process(line)
    textliterals.process(line, currentclass)
    infile+=[line]

inh.close()
if not (options.flags & 2) and errcodes.altered:
    codesh=file(options.codes, "wt")
    errcodes.write(codesh)
    codesh.close()
# Write out munged cpp file
if infilechanged:
    outh=file(options.source, "wt")
    #outh=sys.stdout
    if verbose: print "Output",options.source,"opened"
    for line in infile:
        outh.write(line)
    outh.close()
textliterals.write(options.text)
if verbose: print "Processing complete"
linecnt=0
