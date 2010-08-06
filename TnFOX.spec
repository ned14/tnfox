%define name TnFOX
%define version 0.89
%define release 1

Summary: A secure robust threaded exception-aware portable GUI toolkit library
Name: %{name}
Version: %{version}
Release: %{release}
Source0: /home/ned/TnFOX/%{name}-%{version}-sources.zip
Copyright: Jeroen van der Zijp, Niall Douglas and assorted others under the LGPL
Group: Development/Tools
BuildRoot: %{_tmppath}/%{name}-buildroot
Prefix: %{_prefix}
BuildArchitectures: i486 x64
Vendor: Niall Douglas
Packager: Niall Douglas
Requires: Python v2.3+, GCC v3.4+, SCons v0.94+, X11, OpenSSL, Boost.Python, GCC-XML
Url: http://www.nedprod.com/TnFOX/

%description
TnFOX is a modern secure, robust, multithreaded, exception aware, internationalisable,
portable GUI toolkit library designed for mission-critical work in C++ and Python
forked from the FOX library. It replicates the Qt API in many places and has been
designed primarily for Tn, the port of Tornado to FOX.

%prep
%setup

%build

%install
libtool --mode=install cp lib%{name}-%{version}.la /usr/local/lib

%clean

%files
%defattr(-,root,root)
__RPM_FILES__
%doc %{_mandir}/man1/scons.1*
%doc %{_mandir}/man1/sconsign.1*
