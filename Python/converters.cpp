/********************************************************************************
*                                                                               *
*                             Boost.python converters                           *
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
********************************************************************************/

#include <boost/python.hpp>
#include "../include/fxdefs.h"
#include "../include/FXString.h"
#include "../include/FXException.h"
#include "../include/FXPtrHold.h"
#include "../include/FXProcess.h"
#include "FXPython.h"
#include "CArrays.h"
#include "../include/qvaluelist.h"
#include "../include/qptrlist.h"

using namespace boost::python;

//*******************************************************************************
// FXString converter

struct FXString_to_python_str : public to_python_converter<FX::FXString, FXString_to_python_str>
{
	static PyObject *convert(const FX::FXString &str)
	{
		return PyString_FromStringAndSize(str.text(), str.length());
		//return incref(boost::python::object(str).ptr());
	}
};

struct FXString_from_python_str
{
	FXString_from_python_str()
	{
		converter::registry::push_back(&convertible, &construct, type_id<FX::FXString>());
	}
	static void *convertible(PyObject *obj)
	{
		return PyString_Check(obj) ? obj : 0;
	}
	static void construct(PyObject *obj, converter::rvalue_from_python_stage1_data *data)
	{
		const char *value=PyString_AsString(obj);
		if(!value) throw_error_already_set();
		void *storage=((converter::rvalue_from_python_storage<FX::FXString> *) data)->storage.bytes;
		new(storage) FX::FXString(value, PyString_Size(obj));
		data->convertible = storage;
	}
};

static void RegisterConvFXString()
{
	//class_< FX::FXString >("FXString", init< >());
	FXString_to_python_str();
	FXString_from_python_str();
}

//*******************************************************************************
// FXException converter

static void FXExceptionTranslator(const FX::FXException &e)
{
	if(!e.isValid()) return;	// Means python error is already set
	PyObject *type=0;
	switch(e.code())
	{
	case FX::FXEXCEPTION_NOMEMORY:
		type=PyExc_MemoryError;
		break;
	case FX::FXEXCEPTION_NORESOURCE:
		type=PyExc_EnvironmentError;
		break;
	case FX::FXEXCEPTION_IOERROR:
	case FX::FXEXCEPTION_NOTFOUND:
		type=PyExc_IOError;
		break;
	case FX::FXEXCEPTION_OSSPECIFIC:
		type=PyExc_OSError;
		break;
	default:
		type=PyExc_RuntimeError;
		break;
	}
	PyErr_SetString(type, e.report().text());
}

static void RegisterConvFXException()
{
	register_exception_translator<FX::FXException>(&FXExceptionTranslator);
}

//*******************************************************************************
// Overall initialiser

using namespace FX;

static FXPtrHold<FXProcess> myprocess;

static void DeinitialiseTnFOXPython()
{
	if(myprocess)
	{	// Bit tricky here
		FXPythonInterp::int_exitCPP();	// This silently fails when there's no FXProcess
		PyThreadState *ts=PyThreadState_Get();
		delete static_cast<FXProcess *>(myprocess);
		myprocess=0;
		PyEval_AcquireThread(ts);
#ifdef DEBUG
		fxmessage("*** TnFOXPython DEINITIALISED ***\n");
#endif
	}
}

void InitialiseTnFOXPython()
{
	RegisterConvFXException();
	RegisterConvFXString();
	def("DeinitTnFOX", &DeinitialiseTnFOXPython);

	FXProcess *alreadyinited=FXProcess::instance();
	if(!alreadyinited)
	{
		// TODO: Extract sys.argv and pass to FXProcess
		FXERRHM(myprocess=new FXProcess);
		// Must delete all TnFOX related things before exiting
		PyRun_SimpleString("import atexit\n"
						   "def Internal_CallDeinitTnFOX():\n"
						   "    DeinitTnFOX()\n"
						   "atexit.register(Internal_CallDeinitTnFOX)\n");
		FXPython::int_initEmbeddedEnv();
	}
}

