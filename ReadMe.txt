TnFOX v0.89 ?:
-=-=-=-=-=-=-=-=-=-=-=-=-=-

by Niall Douglas

For full documentation, please consult the index.html file. Note that TnFOX
incorporates a number of third party libraries into itself (especially on
Windows - see its build page in the documentation for details). On all
platforms, it includes v3.3.17 of SQLite3.

How to check out the full source tree:
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
If you are checking this out from GIT HEAD rather than a source archive,
please be aware that GIT is unusual in not automatically fetching subprojects
for you when you do the "git clone". As a result, you will get errors about
src/nedmalloc/SConstruct being missing. To fix this, do "git clone --recursive"
instead or if you have already cloned, do "git submodule update --init
--recursive" to have GIT recursively fetch the submodules for you.

Issues:
-=-=-=-
1: THE PYTHON BINDINGS WERE RECENTLY REIMPLEMENTED
The new pyplusplus solution will be far superior in the long run, but as
yet it's still not quite working. Furthermore, py3k is just around the
corner and Boost.Python is being updated to support it thanks to Google's
Summer of Code. Hence the bindings have NOT been supplied with this release.

2: THERE ARE SOME KNOWN BUGS ON CERTAIN PLATFORMS
See per-platform documentation and Todo.txt for a list of known bugs.


Automated Regression Test Suite:
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
This can be found inside TestSuite with AllTests being the control program.
TestResults.sqlite3 holds a copy of the current test results and
testresults.html is a HTML version of pass & failures.


Installation:
-=-=-=-=-=-=-
If you're installing the binary only on POSIX, simply do:

libtool --mode=install cp libTnFOX-0.89.la /usr/local/lib


You will need a make tool called scons from http://www.scons.org/ v0.95 or
later and an installation of python v2.3 or later. You will know it's
correctly installed when you can run "scons" from the command line (on
windows you may need to copy scons.bat into c:\winnt or c:\windows after
installation).

Extract the archive. Open a command box, change directory to the TnFOX one
and type "scons -h" to get a list of make targets. Support for JPEG, PNG,
TIFF, ZLib, BZip2 and OpenSSL are optional - most POSIX users will already
have these installed and scons will just find them. Alter config.py to set
debug mode and other options.

You should at this stage read the Windows-specific and Unix-specific notes
in the TnFOX HTML documentation. These cover platform-specific third-party
library placement and definition of needed environment variables eg;
PYTHON_INCLUDE.

The compiler configuration is stored in the config directory for each
project. You can add new compiler support eg; for Borland's compiler -
see the scons documentation. To choose a compiler specify "compiler=<x>"
as an argument to scons.

If on Windows, you may prefer to work from within your IDE - simply generate
a MSVC project using "scons msvcproj" and get to work.

WARNING: Some optional facilities provided by FXSSLDevice use patented
and/or legally controlled algorithms. Depending on the country you are
in and whether it enforces US software patents, you may have to pay
royalties for its usage (FXSSLDevice's docs say which). Also in some
other countries, usage of the "stronger" encryption is illegal and
I might add that legally using encryption in one country can mean you
getting locked up if you ever enter some other country (eg; the US)
because their laws apply irrespective of where you did it.

IF YOU CHOOSE TO BREAK THE LAW, THEN IT IS YOUR OWN CHOICE. TNFOX
GIVES YOU THE OPTION TO DO SO OR NOT AND SO THEREFORE NO ONE EXCEPT
THE END-USER IS LIABLE. YOU HAVE BEEN WARNED!

Python bindings:
-=-=-=-=-=-=-=-=
If you want to regenerate the python bindings, you will need a copy of the
Boost library (http://www.boost.org/) which expects to be in the same
directory as TnFOX and called "boost". You will also need GCCXML installed -
see the pyste documentation in boost.python.

Make very sure you consult the HTML documentation before building the
bindings, it takes some time and by preparation you can avoid time-costly
mistakes.

Testing regime:
-=-=-=-=-=-=-=-
Before this release, every program in the TestSuite was compiled and tested
in both debug and release modes on:

1. Microsoft Windows XP SP2 x64 with MSVC9.0 (Visual Studio .NET 2008)
2. KUbuntu Hardy Heron x86 with GCC & libstdc++ 4.2
3. KUbuntu Hardy Heron x64 with GCC & libstdc++ 4.2
4. Apple Mac OS X 10.4.8 x86 with XCode v2.3 [NOT THIS RELEASE]

Up until v0.3 MSVC6 was supported. Unfortunately since then the failings
in its compiler have forced me to drop it.

Other systems aren't accessible to me so I can't do much more :(

Problems:
-=-=-=-=-
A list of known bugs is in the Todo.txt file.

License:
-=-=-=-=
Please see files entitled License.txt, License_Addendum.txt and License_Addendum2.txt.
