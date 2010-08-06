/********************************************************************************
*                                                                               *
*                        Python wrappings common header                         *
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

/* Note that this file includes all common headers so it can be precompiled
portably with GCC and MSVC */

#ifdef __FreeBSD__
#include <math.h> // Stop python from disabling math.h float ops on FreeBSD
#endif
#include <fx.h>
#include "generated/__array_1.pypp.hpp"		// Includes boost/python.hpp
#include <boost/python/suite/indexing/value_traits.hpp>

namespace boost { namespace python { namespace indexing {
    template<> struct value_traits<FX::FXException> : public value_traits<int>
    {
        BOOST_STATIC_CONSTANT (bool, equality_comparable = false);
        BOOST_STATIC_CONSTANT (bool, less_than_comparable = false);
    };
    template<> struct value_traits<FX::FXProcess::MappedFileInfo> : public value_traits<int>
    {
        BOOST_STATIC_CONSTANT (bool, equality_comparable = false);
        BOOST_STATIC_CONSTANT (bool, less_than_comparable = false);
    };
    template<> struct value_traits<FX::QTrans::ProvidedInfo> : public value_traits<int>
    {
        BOOST_STATIC_CONSTANT (bool, equality_comparable = false);
        BOOST_STATIC_CONSTANT (bool, less_than_comparable = false);
    };
} } }

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

