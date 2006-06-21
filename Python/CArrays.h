/********************************************************************************
*                                                                               *
*                        Boost.python C Array converter                         *
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

#include <boost/python/suite/indexing/container_proxy.hpp>
#include <boost/python/suite/indexing/list.hpp>
#include <boost/python/suite/indexing/vector.hpp>
#include "../include/qvaluelist.h"
#include "../include/qptrlist.h"
#include "../include/qmemarray.h"


/* Holder which can dispose of a FXMALLOC when Python garbage collects.
It offers an extremely simplified container interface with only read-only
access and length determined by scanning the array till a zero pointer
is found.
*/
template<class type> class FXMallocHolder
{
	type *mydata;
	int len;
	FXMallocHolder(const FXMallocHolder &);
	FXMallocHolder &operator=(const FXMallocHolder &);
public:
	FXMallocHolder(type *data=0) : mydata(data), len(0)
	{
		if(data)
		{
			type *_data=data;
			while(*_data++) len++;
		}
	}
	~FXMallocHolder() { if(mydata) FXFREE(mydata); mydata=0; }
	type getitem(int idx) { return (mydata && idx<len) ? mydata[idx] : 0; }
	int length() const { return len; }
	static inline void regWithBPL();
};
template<class type> inline void FXMallocHolder<type>::regWithBPL()
{
	using namespace FX;
	using namespace boost::python;
	FXString name("FXMallocHolder_");
	name+=Generic::typeInfo<type>().asIdentifier()+"Indirect";
	class_<FXMallocHolder<type>, boost::noncopyable >(name.text())
		.def("__len__", &FXMallocHolder<type>::length)
		.def("__getitem__", &FXMallocHolder<type>::getitem, return_value_policy<reference_existing_object>())
		;
}

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

