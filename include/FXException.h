/********************************************************************************
*                                                                               *
*                     E x c e p t i o n  H a n d l i n g                        *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2003 by Niall Douglas.   All Rights Reserved.            *
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

#ifndef FXEXCEPTION_H
#define FXEXCEPTION_H
#include <string.h>
#include <errno.h>
#include <new>	// For std::bad_alloc
#include "FXString.h"

namespace FX {

/*! \file FXException.h
\brief Defines macros and classes used in implementation of FOX exception generation & handling
*/

/*! \defgroup FXExceptionFlags FXERRHMAKE() bitwise flags
A bitwise combination of these flags is passed to anything using FXERRHMAKE()
*/
/*! \ingroup FXExceptionFlags
This specifies that the exception is fatal and program exit will happen immediately
*/
#define FXERRH_ISFATAL				1
/*! \ingroup FXExceptionFlags
This specifies that it should not be possible to retry the operation causing the exception
*/
#define FXERRH_ISNORETRY			2
/*! \ingroup FXExceptionFlags
This specifies that the exception is an informational message (changes the presentation)
*/
#define FXERRH_ISINFORMATIONAL		4
/*! \ingroup FXExceptionFlags
This specifies that the exception originated from the other side of a message connection
*/
#define FXERRH_ISFROMOTHER			8
/*! \ingroup FXExceptionFlags
This specified that during the throwing of the exception, further exceptions were thrown.
When this occurs, generally at the very least memory and/or resource leaks have happened
and you should take action where possible (eg; retrying the stack unwind). Note that this
flag being set usually implies FXERRH_ISFATAL.
*/
#define FXERRH_HASNESTED			16
/*! \ingroup FXExceptionFlags
This specifies that the exception is part of run time checks and should only be fatal in
release versions
*/
#define FXERRH_ISDEBUG				32

/*!
\def FXEXCEPTION_DISABLESOURCEINFO
Defining this prevents source file information being compiled in with every generation
of a FXException
*/
#ifdef FXEXCEPTION_DISABLESOURCEINFO
#define FXEXCEPTION_FILE(p) (const char *) 0
#define FXEXCEPTION_LINE(p) 0
#else
#define FXEXCEPTION_FILE(p) __FILE__
#define FXEXCEPTION_LINE(p) __LINE__
#endif

/*!
The preferred way to create a FXException, this constructs one into \em e2
with message \em msg, code \em code and \em flags being the usual exception flags
\sa FXExceptionFlags
*/
#define FXERRMAKE(e2, msg, code, flags)		FX::FXException e2(FXEXCEPTION_FILE(e2), FXEXCEPTION_LINE(e2), msg, code, flags);

//! Throws the specified exception
#define FXERRH_THROW(e2)					{ FX::FXException::int_setThrownException(e2); throw e2; }
//! This combines a FXERRMAKE() with a FXERRH_THROW()
#define FXERRG(msg, code, flags)			{ FXERRMAKE(_int_e_temp, msg, code, flags); FXERRH_THROW(_int_e_temp); }
//! This performs the assertion \em cond, which if false throws the specified exception
#ifdef DEBUG
#define FXERRH(cond, msg, code, flags)		if(!(cond) || FX::FXException::int_testCondition()) FXERRG(msg, code, flags)
#else
#define FXERRH(cond, msg, code, flags)		if(!(cond)) FXERRG(msg, code, flags)
#endif

/*! \defgroup FXExceptionCodes TnFOX exception code constants
*/
/*! \ingroup FXExceptionCodes
Defines all the system exception codes
*/
enum FXExceptionCodes
{
	FXEXCEPTION_USER		=0x00010000,						//!< User code can use this value and upwards
	FXEXCEPTION_SYSCODE_BASE=0x0000FFFF,
	FXEXCEPTION_BADRANGE	=(FXEXCEPTION_SYSCODE_BASE - 0),	//!< This code indicates that the valid range was exceeded
	FXEXCEPTION_NULLPOINTER	=(FXEXCEPTION_SYSCODE_BASE - 1),	//!< This code indicates a null pointer was encountered
	FXEXCEPTION_NORESOURCE	=(FXEXCEPTION_SYSCODE_BASE - 2),	//!< This code indicates a failure to allocate a resource
	FXEXCEPTION_NOMEMORY	=(FXEXCEPTION_SYSCODE_BASE - 3),	//!< This code indicates a failure to allocate memory
	FXEXCEPTION_NOTSUPPORTED=(FXEXCEPTION_SYSCODE_BASE - 4),	//!< This code indicates that an operation is unsupported
	FXEXCEPTION_NOTFOUND	=(FXEXCEPTION_SYSCODE_BASE - 5),	//!< This code indicates a failure to find something (eg; a file, handle, etc)
	FXEXCEPTION_IOERROR		=(FXEXCEPTION_SYSCODE_BASE - 6),	//!< This code indicates a failure to perform i/o
	FXEXCEPTION_CONNECTIONLOST=(FXEXCEPTION_SYSCODE_BASE - 7),	//!< This code indicates that a connection unexpectedly terminated
	FXEXCEPTION_NOPERMISSION=(FXEXCEPTION_SYSCODE_BASE - 8),	//!< This code indicates that the caller did not have permission

	FXEXCEPTION_WINDOW		=(FXEXCEPTION_SYSCODE_BASE - 9),	//!< This code indicates that an error happened in the window system
	FXEXCEPTION_IMAGE		=(FXEXCEPTION_SYSCODE_BASE - 10),	//!< This code indicates that an error happened in the image system
	FXEXCEPTION_FONT		=(FXEXCEPTION_SYSCODE_BASE - 11),	//!< This code indicates that an error happened in the font system

	FXEXCEPTION_OSSPECIFIC	=(FXEXCEPTION_SYSCODE_BASE - 32),	//!< This code indicates an exception was thrown by a call to the local OS thunking layer
	FXEXCEPTION_PYTHONERROR	=(FXEXCEPTION_SYSCODE_BASE - 33),	//!< This code indicates that this exception came from a python exception

	FXEXCEPTION_INTTHREADCANCEL=(FXEXCEPTION_SYSCODE_BASE - 256)
};

/*! Use this macro to generate a FXException representing the CLib/POSIX error code \em code. Flags are as
for usual exceptions
\sa FX::FXExceptionFlags
*/
#define FXERRGOS(code, flags)				{ FX::FXException::int_throwOSError(FXEXCEPTION_FILE(code), FXEXCEPTION_LINE(code), code, flags); }
#define FXERRGOSFN(code, flags, filename)	{ FX::FXException::int_throwOSError(FXEXCEPTION_FILE(code), FXEXCEPTION_LINE(code), code, flags, filename); }
#ifdef DEBUG
#define FXERRHOS(exp)				{ int __errcode=(exp); if(__errcode<0 || FX::FXException::int_testCondition()) FXERRGOS(errno, 0); }
#define FXERRHOSFN(exp, filename)	{ int __errcode=(exp); if(__errcode<0 || FX::FXException::int_testCondition()) FXERRGOSFN(errno, 0, filename); }
#else
/*! Use this macro to wrap POSIX, UNIX or CLib functions. On Win32, the includes anything in
MSVCRT which sets errno
*/
#define FXERRHOS(exp)				{ int __errcode=(exp); if(__errcode<0) FXERRGOS(errno, 0); }
#define FXERRHOSFN(exp, filename)	{ int __errcode=(exp); if(__errcode<0) FXERRGOSFN(errno, 0, filename); }
#endif

//@(
//! Enters a section of code guarded against exceptions
#define FXERRH_TRY			{ bool __retryerrorh; do { __retryerrorh=false; FX::FXException_TryHandler __newtryhandler(FXEXCEPTION_FILE(0), FXEXCEPTION_LINE(0)); try
//! Catches a specific exception
#define FXERRH_CATCH(e2)	catch(e2)
/*! \param owner Either a FXApp or FXWindow
\param e The FXException to report
Reports the exception \em e to the user
\note You must <tt>#include "FXExceptionDialog.h"</tt> for this to compile
\warning You must be within a FXERRH_TRY...FXERRH_ENDTRY
*/
#define FXERRH_REPORT(owner, e2)	{ if(FX::FXExceptionDialog(owner, e2).execute()) __retryerrorh=true; }
//! Ends the section of guarded code plus its exception handlers
#define FXERRH_ENDTRY		} while(__retryerrorh); }
//@)

//! Deletes and zeros a variable so it won't get deleted again (usually a class member variable)
#define FXDELETE(v)			{ delete v; v=0; }

#define FXERRH_NODESTRUCTORMOD
#define FXEXCEPTIONDESTRUCT1 FX::FXException::int_incDestructorCnt(); try
#define FXEXCEPTIONDESTRUCT2 catch(FXException &_int_caught_e) { if(FX::FXException::int_nestedException(_int_caught_e)) throw; } FX::FXException::int_decDestructorCnt()
//@(
//! Enters a section of code guarded against \c std::bad_alloc exceptions
#define FXEXCEPTION_STL1 try {
//! Exits a section of code guarded against \c std::bad_alloc exceptions
#define FXEXCEPTION_STL2 } catch(const std::bad_alloc &) { FXERRGM }
//@)
//@(
//! Enters a section of code called by exception-unaware code eg; FOX GUI code
#define FXEXCEPTION_FOXCALLING1 FXERRH_TRY {
//! Exits a section of code called by exception-unaware code eg; FOX GUI code
#define FXEXCEPTION_FOXCALLING2 } FXERRH_CATCH(FX::FXException &_int_caught_e) { FXERRH_REPORT(FX::FXApp::instance(), _int_caught_e); } FXERRH_ENDTRY
//@)


/*! \class FXException
\brief The base exception class in FOX

FXException is the base class for exception throws in FOX and is probably one of the most feature
rich exception classes on any toolkit for C++. Features include:
\li Custom error message and error code
\li Automatic retries of guarded code
\li Source code file name and line number embedding
\li Full stack backtrace for MSVC
\li Thread safe
\li Serialisable so it can be sent by IPC
\li Support for nested exceptions. See section below about this
\li Code quality testing. See setGlobalErrorCreationCount()

Usage is exceptionally easy with levels of support so you can automate as much or little of the
generation process as you like. Typical forms include:
\li <tt>FXERRH(5==a, "'a' is not five", MY_AISNOTFIVECODE, FXERRH_ISFATAL);</tt> performs an assertion test
\li <tt>FXERRG("File not found", FXEXCEPTION_NOTFOUND, 0);</tt> generates an error and throws it
\li <tt>FXException e=FXERRMAKE("Unknown exception", MY_UNKNOWNEXCEPTIONCODE, FXERRH_ISNORETRY);</tt> generates
an instance of FXException

Automatic retries are easy with <tt>try</tt> & <tt>catch</tt> replaced with macros:
\code
FXERRH_TRY
{
   // do something
}
FXERRH_CATCH(FXException &e)
{
   FXERRH_REPORT(parentwnd, e)
}
FXERRH_ENDTRY
\endcode
You should also replace <tt>throw</tt> with FXERRH_THROW() though this is usually done for you if you
use any of the FXERRH or FXERRG macros.
\sa FXERRHM(), FXERRHPTR(), FXERRGNF()

<h3>Source munging</h3>
FXException has an accompanying source file munging script written in Python called CppMunge.py.
This useful tool can automatically extract error code macros as specified in your C++ sources
and assign them unique constants into a header file (by default called ErrCodes.h) that are guaranteed
to be unique to each class name. This greatly simplifies error handling across entire projects,
irrespective of libraries used. You still need to <tt>#include "ErrCodes.h"</tt> in each of your source
files using this feature. See \link CppMunge
this link for more details
\endlink

<h3>Nested Exceptions</h3>
The C++ standard states that if an exception is thrown during a stack unwind caused by the handling
of an exception throw, the program is to exit immediately via <tt>std::terminate()</tt> (ie; immediate
exit without calling <tt>atexit()</tt> registrants or anything else). The rationale behind this is that destructors
permanently destroy state and so cannot be guaranteed restartable.

This writer personally finds this to be a silly limitation substantially decreasing the reliability
of C++ programs using exceptions. While correct design has the programmer wrapping all destructors
capable of throwing in a <tt>try...catch</tt> section, a single missed wrapper causes immediate
program exit - a situation which may arise once in a blue moon and thus be missed by testing. What
is really needed is to change the C++ spec so that throwing any exception from any destructor is
illegal - and then more importantly, have the compiler issue an error if code tries it. This would
require modification of the linker symbol mangling system and thus break binary compatibility though.

Furthermore, this writer feels that as soon as you use ANY exceptions in C++, you must write all
all destructors to be exception aware because as code grows in size, it becomes impossible to track
whether code called by any non-trivial destructor throws or not. Best IMHO to assume that
everything can throw, at all times. What annoys me most of all about the C++ standard action of
immediately terminating the process is that there is no opportunity for saving out state or even
providing debug information.

The C++ source code munger detailed above can optionally insert <tt>try...catch(FXException)</tt> wraps around
each and every destructor it processes (this can be prevented via placing \c FXERRH_NODESTRUCTORMOD after the
destructor definition). The wrap checks to see if an exception is currently being thrown for the
current thread and if so, it appends the new exception and sets the FXERRH_HASNESTED and
FXERRH_ISFATAL flags. Thus by default the program will report the error, clean up and exit.

\note The nested exception framework is disabled during static data initialisation and destruction.

<h3>Restarting destructors</h3>
It \em is possible to write restartable destructors in C++ - however, they won't always be of use
to you eg; if the object is created on the stack. In that situation, an exception thrown implies
unwinding the rest of the stack before it can be handled which clearly implies that that object
can no longer exist in any useful sense. 

An important point is that the next issue (time of writing: June 2003) of the C++ standard is
likely to make all exception throwing from destructors illegal, so if you wish to future-proof
your code you want to avoid it.

<h3>Static data initialisation and destruction:</h3>
A major headache I had was what to do when an exception is thrown during static data
initialisation and destruction. Firstly, I'll tell you the received wisdom - <i>just don't
do it!</i> - your application is in an undefined state because static data is initialised
and destroyed in a random order, so you can't guarantee anything at all! The facilities
provided by FX::FXProcess_StaticInit are there precisely to permit non-trivial static data
initialisation and destruction, <b>so use them!</b>

However, sometimes you can't escape running non-trivial code and it's better to reuse the
library rather than duplicate the functionality in non-throwing code. For this situation
I've had FXException print its report to \c stderr on construction if static data is
unavailable and it also disables its nested exception functionality (because it requires
holding static state). This should cover informing the user of a problem rather than the
generic and unhelpful message you usually get when \c terminate() gets called.

<h3>Good practice in exception using C++:</h3>
First cardinal rule of all is to <b>always assume every line of code can throw an exception</b>.
Not trying to be clever will prevent nasty bugs!
\li Mark code which does not throw exceptions with the \c throw() modifier - most modern compilers
will avoid generating stack unwind code and thus efficiency is improved. I personally don't use
any more complicated exception specifications because it's very hard to know what exceptions can
be thrown in anything more than trivial code and besides, once one exception has to be handled
the stack unwind code won't change much to handle all exceptions.
\li Never use stack-allocated raw pointers to a <tt>new</tt>-ed object. If an exception were
thrown, the object would never be deleted. Use some sort of smart pointer which deletes its
contents on destruct (eg; std::auto_ptr or FX::FXPtrHold) or a rollback action (FX::FBRBNew()).
\li For that matter always wrap memory allocations with FXERRHM(expression) - for \c malloc(),
\c calloc() etc. it throws a FX::FXMemoryException if a null pointer is returned. For \c
new it catches the \c std::bad_alloc exception and converts it into a FX::FXMemoryException.
The reason we can't use \c std::bad_alloc natively is because the nested exception handling
framework needs all exceptions to derive off FX::FXException.
\li For similar reasons to the point above, wrap all use of the STL where \c std::bad_alloc
might be thrown with FXEXCEPTION_STL1 and FXEXCEPTION_STL2.
\li If you are going to write anything even remotely complex, you will run into the problem
of exception throws causing your program's data structures to become inconsistent (eg; you're
in the middle of altering a set of lists which need to be mutually consistent). To correctly
address this problem, TnFOX has full-featured transaction rollback support, \link rollbacks
more about which you can find here.
\endlink
\li Destructors should never throw exceptions if you want to keep your code future-proof.
\li Objects which can be instantiated on the stack either can not throw an exception during
destruction OR nothing is lost if it does
\li For <tt>new</tt>-ed only objects, write your destructors to be restartable. This implies code like the following:
\code
delete mymemberptr; // C++ spec guarantees delete 0; does nothing
mymemberptr=0;
\endcode
FXException.h defines the convenience macro FXDELETE() which does exactly the above. If you
meticulously stick to this approach throughout all your code, you can recall <tt>delete</tt>
on an object which previously threw an exception (obviously after fixing whatever caused the
problem in the first place).
\li A special case is correctly handling when constructors throw an exception (dragons 
traditionally live here which is probably why so many people dislike exceptions). The C++ spec says that
unless it was being constructed via a placement new, no destructor is called (which is
somewhat inconsistent IMHO since placement new's include everything \em except the vanilla
global new operator). Since most classes of any complexity will contain pointers to other
objects as members variables which are initialised during the constructor, this can leave
the object in a half-constructed state with no automatic rollback.<br>
<br>
My solution to this is to construct your object in two stages: (i) make object's internal state
null but valid and (ii) populate object (you also need to write your destructor so that it
is restartable as detailed above) eg;
\code
Object::Object() : p1(0), p2(0)
{
	FXRBOp unconstruct=FXRBConstruct(this);
	FXERRHM(p1=new ObjectPrivate1);
	FXERRHM(p2=new ObjectPrivate2);
	unconstruct.dismiss();
}

Object::~Object()
{
	FXDELETE(p2);	// Really means "if(p2) { delete p2; p2=0; }"
	FXDELETE(p1);
}
\endcode
NEVER EVER initialise p with a direct \c new (ie; <tt>Object() : p(new ObjectPrivate)</tt>
as is so common in poorly written C++ - if an exception were to be thrown your destructor
could not possibly know how to correctly cleanup the object. The other point is the rollback
operation FXRBConstruct() which invokes your normal destructor if something goes wrong -
however note that it only comes into effect \em after direct member initialisation which
implies if that if non-trivial operations are done at that time, a thrown exception
means no destruction. Watch out for this especially in copy constructors.
\note The received wisdom on this is to never use raw pointers as member variables in your
class - use a smart pointer \c (auto_ptr) instead. The big problem with this is that smart
pointers need to know the destructor of the type they point to which causes a major problem if
your type is deliberately opaque. My solution removes that problem.
\note An often suggested solution to this problem is to use function try blocks (if you can
find a C++ compiler which supports them). They are of the form:
\code
Object::Object()
try
	: p1(new ObjectPrivate1), p2(new ObjectPrivate2)
{
	...
}
catch(exception e)
{
	...
}
\endcode
However this does not solve the problem of knowing whether p1 was actually constructed
or if p1 points at random garbage. Furthermore function try blocks don't actually behave
like normal try blocks, they work subtly differently (more like a filter than a handler).
*/
class FXException;
template<class type> class QValueList;
class FXEXCEPTIONAPI(FXAPI) FXException
{
private:
  int uniqueId;		// zero if exception invalid
  FXString _message;
  FXuint _code;
  FXuint _flags;
  const char *srcfilename;
  int srclineno;
  FXulong _threadId;
  mutable FXString *reporttxt;
  QValueList<FXException> *nestedlist;
#ifdef WIN32
#ifndef FXEXCEPTION_DISABLESOURCEINFO
#define FXEXCEPTION_STACKBACKTRACEDEPTH 8
	struct
	{
		void *pc;
		char module[64];
		char functname[128];
		char file[64];
		int lineno;
	} stack[FXEXCEPTION_STACKBACKTRACEDEPTH];
#endif
#endif
  int stacklevel;
private:
	void init(const char *_filename, int _lineno, const FXString &_msg, FXuint _code, FXuint _flags);
public:
  /*!
  Constructs FXException with the given parameters.

  You should always use FXERRMAKE() or a derivative of it instead of this directly
  */
  FXException(const char *_filename, int _lineno, const FXString &_msg, FXuint _code, FXuint _flags) : reporttxt(0), nestedlist(0)
  { init(_filename, _lineno, _msg, _code, _flags); }
  /*! \overload 
  */
  FXException() : uniqueId(0), reporttxt(0), nestedlist(0) { }
  //! \deprecated For backward code compatibility only
  FXDEPRECATEDEXT FXException(const FXchar *msg) : reporttxt(0), nestedlist(0) { init(0, 0, msg, 0, 0); }
  FXException(const FXException &o);
  FXException &operator=(const FXException &o);
  //! Returns if this exception object is valid or not
  bool isValid() const throw() { return uniqueId!=0; }
  //! Returns true if this exception is fatal
  bool isFatal() const throw() { return _flags & FXERRH_ISFATAL; }
  //! Sets whether this exception is fatal or not
  void setFatal(bool _fatal);
  //! Returns in which source file the exception happened
  void sourceInfo(const char **file, int *lineno) const throw() { if(file) *file=srcfilename; if(lineno) *lineno=srclineno; }
  //! Returns the message this exception represents
  const FXString &message() const throw() { return _message; }
  //! Sets the message to be reported by this exception
  void setMessage(const FXString &msg);
  //! Returns the code of the exception
  FXuint code() const throw() { return _code; }
  //! Returns the flags of the exception
  FXuint flags() const throw() { return _flags; }
  //! Returns the id of the thread in which the exception was thrown
  FXulong threadId() const throw() { return _threadId; }
  /*!
  Returns a string fully describing the exception, including its cause, location (file and
  line number) and a stack backtrace if supported or possible
  */
  const FXString &report() const;
  //! \deprecated For backward code compatibility only
  FXDEPRECATEDEXT const FXchar *what() const { return _message.text(); }
  //! Returns true if this exception is the primary (first) exception currently being thrown ie; not nested
  bool isPrimary() const;
  //! Returns the number of nested exceptions which occurred during the handling of this exception
  FXint nestedLen() const;
  //! Returns the nested exception \em idx
  FXException &nested(FXint idx) const;
  virtual ~FXException();
  /*! \return The previous setting
  \param no The new setting (0=create none (the default), negative number imply randomness)

  Sets a new global error creation count for the current thread. Under debug builds, all FXERRH() assertion tests call
  an internal routine which increments a count which if passed, triggers the exception anyway.
  This permits bulletproofing your code and testing that it handles exceptions being thrown
  correctly (for best use, combine with your favourite memory leak detector etc).

  Probably the most useful setting is to negative numbers which internally sets the actual
  count to a random number between 1 and abs(val). Setting to zero disables the feature.

  The exceptions will keep being thrown until disabled ie; the count restarts or rerandomises.
  */
  static FXint setGlobalErrorCreationCount(FXint no);
  /*! Sets an internal flag causing the construction of a FXException to breakpoint.
  Has no effect on release builds */
  static bool setConstructionBreak(bool v);

  // Internal and not to be publicly used
  static FXDLLLOCAL void int_enableNestedExceptionFramework(bool yes=true);
  static void int_setThrownException(FXException &e);
  static void int_enterTryHandler(const char *srcfile, int lineno);
  static void int_exitTryHandler() throw();
  static void int_incDestructorCnt();
  static bool int_nestedException(FXException &e);
  static void int_decDestructorCnt();
  static bool int_testCondition();
  static void int_throwWinError(const char *file, int lineno, FXuint code, FXuint flags, const FXString &filename=FXString::nullStr());
  static void int_throwOSError(const char *file, int lineno, int code, FXuint flags, const FXString &filename=FXString::nullStr());
  friend FXAPI FXStream &operator<<(FXStream &s, const FXException &i);
  friend FXAPI FXStream &operator>>(FXStream &s, FXException &i);
};

//! Writes the exception to stream \em s
FXAPI FXStream &operator<<(FXStream &s, const FXException &i);
//! Reads an exception from stream \em s
FXAPI FXStream &operator>>(FXStream &s, FXException &i);

struct FXException_TryHandler
{
	FXException_TryHandler(const char *srcfile, int lineno)
	{
		FXException::int_enterTryHandler(srcfile,lineno);
	}
	~FXException_TryHandler()
	{
		FXException::int_exitTryHandler();
	}
};


/*! \class FXRangeException
\brief An out-of-bounds or invalid range error
*/
class FXEXCEPTIONAPI(FXAPI) FXRangeException : public FXException
{
public:
	//! Use FXERRMAKE
	FXRangeException(const char *_filename, int _lineno, const FXString &_msg)
		: FXException(_filename, _lineno, _msg, FXEXCEPTION_BADRANGE, 0) { }
	//! \deprecated For backward code compatibility only
	FXDEPRECATEDEXT FXRangeException(const FXchar *msg)
		: FXException(0, 0, msg, FXEXCEPTION_BADRANGE, 0) { }
};
/*! \return A FX::FXRangeException

Use this macro to generically indicate a parameter exceeding the permitted range. As this is a common
failure in all code, this should greatly help intuitive writing of handlers for this
common error
*/
#define FXERRGRANGE(msg, flags)		{ FX::FXRangeException _int_temp_e(FXEXCEPTION_FILE(msg), FXEXCEPTION_LINE(msg), msg, flags); FXERRH_THROW(_int_temp_e); }

/*! \class FXPointerException
\brief An unexpected null pointer error
*/
class FXEXCEPTIONAPI(FXAPI) FXPointerException : public FXException
{
public:
	//! Use FXERRHPTR()
	FXPointerException(const char *_filename, int _lineno, FXint _flags)
		: FXException(_filename, _lineno, "Null pointer", FXEXCEPTION_NULLPOINTER, _flags) { }
	//! \deprecated For backward code compatibility only
	FXDEPRECATEDEXT FXPointerException(const FXchar *msg)
		: FXException(0, 0, msg, FXEXCEPTION_NULLPOINTER, 0) { }
};
#define FXERRGPTR(flags)			{ FX::FXPointerException _int_temp_e(FXEXCEPTION_FILE(flags), FXEXCEPTION_LINE(flags), flags); FXERRH_THROW(_int_temp_e); }
/*! \return A FX::FXPointerException

Use this macro to test for null pointers
*/
#ifdef DEBUG
#define FXERRHPTR(exp, flags)	if(!(exp) || FX::FXException::int_testCondition()) FXERRGPTR(flags)
#else
#define FXERRHPTR(exp, flags)	if(!(exp)) FXERRGPTR(flags)
#endif

/*! \class FXResourceException
\brief A failure to allocate some resource error
*/
class FXEXCEPTIONAPI(FXAPI) FXResourceException : public FXException
{
public:
	//! Usually instantiated by other types
	FXResourceException(const char *_filename, int _lineno, const FXString &_msg,
		FXuint _code=FXEXCEPTION_NORESOURCE, FXuint _flags=0)
		: FXException(_filename, _lineno, _msg, _code, _flags) { }
	//! \deprecated For backward code compatibility only
	FXDEPRECATEDEXT FXResourceException(const FXchar *msg)
		: FXException(0, 0, msg, FXEXCEPTION_NORESOURCE, 0) { }
};

/*! \class FXMemoryException
\brief A failure to allocate memory error
*/
class FXEXCEPTIONAPI(FXAPI) FXMemoryException : public FXResourceException
{
public:
	//! Use FXERRHM() to instantiate
	FXMemoryException(const char *_filename, int _lineno)
		: FXResourceException(_filename, _lineno, "Out of memory", FXEXCEPTION_NOMEMORY, 0) { }
	//! \deprecated For backward code compatibility only
	FXDEPRECATEDEXT FXMemoryException(const FXchar *msg)
		: FXResourceException(0, 0, msg, FXEXCEPTION_NOMEMORY, 0) { }
};
#define FXERRGM				{ FX::FXMemoryException _int_temp_e(FXEXCEPTION_FILE(0), FXEXCEPTION_LINE(0)); FXERRH_THROW(_int_temp_e); }
/*! \return A FX::FXMemoryException

Use this macro to wrap malloc, calloc and new eg;
\code
  FXPtrHold ptr;
  FXERRHM(ptr=new Object());
\endcode
Always doing this will correctly trap lack of memory and take the appropriate action
*/
#ifdef DEBUG
#define FXERRHM(exp)		do { try { if(!FX::FXException::int_testCondition() && (exp)) break; } catch(const std::bad_alloc &) { } FXERRGM; } while(0)
#else
#define FXERRHM(exp)		do { try { if((exp)) break; } catch(const std::bad_alloc &) { } FXERRGM; } while(0)
#endif

/*! \class FXNotSupportedException
\brief An operation was not supported
*/
class FXEXCEPTIONAPI(FXAPI) FXNotSupportedException : public FXException
{
public:
	//! Use FXERRGNOTSUPP() to instantiate
	FXNotSupportedException(const char *_filename, int _lineno, const FXString &msg)
		: FXException(_filename, _lineno, msg, FXEXCEPTION_NOTSUPPORTED, FXERRH_ISNORETRY|FXERRH_ISDEBUG) { }
};
/*! \return A FX::FXNotSupportedException

Use this macro to indicate that an operation is not supported
*/
#define FXERRGNOTSUPP(msg)	{ FX::FXNotSupportedException _int_temp_e(FXEXCEPTION_FILE(msg), FXEXCEPTION_LINE(msg), msg); FXERRH_THROW(_int_temp_e); }

/*! \class FXNotFoundException
\brief A failure to find something (eg; a file, handle, etc)
*/
class FXEXCEPTIONAPI(FXAPI) FXNotFoundException : public FXResourceException
{
public:
	//! Use FXERRGNF() to instantiate
	FXNotFoundException(const char *_filename, int _lineno, const FXString &_msg, FXint flags)
		: FXResourceException(_filename, _lineno, _msg, FXEXCEPTION_NOTFOUND, flags|FXERRH_ISINFORMATIONAL) { }
};
/*! \return A FX::FXNotFoundException

Use this macro to generically indicate failures to find things. As this is a common
failure in all code, this should greatly help intuitive writing of handlers for this
common error
*/
#define FXERRGNF(msg, flags)	{ FX::FXNotFoundException _int_temp_e(FXEXCEPTION_FILE(msg), FXEXCEPTION_LINE(msg), msg, flags); FXERRH_THROW(_int_temp_e); }

/*! \class FXIOException
\brief A failure to perform i/o
*/
class FXEXCEPTIONAPI(FXAPI) FXIOException : public FXResourceException
{
public:
	//! Use FXERRHIO() to instantiate
	FXIOException(const char *_filename, int _lineno, const FXString &msg, FXuint code=FXEXCEPTION_IOERROR, FXuint flags=0)
		: FXResourceException(_filename, _lineno, msg, code, flags) { }
};
#define FXERRGIO(msg)		{ FX::FXIOException _int_temp_e(FXEXCEPTION_FILE(msg), FXEXCEPTION_LINE(msg), msg); FXERRH_THROW(_int_temp_e); }
/*! \return A FX::FXIOException

Use this macro to wrap standard C library type calls eg;
\code
  FXERRHIO(handle=open("foo.txt", O_CREAT));
\endcode
*/
#ifdef DEBUG
#define FXERRHIO(exp)		if(-1==(int)(exp) || FX::FXException::int_testCondition()) FXERRGIO(strerror(errno))
#else
#define FXERRHIO(exp)		if(-1==(int)(exp)) FXERRGIO(strerror(errno))
#endif

/*! \class FXConnectionLostException
\brief A connection unexpectedly failing
*/
class FXEXCEPTIONAPI(FXAPI) FXConnectionLostException : public FXIOException
{
public:
	//! Use FXERRGCONLOST() to instantiate
	FXConnectionLostException(const FXString &msg, FXuint flags)
		: FXIOException(0, 0, msg, FXEXCEPTION_CONNECTIONLOST, flags|FXERRH_ISINFORMATIONAL) { }
};
#define FXERRGCONLOST(msg, flags)		{ FX::FXConnectionLostException _int_temp_e(msg, flags); FXERRH_THROW(_int_temp_e); }

/*! \class FXNoPermissionException
\brief The caller had insufficient permissions
*/
class FXEXCEPTIONAPI(FXAPI) FXNoPermissionException : public FXException
{
public:
	//! Use FXERRGNOPERM() to instantiate
	FXNoPermissionException(const FXString &msg, FXuint code=FXEXCEPTION_NOPERMISSION, FXuint flags=0)
		: FXException(0, 0, msg, code, flags|FXERRH_ISINFORMATIONAL) { }
};
#define FXERRGNOPERM(msg, flags)		{ FX::FXNoPermissionException _int_temp_e(msg, FXEXCEPTION_NOPERMISSION, flags); FXERRH_THROW(_int_temp_e); }

/*! \class FXWindowException
\brief A failure in the windowing system

\warning The only safe way of handling this exception is process exit due to FOX
code not being exception aware!
*/
class FXEXCEPTIONAPI(FXAPI) FXWindowException : public FXResourceException
{
public:
	//! \deprecated For backward code compatibility only
	FXDEPRECATEDEXT FXWindowException(const FXchar *msg)
		: FXResourceException(0, 0, msg, FXEXCEPTION_WINDOW, FXERRH_ISFATAL) { }
};

/*! \class FXImageException
\brief A failure in the image, cursor or bitmap systems

\warning The only safe way of handling this exception is process exit due to FOX
code not being exception aware!
*/
class FXEXCEPTIONAPI(FXAPI) FXImageException : public FXResourceException
{
public:
	//! \deprecated For backward code compatibility only
	FXDEPRECATEDEXT FXImageException(const FXchar *msg)
		: FXResourceException(0, 0, msg, FXEXCEPTION_IMAGE, FXERRH_ISFATAL) { }
};

/*! \class FXFontException
\brief A failure in the windowing system

\warning The only safe way of handling this exception is process exit due to FOX
code not being exception aware!
*/
class FXEXCEPTIONAPI(FXAPI) FXFontException : public FXResourceException
{
public:
	//! \deprecated For backward code compatibility only
	FXDEPRECATEDEXT FXFontException(const FXchar *msg)
		: FXResourceException(0, 0, msg, FXEXCEPTION_FONT, FXERRH_ISFATAL) { }
};



} // namespace

#endif
