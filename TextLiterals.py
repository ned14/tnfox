#! /usr/bin/env python
#********************************************************************************
#                                                                               *
#                           TnFOX Translations via Google                       *
#                                                                               *
#********************************************************************************
#        Copyright (C) 2008 by Niall Douglas.   All Rights Reserved.            *
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

import string, codecs, sys, time
from shlex import shlex

    
def prepline(line):
     return string.strip(line)

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

class TextTranslations(dict):
    # Of tuple (string in english, source file name (can be None), class name (can be None), hint (can be None), hash of pars settings)
    # mapping to list [[list of pars settings], dict of translated strings]
    def has(self, ltext, lsrcfile=None, lclass=None, lhint=None, tlang=None):
        if type(ltext)!=type(u''):
            ltext=unicode(ltext, "utf-8")
        key=(ltext, lsrcfile, lclass, lhint, 0)
        if not self.has_key(key): return False
        return self[key][1].has_key(tlang)
    def fetch(self, ltext, lsrcfile=None, lclass=None, lhint=None, tlang=None):
        if type(ltext)!=type(u''):
            ltext=unicode(ltext, "utf-8")
        key=(ltext, lsrcfile, lclass, lhint, 0)
        if not self.has_key(key): return None
        return self[key][1][tlang]
    def add(self, ltext, lsrcfile=None, lclass=None, lhint=None, tlang=None, ttext=None, tpars=[]):
        if type(ltext)!=type(u''):
            ltext=unicode(ltext, "utf-8")
        if ttext and type(ttext)!=type(u''):
            ttext=unicode(ttext, "utf-8")
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
    def printError(self, filename, lineno, msg):
        if self.__msvcerrors:
            sys.stderr.write(filename+"("+str(lineno)+") : "+msg+"\n")
        else:
            sys.stderr.write("\""+filename+"\", line "+str(lineno)+": "+msg+"\n")
    def __init__(self, filename, inputname=None, msvcerrors=None):
        maxinsertidx=9
        self.__myversion=1;
        useversion=self.__myversion
        self.__translations=TextTranslations()
        self.__needsupdate=False
        self.__altered=False
        self.__langids=[]
        self.__amdisabled=not filename
        self.__filename=filename
        self.__inputname=inputname
        self.__msvcerrors=msvcerrors
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
                command=lexer.get_token()
                #print "Line no",lineno,"indent",indent,"command",command
                if not token or not command: break
                if skipindent:
                    if indent==0: skipindent=False
                    continue
                if command not in ":=":
                    self.__amdisabled=True
                    self.printError(self.__filename, lexer.lineno, "Expected colon or equals")
                    continue
                #print indent,token,command
                if linenoChanged: filterState(indent)
                if command=="=":
                    modval=lexer.get_token()
                    command=lexer.get_token()
                    if not modval:
                        self.__amdisabled=True
                        self.printError(self.__filename, lexer.lineno, "Expected modifier value")
                        continue
                    if command not in ":=":
                        self.__amdisabled=True
                        self.printError(self.__filename, lexer.lineno, "Expected colon or equals")
                        continue
                    if token=="version":
                        if int(modval)>myversion:
                            self.__amdisabled=True
                            self.printError(self.__filename, lexer.lineno, "Translation file format is too new for me")
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
                            self.printError(self.__filename, lexer.lineno, "Unknown modifier '"+token+"'")
                            continue
                        setState(token, modval, indent+1)                    
                    continue
                if indent==0:
                    if token[0]!='"' or token[-1]!='"':
                        self.__amdisabled=True
                        self.printError(self.__filename, lexer.lineno, "Expected original string")
                        skipindent=True
                        continue
                    setState("origtxt", token, 1)
                    continue
                # Must be a translation then
                trans=lexer.get_token()
                if not trans:
                    self.__amdisabled=True
                    self.printError(self.__filename, lexer.lineno, "Expected translation string")
                    continue
                if token not in self.__langids:
                    self.__amdisabled=True
                    self.printError(self.__filename, lexer.lineno, "WARNING: Unknown language id '"+token+"' - discarding")
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
                        self.printError(self.__filename, lexer.lineno, "WARNING: No master translation available for this language")
                        sys.exit(1)
                    trans=atrans
                self.__translations.add(getState("origtxt"), getState("srcfile"),
                                        getState("class"), getState("hint"), token, trans, transpars)
        finally:
            inh.close()
    def translations(self):
        return self.__translations
        
    def add(self, ltext, lclass=None, lhint=None):
        if not len(self.__langids):
            self.__langids.append("LANG")
            self.__translations.add(ltext, '"'+self.__inputname+'"', lclass, lhint, "LANG", "\"!TODO! enter translation here\"")
            self.__altered=True
            self.__needsupdate=True
        else:
            for lang in self.__langids:
                if not self.__translations.has(ltext, '"'+self.__inputname+'"', lclass, lhint, lang):
                    self.__translations.add(ltext, '"'+self.__inputname+'"', lclass, lhint, lang, "\"!TODO! enter translation here\"")
                    self.__altered=True
                    self.__needsupdate=True
    def addLang(self, lang):
        if not lang in self.__langids: self.__langids.append(lang)
        self.__altered=True
    def write(self, filename):
        if not filename or not self.__altered or self.__amdisabled: return
        try:
            outh=codecs.open(filename, "wt", "utf-8")
            #outh=sys.stdout
        except IOError, (errno, strerror):
            sys.stderr.write("Failed to open language file '"+filename+"' for writing\n")
        try:
            outh.write(u"# TnFOX human language string literal translation file\n")
            outh.write(u"# Last updated: "+time.strftime("%a, %d %b %Y %H:%M:%S +0000")+u"\n\n")
            outh.write(u"version="+str(self.__myversion)+u":\n\n")
            outh.write(u"# If the line below says =Yes, then there are still text literals in here\n"
                       u"# which have not been translated yet. Search for \"!TODO!\" to find them\n\n")
            outh.write(u"needsupdating=")
            if self.__needsupdate:
                outh.write(u"Yes:\n")
            else: outh.write(u"No:\n")
            outh.write(u"\n# Enter the list of all language ids used in the file\n\nlangids=\"")
            langids=""
            for lang in self.__langids: langids+=lang+u","
            outh.write(langids[:-1]+u"\":\n\n")
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
                    assert type(str) == type(u'')
                    for i in range(0, indent): outh.write(u"\t")
                    outh.write(str)
                wr(origtxt+u":\n"); indent=1
                for item in alltrans[sidx:eidx]:
                    src, data=item
                    origtxt, srcfile, classname, hint, parshash=src
                    pars=data[0]; transes=data[1]
                    modifier=u""
                    if srcfile: modifier+=u"srcfile="+srcfile+u":"
                    if classname: modifier+=u"class="+classname+u":"
                    if hint: modifier+=u"hint="+hint+u":"
                    for parno,pardata in pars:
                        modifier+=parno+u"="+pardata+u":"
                    if len(modifier):
                        wr(modifier+u"\n")
                        indent+=1
                    for lang, trans in transes.items():
                        #print repr(lang), repr(trans)
                        wr(u""+lang+u": "+trans+u"\n")
                    if len(modifier): indent-=1
                sidx=eidx
                outh.write(u"\n")
        finally:
            outh.close()
    def process(self, line, currentclass):
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
            self.printError(options.source, linecnt, "WARNING: Translatable strings do not appear to be literal")
        if withclass:
            parts[1], parts[0]=parts[0], parts[1]
        else:
            if currentclass: parts.insert(1, '"'+currentclass+'"')
        self.add(*parts)
