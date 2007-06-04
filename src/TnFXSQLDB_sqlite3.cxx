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
#ifndef FX_DISABLESQL

#ifdef HAVE_SQLITE3_H

#include "TnFXSQLDB_sqlite3.h"
#include "FXRollback.h"
#include <qmemarray.h>
#include "sqlite3.h"
#include "FXMemDbg.h"
#if defined(DEBUG) && !defined(FXMEMDBG_DISABLE)
static const char *_fxmemdbg_current_file_ = __FILE__;
#endif

namespace FX {

struct TnFXSQLDB_sqlite3Private
{
	sqlite3 *handle;
	TnFXSQLDB_sqlite3Private() : handle(0) { }
};
const FXString TnFXSQLDB_sqlite3::MyName("SQLite3");
static TnFXSQLDBRegistry::Register<TnFXSQLDB_sqlite3> sqlite3dbregister;

void TnFXSQLDB_sqlite3::int_throwSQLite3Error(int errcode, const char *file, int lineno)
{
	FXString errmsg(sqlite3_errmsg(p->handle));
	if(SQLITE_PERM==errcode || SQLITE_READONLY==errcode)
	{
		FXERRGNOPERM(errmsg, 0);
	}
	else if(SQLITE_NOMEM==errcode)
	{
		FXERRGM;
	}
	else if(SQLITE_IOERR==errcode)
	{
		FXIOException e(file, lineno, errmsg);
		FXERRH_THROW(e);
	}
	else if(SQLITE_NOTFOUND==errcode || SQLITE_CANTOPEN==errcode)
	{
		FXNotFoundException e(file, lineno, errmsg, 0);
		FXERRH_THROW(e);
	}
	else if(SQLITE_NOLFS==errcode)
	{
		FXNotSupportedException e(file, lineno, errmsg);
		FXERRH_THROW(e);
	}
	else
	{
		FXException e(file, lineno, errmsg, 0, 0);
		FXERRH_THROW(e);
	}
}
void TnFXSQLDB_sqlite3::int_waitOnSQLite3File()
{	// Wait on the lock on the SQLite3 database file to get released
	// TODO: Fix to wait directly on file
	QThread::msleep(1);
}
#define FXERRHSQLITE3IMPL(prefix) \
	if(SQLITE_OK==retcode) \
		return true; \
	else if(SQLITE_BUSY==retcode) \
	{ \
		prefix ->int_waitOnSQLite3File(); \
		return false; \
	} \
	else if(retcode>0 && retcode<SQLITE_ROW) \
		prefix ->int_throwSQLite3Error(retcode, file, lineno); \
	return true;
inline bool TnFXSQLDB_sqlite3::fxerrhsqlite3(int retcode, const char *file, int lineno)
{
	FXERRHSQLITE3IMPL(this)
}
#define FXERRHSQLITE3(st) fxerrhsqlite3(st, FXEXCEPTION_FILE(st), FXEXCEPTION_LINE(st))


TnFXSQLDB_sqlite3::TnFXSQLDB_sqlite3(const FXString &dbpath, const FXString &user, const QHostAddress &host, FXushort port) : TnFXSQLDB(Capabilities().setTransactions().setNoTypeConstraints(), MyName, dbpath, user, host, port), p(0)
{
	FXERRHM(p=new TnFXSQLDB_sqlite3Private);
}
TnFXSQLDB_sqlite3::~TnFXSQLDB_sqlite3()
{ FXEXCEPTIONDESTRUCT1 {
	close();
	FXDELETE(p);
} FXEXCEPTIONDESTRUCT2; }

const FXString &TnFXSQLDB_sqlite3::versionInfo() const
{
	static FXString s(sqlite3_libversion());
	return s;
}

namespace TnFXSQLDB_sqlite3Impl
{
	class Cursor : public TnFXSQLDBCursor
	{
		sqlite3_stmt *&stmth;
		bool first;
	public:
		static inline bool goNext(TnFXSQLDB *mydriver, sqlite3_stmt *stmth)
		{
			for(;;)
			{
				int ret=sqlite3_step(stmth);
				if(SQLITE_ROW==ret)
					return true;
				else if(SQLITE_DONE==ret)
					return false;
				else if(SQLITE_BUSY==ret)
				{	/* Give up timeslice */
					QThread::yield();
				}
				else
				{
					((TnFXSQLDB_sqlite3 *) mydriver)->int_throwSQLite3Error(ret, __FILE__, __LINE__);
				}
			}
		}
		Cursor(sqlite3_stmt *&_stmth, FXuint flags, TnFXSQLDBStatement *parent, QWaitCondition *latch, FXuint columns)
			: TnFXSQLDBCursor(flags, parent, latch, columns), stmth(_stmth), first(true)
		{
			forwards();
		}
		virtual TnFXSQLDBCursorRef copy() const
		{
			FXERRGNOTSUPP(QTrans::tr("TnFXSQLDB_sqlite3", "SQLite3 cursors cannot be copied"));
		}
		virtual FXint forwards()
		{
			if(!atEnd())
			{
				int_setAtEnd(!goNext(statement()->driver(), stmth));
				if(first)
				{
					int rows=atEnd() ? 0 : -1;
					int_setInternals(&rows);
					int_setRowsReady(0, rows);
					first=false;
				}
				return TnFXSQLDBCursor::forwards();
			}
			return TnFXSQLDBCursor::at();
		}
		FXuint typesize(const char *t) const
		{
			t=strchr(t, '(');
			if(!t) return 0;
			return atoi(++t);
		}
		virtual void type(TnFXSQLDB::SQLDataType &datatype, FXint &size, FXuint no) const
		{
			const char *colname=sqlite3_column_decltype(stmth, no);
			FXERRH(colname, QTrans::tr("TnFXSQLDB_sqlite3", "Failed to retrive column type"), 0, 0);
			if(strstr(colname, "VARWCHAR"))
			{
				datatype=TnFXSQLDB::WVarChar;
				size=sizeof(FXnchar)*typesize(colname);
			}
			else if(strstr(colname, "WCHAR"))
			{
				datatype=TnFXSQLDB::WChar;
				size=sizeof(FXnchar)*typesize(colname);
			}
			else if(strstr(colname, "VARCHAR"))
			{
				datatype=TnFXSQLDB::VarChar;
				size=typesize(colname);
			}
			else if(strstr(colname, "CHAR"))
			{
				datatype=TnFXSQLDB::Char;
				size=typesize(colname);
			}
			else if(strstr(colname, "TINYINT"))
			{
				datatype=TnFXSQLDB::TinyInt;
				size=sizeof(FXchar);
			}
			else if(strstr(colname, "SMALLINT"))
			{
				datatype=TnFXSQLDB::SmallInt;
				size=sizeof(FXshort);
			}
			else if(strstr(colname, "INTEGER"))
			{
				datatype=TnFXSQLDB::Integer;
				size=sizeof(FXint);
			}
			else if(strstr(colname, "BIGINT"))
			{
				datatype=TnFXSQLDB::BigInt;
				size=sizeof(FXlong);
			}
			else if(strstr(colname, "DECIMAL"))
			{
				datatype=TnFXSQLDB::Decimal;
				FXuint s=typesize(colname);
				if(s<3)
					size=sizeof(FXchar);
				else if(s<5)
					size=sizeof(FXshort);
				else if(s<10)
					size=sizeof(FXint);
				else
					size=sizeof(FXlong);
			}
			else if(strstr(colname, "NUMERIC"))
			{
				datatype=TnFXSQLDB::Numeric;
				FXuint s=typesize(colname);
				if(s<3)
					size=sizeof(FXchar);
				else if(s<5)
					size=sizeof(FXshort);
				else if(s<10)
					size=sizeof(FXint);
				else
					size=sizeof(FXlong);
			}
			else if(strstr(colname, "REAL"))
			{
				datatype=TnFXSQLDB::Real;
				size=sizeof(float);
			}
			else if(strstr(colname, "DOUBLE"))
			{
				datatype=TnFXSQLDB::Double;
				size=sizeof(double);
			}
			else if(strstr(colname, "FLOAT"))
			{
				datatype=TnFXSQLDB::Float;
				FXuint s=typesize(colname);
				if(s<25)
					size=sizeof(float);
				else
					size=sizeof(double);
			}
			else if(strstr(colname, "TIMESTAMP"))
			{
				datatype=TnFXSQLDB::Timestamp;
				size=sizeof(FXTime);
			}
			else if(strstr(colname, "DATE"))
			{
				datatype=TnFXSQLDB::Date;
				size=sizeof(FXTime);
			}
			else if(strstr(colname, "TIME"))
			{
				datatype=TnFXSQLDB::Time;
				size=sizeof(FXTime);
			}
			else if(strstr(colname, "BINARY"))
			{
				datatype=TnFXSQLDB::BLOB;
				size=typesize(colname);
			}
			else
			{
				datatype=TnFXSQLDB::Null;
				size=0;
			}
		}
		// Create our own column type just so we can be friends with it
		struct Column : public TnFXSQLDBColumn
		{
			friend class TnFXSQLDB_sqlite3Impl::Cursor;
			Column(FXuint flags, TnFXSQLDBCursor *parent, FXuint column, FXint row) : TnFXSQLDBColumn(flags, parent, column, row) { }
			Column(const Column &o) : TnFXSQLDBColumn(o) { }
			virtual TnFXSQLDBColumnRef copy() const
			{
				Column *c;
				FXERRHM(c=new Column(*this));
				return c;
			}
		};
		virtual TnFXSQLDBColumnRef header(FXuint no)
		{
			Column *c=0;
			TnFXSQLDBColumnRef ret;
			FXERRHM(ret=c=new Column(TnFXSQLDBColumn::IsHeader, this, no, -1));
			c->mytype=TnFXSQLDB::VarChar;
			c->mydata=sqlite3_column_name(stmth, no);
			FXERRH(c->mydata, QTrans::tr("TnFXSQLDB_sqlite3", "Failed to retrive column header"), 0, 0);
			c->mydatalen=strlen((const char *) c->mydata);
			return ret;
		}
		virtual TnFXSQLDBColumnRef data(FXuint no)
		{
			Column *c=0;
			TnFXSQLDBColumnRef ret;
			FXERRHM(ret=c=new Column(0, this, no, at()));
			switch(sqlite3_column_type(stmth, no))
			{
			case SQLITE_INTEGER:
				{
					c->mytype=TnFXSQLDB::BigInt;
					c->mydata=&c->scratch.bigint;
					c->mydatalen=sizeof(c->scratch.bigint);
					c->scratch.bigint=sqlite3_column_int64(stmth, no);
					break;
				}
			case SQLITE_FLOAT:
				{
					c->mytype=TnFXSQLDB::Double;
					c->mydata=&c->scratch.double_;
					c->mydatalen=sizeof(c->scratch.double_);
					c->scratch.double_=sqlite3_column_double(stmth, no);
					break;
				}
			case SQLITE_TEXT:
				{
					const char *t;
					c->mytype=TnFXSQLDB::VarChar;
					c->mydata=t=(char *) sqlite3_column_text(stmth, no);
					c->mydatalen=sqlite3_column_bytes(stmth, no);
					// YYYY-MM-DD HH:MM:SS.FFF
					bool isDate=('-'==t[4] && '-'==t[7]);
					bool isTimestamp=isDate && (':'==t[13] && ':'==t[16]);
					bool isTime=(':'==t[2] && ':'==t[5]);
					if(isDate || isTime)
					{	// Set up for FXTime
						FXTime *time;
						struct tm tmbuf={0};
						double fraction=0;
						c->mydata=time=(FXTime *) &c->scratch.timestamp;
						c->mydatalen=sizeof(FXTime);
						time->value=0; time->isLocalTime=false;
						
						if(isTimestamp)
						{
							c->mytype=TnFXSQLDB::Timestamp;
							sscanf(t, "%u-%u-%u %u:%u:%u.%lf", &tmbuf.tm_year, &tmbuf.tm_mon, &tmbuf.tm_mday,
								&tmbuf.tm_hour, &tmbuf.tm_min, &tmbuf.tm_sec, &fraction);
							tmbuf.tm_year-=1900;
							tmbuf.tm_mon-=1;
						}
						else if(isDate)
						{
							c->mytype=TnFXSQLDB::Date;
							sscanf(t, "%u-%u-%u", &tmbuf.tm_year, &tmbuf.tm_mon, &tmbuf.tm_mday);
							tmbuf.tm_year-=1900;
							tmbuf.tm_mon-=1;
						}
						else
						{
							c->mytype=TnFXSQLDB::Time;
							sscanf(t, "%u:%u:%u.%lf", &tmbuf.tm_hour, &tmbuf.tm_min, &tmbuf.tm_sec, &fraction);
						}
						time->set_tm(&tmbuf);
						while(fraction>=1.0) fraction/=10;
						time->value+=(FXulong)(FXTime::micsPerSecond*fraction);
					}
					break;
				}
			case SQLITE_BLOB:
				{
					c->mytype=TnFXSQLDB::BLOB;
					c->mydata=sqlite3_column_blob(stmth, no);
					c->mydatalen=sqlite3_column_bytes(stmth, no);
					break;
				}
			case SQLITE_NULL:
				{	// Already null
					break;
				}
			}
			return ret;
		}
	};
	class Statement : public TnFXSQLDBStatement
	{
		sqlite3 *&handle;
		struct stmth
		{
			bool needsReset;
			sqlite3_stmt *h;
			int columns, parameters;
			stmth(sqlite3_stmt *_h=0) : needsReset(false), h(_h), columns(0), parameters(0) { }
		};
		QMemArray<stmth> stmths;
		FXint mainstmth;
		bool needsReset;
		bool fxerrhsqlite3(int retcode, const char *file, int lineno)
		{
			FXERRHSQLITE3IMPL(((TnFXSQLDB_sqlite3 *) driver()))
		}
		void makeStmths()
		{
			const FXString &text=this->text();
			const char *t=text.text();
			mainstmth=-1;
			for(int togo; (togo=(int)((text.text()+text.length())-t));)
			{
				sqlite3_stmt *sth;
				if(FXERRHSQLITE3(sqlite3_prepare(handle, t, togo, &sth, &t)))
				{
					FXRBOp unsth=FXRBFunc(sqlite3_finalize, sth);
					stmths.push_back(stmth(sth));
					unsth.dismiss();
					stmth &stmt=stmths.at(stmths.count()-1);
					stmt.columns=sqlite3_column_count(stmt.h);
					stmt.parameters=sqlite3_bind_parameter_count(stmt.h);
					// The one which returns results is the main sth
					if(stmt.parameters) mainstmth=stmths.count()-1;
				}
			}
			if(-1==mainstmth)
				mainstmth=stmths.count()-1;
		}
		inline void checkForReset()
		{
			for(FXuint n=0; n<stmths.count(); n++)
			{
				if(stmths[n].needsReset)
				{
					FXERRH(!sqlite3_expired(stmths[n].h), QTrans::tr("TnFXSQLDB_sqlite3", "Statement has expired"), 0, 0);
					while(!FXERRHSQLITE3(sqlite3_reset(stmths[n].h)));
					stmths[n].needsReset=false;
				}
				else break;
			}
		}
	public:
		Statement(sqlite3 *&_handle, TnFXSQLDB *parent, const FXString &text) : TnFXSQLDBStatement(parent, text),
			handle(_handle), needsReset(false)
		{
			makeStmths();
		}
		Statement(const Statement &o) : TnFXSQLDBStatement(o), handle(o.handle), needsReset(false)
		{
			makeStmths();
		}
		~Statement()
		{
			for(FXuint n=0; n<stmths.count(); n++)
			{
				while(!FXERRHSQLITE3(sqlite3_finalize(stmths[n].h)));
				stmths[n].h=0;
			}
			stmths.resize(0);
		}
		virtual TnFXSQLDBStatementRef copy() const
		{
			Statement *s;
			FXERRHM(s=new Statement(*this));
			return s;
		}

		virtual FXint parameters() const
		{
			return stmths[mainstmth].parameters;
		}
		virtual FXint parameterIdx(const FXString &name) const
		{
			return sqlite3_bind_parameter_index(stmths[mainstmth].h, name.text())-1;
		}
		virtual FXString parameterName(FXint idx) const
		{
			FXERRH(idx>=0, QTrans::tr("TnFXSQLDB_sqlite3", "Index is invalid"), 0, 0);
			return FXString(sqlite3_bind_parameter_name(stmths[mainstmth].h, idx+1));
		}
		virtual TnFXSQLDBStatement &bind(FXint idx, TnFXSQLDB::SQLDataType datatype, void *data)
		{	// Do some sanity checks
			FXERRH(idx>=0, QTrans::tr("TnFXSQLDB_sqlite3", "Index is invalid"), 0, 0);
			// Most drivers would also assert that datatype is correct for column type,
			// but SQLite3 has no such problem
			checkForReset();
			switch(datatype)
			{
			case TnFXSQLDB::Null:
				{
					while(!FXERRHSQLITE3(sqlite3_bind_null(stmths[mainstmth].h, idx+1)));
					break;
				}
			case TnFXSQLDB::VarChar:
			case TnFXSQLDB::Char:
				{
					FXString *str=(FXString *) data;
					while(!FXERRHSQLITE3(sqlite3_bind_text(stmths[mainstmth].h, idx+1, str->text(), str->length(), SQLITE_TRANSIENT)));
					break;
				}
			case TnFXSQLDB::WVarChar:
			case TnFXSQLDB::WChar:
				{
					FXString *str=(FXString *) data;
					assert(0);
					//while(!FXERRHSQLITE3(sqlite3_bind_text16(stmths[mainstmth].h, idx+1, str->text(), str->length(), SQLITE_TRANSIENT)));
					break;
				}
			case TnFXSQLDB::TinyInt:
				{
					while(!FXERRHSQLITE3(sqlite3_bind_int(stmths[mainstmth].h, idx+1, *(FXchar *) data)));
					break;
				}
			case TnFXSQLDB::SmallInt:
				{
					while(!FXERRHSQLITE3(sqlite3_bind_int(stmths[mainstmth].h, idx+1, *(FXshort *) data)));
					break;
				}
			case TnFXSQLDB::Integer:
				{
					while(!FXERRHSQLITE3(sqlite3_bind_int(stmths[mainstmth].h, idx+1, *(FXint *) data)));
					break;
				}
			case TnFXSQLDB::BigInt:
			case TnFXSQLDB::Decimal:
			case TnFXSQLDB::Numeric:
				{
					while(!FXERRHSQLITE3(sqlite3_bind_int64(stmths[mainstmth].h, idx+1, *(FXlong *) data)));
					break;
				}
			case TnFXSQLDB::Real:
				{
					while(!FXERRHSQLITE3(sqlite3_bind_double(stmths[mainstmth].h, idx+1, *(float *) data)));
					break;
				}
			case TnFXSQLDB::Double:
			case TnFXSQLDB::Float:
				{
					while(!FXERRHSQLITE3(sqlite3_bind_double(stmths[mainstmth].h, idx+1, *(double *) data)));
					break;
				}
			case TnFXSQLDB::Timestamp:
				{
					FXString str(((FXTime *) data)->asString("%Y-%m-%d %H:%M:%S.%F %Z"));
					while(!FXERRHSQLITE3(sqlite3_bind_text(stmths[mainstmth].h, idx+1, str.text(), str.length(), SQLITE_TRANSIENT)));
					break;
				}
			case TnFXSQLDB::Date:
				{
					FXString str(((FXTime *) data)->asString("%Y-%m-%d"));
					while(!FXERRHSQLITE3(sqlite3_bind_text(stmths[mainstmth].h, idx+1, str.text(), str.length(), SQLITE_TRANSIENT)));
					break;
				}
			case TnFXSQLDB::Time:
				{
					FXString str(((FXTime *) data)->asString("%H:%M:%S.%F"));
					while(!FXERRHSQLITE3(sqlite3_bind_text(stmths[mainstmth].h, idx+1, str.text(), str.length(), SQLITE_TRANSIENT)));
					break;
				}
			case TnFXSQLDB::BLOB:
				{
					QByteArray *ba=(QByteArray *) data;
					while(!FXERRHSQLITE3(sqlite3_bind_blob(stmths[mainstmth].h, idx+1, ba->data(), ba->size(), SQLITE_STATIC)));
					break;
				}
			}
			return TnFXSQLDBStatement::bind(idx, datatype, data);
		}

		virtual TnFXSQLDBCursorRef execute(FXuint flags, QWaitCondition *latch)
		{
			FXERRH(!(flags & TnFXSQLDBCursor::IsStatic) && (flags & Cursor::ForwardOnly), QTrans::tr("TnFXSQLDB_sqlite3", "SQLite3 does not support static or non-forwards cursors"), 0, 0);
			checkForReset();
			TnFXSQLDB_sqlite3Impl::Cursor *c=0;
			for(FXuint n=0; n<stmths.count(); n++)
			{
				if((FXuint) mainstmth==n)
				{
					FXERRHM(c=new TnFXSQLDB_sqlite3Impl::Cursor(stmths[mainstmth].h, flags, this, latch, stmths[mainstmth].columns));
					stmths[n].needsReset=true;
					break;
				}
				else
				{
					TnFXSQLDB_sqlite3Impl::Cursor::goNext(driver(), stmths[n].h);
					stmths[n].needsReset=true;
				}
			}
			return c;
		}
		virtual void immediate()
		{
			checkForReset();
			// Execute everything at least once
			for(FXuint n=0; n<stmths.count(); n++)
			{
				TnFXSQLDB_sqlite3Impl::Cursor::goNext(driver(), stmths[n].h);
				stmths[n].needsReset=true;
			}
		}
	};
}

void TnFXSQLDB_sqlite3::open(const FXString &password)
{
	FXERRHSQLITE3(sqlite3_open(dbName().text(), &p->handle));
}
void TnFXSQLDB_sqlite3::close()
{
	if(p->handle)
	{
		while(!FXERRHSQLITE3(sqlite3_close(p->handle)));
		p->handle=0;
	}
}
TnFXSQLDBStatementRef TnFXSQLDB_sqlite3::prepare(const FXString &text)
{
	FXERRH(p->handle, QTrans::tr("TnFXSQLDB_sqlite3", "Database is not open"), 0, 0);
	TnFXSQLDB_sqlite3Impl::Statement *s;
	FXERRHM(s=new TnFXSQLDB_sqlite3Impl::Statement(p->handle, this, text));
	return s;
}

}

#endif
#endif
