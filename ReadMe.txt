TnFOX v0.85 13th Feb 2005:
-=-=-=-=-=-=-=-=-=-=-=-=-=

by Niall Douglas

For full documentation, please consult the index.html file.

Issues:
-=-=-=-
1: THE PYTHON BINDINGS ARE NOT FINISHED
The python bindings remain incomplete until pyste gets its AllFromHeader()
function working again. There is enough for most things but you'll need
to manually define constants etc. Furthermore, on POSIX exceptions won't
traverse shared object boundaries due to a problem with typeinfo and so
the problems v0.75 had with the python bindings module on POSIX remain.
Interestingly, debug builds let exceptions traverse just fine :(

Any python code creating children of a FXObject (or anything derived from
it eg; FXWindow) MUST ensure that the children are deleted before deleting
the parent. This is due to python reference counting interacting with
FOX's auto-destruction of children ie; FOX will delete objects without
telling python so when python tries to delete them, you get at best heap
corruption and at worst a fault. This may be fixable in the future.

2: 64 BIT ARCHITECTURES ARE NOT FULLY SUPPORTED
FXProcess::virtualAddrSpaceLeft() currently doesn't understand 64 bit
Linux and FreeBSD architectures. Any advice here would be greatly
appreciated. This problem won't affect any code which doesn't use this
function (which is no other code in TnFOX at all)

3: THERE ARE SOME KNOWN BUGS ON CERTAIN PLATFORMS
See Todo.txt for a list of known bugs.

Installation:
-=-=-=-=-=-=-
If you're installing the binary only on POSIX, simply do:

libtool --mode=install cp libTnFOX-0.85.la /usr/local/lib


You will need a make tool called scons from http://www.scons.org/ v0.95 or
later and an installation of python v2.3 or later. You will know it's
correctly installed when you can run "scons" from the command line (on
windows you may need to copy scons.bat into c:\winnt or c:\windows after
installation).

Extract the archive. Open a command box, change directory to the TnFOX one
and type "scons -h" to get a list of make targets. Support for JPEG, PNG,
TIFF, ZLib and OpenSSL are optional - most POSIX users will already have these
installed and scons will just find them. Alter config.py to set debug mode
and other options.

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

1. Microsoft Windows 2000 SP3 with MSVC7.1 (Visual Studio .NET 2003)
2. RedHat Fedora Core 3 with GCC & libstdc++ 3.4.2 (stock release)
3. FreeBSD v5.3 with GCC & libstdc++ 3.4.2 (with visibility patch)

Up until v0.3 MSVC6 was supported. Unfortunately since then the failings
in its compiler have forced me to drop it.

Other systems aren't accessible to me so I can't do much more :(

Problems:
-=-=-=-=-
A list of known bugs is in the Todo.txt file.

License:
-=-=-=-=
Please see files entitled License.txt, License_Addendum.txt and License_Addendum2.txt.
