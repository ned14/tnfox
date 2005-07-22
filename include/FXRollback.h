/********************************************************************************
*                                                                               *
*                         Transaction Rollback Support                          *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2002,2003 by Niall Douglas.   All Rights Reserved.       *
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

#ifndef FXROLLBACK_H
#define FXROLLBACK_H

#include "fxdefs.h"

namespace FX {

class QMutex;

/*! \file FXRollback.h
\brief Defines classes used in rolling back operations when an error occurs
*/

/*! \defgroup rollbacks Transaction rollback functions
While writing Tornado last year, I kept finding myself writing little
auto-instantiated in-place classes to undo an operation should an exception
occur half way through it. I was experiencing a major problem of exception aware
code - how do you guarantee data-consistency when any statement can throw an
exception?

At the time of writing (June 2003), a majority of programmers just prefer to label
fully exception-aware code as a mixture of being impossible or too much work/overhead
to implement. However, it is my belief that like pervasive multithreading and
multiprocessing, fully exception-aware code will become the defacto standard in
the future. The parts of TnFOX written by me are all fully exception-aware.

Before I began TnFOX, I wanted to implement <i>transaction support</i>. Basically
that means replacing those custom-written little classes with something more
convenient. By sheer coincidence, I happened to fail a job interview with Trolltech
because I didn't know anything about compile time metaprogramming, so this led me
to buying Andrei Alexandrescu's <i>Modern C++ Design</i> which then led me to
think I could solve this by getting the compiler to do it for me.

However, searching on google showed that Andrei Alexandrescu had had exactly the
same problem and in his December 2000 CUJ article he gave an almost entirely
compile-time solution (as much as it could be). From some ideas in that, I have
come up with the functions listed below which can be almost as efficient as Andrei's when no rollbacks
occur (around 15-20 instructions in x86). Furthermore, it offers a considerable
superset of functionality including you not needing to restrict your rollbacks to the local scope.

<h3>Usage:</h3>
Firstly, if you merely want a pointer deleted on scope exit, please see FX::FXPtrHold
which is much more useful than at first sight (it can used statically to clean up
fire-and-forget static allocations). For more complex operations such as the calling
of a specific function or method with certain parameters, then FXRollback is the right choice.

Usage is ridiculously easy. For example:
\code
{
	QPtrList<Item> *mylist;
	// I need to atomically append "item" to three lists
	FXMtxHold h(mylist);
	mylist->append(item);
	FXRBOp undomylist=FXRBObj(*mylist, &QPtrList<Item>::removeRef, item);
	myotherlist->append(item);
	FXRBOp undomyotherlist=FXRBObj(*myotherlist, &QPtrList<Item>::removeRef, item);
	yetotherlist->append(item);
	undomyotherlist.dismiss();
	undomylist.dismiss();
}
\endcode
A good point here is use of the contained scope - it prevents namespace clutter by
making all the extra variable declarations go away. If the append to \c myotherlist
or \c yetotherlist throws an exception, the rollback operations are destructed and
thus the operation is undone - thus data integrity is maintained. For functions:
\code
FXRollbackOp appfatalbit=FXRBFunc(::exit, 1);
... bits which could throw an exception and the app would have to immediately exit
appfatalbit.dismiss(); // It's over
\endcode
Sufficient overloads are provided for four parameters via either method. It's trivial
to extend further however and since all the relevent code is contained in FXRollback.h,
you can just edit the header file (warning: the code is \em really nasty).

\warning All FXRB* functions take \b references, not values. This means that if you're
passing basic types and then altering them with the expectation that the rollback will
restore their original values, you'll find they won't be. Solution: take a copy into
a stack allocated variable and pass those instead.

<h4>Groups:</h4>
You should try to use the above wherever possible as they are written to be
extremely efficient in both code size and run-time overhead (in the normal case,
all code executed is inlined). However there are times when you have a complex
set of variable operations to be undertaken, all of which need to be rolled back
on exception. The efficient use above cannot be used because <i>FXRollbackOp must
be created at the point of declaration</i> ie; you can't declare an empty FXRollbackOp
in a higher scope and assign to it during an if() statement say (this is because
of the way FXRollbackOp is implemented to avoid a virtual destructor call). In
this situation, you should declare a FXRollbackGroup and add to it:
\code
void class::method(Item *i)
{
	FXRollbackGroup rbg;
	if(shouldAlterA)
	{
		alist1.append(i);
		rbg.add(FXRBObj(alist1, &QPtrList<Item>::removeRef, i));
	}
	if(shouldAlterB)
	{
		blist1.append(i);
		rbg.add(FXRBObj(blist1, &QPtrList<Item>::removeRef, i));
	}
	...
	rbg.dismiss();
}
\endcode
This also permits a \c try...finally operation - simply don't dismiss().

As of v0.80, you can pass a null code pointer to disable the operation eg;
\code
int refCount=insertObject(obj);
FXRBOp uninsert=FXRBObj(*this, refCount>1 ? 0 : &Me::killObject, obj);
\endcode
This means you can avoid groups for simple cases (more efficient).

<h3>Exception handling</h3>
Of course the big danger when using destructors to roll back operations
interrupted by an exception being thrown is the dreaded ISO C++ guaranteed
\c std::terminate() if another exception happens. However, TnFOX's superior
nested exception support means that that is guaranteed not to happen with
proper use of \link CppMunge
CppMunge.py
\endlink
<yay!>.

<h3>Exceptions during construction:</h3>
To guard against not cleaning up partially constructed objects, you should
place a \c FXRBConstruct(this) first thing in any constructor which might
throw an exception and use my two-stage object construction idiom. Further
details are given in the doc page for FX::FXException.

\sa FX::FXPtrHold, FX::FXRollbackGroup, FX::Generic::DoUndo
*/
class FXAPI FXRollbackBase
{
	friend class FXRollbackGroup;
protected:
	mutable bool dismissed;
	typedef void (FXRollbackBase::*calladdrtype)();
	calladdrtype calladdr;
	mutable QMutex *mutex;
	virtual FXRollbackBase *copy() const=0;
public:
	FXRollbackBase(calladdrtype addr) : dismissed(false), calladdr(addr), mutex(0) { }
	~FXRollbackBase() { if(!dismissed) makeCall(); }
	FXRollbackBase(const FXRollbackBase &o) : dismissed(o.dismissed), calladdr(o.calladdr), mutex(o.mutex) { o.dismiss(); }
	//! Dismisses the rollback of the operation (ie; you have finished your operation)
	void dismiss() const throw() { dismissed=true; }
	//! Sets a mutex to be locked before an operation is rolled back
	void setMutex(QMutex *m) const throw() { mutex=m; }
	//! \overload
	void setMutex(QMutex &m) const throw() { mutex=&m; }
private:
	void makeCall();

	// No, you really can't do this. Use FXRollbackGroup!
	FXRollbackBase &operator=(const FXRollbackBase &);
};
typedef const FXRollbackBase &FXRBOp;
/*! \class FXRollbackGroup
\brief Lets you place a rollback in a group and/or a non-local scope
\ingroup rollbacks
*/
class FXRollbackGroup
{
	void *list;
public:
	FXRollbackGroup();
	~FXRollbackGroup();
	//! Adds a rollback to this group
	void add(FXRollbackBase &item);
	//! Dismisses this group
	void dismiss() const throw();
};
// For newed allocs
template<typename par1> class FXRBNewI : public FXRollbackBase
{
	par1 &mypar1;
	void call() { delete mypar1; mypar1=0; }
	virtual FXRollbackBase *copy() const { return new FXRBNewI<par1>(*this); }
public:
	FXRBNewI(par1 &_par1) : mypar1(_par1), FXRollbackBase((calladdrtype) &FXRBNewI<par1>::call) { }
};
template<typename par1> FXRBNewI<par1> FXRBNew(par1 &_par1)
{
	return FXRBNewI<par1>(_par1);
}
// For newed array allocs
template<typename par1> class FXRBNewAI : public FXRollbackBase
{
	par1 &mypar1;
	void call() { delete[] mypar1; mypar1=0; }
	virtual FXRollbackBase *copy() const { return new FXRBNewAI<par1>(*this); }
public:
	FXRBNewAI(par1 &_par1) : mypar1(_par1), FXRollbackBase((calladdrtype) &FXRBNewAI<par1>::call) { }
};
template<typename par1> FXRBNewAI<par1> FXRBNewA(par1 &_par1)
{
	return FXRBNewAI<par1>(_par1);
}
// For malloc,calloc allocs
template<typename par1> class FXRBAllocI : public FXRollbackBase
{
	par1 &mypar1;
	void call() { if(mypar1) free(mypar1); mypar1=0; }
	virtual FXRollbackBase *copy() const { return new FXRBAllocI<par1>(*this); }
public:
	FXRBAllocI(par1 &_par1) : mypar1(_par1), FXRollbackBase((calladdrtype)&FXRBAllocI<par1>::call) { }
};
template<typename par1> FXRBAllocI<par1> FXRBAlloc(par1 &_par1)
{
	return FXRBAllocI<par1>(_par1);
}
// For constructors
template<class type> class FXRBConstructI : public FXRollbackBase
{
	type *obj;
	class RedirDelete : public type
	{	// Some sugar to invoke our own operator delete
	public:
		RedirDelete() { }
		void operator delete(void *) throw() { /* do nothing */ }
	};
	void call()
	{
		RedirDelete *_obj=static_cast<RedirDelete *>(obj);
		delete _obj;
	}
	virtual FXRollbackBase *copy() const { return new FXRBConstructI<type>(*this); }
public:
	FXRBConstructI(type *_obj) : obj(_obj), FXRollbackBase((calladdrtype) &FXRBConstructI<type>::call) { }
};
template<class type> FXRBConstructI<type> FXRBConstruct(type *obj)
{
	return FXRBConstructI<type>(obj);
}

#ifdef DOXYGEN_SHOULD_SKIP_THIS
/*! \ingroup rollbacks
This lets you call a specific function
*/
FXRollbackOp FXRBFunc(function, [, par1 & [, par2 & [, par3 & [, par4 &]]]]);
/*! \ingroup rollbacks
This lets you call a specific method in an object
*/
FXRollbackOp FXRBObj(object, ptrtomethod, [, par1 & [, par2 & [, par3 & [, par4 &]]]]);
/*! \ingroup rollbacks
This lets you delete a previously new'ed item. You can also use FX::FXPtrHold for this.
\note The variable you pass is set to zero if rolled back
*/
FXRollbackOp FXRBNew(ptrtonewedalloc &);
/*! \ingroup rollbacks
This lets you delete a previously new'ed array item eg; new int[]
\note The variable you pass is set to zero if rolled back
*/
FXRollbackOp FXRBNewA(ptrtonewedalloc &);
/*! \ingroup rollbacks
This lets you delete a previously \c malloc() or \c calloc() allocated block of memory
\note The variable you pass is set to zero if rolled back
*/
FXRollbackOp FXRBAlloc(ptrtomem);
/*! \ingroup rollbacks
This lets you roll back a failed object construction
*/
FXRollbackOp FXRBConstruct(thisptr);
#else
// For 0 parameters
template<typename func> class FXRBFuncI0 : public FXRollbackBase
{
	func myfunc;
	void call() { if(myfunc) (*myfunc)(); }
	virtual FXRollbackBase *copy() const { return new FXRBFuncI0<func>(*this); }
public:
	FXRBFuncI0(func _myfunc) : myfunc(_myfunc), FXRollbackBase((calladdrtype) &FXRBFuncI0<func>::call) { }
};
template<typename func> FXRBFuncI0<func> FXRBFunc(func _func)
{
	return FXRBFuncI0<func>(_func);
}
// For 1 parameter
template<typename func, typename par1> class FXRBFuncI1 : public FXRollbackBase
{
	func myfunc;
	const par1 &mypar1;
	void call() { if(myfunc) myfunc(mypar1); }
	virtual FXRollbackBase *copy() const { return new FXRBFuncI1<func, par1>(*this); }
public:
	FXRBFuncI1(func _myfunc, const par1 &_par1) : myfunc(_myfunc), mypar1(_par1), FXRollbackBase((calladdrtype) &FXRBFuncI1<func, par1>::call) { }
};
template<typename func, typename par1> FXRBFuncI1<func, par1> FXRBFunc(func _func, const par1 &_par1)
{
	return FXRBFuncI1<func, par1>(_func, _par1);
}
// For 2 parameters
template<typename func, typename par1, typename par2> class FXRBFuncI2 : public FXRollbackBase
{
	func myfunc;
	const par1 &mypar1;
	const par2 &mypar2;
	void call() { if(myfunc) myfunc(mypar1, mypar2); }
	virtual FXRollbackBase *copy() const { return new FXRBFuncI2<func, par1, par2>(*this); }
public:
	FXRBFuncI2(func _myfunc, const par1 &_par1, const par2 &_par2) : myfunc(_myfunc), mypar1(_par1), mypar2(_par2), FXRollbackBase((calladdrtype) &FXRBFuncI2<func, par1, par2>::call) { }
};
template<typename func, typename par1, typename par2> FXRBFuncI2<func, par1, par2> FXRBFunc(func _func, const par1 &_par1, const par2 &_par2)
{
	return FXRBFuncI2<func, par1, par2>(_func, _par1, _par2);
}
// For 3 parameters
template<typename func, typename par1, typename par2, typename par3> class FXRBFuncI3 : public FXRollbackBase
{
	func myfunc;
	const par1 &mypar1;
	const par2 &mypar2;
	const par3 &mypar3;
	void call() { if(myfunc) myfunc(mypar1, mypar2, mypar3); }
	virtual FXRollbackBase *copy() const { return new FXRBFuncI3<func, par1, par2, par3>(*this); }
public:
	FXRBFuncI3(func _myfunc, const par1 &_par1, const par2 &_par2, const par3 &_par3)
		: myfunc(_myfunc), mypar1(_par1), mypar2(_par2), mypar3(_par3), FXRollbackBase((calladdrtype) &FXRBFuncI3<func, par1, par2, par3>::call) { }
};
template<typename func, typename par1, typename par2, typename par3> FXRBFuncI3<func, par1, par2, par3> FXRBFunc(func _func, const par1 &_par1, const par2 &_par2, const par3 &_par3)
{
	return FXRBFuncI3<func, par1, par2, par3>(_func, _par1, _par2, _par3);
}
// For 4 parameters
template<typename func, typename par1, typename par2, typename par3, typename par4> class FXRBFuncI4 : public FXRollbackBase
{
	func myfunc;
	const par1 &mypar1;
	const par2 &mypar2;
	const par3 &mypar3;
	const par4 &mypar4;
	void call() { if(myfunc) myfunc(mypar1, mypar2, mypar3, mypar4); }
	virtual FXRollbackBase *copy() const { return new FXRBFuncI4<func, par1, par2, par3, par4>(*this); }
public:
	FXRBFuncI4(func _myfunc, const par1 &_par1, const par2 &_par2, const par3 &_par3, const par4 &_par4)
		: myfunc(_myfunc), mypar1(_par1), mypar2(_par2), mypar3(_par3), mypar4(_par4), FXRollbackBase((calladdrtype) &FXRBFuncI4<func, par1, par2, par3, par4>::call) { }
};
template<typename func, typename par1, typename par2, typename par3, typename par4> FXRBFuncI4<func, par1, par2, par3, par4> FXRBFunc(func _func, const par1 &_par1, const par2 &_par2, const par3 &_par3, const par4 &_par4)
{
	return FXRBFuncI4<func, par1, par2, par3, par4>(_func, _par1, _par2, _par3, _par4);
}

// For 0 parameters
template<class obj, typename method> class FXRBObjI0 : public FXRollbackBase
{
	obj &myobj;
	method mymethod;
	void call() { if(mymethod) (myobj.*mymethod)(); }
	virtual FXRollbackBase *copy() const { return new FXRBObjI0<obj, method>(*this); }
public:
	FXRBObjI0(obj &_myobj, method _mymethod)
		: myobj(_myobj), mymethod(_mymethod), FXRollbackBase((calladdrtype) &FXRBObjI0<obj, method>::call) { }
};
template<class obj, typename method> FXRBObjI0<obj, method> FXRBObj(obj &_obj, method _method)
{
	return FXRBObjI0<obj, method>(_obj, _method);
}
// For 1 parameter
template<class obj, typename method, typename par1> class FXRBObjI1 : public FXRollbackBase
{
	obj &myobj;
	method mymethod;
	const par1 &mypar1;
	void call() { if(mymethod) (myobj.*mymethod)(mypar1); }
	virtual FXRollbackBase *copy() const { return new FXRBObjI1<obj, method, par1>(*this); }
public:
	FXRBObjI1(obj &_myobj, method _mymethod, const par1 &_mypar1)
		: myobj(_myobj), mymethod(_mymethod), mypar1(_mypar1), FXRollbackBase((calladdrtype) &FXRBObjI1<obj, method, par1>::call) { }
};
template<class obj, typename method, typename par1> FXRBObjI1<obj, method, par1> FXRBObj(obj &_obj, method _method, const par1 &_par1)
{
	return FXRBObjI1<obj, method, par1>(_obj, _method, _par1);
}
// For 2 parameters
template<class obj, typename method, typename par1, typename par2> class FXRBObjI2 : public FXRollbackBase
{
	obj &myobj;
	method mymethod;
	const par1 &mypar1;
	const par2 &mypar2;
	void call() { if(mymethod) (myobj.*mymethod)(mypar1, mypar2); }
	virtual FXRollbackBase *copy() const { return new FXRBObjI2<obj, method, par1, par2>(*this); }
public:
	FXRBObjI2(obj &_myobj, method _mymethod, const par1 &_mypar1, const par2 &_mypar2)
		: myobj(_myobj), mymethod(_mymethod), mypar1(_mypar1), mypar2(_mypar2), FXRollbackBase((calladdrtype) &FXRBObjI2<obj, method, par1, par2>::call) { }
};
template<class obj, typename method, typename par1, typename par2> FXRBObjI2<obj, method, par1, par2> FXRBObj(obj &_obj, method _method, const par1 &_par1, const par2 &_par2)
{
	return FXRBObjI2<obj, method, par1, par2>(_obj, _method, _par1, _par2);
}
// For 3 parameters
template<class obj, typename method, typename par1, typename par2, typename par3> class FXRBObjI3 : public FXRollbackBase
{
	obj &myobj;
	method mymethod;
	const par1 &mypar1;
	const par2 &mypar2;
	const par3 &mypar3;
	void call() { if(mymethod) (myobj.*mymethod)(mypar1, mypar2, mypar3); }
	virtual FXRollbackBase *copy() const { return new FXRBObjI3<obj, method, par1, par2, par3>(*this); }
public:
	FXRBObjI3(obj &_myobj, method _mymethod, const par1 &_mypar1, const par2 &_mypar2, const par3 &_mypar3)
		: myobj(_myobj), mymethod(_mymethod), mypar1(_mypar1), mypar2(_mypar2), mypar3(_mypar3), FXRollbackBase((calladdrtype) &FXRBObjI3<obj, method, par1, par2, par3>::call) { }
};
template<class obj, typename method, typename par1, typename par2, typename par3> FXRBObjI3<obj, method, par1, par2, par3> FXRBObj(obj &_obj, method _method, const par1 &_par1, const par2 &_par2, const par3 &_par3)
{
	return FXRBObjI3<obj, method, par1, par2, par3>(_obj, _method, _par1, _par2, _par3);
}
// For 4 parameters
template<class obj, typename method, typename par1, typename par2, typename par3, typename par4> class FXRBObjI4 : public FXRollbackBase
{
	obj &myobj;
	method mymethod;
	const par1 &mypar1;
	const par2 &mypar2;
	const par3 &mypar3;
	const par4 &mypar4;
	void call() { if(mymethod) (myobj.*mymethod)(mypar1, mypar2, mypar3, mypar4); }
	virtual FXRollbackBase *copy() const { return new FXRBObjI4<obj, method, par1, par2, par3, par4>(*this); }
public:
	FXRBObjI4(obj &_myobj, method _mymethod, const par1 &_mypar1, const par2 &_mypar2, const par3 &_mypar3, const par4 &_mypar4)
		: myobj(_myobj), mymethod(_mymethod), mypar1(_mypar1), mypar2(_mypar2), mypar3(_mypar3), mypar4(_mypar4), FXRollbackBase((calladdrtype) &FXRBObjI4<obj, method, par1, par2, par3, par4>::call) { }
};
template<class obj, typename method, typename par1, typename par2, typename par3, typename par4> FXRBObjI4<obj, method, par1, par2, par3, par4> FXRBObj(obj &_obj, method _method, const par1 &_par1, const par2 &_par2, const par3 &_par3, const par4 &_par4)
{
	return FXRBObjI4<obj, method, par1, par2, par3, par4>(_obj, _method, _par1, _par2, _par3, _par4);
}
#endif

} // namespace

#endif
