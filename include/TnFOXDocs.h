/********************************************************************************
*                                                                               *
*                          Differences documentation                            *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2002-2009 by Niall Douglas.   All Rights Reserved.       *
*       NOTE THAT I DO NOT PERMIT ANY OF MY CODE TO BE PROMOTED TO THE GPL      *
*********************************************************************************
* This code is free software; you can redistribute it and/or modify it under    *
* the terms of the GNU Library General Public License v2.1 as published by the  *
* Free Software Foundation EXCEPT that clause 3 does not apply ie; you may not  *
* "upgrade" this code to the GPL without my prior written permission.           *
* Please consult the file "License_Addendum2.txt" accompanying this file.       *
*                                                                               *
* This code is distributed in the hope that it will be useful,                  *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                          *
*********************************************************************************
* $Id:                                                                          *
********************************************************************************/

/*! \file TnFOXDocs.h
\brief A null header file containing ancillory TnFOX documentation only
*/

/*! \mainpage TnFOX documentation
Welcome to TnFOX! TnFOX is an extension of the FOX portable GUI toolkit and it provides enhanced facilities
and features mainly for the upcoming Tn library. It is greatly hoped that others will find use of TnFOX
outside its remit within Tn.

TnFOX could not exist without the efforts of those who wrote FOX, and my thanks go especially
to Jeroen van der Zijp who owns copyright of the lion's share of the code. You can find the original
FOX toolkit at http://www.fox-toolkit.org/ plus the full list of authors in the AUTHORS file. You
can find \link acknowledgements
TnFOX-specific acknowledgements here
\endlink

TnFOX absorbs the latest improvements to the core FOX library on a regular basis (this version is derived
from v1.6.34), and the extensions listed below are designed to not interfere with that process where
possible (hence some functionality has not been folded into FOX where it otherwise would). All extension code
is (C) 2001-2009 Niall Douglas and all code rests under the same licence as FOX but with
one extra restriction - <b>I do not permit any code copyrighted to me to be "promoted" to the GPL</b>
so therefore <b>section 3 of the LGPL does not apply to code (C) Niall Douglas</b>. If you want to know more
about the licensing implications of the FOX licence, see Jeroen's useful comments about the matter in the
\c LICENCE-ADDENDUM.txt file and you can find out more about mine in the \c LICENCE-ADDENDUM2.txt

To get you started, here are a list of common start points:
<ol>

<li> <a class="el" target="_self" href="classes.html">All classes in TnFOX</a> and <a class="el" target="_self" href="modules.html">Non-class entities</a>. By category:
<table border="0">
 <tr>
  <td> \ref guiclasses
  <td> \ref layoutclasses
  <td> \ref resourceclasses
  <td> \ref operationalclasses
 <tr>
  <td> \ref convenclasses
  <td> \ref containclasses
  <td> \ref QTL
  <td> \ref openglclasses
 <tr>
  <td>
  <td> \ref fiodevices
  <td> \ref siodevices
  <td>
</table>

<li> <a class="el" target="_self" href="files.html">A list of files in TnFOX</a>. The ones with
descriptions are entirely written by me.
<li> <a class="el" target="_self" href="functions.html">All class methods individually in TnFOX</a> (long) and <a class="el" target="_self" href="globals.html">All non-class items individually</a> (long)
<li> <a class="el" target="_self" href="deprecated.html">A list of all deprecated facilities</a>
<li> <a target="_blank" href="http://www.nedprod.com/TnFOX/">Go to the TnFOX homepage [external]</a>
</ol>

Extended documentation:
\li \ref breakingchanges16
\li \link FAQ
Frequently Asked Questions about TnFOX
\endlink
\li \link python
TnFOX's Python language bindings
\endlink
\li \link CppMunge
How to use TnFOX's tool CppMunge.py
\endlink
\li \link transfiletutorial
How to provide human-language translations for TnFOX applications
\endlink
\li \link rollbacks
Facilities for writing fully exception-aware code
\endlink
\li \link security
Facilities for writing secure code
\endlink
\li \link qtdiffs
Structural differences between TnFOX and Qt
\endlink
\li \link foxdiffs
Porting from FOX to TnFOX
\endlink
\li \link generic
Generic programming tools provided by TnFOX
\endlink
\li \ref debugging
\li \ref modularbuilding
\li \ref cpp0xsupport

\li \ref applemacosnotes
\li \ref windowsnotes
\li \ref unixnotes

All extensions to FOX are fully documented under doxygen (http://www.doxygen.net/). You will
find a test framework of the extensions (which also serve as code examples) in the TestSuite
directory.

\section whonotuse Who should NOT use TnFOX?
\li TnFOX is a \b very modern C++ library. When I say "modern", I mean that it uses many of the very most
cutting edge techniques in C++ known and this does make a lot of programmers uncomfortable. Probably
the hardest thing for most typical programmers to adjust to is writing fully exception aware code
and unfortunately due to how extensively TnFOX uses exceptions, there isn't any choice in the matter.
Summary: If you don't like exceptions, don't use TnFOX.

\li Ancillory to the above, TnFOX needs a modern compiler. While written almost entirely in ISO C++,
many compilers don't fully support all the constructs TnFOX uses - hence TnFOX supports two primary compilers:
(i) MSVC v7.1 or later on Windows and (ii) GCC v3.2 or later on POSIX (note that other compilers do work and
have build system support eg; Intel's). If you don't use those compilers
on those platforms, expect to do some minor porting work (noting that I will never support using GCC for
building Windows binaries until it becomes properly compatible with the PE binary format - hence, there is
no mingw or cygwin support).

\li Also ancillory to the above, TnFOX is not as "lightweight" as FOX - but then TnFOX is a solution
framework (primarily it's Tn's portability layer) whereas FOX still remains mostly a GUI toolkit. All
the metaprogramming adds a fair bit of extra bulk to generated binaries. As of v0.87, TnFOX can be built
with optional components left out - see \ref modularbuilding

\section whouse Who should use TnFOX?
\li Heavy multithreaded applications. TnFOX's multithreading support is second to none and is heavily
optimised for speed, probably more so than any other C++ toolkit out there.

\li Mission-critical applications. TnFOX works around or patches unhelpful features of C++ to ensure
that no matter what goes wrong in your application, your code can respond (eg; deleting all secret data).
Using TnFOX's facilities, your C++ application can be as robust as any C++ application can be as all
host OS errors are detected and thrown as an exception.

\li Very secure applications. There is strong encryption support, Secure Sockets Layer (SSL), data
shredding and full host OS security support to Access Control List (ACL) level.

\li Anyone interested in compile-time programming (metaprogramming) the C++ compiler will find good
facilities in TnFOX. Unlike the Boost library (http://www.boost.org/), we don't have to work around
compiler limitations which means we can greatly simplify implementation and avoid most usage of macros.

\li Anyone wishing to have one or more processes communicate with one another, including over a
network will find TnFOX's lightweight inter-process communication system extremely interesting as it
happily outperforms CORBA and is very flexible and easy to use whilst remaining endian-safe.

\li Anyone interested in performance. Both device i/o and SQL Database support are far faster than their
comprehensive API's would suggest and the whole of TnFOX is regularly profiled and tuned after every
major addition of new code. A lot of the code uses dynamically adjusted O(1) hash containers for
excellent scaling and compile-time optimised sort algorithms. Full & portable access is provided
to hardware-optimised vector facilities on the host computer.


\section diffs Detailed list of enhancements
<ol>
<li><b>Qt-like automated human language translation</b><br>
When combined with FX::QTrans, human-language translation is now easy as pie! Simply wrap all of your
user-visible string literals with <tt>tr()</tt>, extract into a text file using the provided
utility CppMunge.py and add your translations. You can then choose to embed the file, or place it next
to the executable. You can even place translations within DLL's and arbitrarily load and unload them!

<li><b>FX::FXStream now works with a FX::QIODevice & 64 bit file i/o is now used throughout</b><br>
A FXfval now holds a file pointer or size, ready for the transition to 128 bit disc i/o around
2025. The i/o structure is now based around a Qt-compatible QIODevice which is much much
more flexible than the old FOX structure and for backwards compatibility
FOX's FX::FXFileStream, FX::FXMemoryStream & FX::FXGZStream have been rewritten to use
FX::QFile + FX::FXStream, FX::QBuffer + FX::FXStream & FX::QGZipDevice + FX::FXStream
combinations respectively (note that their use in new code is deprecated).

<li><b>Enhanced i/o facilities</b><br>
FX::QPipe provides a feature-rich portable named pipe across all platforms and 
FX::QLocalPipe provides an intra-process pipe. FX::QBlkSocket and
FX::QHostAddress provide Qt-compatible network access including full IPv4 and IPv6 support.
FX::QGZipDevice provides a transparent gzip format compressor and decompressor which uses LZW
compression to substantially decrease data size. FX::QBZip2Device provides a transparent bzip2
format compressor and decompressor which decreases data size even further.

<li><b>Improved Time facilities</b><br>
FX::FXTime provides an enhanced portable, microsecond granularity date & time system from the
1st January year 0 until the 20th February, year 584,556 with knowledge of local time zones.
This support extends throughout the file system support on operating systems with better than
one second granularity (Windows, FreeBSD). FX::FXProcess provides portable millisecond and
nanosecond counters for extremely accurate timing.

<li><b>Improved User Interface facilities</b><br>
TnFOX understands the user's handedness (reported via FX::FXProcess::userHandedness()) as
do TnFOX's GUI extension classes. FX::FXPrimaryButton is a primary button in a dialog
which orientates itself towards the user's handedness and bottom plus dispatches TnFOX's
apply/reset protocol which lets compliant widgets apply themselves to their data or
restore the correct representation. FX::FXHandedInterfaceI implements an interface with a
handed primary button well and further automates the apply/reset protocol. Small screens
are better supported with dynamic layout spacing via FX::FXWindow::defaultPadding() and
FX::FXWindow::defaultSpacing() plus extra command line arguments such as \c -fxscreenscale
and \c -fxscreensize. There are a number of convenient implementations of handed interfaces
such as FX::FXExceptionDialog, FX::FXHandedDialog, FX::FXHandedPopup and FX::FXHandedMsgBox.

<li><b>Full strength encryption facilities</b><br>
FX::QSSLDevice is built upon the excellent OpenSSL library and provides integrated full strength
encryption facilities which can work with any of the other i/o devices in TnFOX. Both
communications and file data may be encrypted with any supported cipher and key length.
File data can be encrypted by AES or Blowfish symmetric encryption and also by RSA
public key asymmetric encryption. FX::FXSSLKey is a container for symmetric keys and
FX::FXSSLPKey is a container for asymmetric keys and both can be loaded and saved plus
compatibility facilities with other programs through X509/PEM file support. Furthermore, TnFOX provides a
cross-platform entropy gatherer (FX::Secure::Randomness) which ensures up to 8192 bits of
randomness is available plus a number of cryptographically useful routines such
as FX::QIODevice::shredData().

<li><b>Superior provision of host OS facilities portably</b><br>
FX::FXACL and FX::FXACLEntity provide a portable method of setting detailed access
control list based security on those platforms which support it and a reasonable
emulation for everything else. FX::QDir and FX::QFileInfo are mostly Qt compatible classes
which permit easy enumeration and traversal of directories as well as obtaining detailed
information about files. FX::FXFSMonitor permits portable monitoring of directories
for changes. FX::FXProcess contains a wide variety of miscellaneous useful functions,
and FX::FXWinShellLink + FX::FXWinJunctionPoint provide support for all possible
forms of file shortcuts available on Windows (with full POSIX emulation!).

A self-optimising metaprogramming implementation of hardware-assisted math operations
is provided in the FX::Maths namespace. These make full use of any parallel mathematics
hardware.

<li><b>QList, QPtrList, QPtrListIterator, QValueList, QDict, QDictIterator, QIntDict, QIntDictIterator,
QPtrDict, QPtrDictIterator, QPtrVector, QPtrVectorIterator, QMemArray, QByteArray, QStringList,
QFileInfoList</b><br>
All implemented as thunks to the STL. I've personally continued to use these in new code
as the pointer holding classes have auto-deletion, something I find very useful. There
is also the Qt-compatible totally generic Least Recently Used (LRU) cache class
FX::FXLRUCache with specialisations for FX::QCache and FX::QIntCache.

<li><b>Ordered static initialisation</b><br>
Make static data initialisation order problems disappear! With FX::FXProcess' facilities
for declaring dependencies between static data, you always guarantee objects will exist
when you need them! FX::FXProcess can be initialised without FX::FXApp, so if you write a
command-line based program using TnFOX then you can make use of all its facilities
without invoking GUI code
\note You must always initialise a FXProcess first thing in your \c main() - failure to
do this causes many of the extensions to fail

<li><b>Exception Handling</b><br>
With support for nested exceptions, automatic error code extraction, automatic failed operation
retries, extra debug information and a host of useful error checking & input validation macros,
FX::FXException provides facilities for superior robustness and stress-testing. FX::FXExceptionDialog
provides a useful standard window with which to report an error to the user. Writing fully
exception aware code is made much easier with classes such as FX::FXPtrHold and \link rollbacks
transaction rollbacks
\endlink
. Memory leak detection is made much easier on Microsoft compilers with FXMemDbg.h, integrated
support for MSVC's CRT debug library's memory leak checking.

<li><b>Multithreading</b><br>
FX::FXAtomicInt, FX::QWaitCondition, FX::QMutex, FX::QShrdMemMutex, FX::QRWMutex, FX::QMtxHold and
FX::FXZeroedWait all are written for the optimum in efficiency and features.
FX::QThreadLocalStorage provides a portable and C++ friendly interface to host OS TLS
facilities. Even on platforms without POSIX threads support, they can
be compiled into placeholder functionality so that the rest of the library remains useful.
FX::QThread provides a wealth of features, including cleanup handlers, self-destruction, names,
parent-child relationships, portable priority, exit codes and per-thread atexit() handlers.
FX::QThreadPool provides pools of worker threads capable of accepting jobs (FX::Generic::BoundFunctor)
whether for immediate execution or for delayed (timed) execution.

<li><b>Generic programming support</b><br>
In case you didn't already know, the C++ compiler can be programmed to write code for you
(which obviously is useful). TnFOX in a number of places uses policies (which let you
configure the code generation) and traits (where TnFOX configures itself). Furthermore
facilities are provided for extremely generic code writing such as typelists and compile-time
introspection of code. While TnFOX
doesn't go as far as <a href="http://www.boost.org/">Boost</a> or
<a href="http://sourceforge.net/projects/loki-lib/">Loki</a>, it does provide the most
useful compile-time programming support tools and has borrowed ideas from both. For more
information, see \link generic
the page about generic tools
\endlink

<li><b>Inter Process Communication (IPC)</b><br>
Inter-process communications become easy with TnFOX's powerful but lightweight IPC framework
which combines with arbitrary i/o device's which provide socket, named pipe and encrypted
data transport. Automatic endian conversion, optional compression and modular extensibility
also feature. The core of TnFOX's IPC is FX::FXIPCMsgRegistry which provides a messaging
namespace of FX::FXIPCMsg subclasses, ensuring that old and new binaries work together
seamlessly. FX::FXIPCChannel is a base class implementing almost everything you need to
get started immediately. For more information, see \link IPC
the page about Inter Process Communication
\endlink
For running traditional command line programs such as compilers whilst redirecting standard
input and output, FX::QChildProcess is provided. If you would like to use TnFOX to write
web services then FX::FXFCGI provides a FastCGI interface.

<li><b>\link sqldb
SQL Database support
\endlink
</b><br>
FX::TnFXSQLDB is the primary class for accessing SQL databases but unlike other implementations,
it has transparent BLOB support for storing & retrieving arbitrary C++ object instances
via metaprogramming. It is easy to write your own driver, and prewritten drivers come for
SQLite3 (of which there is a customised copy built into TnFOX) and using an IPC connection
to remotely work with another database via FX::TnFXSQLDBServer.

<li><b>Threads can now run their own event loops</b><br>
Through a small implementational change to FX::FXApp, threads can now run their own
event loops permitting per-thread window trees to be run (a window tree is any
hierarchy of parent & child windows which has no relation to any other hierarchy
ie; hierarchies may not span threads) if that application created a FX::TnFXApp as its
primary application object. This is particularly useful for reporting
errors by directly opening a dialog box instead of having to request the GUI thread
to do it for you and can substantially decrease the complexity of worker threads
performing simple interactions with the user.

<li><b>Custom memory pools</b><br>
Through FX::FXMemoryPool, TnFOX allows your code to create as many separate memory
pools as your heart desires! Based upon dlmalloc, one of the fastest and least
fragmentary general purpose dynamic memory allocators known, you can realise all the
benefits of custom memory pools with ease: additional speed, security and robustness.
On Win32, the core process allocator is replaced with nedmalloc (a dlmalloc derivative)
yielding a typical \b 6x speed improvement for heavily multithreaded code!
</ol>

\section todo To do list:
See the file <a target="_blank" href="../Todo.txt">Todo.txt</a>. <a href="tornado_wm.png"></a>

*/



/*! \page breakingchanges16 Breaking changes between v1.4.x and v1.6.x based versions of TnFOX

v0.86 of TnFOX comes in two flavours - one based on v1.4.x of FOX and the other on
v1.6.x of FOX. This is because unicode support is not complete in v1.6.x of FOX
and if you support Latin1 character input, only v1.4.x will currently be of use.

TnFOX remains almost identical between the two builds. The most important difference in the
v1.6.x version is that unicode support is enabled, whereas in the v1.4.x version
that functionality is disabled. However, some API's have changed:
\li FX::FXFile is called FX::QFile in v1.6.x and later builds. FX::FXFile is a new
FOX class in v1.6.x and later.
\li No UTF conversion is implemented. Setting to either FX::QIODevice::NoTranslation
or FX::QIODevice::UTF8 are the only supported options (and neither do anything).
*/



/*! \page FAQ Frequently Asked Questions

General questions:
<ol>
<li>\ref Tn
<li>\ref BuildingNoFOXCompatDiffs
<li>\ref BuildingTnDiffs
<li>\ref BuildingNoGUIDiffs
<li>\ref LotsOfBloat
</ol>

Build questions:
<ol>
<li>\ref msvcbuildnotfound
</ol>

Behaviour questions:
<ol>
<li>\ref strangedefaultpermissions
<li>\ref unabletolockmemorypages
</ol>

FreeBSD questions:
<ol>
<li>\ref disablingFXFSMonitor
<li>\ref spinlockcallederror
</ol>

\section generalqs General Questions:
<ol>
  <li>
	\subsection Tn What is this "Tn" I keep seeing mentioned everywhere?

	TnFOX was forked from FOX to provide the portability library for Tn in May 2003. It was and
	is greatly hoped that TnFOX would find remit outside this core function.

	As can be guessed by TnFOX's description - <i>"TnFOX is a modern secure, robust,
	multithreaded, exception aware, internationalisable, portable GUI toolkit library
	designed for mission-critical work in C++ and Python"</i> - Tn is also all those
	things, however it is also far, far more.

	Tn came from the "Tornado" project, a name I had to drop thanks to Wind River Systems.
	I was most sorry to do so as it is a very apposite name - it engenders "revolution",
	"speed", "dynamism", "unpredictability" and it also means exactly the same thing in
	Spanish. All these things Tn will be.

	I've still not answered the question, have I? Okay - Tn is a total rethink from the
	ground-up of how software should be structured. Inimical to this is a total rethink
	of how humans and computers should interface, as well as how computers should interface
	with each other. At an implementation level, Tn aims to be as good a quality of software
	as I know how to make it - secure, robust, scalable, internationalisable and portable.

	<dl>
	<dt><b>To summarise:</b></dt>
	<dd>The fundamental focus of the whole Tn project is to make your computing experience,
	whether it be using or programming, as productive as possible - both now and well into
	the future.</dd>
	</dl>

	You can find out more about Tn at http://www.nedprod.com/Tn/

  <li>
	\subsection BuildingNoFOXCompatDiffs What are the differences in a no-FOX compatibility build?

	Many of the more recent features added to FOX already have superior implementations in
	TnFOX and for native TnFOX code, it makes no sense to include the thunking code implementing
	the FOX feature. Disabling the FOX compatibility layer removes that code, reducing the
	size of your binary. It is automatically disabled when \c BUILDING_TCOMMON is defined.

	Not compiled:
	\li FX::fxDllOpen etc.
	\li FX::FXBZStream
	\li FX::FXGZStream

  <li>
	\subsection BuildingNoGUIDiffs What are the differences in a no-GUI build?

	As of v0.86, a no-GUI build can be set by editing \c config.py and setting the
	\c disableGUI variable to \c True. The no GUI build consists merely of the TnFOX
	extensions to FOX and virtually no FOX code at all. Here's what you do get:

	\li All the FX::QIODevice classes
	\li All the QTL thunk classes and all classes starting with 'Q'
	\li All the FX::TnFXSQLDB classes
	\li All the stuff in FX::Secure
	\li FX::FXACL, FX::FXFSMonitor, FX::FXRollback, FX::FXProcess, FX::FXNetwork
	\li And from FOX, FX::FXDir, FX::QFile, FX::fxfilematch(), FX::FXIO, FX::FXPath,
	FX::FXStat, FX::FXSystem, FX::FXString and FX::FXStream (the FOX
	compatibility API does nothing however). You also get the contents of fxutils
	(which is all the misc functions in fxdefs.h)

	You should note that there is no FX::FXObject included in this build nor anything
	deriving from it.

	At the time of writing, the no-GUI build shrinks the binary as follows:
	\li On MSVC7.1 x86, from 3.08Mb to 1.06Mb.
	\li On MSVC8.0 x64, from 4.59Mb to 1.7Mb.
	\li On GCC v4.0.1 x64, from 11Mb to 1.9Mb. If you link statically, you can get
	executables less than 1.5Mb (not bad considering the clib is in there too)

  <li>
    \subsection LotsOfBloat TnFOX has a far bigger binary than FOX!

	Well, TnFOX also comes with a lot of extra stuff! There is about a third extra code in TnFOX
	over normal FOX, and much of it is template heavy. Here are some figures:
	\code
	                          msvc7.1 (x86)    gcc4.0.2 (x64)
	FOX v1.6 [1]            : 2.59Mb           4.1Mb
	TnFOX v0.86 (no GUI)    : 0.95Mb           1.6Mb
	TnFOX v0.86 (minimal)   : 2.97Mb (+15%)    5.7Mb (+39%)
	TnFOX v0.86 (everything): 4.40Mb (+70%)    6.3Mb (+54%)
	\endcode
	[1]: This is FOX compiled with the same options on both platforms

	As you can see, GCC produces code between 68% and 91% larger for the same source.
	Newer versions of GCC should catch up with MSVC over time (indeed, it's already
	50-60% better than with v3.3!)

	The optional parts are as follows:
	<ul>
	 <li>Embedded copy of OpenSSL. To remove, edit sconslib.py
	 <li>Embedded copies of zlib, libpng, libjpeg, libtiff. To remove, edit sconslib.py
	 <li>Embedded copy of sqlite3. To remove, edit sconslib.py or delete \c src/sqlite3
	 <li>FOX compatibility layer. To remove, edit config.py
	 <li>All the GUI code. To remove, edit config.py
    </ul>
</ol>

\section buildqs Build Questions:
<ol>
  <li>
	\subsection msvcbuildnotfound Why when building within the MSVC environment things aren't found?

	The MSVC7.x series have a really annoying feature of overriding your environment variables
	when running a build - thus scons and other utilities can't find things they normally find
	when run in a command box plus executables don't find their DLL's.

	To fix, go to <tt>\verbatim
	C:\Documents and Settings\<user>\Local Settings\Application Data\Microsoft\VisualStudio\7.1
	\endverbatim
	</tt> and open the file \c VCComponents.dat in a text editor. Replace with the following:
	\code
	[VC\VC_OBJECTS_PLATFORM_INFO\Win32\Directories]
	Path Dirs=$(PATH)
	Include Dirs=$(INCLUDE)
	Reference Dirs=$(FrameWorkDir)$(FrameWorkVersion)
	Library Dirs=$(LIB)
	Source Dirs=$(VCInstallDir)atlmfc\src\mfc;$(VCInstallDir)atlmfc\src\atl;$(VCInstallDir)crt\src
	\endcode
	Restart MSVC and you'll now find things work as expected.

</ol>

\section behaviourqs Behaviour Questions:
<ol>
  <li>
	\subsection strangedefaultpermissions Why do files created by my TnFOX application have weird permissions?

	As you'll quickly notice, default permissions for files created by TnFOX give read and
	write access only to the owner of the file. On POSIX \c umask() is completely ignored
	and on NT this is very different to the default of granting Everyone full control.
	Quite simply, this behaviour is by default much more secure for both you and your
	users. If it's a problem, you can always use the QFile::setPermissions() method to
	alter the permissions of a created file or indeed via the static method any arbitrary
	file.

  <li>
	\subsection unabletolockmemorypages What does the warning "Unable to lock memory pages" mean?

	This typically happens on POSIX as only root may prevent memory being paged out. To solve,
	you must set the owner of the relevent process to \c root and set the suid and sgid bits.
	To do this (as root):
	\code
	chown root:root <executable name>
	chmod a+s <executable name>
	\endcode
	Now your TnFOX based application runs with root privileges. Isn't this unsafe? Partially
	yes - however, TnFOX does set the correct ownership of things it creates and on Linux
	restricts its filing system access to that of the real user so it still can't access
	things the owning user couldn't. For all intents and purposes the process runs as though
	it were a typical process though obviously if it's an untrusted application it could
	easily abuse these powers to do anything at all. If you don't mind your unencrypted
	secrets getting written to the swap file or have implemented an encrypted swap file,
	feel free to ignore the warning.
</ol>

\section freebsdqs FreeBSD Questions:
<ol>
  <li>
	\subsection disablingFXFSMonitor What does "Disabling FXFSMonitor due to FAMOpen() throwing error" mean?

	On POSIX, FX::FXFSMonitor is implemented using the FAM library for portable file system
	monitoring behaviour. On RedHat and many Linux installations, the FAM daemon \c famd is
	always running as GNOME and KDE use it - however on FreeBSD to which it only became part
	of the main distribution recently, it is not running by default. TnFOX based applications
	will print the above warning at startup and will retry opening a FAM connection on first
	use of FX::FXFSMonitor but if it fails again, an exception will be returned.

	The solution is to enable \c famd. On FreeBSD v5.x, it gets more complicated as first
	you must create a port mapping in \c /etc/rpc, get the port mapper daemon running (called rpcbind), then
	get \c inetd configured and running and in theory, \c famd will get invoked on demand. You
	should consult the documentation for FAM at http://oss.sgi.com/projects/fam/ and bear in
	mind that sysinstall can do most of the hard work for you in its post-install configure
	section (though you should manually tidy up \c /etc/rc.conf regularly). If running \c inetd
	is too risky for you (even with a configure file devoid of anything except \c famd), it is
	possible to run \c famd standalone - again, see the SGI documentation for more info.

	Just to help you get going quicker:
	
	For \c /etc/rpc (already present on 5.x):
	\code
	sgi_fam 391002 fam # file alteration monitor
	\endcode
	For \c /etc/inetd.conf:
	\code
	sgi_fam/1-2 stream  rpc/tcp wait    root    /usr/local/bin/fam    fam
	\endcode

  <li>
	\subsection spinlockcallederror I'm getting the error "Spinlock called when not threaded" when running executables?

	FreeBSD is generally cleaner, better laid out and better thought through than other Unices.
	One area this is definitely not the case is POSIX threads support - this error results from
	the right pig's breakfast that is pthreads support on the 5.x series. I greatly look forward
	to the day they fix this once and for all by permanently retiring \c libc_r.

	What is happening is that the newer kernel scheduled (kse) POSIX threads is conflicting with
	the previous POSIX threads implementation in \c libc_r - they cannot coexist within the same
	process space. TnFOX uses the newer kind exclusively but if you're getting this error then
	some dependent library has linked against \c libc_r.

	There are two solutions: (i) The slow solution is to use \c ldd to find what shared object
	is importing \c libc_r and recompile it to link against \c libpthread or \c libkse depending
	on your FreeBSD version. (ii) The fast solution is to permanently prevent \c libc_r from
	ever being used again on your system no matter what - to do this, open \c /etc/libmap.conf
	and map libc_r.so to libpthread.so or libkse.so - see <tt>man libmap.conf</tt> for more.
</ol>

*/


/*! \defgroup python Python Support

As of v0.86, the python bindings moved from a pyste based generator to a pyplusplus based
generator. This has meant a significant improvement in bindings quality as pyste required
substantial patching to get it to generate suitable output for TnFOX, but also that pyplusplus
generates better quality output anyway. Nevertheless, it took till v0.87 to get both the bindings
and pyplusplus itself up to a reasonable state - my thanks to Roman Yakovenko for the tireless
work he has put into pyplusplus, most especially that he has spent so much time on getting
the TnFOX bindings up to scratch.

One of the big advantages of pyplusplus over pyste is that you can programmatically set
call policies and patch in code. The many days of manually writing policies that pyste required were effectively
wasted time when viewed from this context - however, it also means that the pyplusplus bindings
are far more maintainable than pyste ones. And the pyste ones were already far more maintainable
than any SWIG based solution.

Current issues with the python bindings which will hopefully get fixed soon:
\li pyplusplus generates wrong code for FX::FXWinShellLink (misinterprets unions).
Currently the entire class is excluded.

\section detail How to build the bindings

TnFOX's Python bindings are almost entirely automatically generated and you can
find the necessary files in the Python directory. You will also need Python v2.4
or later and the Boost library v1.34 or later (or else patch in my <tt>void *</tt>
support patch into an earlier version).

You'll also need a modern compiler such as MSVC7.1 or later or GCC v3.22 or later
(GCC v3.4 or later is far faster, so you should use that instead. GCC v4.0 or later
comes with my symbol visibility patch which very substantially improves load times)

You'll also need a fast computer. Anything using this much compile-time introspection
takes an age but you can rest easy knowing that for every two years hence
compilation speed should double. In only a few years, it'll take no more than five
minutes or so. Until then, distributed compilation is very useful as are
multi-processor machines - and you can tell scons to use as many processors as you
have using the -j switch. You should always run one more build than there are processors
in your system eg; two for a uniprocessor system, three for a dual core system etc.

You also need a \b minimum of 1Gb RAM. This is for linking the bindings, which takes a
huge amount of RAM. Anything less swaps to hell and takes forever.

Place <a href="http://www.boost.org/">Boost</a> into a directory called "boost" next to the TnFOX directory.
Overwrite the files in Boost.Python with the contents of BoostPatches.zip. Get a
\c bjam from somewhere (see boost docs). Run <tt>bjam "-sBUILD=release"</tt> in
\c boost/libs/python/build (use debug if you're building a debug version). This
should build a TnFOX customised boost.python library - no compile errors should
occur, but it should refuse to link (this is deliberate).

Install <a href="http://www.gccxml.org/">gccxml</a>,
<a href="http://www.language-binding.net/pygccxml/pygccxml.html">pygccxml</a>
and <a href="http://www.language-binding.net/pyplusplus/pyplusplus.html">pyplusplus</a>.

Open a command window and go into the <tt>TnFOX/Python</tt> directory. If you have
changed the header files of TnFOX (eg; through upgrading the version), make VERY
sure you delete the fx.xml file first - this is the cache of parsing the TnFOX headers -
and also delete the generated directory as pyplusplus uses the existing output to
speed up its operation.

Run '<tt>python create_tnfox.py</tt>'. After a few minutes it should return. If there
is a file called <tt>patch.diff</tt> in the Python directory then apply this using:
\code
cd generated
patch -p1 < ../patch.diff
\endcode
As pyplusplus improves, the requirement to do this patch will disappear.

Run scons. This part takes between an hour and six hours depending on how fast
your machine is.

\note If using GCC, you \b really want to use a \c -fvisibility enabled version (>=v4.x) of GCC
if possible as it will reduce the size of the TnFOX shared object by around
20Mb and you will reduce load times for anything linked against TnFOX.so from
six minutes to four seconds :). You will also need v1.3.2 or later of Boost.

Afterwards, it's as simple as
\code
from TnFOX import *
\endcode
... to begin writing powerful portable GUI based apps from Python!
\section features Features:

In fact it's easier to list what doesn't work rather than what
does which is the overriding quantity - but off the top of my head:
\li Apart from the exceptions below, all classes & all enums and
all members (nested classes, enums, data etc) therein including a full
duplication of the class inheritance structure, except for those
retired for reasons of deprecation or where it doesn't make sense
to include them. See declarations_to_exclude.py and files_to_exclude.py.
Most of these are sensible.
\li Apart from one virtual method in FXApp,
every virtual thing can be overriden by python code.
\li All method & function overloads are available.
\li Default parameters work as expected, and you can name your parameters
\li Complex number support
\li Provision for simple invocation of python code
\li Multiple python interpreters, plus multiple threads therein.
\li Via BPL, extremely easy manipulation of python objects from
C++ plus embedding of python mini-scripts and python expression evaluations.

Furthermore I've implemented a number of automatic conversions
where TnFOX classes transparently become Python ones and vice versa:
\li FX::FXString with python strings. On v1.6 FOX based builds, FX::FXString
always converts to a python
unicode object. String and Unicode objects convert to FX::FXString. On
v1.4 FOX based builds, python unicode object conversion is not supported.
\li FX::FXuchar, FX::FXchar, FX::FXushort, FX::FXshort, FX::FXuint
and possibly FX::FXint & FX::FXulong (depending on host long integer
size) all automatically translate to/from a python integer. FX::FXulong and
FX::FXlong on 32 bit machines automatically translate to/from a
python long integer.
\li A python list of strings becomes <tt>const char **</tt> and vice versa. Be
VERY careful with this as I don't copy the strings so ensure the
list lives forever. This was intended purely for use for argv
situations and nothing more.
\li FXException and anything deriving off it when thrown appear as
a python exception of e.report(). Even the most appropriate type of
python exception is chosen where possible.
\li Python exceptions automatically become a FX::FXPythonException.
\li C style arrays become full python lists which can be sliced,
altered etc. - the only restriction is that they cannot be extended
or shrunk. You can even alter FXImage's bitmap data directly through
altering the sequence returned by getData()! Note however that
FXGLViewer::lasso and FXGLViewer::select's returns are very simple
container emulations and only offer read-only access. However, in
all cases length is correctly observed and maintained.
\li The QTL (currently only QValueList & QMemArray but QPtrList is pending) maps onto a
fully-featured python emulation. Items returned from QPtrList are
expected to be managed entirely by \c new - if this isn't the case,
do not modify the list - use the QPtrList methods directly. If you
take references from a QPtrList, their lifetime is set to expire
with the list reference.
\li The same method for managing FXWindow message dispatching as FXPy
was chosen for aiding compatibility. For each instance of the window
class, in its constructor you should call the member functions FXMAPFUNC(),
FXMAPFUNCS(), FXMAPTYPE() or FXMAPTYPES() as appropriate. These have
the same spec as their macro counterparts in FXObject.h.

If you look inside the Python directory, there is a "FXPy examples" directory
containing some of the same examples that come with FXPy except altered
to use TnFOX. The changes in most cases were trivial, but you should note
that these python bindings are automatically generated rather than by hand,
so useful behaviour like FXPy had is not possible:
\li If an enum is named, it is \b always accessed in TnFOX's python bindings
by prefixing the constant with its enum name eg; \c FXSelType.SEL_COMMAND
rather than just \c SEL_COMMAND. I know this isn't like the C++, but I
agree with BPL's designers that more disambiguation is better than less.
\li Another difference is that python object instances and their C++
counterpart are tied together strongly in the direction of python upwards.
In other words, TnFOX can delete the C++ side of a python object but
the python object shall continue to exist, except that it becomes a
zombie. Similarly to this, if you delete the python side of an object,
it \b always deletes its C++ side. Typical code in C++ such as:
\code
new MenuCommand(parentmenu, ...)
\endcode
... cannot be done in these python bindings as BPL has no idea that
you really want to leave the newed pointer dangling. Therefore, if
you don't tie something to a reference lasting the length of the parent
instance (eg; by setting \c self.something to it), python will delete it.
This is a major difference from FXPy and code will need to be adjusted
to support it - see \c imageviewer.py ported from FXPy to see what I mean.

\subsection caveats Caveats:

Not everything can be supported. Python and C++ are two very different
languages which is why they complement each other so well. Things that are
not supported and never will be supported:
\li Setting or retrieving pointers to functions. There is a workaround
for FOX's sort functions which lets you set arbitrary python code as
the sort function. The code which enables this is public (FX::FXCodeToPythonCode)
so if you have a similar problem, you can use that. Remember that
C++ can't generate new C++ code like python code can, so this will
always be a design constraint.
\li Anything taking ellipsis parameters (...)
\li Python doesn't have the concept of something being unsigned.
Furthermore there are no chars or shorts to speak of - just
a plain signed integer which can be far bigger than a 64 bit integer.
\li The FOX portion of TnFOX is not exception safe. BPL throws an
exception when something displeases it and if there's any GUI code
up the call stack, it can leave FOX in an indeterminate state (ie;
royally screwed sometimes where the only realistic recourse is
an immediate fatal exit).
\li Python code cannot be cancelled via a POSIX thread cancellation
(it would leave reference counts dangling) and so thread cancellation
is automatically disabled during calls into python code. This means
you must take care to call QThread::checkForTerminate() which if it
returns true, raise a SystemExit exception or something (SystemExit
is ignored by the default python run() call code).

And then also there's a long list of things which will be added or
fixed with time:
\li None of the TnFOX docs are reflected into python doc strings
even though Boost.Python supports this. This is one of the future
features which should be added to pyplusplus in time.
\li BPL gets confused where overloads have a static member function
and a normal member function. It will default to the static version -
therefore you'll need to pass the instance as the first parameter
to the non-static version. This may get fixed in some future version
of BPL.

Finally, there is the issue of lifetime management. Boost.Python
allows you to tie the lifetime of things together to prevent
dangling references eg; you get the address() from a QBlkSocket
and delete the QBlkSocket - now the address is clearly no longer
valid unless you made a copy of it which is exactly what has been
enforced.

As it happens, actually in this case as a QHostAddress is copyable,
a copy is made silently for you. But the point remains valid -
wherever I have had a choice, I erred on the side of caution and
chose anything returned (all reference and pointer types - values are
copied as are const references when the object has a copy constructor)
to live only as long as the thing you got it from. Hence
things like FXApp::getWindowAt() where the window returned would almost
certainly outlive the reference you temporarily got from
FXApp::instance() will in fact become invalid when your FXApp reference
is killed. If you think I was being over-zealous, please post to
the newsgroup below stating why with a \b good example of why
alternative behaviour would be better and <b>still safe</b>!

If you want support for these bindings, please join the
tnfox-discussion<br>
at<br>
lists.sourceforge.net mailing list and ask there.

*/


/*! \page CppMunge The CppMunge.py tool

You will find a script written in Python called CppMunge.py (it needs v2.3 or later of 
<a href="http://www.python.org/">Python</a> installed). While use of it is optional,
it does make your life a good deal easier and TnFOX itself uses it. It provides the
following features:
\li <b>Extraction and allocation of error code macros</b><br>
Now you no longer need to concern yourself with allocating constants to error code macros.
Simply write in your macro name and include the header file "ErrCodes.h" at the top of
each of your exception using source files and CppMunge.py takes care of allocating your
exception a unique number it will maintain for the rest of its life.
\code
#include "FXException.h"
#include "ErrCodes.h"
...
FXERRH(file.exists(), tr("File does not exist"), MYCLASS_FILENOTFOUND, 0);
\endcode
TnFOX's own error codes list is in \c FXErrCodes.h. Note that once allocated, a constant
remains forever so you need to remove them manually from \c FXErrCodes.h if you need to.
Bear in mind the number will then be reused, so you may wish to call it RESERVEDx or
something to avoid breaking binary compatibility.

\li <b>Implementation of TnFOX's nested exception handling support</b><br>
More about this can be found at the documentation page of FX::FXException. Basically
it wraps all your destructors (unless marked with \c throw() or
\c FXERRH_NODESTRUCTORMOD) with the run-time support code necessary for nested exception handling.

\li <b>Extraction of text literals delimited by \c tr() </b><br>
You will certainly want to run all your source through the utility and
have it generate a skeleton translation file for you to translate rather
than do it yourself. CppMunge is intelligent and can add in and demarcate
for translation newly added text literals by the programmer in all
supported languages. To add a new language, simply add to the language id list
at the top of the file and CppMunge will add in "!TODO!" literals where you need
to add them. \warning You should keep a backup of the translation file as typing
mistakes in your source can cause CppMunge to remove translations on you.


<h3>Installation:</h3>
CppMunge was designed to modify in-place C++ source without making its changes
too noticeable. A previous solution generated a separate output file which had
the problem of debugging referring to the wrong source file plus I myself
kept altering the munged copy to fix bugs rather than its original. This new
solution is much more convenient.

However it can slow things down a bit as the translation & error codes file
needs to be loaded, decoded, modified and written out for each and every
source file compiled. To avoid this I wrote another python script called
\c UpdateMunged.py which simply scans the source directory and if any
munged files are newer than a time stamp file, it calls \c CppMunge.py on
them as a child process. To permit UpdateMunged to distinguish between
mungeable files and normal ones, munged files use a \c .cxx extension
rather than \c .cpp. Further optimisations include CppMunge not writing
out anything unless it has changed.

You can disable features in either utility via commmand line options -
try running CppMunge.py with \c -help to see what options it supports.

\note The C++ parser used by CppMunge is extremely simple and likely to get confused
by anything complicated so do not modify the lines it changes AT ALL. You
might also want to keep method definitions well spaced and on separate lines
like I naturally tend to write my C++!

*/







/*! \page acknowledgements Acknowledgements

TnFOX could not have been possible without a number of key people and
its development would have been made much harder without the time &
energy expended by many others. This page I hope does something in
return for that.

\li My father, Francis Douglas, for lending me money when my monitor
blew up mid-development of Tornado and since by housing & feeding me
as I develop the code base for Tn. Without his help, I would need at
least a part-time job to provide food money.
\li Jeroen van der Zijp for designing and writing almost all of FOX and
furthermore for placing it under the LGPL so we all could use it.
\li Dave Abrahams & Nicodemus and the others who created Boost.Python library.
\li Eric Young, Tim Hudson & the others who created the lovely OpenSSL library.
\li Marshall Cline for a long & detailed email correspondence helping
me with my problems regarding C++ templates
and pointing out inheritance needs to preserve base API along with many
other corrections and tips for avoiding traps when designing and writing
Tornado. Some Tornado code whose design was fixed by him was incorporated
almost verbatim into TnFOX and I am very grateful to him for helping
accelerate my mastery of C++.
His excellent C++ FAQ which is strongly recommended is at
<a href="http://www.parashift.com/c++-faq-lite/">http://www.parashift.com/c++-faq-lite/</a>
plus he is one of the authors of <i>C++ FAQs</i>, a highly regarded book
on C++ detail.
\li Raoul Gough for his invaluable assistence in getting C arrays wrapped
so Python could access them. His solution was far cleaner than mine!
\li Scott Lees for persuading me to choose Python over Tcl as the Tornado
shell language. You can see evidence of this today in the python bindings
TnFOX provides.
\li Fellow graduates from my class year 2000 at the <a href="http://www.hull.ac.uk/">University
of Hull</a> in the UK who have tirelessly put up with my constant
questions about tiny details.
\li Roman Yakovenko for the tireless work he has put into pyplusplus, most
especially that he has specifically spent so much time on getting the TnFOX
bindings up to scratch.
*/




/*! \page transfiletutorial How to write translation files

TnFOX provides what is probably the most powerful and intuitive automatic human
language translation for any C++ library currently available. While it is mostly
source compatible with Qt, it also provides a good deal more power.

A tool is provided called \c TranslateViaGoogle.py which screen scrapes the
output from Google Translations in order to automate the translations for you
into all supported languages. Simply call with the output and input files
and much of the work will be done for you - though you WILL need to manually
refine the output afterwards. Google Translate will only do so many
translations for you before it realises that it is an automated script and
will refuse to provide any more - thus the tool can restart a partially
completed automatic translation, or indeed it can fill in the blanks of a
manually translated file. You need the \c BeautifulSoup Python package as
a prerequisite for operation.

Much of that power derives from the intuitive and powerful method of
specifying translations. The file format is basically as follows:
\code
"<program original text literal>":
	<langid>: "<generic translation for this language>"
	<langid>_<regionid>: "<localised translation for this language>"
	...
	class="<class name>":
		<langid>: up|"<specific translation for this class in this language>"
		...
	srcfile="<source file name>":
		...
	hint="<hint>":
		...

"<next literal>":
	...
\endcode
As of v0.86 of TnFOX (v1.6 FOX based builds only), you can write this file in
UTF-8, UTF-16 or UTF-32 and TnFOX will automatically detect which format it is
in. Therefore, you can use any text editor you like (including the special TnFOX
edition of Adie which can save out in any of these formats).

More powerful operators are the <tt>%1, %2, %3 ...</tt> which let you specialise
a translation based on parameter insert value eg;
\code
"%1 pretty round teapots":
	ES: "%1 teteras redondas bonitas"
	%1=1:
		EN: "%1 pretty round teapot"
		ES: "%1 tetera redonda bonita"
\endcode
You can also arbitrarily combine modifiers eg;
\code
	srcfile="<source file name>":class="<class name>":hint="<hint>"
		<langid>: "<translation>"
\endcode
Note also that the tabulation is important - and they \b must be tab (0x09)
characters, not spaces. Note also that you can substitute the word "up" for
a particular language - this makes it use the next highest available translation
for that language id. If you look at the output of CppMunge.py, it will look
something like this:
\code
"Device reopen has different mode":
	ES: "El modo de operacion no es iqual que antes"
	srcfile="QBuffer.cxx":class="QBuffer":
		ES: up
	srcfile="QFile.cxx":class="QFile":
		ES: up
	srcfile="QGZipDevice.cxx":class="QGZipDevice":
		ES: up
\endcode
By convention, all source files and classes are listed in the translation
file even though they usually are "up". Why? Because this means you don't
accidentally miss a special translation for some class or source file as
all the classes and source files using a particular literal are listed.

Using this format, you have amazing flexibility with specifying special
translation situations for some languages and not for others, in a very 
large variety of circumstances. <b>It is for this reason that I believe
TnFOX has the most superior translation facilities available today!</b>

<h3>Language codes</h3>
Ideally, a language code is of the form language[_territory][@modifier], where language is
a two-letter ISO 639 language code and territory is a two-letter ISO 3166 country code.
However, due to incompatibilities in the implementation of the standard C library's \c setlocale()
function, it isn't as easy as that.

Generally speaking you will not specialise past language and at most territory (currency
differences eg; between pre and post-euro Europe tend to be held in the modifier. If you
are on a POSIX system, run <tt>"locale -a"</tt> off a command line). Theoretically, Microsoft
Windows reports the same ids as GNU/Linux correctly but I have found during testing
that your milage may vary. If any really annoying incompatibilities arise, let me
know and I'll implement id mapping to make them portable.

Below is a list of common language ids bewtween Microsoft Windows and GNU/Linux:
\code
cs (Czech),     da (Danish),   nl (Dutch),  en (English),   fi (Finnish),
fr (French),    de (German),   el (Greek),  hu (Hungarian), is (Icelandic),
it (Italian),   ja (Japanese), ko (Korean), no (Norwegian), pl (Polish),
pt (Portugese), ru (Russian),  sk (Slovak), es (Spanish),   sv (Swedish),
tr (Turkish).
\endcode

This is a pitiful subset, missing out important ones such as Arabic, Chinese and
Indian languages. I also find it depressing that the above nations are more or
less the major geopolitical players around the time of the first world war!

\note Windows also supports: chs (Chinese simplified) & chn (Chinese traditional).

\note GNU/Linux also supports: ar (Arabic)

As for country codes, I have found the greatest variance here. Below is a list
as defined on the ISO 3166 website:

<table border="0" cellpadding="0">
  <tr>
    <td>AFGHANISTAN</td>
    <td align="center">AF</td>
    <td width="10"></td>
    <td>LIBYAN ARAB JAMAHIRIYA</td>
    <td align="center">LY</td>
  </tr>
  <tr>
    <td>ALBANIA</td>
    <td align="center">AL</td>
    <td width="10"></td>
    <td>LIECHTENSTEIN</td>
    <td align="center">LI</td>
  </tr>
  <tr>
    <td>ALGERIA</td>
    <td align="center">DZ</td>
    <td width="10"></td>
    <td>LITHUANIA</td>
    <td align="center">LT</td>
  </tr>
  <tr>
    <td>AMERICAN SAMOA</td>
    <td align="center">AS</td>
    <td width="10"></td>
    <td>LUXEMBOURG</td>
    <td align="center">LU</td>
  </tr>
  <tr>
    <td>ANDORRA</td>
    <td align="center">AD</td>
    <td width="10"></td>
    <td>MACAO</td>
    <td align="center">MO</td>
  </tr>
  <tr>
    <td>ANGOLA</td>
    <td align="center">AO</td>
    <td width="10"></td>
    <td>MACEDONIA</td>
    <td align="center">MK</td>
  </tr>
  <tr>
    <td>ANGUILLA</td>
    <td align="center">AI</td>
    <td width="10"></td>
    <td>MADAGASCAR</td>
    <td align="center">MG</td>
  </tr>
  <tr>
    <td>ANTARCTICA</td>
    <td align="center">AQ</td>
    <td width="10"></td>
    <td>MALAWI</td>
    <td align="center">MW</td>
  </tr>
  <tr>
    <td>ANTIGUA AND BARBUDA</td>
    <td align="center">AG</td>
    <td width="10"></td>
    <td>MALAYSIA</td>
    <td align="center">MY</td>
  </tr>
  <tr>
    <td>ARGENTINA</td>
    <td align="center">AR</td>
    <td width="10"></td>
    <td>MALDIVES</td>
    <td align="center">MV</td>
  </tr>
  <tr>
    <td>ARMENIA</td>
    <td align="center">AM</td>
    <td width="10"></td>
    <td>MALI</td>
    <td align="center">ML</td>
  </tr>
  <tr>
    <td>ARUBA</td>
    <td align="center">AW</td>
    <td width="10"></td>
    <td>MALTA</td>
    <td align="center">MT</td>
  </tr>
  <tr>
    <td>AUSTRALIA</td>
    <td align="center">AU</td>
    <td width="10"></td>
    <td>MARSHALL ISLANDS</td>
    <td align="center">MH</td>
  </tr>
  <tr>
    <td>AUSTRIA</td>
    <td align="center">AT</td>
    <td width="10"></td>
    <td>MARTINIQUE</td>
    <td align="center">MQ</td>
  </tr>
  <tr>
    <td>AZERBAIJAN</td>
    <td align="center">AZ</td>
    <td width="10"></td>
    <td>MAURITANIA</td>
    <td align="center">MR</td>
  </tr>
  <tr>
    <td>BAHAMAS</td>
    <td align="center">BS</td>
    <td width="10"></td>
    <td>MAURITIUS</td>
    <td align="center">MU</td>
  </tr>
  <tr>
    <td>BAHRAIN</td>
    <td align="center">BH</td>
    <td width="10"></td>
    <td>MAYOTTE</td>
    <td align="center">YT</td>
  </tr>
  <tr>
    <td>BANGLADESH</td>
    <td align="center">BD</td>
    <td width="10"></td>
    <td>MEXICO</td>
    <td align="center">MX</td>
  </tr>
  <tr>
    <td>BARBADOS</td>
    <td align="center">BB</td>
    <td width="10"></td>
    <td>MICRONESIA</td>
    <td align="center">FM</td>
  </tr>
  <tr>
    <td>BELARUS</td>
    <td align="center">BY</td>
    <td width="10"></td>
    <td>MOLDOVA</td>
    <td align="center">MD</td>
  </tr>
  <tr>
    <td>BELGIUM</td>
    <td align="center">BE</td>
    <td width="10"></td>
    <td>MONACO</td>
    <td align="center">MC</td>
  </tr>
  <tr>
    <td>BELIZE</td>
    <td align="center">BZ</td>
    <td width="10"></td>
    <td>MONGOLIA</td>
    <td align="center">MN</td>
  </tr>
  <tr>
    <td>BENIN</td>
    <td align="center">BJ</td>
    <td width="10"></td>
    <td>MONTSERRAT</td>
    <td align="center">MS</td>
  </tr>
  <tr>
    <td>BERMUDA</td>
    <td align="center">BM</td>
    <td width="10"></td>
    <td>MOROCCO</td>
    <td align="center">MA</td>
  </tr>
  <tr>
    <td>BHUTAN</td>
    <td align="center">BT</td>
    <td width="10"></td>
    <td>MOZAMBIQUE</td>
    <td align="center">MZ</td>
  </tr>
  <tr>
    <td>BOLIVIA</td>
    <td align="center">BO</td>
    <td width="10"></td>
    <td>MYANMAR</td>
    <td align="center">MM</td>
  </tr>
  <tr>
    <td>BOSNIA AND HERZEGOVINA</td>
    <td align="center">BA</td>
    <td width="10"></td>
    <td>NAMIBIA</td>
    <td align="center">NA</td>
  </tr>
  <tr>
    <td>BOTSWANA</td>
    <td align="center">BW</td>
    <td width="10"></td>
    <td>NAURU</td>
    <td align="center">NR</td>
  </tr>
  <tr>
    <td>BOUVET ISLAND</td>
    <td align="center">BV</td>
    <td width="10"></td>
    <td>NEPAL</td>
    <td align="center">NP</td>
  </tr>
  <tr>
    <td>BRAZIL</td>
    <td align="center">BR</td>
    <td width="10"></td>
    <td>NETHERLANDS</td>
    <td align="center">NL</td>
  </tr>
  <tr>
    <td>BRITISH INDIAN OCEAN TERRITORY</td>
    <td align="center">IO</td>
    <td width="10"></td>
    <td>NETHERLANDS ANTILLES</td>
    <td align="center">AN</td>
  </tr>
  <tr>
    <td>BRUNEI DARUSSALAM</td>
    <td align="center">BN</td>
    <td width="10"></td>
    <td>NEW CALEDONIA</td>
    <td align="center">NC</td>
  </tr>
  <tr>
    <td>BULGARIA</td>
    <td align="center">BG</td>
    <td width="10"></td>
    <td>NEW ZEALAND</td>
    <td align="center">NZ</td>
  </tr>
  <tr>
    <td>BURKINA FASO</td>
    <td align="center">BF</td>
    <td width="10"></td>
    <td>NICARAGUA</td>
    <td align="center">NI</td>
  </tr>
  <tr>
    <td>BURUNDI</td>
    <td align="center">BI</td>
    <td width="10"></td>
    <td>NIGER</td>
    <td align="center">NE</td>
  </tr>
  <tr>
    <td>CAMBODIA</td>
    <td align="center">KH</td>
    <td width="10"></td>
    <td>NIGERIA</td>
    <td align="center">NG</td>
  </tr>
  <tr>
    <td>CAMEROON</td>
    <td align="center">CM</td>
    <td width="10"></td>
    <td>NIUE</td>
    <td align="center">NU</td>
  </tr>
  <tr>
    <td>CANADA</td>
    <td align="center">CA</td>
    <td width="10"></td>
    <td>NORFOLK ISLAND</td>
    <td align="center">NF</td>
  </tr>
  <tr>
    <td>CAPE VERDE</td>
    <td align="center">CV</td>
    <td width="10"></td>
    <td>NORTHERN MARIANA ISLANDS</td>
    <td align="center">MP</td>
  </tr>
  <tr>
    <td>CAYMAN ISLANDS</td>
    <td align="center">KY</td>
    <td width="10"></td>
    <td>NORWAY</td>
    <td align="center">NO</td>
  </tr>
  <tr>
    <td>CENTRAL AFRICAN REPUBLIC</td>
    <td align="center">CF</td>
    <td width="10"></td>
    <td>OMAN</td>
    <td align="center">OM</td>
  </tr>
  <tr>
    <td>CHAD</td>
    <td align="center">TD</td>
    <td width="10"></td>
    <td>PAKISTAN</td>
    <td align="center">PK</td>
  </tr>
  <tr>
    <td>CHILE</td>
    <td align="center">CL</td>
    <td width="10"></td>
    <td>PALAU</td>
    <td align="center">PW</td>
  </tr>
  <tr>
    <td>CHINA</td>
    <td align="center">CN</td>
    <td width="10"></td>
    <td>PALESTINIAN TERRITORY</td>
    <td align="center">PS</td>
  </tr>
  <tr>
    <td>CHRISTMAS ISLAND</td>
    <td align="center">CX</td>
    <td width="10"></td>
    <td>PANAMA</td>
    <td align="center">PA</td>
  </tr>
  <tr>
    <td>COCOS (KEELING) ISLANDS</td>
    <td align="center">CC</td>
    <td width="10"></td>
    <td>PAPUA NEW GUINEA</td>
    <td align="center">PG</td>
  </tr>
  <tr>
    <td>COLOMBIA</td>
    <td align="center">CO</td>
    <td width="10"></td>
    <td>PARAGUAY</td>
    <td align="center">PY</td>
  </tr>
  <tr>
    <td>COMOROS</td>
    <td align="center">KM</td>
    <td width="10"></td>
    <td>PERU</td>
    <td align="center">PE</td>
  </tr>
  <tr>
    <td>CONGO</td>
    <td align="center">CG</td>
    <td width="10"></td>
    <td>PHILIPPINES</td>
    <td align="center">PH</td>
  </tr>
  <tr>
    <td>COOK ISLANDS</td>
    <td align="center">CK</td>
    <td width="10"></td>
    <td>PITCAIRN</td>
    <td align="center">PN</td>
  </tr>
  <tr>
    <td>COSTA RICA</td>
    <td align="center">CR</td>
    <td width="10"></td>
    <td>POLAND</td>
    <td align="center">PL</td>
  </tr>
  <tr>
    <td>COTE D'IVOIRE</td>
    <td align="center">CI</td>
    <td width="10"></td>
    <td>PORTUGAL</td>
    <td align="center">PT</td>
  </tr>
  <tr>
    <td>CROATIA</td>
    <td align="center">HR</td>
    <td width="10"></td>
    <td>PUERTO RICO</td>
    <td align="center">PR</td>
  </tr>
  <tr>
    <td>CUBA</td>
    <td align="center">CU</td>
    <td width="10"></td>
    <td>QATAR</td>
    <td align="center">QA</td>
  </tr>
  <tr>
    <td>CYPRUS</td>
    <td align="center">CY</td>
    <td width="10"></td>
    <td>REUNION</td>
    <td align="center">RE</td>
  </tr>
  <tr>
    <td>CZECH REPUBLIC</td>
    <td align="center">CZ</td>
    <td width="10"></td>
    <td>ROMANIA</td>
    <td align="center">RO</td>
  </tr>
  <tr>
    <td>DENMARK</td>
    <td align="center">DK</td>
    <td width="10"></td>
    <td>RUSSIAN FEDERATION</td>
    <td align="center">RU</td>
  </tr>
  <tr>
    <td>DJIBOUTI</td>
    <td align="center">DJ</td>
    <td width="10"></td>
    <td>RWANDA</td>
    <td align="center">RW</td>
  </tr>
  <tr>
    <td>DOMINICA</td>
    <td align="center">DM</td>
    <td width="10"></td>
    <td>SAINT HELENA</td>
    <td align="center">SH</td>
  </tr>
  <tr>
    <td>DOMINICAN REPUBLIC</td>
    <td align="center">DO</td>
    <td width="10"></td>
    <td>SAINT KITTS AND NEVIS</td>
    <td align="center">KN</td>
  </tr>
  <tr>
    <td>ECUADOR</td>
    <td align="center">EC</td>
    <td width="10"></td>
    <td>SAINT LUCIA</td>
    <td align="center">LC</td>
  </tr>
  <tr>
    <td>EGYPT</td>
    <td align="center">EG</td>
    <td width="10"></td>
    <td>SAINT PIERRE AND MIQUELON</td>
    <td align="center">PM</td>
  </tr>
  <tr>
    <td>EL SALVADOR</td>
    <td align="center">SV</td>
    <td width="10"></td>
    <td>SAINT VINCENT AND THE GRENADINES</td>
    <td align="center">VC</td>
  </tr>
  <tr>
    <td>EQUATORIAL GUINEA</td>
    <td align="center">GQ</td>
    <td width="10"></td>
    <td>SAMOA</td>
    <td align="center">WS</td>
  </tr>
  <tr>
    <td>ERITREA</td>
    <td align="center">ER</td>
    <td width="10"></td>
    <td>SAN MARINO</td>
    <td align="center">SM</td>
  </tr>
  <tr>
    <td>ESTONIA</td>
    <td align="center">EE</td>
    <td width="10"></td>
    <td>SAO TOME AND PRINCIPE</td>
    <td align="center">ST</td>
  </tr>
  <tr>
    <td>ETHIOPIA</td>
    <td align="center">ET</td>
    <td width="10"></td>
    <td>SAUDI ARABIA</td>
    <td align="center">SA</td>
  </tr>
  <tr>
    <td>FALKLAND ISLANDS (MALVINAS)</td>
    <td align="center">FK</td>
    <td width="10"></td>
    <td>SENEGAL</td>
    <td align="center">SN</td>
  </tr>
  <tr>
    <td>FAROE ISLANDS</td>
    <td align="center">FO</td>
    <td width="10"></td>
    <td>SEYCHELLES</td>
    <td align="center">SC</td>
  </tr>
  <tr>
    <td>FIJI</td>
    <td align="center">FJ</td>
    <td width="10"></td>
    <td>SIERRA LEONE</td>
    <td align="center">SL</td>
  </tr>
  <tr>
    <td>FINLAND</td>
    <td align="center">FI</td>
    <td width="10"></td>
    <td>SINGAPORE</td>
    <td align="center">SG</td>
  </tr>
  <tr>
    <td>FRANCE</td>
    <td align="center">FR</td>
    <td width="10"></td>
    <td>SLOVAKIA</td>
    <td align="center">SK</td>
  </tr>
  <tr>
    <td>FRENCH GUIANA</td>
    <td align="center">GF</td>
    <td width="10"></td>
    <td>SLOVENIA</td>
    <td align="center">SI</td>
  </tr>
  <tr>
    <td>FRENCH POLYNESIA</td>
    <td align="center">PF</td>
    <td width="10"></td>
    <td>SOLOMON ISLANDS</td>
    <td align="center">SB</td>
  </tr>
  <tr>
    <td>FRENCH SOUTHERN TERRITORIES</td>
    <td align="center">TF</td>
    <td width="10"></td>
    <td>SOMALIA</td>
    <td align="center">SO</td>
  </tr>
  <tr>
    <td>GABON</td>
    <td align="center">GA</td>
    <td width="10"></td>
    <td>SOUTH AFRICA</td>
    <td align="center">ZA</td>
  </tr>
  <tr>
    <td>GAMBIA</td>
    <td align="center">GM</td>
    <td width="10"></td>
    <td>SPAIN</td>
    <td align="center">ES</td>
  </tr>
  <tr>
    <td>GEORGIA</td>
    <td align="center">GE</td>
    <td width="10"></td>
    <td>SRI LANKA</td>
    <td align="center">LK</td>
  </tr>
  <tr>
    <td>GERMANY</td>
    <td align="center">DE</td>
    <td width="10"></td>
    <td>SUDAN</td>
    <td align="center">SD</td>
  </tr>
  <tr>
    <td>GHANA</td>
    <td align="center">GH</td>
    <td width="10"></td>
    <td>SURINAME</td>
    <td align="center">SR</td>
  </tr>
  <tr>
    <td>GIBRALTAR</td>
    <td align="center">GI</td>
    <td width="10"></td>
    <td>SVALBARD AND JAN MAYEN</td>
    <td align="center">SJ</td>
  </tr>
  <tr>
    <td>GREECE</td>
    <td align="center">GR</td>
    <td width="10"></td>
    <td>SWAZILAND</td>
    <td align="center">SZ</td>
  </tr>
  <tr>
    <td>GREENLAND</td>
    <td align="center">GL</td>
    <td width="10"></td>
    <td>SWEDEN</td>
    <td align="center">SE</td>
  </tr>
  <tr>
    <td>GRENADA</td>
    <td align="center">GD</td>
    <td width="10"></td>
    <td>SWITZERLAND</td>
    <td align="center">CH</td>
  </tr>
  <tr>
    <td>GUADELOUPE</td>
    <td align="center">GP</td>
    <td width="10"></td>
    <td>SYRIAN ARAB REPUBLIC</td>
    <td align="center">SY</td>
  </tr>
  <tr>
    <td>GUAM</td>
    <td align="center">GU</td>
    <td width="10"></td>
    <td>TAIWAN</td>
    <td align="center">TW</td>
  </tr>
  <tr>
    <td>GUATEMALA</td>
    <td align="center">GT</td>
    <td width="10"></td>
    <td>TAJIKISTAN</td>
    <td align="center">TJ</td>
  </tr>
  <tr>
    <td>GUINEA</td>
    <td align="center">GN</td>
    <td width="10"></td>
    <td>TANZANIA</td>
    <td align="center">TZ</td>
  </tr>
  <tr>
    <td>GUINEA-BISSAU</td>
    <td align="center">GW</td>
    <td width="10"></td>
    <td>THAILAND</td>
    <td align="center">TH</td>
  </tr>
  <tr>
    <td>GUYANA</td>
    <td align="center">GY</td>
    <td width="10"></td>
    <td>TIMOR-LESTE</td>
    <td align="center">TL</td>
  </tr>
  <tr>
    <td>HAITI</td>
    <td align="center">HT</td>
    <td width="10"></td>
    <td>TOGO</td>
    <td align="center">TG</td>
  </tr>
  <tr>
    <td>HOLY SEE (VATICAN CITY STATE)</td>
    <td align="center">VA</td>
    <td width="10"></td>
    <td>TOKELAU</td>
    <td align="center">TK</td>
  </tr>
  <tr>
    <td>HONDURAS</td>
    <td align="center">HN</td>
    <td width="10"></td>
    <td>TONGA</td>
    <td align="center">TO</td>
  </tr>
  <tr>
    <td>HONG KONG</td>
    <td align="center">HK</td>
    <td width="10"></td>
    <td>TRINIDAD AND TOBAGO</td>
    <td align="center">TT</td>
  </tr>
  <tr>
    <td>HUNGARY</td>
    <td align="center">HU</td>
    <td width="10"></td>
    <td>TUNISIA</td>
    <td align="center">TN</td>
  </tr>
  <tr>
    <td>ICELAND</td>
    <td align="center">IS</td>
    <td width="10"></td>
    <td>TURKEY</td>
    <td align="center">TR</td>
  </tr>
  <tr>
    <td>INDIA</td>
    <td align="center">IN</td>
    <td width="10"></td>
    <td>TURKMENISTAN</td>
    <td align="center">TM</td>
  </tr>
  <tr>
    <td>INDONESIA</td>
    <td align="center">ID</td>
    <td width="10"></td>
    <td>TURKS AND CAICOS ISLANDS</td>
    <td align="center">TC</td>
  </tr>
  <tr>
    <td>IRAN</td>
    <td align="center">IR</td>
    <td width="10"></td>
    <td>TUVALU</td>
    <td align="center">TV</td>
  </tr>
  <tr>
    <td>IRAQ</td>
    <td align="center">IQ</td>
    <td width="10"></td>
    <td>UGANDA</td>
    <td align="center">UG</td>
  </tr>
  <tr>
    <td>IRELAND</td>
    <td align="center">IE</td>
    <td width="10"></td>
    <td>UKRAINE</td>
    <td align="center">UA</td>
  </tr>
  <tr>
    <td>ISRAEL</td>
    <td align="center">IL</td>
    <td width="10"></td>
    <td>UNITED ARAB EMIRATES</td>
    <td align="center">AE</td>
  </tr>
  <tr>
    <td>ITALY</td>
    <td align="center">IT</td>
    <td width="10"></td>
    <td>UNITED KINGDOM</td>
    <td align="center">GB</td>
  </tr>
  <tr>
    <td>JAMAICA</td>
    <td align="center">JM</td>
    <td width="10"></td>
    <td>UNITED STATES</td>
    <td align="center">US</td>
  </tr>
  <tr>
    <td>JAPAN</td>
    <td align="center">JP</td>
    <td width="10"></td>
    <td>URUGUAY</td>
    <td align="center">UY</td>
  </tr>
  <tr>
    <td>JORDAN</td>
    <td align="center">JO</td>
    <td width="10"></td>
    <td>UZBEKISTAN</td>
    <td align="center">UZ</td>
  </tr>
  <tr>
    <td>KAZAKHSTAN</td>
    <td align="center">KZ</td>
    <td width="10"></td>
    <td>VANUATU</td>
    <td align="center">VU</td>
  </tr>
  <tr>
    <td>KENYA</td>
    <td align="center">KE</td>
    <td width="10"></td>
    <td>VENEZUELA</td>
    <td align="center">VE</td>
  </tr>
  <tr>
    <td>KIRIBATI</td>
    <td align="center">KI</td>
    <td width="10"></td>
    <td>VIET NAM</td>
    <td align="center">VN</td>
  </tr>
  <tr>
    <td>KOREA</td>
    <td align="center">KR</td>
    <td width="10"></td>
    <td>VIRGIN ISLANDS</td>
    <td align="center">VG</td>
  </tr>
  <tr>
    <td>KUWAIT</td>
    <td align="center">KW</td>
    <td width="10"></td>
    <td>WALLIS AND FUTUNA</td>
    <td align="center">WF</td>
  </tr>
  <tr>
    <td>KYRGYZSTAN</td>
    <td align="center">KG</td>
    <td width="10"></td>
    <td>WESTERN SAHARA</td>
    <td align="center">EH</td>
  </tr>
  <tr>
    <td>LAO PEOPLE'S DEMOCRATIC REPUBLIC</td>
    <td align="center">LA</td>
    <td width="10"></td>
    <td>YEMEN</td>
    <td align="center">YE</td>
  </tr>
  <tr>
    <td>LATVIA</td>
    <td align="center">LV</td>
    <td width="10"></td>
    <td>YUGOSLAVIA</td>
    <td align="center">YU</td>
  </tr>
  <tr>
    <td>LEBANON</td>
    <td align="center">LB</td>
    <td width="10"></td>
    <td>ZAMBIA</td>
    <td align="center">ZM</td>
  </tr>
  <tr>
    <td>LESOTHO</td>
    <td align="center">LS</td>
    <td width="10"></td>
    <td>ZIMBABWE</td>
    <td align="center">ZW</td>
  </tr>
  <tr>
    <td>LIBERIA</td>
    <td align="center">LR</td>
    <td width="10"></td>
  </tr>
</table>

*/




/*! \defgroup security Writing secure code

TnFOX goes considerably further than any other GUI toolkit library that I know of
in allowing you to write secure code. There are four fundamental requirements for
secure code: robustness, access control, encryption and containing sensitive data.

TnFOX's extensive error handling facilities are second-to-none and allow extremely
robust code to be easily written - perhaps even better than any other portable
library for mainstream systems available.
See the section \link rollbacks
Transaction Rollbacks
\endlink
as well as the documentation for FX::FXException.

Managing access control is tough to do across systems portably as they vary so
much. Yet TnFOX's FX::FXACL and FX::FXACLEntity enable most of the native access control
security features available on both Windows NT and POSIX Unix for files (FX::QFile),
directories, named pipes (FX::QPipe) and memory mapped sections (FX::QMemMap).

Via the OpenSSL library, TnFOX also provides a range of strong encryption
facilities (FX::QSSLDevice), strong public and private key manipulation
(FX::FXSSLPKey, FX::FXSSLKey) as well as a portable high-quality entropy gatherer
(FX::Secure::Randomness) and fast cryptographically secure hashing facilities
(FX::Secure::TigerHash).

Sensitive data when on disc should be accessed via i/o devices with the \c
IO_ShredTruncate flag set as the open mode - this shreds any data truncated off
the file, thus ensuring data does not leak into the disc drive's free space.
Similarly if deleting a sensitive data file you should open it, \c truncate(0)
it, and close before deletion. You can manually shred data using FX::QIODevice::shredData().

Sensitive data when in memory should be allocated from the FX::Secure namespace
which provides its own \c new, \c delete, \c malloc() and \c free() implementations
which use a custom heap kept separately from the normal free store. This heap
is permanently locked into memory to prevent paging to virtual memory swap files
and is zeroed on process exit, no matter how that exit occurs (eg; a segmentation
fault). This is to make it harder for secure data to be retrieved
by a number of process-intrusion methods.

Needless to say, calls to these operators must be matched ie; you cannot mix and match
memory allocated within FX::Secure with calls made outside. This unfortunately
causes many compilers not entirely namespace conformant to generate bad code.

Lastly, an often overlooked issue is securely containing passwords entered by the user.
FX::FXTextField with TEXTFIELD_PASSWD is insecure as it uses an FX::FXString to
hold the password - however, it is extremely unlikely the same user entering
a password will want to retrieve it via malicious means (unless they forget it :) ).
Far more likely is that a password entered by a privileged user earlier in the
session may be retrieved maliciously later on - to avoid this, I have patched
FXTextField to zero its contents on destruction when TEXTFIELD_PASSWD is set.
This implies that if you have any dialogs taking passwords as input, you should
\b always destroy them when done - don't leave the dialog hanging around in
memory.

*/





/*! \page qtdiffs Structural differences between TnFOX and Qt
More about Qt can be found at http://www.trolltech.com/

Firstly, there exist API compatible classes for the following Qt classes:

<table>
<tr><td>FX::QBuffer (QBuffer)		<td>FX::QBlkSocket (QSocketDevice)	<td>FX::QDir (QDir)
<tr><td>FX::QFile (QFile)			<td>FX::QFileInfo (QFileInfo)	<td>FX::QHostAddress (QHostAddress)
<tr><td>FX::QIODevice (QIODevice)	<td>FX::QMutex (QMutex)		<td>FX::FXStream (QDataStream)
<tr><td>FX::FXString (QString)		<td>FX::QThread (QThread)		<td>FX::QTrans (tr())
<tr><td>FX::QWaitCondition (QWaitCondition)<td>FX::QByteArray		<td>FX::QCache
<tr><td>FX::QCacheIterator			<td>FX::QDict					<td>FX::QDictIterator
<tr><td>FX::QFileInfoList			<td>FX::QIntCache				<td>FX::QIntCacheIterator
<tr><td>FX::QIntDict				<td>FX::QIntDictIterator		<td>FX::QPtrList
<tr><td>FX::QPtrListIterator		<td>FX::QMemArray				<td>FX::QValueList
<tr><td>FX::QPtrDict				<td>FX::QPtrDictIterator		<td>FX::QPtrVector
<tr><td>FX::QPtrVectorIterator		<td>FX::QStringList				<td>FX::QValueListIterator
<tr><td>FX::QValueListConstIterator
</table>

With the above, you can port most non-GUI related Qt code with very little rewriting.
However, there are a number of structural changes to the API's which have been done
because (a) FOX constraints demanded a different approach (b) Qt's solution was
substantially below optimal and (c) Qt's approach makes multi-threaded code hard.

The first most obvious change is the complete absence of reference counting because
it is fundamentally non-thread-safe - thus there is no implicit nor explicit sharing.
This has two knock-on effects:
\li Many more parameters in TnFOX take references rather than values for efficiency
purposes and also returns tend to avoid a copy construction where possible by
reorganising internal structures.
\li A deep copy is performed each and every time a copy construction is made. This
tends to break code subtly - if you had assumed that copies referred to the same
data for explicitly shared items, then your code will need editing. Note that Qt
actually breaks several C++ conventions in doing this - a copy should always mean
a copy and passing a reference is the conventionally accepted way of semantically
referring to someone else's data as though it were yours. Lastly, and obviously,
if you make a lot of copy constructions, your performance will nosedive where
it didn't under Qt.

The second less obvious change is that FXString is currently ASCII. When normal
FOX finalises its support for unicode, I'll replace normal FX::FXString with FX::FXWString -
however, QString is currently two-byte unicode whereas FXWString will be four-byte
unicode. Most string processing using the API's won't need to know, but your code
might.

Some API's have been slightly altered, whether to provide spelling corrections,
remove irrelevent arguments etc. Generally speaking I've provided deprecated overloads
to maintain old behaviour. Any Qt API's marked as deprecated in the documentation
should be treated as such - do not use them in new code. Furthermore since the
API semantics are different, how you wrote efficient code for Qt will not always
be the same for TnFOX.

Lastly, TnFOX is fully exception-aware when Qt most certainly is not. <b>Any line
is assumed to be able to throw an exception at any time!</b> - thus again your
code may break in subtle ways - most likely being resource & memory leaks or
causing your internal data structure to become corrupted or inconsistent. Look
into TnFOX's \link rollbacks
transaction support
\endlink
which permits rollback actions to be undertaken when an error arises.

\note TnFOX was written to port a substantial Qt project written for Qt v2.x and
v3.0. Hence later API additions or revisions may not be reflected here.
*/

/*! \page foxdiffs Porting from FOX to TnFOX
TnFOX is a fork of FOX which can be found at http://www.fox-toolkit.org/

TnFOX maintains virtually the same API as FOX though there are differences
especially in the i/o and threading classes. Generally speaking, you should be able to
compile your FOX application against TnFOX without incident as those classes
which have been deprecated have thunk replacements implemented in the new code.

However, for various reasons, some semantic changes did need to happen:
\li Anything using the FOX i/o classes may break. An emulation thunking to
TnFOX's much superior replacements is good enough that FOX code itself
doesn't know any better and so neither should your code.
\li FX::FXTime is a class in TnFOX, representing a much finer time quantity
than FOX's which is only per-second granularity. This can cause some trivial
compile errors which were deliberately caused as often you must slightly
adjust your code to use FX::FXTime::as_time_t().
\li All the file metadata functions in FX::QFile return metadata for symbolic
links, not for what the link points to. This can subtly break some FOX code (but
it was necessary to add NTFS junction support).
\li FOX's threading support was added substantially after TnFOX's and are not
compatible. Through the FOX compatibility layer, a FOX compatible API is provided
which thunks through to TnFOX's threading code. Note however that thread cancellation
is not supported when using this emulation.
\li The exception types FX::FXWindowException, FX::FXImageException and FX::FXFontException
are FOR COMPATIBILITY WITH FOX ONLY. They are NOT LIKE NORMAL TNFOX EXCEPTIONS
in that they are thrown from within FOX code which is not exception safe. While
they have been marked as fatal exceptions, you may need to take action within
your code to make very sure you do not attempt recovery from these exceptions.
I did wonder about not deriving them from FX::FXException so that they'd
definitely fatally exit the process, but decided that it's probably best if they
are actually trappable.
\li You should put a <tt>FXProcess myprocess(argc, argv);</tt> directly
after your \c main() but before creating your FXApp.
\li Try to use FX::FXHandedDialog and FX::FXHandedPopup, they are so much
more user friendly. Similarly have your buttons and such observe
FX::FXWindow::userHandednessLayout(). FX::FXHandedMsgBox has replaced
FX::FXMessageBox which is marked as deprecated.
\li \c DEFAULT_PAD and \c DEFAULT_SPACING no longer exist. Their replacement
is either FX::FXWindow::defaultPadding() or FX::FXWindow::defaultSpacing().
This has been done to support small displays. You shouldn't notice the change
as there are \c DEFAULT_PAD and \c DEFAULT_SPACING macros which point to the
new code. However, you should test your code after porting to TnFOX with
the -fxscreenscale=25% argument.
\li All friend functions of FX::FXVecXX classes have had 'vec' prefixed before
them. This was primarily to aid the python bindings where the names were
clashing with python ones, but I think it is more clear anyway.
\li FX::FXApp has had its event loop dispatch code split off into
FX::FXEventLoop and thus some things are no longer available through FXApp.
This in practice should require no source changes to your code if you are
not using per-thread event loops.
\li FX::FXId, and thus all derivative classes (eg; windows) now maintain
what event loop it was created within.
\li As TnFOX permits multiple window trees to run across multiple threads,
you must not use static variables to store state in your GUI widgets unless
it's read-only. Use FX::FXEventLoop_Static instead like as follows:
\code
// static MyObj var;
static FXEventLoop_Static<MyObj> pvar;
if(!pvar) pvar=new MyObj;
MyObj &var=*pvar;
\endcode
\li FXApp::reg() is synchronised for you as FX::FXSettings locks FXApp
during its operation. However as it can return <tt>const char *</tt>,
there is the possibility that one thread deletes an item just after another
thread fetches it and so the pointer is left dangling. If this is a
possibility you must lock FXApp yourself until you are done with the string.
\li If you wish to run multiple event loops, you must initialise
a FX::TnFXApp instead of a FX::FXApp in your code and change your \c main()
slightly. See the docs for FX::TnFXApp.
\li Lastly, the TnFOX extensions are exception aware whereas none of FOX is.
Therefore, if you call \b any TnFOX code where a path exists up the call
stack going through FOX code (eg; a GUI event handler) then you must surround
that code with the FXEXCEPTION_FOXCALLING1 and FXEXCEPTION_FOXCALLING2 macros.
These trap any exceptions thrown and show a FX::FXExceptionDialog.

Summary of what is not supported from FOX v1.6.x:
\li Some FX::FXStream methods. You'll never normally notice these
\li The application wide mutex. It's a bad idea anyway.

You should also see \ref BuildingNoFOXCompatDiffs
*/

/*! \defgroup generic Generic Tools in TnFOX

I failed a job interview at Trolltech for not knowing anything about compile-time
introspection in C++, so you can bet your arse it was the first thing I set
about fixing. And within a few hours of surfing the web and reading about it,
I was hooked!

What I think especially impressed me was <a
href="http://www.cuj.com/experts/1910/alexandr.htm">Andrei Alexandrescu's article
on improving copies</a>. I like most programmers had thought this was a done
deal and we'd got as good as we got - but as I'm really an assembler programmer
at heart, I knew C++ was very suboptimal especially with regard to temporaries.
From that point onwards I resolved to learn this skill.

The generic tools provided by \c FXGenericTools.h provide the following facilities:
\li Do/Undo, whereby arbitrary functions can be called on entry and exit of a
scope
\li A number of simple compile-time metalanguage constructs such as if, equals,
true, false, is convertible etc.
\li An enhanced type information container which works across shared object
boundaries on GCC and provides portable symbol demangling
\li A policy-based smart pointer
\li Typelists with simple operations like append, find, remove etc. plus more
complex operations like instantiate via third-party template
\li Extensive type traits, including user-definable ones
\li Function & member function traits
\li Generic functors, bound functors plus helper functions to create bindings

<h3>Caveats:</h3>
Unfortunately, these come with a price - you will need a compiler supporting
partial template specialisation plus Koenig lookup. That means MSVC7.1 or higher and GCC v3.4 or
higher - with other compilers, your milage may vary. I've heard Intel's C++ compiler
v7.x works fine too. If your compiler is not up to scratch, I did design
the generic tools to be redirectable to a much more comprehensive library eg;
Boost (http://www.boost.org/).

Speaking of Boost, TnFOX's compile-time metaprogramming tools are only fairly
comprehensive. I've aimed merely to provide the most common and useful
facilities which can do 95% of day to day stuff. If you want to do anything
more complicated than this, I \em strongly recommend Boost which should be
able to convert TnFOX's typelists to its own pretty easily.

For lots more on compile-time metaprogramming (especially if you want to
learn it), please consult the book "Modern C++ Design" by Andrei Alexandrescu.
*/



/*! \page debugging Debugging facilities in TnFOX

TnFOX comes with a number of inbuilt debugging and stress-testing facilities:
\li FX::FXException::setGlobalErrorCreationCount() allows you to have random
exceptions fire, thus letting you test that the stack unwinds correctly.
\li FX::QMutex::setMutexDebugYield() allows you to have all mutex locks
immediately yield the processor, thus helping any code altering data without
holding a suitable lock to become more evident.
\li FX::FXProcess::overrideFreeResources() lets you override memory, processor
and disc i/o full indicators so you can test operation in memory low situations.

On MSVC only:
\li FXMemDbg.h lets you list all memory leaks and where they were allocated.
Use valgrind instead on Linux.
*/



/*! \page modularbuilding Modular building of TnFOX

In config.py, there are a series of variables you can set to exclude certain
portions of TnFOX. Remember you can exclude the entire GUI using \c disableGUI:

\subsection configDisableGL disableGL:
\li FX::FXGLCanvas
\li FX::FXGLCone
\li FX::FXGLContext
\li FX::FXGLCube
\li FX::FXGLCylinder
\li FX::FXGLGroup
\li FX::FXGLLine
\li FX::FXGLObject
\li FX::FXGLPoint
\li FX::FXGLShape
\li FX::FXGLSphere
\li FX::FXGLTriangleMesh
\li FX::FXGLViewer
\li FX::FXGLVisual

\subsection configDisableFileDirDialogs disableFileDirDialogs (if False, sets disableMenus to False):
\li FX::FXDirBox
\li FX::FXDirDialog
\li FX::FXDirList
\li FX::FXDirSelector
\li FX::FXDriveBox
\li FX::FXFileDialog
\li FX::FXFileList
\li FX::FXFileSelector

\subsection configDisablePrintDialogs disablePrintDialogs:
\li FX::FXPrintDialog

\subsection configDisableFindReplaceDialogs disableFindReplaceDialogs:
\li FX::FXReplaceDialog
\li FX::FXSearchDialog

\subsection configDisableMenus disableMenus:
\li FX::FXMenuBar
\li FX::FXMenuCaption
\li FX::FXMenuCascade
\li FX::FXMenuCheck
\li FX::FXMenuCommand
\li FX::FXMenuRadio
\li FX::FXMenuSeparator
\li FX::FXMenuTitle
\li FX::FXScrollPane

But NOT FX::FXMenuButton nor FX::FXMenuPane

\subsection configDisableMDI disableMDI:
\li FX::FXMDIChild
\li FX::FXMDIClient
\li FX::FXMDIDeleteButton
\li FX::FXMDIMaximizeButton
\li FX::FXMDIMenu
\li FX::FXMDIMinimizeButton
\li FX::FXMDIRestoreButton
\li FX::FXMDIWindowButton

\subsection configDisableSQL disableSQL:
\li FX::TnFXSQLDB
\li FX::TnFXSQLDB_ipc
\li FX::TnFXSQLDB_sqlite3

Also the embedded copy of SQLite3 is not compiled in.

*/


/*! \page cpp0xsupport C++0x Support in TnFOX

\note v0.88 could not get move constructors working properly in GCC v4.3 and
so this particular feature has been disabled for now.

For a very long time now, TnFOX has contained support for one particular C++0x
feature (move constructors) but the code had always been \c #ifdef-ed out. As
of v0.88 of TnFOX, GCC v4.3 finally has some very limited support for C++0x
features and thus finally we can enable support. This is done by setting the
\c enableCPP0xFeatures variable in \c config.py to \c True.

Changing this is a profound setting - it enables many new coding paradigms.
Almost all existing code is forward compatible, but some is not. In particular,
code which uses one of the new keywords as a variable name will cease to compile
and additionally some constructs will need reworking to function correctly.

However, the benefits are many and huge. TnFOX has implemented move semantics
for quite a few classes where it made sense since its inception, but it has always
been a nasty hack which technically broke the ISO C++ coding standards - the fact
it worked at all was more luck than design though of course it was and is a very
common hack. MSVC had particular support through allowing non-const constructors
which had to be implemented in GCC by deconsting the copy constructor parameter
(always praying that the optimiser wouldn't remove the move construst). Enabling
the C++0x semantics finally fixes this, and furthermore lets the optimiser know
what's going on which should help greatly - in particular, we finally have proper
syntax support for move-only objects and the compiler will correctly barf if you
try something stupid. The move semantics in TnFOX always
operated on a pointer, so speed increases in TnFOX only code will be nil - however,
all the STL containers now have move support which makes all the Qt thunk
emulations considerably quicker when working with value items. You need of course
to add your own rvalue copy constructors in order to avail of move semantics -
the STL and Qt thunk classes will default to normal lvalue copy constructors.
 
As of v0.88 of TnFOX, the following C++0x features have been implemented. This
list is limited by what the latest GCC has implemented:
<ol>
<li>Move constructors using rvalue references (N2118) have been implemented in
most of the class objects which might end up in a container have move
constructors where possible. All Qt thunk classes and anything which can act
as a container will use move construction where possible.
<li>FXSTATIC_ASSERT() will use the new \c static_assert() function (N1720).
This greatly improves error message reporting.
<li>FX::FXException will use the new \c __func__ meta-variable (N2340) in error
reporting.
</ol>
I would just love to add template typedefs and variadic template arguments, and
they will come eventually outside rvalue support. For now, enjoy the performance
increases and let me know if I have broken anything!
*/




/*! \page applemacosnotes Apple MacOS X specific notes

This covers the rather special Unix variant that is Apple MacOS X. You'll need
XCode installed along with the Apple X11 implementation plus at least scons
v0.96.1 (which added MacOS X support). v0.88.1 was tested on 10.5.4, XCode 3.1.1
and scons v0.96.90 (the v1.x series won't finish the build on Macs for some odd reason).

This page doesn't duplicate what's already said in \ref unixnotes so you should
also read that. The very good news is that 10.5 is much improved over 10.4 thanks
to much improved POSIX compatibility.

\section supported Supported configuration:
TnFOX was originally developed against Apple MacOS X v10.4.6 and XCode v2.3 on Intel x86
only. The PowerPC build has however been tested on x86 via Rosetta and appears to
work just fine, but has not been extensively tested.

You have two choices when building TnFOX on Apples - the first is the same as on
other platforms where you set \c x86 or \c x64, however if you choose x86 then you
MUST set architecture_version=7, x86_SSE=2 and x86_3dnow=0 in config.py as the Apple
GCC won't output valid code for other architecture configurations.

The other choice is to specify one of the special architectures \c macosx-ppc/macosx-i386.
If you want to build a Universal binary, then you must use these. Firstly you build one,
then change the config and build the other so you get two full builds - one PowerPC, the
other i386. This is necessary because the endian is different for each.

You must then merge the two binaries into a Universal one using Apple's \c libtool:
\code
Merging code to be added later
\endcode

\section config Directory configuration:
Unlike most Unices, MacOS X is missing libraries for JPEG, PNG and TIFF. Ideally you should
install macports or fink and have these added - I personally had to manually hack
them into \c /usr/local because scons wouldn't find them anywhere else. Failing this,
you can download a \c /usr/local tree containing these from the TnFOX homepage after which
these libraries will be found and used.

\section problems Problems with 10.4.x:
\li Apple's X11 implementation provides Xft support, but will return no fonts available.
Run 'sudo fc-cache' to fix this problem.
\li MacOS X 10.4 has shitty support for thread cancellation. It is inconsistent, unreliable,
and generally downright buggy. In particular, it does not support thread cancellation
during select(). This is a major problem for TnFOX as the POSIX threads spec says it
should be, so TnFOX emulates correct behaviour using an internal per-thread pipe to
signal cancellation. You shouldn't notice any difference generally speaking.
\li You MUST define \c _APPLE_C_SOURCE when compiling any code using TnFOX. This
causes alternative (and improved) implementations of many C library functions to
be used instead of the defaults. Failing to do this will cause random crashes and
other weird behaviour.
\li TestDLL fails on MacOS X due to \c dlclose() not kicking out a shared library like
it's supposed to do.

\section problems2 Problems with both 10.4 and 10.5:
\li MacOS X does not come with a FAM implementation, so the BSD fallback of kqueues
is used. These unfortunately have limitations - see the doc page for FX::FXFSMonitor.
In particular, they won't monitor non-HFS partitions, so if you try running TestFSMonitor
from a FAT partition, it will fail.
\li TestGraphing fails on MacOS X due to buggy support within the X11 OpenGL routines.
Specifically, because TnFOX sets \c XInitThreads(), the OpenGL code hangs in the internally
called \c XLockDisplay(). If you really need OpenGL working, you will have to comment
out this call in \c FXApp.cpp and of course lose multithreaded GUI support.
 
 */

/*! \page windowsnotes Windows-specific notes

This covers Windows 2000, Windows XP and Windows Vista, both 32 bit and 64 bit editions. Windows 95, 98
and ME are not supported due to insufficient host OS facilities. Windows NT should be mostly
compatible - though there are quite a few unsupported calls.

Note that the default binaries of v0.88 of TnFOX on Windows incorporated into itself the following
libraries:
\li libbzip2 v1.0.5
\li libjpeg v6b
\li libpng v1.2.29
\li libtiff v3.8.2
\li zlib v1.2.3
\li openssl v0.9.8g

\section supported Supported configuration:
<h3>Win32</h3>
MSVC6 is \b not supported, nor is MSVC7.0 (Visual Studio .NET). You need MSVC7.1 minimum
(Visual Studio .NET 2003) and MSVC8 as well as MSVC9 work well. GCC v3.2.2 to v4.3 is known
to compile working binaries on Linux but is untested on Windows. The Digital Mars compiler wasn't
up to the job last summer 2004. Intel's C++ compiler v8 for Windows works fine, though its config
is very out of date nowadays.

<h3>Win64</h3>
The only official compiler supporting x64 is MSVC8 (Visual Studio 2005) and MSVC9 so this is what
I describe. You \em can make it work with beta compilers from the Platform SDK, but you NEED at least
the MSVC7.1 STL (rather than the MSVC6 STL which comes with those test compilers) which is hard to get.
Chances are that Intel's C++ compiler as well as GCC should produce working x64 code.

<h3>The Quick & Easy method</h3>
<ol>
<li>Unpack the TnFOX distribution somewhere and add desired third party libraries (see below). Modify
\c config.py as desired - note that by default, config.py will detect whether your command box is a
32 bit or 64 bit binary and will build for that unless specifically overridden.
<li>Install a recent Python (http://www.python.org/)
<li>Install a recent SCons (http://www.scons.org/)
<li>Make sure the PATH environment variable points at the python directory such that you can
run scons from within a command box. Try the My Computer=>Properties=>Advanced=>Environment Variables.
<li>For MSVC8 and later, there will be Start Menu options under Visual Studio Tools which set
up a command box for either the x86 or x64 toolset (eg; Visual Studio 2008 x64 Win64 Command Prompt).
<li>Type \c scons \c msvcproj which will auto-detect your MSVC installation and generate suitable
MSVC project files which will invoke scons for you.
</ol>
As both FOX and TnFOX use very low-level and old API's in Windows, no libraries above those
supplied by default with the system are required. It is recommended you have the latest
service pack installed however.

\section config Directory configuration:
TnFOX demands a rather special configuration of the directories around it on Windows.
As there's no system-provided place for header files and such to live, the SConstruct
file manually looks in the directory in which the TnFOX directory is placed for the
<a href="http://www.boost.org/">Boost library</a> and the <a
href="http://www.openssl.org/">OpenSSL library</a>. The standard system-provided libraries on Unix
must be placed in the \c TnFOX/windows directory - these include the ZLib library (as
\c zlib), the graphics libraries for JPEG (as \c libjpeg), PNG (as \c libpng), TIFF
(as \c libtiff) and BZip2 (as \c libbzip2). In all cases any version information is not used so openssl-0.9.8
must be renamed to simply "openssl". Note that all libraries are linked in \b statically
so don't go building DLL versions! (eg; use \c nt.mak instead of \c ntdll.mak when building
OpenSSL). If this seems like hassle, once you've done it you can forget about it thereafter.

If any library cannot be found, support for it is disabled automatically. You should bear
in mind that after compiling using the enclosed project files where necessary, you should
move the library to the root of its project and append either a 32 or 64 to the filename
as appropriate so that scons can find the correct one.

\section allocator The standard Windows dynamic memory allocator:
TnFOX replaces the standard Windows dynamic memory allocator provided by MSVC's
CRT library (really a thin wrapper around the Win32/64 \c HeapCreate() family) with its
own based on FX::FXMemoryPool when the build mode is release. This is done because quite
frankly, the Win32 standard allocator is crap - especially on systems with more than one
processor. Admittedly it's nothing like as crap as the allocators used to be on older
MSVC's (especially the famous one where \c realloc() in the debug library corrupted your
memory), but when compared to \c ptmalloc2 (the glibc standard allocator) it sucks major
monkey balls.

\note Since v0.80, the allocator is replaced within any DLL or EXE which includes a TnFOX
header file. This comes with certain caveats, see the documentation for \link fxmemoryops
that particular feature
\endlink

\section builds Release builds:
I've incorporated a stack backtracer into FX::FXException which decodes the stack
at the point of exception so that it can be reported to the user in order to locate
precisely what was calling what when the exception occurred.
This depends on having debug symbols available to it, so the build
process is slightly more complicated - when building, you enable Line number debug
info only with full optimisation enabled. This produces a relatively small PDB file
with no more than what FXException requires.

All DLL's are prebound for during the build process to speed loading times plus
I made a number of lesser used DLL's for Win32 release builds delay loaded (ie;
loaded only when they are first needed). This should lower both load times (I've set a
custom load address too) and run-time memory usage for the majority of cases. Note
that prebinding only speeds loading when loading onto your local system (your application's
installer should do this).

TnFOX is able to use >2Gb address spaces and so has the \c IMAGE_FILE_LARGE_ADDRESS_AWARE
bit set in its executable header. You should set the same bit in your applications
if you are also able to handle such addresses (ie; you don't use signed pointer comparisons).
While you never see the advantage of this on most 32 bit Windows, you do when running
in an emulated 32 bit environment on Windows x64. You can produce binaries customised
for your processor by adjusting the \c config.py file.

TnFOX is also no-execute compatible, meaning that its stacks can be marked as no-execute.
This is specified to the MSVC linker (where supported) using the \c /NXCOMPAT switch.

The only thing remaining which shall be fixed later is working set optimisation (WSO)
which I can't do until I have a serious client application for TnFOX to profile against.
But don't worry, it'll come!

*/

/*! \page unixnotes Unix-specific notes

This covers Linux, BSD and partially Apple MacOS X for a minimum of x86, x64 and PowerPC
architectures (but likely covers all GCC supported architectures).

\section supported Supported configuration:
TnFOX was developed against:
\li An x86 RedHat 9 (2.4 kernel) installation with GCC v3.2. As of v0.85 this is no longer
tested, but there is no reason why it shouldn't continue to work.
\li An x64 KUbuntu Linux Hardy Heron (2.6 kernel) installation with GCC v4.3.
\li An x86 FreeBSD v6.0 installation with GCC v4.0.2 (not tested since v0.86).

GCC's as early as v3.2.2 should also work as should Intel's C++ compiler for Linux v8
or later.

Everything must have been built with threads enabled (X11, glibc, python). This
is default on recent Linuces, it may not be so with your installation.

On FreeBSD (last tested some time ago), the defaults aren't quite enough to compile out of the box. The generic
kernel has the \c /proc filing system support compiled in but it isn't mounted by default -
add "proc /proc procfs rw 0 0" to your \c /etc/fstab. You will also need to install
the \c libtool package at the very least. Furthermore I found that GCC wasn't
configured properly on some systems, it should include \c /usr/local/include and
\c /usr/local/lib which it does not and lastly the python environment variables are
missing which causes CppMunge.py to fail.

To fix GCC, the easiest is to set \c CPATH and \c LIBRARY_PATH (though you could
recompile it). Doing this also fixes many other compilation errors in GNU programs
so generally it's a good idea. I did this on my installation by adding to \c /etc/profile:
\code
setvar CPATH /usr/local/include
export CPATH
setvar LIBRARY_PATH /usr/local/lib
export LIBRARY_PATH
setvar PYTHON_INCLUDE /usr/local/include/python2.3
export PYTHON_INCLUDE
setvar PYTHON_ROOT /usr/local/lib/python2.3
export PYTHON_ROOT
setvar PYTHON_LIB /usr/local/lib/python2.3/config/libpython2.3.so
export PYTHON_LIB
setvar PYTHON_VERSION 2.3
export PYTHON_VERSION
\endcode
On versions earlier than FreeBSD v5.3, you may need to use \c libpython2.3.a instead.
Lastly you should probably review the FAQ entry \ref freebsdqs

\section config Directory configuration:
The graphics libraries for JPEG, PNG and TIFF plus zlib should be installed by
default on almost every Unix installation. scons finds these automatically - all
you need to do is make sure the headers are there by installing the \c -dev versions
where necessary.

Most systems already have OpenSSL installed - if not, go to <a href="http://www.openssl.org/">
its website</a> and install yourself a copy.

<a href="http://www.boost.org/">Boost</a> should be placed in a directory called
"boost" next (not in) to the TnFOX directory just like on Windows. scons will
find it automatically if placed there. You \b CANNOT use any system provided
version of Boost as it needs patching to enable thread support in Boost.Python.

\section xim X11 Input Methods (XIM) and X Threads
It was discovered during testing of v0.6 that pressing any key at all with X11
reentrancy enabled causes XIM to hang (this was a bug in all prior TnFOX versions
which shows how little I tested then!). According to qt-interest, this problem
afflicts Qt as well and the only known solution is to disable one or the other.
I chose XIM here which is sad as it costs input flexibility. Hopefully future
versions of X11 will fix the problem (it's a simple "trying to hold the lock
more than once" deadlock) and we can restore support.

\section builds Release builds:
I've written a custom builder for scons (in the \c sconslib.py file) which generates
a libtool format output which can be used just like a genuine libtool library. This
should greatly simplify issues for everyone, though there are some reported issues
with scons on MacOS X.

You can produce binaries customised for your processor by adjusting the \c config.py
file.
*/








/*! \defgroup categorised Categorised list of TnFOX classes

*/

/*! \defgroup guiclasses GUI classes
\ingroup categorised

<ol>
<li><b>High-level:</b>
  <ol>
  <li>FX::TnFXApp, the per-process application object
  <li>FX::FXEventLoop, the per-thread event dispatcher

  <li>FX::FXTopWindow, a top-level user interface window
    <ol>
    <li>One of many dialog boxes based on FX::FXDialogBox listed below
    <li>FX::FXMainWindow, a main window in your application
	<li>FX::FXSplashWindow, a temporary window shown during startup of your application
    <li>FX::FXToolBarShell, contains a number of toolbar items (see operational below)
    </ol>
  <li>FX::FXMenuPane, a popup menu window hovering above all others
  </ol>
<li><b>Representation:</b>
  <ol>
  <li>FX::FX7Segment, displays a number as a digital display
  <li>FX::FXBitmapFrame, displays a third-party bitmap
  <li>FX::FXBitmapView, displays a third-party bitmap in a scrollable box
  <li>FX::FXCanvas, displays an image drawn into by other code
  <li>FX::FXImageFrame, displays a third-party image
  <li>FX::FXImageView, displays a third-party image in a scrollable box
  <li>FX::FXLabel, displays some text and/or an icon
  <li>FX::FXProgressBar, displays a percentage via a completion bar
  <li>FX::FXRuler, displays a ruler showing distance
  <li>FX::FXStatusBar, a bar displaying a status message
  <li>FX::FXStatusLine, displays a status message
  <li>FX::FXToolTip, a floating yellow box showing the user something useful

  <li>FX::FXIconList, a list of icons with optional captions
  <li>FX::FXFoldingList, a collapsable list of items
  <li>FX::FXList, a list of items
  <li>FX::FXTable, a spreadsheet-like table which can be optionally edited
  <li>FX::FXTreeList, a list of items organised in a tree

  <li>FX::FXDirList, a list of directories
  <li>FX::FXFileList, a list of files
  </ol>

<li><b>Operation:</b>
  <ol>
  <li>FX::FXButton, a pressable button containing some text and/or an icon
    <ol>
    <li>FX::FXArrowButton, a button containing an arrow
    <li>FX::FXCheckButton, a tri-state button (on, off, and neither)
    <li>FX::FXPrimaryButton, a master button in a dialog
    </ol>
  <li>FX::FXDockSite, a place where toolbars and such can be docked
  <li>FX::FXMenuBar, a menu bar at the top of a top-level window
  <li>FX::FXMenuButton, a button opening a popup menu
  <li>FX::FXMenuCaption, an item in a menu
  <li>FX::FXMenuCascade
  <li>FX::FXMenuCheck, a tick item in a menu
  <li>FX::FXMenuRadio, a radio item in a menu
  <li>FX::FXMenuSeparator, a separator line in a menu
  <li>FX::FXMenuTitle, a submenu within a menu bar
  <li>FX::FXOption, a button opening an option menu
  <li>FX::FXPopup, base class for a window which appears only temporarily
  <li>FX::FXScrollArea, a region which adds scroll bars if needed
  <li>FX::FXScrollBar, a scroll bar
  <li>FX::FXScrollCorner, a corner between scroll bars
  <li>FX::FXScrollWindow, a window with scroll bars
  <li>FX::FXSeparator, a separator line
    <ol>
    <li>FX::FXHorizontalSeparator, a horizontal separator line
    <li>FX::FXVerticalSeparator, a vertical separator line
    </ol>
  <li>FX::FXShutter, an animated chooser of other widgets
  <li>FX::FXToggleButton, a two-state on/off toggle button
  <li>FX::FXToolBarGrip, the thing the user redocks a toolbar with
  <li>FX::FXToolBarShell, an undocked toolbar
  <li>FX::FXToolBarTab, a pressable item which toggles its sibling widget's appearance
  <li>FX::FXTriStateButton, like a FX::FXToggleButton but can have three states

  <li>FX::FXMDIClient, filters messages for MDI child windows
  <li>FX::FXMDIDeleteButton
  <li>FX::FXMDIMaximizeButton
  <li>FX::FXMDIMenu, MDI child window menu
  <li>FX::FXMDIMinimizeButton	
  <li>FX::FXMDIRestoreButton
  <li>FX::FXMDIWindowButton
  </ol>

<li><b>Choosables:</b>
  <ol>
  <li>FX::FXTextField, edits one line of text
  <li>FX::FXText, edits multiple lines of text
  <li>FX::FXMenuPane, a popup list of selectable menu options

  <li>FX::FXChoiceBox, chooses an option from a drop-down dialog box
  <li>FX::FXComboBox, chooses from a drop down menu
  <li>FX::FXDial, chooses a number by rotation of a dial
  <li>FX::FXFontSelector, chooses a font
  <li>FX::FXKnob, chooses a number via a rotatable knob
  <li>FX::FXListBox, chooses from a drop down list
  <li>FX::FXOptionMenu, chooses from a drop down menu of options
  <li>FX::FXPicker, chooses an arbitrary location on the screen
  <li>FX::FXSpinner, chooses an integer number
  <li>FX::FXRealSpinner, chooses a floating point number
  <li>FX::FXSlider, chooses a number by movement of a slider
  <li>FX::FXRealSlider, chooses a floating point number by movement of a slider
  <li>FX::FXTreeListBox, chooses from a tree organised drop down list

  <li>FX::FXColorSelector, chooses a colour
  <li>FX::FXColorList, chooses a colour from a list
  <li>FX::FXColorRing, chooses a colour using an intuitive ring & triangle device
  <li>FX::FXColorBar, chooses by slider a colour by HSV
  <li>FX::FXColorWell, chooses from a list of settable colours
  <li>FX::FXColorWheel, chooses a colour by HSV from a disc
  <li>FX::FXGradientBar, chooses the gradient of a colour

  <li>FX::FXDirBox, chooses from a tree organised list of directories
  <li>FX::FXDirSelector, chooses a directory
  <li>FX::FXDriveBox, chooses from a list of drives
  <li>FX::FXFileSelector, chooses a file
  </ol>

<li><b>Dialogs:</b>
  <ol>
  <li>FX::FXDialogBox, base class for all FOX dialogs
  <li>FX::FXHandedDialog, base class for all TnFOX dialogs

  <li>FX::FXColorDialog, asks the user for a colour
  <li>FX::FXDirDialog, asks the user for a directory
  <li>FX::FXExceptionDialog, reports an FX::FXException to the user
  <li>FX::FXFileDialog, asks the user for a file
  <li>FX::FXFontDialog, asks the user for a font
  <li>FX::FXHandedMsgBox, reports a message to the user
  <li>FX::FXInputDialog, asks the user for a simple value
  <li>FX::FXMessageBox, reports a simple message to the user
  <li>FX::FXPrintDialog, asks the user for a printer
  <li>FX::FXProgressDialog, reports the progress of an operation to the user
  <li>FX::FXWizard, takes the user through a sequence of steps.

  <li>FX::FXReplaceDialog, asks the user for text to replace
  <li>FX::FXSearchDialog, asks the user for text to search for
  </ol>
</ol>
*/

/*! \defgroup layoutclasses Layout classes
\ingroup categorised

<ol>
<li>FX::FX4Splitter, splits its children into quarters of its area
<li>FX::FXGroupBox, places a border and title around its children
<li>FX::FXHorizontalFrame, orders its children horizontally
<li>FX::FXMatrix, places its children in a grid
<li>FX::FXMDIChild, maintains its children within a "work area"
<li>FX::FXPacker, places its children against the sides
<li>FX::FXRulerView, manages its children whilst showing a ruler
<li>FX::FXSplitter, splits its children into two areas
<li>FX::FXSpring, allows springy distance between siblings
<li>FX::FXSwitcher, places its children all on top of each other (thus
you can bring arbitrary chilren to the front to display them).
<li>FX::FXTabBar, orders its children as tab items side by side
<li>FX::FXTabBook, orders its children as tab items depending on if they
are layout or non-layout objects
<li>FX::FXToolBar, places its children inside a dockable toolbar box
<li>FX::FXVerticalFrame, orders its children vertically
</ol>
*/

/*! \defgroup resourceclasses Resource classes
\ingroup categorised

<ol>
<li>FX::FXDrawable, the base class of all things which can be displayed
on the screen.

<li>FX::FXBitmap, contains a black & white (1 bit) image
<li>FX::FXCursor, contains a mouse cursor in ARGB format
  <ol>
  <li>FX::FXCURCursor, contains a mouse cursor in Windows CUR format
  <li>FX::FXGIFCursor, contains a mouse cursor in GIF format
  </ol>
<li>FX::FXFont, a typeface in a certain typeface and size. FX::FXFontDesc
describes such a font.
<li>FX::FXIcon, contains a non-rectangular image which can be greyed out
  <ol>
  <li>FX::FXBMPIcon, contains an icon in Windows BMP format
  <li>FX::FXGIFIcon, contains an icon in GIF format
  <li>FX::FXICOIcon, contains an icon in Windows ICO format
  <li>FX::FXIFFIcon, contains an icon in EA/Amiga format
  <li>FX::FXJPGIcon, contains an icon in JPEG format
  <li>FX::FXPCXIcon, contains an icon in PCX format
  <li>FX::FXPNGIcon, contains an icon in PNG format
  <li>FX::FXPPMIcon, contains an icon in PPM format
  <li>FX::FXRASIcon, contains an icon in SUN Raster format
  <li>FX::FXRGBIcon, contains an icon in raw RGB format
  <li>FX::FXTGAIcon, contains an icon in TGA format
  <li>FX::FXTIFIcon, contains an icon in TIFF format
  <li>FX::FXXBMIcon, contains an icon in XBM format
  <li>FX::FXXPMIcon, contains an icon in XPM format
  </ol>
<li>FX::FXImage, contains an image
  <ol>
  <li>FX::FXBMPImage, contains an image in Windows BMP format
  <li>FX::FXGIFImage, contains an image in GIF format
  <li>FX::FXICOImage, contains an image in Windows ICO format
  <li>FX::FXIFFImage, contains an image in EA/Amiga format
  <li>FX::FXJPGImage, contains an image in JPEG format
  <li>FX::FXPCXImage, contains an image in PCX format
  <li>FX::FXPNGImage, contains an image in PNG format
  <li>FX::FXPPMImage, contains an image in PPM format
  <li>FX::FXRASImage, contains an image in SUN Raster format
  <li>FX::FXRGBImage, contains an image in raw RGB format
  <li>FX::FXTGAImage, contains an image in TGA format
  <li>FX::FXTIFImage, contains an image in TIFF format
  <li>FX::FXXBMImage, contains an image in XBM format
  <li>FX::FXXPMImage, contains an image in XPM format
  </ol>
</ol>
*/

/*! \defgroup operationalclasses Operational classes
\ingroup categorised

<ol>
<li>FX::FXAccelTable, maps key combinations to targets & messages
<li>FX::FXCommand, a command and undoable action
<li>FX::FXCommandGroup, a group of FX::FXCommand
<li>FX::FXComposeContext, used to permit use of input methods
<li>FX::FXMenuCommand, a command in a menu
<li>FX::FXUndoList, a list of undoable actions

<li>FX::FXDataTarget & FX::FXDataTargetI, a messaging target directly managing
the value of a variable eg; numbers, colours, strings etc.
<li>FX::FXDebugTarget, a messaging target which prints out every message it receives.
<li>FX::FXDelegator, a messaging target which forwards to another target. Useful
for connecting many things to and you can change all their connections with one change.
<li>FX::FXFunctorTarget, a messaging target calling a FX::Generic::Functor
<li>FX::FXObject, the base class for all FOX objects which can receive messages
<li>FX::FXMetaClass, contains information about a FX::FXObject

<li>FX::FXDCWindow, allows drawing graphics output into a FX::FXDrawable
<li>FX::FXDCPrint, allows drawing graphics output in Postscript format (suitable for a printer)
<li>FX::FXPrinter, describes a printer

<li>FX::FXEvent, an event within the GUI
<li>FX::FXEventLoop, the base class of an event dispatch loop
<li>FX::FXEventLoop_Static, a per-event loop static variable

<li>FX::FXDate, a gregorian date container
<li>FX::FXTime, microsecond accurate locale-capable time

<li>FX::FXRex, regular expression support
<li>FX::FXExpression, mathematical expression parser

<li>FX::FXACL, an access control list for a securable entity
<li>FX::FXACLEntity, an entity who can have permissions set for them
<li>FX::FXSSLKey, a symmetric encryption key
<li>FX::FXSSLPKey, an asymmetric encryption key (public/private pair)

<li>FX::FXException, an error within TnFOX
<li>FX::FXMemoryPool, a pool of dynamically allocable memory
<li>FX::FXProcess, provides assorted information about the running process
<li>FX::FXRegistry, either the Windows or FOX registry
<li>FX::FXStream, endian-neutral data (de)serialiser
<li>FX::QTrans, automatic human language translation

<li>FX::FXIPCChannel, an inter process communication channel
<li>FX::FXIPCMsg, an inter process communication message
<li>FX::FXIPCMsgChunk, an inter process message block
<li>FX::FXIPCMsgRegistry, an inter process message namespace

<li>FX::QDir, provides detailed information about a directory
<li>FX::FXFileAssoc, registers information about a file extension (eg; icon, MIME type)
<li>FX::QFileInfo, provides detailed information about a file
<li>FX::FXFSMonitor, monitors a path for changes

<li>FX::QHostAddress, an IPv4 or IPv6 address
<li>FX::FXNetwork, miscellaneous networking facilities such as name lookups

<li>FX::QThread, a thread of execution
<li>FX::QMutex, a mutual exclusion object
<li>FX::QMtxHold, a holder of a FX::QMutex
<li>FX::QWaitCondition, an OS/2 style event object
<li>FX::FXZeroedWait, an event which signals when its count becomes zero
<li>FX::QThreadPool, a pool of worker threads

<li>FX::TnFXSQLDB, a generic SQL database accessor
<li>FX::TnFXSQLDB_sqlite3, a driver for SQLite3 databases

<li>FX::QBZip2Device, accesses bzip2 compressed files
<li>FX::QGZipDevice, accesses zlib compressed files

<li>FX::FXPython, miscellaneous python-related facilities
<li>FX::FXPythonInterp, a python interpreter instance
<li>FX::FXPythonException, an error caused by executing python code
<li>FX::FXCodeToPythonCode, provides dynamic unique C++ code points

<li>FX::FXWinShellLink, a portable reader & writer of Windows Shell link files
<li>FX::FXWinJunctionPoint, a portable reader & writer of NTFS junction points
</ol>
*/

/*! \defgroup convenclasses Convenience classes
\ingroup categorised

<ol>
<li>FX::FXArc, holds an arc (part of an ellipse)
<li>FX::FXPoint, holds a 2d point
<li>FX::FXRegion, a 2d box
<li>FX::FXRectangle, holds a rectangle
<li>FX::FXSegment, a line segment
<li>FX::FXSize, a 2d size
<li>Single precision:
  <ol>
  <li>FX::FXExtentf, a 2d box
  <li>FX::FXMat3f, a 3x3 matrix
  <li>FX::FXMat4f, a 4x4 matrix
  <li>FX::FXRangef, a 3d box
  <li>FX::FXSpheref, a sphere
  <li>FX::FXVec2f, a 2 element vector
  <li>FX::FXVec3f, a 3 element vector
  <li>FX::FXVec4f, a 4 element vector
  </ol>
<li>Double precision:
  <ol>
  <li>FX::FXExtentd, a 2d box
  <li>FX::FXMat3d, a 3x3 matrix
  <li>FX::FXMat4d, a 4x4 matrix
  <li>FX::FXRanged, a 3d box
  <li>FX::FXSphered, a sphere
  <li>FX::FXVec2d, a 2 element vector
  <li>FX::FXVec3d, a 3 element vector
  <li>FX::FXVec4d, a 4 element vector
  </ol>
<li>FX::FXString, a string container
</ol>
*/

/*! \defgroup containclasses Container classes
\ingroup categorised

\note It is recommended you use the QTL over these as they provide
superior type-safety and interoperability with other code. However
if working with FOX, you will need these.

<ol>
<li>FX::FXDict, a hash-based lookup container
<li>FX::FXFileDict, a hash-based lookup container of FX::FXFile's
<li>FX::FXHash, a hash-based lookup
<li>FX::FXObjectList & FX::FXObjectListOf, a list of FX::FXObject's
</ol>
*/

/*! \defgroup QTL Qt Template Library
\ingroup categorised

TnFOX defines a wide range of Qt-compatible container classes and indeed a few
ones extending that functionality as well.

See the page \link qtdiffs
Structural differences between TnFOX and Qt
\endlink
for more information, plus of course the documentation for each of the classes
themselves
*/

/*! \defgroup openglclasses OpenGL classes
\ingroup categorised

\li FX::FXGLCanvas	GLCanvas, an area drawn by another object 	
\li FX::FXGLCone	OpenGL Cone Object 	
\li FX::FXGLContext		
\li FX::FXGLCube	OpenGL Cube Object 	
\li FX::FXGLCylinder	OpenGL Cylinder Object 	
\li FX::FXGLGroup	Group object 	
\li FX::FXGLLine	OpenGL Line Object 	
\li FX::FXGLObject	Basic OpenGL object 	
\li FX::FXGLPoint	OpenGL Point Object 	
\li FX::FXGLShape	OpenGL Shape Object 	
\li FX::FXGLSphere	OpenGL Sphere Object 	
\li FX::FXGLTriangleMesh	OpenGL Triangle Mesh Object 	
\li FX::FXGLViewer	Canvas, an area drawn by another object 	
\li FX::FXGLVisual	Visual describes pixel format of a drawable
\li FX::FXViewport	OpenGL viewer viewport
*/
