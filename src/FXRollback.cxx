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

#include <qptrlist.h>
#include "FXRollback.h"
#include "FXException.h"
#include "QThread.h"

namespace FX {

void FXRollbackBase::makeCall()
{
	FXEXCEPTIONDESTRUCT1
	{
		FXRollbackBase &me=*this;
		if(mutex)
		{
			QMtxHold h(mutex);
			(me.*calladdr)();
		}
		else
			(me.*calladdr)();
	}
	FXEXCEPTIONDESTRUCT2;
}

FXRollbackGroup::FXRollbackGroup()
{
	QPtrList<FXRollbackBase> *l;
	FXERRHM(list=l=new QPtrList<FXRollbackBase>);
	l->setAutoDelete(true);
}

FXRollbackGroup::~FXRollbackGroup()
{ FXEXCEPTIONDESTRUCT1 {
	QPtrList<FXRollbackBase> *l=static_cast<QPtrList<FXRollbackBase> *>(list);
	delete l;
	list=0;
} FXEXCEPTIONDESTRUCT2; }

void FXRollbackGroup::add(FXRollbackBase &item)
{
	QPtrList<FXRollbackBase> *l=static_cast<QPtrList<FXRollbackBase> *>(list);
	l->append(item.copy());
}

void FXRollbackGroup::dismiss() const throw()
{
	QPtrListIterator<FXRollbackBase> it(*static_cast<QPtrList<FXRollbackBase> *>(list));
	FXRollbackBase *item;
	for(; (item=it.current()); ++it)
	{
		item->dismiss();
	}
}

}
