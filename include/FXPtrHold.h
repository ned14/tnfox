/********************************************************************************
*                                                                               *
*                     A u t o  P o i n t e r  H o l d e r                       *
*                                                                               *
*********************************************************************************
* Copyright (C) 2002,2003 by Niall Douglas.   All Rights Reserved.              *
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

#ifndef FXPTRHOLD_H
#define FXPTRHOLD_H

namespace FX {

/*! \file FXPtrHold.h
\brief Defines a stack allocated auto pointer
*/

/*! \class FXPtrHold
\brief A guaranteed deleted pointer holder

In code using exception handling, a major problem is allocating pointers on the stack
to objects and then correctly freeing them on exception. The solution
is this class which guarantees deletion of new-ed pointers stored inside
it.
\code
FXPtrHold<FXString> temp;
FXERRHM(temp=new FXString);
\endcode
Usage is recommended for any new-ed pointer which doesn't inherit FXObject
(as obviously FXObject manages auto-deletion for its children). Other common uses
I have found is fire-and-forget static allocations.

Why use this instead of <tt>std::auto_ptr</tt> from the STL? Well <tt>auto_ptr</tt>
has the concept of \em ownership of the pointed to data and assignments from one to the
other zeros the copyee. FXPtrHold behaves much more like a vanilla auto pointer -
the sole difference it that it deletes its contents on destruction.

Needless to say, you should zero its contents if you delete them manually. FXDELETE()
helps you here. Remember also that if you do make copies, deleting one means setting
the rest to zero to prevent a run-time error.

\warning Do not think of FXPtrHold as a smart pointer - it's a pointer holder, which
is not the same thing. Thus setting a FXPtrHold to some new value does not delete
its previous contents. Furthermore, implicit conversions to the held pointer are
provided which well-designed smart pointers don't do eg; you can do <tt>delete ptr</tt>.

If you \em do want a smart pointer, see FX::Generic::ptr
*/
template<class type> class FXPtrHold
{
	type *ptr;
public:
	FXPtrHold(type *p=0) throw() : ptr(p) { }
	~FXPtrHold() { free(); }
	operator type *() throw() { return ptr; }
	operator const type *() const throw() { return ptr; }
	type *&operator->() throw() { return ptr; }
	type *&operator=(type *p) throw() { ptr=p; return ptr; }
	void free() { delete ptr; ptr=0; }
};

} // namespace

#endif
