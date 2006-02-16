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
from shlex import shlex

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
class TextTranslations(dict):
    # Of tuple (string in english, source file name (can be None), class name (can be None), hint (can be None), hash of pars settings)
    # mapping to list [[list of pars settings], dict of translated strings]
    def has(self, ltext, lsrcfile=None, lclass=None, lhint=None, tlang=None):
        key=(ltext, lsrcfile, lclass, lhint, 0)
        if not self.has_key(key): return False
        return self[key][1].has_key(tlang)
    def fetch(self, ltext, lsrcfile=None, lclass=None, lhint=None, tlang=None):
        key=(ltext, lsrcfile, lclass, lhint, 0)
        if not self.has_key(key): return None
        return self[key][1][tlang]
    def add(self, ltext, lsrcfile=None, lclass=None, lhint=None, tlang=None, ttext=None, tpars=[]):
        hashval=0
        for parno, pardata in tpars:
            hashval+=(int(parno[1:])<<16)
            hashval+=int(pardata)
        key=(ltext, lsrcfile, lclass, lhint, hashval)
        if not self.has_key(key):
            self[key]=[[],{}]
        self[key][0]=tpars
        masterkey=(ltext, None, None, None, 0)
        if self.has_key(masterkey) and self[masterkey][1].has_key(tlang):
            if self[masterkey][1][tlang]==ttext:
                self[key][1][tlang]="up"
                #print "Added up translation of",key,"to",self[key]
                return
        self[key][1][tlang]=ttext
        #print "Added translation of",key,"to",self[key]
    
class TextLiterals:
    def __init__(self, filename):
        maxinsertidx=9
        self.__myversion=1;
        useversion=self.__myversion
        self.__translations=TextTranslations()
        self.__needsupdate=False
        self.__altered=False
        self.__langids=[]
        self.__amdisabled=not filename
        self.__filename=filename
        if not filename: return
        try:
            inh=file(filename, "rU")
        except IOError, (errno, strerror):
            sys.stderr.write("WARNING: Failed to open language file '"+filename+" for reading\n")
            self.__altered=True
            return
        try:
            lexer=shlex(inh, filename)
            lexer.wordchars+="%"
            lexer.whitespace=" \r\n"
            indent=0
            state={}  # dict of parameter name mapping to list[indent] of parameter value
            def setState(parameter, value, i=indent):
                if not state.has_key(parameter): state[parameter]=[]
                list=state[parameter]
                while i>=len(list): list.append(None)
                #print "Setting state",parameter,"indent",i,"to",value
                list[i]=value
            def getState(parameter):
                return state[parameter][-1]
            setState("origtxt", None)
            setState("srcfile", None)
            setState("class", None)
            setState("hint", None)
            for n in range(0,maxinsertidx): setState("%"+str(n), None)
            def filterState(indent):
                for key,list in state.items():
                    while len(list)-1>indent:
                        list.pop()
            skipindent=False
            while 1:
                oldindent=indent
                linenoChanged=False
                while 1:
                    oldlineno=lexer.lineno
                    token=lexer.get_token()
                    lineno=lexer.lineno
                    if lineno-oldlineno>0:
                        indent=0
                        linenoChanged=True
                    if token=="\t":
                        indent+=1
                    else: break
                #print "Line no",lineno,"indent",indent
                command=lexer.get_token()
                if not token or not command: break
                if skipindent:
                    if indent==0: skipindent=False
                    continue
                if command not in ":=":
                    self.__amdisabled=True
                    printError(self.__filename, lexer.lineno, "Expected colon or equals")
                    continue
                #print indent,token,command
                if linenoChanged: filterState(indent)
                if command=="=":
                    modval=lexer.get_token()
                    command=lexer.get_token()
                    if not modval:
                        self.__amdisabled=True
                        printError(self.__filename, lexer.lineno, "Expected modifier value")
                        continue
                    if command not in ":=":
                        self.__amdisabled=True
                        printError(self.__filename, lexer.lineno, "Expected colon or equals")
                        continue
                    if token=="version":
                        if int(modval)>myversion:
                            self.__amdisabled=True
                            printError(self.__filename, lexer.lineno, "Translation file format is too new for me")
                            sys.exit(1)
                        useversion=int(modval)
                    elif token=="needsupdating":
                        if(modval=="No"):
                            self.__needsupdate=False
                        else: self.__needsupdate=True
                    elif token=="langids":
                        modval=string.strip(modval, '"')
                        delimit=-1
                        while delimit<len(modval):
                            odelimit=delimit+1
                            delimit=string.find(modval, ",", odelimit)
                            if delimit==-1: delimit=len(modval)
                            self.__langids.append(modval[odelimit:delimit])
                    else:
                        if not state.has_key(token):
                            self.__amdisabled=True
                            printError(self.__filename, lexer.lineno, "Unknown modifier '"+token+"'")
                            continue
                        setState(token, modval, indent+1)                    
                    continue
                if indent==0:
                    if token[0]!='"' or token[-1]!='"':
                        self.__amdisabled=True
                        printError(self.__filename, lexer.lineno, "Expected original string")
                        skipindent=True
                        continue
                    setState("origtxt", token, 1)
                    continue
                # Must be a translation then
                trans=lexer.get_token()
                if not trans:
                    self.__amdisabled=True
                    printError(self.__filename, lexer.lineno, "Expected translation string")
                    continue
                if token not in self.__langids:
                    self.__amdisabled=True
                    printError(self.__filename, lexer.lineno, "WARNING: Unknown language id '"+token+"' - discarding")
                    continue
                transpars=[]
                indent=0
                for n in range(0,maxinsertidx):
                    parname="%"+str(n)
                    par=getState(parname)
                    if par!=None:
                        transpars.append((parname,par))
                if trans=="up":
                    atrans=self.__translations.fetch(getState("origtxt"), tlang=token)
                    if not atrans:
                        self.__amdisabled=True
                        printError(self.__filename, lexer.lineno, "WARNING: No master translation available for this language")
                        sys.exit(1)
                    trans=atrans
                self.__translations.add(getState("origtxt"), getState("srcfile"),
                                        getState("class"), getState("hint"), token, trans, transpars)
        finally:
            inh.close()
    def add(self, ltext, lclass=None, lhint=None):
        global inputname
        if not len(self.__langids):
            self.__langids.append("LANG")
            self.__translations.add(ltext, '"'+inputname+'"', lclass, lhint, "LANG", "\"!TODO! enter translation here\"")
            self.__altered=True
            self.__needsupdate=True
        else:
            for lang in self.__langids:
                if not self.__translations.has(ltext, '"'+inputname+'"', lclass, lhint, lang):
                    self.__translations.add(ltext, '"'+inputname+'"', lclass, lhint, lang, "\"!TODO! enter translation here\"")
                    self.__altered=True
                    self.__needsupdate=True
    def write(self, filename):
        if not filename or not self.__altered or self.__amdisabled: return
        try:
            outh=file(filename, "wt")
            #outh=sys.stdout
        except IOError, (errno, strerror):
            sys.stderr.write("Failed to open language file '"+filename+"' for writing\n")
        try:
            outh.write("# TnFOX human language string literal translation file\n")
            outh.write("# Last updated: "+time.strftime("%a, %d %b %Y %H:%M:%S +0000")+"\n\n")
            outh.write("version="+str(self.__myversion)+":\n\n")
            outh.write("# If the line below says =Yes, then there are still text literals in here\n"
                       "# which have not been translated yet. Search for \"!TODO!\" to find them\n\n")
            outh.write("needsupdating=")
            if self.__needsupdate:
                outh.write("Yes:\n")
            else: outh.write("No:\n")
            outh.write("\n# Enter the list of all language ids used in the file\n\nlangids=\"")
            langids=""
            for lang in self.__langids: langids+=lang+","
            outh.write(langids[:-1]+"\":\n\n")
            alltrans=self.__translations.items()
            alltrans.sort()
            sidx=0
            while sidx<len(alltrans):
                src, data=alltrans[sidx]
                origtxt=list(src)[0]
                eidx=sidx+1;
                while eidx<len(alltrans):
                    tempsrc,tempdata=alltrans[eidx]
                    if origtxt!=list(tempsrc)[0]: break
                    eidx+=1
                # sidx points at start, eidx points to the end+1
                indent=0
                def wr(str):
                    for i in range(0, indent): outh.write("\t")
                    outh.write(str)
                wr(origtxt+":\n"); indent=1
                for item in alltrans[sidx:eidx]:
                    src, data=item
                    origtxt, srcfile, classname, hint, parshash=src
                    pars=data[0]; transes=data[1]
                    modifier=""
                    if srcfile: modifier+="srcfile="+srcfile+":"
                    if classname: modifier+="class="+classname+":"
                    if hint: modifier+="hint="+hint+":"
                    for parno,pardata in pars:
                        modifier+=parno+"="+pardata+":"
                    if len(modifier):
                        wr(modifier+"\n")
                        indent+=1
                    for lang, trans in transes.items():
                        wr(lang+": "+trans+"\n")
                    if len(modifier): indent-=1
                sidx=eidx
                outh.write("\n")
        finally:
            outh.close()
    def process(self, line):
        if self.__amdisabled: return
	sline=prepline(line)
        sidx=string.find(sline, "tr(")
        if sidx==-1: return
        if sline[sidx-1] not in " :,(=": return
        withclass=sline[sidx-8:sidx]=="QTrans::"
        # Except don't process the QTrans::tr() code in QTrans.cpp!
        if sline[:23]=="QTransString QTrans::tr": return
        parts=[]
        s=myfind(line, ["tr("], 0, True)+3; e=myfind(line, [",", ")"], s)
        if e==-1: raise AssertionError, "tr() currently must be on one line"
        while e>=0:
            parts.append(string.strip(line[s:e]))
            if line[e]==')': break
            s=e+1; e=myfind(line, [",", ")"], s)
        def validate(item):
            instring=False
            nondelimitcnt=0
            for idx in range(0, len(item)):
                if item[idx]=='"' and item[idx-1]!='\\':
                    instring=not instring
                else:
                    if not instring: nondelimitcnt+=1
            return nondelimitcnt==0
        goodparts=filter(validate, parts)
        if goodparts!=parts:
            global options
            global linecnt
            printError(options.source, linecnt, "WARNING: Translatable strings do not appear to be literal")
        if withclass:
            parts[1], parts[0]=parts[0], parts[1]
        else:
            global currentclass
            if currentclass: parts.insert(1, '"'+currentclass+'"')
        self.add(*parts)

textliterals=TextLiterals(options.text)






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
    textliterals.process(line)
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
