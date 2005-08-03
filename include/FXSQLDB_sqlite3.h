/********************************************************************************
*                                                                               *
*                            SQLite3 Database Support                           *
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

#ifndef FXSQLDB_SQLITE3_H
#define FXSQLDB_SQLITE3_H

#include "FXSQLDB.h"

namespace FX {

/*! \file FXSQLDB_sqlite3.h
\brief Defines classes used to work with SQLite3 databases
*/

/*! \class FXSQLDB_sqlite3
\ingroup sqldb
\brief A SQL database driver for SQLite3

This driver accesses a SQLite3 database via an embedded, customised
edition of SQLite3. SQLite3 (http://www.sqlite.org/) is a self-contained,
embeddable, zero-configuration SQL database engine implementing most
of SQL92.

SQLite3 provides the basic functionality of a database, so cursors are
always dynamic and forward-only. Furthermore, you cannot know how many
rows are in result set except by iterating through all of them. Lastly,
you cannot copy cursors except by executing again and iterating to where
you want (which FX::FXSQLDBCursor::at() will do).

SQLite3 provides a much reduced set of SQL datatypes: namely, \c NULL,
\c VARCHAR, \c INTEGER, \c DOUBLE and \c BLOB. If you try to bind any
other type or specify any other type as the column type, silently one of
these types is chosen appropriately. Similarly, when retrieving data
only one of these types will be presented - though note that the
metaprogramming is aware of type convertibility, and will silently
invoke appropriate conversions if needed. FX::FXTime is treated like
all dates & times as text by SQLite3.

Some interesting properties of SQLite3 include that it does not enforce
type constraints ie; you can store any data in any column - though you
may suffer a slight performance penalty if you do. It also ignores type size
constraints eg; you can store any length of text irrespective of the
column type size.

SQLite3 opens and closes the database for each statement executed, so
if you have five operations then for each the entire database must be
loaded in. This is obviously slow, so consider putting your statements
inside a transaction which will then only open it once. However, as
SQLite3 cannot write concurrently to the same database, this will lock
that database exclusively for the duration of the transaction.

Note that UTF-16 support has been disabled in the embedded copy of SQLite3.
You shouldn't need it as UTF-8 support (the same as TnFOX) remains.

SQLite3 can read concurrently with other processes, but must modify
the entire database exclusively. This can make it unsuitable for certain
kinds of application. If FXSQLDB_sqlite3 is told that the database file
is locked, it waits on that file until the lock clears. This implies
that operations may block for a while. This driver knows to wait on the
underlying file rather spin, wasting processor time.
*/
struct FXSQLDB_sqlite3Private;
class FXAPI FXSQLDB_sqlite3 : public FXSQLDB
{
	FXSQLDB_sqlite3Private *p;
	FXSQLDB_sqlite3(const FXSQLDB_sqlite3 &);
	FXSQLDB_sqlite3 &operator=(const FXSQLDB_sqlite3 &);
	inline FXDLLLOCAL bool fxerrhsqlite3(int retcode, const char *file, int lineno);
public:
	static const FXString MyName;
	//! Instantiates a driver accessing \em dbpath
	FXSQLDB_sqlite3(const FXString &dbpath, const FXString &user=FXString::nullStr(), const QHostAddress &host=QHOSTADDRESS_LOCALHOST, FXushort port=0);
	~FXSQLDB_sqlite3();

	virtual void open(const FXString &password=FXString::nullStr());
	virtual void close();
	virtual FXSQLDBStatementRef prepare(const FXString &text);

	void FXDLLLOCAL int_throwSQLite3Error(int errcode, const char *file, int lineno);
	void FXDLLLOCAL int_waitOnSQLite3File();
};


}

#endif
