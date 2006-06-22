/********************************************************************************
*                                                                               *
*                             Boost.python converters                           *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2003-2006 by Niall Douglas.   All Rights Reserved.       *
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

#include "common.h"

#include "../include/fxver.h"
#include <boost/python/suite/indexing/container_proxy.hpp>
#include <boost/python/suite/indexing/list.hpp>
#include <boost/python/suite/indexing/vector.hpp>


// Define a QValueList converter
namespace boost { namespace python { namespace indexing { namespace detail {
template <class T>
class algorithms_selector<FX::QValueList<T> >
    {
      typedef FX::QValueList<T> Container;

      typedef list_traits<Container>       mutable_traits;
      typedef list_traits<Container const> const_traits;

    public:
      typedef list_algorithms<mutable_traits> mutable_algorithms;
      typedef list_algorithms<const_traits>   const_algorithms;
    };
}}}}
template<typename type> void RegisterConvQValueList()
{
	using namespace FX;
	using namespace boost::python;
	class_< QValueList<type> >(Generic::typeInfo<QValueList<type> >().asIdentifier().text())
		.def(indexing::container_suite< QValueList<type> >());
}

// Define a QPtrList converter
/* TODO - need to access internal std::list
namespace boost { namespace python { namespace indexing { namespace detail {
template <class T, class Allocator>
    class algorithms_selector<std::list<T, Allocator> >
    {
      typedef std::list<T, Allocator> Container;

      typedef list_traits<Container>       mutable_traits;
      typedef list_traits<Container const> const_traits;

    public:
      typedef list_algorithms<mutable_traits> mutable_algorithms;
      typedef list_algorithms<const_traits>   const_algorithms;
    };
}}}}
template<typename type> void RegisterConvQPtrList()
{
	class_< FX::QPtrList<type> >(FX::Generic::typeInfo<FX::QPtrList<type> >().asIdentifier().text())
		.def (indexing::container_suite< std::list<type> >::with_policies(return_internal_reference()));
}
*/

// Define a QMemArray converter
namespace boost { namespace python { namespace indexing { namespace detail {
template <class T>
class algorithms_selector<FX::QMemArray<T> >
    {
      typedef FX::QMemArray<T> Container;

      typedef random_access_sequence_traits<Container>       mutable_traits;
      typedef random_access_sequence_traits<Container const> const_traits;

    public:
      typedef default_algorithms<mutable_traits> mutable_algorithms;
      typedef default_algorithms<const_traits>   const_algorithms;
    };
}}}}
template<typename type> void RegisterConvQMemArray()
{
	using namespace boost::python;
	class_< FX::QMemArray<type> >(FX::Generic::typeInfo<FX::QMemArray<type> >().asIdentifier().text())
		.def(indexing::container_suite< std::vector<type> >());
}

// Define a std::vector converter
template<typename type> void RegisterConvStdVector()
{
	using namespace boost::python;
	class_< std::vector<type> >(FX::Generic::typeInfo<std::vector<type> >().asIdentifier().text())
		.def (indexing::container_suite< std::vector<type> >());
}



//*******************************************************************************
// integral integer type converter

using namespace boost::python;


template<typename type> struct integral_to_python_int : public to_python_converter<type, integral_to_python_int<type> >
{
	static PyObject *convert(type v)
	{
		return PyInt_FromLong((long) v);
	}
};

template<typename type> struct integral_from_python_int
{
	integral_from_python_int()
	{
		converter::registry::push_back(&convertible, &construct, type_id<type>());
	}
	static void *convertible(PyObject *obj)
	{
		return PyInt_Check(obj) ? obj : 0;
	}
	static void construct(PyObject *obj, converter::rvalue_from_python_stage1_data *data)
	{
		void *storage=((converter::rvalue_from_python_storage<type> *) data)->storage.bytes;
		*((long *) storage)=PyInt_AsLong(obj);
		data->convertible=storage;
	}
};

template<typename type> void RegisterConvIntegralInt()
{
	integral_to_python_int<type>();
	integral_from_python_int<type>();
}

//*******************************************************************************
// FXString converter

struct FXString_to_python_str : public to_python_converter<FX::FXString, FXString_to_python_str>
{
	static PyObject *convert(const FX::FXString &str)
	{
#if FOX_MAJOR>1 || (FOX_MAJOR==1 && FOX_MINOR>4)
		return PyUnicode_DecodeUTF8(str.text(), str.length(), "replace");
#else
		return PyString_FromStringAndSize(str.text(), str.length());
#endif
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

#if FOX_MAJOR>1 || (FOX_MAJOR==1 && FOX_MINOR>4)
struct FXString_from_unicode_str
{
	FXString_from_unicode_str()
	{
		converter::registry::push_back(&convertible, &construct, type_id<FX::FXString>());
	}
	static void *convertible(PyObject *obj)
	{
		return PyUnicode_Check(obj) ? obj : 0;
	}
	static void construct(PyObject *obj, converter::rvalue_from_python_stage1_data *data)
	{
		FXnchar *value=(FXnchar *) PyUnicode_AS_UNICODE(obj);
		if(!value) throw_error_already_set();
		void *storage=((converter::rvalue_from_python_storage<FX::FXString> *) data)->storage.bytes;
		new(storage) FX::FXString(value, PyUnicode_Size(obj));
		data->convertible = storage;
	}
};
#endif

static void RegisterConvFXString()
{
	//class_< FX::FXString >("FXString", init< >());
	FXString_to_python_str();
	FXString_from_python_str();
#if FOX_MAJOR>1 || (FOX_MAJOR==1 && FOX_MINOR>4)
	FXString_from_unicode_str();
#endif
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
static QPtrDict<Generic::BoundFunctorV> myobjectinstances;

void FXPython::int_pythonObjectCreated(Generic::BoundFunctorV *detach)
{
	FXPython_DoPython ctxhold;
	myobjectinstances.insert(detach, detach);
	QDICTDYNRESIZE(myobjectinstances);
}
void FXPython::int_pythonObjectDeleted(Generic::BoundFunctorV *detach)
{
	FXPython_DoPython ctxhold;
	myobjectinstances.remove(detach);
	QDICTDYNRESIZE(myobjectinstances);
}

static void DeinitialiseTnFOXPython()
{
	if(myprocess)
	{	// Bit tricky here
		FXPythonInterp::int_exitCPP();	// This silently fails when there's no FXProcess
		PyThreadState *ts=PyThreadState_Get();
		// Decouple all TnFOX objects still outstanding
		Generic::BoundFunctorV *bf;
		for(QPtrDictIterator<Generic::BoundFunctorV> it(myobjectinstances); (bf=it.current());)
		{
			(*bf)();	// This should cause deletion and thus the pointer to be removed
		}
		// Ok now delete our FXProcess
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

    RegisterConvQValueList<FX::FXException>();
	RegisterConvQValueList<FX::FXProcess::MappedFileInfo>();
	RegisterConvQValueList<FX::QTrans::ProvidedInfo>();

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
	/* Create magic functions FXMAPFUNC(), FXMAPFUNCS(), FXMAPTYPE() and FXMAPTYPES()
	to aid usage of FXObject derived classes */
	PyRun_SimpleString(	"def FXMAPFUNC(obj, sel, id, handler):\n"
						"    if not obj.__dict__.has_key('int_FXObjectMsgMappings'): obj.int_FXObjectMsgMappings=[]\n"
						"    obj.int_FXObjectMsgMappings.append(((sel<<16)+id, (sel<<16)+id, handler))\n"
						"def FXMAPFUNCS(obj, sel, idlo, idhi, handler):\n"
						"    if not obj.__dict__.has_key('int_FXObjectMsgMappings'): obj.int_FXObjectMsgMappings=[]\n"
						"    obj.int_FXObjectMsgMappings.append(((sel<<16)+idlo, (sel<<16)+idhi, handler))\n"
						"def FXMAPTYPE(obj, sel, handler):\n"
						"    if not obj.__dict__.has_key('int_FXObjectMsgMappings'): obj.int_FXObjectMsgMappings=[]\n"
						"    obj.int_FXObjectMsgMappings.append(((sel<<16), (sel<<16)+65535, handler))\n"
						"def FXMAPTYPES(obj, sello, selhi, handler):\n"
						"    if not obj.__dict__.has_key('int_FXObjectMsgMappings'): obj.int_FXObjectMsgMappings=[]\n"
						"    obj.int_FXObjectMsgMappings.append(((sello<<16), (selhi<<16)+65535, handler))\n"
						);
}

#ifndef FX_DISABLEGUI
bool FXPython::int_FXObjectHandle(long *ret, FXObject *self, FXObject *sender, FXSelector sel, void *ptrval)
{	// return false to have default impl called
	object me(handle<>(borrowed((PyObject *) self->getPythonObject())));
	if(dict(me.attr("__dict__")).has_key("int_FXObjectMsgMappings"))
	{
		list msgmappings(me.attr("int_FXObjectMsgMappings"));
		for(int n=0; n<msgmappings.attr("__len__")(); n++)
		{
			tuple msgmap(msgmappings[n]);
			FXuint lo=extract<FXuint>(msgmap[0]), hi=extract<FXuint>(msgmap[1]);
			if(lo<=sel && sel<=hi)
			{	// We found a handler, so execute it by directly executing
				// the function of the method instance
				FXString methodname;
				object handlerfunc(msgmap[2].attr("im_func"));
				methodname=extract<const char *>(handlerfunc.attr("__name__"));
				if(ptrval)
				{
					int a=1;
				}
				*ret=call_method<long>(me.ptr(), methodname.text(), ptr(sender), sel, handle<>(opaque_pointer_converter<void *>::convert(ptrval)));
#ifdef DEBUG
				fxmessage("int_FXObjectHandle(%p, %p, %u, %p) returns %lu\n", self, sender, sel, ptrval, *ret);
#endif
				return true;
			}
		}
	}
	return false;
}
#endif

