/********************************************************************************
*                                                                               *
*                            Via-IPC Database Support                           *
*                                                                               *
*********************************************************************************
*       Copyright (C) 2005-2006 by Niall Douglas.   All Rights Reserved.        *
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
#if FX_SQLMODULE

#ifndef TNFXSQLDB_IPC_H
#define TNFXSQLDB_IPC_H

#include "TnFXSQLDB.h"
#include "FXIPC.h"
#include "QSSLDevice.h"
#include <qptrvector.h>

// Stop CopyCursor macro from WinUser.h mucking up the code
#undef CopyCursor

namespace FX {

/*! \file TnFXSQLDB_ipc.h
\brief Defines classes used to access databases over a FX::FXIPCChannel
*/

#if !defined(__GCCXML__)
namespace TnFXSQLDBIPCMsgsI
{	/* We have a slight problem in that the chunk code which is normally static
	is user-definable here. Unfortunately this means we must take a chunk parameter
	in every constructor :( */
	struct ColType
	{
		TnFXSQLDB::SQLDataType datatype;
		FXint size;
		ColType(TnFXSQLDB::SQLDataType _datatype=TnFXSQLDB::Null, FXint _size=0) : datatype(_datatype), size(_size) { }
		friend FXStream &operator<<(FXStream &s, const ColType &i)
		{
			s << (FXuchar) i.datatype << i.size;
			return s;
		}
		friend FXStream &operator>>(FXStream &s, ColType &i)
		{
			FXuchar v;
			s >> v >> i.size;
			i.datatype=(TnFXSQLDB::SQLDataType) v;
			return s;
		}
	};
	struct DataContainer
	{	// Holds a parameter being bound or a column being fetched
		FXuchar type;		// A TnFXSQLDB::SQLDataType
		struct Data
		{
			FXuint length;
			union
			{
				FXchar *text;
				FXchar tinyint;
				FXshort smallint;
				FXint integer;
				FXlong bigint;
				FXfloat real;
				FXdouble double_;
				char timestamp[sizeof(FXTime)];
				void *blob;
			};
		} data;
		bool mydata;
		DataContainer(TnFXSQLDB::SQLDataType datatype=TnFXSQLDB::Null) : type((FXuchar) datatype), mydata(false) { }
		void copy(const DataContainer &o)
		{
			type=o.type;
			data=o.data;
			if(TnFXSQLDB::BLOB==type || (type>=TnFXSQLDB::VarChar && type<=TnFXSQLDB::WChar))
			{
				data.blob=malloc(data.length);
				mydata=true;
				memcpy(data.blob, o.data.blob, data.length);
			}
		}
#ifndef HAVE_CPP0XRVALUEREFS
#ifdef HAVE_CONSTTEMPORARIES
		DataContainer(const DataContainer &_o) : type(_o.type), mydata(_o.mydata)
		{
			DataContainer &o=const_cast<DataContainer &>(_o);
#else
		DataContainer(DataContainer &o) : type(o.type), mydata(o.mydata)
		{
#endif
#else
private:
		DataContainer(const DataContainer &);	// disable copy constructor
public:
		DataContainer(DataContainer &&o) : type(std::move(o.type)), mydata(std::move(o.mydata))
		{
#endif
			data=o.data;
			o.data.blob=0;
		}
#ifndef HAVE_CPP0XRVALUEREFS
		DataContainer &operator=(const DataContainer &_o)
#else
private:
		DataContainer &operator=(const DataContainer &_o);
public:
		DataContainer &&operator=(const DataContainer &&_o)
#endif
		{	// Moves
			DataContainer &o=const_cast<DataContainer &>(_o);
			type=o.type;
			data=o.data;
			o.data.blob=0;
			return *this;
		}
		~DataContainer()
		{
			if(mydata && data.blob)
			{
				free(data.blob);
				data.blob=0;
			}
		}
		friend FXStream &operator<<(FXStream &s, const DataContainer &i)
		{
			s << i.type << i.data.length;
			switch(i.type)
			{
			case TnFXSQLDB::Null:
				break;
			case TnFXSQLDB::VarChar:
			case TnFXSQLDB::Char:
			case TnFXSQLDB::WVarChar:
			case TnFXSQLDB::WChar:
				s.writeRawBytes(i.data.text, i.data.length+1);
				break;

			case TnFXSQLDB::TinyInt:
				s << i.data.tinyint;
				break;
			case TnFXSQLDB::SmallInt:
				s << i.data.smallint;
				break;
			case TnFXSQLDB::Integer:
				s << i.data.integer;
				break;
			case TnFXSQLDB::BigInt:
			case TnFXSQLDB::Decimal:
			case TnFXSQLDB::Numeric:
				s << i.data.bigint;
				break;

			case TnFXSQLDB::Real:
				s << i.data.real;
				break;
			case TnFXSQLDB::Double:
			case TnFXSQLDB::Float:
				s << i.data.double_;
				break;

			case TnFXSQLDB::Timestamp:
			case TnFXSQLDB::Date:
			case TnFXSQLDB::Time:
				s << *(FXTime *)(void *) i.data.timestamp;
				break;

			case TnFXSQLDB::BLOB:
				{
					s.writeRawBytes((const char *) i.data.blob, i.data.length);
					break;
				}
			}
			return s;
		}
		friend FXStream &operator>>(FXStream &s, DataContainer &i)
		{
			s >> i.type >> i.data.length;
			switch(i.type)
			{
			case TnFXSQLDB::Null:
				break;
			case TnFXSQLDB::VarChar:
			case TnFXSQLDB::Char:
			case TnFXSQLDB::WVarChar:
			case TnFXSQLDB::WChar:
				{
					FXERRHM(i.data.text=(FXchar *) malloc(i.data.length+1));
					i.mydata=true;
					s.readRawBytes(i.data.text, i.data.length+1);
					break;
				}
			case TnFXSQLDB::TinyInt:
				s >> i.data.tinyint;
				break;
			case TnFXSQLDB::SmallInt:
				s >> i.data.smallint;
				break;
			case TnFXSQLDB::Integer:
				s >> i.data.integer;
				break;
			case TnFXSQLDB::BigInt:
			case TnFXSQLDB::Decimal:
			case TnFXSQLDB::Numeric:
				s >> i.data.bigint;
				break;

			case TnFXSQLDB::Real:
				s >> i.data.real;
				break;
			case TnFXSQLDB::Double:
			case TnFXSQLDB::Float:
				s >> i.data.double_;
				break;

			case TnFXSQLDB::Timestamp:
			case TnFXSQLDB::Date:
			case TnFXSQLDB::Time:
				s >> *(FXTime *)(void *) i.data.timestamp;
				break;

			case TnFXSQLDB::BLOB:
				{
					FXERRHM(i.data.blob=malloc(i.data.length));
					i.mydata=true;
					s.readRawBytes((char *) i.data.blob, i.data.length);
					break;
				}
			}
			return s;
		}
	};
	struct ColumnData
	{	// Holds a column's worth of data
		FXuint flags;
		DataContainer data;
		ColumnData(FXuint _flags=0, TnFXSQLDB::SQLDataType datatype=TnFXSQLDB::Null)
			: flags(_flags), data(std::move(datatype)) { }
#ifdef HAVE_CPP0XRVALUEREFS
private:
		ColumnData(const ColumnData &);	// disable copy constructor
		ColumnData &operator=(const ColumnData &);
public:
		ColumnData(ColumnData &&o) : flags(std::move(o.flags)), data(std::move(o.data)) { }
#endif
		void copy(const ColumnData &o)
		{
			flags=o.flags;
			data.copy(o.data);
		}
		friend FXStream &operator<<(FXStream &s, const ColumnData &i)
		{
			s << i.flags << i.data;
			return s;
		}
		friend FXStream &operator>>(FXStream &s, ColumnData &i)
		{
			s >> i.flags >> i.data;
			return s;
		}
	};
	// Requests a public encryption key
	struct RequestKey : public FXIPCMsg
	{
		typedef FXIPCMsgChunkCodeAlloc<0, true> id;
		typedef FXIPCMsgRegister<id, RequestKey> regtype;
		RequestKey()
			: FXIPCMsg(id::code) { }
		void   endianise(FXStream &s) const { }
		void deendianise(FXStream &s)       { }
	};
	struct RequestKeyAck : public FXIPCMsg
	{
		typedef FXIPCMsgChunkCodeAlloc<0, false> id;
		typedef FXIPCMsgRegister<id, RequestKeyAck> regtype;
		FXSSLPKey pkey;
		RequestKeyAck() : FXIPCMsg(0) { }
		RequestKeyAck(FXuint _id, const FXSSLPKey &_pkey)
			: FXIPCMsg(id::code, _id), pkey(_pkey) { }
		void   endianise(FXStream &s) const { s << pkey; }
		void deendianise(FXStream &s)       { s >> pkey; }
	};
	// Opens a database
	struct Open : public FXIPCMsg
	{
		typedef FXIPCMsgChunkCodeAlloc<RequestKey::id::nextcode, true> id;
		typedef FXIPCMsgRegister<id, Open> regtype;
		FXString driver;		// Name of the remote driver
		FXString dbname;
		FXString user;
		QBuffer password;
		QHostAddress host;
		FXushort port;
		Open() : FXIPCMsg(0) { }
		Open(const FXString &_driver, const FXString &_dbname, const FXString &_user, const QHostAddress &_host, FXushort _port)
			: FXIPCMsg(id::code), driver(_driver), dbname(_dbname), user(_user), host(_host), port(_port) { }
		void   endianise(FXStream &s) const { s << driver << dbname << user << !password.isNull(); if(!password.isNull()) { s << (FXuint)password.buffer().size(); s.writeRawBytes(password.buffer().data(), password.buffer().size()); } s << host << port; }
		void deendianise(FXStream &s)       { s >> driver >> dbname >> user; bool hasp; s >> hasp; if(hasp) { FXuint l; s >> l; password.buffer().resize(l); s.readRawBytes(password.buffer().data(), l); } s >> host >> port; }
	};
	struct OpenAck : public FXIPCMsg
	{
		typedef FXIPCMsgChunkCodeAlloc<RequestKey::id::nextcode, false> id;
		typedef FXIPCMsgRegister<id, OpenAck> regtype;
		FXuint connh;
		OpenAck() : FXIPCMsg(0) { }
		OpenAck(FXuint _id, FXuint _connh)
			: FXIPCMsg(id::code, _id), connh(_connh) { }
		void   endianise(FXStream &s) const { s << connh; }
		void deendianise(FXStream &s)       { s >> connh; }
	};
	// Closes a database
	struct Close : public FXIPCMsg
	{
		typedef FXIPCMsgChunkCodeAlloc<Open::id::nextcode, true> id;
		typedef FXIPCMsgRegister<id, Close> regtype;
		FXuint connh;
		Close(FXuint _connh=0) : FXIPCMsg(id::code), connh(_connh) { }
		void   endianise(FXStream &ds) const { ds << connh; }
		void deendianise(FXStream &ds)       { ds >> connh; }
	};
	struct CloseAck : public FXIPCMsg
	{
		typedef FXIPCMsgChunkCodeAlloc<Open::id::nextcode, false> id;
		typedef FXIPCMsgRegister<id, CloseAck> regtype;
		CloseAck(FXuint _id=0) : FXIPCMsg(id::code, _id) { }
		void   endianise(FXStream &ds) const { }
		void deendianise(FXStream &ds)       { }
	};
	// Prepares a statement
	struct PrepareStatement : public FXIPCMsg
	{
		typedef FXIPCMsgChunkCodeAlloc<Close::id::nextcode, true> id;
		typedef FXIPCMsgRegister<id, PrepareStatement> regtype;
		FXuint connh;			// Connection handle
		FXString statement;
		FXuint request;			// >0 for execute immediately
		FXuint cursflags;		// ==0 for no results
		PrepareStatement() : FXIPCMsg(0), request(0), cursflags(0) { }
		PrepareStatement(FXuint _connh, const FXString &_statement, FXuint _request=0, FXuint _cursflags=0)
			: FXIPCMsg(id::code), connh(_connh), statement(_statement), request(_request), cursflags(_cursflags) { }
		void   endianise(FXStream &s) const { s << connh << statement << request << cursflags; }
		void deendianise(FXStream &s)       { s >> connh >> statement >> request >> cursflags; }
	};
	struct PrepareStatementAck : public FXIPCMsg
	{
		typedef FXIPCMsgChunkCodeAlloc<Close::id::nextcode, false> id;
		typedef FXIPCMsgRegister<id, PrepareStatementAck> regtype;
		FXuint stmth;
		QMemArray<FXString> parNames;

		FXuint cursh;			// If it was immediate
		FXuint flags;
		FXuint columns;
		FXint rows;						// Can be -1 if unknown
		QPtrVector<ColumnData> *data;
		FXint rowsToGo;					// Rows to go after this batch of columns (can be -1)
		PrepareStatementAck() : FXIPCMsg(0), stmth(0), cursh(0), flags(0), columns(0), rows(0), data(0), rowsToGo(0) { }
		PrepareStatementAck(FXuint _id, FXuint _stmth, FXuint _cursh=0, FXuint _flags=0, FXuint _columns=0, FXint _rows=0)
			: FXIPCMsg(id::code, _id), stmth(_stmth), cursh(_cursh), flags(_flags), columns(_columns), rows(_rows), data(0), rowsToGo(-1) { }
		~PrepareStatementAck() { FXDELETE(data); }
		void   endianise(FXStream &s) const { s << stmth << parNames << cursh; if(cursh) s << flags << columns << rows << *data << rowsToGo; }
		void deendianise(FXStream &s)       { s >> stmth >> parNames >> cursh; if(cursh) { FXERRHM(data=new QPtrVector<ColumnData>(true)); s >> flags >> columns >> rows >> *data >> rowsToGo; } }
	};
	// Unprepares a statement
	struct UnprepareStatement : public FXIPCMsg
	{
		typedef FXIPCMsgChunkCodeAlloc<PrepareStatement::id::nextcode, false> id;
		typedef FXIPCMsgRegister<id, UnprepareStatement> regtype;
		FXuint stmth;
		UnprepareStatement() : FXIPCMsg(0), stmth(0) { }
		UnprepareStatement(FXuint _stmth) : FXIPCMsg(id::code), stmth(_stmth) { }
		void   endianise(FXStream &s) const { s << stmth; }
		void deendianise(FXStream &s)       { s >> stmth; }
	};
	// Binds a parameter
	struct BindParameter : public FXIPCMsg
	{
		typedef FXIPCMsgChunkCodeAlloc<UnprepareStatement::id::nextcode, true> id;
		typedef FXIPCMsgRegister<id, BindParameter> regtype;
		FXuint stmth;
		FXint paridx;
		DataContainer par;
		BindParameter() : FXIPCMsg(0), stmth(0), paridx(0) { }
		BindParameter(FXuint _stmth, FXint _paridx, TnFXSQLDB::SQLDataType datatype)
			: FXIPCMsg(id::code), stmth(_stmth), paridx(_paridx), par(datatype) { }
		void   endianise(FXStream &s) const { s << stmth << paridx << par; }
		void deendianise(FXStream &s)       { s >> stmth >> paridx >> par; }
	};
	struct BindParameterAck : public FXIPCMsg
	{
		typedef FXIPCMsgChunkCodeAlloc<UnprepareStatement::id::nextcode, false> id;
		typedef FXIPCMsgRegister<id, BindParameterAck> regtype;
		BindParameterAck(FXuint _id=0) : FXIPCMsg(id::code, _id) { }
		void   endianise(FXStream &s) const { }
		void deendianise(FXStream &s)       { }
	};
	// Executes a prepared statement
	struct Execute : public FXIPCMsg
	{
		typedef FXIPCMsgChunkCodeAlloc<BindParameter::id::nextcode, true> id;
		typedef FXIPCMsgRegister<id, Execute> regtype;
		FXuint stmth;
		FXuint request;		// How many of the results to return immediately
		FXuint cursflags;
		Execute() : FXIPCMsg(0), stmth(0), request(0), cursflags(0) { }
		Execute(FXuint _stmth, FXuint _request, FXuint _cursflags)
			: FXIPCMsg(id::code), stmth(_stmth), request(_request), cursflags(_cursflags) { }
		void   endianise(FXStream &s) const { s << stmth << request << cursflags; }
		void deendianise(FXStream &s)       { s >> stmth >> request >> cursflags; }
	};
	struct ExecuteAck : public FXIPCMsg
	{
		typedef FXIPCMsgChunkCodeAlloc<BindParameter::id::nextcode, false> id;
		typedef FXIPCMsgRegister<id, ExecuteAck> regtype;
		FXuint cursh;					// Handle to the returned cursor
		FXuint flags;
		FXuint columns;
		FXint rows;						// Can be -1 if unknown
		QPtrVector<ColumnData> *data;
		FXint rowsToGo;					// Rows to go after this batch of columns (can be -1)
		ExecuteAck(FXuint _id=0, FXuint _cursh=0, FXuint _flags=0, FXuint _columns=0, FXint _rows=0, FXint _rowsToGo=0)
			: FXIPCMsg(id::code, _id), cursh(_cursh), flags(_flags), columns(_columns), rows(_rows), data(0), rowsToGo(_rowsToGo) { }
		~ExecuteAck() { FXDELETE(data); }
		void   endianise(FXStream &s) const { s << cursh; if(cursh) s << flags << columns << rows << *data << rowsToGo; }
		void deendianise(FXStream &s)       { s >> cursh; if(cursh) { FXERRHM(data=new QPtrVector<ColumnData>(true)); s >> flags >> columns >> rows >> *data >> rowsToGo; } }
	};
	// Closes a cursor
	struct CloseCursor : public FXIPCMsg
	{
		typedef FXIPCMsgChunkCodeAlloc<Execute::id::nextcode, false> id;
		typedef FXIPCMsgRegister<id, CloseCursor> regtype;
		FXuint cursh;
		CloseCursor() : FXIPCMsg(0), cursh(0) { }
		CloseCursor(FXuint _cursh)
			: FXIPCMsg(id::code), cursh(_cursh) { }
		void   endianise(FXStream &s) const { s << cursh; }
		void deendianise(FXStream &s)       { s >> cursh; }
	};
	// Request rows of results
	struct RequestRows : public FXIPCMsg
	{
		typedef FXIPCMsgChunkCodeAlloc<CloseCursor::id::nextcode, true> id;
		typedef FXIPCMsgRegister<id, RequestRows> regtype;
		FXuint cursh;
		FXuint where;
		FXuint request;
		RequestRows() : FXIPCMsg(0), cursh(0), request(0) { }
		RequestRows(FXuint _cursh, FXuint _where, FXuint _request)
			: FXIPCMsg(id::code), cursh(_cursh), where(_where), request(_request) { }
		void   endianise(FXStream &s) const { s << cursh << where << request; }
		void deendianise(FXStream &s)       { s >> cursh >> where >> request; }
	};
	struct RequestRowsAck : public FXIPCMsg
	{
		typedef FXIPCMsgChunkCodeAlloc<CloseCursor::id::nextcode, false> id;
		typedef FXIPCMsgRegister<id, RequestRowsAck> regtype;
		QPtrVector<ColumnData> *data;
		FXint rowsToGo;					// Rows to go after this batch of columns (can be -1)
		RequestRowsAck(FXuint _id=0, FXint _rowsToGo=0)
			: FXIPCMsg(id::code, _id), data(0), rowsToGo(_rowsToGo) { }
		~RequestRowsAck() { FXDELETE(data); }
		void   endianise(FXStream &s) const { s << *data << rowsToGo; }
		void deendianise(FXStream &s)       { FXERRHM(data=new QPtrVector<ColumnData>(true)); s >> *data >> rowsToGo; }
	};
	// Request the types of the results
	struct RequestColTypes : public FXIPCMsg
	{
		typedef FXIPCMsgChunkCodeAlloc<RequestRows::id::nextcode, true> id;
		typedef FXIPCMsgRegister<id, RequestColTypes> regtype;
		FXuint cursh;
		RequestColTypes() : FXIPCMsg(0), cursh(0) { }
		RequestColTypes(FXuint _cursh)
			: FXIPCMsg(id::code), cursh(_cursh) { }
		void   endianise(FXStream &s) const { s << cursh; }
		void deendianise(FXStream &s)       { s >> cursh; }
	};
	struct RequestColTypesAck : public FXIPCMsg
	{
		typedef FXIPCMsgChunkCodeAlloc<RequestRows::id::nextcode, false> id;
		typedef FXIPCMsgRegister<id, RequestColTypesAck> regtype;
		QMemArray<ColType> *data;
		RequestColTypesAck(FXuint _id=0)
			: FXIPCMsg(id::code, _id), data(0) { }
		~RequestColTypesAck() { FXDELETE(data); }
		void   endianise(FXStream &s) const { s << *data; }
		void deendianise(FXStream &s)       { FXERRHM(data=new QMemArray<ColType>); s >> *data; }
	};
	// Request the headers of the results
	struct RequestColHeaders : public FXIPCMsg
	{
		typedef FXIPCMsgChunkCodeAlloc<RequestColTypes::id::nextcode, true> id;
		typedef FXIPCMsgRegister<id, RequestColHeaders> regtype;
		FXuint cursh;
		RequestColHeaders() : FXIPCMsg(0), cursh(0) { }
		RequestColHeaders(FXuint _cursh)
			: FXIPCMsg(id::code), cursh(_cursh) { }
		void   endianise(FXStream &s) const { s << cursh; }
		void deendianise(FXStream &s)       { s >> cursh; }
	};
	struct RequestColHeadersAck : public FXIPCMsg
	{
		typedef FXIPCMsgChunkCodeAlloc<RequestColTypes::id::nextcode, false> id;
		typedef FXIPCMsgRegister<id, RequestColHeadersAck> regtype;
		QPtrVector<ColumnData> *data;
		RequestColHeadersAck(FXuint _id=0)
			: FXIPCMsg(id::code, _id), data(0) { }
		~RequestColHeadersAck() { FXDELETE(data); }
		void   endianise(FXStream &s) const { s << *data; }
		void deendianise(FXStream &s)       { FXERRHM(data=new QPtrVector<ColumnData>(true)); s >> *data; }
	};
	// Copies a cursor
	struct CopyCursor : public FXIPCMsg
	{
		typedef FXIPCMsgChunkCodeAlloc<RequestColHeaders::id::nextcode, true> id;
		typedef FXIPCMsgRegister<id, CopyCursor> regtype;
		FXuint cursh;
		CopyCursor() : FXIPCMsg(0), cursh(0) { }
		CopyCursor(FXuint _cursh)
			: FXIPCMsg(id::code), cursh(_cursh) { }
		void   endianise(FXStream &s) const { s << cursh; }
		void deendianise(FXStream &s)       { s >> cursh; }
	};
	struct CopyCursorAck : public FXIPCMsg
	{
		typedef FXIPCMsgChunkCodeAlloc<RequestColHeaders::id::nextcode, false> id;
		typedef FXIPCMsgRegister<id, CopyCursorAck> regtype;
		FXuint cursh;
		CopyCursorAck(FXuint _id=0, FXuint _cursh=0)
			: FXIPCMsg(id::code, _id), cursh(_cursh) { }
		void   endianise(FXStream &s) const { s << cursh; }
		void deendianise(FXStream &s)       { s >> cursh; }
	};
}

/*! \struct TnFXSQLDBIPCMsgs
\brief Defines the IPC messages used to implement FX::TnFXSQLDBServer and FX::TnFXSQLDBDriver_ipc

Normally these would live inside a namespace, but as the message chunk you wish to use may
vary, I've made it parameterised. You may wish to use typedefing to make for easier
accessing eg;
\code
typedef TnFXSQLDBIPCMsgs<4000> MySQLDBIPCMsgs;
\endcode
*/
template<unsigned int chunkno> struct TnFXSQLDBIPCMsgs
{
	typedef FXIPCMsgChunkCodeAlloc<chunkno, true> ChunkBegin;

	struct RequestKey : public TnFXSQLDBIPCMsgsI::RequestKey
	{
		typedef FXIPCMsgChunkCodeAlloc<ChunkBegin::code, true> id;
		typedef FXIPCMsgRegister<id, RequestKey> regtype;
	};
	struct RequestKeyAck : public TnFXSQLDBIPCMsgsI::RequestKeyAck
	{
		typedef FXIPCMsgChunkCodeAlloc<ChunkBegin::code, false> id;
		typedef FXIPCMsgRegister<id, RequestKeyAck> regtype;
	};
	struct Open : public TnFXSQLDBIPCMsgsI::Open
	{
		typedef FXIPCMsgChunkCodeAlloc<RequestKey::id::nextcode, true> id;
		typedef FXIPCMsgRegister<id, Open> regtype;
	};
	struct OpenAck : public TnFXSQLDBIPCMsgsI::OpenAck
	{
		typedef FXIPCMsgChunkCodeAlloc<RequestKey::id::nextcode, false> id;
		typedef FXIPCMsgRegister<id, OpenAck> regtype;
	};
	struct Close : public TnFXSQLDBIPCMsgsI::Close
	{
		typedef FXIPCMsgChunkCodeAlloc<Open::id::nextcode, true> id;
		typedef FXIPCMsgRegister<id, Close> regtype;
	};
	struct CloseAck : public TnFXSQLDBIPCMsgsI::CloseAck
	{
		typedef FXIPCMsgChunkCodeAlloc<Open::id::nextcode, false> id;
		typedef FXIPCMsgRegister<id, CloseAck> regtype;
	};
	struct PrepareStatement : public TnFXSQLDBIPCMsgsI::PrepareStatement
	{
		typedef FXIPCMsgChunkCodeAlloc<Close::id::nextcode, true> id;
		typedef FXIPCMsgRegister<id, PrepareStatement> regtype;
	};
	struct PrepareStatementAck : public TnFXSQLDBIPCMsgsI::PrepareStatementAck
	{
		typedef FXIPCMsgChunkCodeAlloc<Close::id::nextcode, false> id;
		typedef FXIPCMsgRegister<id, PrepareStatementAck> regtype;
	};
	struct UnprepareStatement : public TnFXSQLDBIPCMsgsI::UnprepareStatement
	{
		typedef FXIPCMsgChunkCodeAlloc<PrepareStatement::id::nextcode, false> id;
		typedef FXIPCMsgRegister<id, UnprepareStatement> regtype;
	};
	struct BindParameter : public TnFXSQLDBIPCMsgsI::BindParameter
	{
		typedef FXIPCMsgChunkCodeAlloc<UnprepareStatement::id::nextcode, true> id;
		typedef FXIPCMsgRegister<id, BindParameter> regtype;
	};
	struct BindParameterAck : public TnFXSQLDBIPCMsgsI::BindParameterAck
	{
		typedef FXIPCMsgChunkCodeAlloc<UnprepareStatement::id::nextcode, false> id;
		typedef FXIPCMsgRegister<id, BindParameterAck> regtype;
	};
	struct Execute : public TnFXSQLDBIPCMsgsI::Execute
	{
		typedef FXIPCMsgChunkCodeAlloc<BindParameter::id::nextcode, true> id;
		typedef FXIPCMsgRegister<id, Execute> regtype;
	};
	struct ExecuteAck : public TnFXSQLDBIPCMsgsI::ExecuteAck
	{
		typedef FXIPCMsgChunkCodeAlloc<BindParameter::id::nextcode, false> id;
		typedef FXIPCMsgRegister<id, ExecuteAck> regtype;
	};
	struct CloseCursor : public TnFXSQLDBIPCMsgsI::CloseCursor
	{
		typedef FXIPCMsgChunkCodeAlloc<Execute::id::nextcode, false> id;
		typedef FXIPCMsgRegister<id, CloseCursor> regtype;
	};
	struct RequestRows : public TnFXSQLDBIPCMsgsI::RequestRows
	{
		typedef FXIPCMsgChunkCodeAlloc<CloseCursor::id::nextcode, true> id;
		typedef FXIPCMsgRegister<id, RequestRows> regtype;
	};
	struct RequestRowsAck : public TnFXSQLDBIPCMsgsI::RequestRowsAck
	{
		typedef FXIPCMsgChunkCodeAlloc<CloseCursor::id::nextcode, false> id;
		typedef FXIPCMsgRegister<id, RequestRowsAck> regtype;
	};
	struct RequestColTypes : public TnFXSQLDBIPCMsgsI::RequestColTypes
	{
		typedef FXIPCMsgChunkCodeAlloc<RequestRows::id::nextcode, true> id;
		typedef FXIPCMsgRegister<id, RequestColTypes> regtype;
	};
	struct RequestColTypesAck : public TnFXSQLDBIPCMsgsI::RequestColTypesAck
	{
		typedef FXIPCMsgChunkCodeAlloc<RequestRows::id::nextcode, false> id;
		typedef FXIPCMsgRegister<id, RequestColTypesAck> regtype;
	};
	struct RequestColHeaders : public TnFXSQLDBIPCMsgsI::RequestColHeaders
	{
		typedef FXIPCMsgChunkCodeAlloc<RequestColTypes::id::nextcode, true> id;
		typedef FXIPCMsgRegister<id, RequestColHeaders> regtype;
	};
	struct RequestColHeadersAck : public TnFXSQLDBIPCMsgsI::RequestColHeadersAck
	{
		typedef FXIPCMsgChunkCodeAlloc<RequestColTypes::id::nextcode, false> id;
		typedef FXIPCMsgRegister<id, RequestColHeadersAck> regtype;
	};
	struct CopyCursor : public TnFXSQLDBIPCMsgsI::CopyCursor
	{
		typedef FXIPCMsgChunkCodeAlloc<RequestColHeaders::id::nextcode, true> id;
		typedef FXIPCMsgRegister<id, CopyCursor> regtype;
	};
	struct CopyCursorAck : public TnFXSQLDBIPCMsgsI::CopyCursorAck
	{
		typedef FXIPCMsgChunkCodeAlloc<RequestColHeaders::id::nextcode, false> id;
		typedef FXIPCMsgRegister<id, CopyCursorAck> regtype;
	};
	typedef FXIPCMsgChunk<typename Generic::TL::append<typename Generic::TL::create<
			typename RequestKey::regtype,
			typename RequestKeyAck::regtype,
			typename Open::regtype,
			typename OpenAck::regtype,
			typename Close::regtype,
			typename CloseAck::regtype,
			typename PrepareStatement::regtype,
			typename PrepareStatementAck::regtype,
			typename UnprepareStatement::regtype,
			typename BindParameter::regtype,
			typename BindParameterAck::regtype,
			typename Execute::regtype,
			typename ExecuteAck::regtype,
			typename CloseCursor::regtype>::value, typename Generic::TL::create<
			typename RequestRows::regtype,
			typename RequestRowsAck::regtype,
			typename RequestColTypes::regtype,
			typename RequestColTypesAck::regtype,
			typename RequestColHeaders::regtype,
			typename RequestColHeadersAck::regtype,
			typename CopyCursor::regtype,
			typename CopyCursorAck::regtype
		>::value>::value> Chunk;

	typedef typename Generic::TL::create<Chunk>::value ChunkTypeList;
};
#endif // !defined(__GCCXML__)

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4275) // non dll interface use as base
#endif

/*! \class TnFXSQLDB_ipc
\ingroup sqldb
\brief A SQL database driver for a FX::FXIPCChannel

This driver accesses a remote FX::TnFXSQLDBDriver instantiation over a FX::FXIPCChannel.
To make the remote FX::TnFXSQLDBDriver instantiation available on the server end, you
must create a FX::TnFXSQLDBServer and populate both your client and server ends of
FX::FXIPCChannel with a FX::TnFXSQLDBIPCMsgs<>::ChunkTypeList, setting the message
chunk on both to the same as the chunk typelist.

Note that if you are opening a password protected database, the password is sent
encrypted using RSA asymmetric encryption. This adds extra time to opening the
database and it is always a synchronous operation.

The big feature of this driver is that it can work asynchronously ie; the local
end can do things while concurrently the FX::TnFXSQLDBDriver performs the
operation. The big advantage of this is speed - high-latency connections have
less effect on performance, but the big disadvantage of this is that errors
get thrown by calls after the one which caused the exception. This means that
in some code, you can't just drop in this driver as a replacement for another -
but in most code you can, especially if it is properly exception safe.

<h3>Speed tips:</h3>
Even if asynchronous mode is not enabled, prefetching of results still happens.
You should tune prefetching appropriate to the size of the rows of the results
you are fetching. By default, it is set to twenty records with
asking for more set to fifteen records (ie; when there are fifteen records left
in the prefetch, ask for another twenty). Depending on the size of each of your
records you may wish to increase or reduce this amount, especially given that
FX::FXIPCChannel imposes a maximum message size which you may exceed otherwise.
Note that prefetched records are held per-cursor, so if you're likely to move
backwards it's more efficient to copy the cursor at the point you are at and use
it later. If you disable prefetching, FX::TnFXSQLDBCursor::next() works synchronously
(which you may need if you need the absolute newest records).

Note that an execute() or immediate() has a faster code path than a prepare() as
it can skip the possibility of binding parameters and move immediately to execution.

If you want to truly maximise your speed, enable asynchronous mode and observe the
following:
\li If you use prepared statements, leave as much time as possible between
preparing a statement and binding the first parameter to it. TnFXSQLDB_ipc must wait
to bind if the statement has not yet been prepared.
\li Similarly, leave as much time as possible between getting your cursor ref
and using it. Note that some values from the cursor ref may be initially invalid
(eg; no of columns) until the call has been actually processed. Calling any of
the virtual methods of FX::TnFXSQLDBCursor will block until this time.
\li The cursor implementation is only optimised for moving forwards ie; prefetching
is always done in a forwards direction.
\li header() and type() are not prefetched and must be always executed synchronously.
However once executed, they are known for the entire dataset.
\li If you are storing data, it is a very good idea to execute synchronise()
after all the INSERT's or else data could get lost

\sa FX::TnFXSQLDB, FX::TnFXSQLDBServer
*/
struct TnFXSQLDB_ipcPrivate;
class FXSQLMODULEAPI TnFXSQLDB_ipc : public TnFXSQLDB, public FXIPCChannelIndirector
{
	friend struct TnFXSQLDB_ipcPrivate;
	TnFXSQLDB_ipcPrivate *p;
	TnFXSQLDB_ipc(const TnFXSQLDB_ipc &);
	TnFXSQLDB_ipc &operator=(const TnFXSQLDB_ipc &);
	typedef Generic::Functor<Generic::TL::create<bool, FXIPCMsg *FXRESTRICT, FXIPCMsg *FXRESTRICT>::value> AckHandler;
	typedef void (*delMsgSpec)(FXIPCMsg *m);
	inline void FXDLLLOCAL addAsyncMsg(FXIPCMsg *FXRESTRICT ia, FXIPCMsg *FXRESTRICT i, void *ref, void (*refdel)(void *), AckHandler handler, delMsgSpec iadel, delMsgSpec idel);
	template<class reftype> static void delRef(void *ptr) { delete static_cast<reftype *>(ptr); }
	template<class msgacktype, class msgtype, class reprtype> inline void sendAsyncMsg(msgtype *i, reprtype *dest, AckHandler handler)
	{	// Avoid including FXRollback.h
		FXAutoPtr<msgacktype> ia;
		FXAutoPtr<FXRefingObject<reprtype> > destref;
		FXERRHM(ia=new msgacktype);
		if(dest) { FXERRHM(destref=new FXRefingObject<reprtype>(dest)); }
		sendMsg(PtrPtr(ia), i, 0);
		addAsyncMsg(PtrPtr(ia), i, PtrPtr(destref), &delRef<FXRefingObject<reprtype> >, std::move(handler), msgacktype::regtype::delMsg, msgtype::regtype::delMsg);
		PtrRelease(ia);
		PtrRelease(destref);
	}
	inline bool FXDLLLOCAL pollAcks(bool waitIfNoneReady=false);
	struct Column;
	struct Cursor;		friend struct Cursor;
	struct Statement;	friend struct Statement;
public:
	static const FXString MyName;
	//! Instantiates a driver accessing \em dbspec of the form &lt;driver&gt;:&lt;dbname&gt;
	TnFXSQLDB_ipc(const FXString &dbspec, const FXString &user=FXString::nullStr(), const QHostAddress &host=QHOSTADDRESS_LOCALHOST, FXushort port=0);
	~TnFXSQLDB_ipc();

	using FXIPCChannelIndirector::channel;
	using FXIPCChannelIndirector::msgChunk;
	using FXIPCChannelIndirector::msgRouting;
	using FXIPCChannelIndirector::setIPCChannel;

	//! Returns if the connection is being operated asynchronously
	bool isAsynchronous() const throw();
	//! Sets if the connection is being operated asynchronously
	TnFXSQLDB_ipc &setIsAsynchronous(bool v=true) throw();
	//! Returns how much prefetching shall be performed
	void prefetching(FXuint &no, FXuint &askForMore) const throw();
	//! Sets how much prefetching shall be performed (=0 for none)
	TnFXSQLDB_ipc &setPrefetching(FXuint no, FXuint askForMore) throw();

	virtual const FXString &versionInfo() const;

	virtual void open(const FXString &password=FXString::nullStr());
	virtual void close();
	virtual TnFXSQLDBStatementRef prepare(const FXString &text);

	virtual TnFXSQLDBCursorRef execute(const FXString &text, FXuint flags=TnFXSQLDBCursor::IsDynamic|TnFXSQLDBCursor::ForwardOnly, QWaitCondition *latch=0);
	virtual void immediate(const FXString &text);

	virtual void synchronise();
};

/*! \class TnFXSQLDBServer
\brief Serves a database over an IPC channel

The implementation of this class is trickier than it might be due to needing
to be compatible with Tn's capability infrastructure, but the bonus is that
it makes it very flexible.

By default, TnFXSQLDBServer refuses to serve everything FX::TnFXSQLDB_ipc requests
for security. You must add databases which can be served using addDatabase()
which accepts patterns. If the pattern matches, the database is served.
*/
struct TnFXSQLDBServerPrivate;
class FXSQLMODULEAPI TnFXSQLDBServer : public FXIPCChannelIndirector
{
	TnFXSQLDBServerPrivate *p;
	TnFXSQLDBServer(const TnFXSQLDBServer &);
	TnFXSQLDBServer &operator=(const TnFXSQLDBServer &);
public:
	/*! Instantiates a server of databases */
	TnFXSQLDBServer();
	~TnFXSQLDBServer();

	using FXIPCChannelIndirector::channel;
	using FXIPCChannelIndirector::msgChunk;
	using FXIPCChannelIndirector::msgRouting;
	using FXIPCChannelIndirector::setIPCChannel;

	/*! Adds a database which is allowed to be served. You can use wildcards
	in the driver name, database name and user according to fxfilematch() -
	see FX::QDir for more. A null host and port means match anything. */
	TnFXSQLDBServer &addDatabase(const FXString &driverName, const FXString &dbname, const FXString &user="*", const QHostAddress &host=QHostAddress(), FXushort port=0);
	//! Removes a previously added database
	bool removeDatabase(const FXString &driverName, const FXString &dbname, const FXString &user="*", const QHostAddress &host=QHostAddress(), FXushort port=0);

	/* The message handler which looks for messages addressed to this server
	and processes them. You must call this in the message handler of that
	which serves the database */
	FXIPCChannel::HandledCode handleMsg(FXIPCMsg *msg);
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

}

#endif
#endif
