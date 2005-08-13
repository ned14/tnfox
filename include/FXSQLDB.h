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

#ifndef FXSQLDB_H
#define FXSQLDB_H

#include "FXTime.h"
#include "FXRefedObject.h"
#include "QBuffer.h"
#include "QHostAddress.h"
#include "QTrans.h"
#include <qcstring.h>

namespace FX {

/*! \file FXSQLDB.h
\brief Defines classes used to work with SQL databases
*/

/*! \defgroup sqldb SQL Database Support

While looking around at other API's implementing SQL Database Supprt,
especially Qt's (as we already have a fair few Qt API's), I was struck
at how few bothered to leverage C++ metaprogramming to help ease query
parameter binding. After all, most of one's time when working with
databases is reading result rows and thus any help here would surely
be wonderful! Therefore, via metaprogramming, TnFOX's SQL Database
Support automatically maps SQL92 data types to TnFOX ones and vice
versa, with further support for serialising and deserialising
arbitrary C++ types into and out from SQL BLOB types via FX::FXStream.
Indeed, unless you are working with weird databases, for the most
part you can forget about binding.

<h3>The Drivers:</h3>
The core of the TnFOX SQL Database Support is FX::FXSQLDB which
is the abstract base class of SQL Database drivers in TnFOX. To use it,
simply instantiate an implementation via FX::FXSQLDBRegistry which is
the future-proof method (which in the future may load in a driver DLL).

The drivers currently provided are:
\li <b>FX::FXSQLDB_sqlite3</b><br>
This driver accesses a SQLite3 database via an embedded, customised
edition of SQLite3. SQLite3 (http://www.sqlite.org/) is a self-contained,
embeddable, zero-configuration SQL database engine implementing most
of SQL92.
\li <b>FX::FXSQLDB_ipc</b><br>
This driver, in tandem with FX::FXSQLDBServer, permits any other
FX::FXSQLDB to be accessed over a FX::FXIPCChannel.

If you want a driver for your particular database, it's very easy to
implement your own - just see the sources. Please do consider donating
your new driver back to the TnFOX project so your hard work need not
be repeated by others. Probably at some point an ODBC driver will be
implemented.

<h3>Quick Usage Example:</h3>
\code
FXAutoPtr<FXSQLDB> mydb=FXSQLDBRegistry::make("SQLite3", dbname);
mydb->open();
mydb->immediate("CREATE TABLE test(id INTEGER PRIMARY KEY, 'value' INTEGER, 'text' VARCHAR(256), 'when' TIMESTAMP DEFAULT CURRENT_TIMESTAMP);");

FXSQLDBStatementRef s=db->prepare("SELECT :field FROM 'test' WHERE :field==:value;");
s->bind(":field", "test");
s->bind(":value", (FXint) 5);
for(FXSQLDBCursorRef c=s->execute(); !c->atEnd(); c->next())
{
  fxmessage("Entry %d: %d\n", c->at(), c->data(0)->get<FXint>(v));
}
\endcode
Obviously, the binding of arguments is pretty pointless in this example
as the prepared statement is used only once, so it would be quicker to
use \c db->execute() directly. However, there is an important caveat -
if you place values directly into a string, you must escape them in a
SQL92 compatible way which isn't required if you simply bind in the values.

TODO:
\li SQLITE_BUSY handler
\li FXSQLDB_ipc
*/
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251) // class 1 needs to have dll-interface to be used by clients of class 2
#pragma warning(disable: 4244) // Conversion from bigger to smaller, possible loss of data
#pragma warning(disable: 4275) // non DLL-interface used as base for DLL-interface class
#endif

class QStringList;

namespace FXSQLDBImpl
{	// We can't partially specialise member functions (which is stupid of the C++ spec)
	template<bool isUnsignedInt, typename type, typename signedIntEquiv> struct checkForOverflow
	{
		static bool Do(const type *v) throw()
		{
			return false;
		}
	};
	template<typename type, typename signedIntEquiv> struct checkForOverflow<true, type, signedIntEquiv>
	{	// Called when type is an unsigned int
		static bool Do(const type *v) throw()
		{
			if(v && *v>Generic::BiggestValue<signedIntEquiv>::value)
			{	// Unsigned is overflowing its container so bump it
				return true;
			}
			return false;
		}
	};
	template<> struct checkForOverflow<true, FXulong, FXlong>
	{	// Called when type is as big as it can be
		static bool Do(const FXulong *v) throw()
		{
			if(v && *v>Generic::BiggestValue<FXlong>::value)
			{	// Best we can do is print a message
#ifdef DEBUG
				fxmessage("WARNING: Unsigned 64 bit value overflows signed 64 bit database type!\n");
#endif
			}
			return false;
		}
	};
	template<bool unknownType, typename type> struct DoSerialise;
	template<int sql92type, bool isUnsignedInt, typename type> struct BindImpl;
}

class FXSQLDBStatement;
//! A reference to a Statement
typedef FXRefingObject<FXSQLDBStatement> FXSQLDBStatementRef;
class FXSQLDBCursor;
//! A reference to a Cursor
typedef FXRefingObject<FXSQLDBCursor> FXSQLDBCursorRef;
class FXSQLDBColumn;
//! A reference to a Column
typedef FXRefingObject<FXSQLDBColumn> FXSQLDBColumnRef;


/*! \class FXSQLDB
\ingroup sqldb
\brief The abstract base class of a SQL database driver

An implementation of this class implements the actual accessing of
a database. A good subset of what full ODBC offers is provided so it
should be good for most purposes.

While the full range of SQL92 data types can be determined, it makes
little sense to maintain some of the type differences in C++. Therefore,
SQL types such as NUMERIC, DECIMAL and FLOAT (ie; the parameterised types)
map straight to integer or floating-point C++ types depending on their
size. The reverse never happens though, so if you want that you must
operate the implementation interface manually.

In general, this is a very thin wrapper around the database itself for
speed. Some emulation is provided eg; if the driver doesn't support
settable cursors but does support forwards and backwards cursors, at()
will iterate those until the desired row is achieved. However, in general,
most of the translation logic is done entirely by metaprogramming. A lot
of the structure has been determined by requirements for FX::FXSQLDB_ipc
as the \c Node.Query capability is mostly implemented using that and
achieving maximum efficiency was important (imagine working through a
5,000 record dataset).

Note that if you store unsigned values, they are stored as their signed
equivalent unless they are too big to fit. If this happens, they move
up a container size which may cause your database driver to throw an
exception if type constraints are exceeded. If you don't want this,
cast your unsigned to signed before passing it to FX::FXSQLDBStatement::bind().

<h3>BLOB support:</h3>
Probably the biggest & best distinction of this SQL Database interface
from others is the BLOB support. Basically, if there is an \c operator<<
and \c operator>> overload for FX::FXStream for the type or a parent
class of that type, FXSQLDB will automatically see them and use them to both
store and load transparently the type instance as a BLOB.

And that's basically it. It just works.

<h3>Asynchronous connections:</h3>
FXSQLDB also supports the notion of asynchronous connections whereby if
say you bind a parameter to a prepared statement, it begins the process of
binding that parameter but returns from the call before the bind has
completed. This means two things: (i) the process of working with a remote
database goes much faster but (ii) errors do not get returned immediately,
but rather at some later stage, usually by calling some other code which
would never normally return such an error.

Normally, if you have written your code to be exception aware, this is
not a problem. If however you need to know that everything you have done
up till now has been processed and is fine, call synchronise().
*/
struct FXSQLDBPrivate;
class FXAPI FXSQLDB
{
	FXSQLDBPrivate *p;
	FXSQLDB(const FXSQLDB &);
	FXSQLDB &operator=(const FXSQLDB &);
public:
	//! Driver capabilities
	struct Capabilities
	{
		FXuint Transactions : 1;		//!< Whether this driver supports transactions
		FXuint QueryRows : 1;			//!< Whether this driver supports returning how many rows there are in a query (otherwise it returns -1)
		FXuint NoTypeConstraints : 1;	//!< Whether this driver does not enforce column type constraints
		FXuint HasBackwardsCursor : 1;	//!< Whether this driver supports cursors moving backwards
		FXuint HasSettableCursor : 1;	//!< Whether this driver supports cursors being set to an arbitrary row
		FXuint HasStaticCursor : 1;		//!< Whether this driver supports static cursors
		FXuint Asynchronous : 1;		//!< Whether this driver works asynchronously
		Capabilities() { *((FXuint *) this)=0; }
		//! Sets Transactions
		Capabilities &setTransactions(bool v=true) { Transactions=v; return *this; }
		//! Sets QueryRows
		Capabilities &setQueryRows(bool v=true) { QueryRows=v; return *this; }
		//! Sets NoTypeConstraints
		Capabilities &setNoTypeConstraints(bool v=true) { NoTypeConstraints=v; return *this; }
		//! Sets HasBackwardsCursor
		Capabilities &setHasBackwardsCursor(bool v=true) { HasBackwardsCursor=v; return *this; }
		//! Sets HasSettableCursor
		Capabilities &setHasSettableCursor(bool v=true) { HasSettableCursor=v; return *this; }
		//! Sets HasStaticCursor
		Capabilities &setHasStaticCursor(bool v=true) { HasStaticCursor=v; return *this; }
		//! Sets Asynchronous
		Capabilities &setAsynchronous(bool v=true) { Asynchronous=v; return *this; }
	};
protected:
	FXSQLDB(Capabilities caps, const FXString &driverName, const FXString &dbname=FXString::nullStr(), const FXString &user=FXString::nullStr(), const QHostAddress &host=QHOSTADDRESS_LOCALHOST, FXushort port=0);
public:
	virtual ~FXSQLDB();

	//! The standard SQL92 types of data which can be stored in a database
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
	//! Returns a SQLDataType as a string representation
	static const char *sql92TypeAsString(SQLDataType type);
	typedef Generic::TL::create<void,
		FXString, FXString, FXString, FXString,
		FXchar, FXshort, FXint, FXlong,
		FXlong, FXlong
	>::value CPPDataTypes1;
	typedef Generic::TL::create<
		FXfloat, FXdouble,
		FXdouble,
		FXTime, FXTime, FXTime,
		QByteArray
	>::value CPPDataTypes2;
	//! A mapping of the SQL92 data types to C++ datatypes
	typedef Generic::TL::append<CPPDataTypes1, CPPDataTypes2>::value CPPDataTypes;
	/*! Compile-time mapper of C++ data type to SQL data type. \c value
	becomes -1 if no mapping is found */
	template<typename type> struct CPPToSQL92Type
	{	// Replace unsigned integrals with their signed equivalents
		static const int unsignedIntIdx=Generic::TL::find<Generic::IntegralLists::unsignedInts, type>::value;
		typedef typename Generic::TL::at<Generic::IntegralLists::signedInts, unsignedIntIdx>::value signedIntEquiv;
		typedef typename Generic::select<-1==unsignedIntIdx, type, signedIntEquiv>::value typeToUse;

		static const int directidx=Generic::TL::find<CPPDataTypes, typeToUse>::value;
		static const SQLDataType value=(SQLDataType)((-1==directidx) ? Generic::TL::findParent<CPPDataTypes, typeToUse>::value : directidx);
	};
	/*! Returns which SQL data type this C++ type best matches. If you specify
	a pointer to a value of that type, it can better tell which type is best -
	equating signed equivalents to unsigned quantities, except when that value
	would overflow its direct signed equivalent */
	template<typename type> SQLDataType toSQL92Type(const type *v=0)
	{
		FXSTATIC_ASSERT(LastSQLDataTypeEntry==Generic::TL::length<CPPDataTypes>::value, Mismatched_SQLDataTypes_And_CPPDataTypes);
		typedef CPPToSQL92Type<type> sql92type;

		return (SQLDataType)(sql92type::value+FXSQLDBImpl::checkForOverflow<-1!=sql92type::unsignedIntIdx, type, typename sql92type::signedIntEquiv>::Do(v));
	}
	//! Invokes \em instance with the C++ type the specified SQL data type best matches via FX::Generic::TL::dynamicAt
	template<template<typename type> class instance> struct toCPPType
		: Generic::TL::dynamicAt<CPPDataTypes1, instance>, Generic::TL::dynamicAt<CPPDataTypes2, instance>
	{
		typedef Generic::TL::dynamicAt<CPPDataTypes1, instance> Base1;
		typedef Generic::TL::dynamicAt<CPPDataTypes2, instance> Base2;
		//! Invokes instance
		toCPPType(SQLDataType datatype)
			: Base1(datatype<Generic::TL::length<CPPDataTypes1>::value ? datatype : Base1::DisableMagicIdx),
			 Base2(datatype>=Generic::TL::length<CPPDataTypes1>::value ? datatype-Generic::TL::length<CPPDataTypes1>::value : Base1::DisableMagicIdx) { }
		//! Invokes instance, passing parameters
		template<typename P1, typename P2, typename P3, typename P4> toCPPType(SQLDataType datatype, P1 p1, P2 p2, P3 p3, P4 p4)
			: Base1(datatype<Generic::TL::length<CPPDataTypes1>::value ? datatype : Base1::DisableMagicIdx, p1, p2, p3, p4),
			 Base2(datatype>=Generic::TL::length<CPPDataTypes1>::value ? datatype-Generic::TL::length<CPPDataTypes1>::value : Base1::DisableMagicIdx, p1, p2, p3, p4) { }
	};

	//! Returns the name of this driver
	const FXString &driverName() const throw();
	//! Returns the capabilities of this driver
	Capabilities capabilities() const throw();
	//! Returns version information about the database this driver accesses
	virtual const FXString &versionInfo() const=0;
	//! The name of the database this driver is accessing
	const FXString &dbName() const throw();
	//! Sets the name of the database this driver is accessing
	void setDBName(const FXString &dbname);
	//! The username used to access this database
	const FXString &user() const throw();
	//! Sets the username used to access this database
	void setUser(const FXString &user);
	//! The IP address of the host serving the database
	const QHostAddress &host() const throw();
	//! Sets the IP address of the host serving the database
	void setHost(const QHostAddress &addr);
	//! The port of the host serving the database
	FXushort port() const throw();
	//! Sets the port of the host serving the database
	void setPort(FXushort port);

	//! Opens the database connection
	virtual void open(const FXString &password=FXString::nullStr())=0;
	//! Closes the database connection
	virtual void close()=0;
	//! Prepares a statement for later execution
	virtual FXSQLDBStatementRef prepare(const FXString &text)=0;

	//! Executes a statement with results
	virtual FXSQLDBCursorRef execute(const FXString &text, FXuint flags=2/*FXSQLDBCursor::IsDynamic*/|4/*FXSQLDBCursor::ForwardOnly*/, QWaitCondition *latch=0);
	//! Executes a statement immediately
	virtual void immediate(const FXString &text);

	//! Synchronises the connection if the driver is asynchronous
	virtual void synchronise();
};

/*! \class FXSQLDBCursor
\ingroup sqldb
\brief Abstract base class for a cursor which can iterate through the results of
executing a statement

You should iterate like this:
\code
for(FXSQLDBCursorRef cursor=statement->execute(); !cursor->atEnd(); cursor->next())
{
  FXSQLDBColumnRef field0=cursor->data(0);
  ...
}
\endcode

\sa FX::FXSQLDB, FX::FXSQLDBStatement
*/
struct FXSQLDBCursorPrivate;
class FXAPI FXSQLDBCursor : public FXRefedObject<int>
{
	FXSQLDBCursorPrivate *p;
	FXSQLDBCursor &operator=(const FXSQLDBCursor &o);
protected:
	FXSQLDBCursor(FXuint flags, FXSQLDBStatement *parent, QWaitCondition *latch, FXuint columns);
	FXSQLDBCursor(const FXSQLDBCursor &o);
	void int_setInternals(FXint *rows, FXuint *flags=0, FXuint *columns=0, QWaitCondition *latch=0);
	void int_setRowsReady(FXint start, FXint end);
	void int_setAtEnd(bool atend);
public:
	virtual ~FXSQLDBCursor();
	//! Copies a cursor
	virtual FXSQLDBCursorRef copy() const=0;

	//! Cursor flags
	enum Flags
	{	// If you change these remember to adjust FXSQLDB above
		IsStatic=1,			//!< The results returned by this cursor never change
		IsDynamic=2,		//!< The results returned by this cursor can change
		ForwardOnly=4		//!< This cursor can only move forwards
	};

	//! Returns flags
	FXuint flags() const throw();
	//! Returns the statement for which this cursor is returning results
	FXSQLDBStatement *statement() const throw();
	//! Returns the wait condition which will be signalled when results become available
	QWaitCondition *resultsLatch() const throw();
	//! Returns how many columns of information there are
	FXuint columns() const throw();
	//! Returns how many results there are in total, or -1 if unknown
	FXint rows() const throw();
	//! Returns which results are ready now, returning false if none are ready
	bool rowsReady(FXint &start, FXint &end) const throw();

	//! True if the cursor has finished enumerating all the rows
	bool atEnd() const throw();
	//! Which row the cursor is currently at
	FXint at() const throw();
	/*! Sets which row the cursor is currently at, returns the row which was actually set.
	If settable cursors are not supported by this driver, iterates forwards() and backwards()
	to achieve the same result */
	virtual FXint at(FXint newrow);
	//! Moves backwards, returning the new row
	virtual FXint backwards();
	//! \overload
	FXint prev() { return backwards(); }
	//! Moves forwards, returning the new row
	virtual FXint forwards();
	//! \overload
	FXint next() { return forwards(); }

	/*! Returns the column type and size for the specified zero-based column. This is the type
	of the column itself, not that of the result itself (they can vary in some drivers) */
	virtual void type(FXSQLDB::SQLDataType &datatype, FXint &size, FXuint no) const=0;
	//! Returns the header for the specified zero-based column in the results
	virtual FXSQLDBColumnRef header(FXuint no)=0;
	//! Returns the data for the specified zero-based column in the results
	virtual FXSQLDBColumnRef data(FXuint no)=0;
};

/*! \class FXSQLDBStatement
\ingroup sqldb
\brief The abstract base class of a prepared SQL statement

You can freely keep these around and simply rebind the parameters for
efficiency. You can use one of three forms of parameter:
\code
INSERT INTO foo VALUES(?, ?);
INSERT INTO foo VALUES(?2, ?1);
INSERT INTO foo VALUES(:niall, :douglas);
\endcode
The first form is simply where the first is 0, the second 1 etc. The second
form puts the lowest insert at 0, the next lowest at 1 etc. And the third
form is probably the most flexible but also the slowest - you literally
specify ":niall".

You should note that binding a BLOB directly via FX::QByteArray uses the
data directly by reference - therefore, the data must continue to exist
until after the statement has been executed. With every other type, bind()
takes a copy.

\sa FX::FXSQL
*/
struct FXSQLDBStatementPrivate;
class FXAPI FXSQLDBStatement : public FXRefedObject<int>
{
	FXSQLDBStatementPrivate *p;
	FXSQLDBStatement &operator=(const FXSQLDBStatement &o);
protected:
	FXSQLDBStatement(FXSQLDB *parent, const FXString &text);
	FXSQLDBStatement(const FXSQLDBStatement &o);
public:
	virtual ~FXSQLDBStatement();
	//! Copies a statement
	virtual FXSQLDBStatementRef copy() const=0;

	//! Returns the driver which will execute this statement
	FXSQLDB *driver() const throw();
	//! Returns the original text of the statement
	const FXString &text() const throw();

	//! Returns how many parameters this statement can take
	virtual FXint parameters() const=0;
	//! Returns the index of a parameter name, -1 if unknown
	virtual FXint parameterIdx(const FXString &name) const=0;
	//! Returns the parameter name at an index
	virtual FXString parameterName(FXint idx) const=0;
	/*! Implementation of binding a value to a parameter. All types
	have their data copied except for anything which becomes a BLOB
	which is used by reference and must exist until the statement is
	destroyed or the parameter rebound */
	virtual FXSQLDBStatement &bind(FXint idx, FXSQLDB::SQLDataType datatype, void *data);
private:
#if defined(_MSC_VER) && _MSC_VER<=1400 && !defined(__INTEL_COMPILER)
#if _MSC_VER<=1310 
	// MSVC7.1 and earlier just won't friend templates with specialisations :(
	friend struct FXSQLDBImpl::DoSerialise;
#else
	// MSVC8.0 is even worse :(. Just give up and call it public
public:
#endif
#else
	// This being the proper ISO C++ form
	template<bool unknownType, typename type> friend struct FXSQLDBImpl::DoSerialise;
#endif
	void int_bindUnknownBLOB(FXint idx, FXAutoPtr<QBuffer> buff);
public:
	//! Binds a value to a parameter by zero-based position
	template<typename type> FXSQLDBStatement &bind(FXint idx, const type &v)
	{
		typedef FXSQLDB::CPPToSQL92Type<type> sql92type;
		FXSQLDBImpl::BindImpl<sql92type::value, -1!=sql92type::unsignedIntIdx, type>(this, idx,
			FXSQLDBImpl::checkForOverflow<-1!=sql92type::unsignedIntIdx, type, typename sql92type::signedIntEquiv>::Do(&v), v);
		return *this;
	}
	//! Binds null to a parameter
	FXSQLDBStatement &bind(FXint idx)
	{
		bind(idx, FXSQLDB::Null, 0);
		return *this;
	}
	/*! Binds a value to a named parameter, returning the parameter index.
	Parameter insert locations can be specified using either the ?&lt;name&gt;
	or :&lt;name&gt; syntax */
	template<typename type> FXint bind(const FXString &name, const type &v)
	{
		FXint idx=parameterIdx(name);
		bind<type>(idx, v);
		return idx;
	}
	//! Binds null to a named parameter, returning the parameter index
	FXint bind(const FXString &name)
	{
		FXint idx=parameterIdx(name);
		bind(idx, FXSQLDB::Null, 0);
		return idx;
	}
	/*! Executes a statement returning results. \em flags are a combination
	of FX::FXSQLDBCursor::Flags. If set and asynchronous results
	set in the flags, \em latch will be signalled for you when results become available */
	virtual FXSQLDBCursorRef execute(FXuint flags=FXSQLDBCursor::IsDynamic|FXSQLDBCursor::ForwardOnly, QWaitCondition *latch=0)=0;
	//! Executes a statement returning no results
	virtual void immediate()=0;
};


namespace FXSQLDBImpl
{	// You can specialise these to use custom dumping routines like Tn does
	template<bool override, typename type> struct SerialiseUnknownBLOB
	{
		SerialiseUnknownBLOB(FXStream &s, const type &v)
		{	// Simply dump
			s << const_cast<type &>(v);
		}
	};
	template<bool override, typename type> struct DeserialiseUnknownBLOB
	{
		DeserialiseUnknownBLOB(type &v, FXStream &s)
		{	// Simply load
			s >> v;
		}
	};



	template<int sql92type, bool isUnsignedInt, typename type> struct BindImpl
	{	// It's known and not an unsigned int, so simply pass as a void *
		BindImpl(FXSQLDBStatement *s, FXint idx, bool upgrade, const type &v)
		{
			FXSQLDB *d=0;
			FXSQLDB::SQLDataType datatype=d->toSQL92Type<type>(&v);
			s->bind(idx, datatype, (void *) &v);
		}
	};
	template<int sql92type, typename type> struct BindImpl<sql92type, true, type>
	{	// It's known and is an unsigned int, so simply pass as a void *
		BindImpl(FXSQLDBStatement *s, FXint idx, bool upgrade, const type &v)
		{
			FXSQLDB *d=0;
			FXSQLDB::SQLDataType datatype=d->toSQL92Type<type>(&v);
			if(upgrade)
			{	// Need to copy to higher container to stay endian safe
				typedef typename Generic::TL::at<FXSQLDB::CPPDataTypes, sql92type+1>::value biggerContainer;
				biggerContainer bv=v;
				s->bind(idx, datatype, (void *) &bv);
			}
			else
				s->bind(idx, datatype, (void *) &v);
		}
	};
	template<bool canSerialise, typename type> struct DoSerialise
	{
		DoSerialise(FXSQLDBStatement *s, FXint idx, const type &v)
		{
			FXERRG(FXString("No operator<< found for type %1").arg(Generic::typeInfo<type>().name()), 0, FXERRH_ISDEBUG);
		}
	};
	template<typename type> struct DoSerialise<true, type>
	{
		DoSerialise(FXSQLDBStatement *s, FXint idx, const type &v)
		{
			FXAutoPtr<QBuffer> buff;
			FXERRHM(buff=new QBuffer);
			buff->open(IO_WriteOnly);
			FXStream ds(PtrPtr(buff));
			SerialiseUnknownBLOB<true, type>(ds, v);
			s->int_bindUnknownBLOB(idx, buff);
		}
	};
	template<bool isUnsignedInt, typename type> struct BindImpl<-1, isUnsignedInt, type>
	{	// It's some unknown type. Try serialising it
		BindImpl(FXSQLDBStatement *s, FXint idx, bool upgrade, const type &v)
		{
			DoSerialise<Generic::hasSerialise<type>::value, type>(s, idx, v);
		}
	};
	template<bool isUnsignedInt> struct BindImpl<-1, isUnsignedInt, const char *>
	{	// A string literal. Convert to FXString and pass
		BindImpl(FXSQLDBStatement *s, FXint idx, bool upgrade, const char *&v)
		{
			FXString l(v);
			FXSQLDB *d=0;
			FXSQLDB::SQLDataType datatype=d->toSQL92Type<FXString>();
			s->bind(idx, datatype, (void *) &l);
		}
	};



	template<bool isConvertible, typename rettype, typename srctype> struct GetImpl
	{
		GetImpl(rettype *dst, const srctype *src, FXSQLDB::SQLDataType sqldatatype, FXuval srcsize)
		{	// For the common case when no conversion exists between src and dst
			FXERRG(FXString("No conversion exists from %1 to %2").arg(Generic::typeInfo<srctype>().name()).arg(Generic::typeInfo<rettype>().name()), 0, FXERRH_ISDEBUG);
		}
	};
	template<typename rettype, typename srctype> struct GetImpl<true, rettype, srctype>
	{	// Good for anything with an implicit conversion available
		GetImpl(rettype *dst, const srctype *src, FXSQLDB::SQLDataType sqldatatype, FXuval srcsize)
		{	// Set dest equal to src, invoking appropriate conversions
			*dst=*src;
		}
	};
	template<typename rettype> struct GetImpl<true, rettype, FXString>
	{	// To avoid excessive memory copying, we don't construct FXString until the last moment
		GetImpl(rettype *dst, const FXString *src, FXSQLDB::SQLDataType sqldatatype, FXuval srcsize)
		{
			if(FXSQLDB::VarChar==sqldatatype || FXSQLDB::Char==sqldatatype)
			{
				*dst=FXString((const FXchar *) src, (FXint) srcsize);
			}
			else if(FXSQLDB::WVarChar==sqldatatype || FXSQLDB::WChar==sqldatatype)
			{
				//*dst=FXString((FXnchar *) src, (FXint) srcsize);
				assert(0);
			}
			else { assert(0); }
		}
	};
	template<> struct GetImpl<false, const char *, FXString>
	{	// This being the by-reference accessor
		GetImpl(const char **dst, const FXString *src, FXSQLDB::SQLDataType sqldatatype, FXuval srcsize)
		{
			if(FXSQLDB::VarChar==sqldatatype || FXSQLDB::Char==sqldatatype)
			{
				*dst=(const FXchar *) src;
			}
			else
				*dst=0;
		}
	};
	template<> struct GetImpl<true, QByteArray, QByteArray>
	{	// To avoid excessive memory copying, construct the QByteArray to directly point at this data
		GetImpl(QByteArray *dst, const QByteArray *src, FXSQLDB::SQLDataType sqldatatype, FXuval srcsize)
		{
			dst->setRawData((FXuchar *) src, (FXuint) srcsize, true);
		}
	};
	template<bool canDeserialise, typename rettype> struct DoDeserialise
	{
		DoDeserialise(rettype *dst, FXuchar *src, FXuint srcsize)
		{
			FXERRG(FXString("No operator>> found for type %1").arg(Generic::typeInfo<rettype>().name()), 0, FXERRH_ISDEBUG);
		}
	};
	template<typename rettype> struct DoDeserialise<true, rettype>
	{
		DoDeserialise(rettype *dst, FXuchar *src, FXuint srcsize)
		{
			QByteArray ba(src, srcsize);
			QBuffer buff(ba);
			buff.open(IO_ReadOnly);
			FXStream ds(&buff);
			DeserialiseUnknownBLOB<true, rettype>(*dst, ds);
		}
	};
	template<bool isConvertible, typename rettype> struct GetImpl<isConvertible, rettype, QByteArray>
	{	// This being the specialisation of BLOB to unknown type
		GetImpl(rettype *dst, const QByteArray *src, FXSQLDB::SQLDataType sqldatatype, FXuval srcsize)
		{
			DoDeserialise<Generic::hasDeserialise<rettype>::value, rettype>(dst, (FXuchar *) src, (FXuint) srcsize);
		}
	};
	template<typename rettype> struct Get
	{
		template<typename type> struct Source
		{	// rettype is the type we're storing to, type is the source type
			static void Do(rettype *dst, const void *src, FXSQLDB::SQLDataType sqldatatype, FXuval srcsize)
			{
				GetImpl<Generic::convertible<rettype, type>::value, rettype, type>(dst, (const type *) src, sqldatatype, srcsize);
			}
		};
		static void Invoke(rettype *dst, const void *src, FXSQLDB::SQLDataType sqldatatype, FXuval srcsize)
		{
			FXSQLDB::toCPPType<Source>(sqldatatype, dst, src, sqldatatype, srcsize);
		}
	};
}

/*! \class FXSQLDBColumn
\ingroup sqldb
\brief Represents information about a column in a row

Via specialisations of the metaprogramming underlying get(), <tt>const
char *</tt> and BLOB's (FX::QByteArray) return the data <b>by reference</b>,
thus avoiding copying.

FX::FXString's always copy both in and out. Note that except for BLOB's and
strings (including headers which really are just a string), the value is stored
in the scratch space inside FXSQLDBColumn and thus their value persists after the
cursor they came from is moved. 

\sa FX::FXSQLDB, FX::FXSQLCursor
*/
class FXAPI FXSQLDBColumn : public FXRefedObject<int>
{
protected:
	FXint myflags;
	FXSQLDBCursorRef myparent;
	FXuint mycolumn;
	FXint myrow;
	FXSQLDB::SQLDataType mytype;
	const void *mydata;
	FXuval mydatalen;
	union Scratch			// Some scratch space to avoid memory allocation for small types
	{
		FXchar tinyint;
		FXshort smallint;
		FXint integer;
		FXlong bigint;
		FXfloat real;
		FXdouble double_;
		char timestamp[sizeof(FXTime)];
	} scratch;
	FXSQLDBColumn(const FXSQLDBColumn &o) : myflags(o.myflags), myparent(o.myparent), mycolumn(o.mycolumn), myrow(o.myrow), mytype(o.mytype),
		mydata(o.mydata), mydatalen(o.mydatalen) { }
public:
	FXSQLDBColumn(FXuint flags, FXSQLDBCursor *parent, FXuint column, FXint row, FXSQLDB::SQLDataType type=FXSQLDB::Null)
		: myflags(flags), myparent(parent), mycolumn(column), myrow(row), mytype(type), mydata(0), mydatalen(0) { }
	virtual ~FXSQLDBColumn() { }
	//! Copies a column
	virtual FXSQLDBColumnRef copy() const=0;
	//! Flags
	enum Flags
	{
		IsHeader=1			//!< This is a header column
	};

	//! Returns flags
	FXuint flags() const throw() { return myflags; }
	//! Returns the cursor which owns this column
	FXSQLDBCursor *cursor() const throw() { return const_cast<FXSQLDBCursor *>(PtrPtr(myparent)); }
	//! Returns which column index this is
	FXuint column() const throw() { return mycolumn; }
	//! Returns which row index this is
	FXint row() const throw() { return myrow; }
	//! Returns the header for this column
	FXSQLDBColumnRef header() const { return cursor()->header(column()); }
	//! Returns the type of this column's data
	FXSQLDB::SQLDataType type() const throw() { return mytype; }
	//! Returns a pointer to the raw data
	const void *data() const throw() { return mydata; }
	//! Returns the size of the column's data
	FXuval size() const throw() { return mydatalen; }
	/*! Returns the effective type of this column's data. This
	is what the parameterised types (Decimal, Numeric, Float) are
	treated as */
	FXSQLDB::SQLDataType effectiveType() const throw()
	{
		if(FXSQLDB::Decimal==mytype || FXSQLDB::Numeric==mytype)
			return (FXSQLDB::SQLDataType)(FXSQLDB::TinyInt+fxbitscan(mydatalen));
		if(FXSQLDB::Float==mytype)
			return (FXSQLDB::SQLDataType)(FXSQLDB::Real-2+fxbitscan(mydatalen));
		return mytype;
	}

	/*! Stores the data in \em dst using \c operator= with the native
	type as returned from the database. */
	template<class T> void get(T &dst) const
	{	// Our problem is that we must make the rvalue for the operator= the
		// type that it is actually stored as
		FXSQLDB::SQLDataType coldatatype=effectiveType();
		FXSQLDBImpl::Get<T>::Invoke(&dst, mydata, coldatatype, mydatalen);
	}
	//! \overload
	template<class T> T get() const
	{
		T ret;
		get<T>(ret);
		return ret;
	}
};



/*! \class FXSQLDBRegistry
\ingroup sqldb
\brief Knows of all currently available FX::FXSQLDB's

In the future, this registry may be expanded to enumerate a directory for
database driver DLL's and thus load them on demand. However, for now, it
merely holds a registry of all known database drivers.
*/
struct FXSQLDBRegistryPrivate;
class FXAPI FXSQLDBRegistry
{
	friend struct FXSQLDBRegistryPrivate;
	FXSQLDBRegistryPrivate *p;
	FXSQLDBRegistry(const FXSQLDBRegistry &);
	FXSQLDBRegistry &operator=(const FXSQLDBRegistry &);
	typedef FXAutoPtr<FXSQLDB> (*createSpec)(const FXString &dbname, const FXString &user, const QHostAddress &host, FXushort port);
	void int_register(const FXString &name, createSpec create);
	void int_deregister(const FXString &name, createSpec create);
public:
	FXSQLDBRegistry();
	~FXSQLDBRegistry();

	//! Returns the process instance of this registry
	static FXSQLDBRegistry *processRegistry();
	//! Returns a list of all known drivers
	QStringList drivers() const;
	//! Instantiates an instance of a driver, returning zero if unknown
	FXAutoPtr<FXSQLDB> instantiate(const FXString &name, const FXString &dbname=FXString::nullStr(), const FXString &user=FXString::nullStr(), const QHostAddress &host=QHOSTADDRESS_LOCALHOST, FXushort port=0) const;
	//! Instantiates an instance of a driver from the process registry, returning zero if unknown
	static FXAutoPtr<FXSQLDB> make(const FXString &name, const FXString &dbname=FXString::nullStr(), const FXString &user=FXString::nullStr(), const QHostAddress &host=QHOSTADDRESS_LOCALHOST, FXushort port=0)
	{
		return processRegistry()->instantiate(name, dbname, user, host, port);
	}

	template<class type> struct Register
	{
		static FXAutoPtr<FXSQLDB> create(const FXString &dbname, const FXString &user, const QHostAddress &host, FXushort port) { return new type(dbname, user, host, port); }
		Register() { processRegistry()->int_register(type::MyName, create); }
		~Register() { processRegistry()->int_deregister(type::MyName, create); }
	};
	template<class type> friend struct Register;
};


#ifdef _MSC_VER
#pragma warning(pop)
#endif

}

#endif
