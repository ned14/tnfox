/* THourglass.h
Much enhanced wait cursor
(C) 2002, 2004 Niall Douglas
Original version created: Nov 2002
This version created: 4th Feb 2004
*/

/* This code is licensed for use only in testing TnFOX facilities.
It may not be used by other code or incorporated into code without
prior written permission */

#ifndef _THourglass_h_
#define _THourglass_h_

#include "tmaster.h"

/*! \file THourglass.h
\brief Defines classes related to the Tn wait cursor
*/
namespace FX { class FXWindow; }
namespace Tn {

/*! \class THourglass
\brief An enhanced wait cursor

Mostly Tn is an amalgalm of various ideas from different operating systems,
but I think this one purely derives from Acorn RISC-OS - the idea of an indicative
wait cursor. Despite the technology on all plaforms being capable of this, I have
not seen once this idea implemented outside RISC-OS despite its neatness.

The cursor used is a custom designed hourglass, but it is run-time modified to
add a percentage between 0..100%. The class is a static class, so usage is simple:
\code
THourglass::on(widget);
...
THourglass::setPercent(widget, x);
...
THourglass::off(widget);
\endcode
You can even forego the on() as the first setPercent() will call it for you
if necessary. Like the old RISC-OS hourglass, it waits a third of a second before
displaying it so it doesn't appear for short operations.
The code does not recalculate the cursor except when the percentage changes,
so you can call it as often as you wish. Furthermore, it is safe for use by
multiple windows eg; a different percentage can be shown in three windows all
doing seperate lengthy tasks individually.

The wait cursor also provides two LED's of arbitrary colour. Tn itself uses
a green one for data receipt and red for data send and we recommend that you
do the same when appropriate. Of course feel free to use other colours for
anything else you think useful for the user.

<h3>Implementation:</h3>
Due to no (legal) way on Windows to set dynamically created animated cursors,
we cheat and animate it from the process thread pool. This raises the spectre
of thread-safety and we in fact rely on FOX's routines for this being written
as they are to ensure it.

\sa Tn::THourglassHold
*/
class TEXPORT_TCLIENT THourglass
{
	//! This is a purely static class so it cannot be instantiated
	THourglass();
public:
	//! Returns the cursor count for widget \em w (when greater than zero, the cursor is showing)
	static u32 count(FXWindow *w);
	/*! Increments the cursor count for widget \em w (when greater than zero,
	the cursor is showing). Displays after period \em wait */
	static void on(FXWindow *w, u32 wait=333);
	//! Decrements the cursor count for widget \em w (when greater than zero, the cursor is showing)
	static void off(FXWindow *w);
	//! Turns off the wait cursor for widget \em w no matter what
	static void smash(FXWindow *w);
	//! Returns the percentage count for widget \em w with -1 being none
	static int percent(FXWindow *w);
	//! Sets the percentage count for widget \em w. Use -1 to turn it off.
	static void setPercent(FXWindow *w, int percent=-1);
	//! Sets LED no \em no to color \em colour. Use (FXColor)-1 to turn off.
	static void setLED(FXWindow *w, u32 no, FXColor colour=(FXColor)-1);

	//! Sets Tn "reading" LED
	static void setReadingLED(FXWindow *w, bool v) { setLED(w, 0, v ? FXRGBA(0,255,0,255) : (FXColor)-1); }
	//! Sets Tn "writing" LED
	static void setWritingLED(FXWindow *w, bool v) { setLED(w, 1, v ? FXRGBA(255,0,0,255) : (FXColor)-1); }
};

/*! \class THourglassHold
\brief Holds an opening of the hourglass

For all the usual exception related reasons, this class holds a Tn::THourglass
open during its life
*/
class THourglassHold
{
	FXWindow *w;
public:
	//! Constructs an instance turning the wait cursor on for window \em w
	THourglassHold(FXWindow *_w) : w(_w) { THourglass::on(w); }
	~THourglassHold() { THourglass::off(w); }
	//! Sets the percentage count. Use -1 to turn it off.
	void setPercent(int percent=-1) { THourglass::setPercent(w, percent); }
	//! Sets LED no \em no to color \em colour. Use (FXColor)-1 to turn off.
	void setLED(u32 no, FXColor colour=(FXColor)-1) { THourglass::setLED(w, no, colour); }

	//! Sets Tn "reading" LED
	void setReadingLED(bool v) { THourglass::setReadingLED(w, v); }
	//! Sets Tn "writing" LED
	void setWritingLED(bool v) { THourglass::setWritingLED(w, v); }
};

} // namespace

#endif
