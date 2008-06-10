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
#ifndef FX_DISABLESQL

#include "TnFXSQLDB_ipc.h"
#include "FXRollback.h"
#include "FXSecure.h"
#include <qintdict.h>
#include "FXErrCodes.h"
#include "FXMemDbg.h"
#if defined(DEBUG) && !defined(FXMEMDBG_DISABLE)
static const char *_fxmemdbg_current_file_ = __FILE__;
#endif

namespace FX {

struct TnFXSQLDB_ipcPrivate
{
	typedef TnFXSQLDB_ipc::AckHandler AckHandler;
	typedef TnFXSQLDB_ipc::delMsgSpec delMsgSpec;
	bool asynchronous;
	FXuint prefetchNo, askForMore;
	struct Ack
	{
		FXIPCMsg *FXRESTRICT ia, *FXRESTRICT i;
		void *ref;
		void (*refdel)(void *);
		AckHandler handler;
		delMsgSpec iadel, idel;
		Ack(FXIPCMsg *FXRESTRICT _ia, FXIPCMsg *FXRESTRICT _i, void *_ref, void (*_refdel)(void *), AckHandler _handler, delMsgSpec _iadel, delMsgSpec _idel)
			: ia(_ia), i(_i), ref(_ref), refdel(_refdel), handler(_handler), iadel(_iadel), idel(_idel) { }
#ifndef HAVE_CPP0XRVALUEREFS
		Ack(const Ack &_o) : ia(_o.ia), i(_o.i), ref(_o.ref), refdel(_o.refdel),
			handler(const_cast<AckHandler &>(_o.handler)), iadel(_o.iadel), idel(_o.idel)
		{
			Ack &o=const_cast<Ack &>(_o);
#else
private:
		Ack(const Ack &);	// disable copy constructor
public:
		Ack(Ack &&o) : ia(o.ia), i(o.i), ref(o.ref), refdel(o.refdel),
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

	TnFXSQLDB_ipcPrivate() : asynchronous(false), prefetchNo(20), askForMore(15), connh(0) { }

	bool openAck(FXIPCMsg *FXRESTRICT _ia, FXIPCMsg *FXRESTRICT _i)
	{
		using namespace TnFXSQLDBIPCMsgsI;
		OpenAck *ia=(OpenAck *) _ia;
		connh=ia->connh;
		return true;
	}
};
const FXString TnFXSQLDB_ipc::MyName("IPC");
static TnFXSQLDBRegistry::Register<TnFXSQLDB_ipc> ipcdbregister;

TnFXSQLDB_ipc::TnFXSQLDB_ipc(const FXString &dbspec, const FXString &user, const QHostAddress &host, FXushort port)
: TnFXSQLDB(TnFXSQLDB::Capabilities().setTransactions().setQueryRows().setNoTypeConstraints().setHasBackwardsCursor().setHasSettableCursor().setHasStaticCursor().setAsynchronous(), TnFXSQLDB_ipc::MyName, dbspec, user, host, port), p(0)
{
	FXERRHM(p=new TnFXSQLDB_ipcPrivate);
}
TnFXSQLDB_ipc::~TnFXSQLDB_ipc()
{ FXEXCEPTIONDESTRUCT1 {
	close();
	FXDELETE(p);
} FXEXCEPTIONDESTRUCT2; }
inline void TnFXSQLDB_ipc::addAsyncMsg(FXIPCMsg *FXRESTRICT ia, FXIPCMsg *FXRESTRICT i, void *ref, void (*refdel)(void *), TnFXSQLDB_ipc::AckHandler handler, TnFXSQLDB_ipc::delMsgSpec iadel, TnFXSQLDB_ipc::delMsgSpec idel)
{
	assert(i->msgId());
	p->acksPending.push_back(TnFXSQLDB_ipcPrivate::Ack(ia, i, ref, refdel, handler, iadel, idel));
}
inline bool TnFXSQLDB_ipc::pollAcks(bool waitIfNoneReady)
{	// Note that getMsgAck() can throw exceptions received from the other side
	TnFXSQLDB_ipcPrivate::Ack &ack=p->acksPending.front();
	if(getMsgAck(ack.ia, ack.i, waitIfNoneReady ? FXINFINITE : 0))
	{
		p->acks.splice(p->acks.end(), p->acksPending, p->acksPending.begin());
		if(p->acks.back().handler(ack.ia, ack.i))
			p->acks.pop_back();
		return true;
	}
	return false;
}


bool TnFXSQLDB_ipc::isAsynchronous() const throw()
{
	return p->asynchronous;
}
TnFXSQLDB_ipc &TnFXSQLDB_ipc::setIsAsynchronous(bool v) throw()
{
	p->asynchronous=v;
	return *this;
}
void TnFXSQLDB_ipc::prefetching(FXuint &no, FXuint &askForMore) const throw()
{
	no=p->prefetchNo;
	askForMore=p->askForMore;
}
TnFXSQLDB_ipc &TnFXSQLDB_ipc::setPrefetching(FXuint no, FXuint askForMore) throw()
{
	p->prefetchNo=no;
	p->askForMore=askForMore;
	return *this;
}


const FXString &TnFXSQLDB_ipc::versionInfo() const
{
	static const FXString myversion("TnFOX SQL DB via IPC channel v1.00");
	return myversion;
}

void TnFXSQLDB_ipc::open(const FXString &password)
{
	if(!p->connh)
	{
		using namespace TnFXSQLDBIPCMsgsI;
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
		sendAsyncMsg<OpenAck>(i, (TnFXSQLDBCursor *) 0, AckHandler(*p, &TnFXSQLDB_ipcPrivate::openAck));
		uni.dismiss();
		if(!p->asynchronous)
			synchronise();
	}
}
void TnFXSQLDB_ipc::close()
{
	if(p->connh)
	{	// Wait for everything to finish processing first
		while(!p->acksPending.empty())
			pollAcks(true);
		using namespace TnFXSQLDBIPCMsgsI;
		Close i(p->connh);
		CloseAck ia;
		sendMsg(ia, i);
		p->acks.clear();
		p->connh=0;
	}
}

struct TnFXSQLDB_ipc::Column : public TnFXSQLDBColumn
{
	friend struct TnFXSQLDB_ipc::Cursor;
	Column(FXuint flags, TnFXSQLDBCursor *parent, FXuint column, FXint row, TnFXSQLDB::SQLDataType type) : TnFXSQLDBColumn(flags, parent, column, row, type) { }
	Column(const Column &o) : TnFXSQLDBColumn(o) { }
	virtual TnFXSQLDBColumnRef copy() const
	{
		Column *c;
		FXERRHM(c=new Column(*this));
		return c;
	}
};
struct TnFXSQLDB_ipc::Cursor : public TnFXSQLDBCursor
{
	TnFXSQLDB_ipc *o;
	TnFXSQLDB_ipcPrivate *p;
	FXuint cursh;
	struct Buffer
	{
		QMemArray<TnFXSQLDBIPCMsgsI::ColumnData> *data;
		FXint rows, rowsToGo;
		Buffer() : data(0), rows(0), rowsToGo(0) { }
		Buffer(const Buffer &o) : data(0), rows(o.rows), rowsToGo(o.rowsToGo)
		{
			if(o.data)
			{
				FXERRHM(data=new QMemArray<TnFXSQLDBIPCMsgsI::ColumnData>);
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
	mutable QMemArray<TnFXSQLDBIPCMsgsI::ColType> *colTypes;
	QMemArray<TnFXSQLDBIPCMsgsI::ColumnData> *headers;

	inline void configBuffer(int buffer, QMemArray<TnFXSQLDBIPCMsgsI::ColumnData> *&data, FXint rowsToGo)
	{
		Buffer *b=buffers[buffer];
		b->data=data; data=0;
		b->rows=b->data->count()/columns();
		b->rowsToGo=rowsToGo;
		int_setRowsReady(bufferrowbegin, bufferrowbegin+buffers[0]->rows+buffers[1]->rows);
	}
	inline void config(FXuint _cursh, FXint rows, FXuint flags, FXuint columns, QMemArray<TnFXSQLDBIPCMsgsI::ColumnData> *&data, FXint rowsToGo)
	{
		cursh=_cursh;
		int_setInternals(&rows, &flags, &columns);
		configBuffer(0, data, rowsToGo);
	}
	bool executeAck(FXIPCMsg *FXRESTRICT _ia, FXIPCMsg *FXRESTRICT _i)
	{
		using namespace TnFXSQLDBIPCMsgsI;
		ExecuteAck *ia=(ExecuteAck *) _ia;
		if(ia->cursh)
			config(ia->cursh, ia->rows, ia->flags, ia->columns, ia->data, ia->rowsToGo);
		return true;
	}
	bool requestRowsAck(FXIPCMsg *FXRESTRICT _ia, FXIPCMsg *FXRESTRICT _i)
	{
		using namespace TnFXSQLDBIPCMsgsI;
		RequestRowsAck *ia=(RequestRowsAck *) _ia;
		configBuffer(buffers[0]->data ? 1 : 0, ia->data, ia->rowsToGo);
		return true;
	}
	bool copyCursorAck(FXIPCMsg *FXRESTRICT _ia, FXIPCMsg *FXRESTRICT _i)
	{
		using namespace TnFXSQLDBIPCMsgsI;
		CopyCursorAck *ia=(CopyCursorAck *) _ia;
		cursh=ia->cursh;
		return true;
	}
	Cursor(TnFXSQLDB_ipcPrivate *_p, TnFXSQLDB_ipc *_o, FXuint flags, TnFXSQLDBStatement *parent, QWaitCondition *latch, FXuint &stmth, bool alreadyInProgress=false)
		: TnFXSQLDBCursor(flags, parent, latch, (FXuint)-1), o(_o), p(_p), cursh(0), bufferrowbegin(0), colTypes(0), headers(0)
	{
		buffers[0]=buffers[1]=0;
		FXERRHM(buffers[0]=new Buffer);
		FXERRHM(buffers[1]=new Buffer);
		if(!alreadyInProgress)
		{
			while(!stmth && !p->acksPending.empty())
				o->pollAcks(true);
			assert(stmth);
			using namespace TnFXSQLDBIPCMsgsI;
			Execute *i;
			FXERRHM(i=new Execute(stmth, p->prefetchNo, flags));
			FXRBOp uni=FXRBNew(i);
			o->sendAsyncMsg<ExecuteAck>(i, this, AckHandler(*this, &Cursor::executeAck));
			uni.dismiss();
		}
		// Ensure I point at record zero, not -1
		TnFXSQLDBCursor::at(0);
	}
	Cursor(const Cursor &ot) : TnFXSQLDBCursor(ot), o(ot.o), p(ot.p), cursh(0), bufferrowbegin(ot.bufferrowbegin), colTypes(0), headers(0)
	{
		buffers[0]=buffers[1]=0;
		if(ot.buffers[0]) FXERRHM(buffers[0]=new Buffer(*ot.buffers[0]));
		if(ot.buffers[1]) FXERRHM(buffers[1]=new Buffer(*ot.buffers[1]));
		if(ot.colTypes) FXERRHM(colTypes=new QMemArray<TnFXSQLDBIPCMsgsI::ColType>(*ot.colTypes));
		if(ot.headers)
		{
			FXERRHM(headers=new QMemArray<TnFXSQLDBIPCMsgsI::ColumnData>);
			for(FXuint n=0; n<ot.headers->count(); n++)
				headers->at(n).copy(ot.headers->at(n));
		}
	}
	~Cursor()
	{
		while(!cursh && !p->acksPending.empty())
			o->pollAcks(true);
		assert(cursh);
		using namespace TnFXSQLDBIPCMsgsI;
		CloseCursor i(cursh);
		o->sendMsg(i);
		cursh=0;

		FXDELETE(headers);
		FXDELETE(colTypes);
		FXDELETE(buffers[1]);
		FXDELETE(buffers[0]);
	}
	virtual TnFXSQLDBCursorRef copy() const
	{
		while(!cursh && !p->acksPending.empty())
			o->pollAcks(true);
		assert(cursh);
		Cursor *c;
		FXERRHM(c=new Cursor(*this));
		FXRBOp unc=FXRBNew(c);
		using namespace TnFXSQLDBIPCMsgsI;
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
			using namespace TnFXSQLDBIPCMsgsI;
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
	using TnFXSQLDBCursor::at;
	virtual FXint at(FXint newrow)
	{
		while((!cursh || !buffers[0]->data) && !p->acksPending.empty())
			o->pollAcks(true);
		FXint endofbuffer=bufferrowbegin+buffers[0]->rows;
		if(newrow>=bufferrowbegin && newrow<endofbuffer)
		{
			int_setAtEnd(false);
			return TnFXSQLDBCursor::at(newrow);
		}
		if(buffers[1]->data && newrow<endofbuffer+buffers[1]->rows)
		{	// It's in the next buffer
			retireBuffer();
			return Cursor::at(newrow);
		}
		// Ok, it's either before our buffers or after them so go fetch
		buffers[0]->reset(); buffers[1]->reset();
		bufferrowbegin=newrow;
		using namespace TnFXSQLDBIPCMsgsI;
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
			return TnFXSQLDBCursor::backwards();
		return at(mypos-1);
	}
	virtual FXint forwards()
	{
		while((!cursh || !buffers[0]->data) && !p->acksPending.empty())
			o->pollAcks(true);
		if(!atEnd())
		{
			FXint ret=TnFXSQLDBCursor::forwards(), endofbuffer=bufferrowbegin+buffers[0]->rows;
			if(ret>=endofbuffer)
			{
				int_setAtEnd(!buffers[0]->rowsToGo);
				retireBuffer();
			}
			else if(ret==endofbuffer-(FXint) p->askForMore)
				askForNextBuffer();
			return ret;
		}
		return TnFXSQLDBCursor::at();
	}
	virtual void type(TnFXSQLDB::SQLDataType &datatype, FXint &size, FXuint no) const
	{
		while(!cursh && !p->acksPending.empty())
			o->pollAcks(true);
		assert(cursh);
		using namespace TnFXSQLDBIPCMsgsI;
		if(!colTypes)
		{
			RequestColTypes i(cursh);
			RequestColTypesAck ia;
			o->sendMsg(ia, i);
			colTypes=ia.data;
			ia.data=0;
		}
		FXERRH(no<colTypes->count(), QTrans::tr("TnFXSQLDB_ipc", "Column does not exist"), 0, 0);
		datatype=(*colTypes)[no].datatype;
		size=(*colTypes)[no].size;
	}
	virtual TnFXSQLDBColumnRef header(FXuint no)
	{
		while(!cursh && !p->acksPending.empty())
			o->pollAcks(true);
		assert(cursh);
		using namespace TnFXSQLDBIPCMsgsI;
		if(!headers)
		{
			RequestColHeaders i(cursh);
			RequestColHeadersAck ia;
			o->sendMsg(ia, i);
			headers=ia.data;
			ia.data=0;
		}
		FXERRH(no<headers->count(), QTrans::tr("TnFXSQLDB_ipc", "Header does not exist"), 0, 0);
		ColumnData &cd=(*headers)[no];
		Column *c;
		FXERRHM(c=new Column(cd.flags, this, no, -1, (TnFXSQLDB::SQLDataType) cd.data.type));
		c->mydatalen=cd.data.data.length;
		if(BLOB==cd.data.type)
			c->mydata=cd.data.data.blob;
		else if(cd.data.type>=VarChar && cd.data.type<=WChar)
			c->mydata=cd.data.data.text;
		else
			c->mydata=&cd.data.data.tinyint;
		return c;
	}
	virtual TnFXSQLDBColumnRef data(FXuint no)
	{
		while((!cursh || !buffers[0]->data) && !p->acksPending.empty())
			o->pollAcks(true);
		assert(cursh && buffers[0]->data);
		FXuint dataoffset=(at()*columns())+no-bufferrowbegin;
		assert(dataoffset<buffers[0]->data->count());
		using namespace TnFXSQLDBIPCMsgsI;
		ColumnData &cd=(*buffers[0]->data)[dataoffset];
		Column *c;
		FXERRHM(c=new Column(cd.flags, this, no, at(), (TnFXSQLDB::SQLDataType) cd.data.type));
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
struct TnFXSQLDB_ipc::Statement : public TnFXSQLDBStatement
{
	TnFXSQLDB_ipc *o;
	TnFXSQLDB_ipcPrivate *p;
	FXuint stmth;
	QMemArray<FXString> parNames;
	Cursor *inProgressCursor;
	bool prepareStatementAck(FXIPCMsg *FXRESTRICT _ia, FXIPCMsg *FXRESTRICT _i)
	{
		using namespace TnFXSQLDBIPCMsgsI;
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
	bool bindParameterAck(FXIPCMsg *FXRESTRICT _ia, FXIPCMsg *FXRESTRICT _i)
	{
		using namespace TnFXSQLDBIPCMsgsI;
		BindParameterAck *ia=(BindParameterAck *) _ia;
		return true;
	}
	bool executeAck(FXIPCMsg *FXRESTRICT _ia, FXIPCMsg *FXRESTRICT _i)
	{
		using namespace TnFXSQLDBIPCMsgsI;
		ExecuteAck *ia=(ExecuteAck *) _ia;
		assert(!ia->cursh);
		return true;
	}
	Statement(TnFXSQLDB_ipcPrivate *_p, TnFXSQLDB_ipc *parent, const FXString &text, bool executeNow=false, FXuint cursflags=0)
		: TnFXSQLDBStatement(parent, text), o(parent), p(_p), stmth(0), inProgressCursor(0)
	{
		using namespace TnFXSQLDBIPCMsgsI;
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
		using namespace TnFXSQLDBIPCMsgsI;
		UnprepareStatement i(stmth);
		o->sendMsg(i);
		stmth=0;
	}
	virtual TnFXSQLDBStatementRef copy() const
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
	virtual TnFXSQLDBStatement &bind(FXint idx, TnFXSQLDB::SQLDataType datatype, void *data)
	{
		while(!stmth && !p->acksPending.empty())
			o->pollAcks(true);
		assert(stmth);
		using namespace TnFXSQLDBIPCMsgsI;
		BindParameter *i;
		FXERRHM(i=new BindParameter(stmth, idx, datatype));
		FXRBOp uni=FXRBNew(i);
		switch(datatype)
		{
		case TnFXSQLDB::Null:
			break;
		case TnFXSQLDB::VarChar:
		case TnFXSQLDB::Char:
		case TnFXSQLDB::WVarChar:
		case TnFXSQLDB::WChar:
			i->par.data.length=((FXString *) data)->length();
			i->par.data.text=(FXchar *)((FXString *) data)->text();
			break;

		case TnFXSQLDB::TinyInt:
			i->par.data.length=sizeof(FXuchar);
			i->par.data.tinyint=*(FXuchar *) data;
			break;
		case TnFXSQLDB::SmallInt:
			i->par.data.length=sizeof(FXushort);
			i->par.data.smallint=*(FXushort *) data;
			break;
		case TnFXSQLDB::Integer:
			i->par.data.length=sizeof(FXuint);
			i->par.data.integer=*(FXuint *) data;
			break;
		case TnFXSQLDB::BigInt:
		case TnFXSQLDB::Decimal:
		case TnFXSQLDB::Numeric:
			i->par.data.length=sizeof(FXulong);
			i->par.data.bigint=*(FXulong *) data;
			break;
		case TnFXSQLDB::Real:
			i->par.data.length=sizeof(float);
			i->par.data.real=*(float *) data;
			break;
		case TnFXSQLDB::Double:
		case TnFXSQLDB::Float:
			i->par.data.length=sizeof(double);
			i->par.data.double_=*(double *) data;
			break;
		case TnFXSQLDB::Timestamp:
		case TnFXSQLDB::Date:
		case TnFXSQLDB::Time:
			i->par.data.length=sizeof(FXTime);
			*(FXTime *)&i->par.data.timestamp=*(FXTime *) data;
			break;

		case TnFXSQLDB::BLOB:
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
	TnFXSQLDBCursorRef int_execute(FXuint flags, QWaitCondition *latch)
	{
		TnFXSQLDBCursorRef cr;
		Cursor *c=0;
		FXERRHM(cr=c=new Cursor(p, o, flags, this, latch, stmth, true));
		assert(!inProgressCursor);
		inProgressCursor=c;
		if(!p->asynchronous)
			o->synchronise();
		return cr;
	}
	virtual TnFXSQLDBCursorRef execute(FXuint flags=TnFXSQLDBCursor::IsDynamic|TnFXSQLDBCursor::ForwardOnly, QWaitCondition *latch=0)
	{
		TnFXSQLDBCursorRef cr;
		FXERRHM(cr=new Cursor(p, o, flags, this, latch, stmth));
		if(!p->asynchronous)
			o->synchronise();
		return cr;
	}
	virtual void immediate()
	{	// We just pretend to be a Cursor here for simplicity
		assert(stmth);
		using namespace TnFXSQLDBIPCMsgsI;
		Execute *i;
		FXERRHM(i=new Execute(stmth, p->prefetchNo, 0/*don't return a cursh*/));
		FXRBOp uni=FXRBNew(i);
		o->sendAsyncMsg<ExecuteAck>(i, this, AckHandler(*this, &Statement::executeAck));
		uni.dismiss();
		if(!p->asynchronous)
			o->synchronise();
	}
};

TnFXSQLDBStatementRef TnFXSQLDB_ipc::prepare(const FXString &text)
{
	while(!p->connh && !p->acksPending.empty())
		pollAcks(true);
	FXERRH(p->connh, QTrans::tr("TnFXSQLDB_ipc", "Database is not open"), 0, 0);
	TnFXSQLDBStatementRef sr;
	FXERRHM(sr=new Statement(p, this, text));
	if(!p->asynchronous)
		synchronise();
	return sr;
}

TnFXSQLDBCursorRef TnFXSQLDB_ipc::execute(const FXString &text, FXuint flags, QWaitCondition *latch)
{
	while(!p->connh && !p->acksPending.empty())
		pollAcks(true);
	FXERRH(p->connh, QTrans::tr("TnFXSQLDB_ipc", "Database is not open"), 0, 0);
	TnFXSQLDBStatementRef sr;
	Statement *s=0;
	FXERRHM(sr=s=new Statement(p, this, text, true, flags));
	return s->int_execute(flags, latch);
}
void TnFXSQLDB_ipc::immediate(const FXString &text)
{
	while(!p->connh && !p->acksPending.empty())
		pollAcks(true);
	FXERRH(p->connh, QTrans::tr("TnFXSQLDB_ipc", "Database is not open"), 0, 0);
	TnFXSQLDBStatementRef sr;
	FXERRHM(sr=new Statement(p, this, text, true));
	if(!p->asynchronous)
		synchronise();
}

void TnFXSQLDB_ipc::synchronise()
{
	while(!p->acksPending.empty())
		pollAcks(true);
}

//*******************************************************************************

struct TnFXSQLDBServerPrivate
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
		TnFXSQLDBServerPrivate *p;
		FXuint handle;
		Handle(int _type, TnFXSQLDBServerPrivate *_p, FXuint _handle) : type(_type), p(_p), handle(_handle) { }
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
		TnFXSQLDBCursorRef curs;
		CursorH(TnFXSQLDBServerPrivate *p, StatementH *_parent, FXuint cursh, TnFXSQLDBCursorRef _curs) : Handle(3, p, cursh), parent(_parent), curs(_curs) { }
	};
	struct StatementH : Handle
	{
		ConnectionH *parent;
		TnFXSQLDBStatementRef stmt;
		QIntDict<CursorH> curshs;
		QMemArray<void *> blobs;
		StatementH(TnFXSQLDBServerPrivate *p, ConnectionH *_parent, FXuint stmth, TnFXSQLDBStatementRef _stmt) : Handle(2, p, stmth), parent(_parent), stmt(_stmt), curshs(1, true) { }
		~StatementH()
		{
			for(FXuint n=0; n<blobs.count(); n++)
				free(blobs[n]);
		}
	};
	struct ConnectionH : Handle
	{
		FXAutoPtr<TnFXSQLDB> driver;
		QIntDict<StatementH> stmths;
		ConnectionH(TnFXSQLDBServerPrivate *p, FXuint connh, FXAutoPtr<TnFXSQLDB> _driver) : Handle(1, p, connh), driver(_driver), stmths(1, true) { }
	};
	QIntDict<Handle> handles;			// For quick lookup
	QIntDict<ConnectionH> connhs;
	TnFXSQLDBServerPrivate() : connhs(1, true), handles(1) { }
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
		TnFXSQLDBCursorRef sr=sh->stmt->execute(!cursflags ? TnFXSQLDBCursor::IsDynamic|TnFXSQLDBCursor::ForwardOnly : cursflags);
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
	void getcursor(FXuint &flags, FXuint &columns, FXint &rows, QMemArray<TnFXSQLDBIPCMsgsI::ColumnData> *&data, FXint &rowsToGo, FXuint cursh, FXuint request)
	{
		CursorH *ch=(CursorH *) handles.find(cursh);
		FXERRH(ch, QTrans::tr("TnFXSQLDBServer", "Cursor handle not found"), TNFXSQLDBSERVER_NOTFOUND, 0);
		flags=ch->curs->flags();
		columns=ch->curs->columns();
		rows=ch->curs->rows();
		if(request) getdata(data, rowsToGo, ch, request);
	}
	void getdata(QMemArray<TnFXSQLDBIPCMsgsI::ColumnData> *&data, FXint &rowsToGo, CursorH *ch, FXuint request)
	{
		FXuint r, origrequest=request;
		if(request>10000) request=10000;
		if(!data)
			FXERRHM(data=new QMemArray<TnFXSQLDBIPCMsgsI::ColumnData>(ch->curs->columns()*request));
		else
			data->resize(ch->curs->columns()*request);
		TnFXSQLDBCursor *curs=PtrPtr(ch->curs);
		for(r=0; r<request && !curs->atEnd(); r++, curs->next())
		{
			for(FXuint c=0; c<curs->columns(); c++)
			{
				TnFXSQLDBColumnRef cr_=curs->data(c);
				TnFXSQLDBColumn *cr=PtrPtr(cr_);
				TnFXSQLDBIPCMsgsI::ColumnData &cd=(*data)[r*curs->columns()+c];
				cd.data.data.length=(FXuint) cr->size();
				switch((cd.data.type=(FXuchar) cr->type()))
				{
				case TnFXSQLDB::Null:
					break;
				case TnFXSQLDB::VarChar:
				case TnFXSQLDB::Char:
				case TnFXSQLDB::WVarChar:
				case TnFXSQLDB::WChar:
					FXERRHM(cd.data.data.text=(FXchar *) malloc(cd.data.data.length+1));
					cd.data.mydata=true;
					memcpy(cd.data.data.text, cr->data(), cd.data.data.length+1);
					break;

				case TnFXSQLDB::TinyInt:
				case TnFXSQLDB::SmallInt:
				case TnFXSQLDB::Integer:
				case TnFXSQLDB::BigInt:
				case TnFXSQLDB::Decimal:
				case TnFXSQLDB::Numeric:
				case TnFXSQLDB::Real:
				case TnFXSQLDB::Double:
				case TnFXSQLDB::Float:
				case TnFXSQLDB::Timestamp:
				case TnFXSQLDB::Date:
				case TnFXSQLDB::Time:
					memcpy(&cd.data.data.tinyint, cr->data(), cr->size());
					break;

				case TnFXSQLDB::BLOB:
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

TnFXSQLDBServer::TnFXSQLDBServer() : p(0)
{
	FXERRHM(p=new TnFXSQLDBServerPrivate);
}
TnFXSQLDBServer::~TnFXSQLDBServer()
{ FXEXCEPTIONDESTRUCT1 {
	FXDELETE(p);
} FXEXCEPTIONDESTRUCT2; }

TnFXSQLDBServer &TnFXSQLDBServer::addDatabase(const FXString &driverName, const FXString &dbname, const FXString &user, const QHostAddress &host, FXushort port)
{
	p->permitted.push_back(TnFXSQLDBServerPrivate::Database(driverName, dbname, user, host, port));
	return *this;
}
bool TnFXSQLDBServer::removeDatabase(const FXString &driverName, const FXString &dbname, const FXString &user, const QHostAddress &host, FXushort port)
{
	size_t c=p->permitted.size();
	p->permitted.remove(TnFXSQLDBServerPrivate::Database(driverName, dbname, user, host, port));
	return c!=p->permitted.size();
}

FXIPCChannel::HandledCode TnFXSQLDBServer::handleMsg(FXIPCMsg *msg)
{
	using namespace TnFXSQLDBIPCMsgsI;
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
			for(QValueList<TnFXSQLDBServerPrivate::Database>::const_iterator it=p->permitted.begin(); it!=p->permitted.end(); ++it)
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
			FXERRH(isOk, QTrans::tr("TnFXSQLDBServer", "Requested database '%1' via driver '%2' not permitted").arg(i->dbname).arg(i->driver), TNFXSQLDBSERVER_NOTPERMITTED, 0);
			OpenAck ia(i->msgId(), p->makeRandomId());
			TnFXSQLDBServerPrivate::ConnectionH *ch;
			FXERRHM(ch=new TnFXSQLDBServerPrivate::ConnectionH(p, ia.connh, TnFXSQLDBRegistry::make(i->driver, i->dbname, i->user, i->host, i->port)));
			FXRBOp unch=FXRBNew(ch);
			p->connhs.insert(ia.connh, ch);
			unch.dismiss();
			p->handles.insert(ia.connh, ch);
			QDICTDYNRESIZE(p->connhs);
			QDICTDYNRESIZE(p->handles);
			FXRBOp uneverything=FXRBObj(p->connhs, &QIntDict<TnFXSQLDBServerPrivate::ConnectionH>::remove, ia.connh);
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
			TnFXSQLDBServerPrivate::ConnectionH *ch=p->connhs.find(i->connh);
			FXERRH(ch, QTrans::tr("TnFXSQLDBServer", "Connection handle not found"), TNFXSQLDBSERVER_NOTFOUND, 0);
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
			TnFXSQLDBServerPrivate::ConnectionH *ch=(TnFXSQLDBServerPrivate::ConnectionH *) p->handles.find(i->connh);
			FXERRH(ch, QTrans::tr("TnFXSQLDBServer", "Connection handle not found"), TNFXSQLDBSERVER_NOTFOUND, 0);
			PrepareStatementAck ia(i->msgId(), p->makeRandomId());
			TnFXSQLDBServerPrivate::StatementH *sh;
			FXERRHM(sh=new TnFXSQLDBServerPrivate::StatementH(p, ch, ia.stmth, ch->driver->prepare(i->statement)));
			FXRBOp unsh=FXRBNew(sh);
			ch->stmths.insert(ia.stmth, sh);
			unsh.dismiss();
			p->handles.insert(ia.stmth, sh);
			QDICTDYNRESIZE(ch->stmths);
			QDICTDYNRESIZE(p->handles);
			FXRBOp uneverything=FXRBObj(ch->stmths, &QIntDict<TnFXSQLDBServerPrivate::StatementH>::remove, ia.stmth);
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
			TnFXSQLDBServerPrivate::StatementH *sh=(TnFXSQLDBServerPrivate::StatementH *) p->handles.find(i->stmth);
			FXERRH(sh, QTrans::tr("TnFXSQLDBServer", "Statement handle not found"), TNFXSQLDBSERVER_NOTFOUND, 0);
			TnFXSQLDBServerPrivate::ConnectionH *ch=sh->parent;
			ch->stmths.remove(i->stmth);
			QDICTDYNRESIZE(ch->stmths);
			QDICTDYNRESIZE(p->handles);
			return FXIPCChannel::Handled;
		}
	case BindParameter::id::code:
		{
			BindParameter *i=(BindParameter *) msg;
			TnFXSQLDBServerPrivate::StatementH *sh=(TnFXSQLDBServerPrivate::StatementH *) p->handles.find(i->stmth);
			FXERRH(sh, QTrans::tr("TnFXSQLDBServer", "Statement handle not found"), TNFXSQLDBSERVER_NOTFOUND, 0);
			TnFXSQLDB::SQLDataType partype=(TnFXSQLDB::SQLDataType) i->par.type;
			if(TnFXSQLDB::BLOB==partype)
			{
				QByteArray ba((FXuchar *) i->par.data.blob, i->par.data.length);
				sh->stmt->bind(i->paridx, partype, (void *) &ba);
				// Keep the blob data around as it is used by reference
				sh->blobs.push_back(i->par.data.blob);
				i->par.data.blob=0;
			}
			else if(partype>=TnFXSQLDB::VarChar && partype<=TnFXSQLDB::WChar)
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
			TnFXSQLDBServerPrivate::StatementH *sh=(TnFXSQLDBServerPrivate::StatementH *) p->handles.find(i->stmth);
			FXERRH(sh, QTrans::tr("TnFXSQLDBServer", "Statement handle not found"), TNFXSQLDBSERVER_NOTFOUND, 0);
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
			TnFXSQLDBServerPrivate::CursorH *ch=(TnFXSQLDBServerPrivate::CursorH *) p->handles.find(i->cursh);
			FXERRH(ch, QTrans::tr("TnFXSQLDBServer", "Cursor handle not found"), TNFXSQLDBSERVER_NOTFOUND, 0);
			TnFXSQLDBServerPrivate::StatementH *sh=ch->parent;
			sh->curshs.remove(i->cursh);
			QDICTDYNRESIZE(sh->curshs);
			QDICTDYNRESIZE(p->handles);
			return FXIPCChannel::Handled;
		}
	case RequestRows::id::code:
		{
			RequestRows *i=(RequestRows *) msg;
			TnFXSQLDBServerPrivate::CursorH *ch=(TnFXSQLDBServerPrivate::CursorH *) p->handles.find(i->cursh);
			FXERRH(ch, QTrans::tr("TnFXSQLDBServer", "Cursor handle not found"), TNFXSQLDBSERVER_NOTFOUND, 0);
			RequestRowsAck ia(i->msgId());
			p->getdata(ia.data, ia.rowsToGo, ch, i->request);
			if(i->wantsAck())
				sendMsg(ia);
			return FXIPCChannel::Handled;
		}
	case RequestColTypes::id::code:
		{
			RequestColTypes *i=(RequestColTypes *) msg;
			TnFXSQLDBServerPrivate::CursorH *ch=(TnFXSQLDBServerPrivate::CursorH *) p->handles.find(i->cursh);
			FXERRH(ch, QTrans::tr("TnFXSQLDBServer", "Cursor handle not found"), TNFXSQLDBSERVER_NOTFOUND, 0);
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
			TnFXSQLDBServerPrivate::CursorH *ch=(TnFXSQLDBServerPrivate::CursorH *) p->handles.find(i->cursh);
			FXERRH(ch, QTrans::tr("TnFXSQLDBServer", "Cursor handle not found"), TNFXSQLDBSERVER_NOTFOUND, 0);
			RequestColHeadersAck ia(i->msgId());
			FXERRHM(ia.data=new QMemArray<ColumnData>(ch->curs->columns()));
			for(FXuint n=0; n<ch->curs->columns(); n++)
			{
				TnFXSQLDBColumnRef cr=ch->curs->header(n);
				ColumnData &cd=(*ia.data)[n];
				cd.flags=cr->flags();
				cd.data.type=(FXuchar) cr->type();
				assert(TnFXSQLDB::VarChar==cr->type());
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
			TnFXSQLDBServerPrivate::CursorH *ch=(TnFXSQLDBServerPrivate::CursorH *) p->handles.find(i->cursh);
			FXERRH(ch, QTrans::tr("TnFXSQLDBServer", "Cursor handle not found"), TNFXSQLDBSERVER_NOTFOUND, 0);
			CopyCursorAck ia(i->msgId());
			TnFXSQLDBServerPrivate::CursorH *ch2;
			FXERRHM(ch2=new TnFXSQLDBServerPrivate::CursorH(p, ch->parent, p->makeRandomId(), ch->curs->copy()));
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

#endif
