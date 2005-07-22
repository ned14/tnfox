/********************************************************************************
*                                                                               *
*                           Python embedding support                            *
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

#ifndef FXPYTHON_H

#ifndef FXPYTHON_FROMBOOSTINVOKE
// Include boost.python as it includes us
#ifndef BOOST_PYTHON_DYNAMIC_LIB
#define BOOST_PYTHON_DYNAMIC_LIB
#endif
#ifndef BOOST_PYTHON_MAX_ARITY
#define BOOST_PYTHON_MAX_ARITY 19
#endif
#include <boost/python.hpp>
#else
#define FXPYTHON_H
#include "../include/FXString.h"
#include "../include/FXGenericTools.h"
#include "../include/FXObject.h"
#include <boost/python/opaque_pointer_converter.hpp>

#ifdef WIN32
  #ifdef FOXPYTHONDLL_EXPORTS
    #define FXPYTHONAPI __declspec(dllexport)
  #else
    #define FXPYTHONAPI __declspec(dllimport)
  #endif
#else
  #ifdef GCC_HASCLASSVISIBILITY
    #define FXPYTHONAPI __attribute__ ((visibility("default")))
  #else
    #define FXPYTHONAPI
  #endif
#endif
#ifndef FXPYTHONAPI
  #define FXPYTHONAPI
#endif

// Define a type for representing void *. Type is defined in converters.cpp
struct void_ {};


namespace FX {

/*! \file FXPython.h
\brief Defines things used in embedding python and providing support
*/

class QThread;
class FXFoldingList;
class FXGLViewer;
class FXIconList;
class FXList;
class FXTreeList;
class FXComposite;

/*! \class FXPythonException
\ingroup python
\brief An exception originally thrown within Python
*/
class FXEXCEPTIONAPI(FXPYTHONAPI) FXPythonException : public FXException
{
	friend class FXPython;
	FXPythonException() { }
public:
	//! Use FXERRHPY() to instantiate
	FXPythonException(const FXString &msg, FXuint flags)
		: FXException(0, 0, msg, FXEXCEPTION_PYTHONERROR, flags) { }
};
#define FXERRHPY(ret) { if((ret)<=0) FXPython::int_throwPythonException(); }


/*! \class FXPython
\ingroup python
\brief Miscellaneous Python facilities

Apart from some minor static information functions and the ability to run embedded
pieces of python, the major functionality of this class is solving the "setting
python sort functions" problem. Using the static member of this class setSortFunc()
you can set an arbitrary python function as the sort function for all of the TnFOX
GUI classes using them.

Default implementations are provided for FX::FXListSortFunc, FX::FXFoldingListSortFunc,
FX::FXIconListSortFunc, FX::TreeListSortFunc and FX::FXZSortFunc. You can set a maximum
of 8 process-wide python sorting functions for each (adjustable by a compile-time
constant).

\sa FX::FXPythonInterp
*/
class FXPYTHONAPI FXPython
{
public:
	//! Returns the absolute path of the python interpreter
	static FXString interpreterPath();
	//! Returns the version of the interpreter (same as \c sys.version)
	static FXString version();
	//! Returns the current platform according to python (same as \c sys.platform)
	static FXString platform();
	//! Returns the name of the program as python thinks it is
	static FXString programName();
	//! Sets the name of the program as python thinks it is. Normally done for you if you created python.
	static void setProgramName(const FXString &name);
	/*! Sets the process arguments for python. Normally done for you
	if you created python. Normally, you'll want argv[0] to be a null
	string - see the docs for \c PySys_SetArgv().
	*/
	static void setArgv(int argc, char **argv);
	//! Returns a handle to the dictionary \c globals() within \c __main__
	static boost::python::handle<> globals();
	/*! Executes the specified piece of code within the specified context (ie; globals())
	which can contain multiple lines (separate with \\n), returning whatever the code returns.
	Ensure that an interpreter context has been set beforehand using
	FX::FXPythonInterp::setContext()
	*/
	static boost::python::handle<> execute(const FXString &code, boost::python::handle<> context=globals());
	/*! Evaluates the specified python expression, returning the result.
	Ensure that an interpreter context has been set beforehand using
	FX::FXPythonInterp::setContext()
	*/
	static boost::python::handle<> evaluate(const FXString &code, boost::python::handle<> context=globals());
	//! Sets a C++ callable sorting function for python function \em code
	static void setSortFunc(FXFoldingList &list, boost::python::api::object *code);
	//! Sets a C++ callable sorting function for python function \em code
	static void setSortFunc(FXIconList &list, boost::python::api::object *code);
	//! Sets a C++ callable sorting function for python function \em code
	static void setSortFunc(FXList &list, boost::python::api::object *code);
	//! Sets a C++ callable sorting function for python function \em code
	static void setSortFunc(FXTreeList &list, boost::python::api::object *code);
	//! Sets a C++ callable sorting function for python function \em code
	static void setSortFunc(FXGLViewer &list, boost::python::api::object *code);

	static void int_throwPythonException();
	static void int_initEmbeddedEnv();
	static void int_runPythonThread(PyObject *self, QThread *cthread);
	static void int_pythonObjectCreated(Generic::BoundFunctorV *detach);
	static void int_pythonObjectDeleted(Generic::BoundFunctorV *detach);
	static bool int_FXObjectHandle(long *ret, FXObject *self, FXObject *sender, FXSelector sel, void *ptr);
};
inline PyObject *fxerrhpy(PyObject *ptr)
{
	if(ptr<=0) FXPython::int_throwPythonException();
	return ptr;
}
//! Convenience function calling FX::FXPython::execute()
inline boost::python::handle<> py(const FXString &code) { return FXPython::execute(code); }
//! Convenience function calling FX::FXPython::evaluate()
inline boost::python::handle<> pyeval(const FXString &code) { return FXPython::evaluate(code); }

/*! \class FXPythonInterp
\ingroup python
\brief An instance of the python interpreter

Embedding Python into your TnFOX application is extremely easy - simply
create an instance of the Python interpreter by creating an FXPythonInterp.
Thereafter, make calls to Boost.Python passing whatever parameters you like
after setting which interpreter you want them to use by setContext().
Full support is provided for multithreaded code, nesting calls to multiple
interpreters and the python Global Interpreter Lock (GIL) is correctly maintained.

One issue is what happens when you are embedding python into TnFOX code
which is itself embedded in python code? In this situation, you want one
of two things: (i) to create a new virgin python environment or (ii) to
link into the python environment in which you reside.
The former is easy, simply create a new FXPythonInterp. The latter is
even easier, simply inspect FXPythonInterp::current() which returns a
pointer to the FXPythonInterp responsible for executing the caller (thus
if python code calls C++ code, it is set - but if no python is higher
up the call stack, it is zero).

Furthermore, nesting of interpreters is possible eg; interpreter A
calls C++ code which calls python code in interpreter B which calls
some more C++ code. At each stage, current() returns the correct value
for the current context. You should note that python treats interpreters
and threads running inside an interpreter very similarly and thus a
FXPythonInterp exists for each running thread in each interpreter plus
an instance for each interpreter you create.
\note The current version of python (v2.3) has issues regarding usage
of multiple interpreters, particularly with regard to certain extension
modules which were not written to allow it.

<h3>Usage:</h3>
When creating a new interpreter which is the first "touch" of python
in the process, the current python program name and arguments will
set from FX::FXApp if one of those has been initialised. Otherwise,
you must set it yourself (see FX::FXPython). TnFOX.pyd is loaded into
your new python environment so you can use it immediately.

The setContext() and unsetContext() methods are used to select the
current interpreter that calls to Boost.Python will use. It is \b very
important that for every setContext() there is an equivalent unsetContext()
and you must \b ensure a context is set before calling any python code
at all (failing to do this results in an immediate crash and process exit).
Furthermore you should be aware that python currently requires mutual exclusion
between its interpreters and also between threads running within
each of its interpreters so therefore while a setContext() is in place,
you are also holding a mutex preventing all other threads in python
or going through python from running. Needless to say, you don't want to
hold it for too long - and don't rely on this behaviour, as it may change
(especially different interpreters being able to run concurrently).

Calls to setContext() and unsetContext() can be nested. When python calls
you, the context is released (by the patch applied to boost.python) but when
you call python, you must ensure the context \b is set (our patched pyste
ensures all generated bindings for virtual calls do this). You may find
FX::FXPython_CtxHold of use which doubly ensures exception safety. Be
especially cautious of the fact that setContext() is recursive ie; if
a context is set on entry to setContext(), it is restored on unsetContext()
and so deadlock is extremely likely.

You must not create threads using python's threading module or any other
method - or if you do, these threads must \b never call TnFOX code. TnFOX
relies on QThread creating all the threads as associated
state is created with each thread which would be unset otherwise. Besides,
currently python's threads behave a bit oddly. v0.5 now permits any
QThread calling python for the first time to automatically set up a
python thread state so you no longer need worry about it. Note that
any threads started before the python DLL is loaded will assume they
reference code in the primary interpreter - this contrasts against
threads normally inheriting the interpreter in force when the start()
method was called for that QThread.

<h3>The Boost.Python Library:</h3>
You will almost certainly make great use of Boost.Python's "python like"
C++ objects which make interacting between the two environments a real
boon. While BPL's documentation isn't great, its reference manual
outlines what the following objects can do: <tt>dict, list, long, numeric,
object, str, tuple</tt>. In particular, you want to read \c object and
then read it a second time - and remember that BPL reminds you that
python isn't C++ by forcing you to do a lot of explicit copy construction.
Instead of throwing a \c error_already_set exception, TnFOX's customised
BPL throws a FX::FXPythonException.

Finally, while it has been intended with the facilities provided here
to cover all common usage without having to resort to the Python C API,
obviously there are limits. If you want to do specialist things, you
will need to dive into the direct API from time to time - remember to
maintain reference counts (look into \c handle<> in BPL). However, I
think that more often than not you will be too quick to "go direct"
when in fact BPL does actually provide an easier solution. Ask on the
C++-SIG mailing list on <a href="http://www.python.org/sigs/">python.org</a>.
\sa FX::FXPython

<h3>Example:</h3>
\code
using namespace FX;
using namespace boost::python;
{
	FXPythonCtxHold ctxhold;
	py("def printNum(no): print no");
	object printNum(pyeval("printNum"));
	long_ bignum((FXulong)-1);
	fxmessage("\nThe biggest number C++ can handle: "); printNum(bignum);
	bignum*=bignum;
	fxmessage("That number multiplied by itself: "); printNum(bignum);
}
\endcode
See the source for the TestEmbedPython test for a lot more of this kind
of code.
*/
struct FXPythonInterpPrivate;
class FXPYTHONAPI FXPythonInterp
{
	friend struct FXPythonInterpPrivate;
	friend class FXPython;
	FXPythonInterpPrivate *p;
	FXPythonInterp(const FXPythonInterp &);
	FXPythonInterp &operator=(const FXPythonInterp &);
	FXPythonInterp(void *ts);
public:
	/*! Creates an instance of the python interpreter optionally with the
	specified arguments and name. If python previously was not initialised, this
	initialises python and returns the default interpreter.
	In all other cases, creates a new interpreter. Note that
	if this initialises python, when the last is destroyed it will deinitialise
	python too.
	\note On return, current context is set to the new interpreter.
	*/
	FXPythonInterp(int argc=0, char **argv=0, const char *name=0);
	~FXPythonInterp();
	//! Returns the name of this python interpreter
	const char *name() const throw();
	//! Returns the python interpreter executing the calling code or 0 if there is none
	static FXPythonInterp *current();
	//! Sets the execution context to this interpreter
	void setContext();
	//! Unsets the execution context. This MUST be matched with a corresponding setContext()
	void unsetContext();
	static void int_enterCPP();
	static void int_exitCPP();
};

/*! \class FXPython_CtxHold
\ingroup python
\brief Wraps context sets in an exception-proof fashion

This little helper class works similarly to FX::QMtxHold in setting
a context on construction and unsetting it on destruction.
You can use undo() and redo() if needed.
*/
class FXPython_CtxHold : public Generic::DoUndo<FXPythonInterp, void (FXPythonInterp::*)(), void (FXPythonInterp::*)()>
{
public:
	//! Constructs an instance setting context to interpreter \em interp, defaulting to the current context
	FXPython_CtxHold(FXPythonInterp *i=FXPythonInterp::current())
		: Generic::DoUndo<FXPythonInterp, void (FXPythonInterp::*)(), void (FXPythonInterp::*)()>(i, &FXPythonInterp::setContext, &FXPythonInterp::unsetContext)
	{ }
};

class FXPython_DoCPP : public Generic::DoUndo<void, void (*)(), void (*)()>
{
public:
	FXPython_DoCPP()
		: Generic::DoUndo<void, void (*)(), void (*)()>(&FXPythonInterp::int_enterCPP, &FXPythonInterp::int_exitCPP)
	{ }
};

class FXPython_DoPython : public Generic::DoUndo<void, void (*)(), void (*)()>
{
public:
	FXPython_DoPython()
		: Generic::DoUndo<void, void (*)(), void (*)()>(&FXPythonInterp::int_exitCPP, &FXPythonInterp::int_enterCPP)
	{ }
};


} // namespace

#endif
#endif
