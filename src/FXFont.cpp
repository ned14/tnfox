/********************************************************************************
*                                                                               *
*                               F o n t   O b j e c t                           *
*                                                                               *
*********************************************************************************
* Copyright (C) 1997,2004 by Jeroen van der Zijp.   All Rights Reserved.        *
*********************************************************************************
* This library is free software; you can redistribute it and/or                 *
* modify it under the terms of the GNU Lesser General Public                    *
* License as published by the Free Software Foundation; either                  *
* version 2.1 of the License, or (at your option) any later version.            *
*                                                                               *
* This library is distributed in the hope that it will be useful,               *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU             *
* Lesser General Public License for more details.                               *
*                                                                               *
* You should have received a copy of the GNU Lesser General Public              *
* License along with this library; if not, write to the Free Software           *
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.    *
*********************************************************************************
* $Id: FXFont.cpp,v 1.94 2004/03/11 00:19:23 fox Exp $                          *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "fxpriv.h"
#include "FXStream.h"
#include "FXString.h"
#include "FXSize.h"
#include "FXPoint.h"
#include "FXRectangle.h"
#include "FXSettings.h"
#include "FXRegistry.h"
#include "FXHash.h"
#include "FXApp.h"
#include "FXId.h"
#include "FXFont.h"


/*
  Notes:

  - Interpretation of the hints:

      FONTPITCH_DEFAULT     No preference for pitch
      FONTPITCH_FIXED       If specified, match for fixed pitch fonts are strongly preferred
      FONTPITCH_VARIABLE    If specified, match for variable pitch font strongly preferred

      FONTHINT_DONTCARE     No hints given
      FONTHINT_DECORATIVE   Ye Olde Fonte
      FONTHINT_MODERN       Monospace fonts such as courier and so on
      FONTHINT_ROMAN        Serif font such as times
      FONTHINT_SCRIPT       Cursive font/script
      FONTHINT_SWISS        Sans serif font such as swiss, helvetica, arial
      FONTHINT_SYSTEM       Raster based fonts, typically monospaced

      FONTHINT_X11          Set internally to force X11-style font specification for font name

      FONTHINT_SCALABLE     Strong emphasis on scalable fonts; under Windows, this means
                            TrueType fonts are desired

      FONTHINT_POLYMORPHIC  Strong emphasis on polymorphic fonts; under Windows, this means
                            TrueType fonts desired also

  - For font size, the largest font not larger than the indicated size is taken;
    rationale is that larger fonts may make applications too big for the screen, but
    smaller ones are OK.

  - FONTENCODING_DEFAULT means we prefer the fonts for the current locale;
    currently, this is hardwired to iso8859-1 until we have some means of
    determining the preferred encoding from the locale.

  - FONTSLANT_ITALIC is a cursive typeface for some fonts; FONTSLANT_OBLIQUE is the same
    basic font written at an angle; for many fonts, FONTSLANT_ITALIC and FONTSLANT_OBLIQUE
    means pretty much the same thing.  FONTSLANT_DONTCARE indicates a preference for
    normal, non-cursive fonts.

  - FONTWEIGHT_DONTCARE indicates a preference for a normal font weight.

  - XFontStruct.ascent+XFontStruct.descent is the height of the font, as far as line
    spacing goes.  XFontStruct.max_bounds.ascent+XFontStruct.max_bounds.descent is
    larger, as some characters can apparently extend beyond ascent or descent!!

  - We should assume success first, i.e. only fall back on complicated matching if
    stuff fails; in many cases, fonts are picked from FXFontSelector and already known
    to exist...

  - Registry section FONTSUBSTITUTIONS can be used to map typeface names to platform
    specific typeface names:

        [FONTSUBSTITUTIONS]
        arial = helvetica
        swiss = helvetica

    This allows you to change fonts in programs with hard-wired fonts.

  - Text txfm matrix [a b c d] premultiplies.

  - Should we perhaps build our own tables of font metrics? This might make
    things simpler for the advanced stuff, and be conceivably a lot faster
    under MS-Windows [no need to SelectObject() all the time just to get some
    info; also, this could be useful in case the drawing surface is not a
    window].

  - FOR THE MOMENT we're creating a dummy DC to keep the font locked into the GDI
    for MUCH quicker access to text metrics.  Soon however we want to just build
    our own font metrics tables and determine the metrics entirely with client-side
    code.  This will [predictably] be the fastest possible method as it will not
    involve context switches...

  - Somehow, create() of fonts assumes its for the screen; how about other
    surfaces?

  - Need some API to make OpenGL fonts out of the 2D fonts...

  - Matching algorithm should favor bitmapped fonts over scalable ones [as the
    latter may not be optimized easthetically; also, the matching algorithm should
    not weight resolution as much.

  - UNICODE means registry and encoding are set to iso10646-1

  - More human-readable font strings (e.g. registry):

       family [foundry],size,weight,slant,setwidth,encoding,hints

    For example:

       times [urw],120,bold,i,normal,iso8859-1,0

    Note that the size is in decipoints!

  - Get encoding from locale (see X11).

  - Need to be able to specify size in pixels as well as points.

  - Fonts shown in XListFonts() may *still* not exist!  Need to
    change create() so it tries various things by XLoadQueryFont()
    instead of assuming font from XListFonts() used in XLoadQueryFont()
    will always succeed.
*/


// X11
#ifndef WIN32


// Hint mask
#define FONTHINT_MASK     (FONTHINT_DECORATIVE|FONTHINT_MODERN|FONTHINT_ROMAN|FONTHINT_SCRIPT|FONTHINT_SWISS|FONTHINT_SYSTEM)

// Convenience macros
#define DISPLAY(app)      ((Display*)((app)->display))



#ifdef HAVE_XFT_H


// A glyph exists for the given character
#define HASCHAR(font,ch)        TRUE    // FIXME
#define FIRSTCHAR(font)         0       // FIXME
#define LASTCHAR(font)          255     // FIXME


#else


// Maximum XLFD name length
#define MAX_XLFD          512

// XLFD Fields
#define XLFD_FOUNDRY      0
#define XLFD_FAMILY       1
#define XLFD_WEIGHT       2
#define XLFD_SLANT        3
#define XLFD_SETWIDTH     4
#define XLFD_ADDSTYLE     5
#define XLFD_PIXELSIZE    6
#define XLFD_POINTSIZE    7
#define XLFD_RESOLUTION_X 8
#define XLFD_RESOLUTION_Y 9
#define XLFD_SPACING      10
#define XLFD_AVERAGE      11
#define XLFD_REGISTRY     12
#define XLFD_ENCODING     13

// Match factors
#define ENCODING_FACTOR   1024
#define FAMILY_FACTOR     512
#define FOUNDRY_FACTOR    256
#define PITCH_FACTOR      128
#define RESOLUTION_FACTOR 64
#define SCALABLE_FACTOR   32
#define POLY_FACTOR       16
#define SIZE_FACTOR       8
#define WEIGHT_FACTOR     4
#define SLANT_FACTOR      2
#define SETWIDTH_FACTOR   1

#define EQUAL1(str,c)     (str[0]==c && str[1]=='\0')
#define EQUAL2(str,c1,c2) (str[0]==c1 && str[1]==c2 && str[2]=='\0')

// A glyph exists for the given character
#define HASCHAR(font,ch)  ((((XFontStruct*)font)->min_char_or_byte2 <= (FXuint)ch) && ((FXuint)ch <= ((XFontStruct*)font)->max_char_or_byte2))
#define FIRSTCHAR(font)   (((XFontStruct*)font)->min_char_or_byte2)
#define LASTCHAR(font)    (((XFontStruct*)font)->max_char_or_byte2)

#endif


// MS-Windows
#else

// A glyph exists for the given character
#define HASCHAR(font,ch)  ((((TEXTMETRIC*)font)->tmFirstChar <= (FXuint)ch) && ((FXuint)ch <= ((TEXTMETRIC*)font)->tmLastChar))
#define FIRSTCHAR(font)   (((TEXTMETRIC*)font)->tmFirstChar)
#define LASTCHAR(font)    (((TEXTMETRIC*)font)->tmLastChar)

#endif




namespace FX {

// For tables
struct ENTRY {
  const FXchar *name;
  FXuint        value;
  };

/*******************************************************************************/

// Helper functions X11

#ifndef WIN32


// Get family and foundry from name
static void familyandfoundryfromname(FXchar* family,FXchar* foundry,const FXchar* name){
  while(*name && isspace(*name)) name++;
  if(*name){
    while(*name && *name!='[') *family++=*name++;
    while(isspace(*(family-1))) family--;
    }
  *family='\0';
  if(*name=='['){
    name++;
    while(*name && isspace(*name)) name++;
    if(*name){
      while(*name && *name!=']') *foundry++=*name++;
      while(isspace(*(foundry-1))) foundry--;
      }
    }
  *foundry='\0';
  }


#ifdef HAVE_XFT_H                       // Using XFT

#ifndef FC_WEIGHT_THIN
#define FC_WEIGHT_THIN 1
#endif
#ifndef FC_WEIGHT_EXTRALIGHT
#define FC_WEIGHT_EXTRALIGHT 2
#endif
#ifndef FC_WEIGHT_NORMAL
#define FC_WEIGHT_NORMAL 99
#endif
#ifndef FC_WEIGHT_EXTRABOLD
#define FC_WEIGHT_EXTRABOLD 201
#endif
#ifndef FC_WEIGHT_HEAVY
#define FC_WEIGHT_HEAVY 211
#endif

// From FOX weight to fontconfig weight
static FXint weight2FcWeight(FXint weight){
  switch(weight){
    case FONTWEIGHT_THIN:      return FC_WEIGHT_THIN;
    case FONTWEIGHT_EXTRALIGHT:return FC_WEIGHT_EXTRALIGHT;
    case FONTWEIGHT_LIGHT:     return FC_WEIGHT_LIGHT;
    case FONTWEIGHT_NORMAL:    return FC_WEIGHT_NORMAL;
    case FONTWEIGHT_MEDIUM:    return FC_WEIGHT_MEDIUM;
    case FONTWEIGHT_DEMIBOLD:  return FC_WEIGHT_DEMIBOLD;
    case FONTWEIGHT_BOLD:      return FC_WEIGHT_BOLD;
    case FONTWEIGHT_EXTRABOLD: return FC_WEIGHT_EXTRABOLD;
    case FONTWEIGHT_HEAVY:     return FC_WEIGHT_HEAVY;
    }
  return FC_WEIGHT_NORMAL;
  }


// From fontconfig weight to FOX weight
static FXint fcWeight2Weight(FXint fcWeight){
  switch(fcWeight){
    case FC_WEIGHT_THIN:      return FONTWEIGHT_THIN;
    case FC_WEIGHT_EXTRALIGHT:return FONTWEIGHT_EXTRALIGHT;
    case FC_WEIGHT_LIGHT:     return FONTWEIGHT_LIGHT;
    case FC_WEIGHT_NORMAL:    return FONTWEIGHT_NORMAL;
    case FC_WEIGHT_MEDIUM:    return FONTWEIGHT_MEDIUM;
    case FC_WEIGHT_DEMIBOLD:  return FONTWEIGHT_DEMIBOLD;
    case FC_WEIGHT_BOLD:      return FONTWEIGHT_BOLD;
    case FC_WEIGHT_EXTRABOLD: return FONTWEIGHT_EXTRABOLD;
    case FC_WEIGHT_BLACK:     return FONTWEIGHT_BLACK;
    }
  return FONTWEIGHT_NORMAL;
  }


// From FOX slant to fontconfig slant
static FXint slant2FcSlant(FXint slant){
  switch(slant){
    case FONTSLANT_REGULAR:        return FC_SLANT_ROMAN;
    case FONTSLANT_ITALIC:         return FC_SLANT_ITALIC;
    case FONTSLANT_OBLIQUE:        return FC_SLANT_OBLIQUE;
    case FONTSLANT_REVERSE_ITALIC: return FC_SLANT_ITALIC; // No equivalent FC value
    case FONTSLANT_REVERSE_OBLIQUE:return FC_SLANT_OBLIQUE;// No equivalent FC value
    }
  return FC_SLANT_ROMAN;
  }


// From fontconfig slant to FOX slant
static FXint fcSlant2Slant(FXint fcSlant){
  switch(fcSlant){
    case FC_SLANT_ROMAN:  return FONTSLANT_REGULAR;
    case FC_SLANT_ITALIC: return FONTSLANT_ITALIC;
    case FC_SLANT_OBLIQUE:return FONTSLANT_OBLIQUE;
    }
  return FONTSLANT_REGULAR;
  }

#ifdef FC_WIDTH
// From FOX setwidth to fontconfig setwidth
static FXint setWidth2FcSetWidth(FXint setwidth){
  switch(setwidth){
    case FONTSETWIDTH_ULTRACONDENSED:return FC_WIDTH_ULTRACONDENSED;
    case FONTSETWIDTH_EXTRACONDENSED:return FC_WIDTH_EXTRACONDENSED;
    case FONTSETWIDTH_CONDENSED:     return FC_WIDTH_CONDENSED;
    case FONTSETWIDTH_SEMICONDENSED: return FC_WIDTH_SEMICONDENSED;
    case FONTSETWIDTH_NORMAL:        return FC_WIDTH_NORMAL;
    case FONTSETWIDTH_SEMIEXPANDED:  return FC_WIDTH_SEMIEXPANDED;
    case FONTSETWIDTH_EXPANDED:      return FC_WIDTH_EXPANDED;
    case FONTSETWIDTH_EXTRAEXPANDED: return FC_WIDTH_EXTRAEXPANDED;
    case FONTSETWIDTH_ULTRAEXPANDED: return FC_WIDTH_ULTRAEXPANDED;
    }
  return FC_WIDTH_NORMAL;
  }


// From fontconfig setwidth to FOX setwidth
static FXint fcSetWidth2SetWidth(FXint fcSetWidth){
  switch(fcSetWidth){
    case FC_WIDTH_ULTRACONDENSED:return FONTSETWIDTH_ULTRACONDENSED;
    case FC_WIDTH_EXTRACONDENSED:return FONTSETWIDTH_EXTRACONDENSED;
    case FC_WIDTH_CONDENSED:     return FONTSETWIDTH_CONDENSED;
    case FC_WIDTH_SEMICONDENSED: return FONTSETWIDTH_SEMICONDENSED;
    case FC_WIDTH_NORMAL:        return FONTSETWIDTH_NORMAL;
    case FC_WIDTH_SEMIEXPANDED:  return FONTSETWIDTH_SEMIEXPANDED;
    case FC_WIDTH_EXPANDED:      return FONTSETWIDTH_EXPANDED;
    case FC_WIDTH_EXTRAEXPANDED: return FONTSETWIDTH_EXTRAEXPANDED;
    case FC_WIDTH_ULTRAEXPANDED: return FONTSETWIDTH_ULTRAEXPANDED;
    }
  return FONTSETWIDTH_NORMAL;
  }
#endif

// Build fontconfig pattern from face name and hints
static FcPattern* buildPatternXft(const FXchar* face,FXuint size,FXuint weight,FXuint slant,FXuint setwidth,FXuint encoding,FXuint hints){
  FXchar family[104];
  FXchar foundry[104];

  // Create pattern object
  FcPattern* pattern=FcPatternCreate();

  // Family and foundry name
  familyandfoundryfromname(family,foundry,face);

  FXTRACE((150,"family=\"%s\", foundry=\"%s\"\n",family,foundry));

  // Set family
  if(strlen(family)>0){
    FcPatternAddString(pattern,FC_FAMILY,(const FcChar8*)family);
    }

  // Set foundry
  if(strlen(foundry)>0){
    FcPatternAddString(pattern,FC_FOUNDRY,(const FcChar8*)foundry);
    }

  // Set point size
  if(size>0){
    // Should be 10 but this makes it slightly better;
    // Suggested by "Niall Douglas" <s_sourceforge@nedprod.com>.
    FcPatternAddDouble(pattern,FC_SIZE,size/7.5);
//    FcPatternAddDouble(pattern,FC_SIZE,size/10.0);
//  FcPatternAddInteger(pattern,FC_PIXEL_SIZE,size/10);
    }

  // Set font weight
  if(weight!=FONTWEIGHT_DONTCARE){
    FcPatternAddInteger(pattern,FC_WEIGHT,weight2FcWeight(weight));
    }
#ifdef FC_WIDTH
  // Set setwidth
  if(setwidth!=FONTSETWIDTH_DONTCARE){
    FcPatternAddInteger(pattern,FC_WIDTH,setWidth2FcSetWidth(setwidth));
    }
#endif
  // Set slant
  if(slant!=FONTSLANT_DONTCARE){
    FcPatternAddInteger(pattern,FC_SLANT,slant2FcSlant(slant));
    }

  // Set encoding
  if(encoding!=FONTENCODING_DEFAULT){
    FcCharSet* charSet=FcCharSetCreate();
    //encoding2FcCharSet((void*)charSet, (FXFontEncoding)encoding);
    //FcPatternAddCharSet(pattern, FC_CHARSET, charSet);
    FcCharSetDestroy(charSet);
    }

  // Set pitch
  if((hints&FONTPITCH_FIXED)!=0){
    FcPatternAddInteger(pattern,FC_SPACING,FC_MONO);
    }

  // Set mono/proportional hint
  if((hints&FONTPITCH_VARIABLE)!=0){
    FcPatternAddInteger(pattern,FC_SPACING,FC_PROPORTIONAL);
    }

  // Scalable font hint
  if((hints&FONTHINT_SCALABLE)!=0){
    FcPatternAddBool(pattern,FC_SCALABLE,TRUE);
    }

  //if((hints&FONTHINT_X11) != 0)
    //FcPatternAddBool(pattern, FC_CORE, TRUE);

  // Return pattern object
  return pattern;
  }




#else                                   // Using XLFD



// Convert text to font weight
static FXuint weightfromtext(const FXchar* text){
  register FXchar c1=tolower((FXuchar)text[0]);
  register FXchar c2=tolower((FXuchar)text[1]);
  if(c1=='l' && c2=='i') return FONTWEIGHT_LIGHT;
  if(c1=='n' && c2=='o') return FONTWEIGHT_NORMAL;
  if(c1=='r' && c2=='e') return FONTWEIGHT_REGULAR;
  if(c1=='m' && c2=='e') return FONTWEIGHT_MEDIUM;
  if(c1=='d' && c2=='e') return FONTWEIGHT_DEMIBOLD;
  if(c1=='b' && c2=='o') return FONTWEIGHT_BOLD;
  if(c1=='b' && c2=='l') return FONTWEIGHT_BLACK;
  return FONTWEIGHT_DONTCARE;
  }


// Convert text to slant
static FXuint slantfromtext(const FXchar* text){
  register FXchar c1=tolower((FXuchar)text[0]);
  register FXchar c2=tolower((FXuchar)text[1]);
  if(c1=='i') return FONTSLANT_ITALIC;
  if(c1=='o') return FONTSLANT_OBLIQUE;
  if(c1=='r' && c2=='i') return FONTSLANT_REVERSE_ITALIC;
  if(c1=='r' && c2=='o') return FONTSLANT_REVERSE_OBLIQUE;
  if(c1=='r') return FONTSLANT_REGULAR;
  return FONTSLANT_DONTCARE;
  }


// Convert text to setwidth
static FXuint setwidthfromtext(const FXchar* text){
  if(text[0]=='m') return FONTSETWIDTH_MEDIUM;
  if(text[0]=='w') return FONTSETWIDTH_EXTRAEXPANDED;
  if(text[0]=='r') return FONTSETWIDTH_MEDIUM;
  if(text[0]=='c') return FONTSETWIDTH_CONDENSED;
  if(text[0]=='n'){
    if(text[1]=='a') return FONTSETWIDTH_CONDENSED;
    if(text[1]=='o') return FONTSETWIDTH_MEDIUM;
    return FONTSETWIDTH_DONTCARE;
    }
  if(text[0]=='e' && text[1]=='x' && text[2]=='p') return FONTSETWIDTH_EXPANDED;
  if(text[0]=='e' && text[1]=='x' && text[2]=='t' && text[3]=='r' && text[4]=='a'){
    if(text[5]=='c') return FONTSETWIDTH_EXTRACONDENSED;
    if(text[5]=='e') return FONTSETWIDTH_EXTRAEXPANDED;
    return FONTSETWIDTH_DONTCARE;
    }
  if(text[0]=='u' && text[1]=='l' && text[2]=='t' && text[3]=='r' && text[4]=='a'){
    if(text[5]=='c') return FONTSETWIDTH_ULTRACONDENSED;
    if(text[5]=='e') return FONTSETWIDTH_ULTRAEXPANDED;
    return FONTSETWIDTH_DONTCARE;
    }
  if((text[0]=='s' || text[0]=='d') && text[1]=='e' && text[2]=='m' && text[3]=='i'){
    if(text[5]=='c') return FONTSETWIDTH_SEMICONDENSED;
    if(text[5]=='e') return FONTSETWIDTH_SEMIEXPANDED;
    return FONTSETWIDTH_DONTCARE;
    }
  return FONTSETWIDTH_DONTCARE;
  }


// Convert pitch to flags
static FXuint pitchfromtext(const FXchar* text){
  register FXchar c=tolower((FXuchar)text[0]);
  if(c=='p') return FONTPITCH_VARIABLE;
  if(c=='m' || c=='c') return FONTPITCH_FIXED;
  return FONTPITCH_DEFAULT;
  }


// Test if font is ISO8859
static FXbool isISO8859(const FXchar* text){
  return tolower((FXuchar)text[0])=='i' && tolower((FXuchar)text[1])=='s' && tolower((FXuchar)text[2])=='o' && text[3]=='8' && text[4]=='8' && text[5]=='5' && text[6]=='9';
  }


// Test if font is KOI8
static FXbool isKOI8(const FXchar* text){
  return tolower((FXuchar)text[0])=='k' && tolower((FXuchar)text[1])=='o' && tolower((FXuchar)text[2])=='i' && text[3]=='8';
  }

// Test if font is microsoft cpXXXX
static FXbool isMSCP(const FXchar* text){
  return tolower((FXuchar)text[0])=='m' && tolower((FXuchar)text[1])=='i' && tolower((FXuchar)text[2])=='c' && tolower((FXuchar)text[3])=='r' && tolower((FXuchar)text[4])=='o' && tolower((FXuchar)text[5])=='s' && tolower((FXuchar)text[6])=='o' && tolower((FXuchar)text[7])=='f' && tolower((FXuchar)text[8])=='t';
  }


// Test if font is multi-byte
static FXbool ismultibyte(const FXchar* text){

  // Unicode font; not yet ...
  if(tolower((FXuchar)text[0])=='i' && tolower((FXuchar)text[1])=='s' && tolower((FXuchar)text[2])=='o' && text[3]=='6' && text[4]=='4' && text[5]=='6') return TRUE;

  // Japanese font
  if(tolower((FXuchar)text[0])=='j' && tolower((FXuchar)text[1])=='i' && tolower((FXuchar)text[2])=='s' && text[3]=='x') return TRUE;

  // Chinese font
  if(tolower((FXuchar)text[0])=='g' && tolower((FXuchar)text[1])=='b') return TRUE;

  // Another type of chinese font
  if(tolower((FXuchar)text[0])=='b' && tolower((FXuchar)text[1])=='i' && tolower((FXuchar)text[2])=='g' && text[3]=='5') return TRUE;

  // Korean
  if(tolower((FXuchar)text[0])=='k' && tolower((FXuchar)text[1])=='s' && tolower((FXuchar)text[2])=='c') return TRUE;

  return FALSE;
  }


// Encoding from xlfd-registry and xlfd-encoding
static FXuint encodingfromxlfd(const FXchar* registry,const FXchar* encoding){
  if(isISO8859(registry)){
    return FONTENCODING_ISO_8859_1+atoi(encoding)-1;
    }
  if(isKOI8(registry)){
    if(encoding[0]=='u' || encoding[0]=='U'){
      return FONTENCODING_KOI8_U;
      }
    if(encoding[0]=='r' || encoding[0]=='R'){
      return FONTENCODING_KOI8_R;
      }
    return FONTENCODING_KOI8;
    }
  if(isMSCP(registry)){
    if((encoding[0]=='c' || encoding[0]=='C') && (encoding[1]=='p' || encoding[1]=='P')){
      return atoi(encoding+2);
      }
    }
  return FONTENCODING_DEFAULT;
  }


// Get list of font names matching pattern
static char **listfontnames(Display* dpy,const char* pattern,int& numfnames){
  int maxfnames=1024;
  char **fnames;
  for(;;){
    fnames=XListFonts(dpy,pattern,maxfnames,&numfnames);
    if((fnames==NULL) || (numfnames<maxfnames)) break;
    XFreeFontNames(fnames);
    maxfnames<<=1;
    }
  return fnames;
  }


// Return number of fonts matching name
static int matchingfonts(Display* dpy,const char* pattern){
  char **fnames; int numfnames;
  fnames=listfontnames(dpy,pattern,numfnames);
  XFreeFontNames(fnames);
  if(numfnames>0) FXTRACE((100,"matched: %s\n",pattern));
  return numfnames;
  }


// Parse font name into parts
static int parsefontname(char** fields,char* fontname){
  register char** end=fields+XLFD_ENCODING;
  register char** beg=fields;
  register char*  ptr=fontname;
  if(*ptr++ == '-'){
    while(1){
      *beg++=ptr;
      if(beg>end) return 1;
      while(*ptr!='-'){
        if(*ptr=='\0') return 0;
        ptr++;
        }
      *ptr++='\0';
      }
    }
  return 0;
  }


// Try find matching font
char* FXFont::findmatch(char* fontname,const char* forge,const char* family){
  FXchar candidate[MAX_XLFD],*field[14],**fnames;
  FXuint size,weight,slant,setwidth,encoding,pitch;
  FXint bestf,bestdweight,bestdsize,bestvalue,bestscalable,bestxres,bestyres;
  FXint screenres,xres,yres;
  FXint dweight,scalable,dsize;
  FXint numfnames,f,value;

  // Get fonts matching the pattern
  sprintf(candidate,"-%s-%s-*-*-*-*-*-*-*-*-*-*-*-*",forge,family);
  fnames=listfontnames(DISPLAY(getApp()),candidate,numfnames);
  if(!fnames) return NULL;

  // Init match values
  bestf=-1;
  bestvalue=0;

  bestdsize=10000000;
  bestdweight=10000000;
  bestscalable=0;
  bestxres=75;
  bestyres=75;

  // Perhaps override screen resolution via registry
  screenres=getApp()->reg().readUnsignedEntry("SETTINGS","screenres",100);

  // Validate
  if(screenres<50) screenres=50;
  if(screenres>200) screenres=200;

  FXTRACE((150,"Matching Fonts for screenres=%d :\n",screenres));

  // Loop over all fonts to find the best match
  for(f=0; f<numfnames; f++){
    strncpy(candidate,fnames[f],MAX_XLFD);
    if(parsefontname(field,candidate)){

      // This font's match value
      value=0;
      scalable=0;
      dsize=1000000;
      dweight=1000;

      // Match encoding
      encoding=encodingfromxlfd(field[XLFD_REGISTRY],field[XLFD_ENCODING]);
      if(wantedEncoding==FONTENCODING_DEFAULT){
        if(encoding==FONTENCODING_ISO_8859_1) value+=ENCODING_FACTOR;
        }
      else{
        if(encoding==wantedEncoding) value+=ENCODING_FACTOR;
        }

      // Match pitch
      pitch=pitchfromtext(field[XLFD_SPACING]);
      if(hints&FONTPITCH_FIXED){
        if(pitch&FONTPITCH_FIXED) value+=PITCH_FACTOR;
        }
      else if(hints&FONTPITCH_VARIABLE){
        if(pitch&FONTPITCH_VARIABLE) value+=PITCH_FACTOR;
        }

      // Scalable
      if(EQUAL1(field[XLFD_PIXELSIZE],'0') && EQUAL1(field[XLFD_POINTSIZE],'0') && EQUAL1(field[XLFD_AVERAGE],'0')){
        value+=SCALABLE_FACTOR;
        scalable=1;
        }
      else{
        if(!(hints&FONTHINT_SCALABLE)) value+=SCALABLE_FACTOR;
        }

      // Polymorphic
      if(EQUAL1(field[XLFD_WEIGHT],'0') || EQUAL1(field[XLFD_SETWIDTH],'0') || EQUAL1(field[XLFD_SLANT],'0') || EQUAL1(field[XLFD_ADDSTYLE],'0')){
        value+=POLY_FACTOR;
        }
      else{
        if(!(hints&FONTHINT_POLYMORPHIC)) value+=POLY_FACTOR;
        }

      // Prefer normal weight
      weight=weightfromtext(field[XLFD_WEIGHT]);
      if(wantedWeight==FONTWEIGHT_DONTCARE){
        dweight=weight-FONTWEIGHT_NORMAL;
        dweight=FXABS(dweight);
        }
      else{
        dweight=weight-wantedWeight;
        dweight=FXABS(dweight);
        }

      // Prefer normal slant
      slant=slantfromtext(field[XLFD_SLANT]);
      if(wantedSlant==FONTSLANT_DONTCARE){
        if(slant==FONTSLANT_REGULAR) value+=SLANT_FACTOR;
        }
      else{
        if(slant==wantedSlant) value+=SLANT_FACTOR;
        }

      // Prefer unexpanded and uncompressed
      setwidth=setwidthfromtext(field[XLFD_SETWIDTH]);
      if(wantedSetwidth==FONTSETWIDTH_DONTCARE){
        if(setwidth==FONTSETWIDTH_NORMAL) value+=SETWIDTH_FACTOR;
        }
      else{
        if(setwidth==wantedSetwidth) value+=SETWIDTH_FACTOR;
        }

      // The font can be rendered at any resolution, so render at actual device resolution
      if(EQUAL1(field[XLFD_RESOLUTION_X],'0') && EQUAL1(field[XLFD_RESOLUTION_Y],'0')){
        xres=screenres;
        yres=screenres;
        }

      // Else get the resolution for which the font is designed
      else{
        xres=atoi(field[XLFD_RESOLUTION_X]);
        yres=atoi(field[XLFD_RESOLUTION_Y]);
        }

      // If scalable, we can of course get the exact size we want
      // We do not set dsize to 0, as we prefer a bitmapped font that gets within
      // 10% over a scalable one that's exact, as the bitmapped fonts look much better
      // at small sizes than scalable ones...
      if(scalable){
        value+=SIZE_FACTOR;
        dsize=wantedSize/10;
        size=wantedSize;
        }

      // Otherwise, we try to get something close
      else{

        // We correct for the actual screen resolution; if the font is rendered at a
        // 100 dpi, and we have a screen with 90dpi, the actual point size of the font
        // should be multiplied by (100/90).
        size=(yres*atoi(field[XLFD_POINTSIZE]))/screenres;

        // We strongly prefer the largest pointsize not larger than the desired pointsize
        if(size<=wantedSize){
          value+=SIZE_FACTOR;
          dsize=wantedSize-size;
          }

        // But if we can't get that, we'll take anything thats close...
        else{
          dsize=size-wantedSize;
          }
        }

      FXTRACE((160,"%4d: match=%-3x dw=%-3d ds=%3d sc=%d xres=%-3d yres=%-3d xlfd=\"%s\"\n",f,value,dweight,dsize,scalable,xres,yres,fnames[f]));

      // How close is the match?
      if((value>bestvalue) || ((value==bestvalue) && (dsize<bestdsize)) || ((value==bestvalue) && (dsize==bestdsize) && (dweight<bestdweight))){
        bestvalue=value;
        actualName=field[XLFD_FAMILY];
        actualName.append(" [");
        actualName.append(field[XLFD_FOUNDRY]);
        actualName.append("]");
        actualSize=size;
        actualWeight=weight;
        actualSlant=slant;
        actualSetwidth=setwidth;
        actualEncoding=encoding;
        bestdsize=dsize;
        bestdweight=dweight;
        bestscalable=scalable;
        bestxres=xres;
        bestyres=yres;
        bestf=f;
        }
      }
    }

  // Got one
  if(0<=bestf){
    if(!bestscalable){
      strncpy(fontname,fnames[bestf],MAX_XLFD);
      }
    else{
      strncpy(candidate,fnames[bestf],MAX_XLFD);
      parsefontname(field,candidate);

      // Build XLFD, correcting for possible difference between font resolution and screen resolution
      sprintf(fontname,"-%s-%s-%s-%s-%s-%s-*-%d-%d-%d-%s-*-%s-%s",field[XLFD_FOUNDRY],field[XLFD_FAMILY],field[XLFD_WEIGHT],field[XLFD_SLANT],field[XLFD_SETWIDTH],field[XLFD_ADDSTYLE],(bestyres*wantedSize)/screenres,bestxres,bestyres,field[XLFD_SPACING],field[XLFD_REGISTRY],field[XLFD_ENCODING]);

      // This works!! draw text sideways:- but need to get some more experience with this
      //sprintf(fontname,"-%s-%s-%s-%s-%s-%s-*-[0 64 ~64 0]-%d-%d-%s-*-%s-%s",field[XLFD_FOUNDRY],field[XLFD_FAMILY],field[XLFD_WEIGHT],field[XLFD_SLANT],field[XLFD_SETWIDTH],field[XLFD_ADDSTYLE],screenxres,screenyres,field[XLFD_SPACING],field[XLFD_REGISTRY],field[XLFD_ENCODING]);
      }
    FXTRACE((150,"Best Font:\n"));
    FXTRACE((150,"%4d: match=%3x dw=%-3d ds=%-3d sc=%d xres=%-3d yres=%-3d xlfd=\"%s\"\n",bestf,bestvalue,bestdweight,bestdsize,bestscalable,bestxres,bestyres,fontname));

    // Free fonts
    XFreeFontNames(fnames);
    return fontname;
    }

  // Free fonts
  XFreeFontNames(fnames);
  return NULL;
  }


// Try load the best matching font
char* FXFont::findbestfont(char *fontname){
  register const FXchar *altfoundry;
  register const FXchar *altfamily;
  FXchar family[104];
  FXchar foundry[104];

  // Family and foundry name
  familyandfoundryfromname(family,foundry,wantedName.text());

  // Try match with specified family and foundry
  if(family[0]){
    altfamily=getApp()->reg().readStringEntry("FONTSUBSTITUTIONS",family,family);
    if(foundry[0]){
      altfoundry=getApp()->reg().readStringEntry("FONTSUBSTITUTIONS",foundry,foundry);
      if(findmatch(fontname,altfoundry,altfamily)) return fontname;
      }
    if(findmatch(fontname,"*",altfamily)) return fontname;
    }

  // Try swiss if we didn't have a match yet
  if((hints&(FONTHINT_SWISS|FONTHINT_SYSTEM) || !(hints&FONTHINT_MASK))){
    altfamily=getApp()->reg().readStringEntry("FONTSUBSTITUTIONS","helvetica","helvetica");
    if(findmatch(fontname,"*",altfamily)) return fontname;
    }

  // Try roman if we didn't have a match yet
  if((hints&FONTHINT_ROMAN) || !(hints&FONTHINT_MASK)){
    altfamily=getApp()->reg().readStringEntry("FONTSUBSTITUTIONS","times","times");
    if(findmatch(fontname,"*",altfamily)) return fontname;
    }

  // Try modern if we didn't have a match yet
  if((hints&FONTHINT_MODERN) || !(hints&FONTHINT_MASK)){
    altfamily=getApp()->reg().readStringEntry("FONTSUBSTITUTIONS","courier","courier");
    if(findmatch(fontname,"*",altfamily)) return fontname;
    }

  // Try decorative if we didn't have a match yet
  if((hints&FONTHINT_DECORATIVE) || !(hints&FONTHINT_MASK)){
    altfamily=getApp()->reg().readStringEntry("FONTSUBSTITUTIONS","gothic","gothic");
    if(findmatch(fontname,"*",altfamily)) return fontname;
    }

  // Try anything
  if(findmatch(fontname,"*","*")) return fontname;

  // Fail
  return "";
  }


// Try these fallbacks for swiss hint
const char* swissfallback[]={
  "-*-helvetica-bold-r-*-*-*-120-*-*-*-*-*-*",
  "-*-lucida-bold-r-*-*-*-120-*-*-*-*-*-*",
  "-*-helvetica-medium-r-*-*-*-120-*-*-*-*-*-*",
  "-*-lucida-medium-r-*-*-*-120-*-*-*-*-*-*",
  "-*-helvetica-*-*-*-*-*-120-*-*-*-*-*-*",
  "-*-lucida-*-*-*-*-*-120-*-*-*-*-*-*",
  NULL
  };


// Try these fallbacks for times hint
const char* romanfallback[]={
  "-*-times-bold-r-*-*-*-120-*-*-*-*-*-*",
  "-*-charter-bold-r-*-*-*-120-*-*-*-*-*-*",
  "-*-times-medium-r-*-*-*-120-*-*-*-*-*-*",
  "-*-charter-medium-r-*-*-*-120-*-*-*-*-*-*",
  "-*-times-*-*-*-*-*-120-*-*-*-*-*-*",
  "-*-charter-*-*-*-*-*-120-*-*-*-*-*-*",
  NULL
  };


// Try these fallbacks for modern hint
const char* modernfallback[]={
  "-*-courier-bold-r-*-*-*-120-*-*-*-*-*-*",
  "-*-lucidatypewriter-bold-r-*-*-*-120-*-*-*-*-*-*",
  "-*-courier-medium-r-*-*-*-120-*-*-*-*-*-*",
  "-*-lucidatypewriter-medium-r-*-*-*-120-*-*-*-*-*-*",
  "-*-courier-*-*-*-*-*-120-*-*-*-*-*-*",
  "-*-lucidatypewriter-*-*-*-*-*-120-*-*-*-*-*-*",
  NULL
  };


// Try these final fallbacks
const char* finalfallback[]={
  "7x13",
  "8x13",
  "7x14",
  "8x16",
  "9x15",
  NULL
  };


// See which fallback font exists
static const char* fallbackfont(Display *dpy,FXuint hints){
  register const char *fname;
  register int i;

  // Try swiss if we wanted swiss, or if we don't care
  if((hints&FONTHINT_SWISS) || !(hints&FONTHINT_MASK)){
    for(i=0; (fname=swissfallback[i])!=NULL; i++){
      if(matchingfonts(dpy,fname)>0) return fname;
      }
    }

  // Try roman if we wanted roman, or if we don't care
  if((hints&FONTHINT_ROMAN) || !(hints&FONTHINT_MASK)){
    for(i=0; (fname=romanfallback[i])!=NULL; i++){
      if(matchingfonts(dpy,fname)>0) return fname;
      }
    }

  // Try modern if we wanted modern, or if we don't care
  if((hints&FONTHINT_MODERN) || !(hints&FONTHINT_MASK)){
    for(i=0; (fname=modernfallback[i])!=NULL; i++){
      if(matchingfonts(dpy,fname)>0) return fname;
      }
    }

  // Try final fallback fonts
  for(i=0; (fname=finalfallback[i])!=NULL; i++){
    if(matchingfonts(dpy,fname)>0) return fname;
    }

  // This one has to be there
  return "fixed";
  }

#endif


/*******************************************************************************/

// Helper functions WIN32

#else


// Character set encoding
static BYTE FXFontEncoding2CharSet(FXuint encoding){
  switch(encoding){
    case FONTENCODING_DEFAULT: return DEFAULT_CHARSET;
    case FONTENCODING_TURKISH: return TURKISH_CHARSET;
    case FONTENCODING_BALTIC: return BALTIC_CHARSET;
    case FONTENCODING_CYRILLIC: return RUSSIAN_CHARSET;
    case FONTENCODING_ARABIC: return ARABIC_CHARSET;
    case FONTENCODING_GREEK: return GREEK_CHARSET;
    case FONTENCODING_HEBREW: return HEBREW_CHARSET;
    case FONTENCODING_THAI: return THAI_CHARSET;
    case FONTENCODING_EASTEUROPE: return EASTEUROPE_CHARSET;
    case FONTENCODING_USASCII: return ANSI_CHARSET;
    }
  return DEFAULT_CHARSET;
  }


// Character set encoding
static FXuint CharSet2FXFontEncoding(BYTE lfCharSet){
  switch(lfCharSet){
    case ANSI_CHARSET: return FONTENCODING_USASCII;
    case ARABIC_CHARSET: return FONTENCODING_ARABIC;
    case BALTIC_CHARSET: return FONTENCODING_BALTIC;
    case CHINESEBIG5_CHARSET: return FONTENCODING_DEFAULT;
    case DEFAULT_CHARSET: return FONTENCODING_DEFAULT;
    case EASTEUROPE_CHARSET: return FONTENCODING_EASTEUROPE;
    case GB2312_CHARSET: return FONTENCODING_DEFAULT;
    case GREEK_CHARSET: return FONTENCODING_GREEK;
    case HANGUL_CHARSET: return FONTENCODING_DEFAULT;
    case HEBREW_CHARSET: return FONTENCODING_HEBREW;
    case MAC_CHARSET: return FONTENCODING_DEFAULT;
    case OEM_CHARSET: return FONTENCODING_DEFAULT;
    case SYMBOL_CHARSET: return FONTENCODING_DEFAULT;
    case RUSSIAN_CHARSET: return FONTENCODING_CYRILLIC;
    case SHIFTJIS_CHARSET: return FONTENCODING_DEFAULT;
    case THAI_CHARSET: return FONTENCODING_THAI;
    case TURKISH_CHARSET: return FONTENCODING_TURKISH;
    }
  return FONTENCODING_DEFAULT;
  }


// Yuk. Need to get some data into the callback function.
struct FXFontStore {
  HDC         hdc;
  FXFontDesc *fonts;
  FXuint      numfonts;
  FXuint      size;
  FXFontDesc  desc;
  };


// Callback function for EnumFontFamiliesEx()
static int CALLBACK EnumFontFamExProc(const LOGFONT *lf,const TEXTMETRIC *lptm,DWORD FontType,LPARAM lParam){
  register FXFontStore *pFontStore=(FXFontStore*)lParam;
  FXASSERT(lf);
  FXASSERT(lptm);
  FXASSERT(pFontStore);

  // Get pitch
  FXuint flags=FONTPITCH_DEFAULT;
  if(lf->lfPitchAndFamily&FIXED_PITCH) flags|=FONTPITCH_FIXED;
  if(lf->lfPitchAndFamily&VARIABLE_PITCH) flags|=FONTPITCH_VARIABLE;

  // Get hints
  if(lf->lfPitchAndFamily&FF_DONTCARE) flags|=FONTHINT_DONTCARE;
  if(lf->lfPitchAndFamily&FF_MODERN) flags|=FONTHINT_MODERN;
  if(lf->lfPitchAndFamily&FF_ROMAN) flags|=FONTHINT_ROMAN;
  if(lf->lfPitchAndFamily&FF_SCRIPT) flags|=FONTHINT_SCRIPT;
  if(lf->lfPitchAndFamily&FF_DECORATIVE) flags|=FONTHINT_DECORATIVE;
  if(lf->lfPitchAndFamily&FF_SWISS) flags|=FONTHINT_SWISS;

  // Skip if no match
  FXuint h=pFontStore->desc.flags;
  if((h&FONTPITCH_FIXED) && !(flags&FONTPITCH_FIXED)) return 1;
  if((h&FONTPITCH_VARIABLE) && !(flags&FONTPITCH_VARIABLE)) return 1;

  // Get weight (also guess from the name)
  FXuint weight=lf->lfWeight;
  if(strstr(lf->lfFaceName,"Bold")!=NULL) weight=FONTWEIGHT_BOLD;
  if(strstr(lf->lfFaceName,"Black")!=NULL) weight=FONTWEIGHT_BLACK;
  if(strstr(lf->lfFaceName,"Demi")!=NULL) weight=FONTWEIGHT_DEMIBOLD;
  if(strstr(lf->lfFaceName,"Light")!=NULL) weight=FONTWEIGHT_LIGHT;
  if(strstr(lf->lfFaceName,"Medium")!=NULL) weight=FONTWEIGHT_MEDIUM;

  // Skip if weight doesn't match
  FXuint wt=pFontStore->desc.weight;
  if((wt!=FONTWEIGHT_DONTCARE) && (wt!=weight)) return 1;

  // Get slant
  FXuint slant=FONTSLANT_REGULAR;
  if(lf->lfItalic==TRUE) slant=FONTSLANT_ITALIC;
  if(strstr(lf->lfFaceName,"Italic")!=NULL) slant=FONTSLANT_ITALIC;
  if(strstr(lf->lfFaceName,"Roman")!=NULL) slant=FONTSLANT_REGULAR;

  // Skip if no match
  FXuint sl=pFontStore->desc.slant;
  if((sl!=FONTSLANT_DONTCARE) && (sl!=slant)) return 1;

  // Get set width (also guess from the name)
  FXuint setwidth=FONTSETWIDTH_DONTCARE;
  if(strstr(lf->lfFaceName,"Cond")!=NULL) setwidth=FONTSETWIDTH_CONDENSED;
  if(strstr(lf->lfFaceName,"Narrow")!=NULL) setwidth=FONTSETWIDTH_NARROW;
  if(strstr(lf->lfFaceName,"Ext Cond")!=NULL) setwidth=FONTSETWIDTH_EXTRACONDENSED;

  // Skip if no match
  FXuint sw=pFontStore->desc.setwidth;
  if((sw!=FONTSETWIDTH_DONTCARE) && (sw!=setwidth)) return 1;

  // Get encoding
  FXuint encoding=CharSet2FXFontEncoding(lf->lfCharSet);

  // Skip if no match
  FXuint en=pFontStore->desc.encoding;
  if((en!=FONTENCODING_DEFAULT) && (en!=encoding)) return 1;

  // Is it scalable?
  if(FontType==TRUETYPE_FONTTYPE){
    flags|=FONTHINT_SCALABLE;
    }

  // Is it polymorphic?
  if(FontType==TRUETYPE_FONTTYPE){
    flags|=FONTHINT_POLYMORPHIC;
    }

  // Initial allocation of storage?
  if(pFontStore->numfonts==0){
    FXMALLOC(&pFontStore->fonts,FXFontDesc,50);
    if(pFontStore->fonts==0) return 0;
    pFontStore->size=50;
    }

  // Grow the array if needed
  if(pFontStore->numfonts>=pFontStore->size){
    FXRESIZE(&pFontStore->fonts,FXFontDesc,pFontStore->size+50);
    if(pFontStore->fonts==0) return 0;
    pFontStore->size+=50;
    }

  FXFontDesc *fonts=pFontStore->fonts;
  FXuint numfonts=pFontStore->numfonts;

  strncpy(fonts[numfonts].face,lf->lfFaceName,sizeof(fonts[0].face));
  if(lf->lfHeight<0){
    fonts[numfonts].size=-MulDiv(lf->lfHeight,720,GetDeviceCaps(pFontStore->hdc,LOGPIXELSY));
    }
  else{
    fonts[numfonts].size=MulDiv(lf->lfHeight,720,GetDeviceCaps(pFontStore->hdc,LOGPIXELSY));
    }
  fonts[numfonts].weight=weight;
  fonts[numfonts].slant=slant;
  fonts[numfonts].encoding=encoding;
  fonts[numfonts].setwidth=setwidth;
  fonts[numfonts].flags=flags;

  pFontStore->fonts=fonts;
  pFontStore->numfonts++;

  // Must return 1 to continue enumerating fonts
  return 1;
  }



#endif


/*******************************************************************************/

// Object implementation
FXIMPLEMENT(FXFont,FXId,NULL,0)


// Deserialization
FXFont::FXFont(){
  wantedSize=0;
  actualSize=0;
  wantedWeight=0;
  actualWeight=0;
  wantedSlant=0;
  actualSlant=0;
  wantedSetwidth=0;
  actualSetwidth=0;
  wantedEncoding=0;
  actualEncoding=0;
  hints=0;
  font=NULL;
#ifdef WIN32
  dc=NULL;
#endif
  }


// Construct font from given font description
FXFont::FXFont(FXApp* a,const FXString& string):FXId(a){
  FXTRACE((100,"FXFont::FXFont %p\n",this));
  wantedSize=0;
  actualSize=0;
  wantedWeight=0;
  actualWeight=0;
  wantedSlant=0;
  actualSlant=0;
  wantedSetwidth=0;
  actualSetwidth=0;
  wantedEncoding=0;
  actualEncoding=0;
  hints=0;
  font=NULL;
#ifdef WIN32
  dc=NULL;
#endif
  setFont(string);
  }


// Construct a font with given family name, size in points(pixels), weight, slant, character set encoding, setwidth, and hints
FXFont::FXFont(FXApp* a,const FXString& face,FXuint sz,FXuint wt,FXuint sl,FXuint enc,FXuint setw,FXuint h):FXId(a),wantedName(face){
  FXTRACE((100,"FXFont::FXFont %p\n",this));
  wantedSize=10*sz;
  wantedWeight=wt;
  wantedSlant=sl;
  wantedSetwidth=setw;
  wantedEncoding=enc;
  actualSize=0;
  actualWeight=0;
  actualSlant=0;
  actualSetwidth=0;
  actualEncoding=0;
  hints=(h&~FONTHINT_X11);          // System-independent method
  font=NULL;
#ifdef WIN32
  dc=NULL;
#endif
  }


// Construct font from font description
FXFont::FXFont(FXApp* a,const FXFontDesc& fontdesc):FXId(a),wantedName(fontdesc.face){
  FXTRACE((100,"FXFont::FXFont %p\n",this));
  wantedSize=fontdesc.size;
  wantedWeight=fontdesc.weight;
  wantedSlant=fontdesc.slant;
  wantedSetwidth=fontdesc.setwidth;
  wantedEncoding=fontdesc.encoding;
  actualSize=0;
  actualWeight=0;
  actualSlant=0;
  actualSetwidth=0;
  actualEncoding=0;
  hints=fontdesc.flags;
  font=NULL;
#ifdef WIN32
  dc=NULL;
#endif
  }


/*******************************************************************************/


// Create font
void FXFont::create(){
  if(!xid){
    if(getApp()->isInitialized()){
      FXTRACE((100,"%s::create %p\n",getClassName(),this));
#ifndef WIN32

#ifdef HAVE_XFT_H                       // Using XFT
      FcPattern *pattern,*p;
      FcResult result;

      FXTRACE((150,"%s::create: Xft font=\"%s\"\n",getClassName(),wantedName.text()));

      // Build pattern
      pattern=buildPatternXft(wantedName.text(),wantedSize,wantedWeight,wantedSlant,wantedSetwidth,wantedEncoding,hints);

      // Pattern substitutions
      FcConfigSubstitute(0,pattern,FcMatchPattern);
      FcDefaultSubstitute(pattern);

      // Find pattern matching a font
      p=FcFontMatch(0,pattern,&result);

      // Create font
      font=XftFontOpenPattern(DISPLAY(getApp()),p);     // FIXME currently unknown how to determine what font was matched
      xid=(unsigned long)font;

      // Destroy pattern
      FcPatternDestroy(pattern);

      // Uh-oh, we failed
      if(!xid){ fxerror("%s::create: unable to create font.\n",getClassName()); }

#else                                   // Using XLFD

      char fontname[MAX_XLFD];

      // X11 font specification
      if(hints&FONTHINT_X11){

        FXTRACE((150,"%s::create: X11 font=\"%s\"\n",getClassName(),wantedName.text()));

        // Try load the font
        font=XLoadQueryFont(DISPLAY(getApp()),wantedName.text());
        }

      // Platform independent specification
      else{

        FXTRACE((150,"%s::create: findbestfont\n",getClassName()));

        // Try load best font
        font=XLoadQueryFont(DISPLAY(getApp()),findbestfont(fontname));
        }

      // If we still don't have a font yet, try fallback fonts
      if(!font){
        FXTRACE((150,"%s::create: fallback\n",getClassName()));
        font=XLoadQueryFont(DISPLAY(getApp()),fallbackfont(DISPLAY(getApp()),hints));
        }

      // Remember font id
      if(font){ xid=((XFontStruct*)font)->fid; }

      // Uh-oh, we failed
      if(!xid){ fxerror("%s::create: unable to create font.\n",getClassName()); }

      // Dump some useful stuff
      FXTRACE((150,"min_char_or_byte2   = %d\n",((XFontStruct*)font)->min_char_or_byte2));
      FXTRACE((150,"max_char_or_byte2   = %d\n",((XFontStruct*)font)->max_char_or_byte2));
      FXTRACE((150,"default_char        = %c\n",((XFontStruct*)font)->default_char));
      FXTRACE((150,"min_bounds.lbearing = %d\n",((XFontStruct*)font)->min_bounds.lbearing));
      FXTRACE((150,"min_bounds.rbearing = %d\n",((XFontStruct*)font)->min_bounds.rbearing));
      FXTRACE((150,"min_bounds.width    = %d\n",((XFontStruct*)font)->min_bounds.width));
      FXTRACE((150,"min_bounds.ascent   = %d\n",((XFontStruct*)font)->min_bounds.ascent));
      FXTRACE((150,"min_bounds.descent  = %d\n",((XFontStruct*)font)->min_bounds.descent));
      FXTRACE((150,"max_bounds.lbearing = %d\n",((XFontStruct*)font)->max_bounds.lbearing));
      FXTRACE((150,"max_bounds.rbearing = %d\n",((XFontStruct*)font)->max_bounds.rbearing));
      FXTRACE((150,"max_bounds.width    = %d\n",((XFontStruct*)font)->max_bounds.width));
      FXTRACE((150,"max_bounds.ascent   = %d\n",((XFontStruct*)font)->max_bounds.ascent));
      FXTRACE((150,"max_bounds.descent  = %d\n",((XFontStruct*)font)->max_bounds.descent));
#endif

#else
      FXchar buffer[256];

      // Windows will not support the X11 font string specification method
      if(hints&FONTHINT_X11){ fxerror("%s::create: this method of font specification not supported under Windows.\n",getClassName()); }

      // Hang on to this for text metrics functions
      dc=CreateCompatibleDC(NULL);

      // Now fill in the fields
      LOGFONT lf;
      lf.lfHeight=-MulDiv(wantedSize,GetDeviceCaps((HDC)dc,LOGPIXELSY),720);
      lf.lfWidth=0;
      lf.lfEscapement=0;
      lf.lfOrientation=0;
      lf.lfWeight=wantedWeight;
      if((wantedSlant==FONTSLANT_ITALIC) || (wantedSlant==FONTSLANT_OBLIQUE))
        lf.lfItalic=TRUE;
      else
        lf.lfItalic=FALSE;
      lf.lfUnderline=FALSE;
      lf.lfStrikeOut=FALSE;

      // Character set encoding
      lf.lfCharSet=FXFontEncoding2CharSet(wantedEncoding);

      // Other hints
      lf.lfOutPrecision=OUT_DEFAULT_PRECIS;
      if(hints&FONTHINT_SYSTEM) lf.lfOutPrecision=OUT_RASTER_PRECIS;
      if(hints&FONTHINT_SCALABLE) lf.lfOutPrecision=OUT_TT_PRECIS;
      if(hints&FONTHINT_POLYMORPHIC) lf.lfOutPrecision=OUT_TT_PRECIS;

      // Clip precision
      lf.lfClipPrecision=CLIP_DEFAULT_PRECIS;

      // Quality
      lf.lfQuality=DEFAULT_QUALITY;

      // Pitch and Family
      lf.lfPitchAndFamily=0;

      // Pitch
      if(hints&FONTPITCH_FIXED) lf.lfPitchAndFamily|=FIXED_PITCH;
      else if(hints&FONTPITCH_VARIABLE) lf.lfPitchAndFamily|=VARIABLE_PITCH;
      else lf.lfPitchAndFamily|=DEFAULT_PITCH;

      // Family
      if(hints&FONTHINT_DECORATIVE) lf.lfPitchAndFamily|=FF_DECORATIVE;
      else if(hints&FONTHINT_MODERN) lf.lfPitchAndFamily|=FF_MODERN;
      else if(hints&FONTHINT_ROMAN) lf.lfPitchAndFamily|=FF_ROMAN;
      else if(hints&FONTHINT_SCRIPT) lf.lfPitchAndFamily|=FF_SCRIPT;
      else if(hints&FONTHINT_SWISS) lf.lfPitchAndFamily|=FF_SWISS;
      else lf.lfPitchAndFamily|=FF_DONTCARE;

      // Font substitution
      if(!wantedName.empty()){
        strncpy(lf.lfFaceName,getApp()->reg().readStringEntry("FONTSUBSTITUTIONS",wantedName.text(),wantedName.text()),sizeof(lf.lfFaceName));
        lf.lfFaceName[sizeof(lf.lfFaceName)-1]='\0';
        }
      else{
        lf.lfFaceName[0]='\0';
        }

      // Here we go!
      xid=CreateFontIndirect(&lf);

      // Uh-oh, we failed
      if(!xid){ fxerror("%s::create: unable to create font.\n",getClassName()); }

      // Obtain text metrics
      FXCALLOC(&font,TEXTMETRIC,1);
      SelectObject((HDC)dc,xid);
      GetTextMetrics((HDC)dc,(TEXTMETRIC*)font);

      // Get actual face name
      GetTextFace((HDC)dc,sizeof(buffer),buffer);
      actualName=buffer;
      actualSize=MulDiv(((TEXTMETRIC*)font)->tmHeight,720,GetDeviceCaps((HDC)dc,LOGPIXELSY)); // FIXME no sigar yet?
      actualWeight=((TEXTMETRIC*)font)->tmWeight;
      actualSlant=((TEXTMETRIC*)font)->tmItalic?FONTSLANT_ITALIC:FONTSLANT_REGULAR;
      actualSetwidth=0;
      actualEncoding=CharSet2FXFontEncoding(((TEXTMETRIC*)font)->tmCharSet);
#endif

      // What was really matched
      FXTRACE((100,"wantedName=%s wantedSize=%d wantedWeight=%d wantedSlant=%d wantedSetwidth=%d wantedEncoding=%d\n",wantedName.text(),wantedSize,wantedWeight,wantedSlant,wantedSetwidth,wantedEncoding));
      FXTRACE((100,"actualName=%s actualSize=%d actualWeight=%d actualSlant=%d actualSetwidth=%d actualEncoding=%d\n",actualName.text(),actualSize,actualWeight,actualSlant,actualSetwidth,actualEncoding));
      }
    }
  }


// Detach font
void FXFont::detach(){
  if(xid){
    FXTRACE((100,"%s::detach %p\n",getClassName(),this));

#ifndef WIN32

    // Free font struct w/o doing anything else...
#ifdef HAVE_XFT_H
    XftFontClose(DISPLAY(getApp()), (XftFont*)font);
#else
    XFreeFont(DISPLAY(getApp()),(XFontStruct*)font);
#endif

#else

    // Free font metrics
    FXFREE(&font);
#endif

    // Forget all about actual font
    actualName=FXString::null;
    actualSize=0;
    actualWeight=0;
    actualSlant=0;
    actualSetwidth=0;
    actualEncoding=0;
    font=NULL;
    xid=0;
    }
  }


// Destroy font
void FXFont::destroy(){
  if(xid){
    if(getApp()->isInitialized()){
      FXTRACE((100,"%s::destroy %p\n",getClassName(),this));

#ifndef WIN32

      // Free font
#ifdef HAVE_XFT_H
      XftFontClose(DISPLAY(getApp()),(XftFont*)font);
#else
      XFreeFont(DISPLAY(getApp()),(XFontStruct*)font);
#endif

#else

      // Necessary to prevent resource leak
      SelectObject((HDC)dc,GetStockObject(SYSTEM_FONT));

      // Delete font
      DeleteObject((HFONT)xid);

      // Delete dummy DC
      DeleteDC((HDC)dc);

      // Free font metrics
      FXFREE(&font);
#endif
      }

    // Forget all about actual font
    actualName=FXString::null;
    actualSize=0;
    actualWeight=0;
    actualSlant=0;
    actualSetwidth=0;
    actualEncoding=0;
    font=NULL;
    xid=0;
    }
  }


// Does font have given character glyph?
FXbool FXFont::hasChar(FXint ch) const {
  if(font) return HASCHAR(font,ch);
  return FALSE;
  }


// Get first character glyph in font
FXint FXFont::getMinChar() const {
  if(font) return FIRSTCHAR(font);
  return 0;
  }


// Get last character glyph in font
FXint FXFont::getMaxChar() const {
  if(font) return LASTCHAR(font);
  return 0;
  }


// Get font leading [that is lead-ing as in Pb!]
FXint FXFont::getFontLeading() const {
  if(font){
#ifndef WIN32
#ifdef HAVE_XFT_H
    return 0; // TODO
#else
    return ((XFontStruct*)font)->ascent+((XFontStruct*)font)->descent-((XFontStruct*)font)->max_bounds.ascent-((XFontStruct*)font)->max_bounds.descent;
#endif
#else
    return ((TEXTMETRIC*)font)->tmExternalLeading;
#endif
    }
  return 0;
  }


// Get font line spacing [height+leading]
FXint FXFont::getFontSpacing() const {
  if(font){
#ifndef WIN32
#ifdef HAVE_XFT_H
    return ((XftFont*)font)->ascent+((XftFont*)font)->descent;
#else
    return ((XFontStruct*)font)->ascent+((XFontStruct*)font)->descent;
#endif
#else
    return ((TEXTMETRIC*)font)->tmHeight; // tmHeight includes font point size plus internal leading
#endif
    }
  return 1;
  }


// Left bearing
FXint FXFont::leftBearing(FXchar ch) const {
  if(font){
#ifndef WIN32
#ifdef HAVE_XFT_H
    return 0; // FIXME
#else
    if(((XFontStruct*)font)->per_char){
      if(!HASCHAR(font,ch)){ ch=((XFontStruct*)font)->default_char; }
      return ((XFontStruct*)font)->per_char[(FXuint)ch-((XFontStruct*)font)->min_char_or_byte2].lbearing;
      }
    return ((XFontStruct*)font)->max_bounds.lbearing;
#endif
#else
    return 0; // FIXME
#endif
    }
  return 0;
  }


// Right bearing
FXint FXFont::rightBearing(FXchar ch) const {
  if(font){
#ifndef WIN32
#ifdef HAVE_XFT_H
    return 0; // FIXME
#else
    if(((XFontStruct*)font)->per_char){
      if(!HASCHAR(font,ch)){ ch=((XFontStruct*)font)->default_char; }
      return ((XFontStruct*)font)->per_char[(FXuint)ch-((XFontStruct*)font)->min_char_or_byte2].rbearing;
      }
    return ((XFontStruct*)font)->max_bounds.rbearing;
#endif
#else
    return 0; // FIXME
#endif
    }
  return 0;
  }


// Is it a mono space font
FXbool FXFont::isFontMono() const {
  if(font){
#ifndef WIN32
#ifdef HAVE_XFT_H
    return (hints&FONTPITCH_FIXED) != 0;      // FIXME this is just a hint, not about actual font
#else
    return ((XFontStruct*)font)->min_bounds.width == ((XFontStruct*)font)->max_bounds.width;
#endif
#else
    return !(((TEXTMETRIC*)font)->tmPitchAndFamily&TMPF_FIXED_PITCH);
#endif
    }
  return TRUE;
  }


// Get font width
FXint FXFont::getFontWidth() const {
  if(font){
#ifndef WIN32
#ifdef HAVE_XFT_H
    return ((XftFont*)font)->max_advance_width;
#else
    return ((XFontStruct*)font)->max_bounds.width;
#endif
#else
    return ((TEXTMETRIC*)font)->tmMaxCharWidth;
#endif
    }
  return 1;
  }


// Get font height
FXint FXFont::getFontHeight() const {
  if(font){
#ifndef WIN32
#ifdef HAVE_XFT_H
    return ((XftFont*)font)->ascent+((XftFont*)font)->descent;
#else
    return ((XFontStruct*)font)->ascent+((XFontStruct*)font)->descent;  // This is wrong!
    //return ((XFontStruct*)font)->max_bounds.ascent+((XFontStruct*)font)->max_bounds.descent;  // This is right!
#endif
#else
    return ((TEXTMETRIC*)font)->tmHeight;
#endif
    }
  return 1;
  }


// Get font ascent
FXint FXFont::getFontAscent() const {
  if(font){
#ifndef WIN32
#ifdef HAVE_XFT_H
    return ((XftFont*)font)->ascent;
#else
    return ((XFontStruct*)font)->ascent;
#endif
#else
    return ((TEXTMETRIC*)font)->tmAscent;
#endif
    }
  return 1;
  }


// Get font descent
FXint FXFont::getFontDescent() const {
  if(font){
#ifndef WIN32
#ifdef HAVE_XFT_H
    return ((XftFont*)font)->descent;
#else
    return ((XFontStruct*)font)->descent;
#endif
#else
    return ((TEXTMETRIC*)font)->tmDescent;
#endif
    }
  return 0;
  }


// Text width
FXint FXFont::getTextWidth(const FXchar *text,FXuint n) const {
  if(!text && n){ fxerror("%s::getTextWidth: NULL string argument\n",getClassName()); }
  if(font){
#ifndef WIN32
#ifdef HAVE_XFT_H
    XGlyphInfo extents;
    XftTextExtents8(DISPLAY(getApp()),(XftFont*)font,(const FcChar8*)text,n,&extents);
    return extents.xOff;
#else
    return XTextWidth((XFontStruct*)font,text,n);
#endif
#else
    SIZE size;
    FXASSERT(dc!=NULL);
    GetTextExtentPoint32((HDC)dc,text,n,&size);
    return size.cx;
#endif
    }
  return n;
  }


// Text width
FXint FXFont::getTextWidth(const FXString& text) const {
  return getTextWidth(text.text(),text.length());
  }


// Text height
FXint FXFont::getTextHeight(const FXchar *text,FXuint n) const {
  if(!text && n){ fxerror("%s::getTextHeight: NULL string argument\n",getClassName()); }
  if(font){
#ifndef WIN32
#ifdef HAVE_XFT_H
//    XGlyphInfo extents;
//    XftTextExtents8(DISPLAY(getApp()),(XftFont*)font,(const FcChar8*)text,n,&extents);
//    return extents.height; // TODO: Is this correct?

    // Patch from ivan.markov@wizcom.bg
    return ((XftFont*)font)->ascent+((XftFont*)font)->descent;
#else
    XCharStruct chst; int dir,asc,desc;
    XTextExtents((XFontStruct*)font,text,n,&dir,&asc,&desc,&chst);
    return asc+desc;
#endif
#else
    SIZE size;
    FXASSERT(dc!=NULL);
    GetTextExtentPoint32((HDC)dc,text,n,&size);
    return size.cy;
#endif
    }
  return 1;
  }


// Text height
FXint FXFont::getTextHeight(const FXString& text) const {
  return getTextHeight(text.text(),text.length());
  }


/*******************************************************************************/


// Function to sort by name, weight, slant, and size
static int comparefont(const void *a,const void *b){
  register FXFontDesc *fa=(FXFontDesc*)a;
  register FXFontDesc *fb=(FXFontDesc*)b;
  register FXint cmp=strcmp(fa->face,fb->face);
  return cmp ? cmp : (fa->weight!=fb->weight) ? fa->weight-fb->weight : (fa->slant!=fb->slant) ? fa->slant-fb->slant : fa->size-fb->size;
  }


#ifndef WIN32

#ifdef HAVE_XFT_H

/*
// TODO
const FXchar* FXFont::encodingName(FXFontEncoding encoding) {
  switch(encoding){
    case FONTENCODING_ISO_8859_1:  return "ISO-8859-1";
    case FONTENCODING_ISO_8859_2:  return "ISO-8859-2";
    case FONTENCODING_ISO_8859_3:  return "ISO-8859-3";
    case FONTENCODING_ISO_8859_4:  return "ISO-8859-4";
    case FONTENCODING_ISO_8859_5:  return "ISO-8859-5";
    //case FONTENCODING_ISO_8859_6:  return "ISO-8859-6";
    case FONTENCODING_ISO_8859_7:  return "ISO-8859-7";
    //case FONTENCODING_ISO_8859_8:  return "ISO-8859-8";
    case FONTENCODING_ISO_8859_9:  return "ISO-8859-9";
    //case FONTENCODING_ISO_8859_10: return "ISO-8859-10";
    //case FONTENCODING_ISO_8859_11: return "ISO-8859-11";
    //case FONTENCODING_ISO_8859_13: return "ISO-8859-12";
    //case FONTENCODING_ISO_8859_14: return "ISO-8859-13";
    case FONTENCODING_ISO_8859_15: return "ISO-8859-14";
    //case FONTENCODING_ISO_8859_16: return "ISO-8859-15";
    case FONTENCODING_KOI8:        return "ISO-8859-1"; // TODO
    case FONTENCODING_KOI8_R:      return "ISO-8859-1"; // TODO
    case FONTENCODING_KOI8_U:      return "ISO-8859-1"; // TODO
    case FONTENCODING_KOI8_UNIFIED:return "ISO-8859-1"; // TODO
    }
  return "ISO-8859-1";
  }
*/

/*
void FXFont::encoding2FcCharSet(void *cs,FXFontEncoding encoding){
  FXTextCodec* textcodec = FXTextCodec::codecForName(encodingName(encoding));
  FXASSERT(textcodec != NULL);

  FcCharSet *charSet = (FcCharSet*)cs;

  FXuchar charset1[256];
  FXwchar charset4[256];
  // TODO: There should be a way in FXTextCodec to return the single-byte ranges covered by the particular charset
  for(int i = 0; i < 256; i++)
    charset1[i] = i >= 32 && (i < 0x80 || i > 0x9f) && i < 128? (unsigned char)i: 32;

  const FXuchar* chs1 = charset1;
  FXwchar* chs4 = charset4;
  textcodec->toUnicode(chs4, 256LU, chs1, 256LU);

  //for(int j = 0; j < 256; j++)
//    FcCharSetAddChar(charSet, 33  / * charset4[j] * /);
  }
*/



// List all fonts that match the passed requirements
FXbool FXFont::listFonts(FXFontDesc*& fonts,FXuint& numfonts,const FXString& face,FXuint wt,FXuint sl,FXuint sw,FXuint en,FXuint h){
#ifdef FC_WIDTH
  FcObjectSet *objset = FcObjectSetBuild(FC_FAMILY, FC_FOUNDRY, FC_SPACING, FC_SCALABLE, FC_WIDTH, FC_PIXEL_SIZE, FC_WEIGHT, FC_SLANT, NULL);
#else
  FcObjectSet *objset = FcObjectSetBuild(FC_FAMILY, FC_FOUNDRY, FC_SPACING, FC_SCALABLE, FC_PIXEL_SIZE, FC_WEIGHT, FC_SLANT, NULL);
#endif

  // The pattern will contain all passed required font properties
  FcPattern *pattern=buildPatternXft(face.text(),0,wt,sl,sw,en,h);

  FcFontSet *fontset=FcFontList(0,pattern,objset);

  numfonts = fontset->nfont;
  if(numfonts>0){
    FXMALLOC(&fonts,FXFontDesc,numfonts);

    unsigned realnumfonts = 0;

    for(unsigned i=0; i<numfonts; i++){
      FXFontDesc *desc = &fonts[realnumfonts];
      FcPattern *p = fontset->fonts[i];

      FcChar8 *family, *foundry;
      int spacing, setwidth, weight, slant, size;
      FcBool scalable;

      if(FcPatternGetString(p, FC_FAMILY, 0, &family) != FcResultMatch)
        continue;

      FXString fc = (const char*)family;

      if(FcPatternGetString(p, FC_FOUNDRY, 0, &foundry) == FcResultMatch)
        fc = fc + " [" + (const char*)foundry + "]";

      strncpy(desc->face, fc.text(), sizeof(desc->face) - 1);

#ifdef FC_WIDTH
      if(FcPatternGetInteger(p, FC_WIDTH, 0, &setwidth) == FcResultMatch)
        desc->setwidth = fcSetWidth2SetWidth(setwidth);
      else
#endif
        desc->setwidth = FONTSETWIDTH_NORMAL;

      if(FcPatternGetInteger(p, FC_PIXEL_SIZE, 0, &size) == FcResultMatch)
        desc->size = size*10;
      else
        desc->size = 0;

      if(FcPatternGetInteger(p, FC_WEIGHT, 0, &weight) == FcResultMatch)
        desc->weight = fcWeight2Weight(weight);
      else
        desc->weight = FONTWEIGHT_NORMAL;

      if(FcPatternGetInteger(p, FC_SLANT, 0, &slant) == FcResultMatch)
        desc->slant = fcSlant2Slant(slant);
      else
        desc->slant = FONTSLANT_REGULAR;

      if(FcPatternGetInteger(p, FC_SPACING, 0, &spacing) == FcResultMatch) {
        if(spacing == FC_PROPORTIONAL)
          desc->flags |= FONTPITCH_VARIABLE;
        else if(spacing == FC_MONO)
          desc->flags |= FONTPITCH_FIXED;
      } else
          desc->flags |= FONTPITCH_VARIABLE;

      if(FcPatternGetBool(p, FC_SCALABLE, 0, &scalable) == FcResultMatch) {
        if(scalable)
          desc->flags |= FONTHINT_SCALABLE;
      }

      desc->encoding = en; //FIXME fcEncoding2Encoding(encoding);

      if(face.empty()) {
        // Should return one font entry for each font family
        FXbool addIt = TRUE;
        for(unsigned j = 0; j < realnumfonts; j++)
          if(strcmp(fonts[j].face, desc->face) == 0) {
            addIt = FALSE;
            break;
          }

        if(!addIt)
          continue;
      }

      realnumfonts++;

      // Dump what we have found out
      FXTRACE((150, "Font=%s weight=%d slant=%d size=%3d setwidth=%d encoding=%d flags=%d\n", desc->face, desc->weight, desc->slant, desc->size, desc->setwidth, desc->encoding, desc->flags));
    }

    if(realnumfonts < numfonts) {
      numfonts = realnumfonts;

      if(numfonts > 0)
        FXRESIZE(&fonts, FXFontDesc, numfonts);
      else
        FXFREE(&fonts);
    }

    if(numfonts > 0)
      // Sort them by name, weight, slant, and size respectively
      qsort(fonts, numfonts, sizeof(FXFontDesc), comparefont);
  }

  FcFontSetDestroy(fontset);
  FcObjectSetDestroy(objset);
  FcPatternDestroy(pattern);

  return numfonts > 0;
  }

#else                                   // Using XLFD


// List all fonts matching hints
FXbool FXFont::listFonts(FXFontDesc*& fonts,FXuint& numfonts,const FXString& face,FXuint wt,FXuint sl,FXuint sw,FXuint en,FXuint h){
  FXchar candidate[MAX_XLFD],family[104],foundry[104],fullname[104],*field[14],**fnames;
  FXuint size,weight,slant,encoding,setwidth,flags;
  const FXchar *scalable,*facename,*foundryname;
  FXint screenres,xres,yres;
  FXint numfnames,f,j;

  fonts=NULL;
  numfonts=0;

  // Gotta have display open!
  if(!FXApp::instance()){ fxerror("FXFont::listFonts: no application object.\n"); }
  if(!DISPLAY(FXApp::instance())){ fxerror("FXFont::listFonts: trying to list fonts before opening display.\n"); }

  // Screen resolution may be overidden by registry
  screenres=FXApp::app->reg().readUnsignedEntry("SETTINGS","screenres",100);

  // Validate
  if(screenres<50) screenres=50;
  if(screenres>200) screenres=200;

  FXTRACE((150,"Listing fonts for screenres=%d:\n",screenres));

  // Family and foundry name
  familyandfoundryfromname(family,foundry,face.text());

  FXTRACE((150,"family=\"%s\", foundry=\"%s\"\n",family,foundry));

  // Define pattern to match against
  if(h&FONTHINT_X11){
    facename="*";
    if(family[0]) facename=family;
    strncpy(candidate,facename,MAX_XLFD);
    }

  // Match XLFD fonts; try to limit the number by using
  // some of the info we already have acquired.
  else{
    scalable="*";
    if(h&FONTHINT_SCALABLE) scalable="0";
    facename="*";
    if(family[0]) facename=family;
    foundryname="*";
    if(foundry[0]) foundryname=foundry;
    sprintf(candidate,"-%s-%s-*-*-*-*-%s-%s-*-*-*-%s-*-*",foundryname,facename,scalable,scalable,scalable);
    }

  // Get list of all font names
  fnames=listfontnames(DISPLAY(FXApp::instance()),candidate,numfnames);
  if(!fnames) return FALSE;

  // Make room to receive face names
  FXMALLOC(&fonts,FXFontDesc,numfnames);
  if(!fonts){ XFreeFontNames(fnames); return FALSE; }

  // Add all matching fonts to the list
  for(f=0; f<numfnames; f++){
    strncpy(candidate,fnames[f],MAX_XLFD);

    // XLFD font name; parse out unique face names
    if(parsefontname(field,candidate)){

      flags=0;

      // Get encoding
      encoding=encodingfromxlfd(field[XLFD_REGISTRY],field[XLFD_ENCODING]);

      // Skip if no match
      if((en!=FONTENCODING_DEFAULT) && (en!=encoding)) continue;

      // Get pitch
      flags|=pitchfromtext(field[XLFD_SPACING]);

      // Skip this font if pitch does not match
      if((h&FONTPITCH_FIXED) && !(flags&FONTPITCH_FIXED)) continue;
      if((h&FONTPITCH_VARIABLE) && !(flags&FONTPITCH_VARIABLE)) continue;

      // Skip if weight does not match
      weight=weightfromtext(field[XLFD_WEIGHT]);
      if((wt!=FONTWEIGHT_DONTCARE) && (wt!=weight)) continue;

      // Skip if slant does not match
      slant=slantfromtext(field[XLFD_SLANT]);
      if((sl!=FONTSLANT_DONTCARE) && (sl!=slant)) continue;

      // Skip if setwidth does not match
      setwidth=setwidthfromtext(field[XLFD_SETWIDTH]);
      if((sw!=FONTSETWIDTH_DONTCARE) && (sw!=setwidth)) continue;

      // Scalable
      if(EQUAL1(field[XLFD_PIXELSIZE],'0') && EQUAL1(field[XLFD_POINTSIZE],'0') && EQUAL1(field[XLFD_AVERAGE],'0')){
        flags|=FONTHINT_SCALABLE;
        }

      // Polymorphic
      if(EQUAL1(field[XLFD_WEIGHT],'0') || EQUAL1(field[XLFD_SETWIDTH],'0') || EQUAL1(field[XLFD_SLANT],'0') || EQUAL1(field[XLFD_ADDSTYLE],'0')){
        flags|=FONTHINT_POLYMORPHIC;
        }

      // Get Font resolution
      if(EQUAL1(field[XLFD_RESOLUTION_X],'0') && EQUAL1(field[XLFD_RESOLUTION_Y],'0')){
        xres=screenres;
        yres=screenres;
        }
      else{
        xres=atoi(field[XLFD_RESOLUTION_X]);
        yres=atoi(field[XLFD_RESOLUTION_Y]);
        }

      // Get size, corrected for screen resolution
      if(!(flags&FONTHINT_SCALABLE)){
        size=(yres*atoi(field[XLFD_POINTSIZE]))/screenres;
        }
      else{
        size=0;
        }

      // Dump what we have found out
      FXTRACE((160,"Font=\"%s\" weight=%d slant=%d size=%3d setwidth=%d encoding=%d\n",field[XLFD_FAMILY],weight,slant,size,setwidth,encoding));

      // If NULL face name, just list one of each face
      sprintf(fullname,"%s [%s]",field[XLFD_FAMILY],field[XLFD_FOUNDRY]);
      if(family[0]=='\0'){
        for(j=numfonts-1; j>=0; j--){
          if(strcmp(fullname,fonts[j].face)==0) goto next;
          }
        }

      // Add this font
      strncpy(fonts[numfonts].face,fullname,sizeof(fonts[0].face));
      fonts[numfonts].size=size;
      fonts[numfonts].weight=weight;
      fonts[numfonts].slant=slant;
      fonts[numfonts].encoding=encoding;
      fonts[numfonts].setwidth=setwidth;
      fonts[numfonts].flags=flags;
      numfonts++;

      // Next font
next: continue;
      }

    // X11 font, add it to the list
    strncpy(fonts[numfonts].face,fnames[f],sizeof(fonts[0].face));
    fonts[numfonts].size=0;
    fonts[numfonts].weight=FONTWEIGHT_DONTCARE;
    fonts[numfonts].slant=FONTSLANT_DONTCARE;
    fonts[numfonts].encoding=FONTENCODING_DEFAULT;
    fonts[numfonts].setwidth=FONTSETWIDTH_DONTCARE;
    fonts[numfonts].flags=FONTHINT_X11;
    numfonts++;
    }

  // Any fonts found?
  if(numfonts==0){
    FXFREE(&fonts);
    XFreeFontNames(fnames);
    return FALSE;
    }

  // Realloc to shrink the block
  FXRESIZE(&fonts,FXFontDesc,numfonts);

  // Sort them by name, weight, slant, and size respectively
  qsort(fonts,numfonts,sizeof(FXFontDesc),comparefont);

//   FXTRACE((150,"%d fonts:\n",numfonts));
//   for(f=0; f<numfonts; f++){
//     FXTRACE((150,"Font=\"%s\" weight=%d slant=%d size=%3d setwidth=%d encoding=%d\n",fonts[f].face,fonts[f].weight,fonts[f].slant,fonts[f].size,fonts[f].setwidth,fonts[f].encoding));
//     }
//   FXTRACE((150,"\n\n"));

  // Free the font names
  XFreeFontNames(fnames);
  return TRUE;
  }

#endif


#else


// List all fonts matching hints
FXbool FXFont::listFonts(FXFontDesc*& fonts,FXuint& numfonts,const FXString& face,FXuint wt,FXuint sl,FXuint sw,FXuint en,FXuint h){
  register FXuint i,j;

  // Initialize return values
  fonts=NULL;
  numfonts=0;

  // This data gets passed into the callback function
  FXFontStore fontStore;
  HDC hdc=GetDC(GetDesktopWindow());
  SaveDC(hdc);
  fontStore.hdc=hdc;
  fontStore.fonts=fonts;
  fontStore.numfonts=numfonts;
  fontStore.desc.weight=wt;
  fontStore.desc.slant=sl;
  fontStore.desc.setwidth=sw;
  fontStore.desc.encoding=en;
  fontStore.desc.flags=h;

  // Fill in the appropriate fields of the LOGFONT structure. Note that
  // EnumFontFamiliesEx() only examines the lfCharSet, lfFaceName and
  // lpPitchAndFamily fields of this struct.
  LOGFONT lf;
  lf.lfHeight=0;
  lf.lfWidth=0;
  lf.lfEscapement=0;
  lf.lfOrientation=0;
  lf.lfWeight=0;
  lf.lfItalic=0;
  lf.lfUnderline=0;
  lf.lfStrikeOut=0;
  lf.lfCharSet=FXFontEncoding2CharSet(en);
  lf.lfOutPrecision=0;
  lf.lfClipPrecision=0;
  lf.lfQuality=0;
  lf.lfPitchAndFamily=0;                          // Should be MONO_FONT for Hebrew and Arabic?
  FXASSERT(face.length()<LF_FACESIZE);
  strncpy(lf.lfFaceName,face.text(),LF_FACESIZE);

  // Start enumerating!
  EnumFontFamiliesEx(hdc,&lf,EnumFontFamExProc,(LPARAM)&fontStore,0);
  RestoreDC(hdc,-1);
  ReleaseDC(GetDesktopWindow(),hdc);

  // Copy stuff back from the store
  fonts=fontStore.fonts;
  numfonts=fontStore.numfonts;

  // Any fonts found?
  if(numfonts==0){
    FXFREE(&fonts);
    return FALSE;
    }

  // Sort them by name, weight, slant, and size respectively
  qsort(fonts,numfonts,sizeof(FXFontDesc),comparefont);

  // Weed out duplicates if we were just listing the face names
  if(lf.lfCharSet==DEFAULT_CHARSET && lf.lfFaceName[0]==0){
    i=j=1;
    while(j<numfonts){
      if(strcmp(fonts[i-1].face,fonts[j].face)!=0){
        fonts[i]=fonts[j];
        i++;
        }
      j++;
      }
    numfonts=i;
    }

  // Realloc to shrink the block
  FXRESIZE(&fonts,FXFontDesc,numfonts);

//   FXTRACE((150,"%d fonts:\n",numfonts));
//   for(FXuint f=0; f<numfonts; f++){
//     FXTRACE((150,"Font=%s weight=%d slant=%d size=%3d setwidth=%d encoding=%d\n",fonts[f].face,fonts[f].weight,fonts[f].slant,fonts[f].size,fonts[f].setwidth,fonts[f].encoding));
//     }
//   FXTRACE((150,"\n\n"));

  return TRUE;
  }

#endif


/*******************************************************************************/


// Font style table
static const ENTRY styletable[]={
  {"",FONTHINT_DONTCARE},
  {"decorative",FONTHINT_DECORATIVE},
  {"modern",FONTHINT_MODERN},
  {"roman",FONTHINT_ROMAN},
  {"script",FONTHINT_SCRIPT},
  {"swiss",FONTHINT_SWISS},
  {"system",FONTHINT_SYSTEM}
  };


// Font pitch table
static const ENTRY pitchtable[]={
  {"",FONTPITCH_DEFAULT},
  {"mono",FONTPITCH_FIXED},
  {"fixed",FONTPITCH_FIXED},
  {"constant",FONTPITCH_FIXED},
  {"variable",FONTPITCH_VARIABLE},
  {"proportional",FONTPITCH_VARIABLE},
  {"c",FONTPITCH_FIXED},
  {"m",FONTPITCH_FIXED},
  {"p",FONTPITCH_VARIABLE}
  };


// Font text angles
static const ENTRY slanttable[]={
  {"",FONTSLANT_DONTCARE},
  {"regular",FONTSLANT_REGULAR},
  {"italic",FONTSLANT_ITALIC},
  {"oblique",FONTSLANT_OBLIQUE},
  {"normal",FONTSLANT_REGULAR},
  {"reverse italic",FONTSLANT_REVERSE_ITALIC},
  {"reverse oblique",FONTSLANT_REVERSE_OBLIQUE},
  {"r",FONTSLANT_REGULAR},
  {"n",FONTSLANT_REGULAR},
  {"i",FONTSLANT_ITALIC},
  {"o",FONTSLANT_OBLIQUE},
  {"ri",FONTSLANT_REVERSE_ITALIC},
  {"ro",FONTSLANT_REVERSE_OBLIQUE}
  };


// Character set encodings
static const ENTRY encodingtable[]={
  {"",FONTENCODING_DEFAULT},
  {"iso8859-1",FONTENCODING_ISO_8859_1},
  {"iso8859-2",FONTENCODING_ISO_8859_2},
  {"iso8859-3",FONTENCODING_ISO_8859_3},
  {"iso8859-4",FONTENCODING_ISO_8859_4},
  {"iso8859-5",FONTENCODING_ISO_8859_5},
  {"iso8859-6",FONTENCODING_ISO_8859_6},
  {"iso8859-7",FONTENCODING_ISO_8859_7},
  {"iso8859-8",FONTENCODING_ISO_8859_8},
  {"iso8859-9",FONTENCODING_ISO_8859_9},
  {"iso8859-10",FONTENCODING_ISO_8859_10},
  {"iso8859-11",FONTENCODING_ISO_8859_11},
  {"iso8859-13",FONTENCODING_ISO_8859_13},
  {"iso8859-14",FONTENCODING_ISO_8859_14},
  {"iso8859-15",FONTENCODING_ISO_8859_15},
  {"iso8859-16",FONTENCODING_ISO_8859_16},
  {"koi8",FONTENCODING_KOI8},
  {"koi8-r",FONTENCODING_KOI8_R},
  {"koi8-u",FONTENCODING_KOI8_U},
  {"koi8-unified",FONTENCODING_KOI8_UNIFIED},
  {"cp437",FONTENCODING_CP437},
  {"cp850",FONTENCODING_CP850},
  {"cp851",FONTENCODING_CP851},
  {"cp852",FONTENCODING_CP852},
  {"cp855",FONTENCODING_CP855},
  {"cp856",FONTENCODING_CP856},
  {"cp857",FONTENCODING_CP857},
  {"cp860",FONTENCODING_CP860},
  {"cp861",FONTENCODING_CP861},
  {"cp862",FONTENCODING_CP862},
  {"cp863",FONTENCODING_CP863},
  {"cp864",FONTENCODING_CP864},
  {"cp865",FONTENCODING_CP865},
  {"cp866",FONTENCODING_CP866},
  {"cp869",FONTENCODING_CP869},
  {"cp870",FONTENCODING_CP870},
  {"cp1250",FONTENCODING_CP1250},
  {"cp1251",FONTENCODING_CP1251},
  {"cp1252",FONTENCODING_CP1252},
  {"cp1253",FONTENCODING_CP1253},
  {"cp1254",FONTENCODING_CP1254},
  {"cp1255",FONTENCODING_CP1255},
  {"cp1256",FONTENCODING_CP1256},
  {"cp1257",FONTENCODING_CP1257},
  {"cp1258",FONTENCODING_CP1258},
  {"cp874",FONTENCODING_CP874},
  {"ascii",FONTENCODING_ISO_8859_1}
  };


// Set width table
static const ENTRY setwidthtable[]={
  {"",FONTSETWIDTH_DONTCARE},
  {"ultracondensed",FONTSETWIDTH_ULTRACONDENSED},
  {"extracondensed",FONTSETWIDTH_EXTRACONDENSED},
  {"condensed",FONTSETWIDTH_CONDENSED},
  {"narrow",FONTSETWIDTH_NARROW},
  {"compressed",FONTSETWIDTH_COMPRESSED},
  {"semicondensed",FONTSETWIDTH_SEMICONDENSED},
  {"medium",FONTSETWIDTH_MEDIUM},
  {"normal",FONTSETWIDTH_NORMAL},
  {"regular",FONTSETWIDTH_REGULAR},
  {"semiexpanded",FONTSETWIDTH_SEMIEXPANDED},
  {"expanded",FONTSETWIDTH_EXPANDED},
  {"wide",FONTSETWIDTH_WIDE},
  {"extraexpanded",FONTSETWIDTH_EXTRAEXPANDED},
  {"ultraexpanded",FONTSETWIDTH_ULTRAEXPANDED},
  {"n",FONTSETWIDTH_NARROW},
  {"r",FONTSETWIDTH_REGULAR},
  {"c",FONTSETWIDTH_CONDENSED},
  {"w",FONTSETWIDTH_WIDE},
  {"m",FONTSETWIDTH_MEDIUM},
  {"x",FONTSETWIDTH_EXPANDED}
  };


// Weight table
static const ENTRY weighttable[]={
  {"",FONTWEIGHT_DONTCARE},
  {"thin",FONTWEIGHT_THIN},
  {"extralight",FONTWEIGHT_EXTRALIGHT},
  {"light",FONTWEIGHT_LIGHT},
  {"normal",FONTWEIGHT_NORMAL},
  {"regular",FONTWEIGHT_REGULAR},
  {"medium",FONTWEIGHT_MEDIUM},
  {"demibold",FONTWEIGHT_DEMIBOLD},
  {"bold",FONTWEIGHT_BOLD},
  {"extrabold",FONTWEIGHT_EXTRABOLD},
  {"heavy",FONTWEIGHT_HEAVY},
  {"black",FONTWEIGHT_BLACK},
  {"b",FONTWEIGHT_BOLD},
  {"l",FONTWEIGHT_LIGHT},
  {"n",FONTWEIGHT_NORMAL},
  {"r",FONTWEIGHT_REGULAR},
  {"m",FONTWEIGHT_MEDIUM},
  };


// Search for value and return name
static FXString findbyvalue(const ENTRY* table,FXint n,FXuint value){
  for(int i=0; i<n; i++){ if(table[i].value==value) return table[i].name; }
  return FXStringVal(value);
  }


// Search for name and return value
static FXuint findbyname(const ENTRY* table,FXint n,const FXString& name){
  for(int i=0; i<n; i++){ if(comparecase(table[i].name,name)==0) return table[i].value; }
  return FXUIntVal(name);
  }


// Get font description
void FXFont::getFontDesc(FXFontDesc& fontdesc) const {
  strncpy(fontdesc.face,wantedName.text(),sizeof(fontdesc.face));
  fontdesc.size=wantedSize;
  fontdesc.weight=wantedWeight;
  fontdesc.slant=wantedSlant;
  fontdesc.setwidth=wantedSetwidth;
  fontdesc.encoding=wantedEncoding;
  fontdesc.flags=hints;
  }


// Change font description
void FXFont::setFontDesc(const FXFontDesc& fontdesc){
  wantedName=fontdesc.face;
  wantedSize=fontdesc.size;
  wantedWeight=fontdesc.weight;
  wantedSlant=fontdesc.slant;
  wantedSetwidth=fontdesc.setwidth;
  wantedEncoding=fontdesc.encoding;
  hints=fontdesc.flags;
  }


// Change font description from a string
FXbool FXFont::setFont(const FXString& string){
  FXuint size,weight,slant,setwidth,encoding,hh;
  FXchar face[256];
  FXint len;

  // Initialize
  wantedName=FXString::null;
  wantedSize=0;
  wantedWeight=0;
  wantedSlant=0;
  wantedSetwidth=0;
  wantedEncoding=0;
  hints=0;

  // Non-empty name
  if(!string.empty()){

    // Try to match old format
    if(string.scan("[%[^]]] %u %u %u %u %u %u",face,&size,&weight,&slant,&encoding,&setwidth,&hh)==7){
      wantedName=face;
      wantedSize=size;
      wantedWeight=weight;
      wantedSlant=slant;
      wantedSetwidth=setwidth;
      wantedEncoding=encoding;
      hints=hh;
      return TRUE;
      }

    // Check for X11 font
    len=string.find(',');
    if(len<0){
      wantedName=string;
      hints|=FONTHINT_X11;
      return TRUE;
      }

    // Normal font description
    wantedName=string.left(len);
    wantedSize=FXUIntVal(string.section(',',1));
    wantedWeight=findbyname(weighttable,ARRAYNUMBER(weighttable),string.section(',',2));
    wantedSlant=findbyname(slanttable,ARRAYNUMBER(slanttable),string.section(',',3));
    wantedSetwidth=findbyname(setwidthtable,ARRAYNUMBER(setwidthtable),string.section(',',4));
    wantedEncoding=findbyname(encodingtable,ARRAYNUMBER(encodingtable),string.section(',',5));
    hints=FXUIntVal(string.section(',',6));

    return TRUE;
    }
  return FALSE;
  }



// Return the font description as a string; keep it as simple
// as possible by dropping defaulted fields at the end.
FXString FXFont::getFont() const {
  FXString string=wantedName;

  // Raw X11 is only the name
  if(!(hints&FONTHINT_X11)){

    // Append size
    string.append(',');
    string.append(FXStringVal(wantedSize));

    // Weight and other stuff
    if(wantedWeight || wantedSlant || wantedSetwidth || wantedEncoding || hints){

      // Append weight
      string.append(',');
      string.append(findbyvalue(weighttable,ARRAYNUMBER(weighttable),wantedWeight));

      // Slant and other stuff
      if(wantedSlant || wantedSetwidth || wantedEncoding || hints){

        // Append slant
        string.append(',');
        string.append(findbyvalue(slanttable,ARRAYNUMBER(slanttable),wantedSlant));

        // Setwidth and other stuff
        if(wantedSetwidth || wantedEncoding || hints){

          // Append set width
          string.append(',');
          string.append(findbyvalue(setwidthtable,ARRAYNUMBER(setwidthtable),wantedSetwidth));

          // Encoding and other stuff
          if(wantedEncoding || hints){

            // Append encoding
            string.append(',');
            string.append(findbyvalue(encodingtable,ARRAYNUMBER(encodingtable),wantedEncoding));

            // Hints
            if(hints){

              // Append hint flags
              string.append(',');
              string.append(FXStringVal(hints));
              }
            }
          }
        }
      }
    }
  return string;
  }


// Thanks to Yakubenko Maxim <max@tiki.sio.rssi.ru> the patch to the
// functions below; the problem was that spaces may occur in the font
// name (e.g. Courier New).  The scanset method will simply parse the
// string, including any spaces, until the matching bracket.

// Parse font description DEPRECATED
FXbool fxparsefontdesc(FXFontDesc& fontdesc,const FXchar* string){
  return string && (sscanf(string,"[%[^]]] %u %u %u %u %u %u",fontdesc.face,&fontdesc.size,&fontdesc.weight,&fontdesc.slant,&fontdesc.encoding,&fontdesc.setwidth,&fontdesc.flags)==7);
  }


// Unparse font description DEPRECATED
FXbool fxunparsefontdesc(FXchar *string,const FXFontDesc& fontdesc){
  sprintf(string,"[%s] %u %u %u %u %u %u",fontdesc.face,fontdesc.size,fontdesc.weight,fontdesc.slant,fontdesc.encoding,fontdesc.setwidth,fontdesc.flags);
  return TRUE;
  }


/*******************************************************************************/


// Save font to stream
void FXFont::save(FXStream& store) const {
  FXId::save(store);
  store << wantedName;
  store << wantedSize;
  store << wantedWeight;
  store << wantedSlant;
  store << wantedSetwidth;
  store << wantedEncoding;
  store << hints;
  }


// Load font from stream; create() should be called later
void FXFont::load(FXStream& store){
  FXId::load(store);
  store >> wantedName;
  store >> wantedSize;
  store >> wantedWeight;
  store >> wantedSlant;
  store >> wantedSetwidth;
  store >> wantedEncoding;
  store >> hints;
  }


// Clean up
FXFont::~FXFont(){
  FXTRACE((100,"FXFont::~FXFont %p\n",this));
  destroy();
  }

}
