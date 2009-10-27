/********************************************************************************
*                                                                               *
*                          Q t   A r r a y   t h u n k                          *
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

#ifndef QMEMARRAY_H
#define QMEMARRAY_H

#include <vector>
#include <algorithm>
#include "fxdefs.h"
#include "FXException.h"
#include "FXStream.h"

namespace FX {

#if _MSC_VER==1200
#pragma warning(disable: 4786)
#endif

/*! \file qmemarray.h
\brief Defines a thunk of Qt's QMemArray to the STL
*/

namespace QGArray {
typedef int Optimization;
}

/*! \class QMemArray
\ingroup QTL
\brief A thunk of Qt's QMemArray to the STL

To aid porting of Qt programs to FOX.

Note that the following are not quite the same:
\li data() assumes that the STL's std::vector<> class stores its elements contiguously
(as does begin() and end()). This isn't guaranteed by the ISO standard (though it is
in a standard amendment, so it shall be very soon).
\li Setting QMemArray to use an external array is mostly supported, but not
entirely (some methods aren't implemented).
\li Useful functions such as push_back() and pop_back() are implemented
*/
template<typename type, class allocator=FX::aligned_allocator<type, 0> > class QMemArray;
template<typename type, class allocator> class QMemArray : private std::vector<type, allocator>
{
	typedef std::vector<type, allocator> Base;
private:
	type *extArray;
	FXuval extArrayLen;
	bool noDeleteExtArray;
	typename Base::const_iterator int_retIndex(const type &d) const
	{
		typename Base::const_iterator it;
		for(it=begin(); it!=end(); ++it)
			if(*it==d) return it;
		return it;
	}
public:
	typedef type * Iterator;
	typedef type * iterator;
	typedef const type * ConstIterator;
	typedef const type * const_iterator;
	typedef type ValueType;
	//! Constructs an empty array of \em type
	QMemArray() : extArray(0), extArrayLen(0), noDeleteExtArray(false), Base() { }
	~QMemArray()
	{
		if(extArray && !noDeleteExtArray)
		{
			delete[] extArray;
			extArray=0;
		}
	}
	//! Constructs an array of \em type \em size long
	QMemArray(FXuval size) : extArray(0), extArrayLen(0), noDeleteExtArray(false), Base(size) { }
	//! Constructs an array using an external array
	QMemArray(type *a, FXuval n, bool _noDeleteExtArray=true) : extArray(a), extArrayLen(n), noDeleteExtArray(_noDeleteExtArray) { }
	QMemArray(const QMemArray<type, allocator> &o) : extArray(o.extArray), extArrayLen(o.extArrayLen), noDeleteExtArray(o.noDeleteExtArray), Base(o) { }
	QMemArray<type, allocator> &operator=(const QMemArray<type, allocator> &o) { extArray=o.extArray; extArrayLen=o.extArrayLen; noDeleteExtArray=o.noDeleteExtArray; Base::operator=(o); return *this; }
	FXADDMOVEBASECLASS(QMemArray, Base)

	using Base::capacity;
	//using Base::clear;
	using Base::empty;
	using Base::reserve;

	//! Returns a pointer to the array
	type *data() const { return (extArray) ? extArray : const_cast<type *>(&Base::front()); }
	//! \overload
	operator const type *() const { return data(); }
	//! Returns the number of elements in the array
	FXuval size() const { return (extArray) ? extArrayLen : Base::size(); }
	//! \overload
	FXuval count() const { return (extArray) ? extArrayLen : Base::size(); }
	//! Returns true if the array is empty
	bool isEmpty() const { return isNull(); }
	//! Returns true if the array is empty
	bool isNull() const { return (extArray) ? extArrayLen!=0 : Base::empty(); }
	//! Resizes the array
	bool resize(FXuval size) { FXEXCEPTION_STL1 { Base::resize(size); } FXEXCEPTION_STL2; return true; }
	//! \overload
	bool resize(FXuval size, QGArray::Optimization optim) { Base::resize(size); return true; }
	//! \overload
	bool truncate(FXuval pos) { FXEXCEPTION_STL1 { Base::resize(pos); } FXEXCEPTION_STL2; return true; }
	//! Swaps the contents of two arrays
	void swap(QMemArray<type, allocator> &o)
	{
		Base::swap(o);
		type *_extArray=o.extArray;
		FXuval _extArrayLen=o.extArrayLen;
		bool _noDeleteExtArray=o.noDeleteExtArray;
		o.extArray=extArray;
		o.extArrayLen=extArrayLen;
		o.noDeleteExtArray=noDeleteExtArray;
		extArray=_extArray;
		extArrayLen=_extArrayLen;
		noDeleteExtArray=_noDeleteExtArray;
	}
	//! Fills the array with value \em val
	bool fill(const type &val, int newsize=-1)
	{
		if(-1!=newsize) resize(newsize);
		for(FXuval n=0; n<size(); n++)
		{
			at(n)=val;
		}
		return true;
	}
	//! \deprecated For Qt compatibility only
	FXDEPRECATEDEXT void detach() { }
	//! \deprecated For Qt compatibility only
	FXDEPRECATEDEXT QMemArray<type, allocator> copy() const { return *this; }
	//! \deprecated For Qt compatibility only
	FXDEPRECATEDEXT QMemArray<type, allocator> &assign(const QMemArray<type, allocator> &o) { Base::operator=(o); return *this; }
	//! \overload
	FXDEPRECATEDEXT QMemArray<type, allocator> &assign(const type *a, FXuval n) { return setRawData(a, n); }
	//! \deprecated For Qt compatibility only
	FXDEPRECATEDEXT QMemArray<type, allocator> &duplicate(const QMemArray<type, allocator> &o) { Base::operator=(o); return *this; }
	//! \overload
	FXDEPRECATEDEXT QMemArray<type, allocator> &duplicate(const type *a, FXuval n) { return setRawData(a, n); }
	/*! Sets QMemArray<> to use an external array. Note that not all methods are implemented for this
	(see the header file). Note also that like Qt's version, you must not resize or reassign the array
	when in this state. Failure to call resetRawData() before destruction causes \c delete on the data
	unless you set \em _noDeleteExtArray.
	*/
	QMemArray<type, allocator> &setRawData(const type *a, FXuval n, bool _noDeleteExtArray=false)
	{
		extArray=const_cast<type *>(a); extArrayLen=n;
		noDeleteExtArray=_noDeleteExtArray;
		return *this;
	}
	//! Resets QMemArray<> to use its internally managed data.
	void resetRawData(const type *a, FXuval n) { extArray=0; extArrayLen=0; noDeleteExtArray=false; }
	//! Returns the index of \em val starting the search from \em i, returning -1 if not found
	FXival find(const type &val, FXuval i=0) const
	{
		for(; i<size(); i++)
		{
			if(at(i)==val) return i;
		}
		return -1;
	}
	//! Returns the number of times \em val is in the array
	FXival contains(const type &val) const
	{
		FXuval count=0;
		for(FXuval n=0; n<size(); n++)
		{
			if(at(n)==val) count++;
		}
		return count;
	}
private:
	static bool sortPredicate(const type &a, const type &b)
	{
		return a<b;
	}
public:
	//! Sorts the array into numerical order
	void sort()
	{
		if(extArray) throw "Not implemented for external arrays";
		std::sort(Base::begin(), Base::end(), sortPredicate);		
	}
	//! Performs a binary search to find an item (needs a numerically sorted array), returning -1 if not found
	int bsearch(const type &val) const
	{	// God damn std::binary_search doesn't return the position :(
		int lbound=0, ubound=(int)size()-1, c;
		while(lbound<=ubound)
		{
			c=(lbound+ubound)/2;
			const type &cval=at(c);
			if(val==cval)
				return c;
			else if(val<cval)
				ubound=c-1;
			else
				lbound=c+1;
		}
		return -1;
	}
	//! Returns a reference to the element at index \em i in the array
	type &at(FXuval i) const { return (extArray) ? extArray[i] : const_cast<type &>(Base::at(i)); }
	//! \overload
	type &operator[](FXuint i) const { return at(i); }
	//! \overload
	type &operator[](int i) const { return at((FXuval) i); }
	bool operator==(const QMemArray<type, allocator> &o) const
	{
		if(extArray) throw "Not implemented for external arrays";
		const Base &me=*this;
		return me==o;
	}
	bool operator!=(const QMemArray<type, allocator> &o) const
	{
		if(extArray) throw "Not implemented for external arrays";
		const Base &me=*this;
		return me!=o;
	}
	//! Returns an iterator pointing to the first element
	Iterator begin() { return data(); }
	//! Returns an iterator pointing one past the last element
	Iterator end() { return data()+size(); }
	//! Returns a const iterator pointint to the first element
	ConstIterator begin() const { return data(); }
	//! Returns a const iterator pointing one past the last element
	ConstIterator end() const { return data()+size(); }

	//! Appends an item onto the end
	void push_back(const type &v) { FXEXCEPTION_STL1 { Base::push_back(v); } FXEXCEPTION_STL2; }
	//! \overload
	void append(const type &v) { push_back(v); }
	//! Removes an item from the end
	using Base::pop_back;
};

namespace QMemArrayImpl
{
	template<bool isPOD, typename type, class allocator> struct Serialise
	{
		Serialise(FXStream &s, const QMemArray<type, allocator> &a)
		{
			FXulong mysize=a.size();
			s << mysize;
			for(FXuval i=0; i<mysize; i++)
				s << a.at(i);
		}
	};
	template<typename type, class allocator> struct Serialise<true, type, allocator>
	{
		Serialise(FXStream &s, const QMemArray<type, allocator> &a)
		{
			FXulong mysize=a.size();
			s << mysize;
			s.save(a.data(), (unsigned long) mysize);
		}
	};
	template<bool isPOD, typename type, class allocator> struct Deserialise
	{
		Deserialise(FXStream &s, QMemArray<type, allocator> &a)
		{
			FXulong mysize;
			s >> mysize;
			a.resize((FXuval)mysize);
			for(FXuval i=0; i<mysize; i++)
				s >> a.at(i);
		}
	};
	template<typename type, class allocator> struct Deserialise<true, type, allocator>
	{
		Deserialise(FXStream &s, QMemArray<type, allocator> &a)
		{
			FXulong mysize;
			s >> mysize;
			a.resize((FXuval)mysize);
			s.load(a.data(), (unsigned long) mysize);
		}
	};
}

//! Writes the contents of the array to stream \em s
template<typename type, class allocator> FXStream &operator<<(FXStream &s, const QMemArray<type, allocator> &a)
{
	QMemArrayImpl::Serialise<Generic::TraitsBasic<type>::isPOD, type, allocator>(s, a);
	return s;
}
//! Reads an array from stream \em s
template<typename type, class allocator> FXStream &operator>>(FXStream &s, QMemArray<type, allocator> &a)
{
	QMemArrayImpl::Deserialise<Generic::TraitsBasic<type>::isPOD, type, allocator>(s, a);
	return s;
}

//! For Qt 2.x compatibility
#define QArray QMemArray

} // namespace

#endif
