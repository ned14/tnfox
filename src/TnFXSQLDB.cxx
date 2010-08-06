/********************************************************************************
*                                                                               *
*                              SQL Database Support                             *
*                                                                               *
*********************************************************************************
* Copyright (C) 2005 by Niall Douglas.   All Rights Reserved.                   *
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
#ifndef FX_DISABLESQL

#include "TnFXSQLDB.h"
#include <qdict.h>
#include <qstringlist.h>
#include <qptrvector.h>
#include "FXMemDbg.h"
#if defined(DEBUG) && !defined(FXMEMDBG_DISABLE)
static const char *_fxmemdbg_current_file_ = __FILE__;
#endif

namespace FX {

enum SQLDataType
{
	Null=0,		//!< Null type

	VarChar,	//!< Variable length UTF-8 string
	Char,		//!< Fixed length UTF-8 string
	WVarChar,	//!< Variable length UTF-16 string
	WChar,		//!< Fixed length UTF-16 string

	TinyInt,	//!< Signed 8 bit integer equal to a FX::FXchar
	SmallInt,	//!< Signed 16 bit integer equal to a FX::FXshort
	Integer,	//!< Signed 32 bit integer equal to a FX::FXint
	BigInt,		//!< Signed 64 bit integer equal to a FX::FXlong
	Decimal,	//!< Fixed precision integer number
	Numeric,	//!< Fixed precision integer number

	Real,		//!< Floating point number equal to a FX::FXfloat
	Double,		//!< Floating point number equal to a FX::FXdouble
	Float,		//!< Fixed precision floating point number (equal to either a FX::FXfloat or FX::FXdouble)

	Timestamp,	//!< A date and time
	Date,		//!< A date
	Time,		//!< A time

	BLOB,		//!< A Binary Large Object

	LastSQLDataTypeEntry
};
const char *TnFXSQLDB::sql92TypeAsString(SQLDataType type)
{
	static const char *strs[]={
		"NULL",
		"VARCHAR",
		"CHAR",
		"VARWCHAR",
		"WCHAR",
		"TINYINT",
		"SMALLINT",
		"INTEGER",
		"BIGINT",
		"DECIMAL",
		"NUMERIC",
		"REAL",
		"DOUBLE PRECISION",
		"FLOAT",
		"TIMESTAMP",
		"DATE",
		"TIME",
		"VARBINARY"
	};
	if((FXuint) type<sizeof(strs)/sizeof(const char *))
		return strs[type];
	else
		return 0;
}

struct TnFXSQLDBPrivate
{
	TnFXSQLDB::Capabilities caps;
	const FXString &driverName;		// stored statically
	FXString dbname, user;
	QHostAddress host;
	FXushort port;

	TnFXSQLDBPrivate(TnFXSQLDB::Capabilities _caps, const FXString &_driverName, const FXString &_dbname, const FXString &_user, const QHostAddress &_host, FXushort _port)
		: caps(_caps), driverName(_driverName), dbname(_dbname), user(_user), host(_host), port(_port) { }
};

TnFXSQLDB::TnFXSQLDB(TnFXSQLDB::Capabilities caps, const FXString &driverName, const FXString &dbname, const FXString &user, const QHostAddress &host, FXushort port) : p(0)
{
	FXERRHM(p=new TnFXSQLDBPrivate(caps, driverName, dbname, user, host, port));
}
TnFXSQLDB::~TnFXSQLDB()
{ FXEXCEPTIONDESTRUCT1 {
	FXDELETE(p);
} FXEXCEPTIONDESTRUCT2; }

const FXString &TnFXSQLDB::driverName() const throw()
{
	return p->driverName;
}
TnFXSQLDB::Capabilities TnFXSQLDB::capabilities() const throw()
{
	return p->caps;
}
const FXString &TnFXSQLDB::dbName() const throw()
{
	return p->dbname;
}
void TnFXSQLDB::setDBName(const FXString &dbname)
{
	p->dbname=dbname;
}
const FXString &TnFXSQLDB::user() const throw()
{
	return p->user;
}
void TnFXSQLDB::setUser(const FXString &user)
{
	p->user=user;
}
const QHostAddress &TnFXSQLDB::host() const throw()
{
	return p->host;
}
void TnFXSQLDB::setHost(const QHostAddress &addr)
{
	p->host=addr;
}
FXushort TnFXSQLDB::port() const throw()
{
	return p->port;
}
void TnFXSQLDB::setPort(FXushort port)
{
	p->port=port;
}

TnFXSQLDBCursorRef TnFXSQLDB::execute(const FXString &text, FXuint flags, QWaitCondition *latch)
{
	TnFXSQLDBStatementRef ret=prepare(text);
	return ret->execute(flags, latch);
}
void TnFXSQLDB::immediate(const FXString &text)
{
	TnFXSQLDBStatementRef ret=prepare(text);
	ret->immediate();
}

void TnFXSQLDB::synchronise()
{

}


//*******************************************************************************

struct TnFXSQLDBStatementPrivate
{
	TnFXSQLDB *parent;
	FXString text;
	QPtrVector<QBuffer> unknownBLOBs;

	TnFXSQLDBStatementPrivate(TnFXSQLDB *_parent, const FXString &_text) : parent(_parent), text(_text), unknownBLOBs(true) { }
};

TnFXSQLDBStatement::TnFXSQLDBStatement(TnFXSQLDB *parent, const FXString &text)
{
	FXERRHM(p=new TnFXSQLDBStatementPrivate(parent, text));
}
TnFXSQLDBStatement::TnFXSQLDBStatement(const TnFXSQLDBStatement &o) : p(0)
{
	FXERRHM(p=new TnFXSQLDBStatementPrivate(*o.p));
}
TnFXSQLDBStatement::~TnFXSQLDBStatement()
{ FXEXCEPTIONDESTRUCT1 {
	FXDELETE(p);
} FXEXCEPTIONDESTRUCT2; }

TnFXSQLDB *TnFXSQLDBStatement::driver() const throw()
{
	return p->parent;
}
const FXString &TnFXSQLDBStatement::text() const throw()
{
	return p->text;
}

TnFXSQLDBStatement &TnFXSQLDBStatement::bind(FXint idx, TnFXSQLDB::SQLDataType datatype, void *data)
{
	if((FXuint) idx<p->unknownBLOBs.count())
		p->unknownBLOBs.replace(idx, 0);
	return *this;
}
void TnFXSQLDBStatement::int_bindUnknownBLOB(FXint idx, FXAutoPtr<QBuffer> buff)
{
	if((FXuint) idx>=p->unknownBLOBs.count())
		p->unknownBLOBs.extend(idx+1);
	bind(idx, TnFXSQLDB::BLOB, (void *) &PtrPtr(buff)->buffer());
	p->unknownBLOBs.replace(idx, PtrRelease(buff));
}

//*******************************************************************************

struct TnFXSQLDBCursorPrivate
{
	FXuint flags;
	TnFXSQLDBStatementRef parent;
	QWaitCondition *latch;
	FXuint columns;

	FXint rows, readyBegin, readyEnd;
	bool atEnd;
	FXint crow;
	TnFXSQLDBCursorPrivate(FXuint _flags, TnFXSQLDBStatement *_parent, QWaitCondition *_latch, FXuint _columns)
		: flags(_flags), parent(_parent), latch(_latch), columns(_columns),
		rows(0), readyBegin(0), readyEnd(0), atEnd(false), crow(-1) { }
};

TnFXSQLDBCursor::TnFXSQLDBCursor(FXuint flags, TnFXSQLDBStatement *parent, QWaitCondition *latch, FXuint columns) : p(0)
{
	FXERRHM(p=new TnFXSQLDBCursorPrivate(flags, parent, latch, columns));
}
TnFXSQLDBCursor::TnFXSQLDBCursor(const TnFXSQLDBCursor &o) : p(0)
{
	FXERRHM(p=new TnFXSQLDBCursorPrivate(*o.p));
}
TnFXSQLDBCursor::~TnFXSQLDBCursor()
{ FXEXCEPTIONDESTRUCT1 {
	FXDELETE(p);
} FXEXCEPTIONDESTRUCT2; }
void TnFXSQLDBCursor::int_setInternals(FXint *rows, FXuint *flags, FXuint *columns, QWaitCondition *latch)
{
	if(rows)
	{
		p->rows=*rows;
		if(!p->rows) p->atEnd=true;
	}
	if(flags) p->flags=*flags;
	if(columns) p->columns=*columns;
	if(latch) p->latch=latch;
}
void TnFXSQLDBCursor::int_setRowsReady(FXint start, FXint end)
{
	p->readyBegin=start; p->readyEnd=end;
}
void TnFXSQLDBCursor::int_setAtEnd(bool atend)
{
	p->atEnd=atend;
}

FXuint TnFXSQLDBCursor::flags() const throw()
{
	return p->flags;
}
TnFXSQLDBStatement *TnFXSQLDBCursor::statement() const throw()
{
	return PtrPtr(p->parent);
}
QWaitCondition *TnFXSQLDBCursor::resultsLatch() const throw()
{
	return p->latch;
}
FXuint TnFXSQLDBCursor::columns() const throw()
{
	return p->columns;
}
FXint TnFXSQLDBCursor::rows() const throw()
{
	return p->rows;
}
bool TnFXSQLDBCursor::rowsReady(FXint &start, FXint &end) const throw()
{
	if(start==end) return false;
	start=p->readyBegin; end=p->readyEnd;
	return true;
}

bool TnFXSQLDBCursor::atEnd() const throw()
{
	return p->atEnd;
}
FXint TnFXSQLDBCursor::at() const throw()
{
	return p->crow;
}
FXint TnFXSQLDBCursor::at(FXint newrow)
{
	TnFXSQLDB::Capabilities caps=statement()->driver()->capabilities();
	if(!caps.HasSettableCursor)
	{	// Emulate using forwards() and backwards()
		FXint crow;
		while(newrow<p->crow && (crow=p->crow, crow!=backwards()));
		while(newrow>p->crow && (crow=p->crow, crow!=forwards()));
		return p->crow;
	}
	if(newrow<-1) newrow=-1;
	if(p->rows>=0 && newrow>p->rows) newrow=p->rows;
	return (p->crow=newrow);
}
FXint TnFXSQLDBCursor::backwards()
{
	if(!statement()->driver()->capabilities().HasBackwardsCursor)
	{
		FXERRGNOTSUPP(QTrans::tr("TnFXSQLDB::Statement::Cursor", "This driver does not support moving cursors backwards"));
	}
	if(p->crow<=-1) return p->crow;
	return --p->crow;
}
FXint TnFXSQLDBCursor::forwards()
{
	if(p->rows>=0 && p->crow>=p->rows) return p->crow;
	return ++p->crow;
}

//*******************************************************************************

struct TnFXSQLDBRegistryPrivate
{
	QDict<void> drivers;
};
static TnFXSQLDBRegistry *registry;

void TnFXSQLDBRegistry::int_register(const FXString &name, createSpec create)
{
	p->drivers.insert(name, (void *) create);
}
void TnFXSQLDBRegistry::int_deregister(const FXString &name, createSpec create)
{
	p->drivers.remove(name);
	if(registry==this && p->drivers.isEmpty())
	{
		FXDELETE(registry);
	}
}
TnFXSQLDBRegistry::TnFXSQLDBRegistry() : p(0)
{
	FXERRHM(p=new TnFXSQLDBRegistryPrivate);
}
TnFXSQLDBRegistry::~TnFXSQLDBRegistry()
{ FXEXCEPTIONDESTRUCT1 {
	FXDELETE(p);
} FXEXCEPTIONDESTRUCT2; }

TnFXSQLDBRegistry *TnFXSQLDBRegistry::processRegistry()
{
	if(!registry)
	{
		FXERRHM(registry=new TnFXSQLDBRegistry);
	}
	return registry;
}

QStringList TnFXSQLDBRegistry::drivers() const
{
	QStringList ret;
	for(QDictIterator<void> it(p->drivers); it.current(); ++it)
		ret.push_back(it.currentKey());
	return ret;
}

FXAutoPtr<TnFXSQLDB> TnFXSQLDBRegistry::instantiate(const FXString &name, const FXString &dbname, const FXString &user, const QHostAddress &host, FXushort port) const
{
	createSpec c=(createSpec) p->drivers.find(name);
	if(!c) return FXAutoPtr<TnFXSQLDB>(0);
	return c(dbname, user, host, port);
}


}

#endif
