/********************************************************************************
*                                                                               *
*                            Via-IPC Database Support                           *
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
#include "FXSQLDB_ipc.h"
#include "FXRollback.h"
#include "FXSecure.h"
#include <qintdict.h>
#include "FXErrCodes.h"
#include "FXMemDbg.h"
#if defined(DEBUG) && !defined(FXMEMDBG_DISABLE)
static const char *_fxmemdbg_current_file_ = __FILE__;
#endif

namespace FX {

struct FXSQLDB_ipcPrivate
{
	typedef FXSQLDB_ipc::AckHandler AckHandler;
	typedef FXSQLDB_ipc::delMsgSpec delMsgSpec;
	bool asynchronous;
	FXuint prefetchNo, askForMore;
	struct Ack
	{
		FXIPCMsg *ia, *i;
		void *ref;
		void (*refdel)(void *);
		AckHandler handler;
		delMsgSpec iadel, idel;
		Ack(FXIPCMsg *_ia, FXIPCMsg *_i, void *_ref, void (*_refdel)(void *), AckHandler _handler, delMsgSpec _iadel, delMsgSpec _idel)
			: ia(_ia), i(_i), ref(_ref), refdel(_refdel), handler(_handler), iadel(_iadel), idel(_idel) { }
#ifndef HAVE_MOVECONSTRUCTORS
		Ack(const Ack &_o) : ia(_o.ia), i(_o.i), ref(_o.ref), refdel(_o.refdel),
			handler(const_cast<AckHandler &>(_o.handler)), iadel(_o.iadel), idel(_o.idel)
		{
			Ack &o=const_cast<Ack &>(_o);
#else
#error Fixme!
private:
		Ack(const Ack &);	// disable copy constructor
public:
		Ack(Ack &&o) : ia(o.ia), i(o.i), ref(_o.ref), refdel(_o.refdel),
			handler(o.handler), iadel(o.iadel), idel(o.idel)
		{
#endif
			o.i=0;
			o.ia=0;
			o.ref=0;
		}
		~Ack()
		{
			if(i)
			{
				idel(i);
				i=0;
			}
			if(ia)
			{
				iadel(ia);
				ia=0;
			}
			if(ref)
			{
				refdel(ref);
				ref=0;
			}
		}
	};
	QValueList<Ack> acksPending, acks;
	FXuint connh;

	FXSQLDB_ipcPrivate() : asynchronous(false), prefetchNo(20), askForMore(15), connh(0) { }

	bool openAck(FXIPCMsg *_ia, FXIPCMsg *_i)
	{
		using namespace FXSQLDBIPCMsgsI;
		OpenAck *ia=(OpenAck *) _ia;
		connh=ia->connh;
		return true;
	}
};
const FXString FXSQLDB_ipc::MyName("IPC");
static FXSQLDBRegistry::Register<FXSQLDB_ipc> ipcdbregister;

FXSQLDB_ipc::FXSQLDB_ipc(const FXString &dbspec, const FXString &user, const QHostAddress &host, FXushort port)
: FXSQLDB(FXSQLDB::Capabilities().setTransactions().setQueryRows().setNoTypeConstraints().setHasBackwardsCursor().setHasSettableCursor().setHasStaticCursor().setAsynchronous(), FXSQLDB_ipc::MyName, dbspec, user, host, port), p(0)
{
	FXERRHM(p=new FXSQLDB_ipcPrivate);
}
FXSQLDB_ipc::~FXSQLDB_ipc()
{ FXEXCEPTIONDESTRUCT1 {
	close();
	FXDELETE(p);
} FXEXCEPTIONDESTRUCT2; }
inline void FXSQLDB_ipc::addAsyncMsg(FXIPCMsg *ia, FXIPCMsg *i, void *ref, void (*refdel)(void *), FXSQLDB_ipc::AckHandler handler, FXSQLDB_ipc::delMsgSpec iadel, FXSQLDB_ipc::delMsgSpec idel)
{
	assert(i->msgId());
	p->acksPending.push_back(FXSQLDB_ipcPrivate::Ack(ia, i, ref, refdel, handler, iadel, idel));
}
inline bool FXSQLDB_ipc::pollAcks(bool waitIfNoneReady)
{	// Note that getMsgAck() can throw exceptions received from the other side
	FXSQLDB_ipcPrivate::Ack &ack=p->acksPending.front();
	if(getMsgAck(ack.ia, ack.i, waitIfNoneReady ? FXINFINITE : 0))
	{
		p->acks.splice(p->acks.end(), p->acksPending, p->acksPending.begin());
		if(p->acks.back().handler(ack.ia, ack.i))
			p->acks.pop_back();
		return true;
	}
	return false;
}


bool FXSQLDB_ipc::isAsynchronous() const throw()
{
	return p->asynchronous;
}
FXSQLDB_ipc &FXSQLDB_ipc::setIsAsynchronous(bool v) throw()
{
	p->asynchronous=v;
	return *this;
}
void FXSQLDB_ipc::prefetching(FXuint &no, FXuint &askForMore) const throw()
{
	no=p->prefetchNo;
	askForMore=p->askForMore;
}
FXSQLDB_ipc &FXSQLDB_ipc::setPrefetching(FXuint no, FXuint askForMore) throw()
{
	p->prefetchNo=no;
	p->askForMore=askForMore;
	return *this;
}


const FXString &FXSQLDB_ipc::versionInfo() const
{
	static const FXString myversion("TnFOX SQL DB via IPC channel v1.00");
	return myversion;
}

void FXSQLDB_ipc::open(const FXString &password)
{
	if(!p->connh)
	{
		using namespace FXSQLDBIPCMsgsI;
		FXint sidx=dbName().find(':');
		assert(sidx>=0);
		Open *i;
		FXERRHM(i=new Open(dbName().left(sidx), dbName().mid(sidx+1), user(), host(), port()));
		FXRBOp uni=FXRBNew(i);
		if(!password.empty())
		{
			RequestKey i2;
			RequestKeyAck i2a;
			sendMsg(i2a, i2);
			QSSLDevice ssl(&i->password);
			ssl.setKey(FXSSLKey(128, FXSSLKey::AES).setAsymmetricKey(&i2a.pkey));
			FXStream s(&ssl);
			ssl.open(IO_WriteOnly);
			s << password;
		}
		sendAsyncMsg<OpenAck>(i, (FXSQLDBCursor *) 0, AckHandler(*p, &FXSQLDB_ipcPrivate::openAck));
		uni.dismiss();
		if(!p->asynchronous)
			synchronise();
	}
}
void FXSQLDB_ipc::close()
{
	if(p->connh)
	{	// Wait for everything to finish processing first
		while(!p->acksPending.empty())
			pollAcks(true);
		using namespace FXSQLDBIPCMsgsI;
		Close i(p->connh);
		CloseAck ia;
		sendMsg(ia, i);
		p->acks.clear();
		p->connh=0;
	}
}

struct FXSQLDB_ipc::Column : public FXSQLDBColumn
{
	friend struct FXSQLDB_ipc::Cursor;
	Column(FXuint flags, FXSQLDBCursor *parent, FXuint column, FXint row, FXSQLDB::SQLDataType type) : FXSQLDBColumn(flags, parent, column, row, type) { }
	Column(const Column &o) : FXSQLDBColumn(o) { }
	virtual FXSQLDBColumnRef copy() const
	{
		Column *c;
		FXERRHM(c=new Column(*this));
		return c;
	}
};
struct FXSQLDB_ipc::Cursor : public FXSQLDBCursor
{
	FXSQLDB_ipc *o;
	FXSQLDB_ipcPrivate *p;
	FXuint cursh;
	struct Buffer
	{
		QMemArray<FXSQLDBIPCMsgsI::ColumnData> *data;
		FXint rows, rowsToGo;
		Buffer() : data(0), rows(0), rowsToGo(0) { }
		Buffer(const Buffer &o) : data(0), rows(o.rows), rowsToGo(o.rowsToGo)
		{
			if(o.data)
			{
				FXERRHM(data=new QMemArray<FXSQLDBIPCMsgsI::ColumnData>);
				for(FXuint n=0; n<o.data->count(); n++)
                    data->at(n).copy(o.data->at(n));
			}
		}
		~Buffer() { reset(); }
		void reset()
		{
			FXDELETE(data);
#ifdef DEBUG
			rows=rowsToGo=0;
#endif
		}
	};
	FXint bufferrowbegin;
	Buffer *buffers[2];
	mutable QMemArray<FXSQLDBIPCMsgsI::ColType> *colTypes;
	QMemArray<FXSQLDBIPCMsgsI::ColumnData> *headers;

	inline void configBuffer(int buffer, QMemArray<FXSQLDBIPCMsgsI::ColumnData> *&data, FXint rowsToGo)
	{
		Buffer *b=buffers[buffer];
		b->data=data; data=0;
		b->rows=b->data->count()/columns();
		b->rowsToGo=rowsToGo;
		int_setRowsReady(bufferrowbegin, bufferrowbegin+buffers[0]->rows+buffers[1]->rows);
	}
	inline void config(FXuint _cursh, FXint rows, FXuint flags, FXuint columns, QMemArray<FXSQLDBIPCMsgsI::ColumnData> *&data, FXint rowsToGo)
	{
		cursh=_cursh;
		int_setInternals(&rows, &flags, &columns);
		configBuffer(0, data, rowsToGo);
	}
	bool executeAck(FXIPCMsg *_ia, FXIPCMsg *_i)
	{
		using namespace FXSQLDBIPCMsgsI;
		ExecuteAck *ia=(ExecuteAck *) _ia;
		if(ia->cursh)
			config(ia->cursh, ia->rows, ia->flags, ia->columns, ia->data, ia->rowsToGo);
		return true;
	}
	bool requestRowsAck(FXIPCMsg *_ia, FXIPCMsg *_i)
	{
		using namespace FXSQLDBIPCMsgsI;
		RequestRowsAck *ia=(RequestRowsAck *) _ia;
		configBuffer(buffers[0]->data ? 1 : 0, ia->data, ia->rowsToGo);
		return true;
	}
	bool copyCursorAck(FXIPCMsg *_ia, FXIPCMsg *_i)
	{
		using namespace FXSQLDBIPCMsgsI;
		CopyCursorAck *ia=(CopyCursorAck *) _ia;
		cursh=ia->cursh;
		return true;
	}
	Cursor(FXSQLDB_ipcPrivate *_p, FXSQLDB_ipc *_o, FXuint flags, FXSQLDBStatement *parent, QWaitCondition *latch, FXuint &stmth, bool alreadyInProgress=false)
		: FXSQLDBCursor(flags, parent, latch, (FXuint)-1), o(_o), p(_p), cursh(0), bufferrowbegin(0), colTypes(0), headers(0)
	{
		buffers[0]=buffers[1]=0;
		FXERRHM(buffers[0]=new Buffer);
		FXERRHM(buffers[1]=new Buffer);
		if(!alreadyInProgress)
		{
			while(!stmth && !p->acksPending.empty())
				o->pollAcks(true);
			assert(stmth);
			using namespace FXSQLDBIPCMsgsI;
			Execute *i;
			FXERRHM(i=new Execute(stmth, p->prefetchNo, flags));
			FXRBOp uni=FXRBNew(i);
			o->sendAsyncMsg<ExecuteAck>(i, this, AckHandler(*this, &Cursor::executeAck));
			uni.dismiss();
		}
		// Ensure I point at record zero, not -1
		FXSQLDBCursor::at(0);
	}
	Cursor(const Cursor &ot) : FXSQLDBCursor(ot), o(ot.o), p(ot.p), cursh(0), bufferrowbegin(ot.bufferrowbegin), colTypes(0), headers(0)
	{
		buffers[0]=buffers[1]=0;
		if(ot.buffers[0]) FXERRHM(buffers[0]=new Buffer(*ot.buffers[0]));
		if(ot.buffers[1]) FXERRHM(buffers[1]=new Buffer(*ot.buffers[1]));
		if(ot.colTypes) FXERRHM(colTypes=new QMemArray<FXSQLDBIPCMsgsI::ColType>(*ot.colTypes));
		if(ot.headers)
		{
			FXERRHM(headers=new QMemArray<FXSQLDBIPCMsgsI::ColumnData>);
			for(FXuint n=0; n<ot.headers->count(); n++)
				headers->at(n).copy(ot.headers->at(n));
		}
	}
	~Cursor()
	{
		while(!cursh && !p->acksPending.empty())
			o->pollAcks(true);
		assert(cursh);
		using namespace FXSQLDBIPCMsgsI;
		CloseCursor i(cursh);
		o->sendMsg(i);
		cursh=0;

		FXDELETE(headers);
		FXDELETE(colTypes);
		FXDELETE(buffers[1]);
		FXDELETE(buffers[0]);
	}
	virtual FXSQLDBCursorRef copy() const
	{
		while(!cursh && !p->acksPending.empty())
			o->pollAcks(true);
		assert(cursh);
		Cursor *c;
		FXERRHM(c=new Cursor(*this));
		FXRBOp unc=FXRBNew(c);
		using namespace FXSQLDBIPCMsgsI;
		CopyCursor *i;
		FXERRHM(i=new CopyCursor(cursh));
		FXRBOp uni=FXRBNew(i);
		o->sendAsyncMsg<CopyCursorAck>(i, c, AckHandler(c, &Cursor::copyCursorAck));
		uni.dismiss();
		unc.dismiss();
		if(!p->asynchronous)
			o->synchronise();
		return c;
	}

	inline void askForNextBuffer()
	{
		assert(!buffers[1]->data);
		if(buffers[0]->rowsToGo)
		{
			using namespace FXSQLDBIPCMsgsI;
			RequestRows *i;
			FXERRHM(i=new RequestRows(cursh, bufferrowbegin+buffers[0]->rows, p->prefetchNo));
			FXRBOp uni=FXRBNew(i);
			o->sendAsyncMsg<RequestRowsAck>(i, this, AckHandler(*this, &Cursor::requestRowsAck));
			uni.dismiss();
			// We don't synchronise here
		}
	}
	inline void retireBuffer()
	{
		bufferrowbegin+=buffers[0]->rows;
		buffers[0]->reset();
		Buffer *t=buffers[0];
		buffers[0]=buffers[1];
		buffers[1]=t;
	}
	using FXSQLDBCursor::at;
	virtual FXint at(FXint newrow)
	{
		while((!cursh || !buffers[0]->data) && !p->acksPending.empty())
			o->pollAcks(true);
		FXint endofbuffer=bufferrowbegin+buffers[0]->rows;
		if(newrow>=bufferrowbegin && newrow<endofbuffer)
		{
			int_setAtEnd(false);
			return FXSQLDBCursor::at(newrow);
		}
		if(buffers[1]->data && newrow<endofbuffer+buffers[1]->rows)
		{	// It's in the next buffer
			retireBuffer();
			return Cursor::at(newrow);
		}
		// Ok, it's either before our buffers or after them so go fetch
		buffers[0]->reset(); buffers[1]->reset();
		bufferrowbegin=newrow;
		using namespace FXSQLDBIPCMsgsI;
		RequestRows *i;
		FXERRHM(i=new RequestRows(cursh, bufferrowbegin, p->prefetchNo));
		FXRBOp uni=FXRBNew(i);
		o->sendAsyncMsg<RequestRowsAck>(i, this, AckHandler(*this, &Cursor::requestRowsAck));
		uni.dismiss();
		return newrow;
	}
	virtual FXint backwards()
	{
		while((!cursh || !buffers[0]->data) && !p->acksPending.empty())
			o->pollAcks(true);
		FXint mypos=at();
		int_setAtEnd(false);
		if(mypos>bufferrowbegin)
			return FXSQLDBCursor::backwards();
		return at(mypos-1);
	}
	virtual FXint forwards()
	{
		while((!cursh || !buffers[0]->data) && !p->acksPending.empty())
			o->pollAcks(true);
		if(!atEnd())
		{
			FXint ret=FXSQLDBCursor::forwards(), endofbuffer=bufferrowbegin+buffers[0]->rows;
			if(ret>=endofbuffer)
			{
				int_setAtEnd(!buffers[0]->rowsToGo);
				retireBuffer();
			}
			else if(ret==endofbuffer-(FXint) p->askForMore)
				askForNextBuffer();
			return ret;
		}
		return FXSQLDBCursor::at();
	}
	virtual void type(FXSQLDB::SQLDataType &datatype, FXint &size, FXuint no) const
	{
		while(!cursh && !p->acksPending.empty())
			o->pollAcks(true);
		assert(cursh);
		using namespace FXSQLDBIPCMsgsI;
		if(!colTypes)
		{
			RequestColTypes i(cursh);
			RequestColTypesAck ia;
			o->sendMsg(ia, i);
			colTypes=ia.data;
			ia.data=0;
		}
		FXERRH(no<colTypes->count(), QTrans::tr("FXSQLDB_ipc", "Column does not exist"), 0, 0);
		datatype=(*colTypes)[no].datatype;
		size=(*colTypes)[no].size;
	}
	virtual FXSQLDBColumnRef header(FXuint no)
	{
		while(!cursh && !p->acksPending.empty())
			o->pollAcks(true);
		assert(cursh);
		using namespace FXSQLDBIPCMsgsI;
		if(!headers)
		{
			RequestColHeaders i(cursh);
			RequestColHeadersAck ia;
			o->sendMsg(ia, i);
			headers=ia.data;
			ia.data=0;
		}
		FXERRH(no<headers->count(), QTrans::tr("FXSQLDB_ipc", "Header does not exist"), 0, 0);
		ColumnData &cd=(*headers)[no];
		Column *c;
		FXERRHM(c=new Column(cd.flags, this, no, -1, (FXSQLDB::SQLDataType) cd.data.type));
		c->mydatalen=cd.data.data.length;
		if(BLOB==cd.data.type)
			c->mydata=cd.data.data.blob;
		else if(cd.data.type>=VarChar && cd.data.type<=WChar)
			c->mydata=cd.data.data.text;
		else
			c->mydata=&cd.data.data.tinyint;
		return c;
	}
	virtual FXSQLDBColumnRef data(FXuint no)
	{
		while((!cursh || !buffers[0]->data) && !p->acksPending.empty())
			o->pollAcks(true);
		assert(cursh && buffers[0]->data);
		FXuint dataoffset=(at()*columns())+no-bufferrowbegin;
		assert(dataoffset<buffers[0]->data->count());
		using namespace FXSQLDBIPCMsgsI;
		ColumnData &cd=(*buffers[0]->data)[dataoffset];
		Column *c;
		FXERRHM(c=new Column(cd.flags, this, no, at(), (FXSQLDB::SQLDataType) cd.data.type));
		c->mydatalen=cd.data.data.length;
		if(BLOB==cd.data.type)
			c->mydata=cd.data.data.blob;
		else if(cd.data.type>=VarChar && cd.data.type<=WChar)
			c->mydata=cd.data.data.text;
		else
			c->mydata=&cd.data.data.tinyint;
		return c;
	}
};
struct FXSQLDB_ipc::Statement : public FXSQLDBStatement
{
	FXSQLDB_ipc *o;
	FXSQLDB_ipcPrivate *p;
	FXuint stmth;
	QMemArray<FXString> parNames;
	Cursor *inProgressCursor;
	bool prepareStatementAck(FXIPCMsg *_ia, FXIPCMsg *_i)
	{
		using namespace FXSQLDBIPCMsgsI;
		PrepareStatementAck *ia=(PrepareStatementAck *) _ia;
		stmth=ia->stmth;
		parNames=ia->parNames;
		if(ia->cursh)
		{
			assert(inProgressCursor);
			inProgressCursor->config(ia->cursh, ia->rows, ia->flags, ia->columns, ia->data, ia->rowsToGo);
			inProgressCursor=0;
		}
		return true;
	}
	bool bindParameterAck(FXIPCMsg *_ia, FXIPCMsg *_i)
	{
		using namespace FXSQLDBIPCMsgsI;
		BindParameterAck *ia=(BindParameterAck *) _ia;
		return true;
	}
	bool executeAck(FXIPCMsg *_ia, FXIPCMsg *_i)
	{
		using namespace FXSQLDBIPCMsgsI;
		ExecuteAck *ia=(ExecuteAck *) _ia;
		assert(!ia->cursh);
		return true;
	}
	Statement(FXSQLDB_ipcPrivate *_p, FXSQLDB_ipc *parent, const FXString &text, bool executeNow=false, FXuint cursflags=0)
		: FXSQLDBStatement(parent, text), o(parent), p(_p), stmth(0), inProgressCursor(0)
	{
		using namespace FXSQLDBIPCMsgsI;
		PrepareStatement *i;
		assert(p->connh);
		FXERRHM(i=new PrepareStatement(o->p->connh, text, executeNow ? p->prefetchNo : 0, cursflags));
		FXRBOp uni=FXRBNew(i);
		o->sendAsyncMsg<PrepareStatementAck>(i, this, AckHandler(*this, &Statement::prepareStatementAck));
		uni.dismiss();
	}
	~Statement()
	{
		while(!stmth && !p->acksPending.empty())
			o->pollAcks(true);
		assert(stmth);
		using namespace FXSQLDBIPCMsgsI;
		UnprepareStatement i(stmth);
		o->sendMsg(i);
		stmth=0;
	}
	virtual FXSQLDBStatementRef copy() const
	{
		Statement *s;
		FXERRHM(s=new Statement(p, o, text()));
		return s;
	}
	virtual FXint parameters() const
	{
		while(!stmth && !p->acksPending.empty())
			o->pollAcks(true);
		assert(stmth);
		return parNames.count();
	}
	virtual FXint parameterIdx(const FXString &name) const
	{
		while(!stmth && !p->acksPending.empty())
			o->pollAcks(true);
		assert(stmth);
		for(FXuint n=0; n<parNames.count(); n++)
		{
			if(name==parNames[n])
				return n;
		}
		return -1;
	}
	virtual FXString parameterName(FXint idx) const
	{
		while(!stmth && !p->acksPending.empty())
			o->pollAcks(true);
		assert(stmth);
		if(idx<0 || idx>=(FXint) parNames.count()) return FXString::nullStr();
		return parNames[idx];
	}
	virtual FXSQLDBStatement &bind(FXint idx, FXSQLDB::SQLDataType datatype, void *data)
	{
		while(!stmth && !p->acksPending.empty())
			o->pollAcks(true);
		assert(stmth);
		using namespace FXSQLDBIPCMsgsI;
		BindParameter *i;
		FXERRHM(i=new BindParameter(stmth, idx, datatype));
		FXRBOp uni=FXRBNew(i);
		switch(datatype)
		{
		case FXSQLDB::Null:
			break;
		case FXSQLDB::VarChar:
		case FXSQLDB::Char:
		case FXSQLDB::WVarChar:
		case FXSQLDB::WChar:
			i->par.data.length=((FXString *) data)->length();
			i->par.data.text=(FXchar *)((FXString *) data)->text();
			break;

		case FXSQLDB::TinyInt:
			i->par.data.length=sizeof(FXuchar);
			i->par.data.tinyint=*(FXuchar *) data;
			break;
		case FXSQLDB::SmallInt:
			i->par.data.length=sizeof(FXushort);
			i->par.data.smallint=*(FXushort *) data;
			break;
		case FXSQLDB::Integer:
			i->par.data.length=sizeof(FXuint);
			i->par.data.integer=*(FXuint *) data;
			break;
		case FXSQLDB::BigInt:
		case FXSQLDB::Decimal:
		case FXSQLDB::Numeric:
			i->par.data.length=sizeof(FXulong);
			i->par.data.bigint=*(FXulong *) data;
			break;
		case FXSQLDB::Real:
			i->par.data.length=sizeof(float);
			i->par.data.real=*(float *) data;
			break;
		case FXSQLDB::Double:
		case FXSQLDB::Float:
			i->par.data.length=sizeof(double);
			i->par.data.double_=*(double *) data;
			break;
		case FXSQLDB::Timestamp:
		case FXSQLDB::Date:
		case FXSQLDB::Time:
			i->par.data.length=sizeof(FXTime);
			*(FXTime *)&i->par.data.timestamp=*(FXTime *) data;
			break;

		case FXSQLDB::BLOB:
			{
				QByteArray *ba=(QByteArray *) data;
				i->par.data.length=ba->size();
				i->par.data.blob=ba->data();
				break;
			}
		default:
			assert(0);
			break;
		}
		o->sendAsyncMsg<BindParameterAck>(i, this, AckHandler(*this, &Statement::bindParameterAck));
		uni.dismiss();
		if(!p->asynchronous)
			o->synchronise();
		return *this;
	}
	FXSQLDBCursorRef int_execute(FXuint flags, QWaitCondition *latch)
	{
		FXSQLDBCursorRef cr;
		Cursor *c=0;
		FXERRHM(cr=c=new Cursor(p, o, flags, this, latch, stmth, true));
		assert(!inProgressCursor);
		inProgressCursor=c;
		if(!p->asynchronous)
			o->synchronise();
		return cr;
	}
	virtual FXSQLDBCursorRef execute(FXuint flags=FXSQLDBCursor::IsDynamic|FXSQLDBCursor::ForwardOnly, QWaitCondition *latch=0)
	{
		FXSQLDBCursorRef cr;
		FXERRHM(cr=new Cursor(p, o, flags, this, latch, stmth));
		if(!p->asynchronous)
			o->synchronise();
		return cr;
	}
	virtual void immediate()
	{	// We just pretend to be a Cursor here for simplicity
		assert(stmth);
		using namespace FXSQLDBIPCMsgsI;
		Execute *i;
		FXERRHM(i=new Execute(stmth, p->prefetchNo, 0/*don't return a cursh*/));
		FXRBOp uni=FXRBNew(i);
		o->sendAsyncMsg<ExecuteAck>(i, this, AckHandler(*this, &Statement::executeAck));
		uni.dismiss();
		if(!p->asynchronous)
			o->synchronise();
	}
};

FXSQLDBStatementRef FXSQLDB_ipc::prepare(const FXString &text)
{
	while(!p->connh && !p->acksPending.empty())
		pollAcks(true);
	FXERRH(p->connh, QTrans::tr("FXSQLDB_ipc", "Database is not open"), 0, 0);
	FXSQLDBStatementRef sr;
	FXERRHM(sr=new Statement(p, this, text));
	if(!p->asynchronous)
		synchronise();
	return sr;
}

FXSQLDBCursorRef FXSQLDB_ipc::execute(const FXString &text, FXuint flags, QWaitCondition *latch)
{
	while(!p->connh && !p->acksPending.empty())
		pollAcks(true);
	FXERRH(p->connh, QTrans::tr("FXSQLDB_ipc", "Database is not open"), 0, 0);
	FXSQLDBStatementRef sr;
	Statement *s=0;
	FXERRHM(sr=s=new Statement(p, this, text, true, flags));
	return s->int_execute(flags, latch);
}
void FXSQLDB_ipc::immediate(const FXString &text)
{
	while(!p->connh && !p->acksPending.empty())
		pollAcks(true);
	FXERRH(p->connh, QTrans::tr("FXSQLDB_ipc", "Database is not open"), 0, 0);
	FXSQLDBStatementRef sr;
	FXERRHM(sr=new Statement(p, this, text, true));
	if(!p->asynchronous)
		synchronise();
}

void FXSQLDB_ipc::synchronise()
{
	while(!p->acksPending.empty())
		pollAcks(true);
}

//*******************************************************************************

struct FXSQLDBServerPrivate
{
	FXSSLPKey passwordKey;
	struct Database
	{
		FXString driverName;
		FXString dbname;
		FXString user;
		QHostAddress host;
		FXushort port;
		Database(const FXString &_driverName, const FXString &_dbname, const FXString &_user, const QHostAddress &_host, FXushort _port)
			: driverName(_driverName), dbname(_dbname), user(_user), host(_host), port(_port) { }
		bool operator==(const Database &o) const { return driverName==o.driverName && dbname==o.dbname && user==o.user && host==o.host && port==o.port; }
	};
	QValueList<Database> permitted;
	struct Handle
	{
		int type;
		FXSQLDBServerPrivate *p;
		FXuint handle;
		Handle(int _type, FXSQLDBServerPrivate *_p, FXuint _handle) : type(_type), p(_p), handle(_handle) { }
		~Handle()
		{
			p->handles.remove(handle);
		}
	};
	struct StatementH;
	struct ConnectionH;
	struct CursorH : Handle
	{
		StatementH *parent;
		FXSQLDBCursorRef curs;
		CursorH(FXSQLDBServerPrivate *p, StatementH *_parent, FXuint cursh, FXSQLDBCursorRef _curs) : Handle(3, p, cursh), parent(_parent), curs(_curs) { }
	};
	struct StatementH : Handle
	{
		ConnectionH *parent;
		FXSQLDBStatementRef stmt;
		QIntDict<CursorH> curshs;
		QMemArray<void *> blobs;
		StatementH(FXSQLDBServerPrivate *p, ConnectionH *_parent, FXuint stmth, FXSQLDBStatementRef _stmt) : Handle(2, p, stmth), parent(_parent), stmt(_stmt), curshs(1, true) { }
		~StatementH()
		{
			for(FXuint n=0; n<blobs.count(); n++)
				free(blobs[n]);
		}
	};
	struct ConnectionH : Handle
	{
		FXAutoPtr<FXSQLDB> driver;
		QIntDict<StatementH> stmths;
		ConnectionH(FXSQLDBServerPrivate *p, FXuint connh, FXAutoPtr<FXSQLDB> _driver) : Handle(1, p, connh), driver(_driver), stmths(1, true) { }
	};
	QIntDict<Handle> handles;			// For quick lookup
	QIntDict<ConnectionH> connhs;
	FXSQLDBServerPrivate() : connhs(1, true), handles(1) { }
	FXuint makeRandomId() const
	{	// Use OpenSSL if we have it as more random = more secure
		FXuint v=0;
		do
		{
#ifdef HAVE_OPENSSL
			if(!Secure::PRandomness::readBlock((FXuchar *) &v, sizeof(FXuint)))
#endif
			{
				static FXuint seed=FXProcess::getMsCount();
				v=fxrandom(seed);
			}
		} while(!v || handles.find(v));
		return v;
	}
	FXuint execute(StatementH *sh, FXuint cursflags)
	{	// If cursflags==0, it's an immediate statement
		FXSQLDBCursorRef sr=sh->stmt->execute(!cursflags ? FXSQLDBCursor::IsDynamic|FXSQLDBCursor::ForwardOnly : cursflags);
		if(cursflags)
		{
			CursorH *ch;
			FXERRHM(ch=new CursorH(this, sh, makeRandomId(), sr));
			FXRBOp unch=FXRBNew(ch);
			sh->curshs.insert(ch->handle, ch);
			unch.dismiss();
			handles.insert(ch->handle, ch);
			QDICTDYNRESIZE(sh->curshs);
			QDICTDYNRESIZE(handles);
			return ch->handle;
		}
		return 0;
	}
	void getcursor(FXuint &flags, FXuint &columns, FXint &rows, QMemArray<FXSQLDBIPCMsgsI::ColumnData> *&data, FXint &rowsToGo, FXuint cursh, FXuint request)
	{
		CursorH *ch=(CursorH *) handles.find(cursh);
		FXERRH(ch, QTrans::tr("FXSQLDBServer", "Cursor handle not found"), FXSQLDBSERVER_NOTFOUND, 0);
		flags=ch->curs->flags();
		columns=ch->curs->columns();
		rows=ch->curs->rows();
		if(request) getdata(data, rowsToGo, ch, request);
	}
	void getdata(QMemArray<FXSQLDBIPCMsgsI::ColumnData> *&data, FXint &rowsToGo, CursorH *ch, FXuint request)
	{
		FXuint r, origrequest=request;
		if(request>10000) request=10000;
		if(!data)
			FXERRHM(data=new QMemArray<FXSQLDBIPCMsgsI::ColumnData>(ch->curs->columns()*request));
		else
			data->resize(ch->curs->columns()*request);
		FXSQLDBCursor *curs=PtrPtr(ch->curs);
		for(r=0; r<request && !curs->atEnd(); r++, curs->next())
		{
			for(FXuint c=0; c<curs->columns(); c++)
			{
				FXSQLDBColumnRef cr_=curs->data(c);
				FXSQLDBColumn *cr=PtrPtr(cr_);
				FXSQLDBIPCMsgsI::ColumnData &cd=(*data)[r*curs->columns()+c];
				cd.data.data.length=(FXuint) cr->size();
				switch((cd.data.type=(FXuchar) cr->type()))
				{
				case FXSQLDB::Null:
					break;
				case FXSQLDB::VarChar:
				case FXSQLDB::Char:
				case FXSQLDB::WVarChar:
				case FXSQLDB::WChar:
					FXERRHM(cd.data.data.text=(FXchar *) malloc(cd.data.data.length+1));
					cd.data.mydata=true;
					memcpy(cd.data.data.text, cr->data(), cd.data.data.length+1);
					break;

				case FXSQLDB::TinyInt:
				case FXSQLDB::SmallInt:
				case FXSQLDB::Integer:
				case FXSQLDB::BigInt:
				case FXSQLDB::Decimal:
				case FXSQLDB::Numeric:
				case FXSQLDB::Real:
				case FXSQLDB::Double:
				case FXSQLDB::Float:
				case FXSQLDB::Timestamp:
				case FXSQLDB::Date:
				case FXSQLDB::Time:
					memcpy(&cd.data.data.tinyint, cr->data(), cr->size());
					break;

				case FXSQLDB::BLOB:
					FXERRHM(cd.data.data.blob=malloc(cd.data.data.length));
					cd.data.mydata=true;
					memcpy(cd.data.data.blob, cr->data(), cd.data.data.length);
					break;
				}
			}
		}
		rowsToGo=curs->atEnd() ? 0 : (-1==curs->rows() ? -1 : curs->rows()-curs->at()-1);
	}
};

FXSQLDBServer::FXSQLDBServer() : p(0)
{
	FXERRHM(p=new FXSQLDBServerPrivate);
}
FXSQLDBServer::~FXSQLDBServer()
{ FXEXCEPTIONDESTRUCT1 {
	FXDELETE(p);
} FXEXCEPTIONDESTRUCT2; }

FXSQLDBServer &FXSQLDBServer::addDatabase(const FXString &driverName, const FXString &dbname, const FXString &user, const QHostAddress &host, FXushort port)
{
	p->permitted.push_back(FXSQLDBServerPrivate::Database(driverName, dbname, user, host, port));
	return *this;
}
bool FXSQLDBServer::removeDatabase(const FXString &driverName, const FXString &dbname, const FXString &user, const QHostAddress &host, FXushort port)
{
	size_t c=p->permitted.size();
	p->permitted.remove(FXSQLDBServerPrivate::Database(driverName, dbname, user, host, port));
	return c!=p->permitted.size();
}

FXIPCChannel::HandledCode FXSQLDBServer::handleMsg(FXIPCMsg *msg)
{
	using namespace FXSQLDBIPCMsgsI;
	switch(msg->msgType()-msgChunk())
	{
	case RequestKey::id::code:
		{
			RequestKey *i=(RequestKey *) msg;
			if(i->wantsAck())
			{
				if(FXSSLPKey::NoEncryption==p->passwordKey.type())
				{
					p->passwordKey.setType(FXSSLPKey::RSA);
					p->passwordKey.setBitsLen(1024);
					p->passwordKey.generate();
				}
				RequestKeyAck ia(i->msgId(), p->passwordKey.publicKey());
				sendMsg(ia);
			}
			return FXIPCChannel::Handled;
		}
	case Open::id::code:
		{
			Open *i=(Open *) msg;
			bool isOk=false;
			for(QValueList<FXSQLDBServerPrivate::Database>::const_iterator it=p->permitted.begin(); it!=p->permitted.end(); ++it)
			{
				if(fxfilematch(it->driverName.text(), i->driver.text(), 0)
					&& fxfilematch(it->dbname.text(), i->dbname.text(), 0)
					&& fxfilematch(it->user.text(), i->user.text(), 0)
					&& (!it->host || it->host==i->host)
					&& (!it->port || it->port==i->port))
				{
					isOk=true;
					break;
				}
			}
			FXERRH(isOk, QTrans::tr("FXSQLDBServer", "Requested database '%1' via driver '%2' not permitted").arg(i->dbname).arg(i->driver), FXSQLDBSERVER_NOTPERMITTED, 0);
			OpenAck ia(i->msgId(), p->makeRandomId());
			FXSQLDBServerPrivate::ConnectionH *ch;
			FXERRHM(ch=new FXSQLDBServerPrivate::ConnectionH(p, ia.connh, FXSQLDBRegistry::make(i->driver, i->dbname, i->user, i->host, i->port)));
			FXRBOp unch=FXRBNew(ch);
			p->connhs.insert(ia.connh, ch);
			unch.dismiss();
			p->handles.insert(ia.connh, ch);
			QDICTDYNRESIZE(p->connhs);
			QDICTDYNRESIZE(p->handles);
			FXRBOp uneverything=FXRBObj(p->connhs, &QIntDict<FXSQLDBServerPrivate::ConnectionH>::remove, ia.connh);
			FXString password;
			if(!i->password.isNull())
			{
				QSSLDevice ssl(&i->password);
				FXStream s(&ssl);
				ssl.setKey(FXSSLKey().setAsymmetricKey(&p->passwordKey));
				ssl.open(IO_ReadOnly);
				s >> password;
			}
			ch->driver->open(password);
			uneverything.dismiss();
			if(i->wantsAck())
				sendMsg(ia);
			return FXIPCChannel::Handled;
		}
	case Close::id::code:
		{
			Close *i=(Close *) msg;
			FXSQLDBServerPrivate::ConnectionH *ch=p->connhs.find(i->connh);
			FXERRH(ch, QTrans::tr("FXSQLDBServer", "Connection handle not found"), FXSQLDBSERVER_NOTFOUND, 0);
			p->connhs.remove(i->connh);
			QDICTDYNRESIZE(p->connhs);
			QDICTDYNRESIZE(p->handles);
			if(i->wantsAck())
			{
				CloseAck ia(i->msgId());
				sendMsg(ia);
			}
			return FXIPCChannel::Handled;
		}
	case PrepareStatement::id::code:
		{
			PrepareStatement *i=(PrepareStatement *) msg;
			FXSQLDBServerPrivate::ConnectionH *ch=(FXSQLDBServerPrivate::ConnectionH *) p->handles.find(i->connh);
			FXERRH(ch, QTrans::tr("FXSQLDBServer", "Connection handle not found"), FXSQLDBSERVER_NOTFOUND, 0);
			PrepareStatementAck ia(i->msgId(), p->makeRandomId());
			FXSQLDBServerPrivate::StatementH *sh;
			FXERRHM(sh=new FXSQLDBServerPrivate::StatementH(p, ch, ia.stmth, ch->driver->prepare(i->statement)));
			FXRBOp unsh=FXRBNew(sh);
			ch->stmths.insert(ia.stmth, sh);
			unsh.dismiss();
			p->handles.insert(ia.stmth, sh);
			QDICTDYNRESIZE(ch->stmths);
			QDICTDYNRESIZE(p->handles);
			FXRBOp uneverything=FXRBObj(ch->stmths, &QIntDict<FXSQLDBServerPrivate::StatementH>::remove, ia.stmth);
			// Enumerate the parameter names
			FXString parname;
			for(FXint paridx=0; !(parname=sh->stmt->parameterName(paridx)).empty(); paridx++)
				ia.parNames.push_back(parname);
			// Execute immediately if asked
			if(i->request)
			{
				if((ia.cursh=p->execute(sh, i->cursflags)))
					p->getcursor(ia.flags, ia.columns, ia.rows, ia.data, ia.rowsToGo, ia.cursh, i->request);
			}
			uneverything.dismiss();
			if(i->wantsAck())
				sendMsg(ia);
			return FXIPCChannel::Handled;
		}
	case UnprepareStatement::id::code:
		{
			UnprepareStatement *i=(UnprepareStatement *) msg;
			FXSQLDBServerPrivate::StatementH *sh=(FXSQLDBServerPrivate::StatementH *) p->handles.find(i->stmth);
			FXERRH(sh, QTrans::tr("FXSQLDBServer", "Statement handle not found"), FXSQLDBSERVER_NOTFOUND, 0);
			FXSQLDBServerPrivate::ConnectionH *ch=sh->parent;
			ch->stmths.remove(i->stmth);
			QDICTDYNRESIZE(ch->stmths);
			QDICTDYNRESIZE(p->handles);
			return FXIPCChannel::Handled;
		}
	case BindParameter::id::code:
		{
			BindParameter *i=(BindParameter *) msg;
			FXSQLDBServerPrivate::StatementH *sh=(FXSQLDBServerPrivate::StatementH *) p->handles.find(i->stmth);
			FXERRH(sh, QTrans::tr("FXSQLDBServer", "Statement handle not found"), FXSQLDBSERVER_NOTFOUND, 0);
			FXSQLDB::SQLDataType partype=(FXSQLDB::SQLDataType) i->par.type;
			if(FXSQLDB::BLOB==partype)
			{
				QByteArray ba((FXuchar *) i->par.data.blob, i->par.data.length);
				sh->stmt->bind(i->paridx, partype, (void *) &ba);
				// Keep the blob data around as it is used by reference
				sh->blobs.push_back(i->par.data.blob);
				i->par.data.blob=0;
			}
			else if(partype>=FXSQLDB::VarChar && partype<=FXSQLDB::WChar)
			{
				FXString str(i->par.data.text, i->par.data.length);
				sh->stmt->bind(i->paridx, partype, (void *) &str);
			}
			else
				sh->stmt->bind(i->paridx, partype, (void *) &i->par.data.tinyint);
			if(i->wantsAck())
			{
				BindParameterAck ia(i->msgId());
				sendMsg(ia);
			}
			return FXIPCChannel::Handled;
		}
	case Execute::id::code:
		{
			Execute *i=(Execute *) msg;
			FXSQLDBServerPrivate::StatementH *sh=(FXSQLDBServerPrivate::StatementH *) p->handles.find(i->stmth);
			FXERRH(sh, QTrans::tr("FXSQLDBServer", "Statement handle not found"), FXSQLDBSERVER_NOTFOUND, 0);
			ExecuteAck ia(i->msgId(), p->execute(sh, i->cursflags));
			if(ia.cursh)
				p->getcursor(ia.flags, ia.columns, ia.rows, ia.data, ia.rowsToGo, ia.cursh, i->request);
			if(i->wantsAck())
				sendMsg(ia);
			return FXIPCChannel::Handled;
		}
	case CloseCursor::id::code:
		{
			CloseCursor *i=(CloseCursor *) msg;
			FXSQLDBServerPrivate::CursorH *ch=(FXSQLDBServerPrivate::CursorH *) p->handles.find(i->cursh);
			FXERRH(ch, QTrans::tr("FXSQLDBServer", "Cursor handle not found"), FXSQLDBSERVER_NOTFOUND, 0);
			FXSQLDBServerPrivate::StatementH *sh=ch->parent;
			sh->curshs.remove(i->cursh);
			QDICTDYNRESIZE(sh->curshs);
			QDICTDYNRESIZE(p->handles);
			return FXIPCChannel::Handled;
		}
	case RequestRows::id::code:
		{
			RequestRows *i=(RequestRows *) msg;
			FXSQLDBServerPrivate::CursorH *ch=(FXSQLDBServerPrivate::CursorH *) p->handles.find(i->cursh);
			FXERRH(ch, QTrans::tr("FXSQLDBServer", "Cursor handle not found"), FXSQLDBSERVER_NOTFOUND, 0);
			RequestRowsAck ia(i->msgId());
			p->getdata(ia.data, ia.rowsToGo, ch, i->request);
			if(i->wantsAck())
				sendMsg(ia);
			return FXIPCChannel::Handled;
		}
	case RequestColTypes::id::code:
		{
			RequestColTypes *i=(RequestColTypes *) msg;
			FXSQLDBServerPrivate::CursorH *ch=(FXSQLDBServerPrivate::CursorH *) p->handles.find(i->cursh);
			FXERRH(ch, QTrans::tr("FXSQLDBServer", "Cursor handle not found"), FXSQLDBSERVER_NOTFOUND, 0);
			RequestColTypesAck ia(i->msgId());
			FXERRHM(ia.data=new QMemArray<ColType>(ch->curs->columns()));
			for(FXuint n=0; n<ch->curs->columns(); n++)
				ch->curs->type((*ia.data)[n].datatype, (*ia.data)[n].size, n);
			if(i->wantsAck())
				sendMsg(ia);
			return FXIPCChannel::Handled;
		}
	case RequestColHeaders::id::code:
		{
			RequestColHeaders *i=(RequestColHeaders *) msg;
			FXSQLDBServerPrivate::CursorH *ch=(FXSQLDBServerPrivate::CursorH *) p->handles.find(i->cursh);
			FXERRH(ch, QTrans::tr("FXSQLDBServer", "Cursor handle not found"), FXSQLDBSERVER_NOTFOUND, 0);
			RequestColHeadersAck ia(i->msgId());
			FXERRHM(ia.data=new QMemArray<ColumnData>(ch->curs->columns()));
			for(FXuint n=0; n<ch->curs->columns(); n++)
			{
				FXSQLDBColumnRef cr=ch->curs->header(n);
				ColumnData &cd=(*ia.data)[n];
				cd.flags=cr->flags();
				cd.data.type=(FXuchar) cr->type();
				assert(FXSQLDB::VarChar==cr->type());
				cd.data.data.length=(FXuint) cr->size();
				cd.data.data.text=(FXchar *) cr->data();
			}
			if(i->wantsAck())
				sendMsg(ia);
			return FXIPCChannel::Handled;
		}
	case CopyCursor::id::code:
		{
			CopyCursor *i=(CopyCursor *) msg;
			FXSQLDBServerPrivate::CursorH *ch=(FXSQLDBServerPrivate::CursorH *) p->handles.find(i->cursh);
			FXERRH(ch, QTrans::tr("FXSQLDBServer", "Cursor handle not found"), FXSQLDBSERVER_NOTFOUND, 0);
			CopyCursorAck ia(i->msgId());
			FXSQLDBServerPrivate::CursorH *ch2;
			FXERRHM(ch2=new FXSQLDBServerPrivate::CursorH(p, ch->parent, p->makeRandomId(), ch->curs->copy()));
			FXRBOp unch2=FXRBNew(ch2);
			ch->parent->curshs.insert(ch2->handle, ch2);
			unch2.dismiss();
			p->handles.insert(ch2->handle, ch2);
			QDICTDYNRESIZE(ch->parent->curshs);
			QDICTDYNRESIZE(p->handles);
			if(i->wantsAck())
				sendMsg(ia);
			return FXIPCChannel::Handled;
		}
	}
	return FXIPCChannel::NotHandled;
}

}
