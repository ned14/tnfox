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

#include "common.h"
#ifdef _MSC_VER
#pragma hdrstop
#endif
#include "FXCodeToPythonCode.h"
#include "../include/qptrdict.h"
#include "../include/qptrlist.h"
#include "../include/FXGLViewer.h"
#include "CArrays.h"
#include "../include/FXMemDbg.h"
#if defined(DEBUG) && defined(FXMEMDBG_H)
static const char *_fxmemdbg_current_file_ = __FILE__;
#endif

// Defines how many sort function vectors there are (per type)
#define SORTFUNCVECTORS 8

namespace FX {

FXString FXPython::interpreterPath()
{
	return FXString(Py_GetProgramFullPath());
}

FXString FXPython::version()
{
	return FXString(Py_GetVersion());
}

FXString FXPython::platform()
{
	return FXString(Py_GetPlatform());
}

FXString FXPython::programName()
{
	return FXString(Py_GetProgramName());
}

void FXPython::setProgramName(const FXString &name)
{
	static FXPtrHold<char> staticname;
	if(staticname) { delete static_cast<char *>(staticname); staticname=0; }
	FXERRHM(staticname=new char[name.length()+1]);
	memcpy(staticname, name.text(), name.length()+1);
	Py_SetProgramName(staticname);
}

void FXPython::setArgv(int argc, char **argv)
{
	PySys_SetArgv(argc, argv);
}

boost::python::handle<> FXPython::globals()
{
	return boost::python::handle<>(
		boost::python::borrowed(PyModule_GetDict(PyImport_AddModule("__main__"))));
}

boost::python::handle<> FXPython::execute(const FXString &code, boost::python::handle<> context)
{
	FXThread_DTHold dthold;
	PyObject *ret;
	FXERRHPY(ret=PyRun_String((char *) code.text(), Py_file_input, context.get(), context.get()));
	return boost::python::handle<>(ret);
}

boost::python::handle<> FXPython::evaluate(const FXString &code, boost::python::handle<> context)
{
	FXThread_DTHold dthold;
	PyObject *ret;
	FXERRHPY(ret=PyRun_String((char *) code.text(), Py_eval_input, context.get(), context.get()));
	return boost::python::handle<>(ret);
}


//**************************************************************************************

struct ThreadData
{
	FXAtomicInt GILcnt;
	Generic::BoundFunctorV *threadcleanupH;
	QPtrList<FXPythonInterp> callstack;
	QValueList<PyThreadState *> tsstack;
	ThreadData() : threadcleanupH(0) { }
};
static FXRWMutex indexlock;
static QPtrDict<ThreadData> index(13, true);
static FXMutex interplistlock;
static QPtrList<FXPythonInterp> interplist;
static bool amEmbeddedInPython=true;

struct FXPythonInterpPrivate
{
	const char *name;
	PyThreadState *ts;
	bool amMaster, myinterpreter, myts;
	FXPythonInterpPrivate(const char *_name, PyThreadState *_ts=0)
		: name(_name), ts(_ts), amMaster(false), myinterpreter(false), myts(false) { }
	static void informPython();
	static void cleanupThread(FXThread *cthread);
	static ThreadData *addThread(FXThread *cthread, PyThreadState *ts, FXPythonInterp *parentinterp=0);
};

void FXPythonInterpPrivate::cleanupThread(FXThread *cthread)
{	// Ensure you don't add anything here which also needs to go into the destructor
	FXMtxHold h(indexlock, true);
	FXMtxHold h2(interplistlock);
	ThreadData *td=index.find(cthread);
	if(td)
	{
		if(td->GILcnt>0)
		{
			//fxmessage("Thread %d Unsetting %p (GILcnt=%d)\n", cthread->id(), PyThreadState_Get(), td->GILcnt);
			PyEval_ReleaseThread(PyThreadState_Get());
			td->GILcnt=0;
		}
		FXPythonInterp *myinterp=td->callstack.getLast();
		assert(myinterp);
		td->threadcleanupH=0;	// Prevent deletion as breaks FXThread's iterator
		delete myinterp;
		index.remove(cthread);
	}
}
ThreadData *FXPythonInterpPrivate::addThread(FXThread *cthread, PyThreadState *ts, FXPythonInterp *parentinterp)
{
	FXERRH(cthread, "FXThread uninitialised - did you use FXThread to create your thread?", 0, FXERRH_ISDEBUG);
	FXPythonInterp *interp;
	if(!ts)
	{	// Derive a thread state from the primary interpreter
		if(!parentinterp) parentinterp=interplist.getFirst();
		ts=PyThreadState_New(parentinterp->p->ts->interp);
		FXRBOp unts=FXRBFunc(PyThreadState_Delete, ts);
		FXERRHM(interp=new FXPythonInterp(ts));
		interp->p->myts=true;
		FXRBOp uni=FXRBNew(interp);
		unts.dismiss();
		FXMtxHold h(interplistlock);
		interplist.append(interp);
		uni.dismiss();
	}
	else
	{
		for(QPtrListIterator<FXPythonInterp> it(interplist); (interp=it.current()); ++it)
		{
			if(interp->p->ts==ts) break;
		}
		if(!interp)
		{
			FXMtxHold h2(interplistlock);
			FXERRHM(interp=new FXPythonInterp(ts));
			FXRBOp uninterp=FXRBNew(interp);
			interplist.append(interp);
			uninterp.dismiss();
		}
	}
	ThreadData *td;
	if(!(td=index.find(cthread)))
	{
		FXERRHM(td=new ThreadData);
		FXRBOp untd=FXRBNew(td);
		index.insert(cthread, td);
		untd.dismiss();
		td->threadcleanupH=cthread->addCleanupCall(Generic::BindFuncN(&FXPythonInterpPrivate::cleanupThread, cthread));
		td->callstack.prepend(interp);	// Push base interpreter
	}
	return td;
}
static void onThreadCreation(FXThread *t)
{	// ie; set the new thread's interpreter to what it was when started
	if(interplist.isEmpty()) return;
	FXPythonInterp *cinterp=FXPythonInterp::current();
	if(cinterp)
	{
		FXPythonInterpPrivate::addThread(t, 0, cinterp);
	}
}
class FXPythonInit
{
public:
	FXPythonInit()
	{
		FXThread::addCreationUpcall(FXThread::CreationUpcallSpec(onThreadCreation));
	}
	~FXPythonInit()
	{
		FXThread::removeCreationUpcall(FXThread::CreationUpcallSpec(onThreadCreation));
	}
};
static FXProcess_StaticInit<FXPythonInit> myinit("FXPython");

FXPythonInterp::FXPythonInterp(void *ts) : p(0)
{
	FXERRHM(p=new FXPythonInterpPrivate("Auto created interpreter", (PyThreadState *) ts));
}

FXPythonInterp::FXPythonInterp(int argc, char **argv, const char *name) : p(0)
{
	FXThread *cthread=FXThread::current();
	PyThreadState *oldts=0;
	FXMtxHold h(indexlock, true);
	FXMtxHold h2(interplistlock);
	bool addTnFOX=false;
	FXRBOp unconstr=FXRBConstruct(this);
	FXERRHM(p=new FXPythonInterpPrivate(name));
	if(!Py_IsInitialized())
	{	// Init overall
		amEmbeddedInPython=false;
		PyEval_InitThreads();
		FXPython::setProgramName(FXProcess::execpath());
		Py_Initialize();
		p->ts=PyThreadState_Get();
		p->amMaster=true;
		FXPython::setArgv(argc, argv);
		addTnFOX=true;
	}
	else
	{	// Despite what the docs might say, you must have a thread state set
		oldts=PyThreadState_Swap(NULL);
		{
			FXPython_CtxHold ctxhold;
			PyThreadState *oldts=PyThreadState_Get();
			FXERRHM(p->ts=Py_NewInterpreter());
			PyThreadState_Swap(oldts);	// Avoid assertion failure
			p->myinterpreter=true;
		}
		addTnFOX=true;
	}
	interplist.append(this);
	ThreadData *td=FXPythonInterpPrivate::addThread(cthread, p->ts, this);
	// Push current context
	td->callstack.prepend(this);
	td->tsstack.prepend(oldts);
	if(!td->GILcnt++ && !p->amMaster)
	{
		PyEval_AcquireThread(p->ts);
	}
	else PyThreadState_Swap(p->ts);
	//fxmessage("Thread %d Setting %p was %p (GILcnt=%d)\n", cthread->id(), p->ts, oldts, td->GILcnt);
	if(addTnFOX) py(
		"# Thanks to Ralf W. Grosse-Kunstleve for this\n"
		"import os, sys\n"
#ifdef USE_POSIX
		"previous_dlopenflags=sys.getdlopenflags()\n"
		"sys.setdlopenflags(0x102)\n"
		"print 'Former flags were',previous_dlopenflags,'now',0x102\n"
		"print 'sys.path=',sys.path\n"
#endif
		// Hack in current directory to module search path
		"sys.path.append(os.getcwd())\n"
		"print 'new sys.path=',sys.path\n"
		"from TnFOX import *\n"
#ifdef USE_POSIX
		"print 'After flags were',sys.getdlopenflags()\n"
		//"sys.setdlopenflags(previous_dlopenflags)\n"
		"del previous_dlopenflags\n"
#endif
		);
	unconstr.dismiss();
}

#ifdef USE_POSIX
static struct TestWhere
{
	TestWhere()
	{
		fxmessage("TestWhere constructed at %p\n", this);
	}
	~TestWhere()
	{
		fxmessage("TestWhere destroyed at %p\n", this);
	}
} testwhere;
#endif

FXPythonInterp::~FXPythonInterp()
{
	FXThread *cthread=FXThread::current();
	FXMtxHold h(indexlock, true);
	FXMtxHold h2(interplistlock);
	ThreadData *td=index.find(cthread);
	if(td)
	{
		if(td->callstack.getLast()==this)
		{
			if(td->threadcleanupH) cthread->removeCleanupCall(td->threadcleanupH);
			FXDELETE(td->threadcleanupH);
			index.remove(cthread);
		}
	}
	// If master is exiting, everything else should already be dead
	if(!p->amMaster)
	{
		FXPython_DoPython ctxhold;
		if(p->myinterpreter)
		{
			PyThreadState *oldts=PyThreadState_Swap(p->ts);
			Py_EndInterpreter(p->ts); p->ts=0;
			PyThreadState_Swap(oldts);
		}
		else if(p->myts)
		{
			PyThreadState_Clear(p->ts);
			PyThreadState_Delete(p->ts); p->ts=0;
		}
	}
	PyThreadState *oldts=p->ts;
	FXDELETE(p);
	interplist.removeRef(this);
	if(interplist.isEmpty() && !amEmbeddedInPython)
	{	// Destruct overall
		PyThreadState_Swap(oldts);	// Prevent assertion error
		Py_Finalize();
	}
}

const char *FXPythonInterp::name() const throw()
{
	return p->name;
}

FXPythonInterp *FXPythonInterp::current()
{
	FXMtxHold h(indexlock, false);
	ThreadData *td=index.find(FXThread::current());
	if(!td) return 0;
	if(td->callstack.isEmpty()) return 0;
	return td->callstack.getFirst();
}

void FXPythonInterp::setContext()
{
	assert(this);
	FXThread *cthread=FXThread::current();
	FXMtxHold h(indexlock, false);
	ThreadData *td=index.find(cthread);
	if(!td)
	{
		FXMtxHold h(indexlock, true);
		td=FXPythonInterpPrivate::addThread(cthread, p->ts, this);
	}
	h.unlock();
	if(!td->GILcnt++)
	{
		PyEval_AcquireLock();
	}
	PyThreadState *oldts=PyThreadState_Swap(p->ts);
	td->tsstack.push_front(oldts);
	td->callstack.prepend(this);
	//fxmessage("Thread %d Setting   %p was %p (GILcnt=%d)\n", cthread->id(), p->ts, oldts, td->GILcnt);
}

void FXPythonInterp::unsetContext()
{
	FXThread *cthread=FXThread::current();
	FXMtxHold h(indexlock, false);
	ThreadData *td=index.find(cthread);
	assert(td);
	h.unlock();
	td->callstack.removeFirst();
	PyThreadState *oldts=td->tsstack.front();
	FXERRH(!td->tsstack.isEmpty(), "unsetContext() called too many times!", 0, FXERRH_ISDEBUG);
	td->tsstack.pop_front();
	//fxmessage("Thread %d Unsetting %p now %p (GILcnt=%d)\n", cthread->id(), p->ts, oldts, td->GILcnt-1);
	PyThreadState_Swap(oldts);
	if(!--td->GILcnt)
	{
		PyEval_ReleaseLock();
	}
}

// Called whenever python crosses into C++ land
void FXPythonInterp::int_enterCPP()
{
	FXThread *cthread=FXThread::current();
	if(!cthread && !FXProcess::instance())
	{	// Seems we're just about to exit - leave the lock alone
		return;
	}
	FXMtxHold h(indexlock, false);
	ThreadData *td=index.find(cthread);
	if(!td)
	{
		FXMtxHold h(indexlock, true);
		td=FXPythonInterpPrivate::addThread(cthread, PyThreadState_Get());
	}
	h.unlock();
	cthread->disableTermination();
	//fxmessage("Thread %d Unsetting %p (GILcnt=%d)\n", cthread->id(), td->callstack.getFirst()->p->ts, td->GILcnt-1);
	if(!--td->GILcnt)
	{
		PyEval_ReleaseThread(td->callstack.getFirst()->p->ts);
	}
}

// Called whenever C++ crosses into Python land
void FXPythonInterp::int_exitCPP()
{
	FXThread *cthread=FXThread::current();
	if(!cthread && !FXProcess::instance())
	{	// Seems we're just about to exit - the lock will be in the right state
		return;
	}
	FXMtxHold h(indexlock, false);
	ThreadData *td=index.find(cthread);
	if(!td)
	{
		FXMtxHold h(indexlock, true);
		td=FXPythonInterpPrivate::addThread(cthread, 0);
	}
	h.unlock();
	if(cthread) cthread->enableTermination();
	if(!td->GILcnt++)
	{
		PyEval_AcquireThread(td->callstack.getFirst()->p->ts);
	}
	//fxmessage("Thread %d Setting   %p (GILcnt=%d)\n", cthread->id(), td->callstack.getFirst()->p->ts, td->GILcnt);
}

//**************************************************************************************

using namespace boost::python;

template<typename fnspec> struct FOXSortFunc
{
	template<typename base, int no> struct vector : public base
	{
		typedef typename Generic::FnInfo<fnspec> functionInfo;
		typedef typename functionInfo::asList functionSpec;
		typedef typename functionInfo::resultType retType;
		typedef typename functionInfo::par1Type itemType;
		static retType function(itemType a, itemType b)
		{
			return call<retType>(base::getParent()->lookupVector(no).ptr(), ptr(a), ptr(b));
		}
	};
};
typedef FXCodeToPythonCode<FOXSortFunc<FXFoldingListSortFunc>::vector, SORTFUNCVECTORS> FXFoldingListVectorsType;
typedef FXCodeToPythonCode<FOXSortFunc<FXIconListSortFunc	>::vector, SORTFUNCVECTORS> FXIconListVectorsType;
typedef FXCodeToPythonCode<FOXSortFunc<FXListSortFunc		>::vector, SORTFUNCVECTORS> FXListVectorsType;
typedef FXCodeToPythonCode<FOXSortFunc<FXTreeListSortFunc	>::vector, SORTFUNCVECTORS> FXTreeListVectorsType;
static FXFoldingListVectorsType	FXFoldingListVectors;
static FXIconListVectorsType	FXIconListVectors;
static FXListVectorsType		FXListVectors;
static FXTreeListVectorsType	FXTreeListVectors;

static void checkIsPythonFunc(object *code, int pars)
{
	assert(code);
	FXERRH(PyCallable_Check(code->ptr()), "Python object must be callable", 0, FXERRH_ISDEBUG);
	FXERRH(extract<int>((*code).attr("func_code").attr("co_argcount"))==pars, "Python code object has an incorrect number of parameters", 0, FXERRH_ISDEBUG);
}

void FXPython::setSortFunc(FXFoldingList &list, boost::python::api::object *code)
{
	checkIsPythonFunc(code, 2);
	list.setSortFunc(FXFoldingListVectors.allocate(code));
}
void FXPython::setSortFunc(FXIconList &list, boost::python::api::object *code)
{
	checkIsPythonFunc(code, 2);
	list.setSortFunc(FXIconListVectors.allocate(code));
}
void FXPython::setSortFunc(FXList &list, boost::python::api::object *code)
{
	checkIsPythonFunc(code, 2);
	list.setSortFunc(FXListVectors.allocate(code));
}
void FXPython::setSortFunc(FXTreeList &list, boost::python::api::object *code)
{
	checkIsPythonFunc(code, 2);
	list.setSortFunc(FXTreeListVectors.allocate(code));
}

template<typename base, int no> struct FXGLViewerVector : public base
{
	typedef typename Generic::FnInfo<FXZSortFunc>::asList functionSpec;
	static FXbool function(FXfloat *&buffer, FXint &used, FXint &size)
	{	// Convert array of floats into a python container emulation (from CArrays.h)
		typedef indexing::iterator_range<FXfloat *> IterPair;
		class_<IterPair>("FXGLViewer_sortFuncIndirect", init<FXfloat *, FXfloat *>())
			.def(indexing::container_suite<IterPair>());
		IterPair pybuffer(buffer, buffer+size);
		return call<FXbool>(base::getParent()->lookupVector(no).ptr(), pybuffer, used, size);
	}
};
static FXCodeToPythonCode<FXGLViewerVector, SORTFUNCVECTORS> FXGLViewerVectors;
void FXPython::setSortFunc(FXGLViewer &list, boost::python::api::object *code)
{
	checkIsPythonFunc(code, 3);
	list.setZSortFunc(FXGLViewerVectors.allocate(code));
}

void FXPython::int_throwPythonException()
{
	assert(PyThreadState_Get());
	PyObject *err=PyErr_Occurred();
	if(!err) return;
	if(PyErr_GivenExceptionMatches(err, PyExc_StopIteration))
	{
#ifdef DEBUG
		fxmessage("StopIteration exception thrown\n");
#endif
		throw FXPythonException();
	}

	FXTransString msg=FXTrans::tr("FXPython", "%3,%4 at line %1 in %2");
	{
		PyObject *_type, *_pvalue, *_ptraceback;
		PyErr_Fetch(&_type, &_pvalue, &_ptraceback);
		PyErr_NormalizeException(&_type, &_pvalue, &_ptraceback);
		if(!_type) return;
		if(_ptraceback)
		{
			handle<> ptraceback(_ptraceback);
			object traceback(ptraceback);
			int lineno=extract<int>(traceback.attr("tb_lineno"));
			str file(traceback.attr("tb_frame").attr("f_code").attr("co_filename"));
			msg.arg(lineno).arg(FXString(extract<const char *>(file)));
		}
		else msg.arg("?").arg("?");
		handle<> type(_type), pvalue(_pvalue);
		str etype(type), eparam(pvalue);
		msg.arg(FXString(extract<const char *>(etype))).arg(FXString(extract<const char *>(eparam)));
	}
	FXPythonException e(msg, 0);
	FXERRH_THROW(e);
}

extern FXPYTHONAPI void *testfunc();
void *testfunc()
{
		printf("Typeinfo of FXException in DLL is %p,%p (%s)\n", &typeid(FXException), typeid(FXException).name(), typeid(FXException).name());
		printf("Typeinfo of FXPythonException in DLL is %p,%p (%s)\n", &typeid(FXPythonException), typeid(FXPythonException).name(), typeid(FXPythonException).name());
		FXERRH_TRY
		{
			py( "print '\\nThrowing a python exception ...'\n"
				"raise IOError, 'A parameter to a python exception'");
		}
		FXERRH_CATCH(FXException &e)
		{
			printf("Typeinfo of caught exception in DLL is %p,%p (%s)\n", &typeid(e), typeid(e).name(), typeid(e).name());
			fxmessage("C++ exception caught: %s\n", e.report().text());
		}
		FXERRH_ENDTRY
	static FXPythonException foo("Hello", 0);
	void *ret=(void *) &foo;
	printf("Typeinfo of pointer to be returned from DLL is %p,%p (%s)\n", &typeid(foo), typeid(foo).name(), typeid(foo).name());
	return ret;
}

void FXPython::int_initEmbeddedEnv()
{
	FXThread *cthread=FXThread::current();
	FXMtxHold h(indexlock, false);
	ThreadData *td=index.find(cthread);
	if(!td)
	{
		FXMtxHold h(indexlock, true);
		td=FXPythonInterpPrivate::addThread(cthread, PyThreadState_Get());
		FXPythonInterp *myinterp=td->callstack.getLast();
		myinterp->p->amMaster=true;
		// Push current context
		td->callstack.prepend(myinterp);
		td->tsstack.prepend(0);
		++td->GILcnt;
	}
}

void FXPython::int_runPythonThread(PyObject *self, FXThread *cthread)
{
	FXPython_DoPython ctxhold;
	handle<> ret(allow_null(PyEval_CallMethod(self, "run", "()")));
	if(!ret)
	{
		if(PyErr_ExceptionMatches(PyExc_SystemExit))
		{
			PyErr_Clear();
			return;
		}
		fxerrhpy(ret.get());
	}
}


}
