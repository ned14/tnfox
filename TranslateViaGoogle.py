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

myversion="0.10"

# Some of this code culled from http://www.technobabble.dk/2008/may/25/translate-strings-using-google-translate/

import sys, codecs, os
import urllib2
import urllib

from BeautifulSoup import BeautifulSoup
from BeautifulSoup import BeautifulStoneSoup

opener = urllib2.build_opener()
opener.addheaders = [('User-agent', 'SomethingElse')]

# lookup supported languages

translate_page = opener.open("http://translate.google.com/translate_t")
translate_soup = BeautifulSoup(translate_page)

source_languages = {}
target_languages = {}

for language in translate_soup("select", id="old_sl")[0].childGenerator():
    if language['value'] != 'auto':
        source_languages[language['value']] = language.string

for language in translate_soup("select", id="old_tl")[0].childGenerator():
    if language['value'] != 'auto':
        target_languages[language['value']] = language.string

def translate(sl, tl, text):

    """ Translates a given text from source language (sl) to
        target language (tl) """

    assert sl in source_languages, "Unknown source language."
    assert tl in target_languages, "Unknown taret language."

    assert type(text) == type(u''), "Expects input to be unicode."

    translated_page = opener.open(
        "http://translate.google.com/translate_t?" + 
        urllib.urlencode({'sl': sl, 'tl': tl}),
        data=urllib.urlencode({'hl': 'en',
                               'ie': 'UTF-8',
                               'text': text.encode('utf-8'),
                               'sl': sl, 'tl': tl})
    )
    
    # Unfortunately Google returns all sorts of page encodings
    # but missets the meta page header, so we must extract the
    # true page encoding from the HTTP protocol
    contenttype=translated_page.info()['Content-Type'][19:]
    translated_pagesrc = translated_page.read()
    sidx=translated_pagesrc.find('<meta content="text/html; charset=')
    eidx=translated_pagesrc.find('" http-equiv="content-type"/>', sidx)
    assert sidx>=0 and eidx>=0
    if sidx>=0 and eidx>=0:
        #print contenttype, translated_pagesrc[sidx:eidx]
        translated_pagesrc=translated_pagesrc[:sidx+34]+contenttype+translated_pagesrc[eidx:]
    translated_soup = BeautifulSoup(translated_pagesrc, fromEncoding=contenttype)

    ret = translated_soup('div', id='result_box')[0].string
    ret = BeautifulStoneSoup(ret, convertEntities=BeautifulStoneSoup.ALL_ENTITIES).contents[0]
    #print translated_soup.originalEncoding
    #if translated_soup.originalEncoding!="utf-8":
    #    ret=ret.encode('utf-8')
    return ret


from optparse import OptionParser

parser=OptionParser(usage="""%prog -o <output.txt> -i <input.txt>
      Copyright (C) 2008 by Niall Douglas.   All Rights Reserved.
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
parser.add_option("-o", "--output", default="TnFOXTrans_googled.txt",
                  help="Specify output file")
parser.add_option("-i", "--input", default=None,
                  help="Specify input file")
args=sys.argv;
(options, args)=parser.parse_args(args)


from TextLiterals import TextLiterals
if not options.input:
    if os.path.exists("TnFOXTrans_googled.txt"):
        options.input="TnFOXTrans_googled.txt"
    else:
        options.input="TnFOXTrans.txt"
textliterals=TextLiterals(options.input)
langtolang={ u"zh-CN" : u"cn" }
googlelangs=[]
for lang in target_languages:
    if not langtolang.has_key(lang) and (len(lang)>2 or lang=="en"): continue
    googlelangs.append(lang)
def ToTnFOXLang(lang):
    global langtolang
    if langtolang.has_key(lang): lang=langtolang[lang]
    return lang.upper()

print "TnFOX Translations via Google v"+myversion+"\n"
print
print "Google provides the following translations:"
for lang in googlelangs:
    print lang+",",
print "\n"
print "Translating strings ..."
translations=textliterals.translations()
#oh=codecs.open("dump.txt", "wt", "utf-8")
skip=0
for (ltexto, lsrcfile, lclass, lhint, hashval) in translations.keys():
    if skip>0:
        skip-=1
        continue
    ltext=ltexto
    #print ltext, lsrcfile, lclass, lhint
    if lsrcfile or lclass or lhint: continue
    print "\nTranslating master key ",ltext," ..."
    #oh.write(u'Translating '+ltext+u' ...\n')
    ltext=ltext[1:-1]
    # Swap all inserts
    n=0
    while n<len(ltext)-1:
        if ltext[n]=='%' and ltext[n+1].isdigit():
            ltext=ltext[:n]+'nialldouglas'+ltext[n+1]+ltext[n+2:]
        n+=1
    ltext=ltext.replace('\\n', "niall2douglas")
    #print ltext
    for lang in googlelangs:
        if translations[(ltexto, lsrcfile, lclass, lhint, hashval)][1].has_key(ToTnFOXLang(lang)): continue
        outtext=translate("en", lang, ltext)
        #print repr(outtext)
        # It needs massaging
        n=0
        while n<len(outtext)-13:
            if outtext[n:n+12]=='nialldouglas' and outtext[n+12].isdigit():
                outtext=outtext[:n]+'%'+outtext[n+12:]
            n+=1
        outtext=outtext.replace('"', "'")
        outtext='"'+outtext.replace('niall2douglas', '\\n')+'"'
        print ToTnFOXLang(lang)+u":",repr(outtext)
        #oh.write(ToTnFOXLang(lang)+u': '+outtext+u'\n')
        textliterals.addLang(ToTnFOXLang(lang))
        translations.add(ltexto, None, None, None, ToTnFOXLang(lang), outtext)
        # Add up's
        for (ctext, csrcfile, cclass, chint, hashval) in filter(lambda x: x[0]==ltexto and (x[1] or x[2]), translations.keys()):
            translations.add(ltexto, csrcfile, cclass, chint,  ToTnFOXLang(lang), outtext)
    textliterals.write(options.output)
    #break
#oh.close()
print "\nGoogle Translated results written out!"
