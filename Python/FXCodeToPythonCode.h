/********************************************************************************
*                                                                               *
*                    Converts a code location into a number                     *
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

#ifndef FXCODETOPYTHONCODE_H
#define FXCODETOPYTHONCODE_H
#include "FXPython.h"
#include "../include/qmemarray.h"
#include "../include/FXException.h"

namespace FX {

#ifdef _MSC_VER
// Disable warning about initialising bases off this
#pragma warning(disable: 4355)
#endif
/*! \class FXCodeToPythonCode
\ingroup python
\brief Allows you to convert calls to C++ code to calls to Python code

I had the idea for this in the shower yesterday and it really does show
the power of TnFOX's generic tools. This class lets you create an array
of code entry points of your choice of parameters & return which can
be attached to code objects within Python. This ability solves one major
failing of Boost.Python which prevents GUI list item sorting in FOX - the
inability to set python code as the sort function. No doubt there are many
other uses too, which is why this class has been made public.

Upon instantiation, this class creates \em length code entry points attached to \em length
python code objects. You allocate a code entry, set it to its corresponding
python code object and pass the returned function pointer to whichever C++
class. All calls made by the C++ class to that function pointer will now
call the python code object. Asking to allocate for an already existing
code object returns the original function pointer ie; it's a once off
allocation. Because the tables must be fixed in size, you must take care
not to have more than the maximum total code objects than the table can hold.

FXCodeToPythonCode is a policy-driven class whereby you must provide a
policy specifying what kind of code we are vectoring:
\code
template<typename base, int no> struct myVector : public base
{
	typedef Generic::TL::create<returnType, firstParType &firstPar, secondParType &secondPar>::value functionSpec;
	static returnType function(firstParType &firstPar, secondParType &secondPar)
	{
		return call<returnType>(getParent()->lookupVector(no).ptr(), boost::ref(firstPar), boost::ref(secondPar));
	}
};
\endcode
ie; the type of the vector as functionSpec and the vector itself following
the same prototype. This should call boost::python::extract() if there is
a return value to extract, otherwise you can safely throw away the return.

Your first reaction will likely be to wonder why the duplication? Unfortunately
the current ISO C++ spec does not let you obtain function types except via
template argument deduction and while you could wrap the policy creation
in such, it gets rapidly very complicated for very little benefit. The above
I believe is a good balance.

\sa FX::FXPython
*/
template<template<typename, int> class vector, int length> class FXCodeToPythonCode
{
	struct vectorBase
	{
		static FXCodeToPythonCode *&getParent()
		{
			static FXCodeToPythonCode *parent;
			return parent;
		}
	};
	typedef typename Generic::FnFromList<typename vector<vectorBase, 0>::functionSpec>::value vectorSpec;
	typedef typename Generic::TL::numberRange<length>::value noRange;
	struct CodeItem
	{
		vectorSpec ptr;
		boost::python::api::object *code;
		CodeItem(vectorSpec _ptr=0) : ptr(_ptr), code(0) { }
	};
	QMemArray<CodeItem> list;
public:
	boost::python::object &lookupVector(int idx)
	{
		boost::python::object *code=list[idx].code;
		assert(code);
		return *code;
	}
private:
	template<typename type> struct CodeAddr;
	template<int no> struct CodeAddr<Generic::IntToType<no> > : public vector<vectorBase, no>
	{
		CodeAddr();
		CodeAddr(FXCodeToPythonCode *_parent) { vector<vectorBase, no>::getParent()=_parent; _parent->list[no]=&vector<vectorBase, no>::function; }
	};
	Generic::TL::instantiateH<noRange, CodeAddr> codevectors;
public:
	//! Creates an instance of the class
	FXCodeToPythonCode() : list(length), codevectors(this) { }
	//! Allocates an entry point
	vectorSpec allocate(boost::python::api::object *code)
	{
		int n, empty=-1;
		for(n=0; n<length; n++)
		{
			CodeItem &ci=list[n];
			if(empty<0 && !ci.code) empty=n;
			if(code==ci.code) return ci.ptr;
		}
		FXERRH(empty>=0, "No more space in list", 0, FXERRH_ISDEBUG);
		list[empty].code=code;
		return list[empty].ptr;
	}
	//! Deallocates an entry point
	bool deallocate(boost::python::api::object *code)
	{
		for(int n=0; n<length; n++)
		{
			CodeItem &ci=list[n];
			if(code==ci.code)
			{
				ci.code=0;
				return true;
			}
		}
		return false;
	}
	//! Deallocates an entry point
	bool deallocate(vectorSpec spec)
	{
		for(int n=0; n<length; n++)
		{
			CodeItem &ci=list[n];
			if(spec==ci.ptr)
			{
				ci.code=0;
				return true;
			}
		}
		return false;
	}
};

/*template<typename base, int no> struct sortFuncVector : public base
{
	typedef Generic::TL::create<FXint, const FXListItem *, const FXListItem *>::value functionSpec;
	static FXint function(const FXListItem *a, const FXListItem *b)
	{
		return extract<FXint>(getParent()->lookupVector(no)(a, b));
	}
};
*/

#ifdef _MSC_VER
#pragma warning(default: 4355)
#endif

}

#endif
