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

#include "FXSQLDB.h"
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
const char *FXSQLDB::sql92TypeAsString(SQLDataType type)
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

struct FXSQLDBPrivate
{
	FXSQLDB::Capabilities caps;
	const FXString &driverName;		// stored statically
	FXString dbname, user;
	QHostAddress host;
	FXushort port;

	FXSQLDBPrivate(FXSQLDB::Capabilities _caps, const FXString &_driverName, const FXString &_dbname, const FXString &_user, const QHostAddress &_host, FXushort _port)
		: caps(_caps), driverName(_driverName), dbname(_dbname), user(_user), host(_host), port(_port) { }
};

FXSQLDB::FXSQLDB(FXSQLDB::Capabilities caps, const FXString &driverName, const FXString &dbname, const FXString &user, const QHostAddress &host, FXushort port) : p(0)
{
	FXERRHM(p=new FXSQLDBPrivate(caps, driverName, dbname, user, host, port));
}
FXSQLDB::~FXSQLDB()
{ FXEXCEPTIONDESTRUCT1 {
	FXDELETE(p);
} FXEXCEPTIONDESTRUCT2; }

const FXString &FXSQLDB::driverName() const throw()
{
	return p->driverName;
}
FXSQLDB::Capabilities FXSQLDB::capabilities() const throw()
{
	return p->caps;
}
const FXString &FXSQLDB::dbName() const throw()
{
	return p->dbname;
}
void FXSQLDB::setDBName(const FXString &dbname)
{
	p->dbname=dbname;
}
const FXString &FXSQLDB::user() const throw()
{
	return p->user;
}
void FXSQLDB::setUser(const FXString &user)
{
	p->user=user;
}
const QHostAddress &FXSQLDB::host() const throw()
{
	return p->host;
}
void FXSQLDB::setHost(const QHostAddress &addr)
{
	p->host=addr;
}
FXushort FXSQLDB::port() const throw()
{
	return p->port;
}
void FXSQLDB::setPort(FXushort port)
{
	p->port=port;
}

FXSQLDBCursorRef FXSQLDB::execute(const FXString &text, FXuint flags, QWaitCondition *latch)
{
	FXSQLDBStatementRef ret=prepare(text);
	return ret->execute(flags, latch);
}
void FXSQLDB::immediate(const FXString &text)
{
	FXSQLDBStatementRef ret=prepare(text);
	ret->immediate();
}

void FXSQLDB::synchronise()
{

}


//*******************************************************************************

struct FXSQLDBStatementPrivate
{
	FXSQLDB *parent;
	FXString text;
	QPtrVector<QBuffer> unknownBLOBs;

	FXSQLDBStatementPrivate(FXSQLDB *_parent, const FXString &_text) : parent(_parent), text(_text), unknownBLOBs(true) { }
};

FXSQLDBStatement::FXSQLDBStatement(FXSQLDB *parent, const FXString &text)
{
	FXERRHM(p=new FXSQLDBStatementPrivate(parent, text));
}
FXSQLDBStatement::FXSQLDBStatement(const FXSQLDBStatement &o) : p(0)
{
	FXERRHM(p=new FXSQLDBStatementPrivate(*o.p));
}
FXSQLDBStatement::~FXSQLDBStatement()
{ FXEXCEPTIONDESTRUCT1 {
	FXDELETE(p);
} FXEXCEPTIONDESTRUCT2; }

FXSQLDB *FXSQLDBStatement::driver() const throw()
{
	return p->parent;
}
const FXString &FXSQLDBStatement::text() const throw()
{
	return p->text;
}

FXSQLDBStatement &FXSQLDBStatement::bind(FXint idx, FXSQLDB::SQLDataType datatype, void *data)
{
	if((FXuint) idx<p->unknownBLOBs.count())
		p->unknownBLOBs.replace(idx, 0);
	return *this;
}
void FXSQLDBStatement::int_bindUnknownBLOB(FXint idx, FXAutoPtr<QBuffer> buff)
{
	if((FXuint) idx>=p->unknownBLOBs.count())
		p->unknownBLOBs.extend(idx+1);
	bind(idx, FXSQLDB::BLOB, (void *) &PtrPtr(buff)->buffer());
	p->unknownBLOBs.replace(idx, PtrRelease(buff));
}

//*******************************************************************************

struct FXSQLDBCursorPrivate
{
	FXuint flags;
	FXSQLDBStatementRef parent;
	QWaitCondition *latch;
	FXuint columns;

	FXint rows, readyBegin, readyEnd;
	bool atEnd;
	FXint crow;
	FXSQLDBCursorPrivate(FXuint _flags, FXSQLDBStatement *_parent, QWaitCondition *_latch, FXuint _columns)
		: flags(_flags), parent(_parent), latch(_latch), columns(_columns),
		rows(0), readyBegin(0), readyEnd(0), atEnd(false), crow(-1) { }
};

FXSQLDBCursor::FXSQLDBCursor(FXuint flags, FXSQLDBStatement *parent, QWaitCondition *latch, FXuint columns) : p(0)
{
	FXERRHM(p=new FXSQLDBCursorPrivate(flags, parent, latch, columns));
}
FXSQLDBCursor::FXSQLDBCursor(const FXSQLDBCursor &o) : p(0)
{
	FXERRHM(p=new FXSQLDBCursorPrivate(*o.p));
}
FXSQLDBCursor::~FXSQLDBCursor()
{ FXEXCEPTIONDESTRUCT1 {
	FXDELETE(p);
} FXEXCEPTIONDESTRUCT2; }
void FXSQLDBCursor::int_setInternals(FXint *rows, FXuint *flags, FXuint *columns, QWaitCondition *latch)
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
void FXSQLDBCursor::int_setRowsReady(FXint start, FXint end)
{
	p->readyBegin=start; p->readyEnd=end;
}
void FXSQLDBCursor::int_setAtEnd(bool atend)
{
	p->atEnd=atend;
}

FXuint FXSQLDBCursor::flags() const throw()
{
	return p->flags;
}
FXSQLDBStatement *FXSQLDBCursor::statement() const throw()
{
	return PtrPtr(p->parent);
}
QWaitCondition *FXSQLDBCursor::resultsLatch() const throw()
{
	return p->latch;
}
FXuint FXSQLDBCursor::columns() const throw()
{
	return p->columns;
}
FXint FXSQLDBCursor::rows() const throw()
{
	return p->rows;
}
bool FXSQLDBCursor::rowsReady(FXint &start, FXint &end) const throw()
{
	if(start==end) return false;
	start=p->readyBegin; end=p->readyEnd;
	return true;
}

bool FXSQLDBCursor::atEnd() const throw()
{
	return p->atEnd;
}
FXint FXSQLDBCursor::at() const throw()
{
	return p->crow;
}
FXint FXSQLDBCursor::at(FXint newrow)
{
	FXSQLDB::Capabilities caps=statement()->driver()->capabilities();
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
FXint FXSQLDBCursor::backwards()
{
	if(!statement()->driver()->capabilities().HasBackwardsCursor)
	{
		FXERRGNOTSUPP(QTrans::tr("FXSQLDB::Statement::Cursor", "This driver does not support moving cursors backwards"));
	}
	if(p->crow<=-1) return p->crow;
	return --p->crow;
}
FXint FXSQLDBCursor::forwards()
{
	if(p->rows>=0 && p->crow>=p->rows) return p->crow;
	return ++p->crow;
}

//*******************************************************************************

struct FXSQLDBRegistryPrivate
{
	QDict<void> drivers;
};
static FXSQLDBRegistry *registry;

void FXSQLDBRegistry::int_register(const FXString &name, createSpec create)
{
	p->drivers.insert(name, (void *) create);
}
void FXSQLDBRegistry::int_deregister(const FXString &name, createSpec create)
{
	p->drivers.remove(name);
	if(registry==this && p->drivers.isEmpty())
	{
		FXDELETE(registry);
	}
}
FXSQLDBRegistry::FXSQLDBRegistry() : p(0)
{
	FXERRHM(p=new FXSQLDBRegistryPrivate);
}
FXSQLDBRegistry::~FXSQLDBRegistry()
{ FXEXCEPTIONDESTRUCT1 {
	FXDELETE(p);
} FXEXCEPTIONDESTRUCT2; }

FXSQLDBRegistry *FXSQLDBRegistry::processRegistry()
{
	if(!registry)
	{
		FXERRHM(registry=new FXSQLDBRegistry);
	}
	return registry;
}

QStringList FXSQLDBRegistry::drivers() const
{
	QStringList ret;
	for(QDictIterator<void> it(p->drivers); it.current(); ++it)
		ret.push_back(it.currentKey());
	return ret;
}

FXAutoPtr<FXSQLDB> FXSQLDBRegistry::instantiate(const FXString &name, const FXString &dbname, const FXString &user, const QHostAddress &host, FXushort port) const
{
	createSpec c=(createSpec) p->drivers.find(name);
	if(!c) return FXAutoPtr<FXSQLDB>(0);
	return c(dbname, user, host, port);
}


}

