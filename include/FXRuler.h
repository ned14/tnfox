/********************************************************************************
*                                                                               *
*                            R u l e r   W i d g e t                            *
*                                                                               *
*********************************************************************************
* Copyright (C) 2002,2004 by Jeroen van der Zijp.   All Rights Reserved.        *
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
* $Id: FXRuler.h,v 1.19 2004/02/08 17:17:34 fox Exp $                           *
********************************************************************************/
#ifndef FXRULER_H
#define FXRULER_H

#ifndef FXFRAME_H
#include "FXFrame.h"
#endif

namespace FX {


/// Ruler options
enum {
  RULER_NORMAL       = 0,                                   /// Default appearance (default)
  RULER_HORIZONTAL   = 0,                                   /// Ruler is horizontal (default)
  RULER_VERTICAL     = 0x00008000,                          /// Ruler is vertical
  RULER_TICKS_OFF    = 0,                                   /// Tick marks off (default)
  RULER_TICKS_TOP    = 0x00010000,                          /// Ticks on the top (if horizontal)
  RULER_TICKS_BOTTOM = 0x00020000,                          /// Ticks on the bottom (if horizontal)
  RULER_TICKS_LEFT   = RULER_TICKS_TOP,                     /// Ticks on the left (if vertical)
  RULER_TICKS_RIGHT  = RULER_TICKS_BOTTOM,                  /// Ticks on the right (if vertical)
  RULER_TICKS_CENTER = RULER_TICKS_TOP|RULER_TICKS_BOTTOM,  /// Tickmarks centered
  RULER_NUMBERS      = 0x00040000,                          /// Show numbers
  RULER_ARROW        = 0x00080000,                          /// Draw small arrow for cursor position
  RULER_MARKERS      = 0x00100000,                          /// Draw markers for indentation settings
  RULER_METRIC       = 0,                                   /// Metric subdivision (default)
  RULER_ENGLISH      = 0x00200000                           /// English subdivision
  };


class FXFont;


/**
* The Ruler widget is placed alongside a document to measure position
* and size of entities within the document.
*/
class FXAPI FXRuler : public FXFrame {
  FXDECLARE(FXRuler)
protected:
  FXFont  *font;                // Font for numbers
  FXColor  textColor;           // Color for numbers and ticks
  FXint    lowerMargin;         // Lower margin
  FXint    upperMargin;         // Upper margin
  FXint    firstPara;           // First line paragraph indent
  FXint    lowerPara;           // Lower paragraph indent
  FXint    upperPara;           // Upper paragraph indent
  FXint    docSpace;            // Empty space around document
  FXint    docSize;             // Size of document
  FXint    contentSize;         // Size of content
  FXint    arrowPos;            // Arrow position
  FXdouble pixelPerTick;        // Number of pixels between ticks
  FXint    majorTicks;          // Interval between major ticks
  FXint    mediumTicks;         // Interval between medium ticks
  FXint    tinyTicks;           // Interval between tiny ticks
  FXint    shift;               // Shift amount
  FXString tip;                 // Tooltip text
  FXString help;                // Help text
protected:
  FXRuler();
  void drawLeftArrow(FXDCWindow& dc,FXint x,FXint y);
  void drawRightArrow(FXDCWindow& dc,FXint x,FXint y);
  void drawUpArrow(FXDCWindow& dc,FXint x,FXint y);
  void drawDownArrow(FXDCWindow& dc,FXint x,FXint y);
  void drawLeftMarker(FXDCWindow& dc,FXint x,FXint y);
  void drawRightMarker(FXDCWindow& dc,FXint x,FXint y);
  void drawUpMarker(FXDCWindow& dc,FXint x,FXint y);
  void drawDownMarker(FXDCWindow& dc,FXint x,FXint y);
//  void drawHzTicks(FXDCWindow& dc
private:
  FXRuler(const FXRuler&);
  FXRuler &operator=(const FXRuler&);
public:
  long onPaint(FXObject*,FXSelector,void*);
  long onLeftBtnPress(FXObject*,FXSelector,void*);
  long onLeftBtnRelease(FXObject*,FXSelector,void*);
  long onMotion(FXObject*,FXSelector,void*);
  long onCmdSetValue(FXObject*,FXSelector,void*);
  long onCmdSetIntValue(FXObject*,FXSelector,void*);
  long onCmdGetIntValue(FXObject*,FXSelector,void*);
  long onCmdSetHelp(FXObject*,FXSelector,void*);
  long onCmdGetHelp(FXObject*,FXSelector,void*);
  long onCmdSetTip(FXObject*,FXSelector,void*);
  long onCmdGetTip(FXObject*,FXSelector,void*);
  long onQueryHelp(FXObject*,FXSelector,void*);
  long onQueryTip(FXObject*,FXSelector,void*);
public:
  enum {
    ID_ARROW=FXFrame::ID_LAST,
    ID_LAST
    };
public:

  /// Construct label with given text and icon
  FXRuler(FXComposite* p,FXObject* tgt=NULL,FXSelector sel=0,FXuint opts=RULER_NORMAL,FXint x=0,FXint y=0,FXint w=0,FXint h=0,FXint pl=DEFAULT_PAD,FXint pr=DEFAULT_PAD,FXint pt=DEFAULT_PAD,FXint pb=DEFAULT_PAD);

  /// Create server-side resources
  virtual void create();

  /// Detach server-side resources
  virtual void detach();

  /// Return default width
  virtual FXint getDefaultWidth();

  /// Return default height
  virtual FXint getDefaultHeight();

  /// Set the text font
  void setFont(FXFont *fnt);

  /// Get the text font
  FXFont* getFont() const { return font; }

  /// Change slider value
  void setValue(FXint value);

  /// Return slider value
  FXint getValue() const { return arrowPos; }

  /// Set ruler style
  void setRulerStyle(FXuint style);

  /// Get ruler style
  FXuint getRulerStyle() const;

  /// Get the current text color
  FXColor getTextColor() const { return textColor; }

  /// Set the current text color
  void setTextColor(FXColor clr);

  /// Set the status line help text for this label
  void setHelpText(const FXString& text);

  /// Get the status line help text for this label
  FXString getHelpText() const { return help; }

  /// Set the tool tip message for this label
  void setTipText(const FXString&  text);

  /// Get the tool tip message for this label
  FXString getTipText() const { return tip; }

  /// Save label to a stream
  virtual void save(FXStream& store) const;

  /// Load label from a stream
  virtual void load(FXStream& store);

  /// Destructor
  virtual ~FXRuler();
  };

}

#endif

