/********************************************************************************
*                                                                               *
*                 L a n g u a g e    T r a n s l a t i o n                      *
*                                                                               *
*********************************************************************************
* Copyright (C) 2003 by Niall Douglas.   All Rights Reserved.                   *
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

#include <qdict.h>
#include <qcstring.h>
#include <qptrlist.h>
#include <qptrvector.h>
#include <qvaluelist.h>
#include <stdarg.h>
#include <locale.h>
#include <ctype.h>
#include <assert.h>
#include "fxver.h"
#include "fxdefs.h"
#include "FXTrans.h"
#include "FXString.h"
#include "FXProcess.h"
#include "FXThread.h"
#include "FXFile.h"
#include "FXBuffer.h"
#include "FXPtrHold.h"
#include "FXRollback.h"
#include "FXErrCodes.h"
#include "FXGZipDevice.h"
#include "FXApp.h"
#ifdef WIN32
#include "WindowsGubbins.h"
#endif
#include "FXMemDbg.h"
#if defined(DEBUG) && defined(FXMEMDBG_H)
static const char *_fxmemdbg_current_file_ = __FILE__;
#endif

namespace FX {

// Defines the longest text literal which can be read in
#define MAXLITERALLEN 4096

struct FXTransEmbeddedFile
{
	const FXuchar *buffer;
	FXuint len;
	void *staticaddr, *dllstart, *dllend;
	FXString dllpath;
	FXTransEmbeddedFile(const FXuchar *b, FXuint l, void *sa): buffer(b), len(l), staticaddr(sa), dllstart(0), dllend(0) { }
	bool operator==(const FXTransEmbeddedFile &o) const { return buffer==o.buffer && len==o.len; }
	bool operator!=(const FXTransEmbeddedFile &o) const { return buffer!=o.buffer || len!=o.len; }
};
static QValueList<FXTransEmbeddedFile> *embedded;
static FXTrans *me;

class FXTransPrivate : public FXRWMutex
{
public:
	bool quitNow;
	FXString language[3];
	FXString country[3];
	struct Overrides_t
	{
		FXString language;
		FXString country;
	} overrides;
	bool noTransFiles;

	struct LangTransItem
	{
		FXString *srcfile, *classname, *hint;	// Links to items in ModuleTrans::srcfiles, classnames, hints
		QMemArray<FXString> pars;
		FXString *translation;					// Links to items in parent ModuleTrans::transstrs
		LangTransItem() : srcfile(0), classname(0), hint(0), translation(0) { }
	};
	typedef QValueList<LangTransItem> LangTrans;
	struct ModuleTrans
	{
		const void *staticaddr, *modulestart, *moduleend;
		FXString modulepath;
		QDict<LangTrans> langs; // by <langid>[_<region>[@<modifier>]]
		QPtrList<FXString> srcfiles, classnames, hints, transstrs;
		ModuleTrans(const void *sa, const void *modstart, const void *modend, const FXString &modpath)
			: staticaddr(sa), modulestart(modstart), moduleend(modend), modulepath(modpath),
			langs(7, true, true), srcfiles(true), classnames(true), hints(true), transstrs(true) { }
		FXString *getLiteral(QPtrList<FXString> &list, const FXString &str) const
		{
			if(str.empty()) return 0;
			FXString *ret;
			for(QPtrListIterator<FXString> it(list); (ret=it.current()); ++it)
			{
				if(*ret==str) return ret;
			}
			FXERRHM(ret=new FXString(str));
			FXRBOp unnew=FXRBNew(ret);
			list.append(ret);
			unnew.dismiss();
			return ret;
		}
	};
	struct EnglishTrans : public QPtrVector<ModuleTrans>
	{
		EnglishTrans() : QPtrVector<ModuleTrans>(true) { }
		ModuleTrans *find(const void *sa) const
		{
			ModuleTrans *i;
			for(QPtrVectorIterator<ModuleTrans> it(*this); (i=it.current()); ++it)
			{
				if(sa==i->staticaddr) return i;
			}
			return 0;
		}
	};
	QDict<EnglishTrans> dict; // by english literal from the code
	FXTransPrivate() : quitNow(false), noTransFiles(true), dict(257, true, true), FXRWMutex() { }
	void dump() const
	{
		QDictIterator<FXTransPrivate::EnglishTrans> engit(dict);
		for(FXTransPrivate::EnglishTrans *engtrans; (engtrans=engit.current()); ++engit)
		{
			QPtrVectorIterator<FXTransPrivate::ModuleTrans> modit(*engtrans);
			for(FXTransPrivate::ModuleTrans *modtrans; (modtrans=modit.current()); ++modit)
			{
				QDictIterator<FXTransPrivate::LangTrans> langit(modtrans->langs);
				for(FXTransPrivate::LangTrans *langtrans; (langtrans=langit.current()); ++langit)
				{
					fxmessage("String '%s' module '%s' language '%s' translation '%s'\n",
						engit.currentKey().text(), modtrans->modulepath.text(),
						langit.currentKey().text(), (*langtrans)[0].translation->text()
						);
				}
			}
		}
	}
};

static class FXTransInit : public FXProcess_StaticInitBase
{
public:
	FXTransInit() : FXProcess_StaticInitBase("FXTrans")
	{
		FXProcess::int_addStaticInit(this);
	}
	~FXTransInit()
	{
		FXDELETE(me);
		FXProcess::int_removeStaticInit(this);
	}
	bool create(int &argc, char **argv, FXStream &txtout)
	{
		if(!me)
		{
			FXERRHM(me=new FXTrans(argc, argv, txtout));
		}
		return me->p->quitNow;
	}
	void destroy()
	{
		FXDELETE(me);
	}
} mystaticinit;
FXPROCESS_STATICDEP(mystaticinit, "FXPrimaryThread");

class FXTSArgBase
{
protected:
	FXint fieldwidth;
public:
	FXTSArgBase(FXint _fieldwidth) : fieldwidth(_fieldwidth) { }
	virtual ~FXTSArgBase() { }
	virtual void doInsert(FXString &s) const=0;
	virtual FXTSArgBase *copy() const=0;
};
class FXTSArgString : public FXTSArgBase
{
	FXString str;
public:
	FXTSArgString(const FXString &_str, FXint fw) : str(_str), FXTSArgBase(fw) { }
	void doInsert(FXString &s) const { s.arg(str, fieldwidth); }
	FXTSArgString *copy() const { return new FXTSArgString(*this); }
};
class FXTSArgChar : public FXTSArgBase
{
	char ch;
public:
	FXTSArgChar(char c, FXint fw) : ch(c), FXTSArgBase(fw) { }
	void doInsert(FXString &s) const { s.arg(ch, fieldwidth); }
	FXTSArgChar *copy() const { return new FXTSArgChar(*this); }
};
class FXTSArgLong : public FXTSArgBase
{
	FXlong num;
	FXint base;
public:
	FXTSArgLong(FXlong _num, FXint fw, FXint _base) : num(_num), base(_base), FXTSArgBase(fw) { }
	void doInsert(FXString &s) const { s.arg(num, fieldwidth, base); }
	FXTSArgLong *copy() const { return new FXTSArgLong(*this); }
};
class FXTSArgUlong : public FXTSArgBase
{
	FXulong num;
	FXint base;
public:
	FXTSArgUlong(FXulong _num, FXint fw, FXint _base) : num(_num), base(_base), FXTSArgBase(fw) { }
	void doInsert(FXString &s) const { s.arg(num, fieldwidth, base); }
	FXTSArgUlong *copy() const { return new FXTSArgUlong(*this); }
};
class FXTSArgDouble : public FXTSArgBase
{
	double num;
	FXchar fmt;
	int prec;
public:
	FXTSArgDouble(double _num, FXint fw, FXchar _fmt, int _prec) : num(_num), fmt(_fmt), prec(_prec), FXTSArgBase(fw) { }
	void doInsert(FXString &s) const { s.arg(num, fieldwidth, fmt, prec); }
	FXTSArgDouble *copy() const { return new FXTSArgDouble(*this); }
};


struct FXDLLLOCAL FXTransStringPrivate
{
	const char *context, *text, *hint;
	FXTransString::GetLangIdSpec *langidfunc;
	const FXString *langid;
	QPtrVector<FXTSArgBase> args;
	FXString *translation;
	FXTransStringPrivate(const char *_context, const char *_text, const char *_hint, const FXString *_langid)
		: context(_context), text(_text), hint(_hint), langidfunc(0), langid(_langid), args(true), translation(0)
	{
		args.reserve(8);	// Avoid reallocs
	}
	~FXTransStringPrivate() { FXDELETE(translation); }
};

FXTransString::FXTransString(const char *context, const char *text, const char *hint, const FXString *langid) : p(0)
{
	FXERRHM(p=new FXTransStringPrivate(context, text, hint, langid));
}

FXTransString FXTransString::copy() const
{
	FXTransString ret;
	if(!p) return ret;
	FXERRHM(ret.p=new FXTransStringPrivate(p->context, p->text, p->hint, p->langid));
	ret.p->langidfunc=p->langidfunc;
	FXTSArgBase *a;
	for(QPtrVectorIterator<FXTSArgBase> it(p->args); (a=it.current()); ++it)
	{
		FXTSArgBase *c;
		FXERRHM(c=a->copy());
		FXRBOp unc=FXRBNew(c);
		ret.p->args.append(c);
		unc.dismiss();
	}
	if(p->translation)
	{
		FXERRHM(ret.p->translation=new FXString(*p->translation));
	}
	return ret;
}

FXTransString::GetLangIdSpec *FXTransString::langIdFunc() const throw()
{
	return p->langidfunc;
}

void FXTransString::setLangIdFunc(FXTransString::GetLangIdSpec *langidfunc) throw()
{
	if(!(p->langidfunc=langidfunc)) p->langid=0;
}

#ifndef HAVE_MOVECONSTRUCTORS
#ifdef HAVE_CONSTTEMPORARIES
FXTransString::FXTransString(const FXTransString &other) : p(other.p)
{
	FXTransString &o=const_cast<FXTransString &>(other);
#else
FXTransString::FXTransString(FXTransString &o) : p(o.p)
{
#endif
#else
FXTransString::FXTransString(FXTransString &&o) : p(o.p)
{
#endif
	// Assume that since only FXTrans can create that it's always a destructive copy
	o.p=0;
}

FXTransString::~FXTransString()
{ FXEXCEPTIONDESTRUCT1 {
	FXDELETE(p);
} FXEXCEPTIONDESTRUCT2; }

FXTransString &FXTransString::arg(const FXString &str, FXint fieldwidth)
{
	FXTSArgBase *arg;
	FXERRHM(arg=new FXTSArgString(str, fieldwidth));
	FXRBOp unnew=FXRBNew(arg);
	p->args.append(arg);
	unnew.dismiss();
	return *this;
}
FXTransString &FXTransString::arg(char c, FXint fieldwidth)
{
	FXTSArgBase *arg;
	FXERRHM(arg=new FXTSArgChar(c, fieldwidth));
	FXRBOp unnew=FXRBNew(arg);
	p->args.append(arg);
	unnew.dismiss();
	return *this;
}
FXTransString &FXTransString::arg(FXlong num,  FXint fieldwidth, FXint base)
{
	FXTSArgBase *arg;
	FXERRHM(arg=new FXTSArgLong(num, fieldwidth, base));
	FXRBOp unnew=FXRBNew(arg);
	p->args.append(arg);
	unnew.dismiss();
	return *this;
}
FXTransString &FXTransString::arg(FXulong num, FXint fieldwidth, FXint base)
{
	FXTSArgBase *arg;
	FXERRHM(arg=new FXTSArgUlong(num, fieldwidth, base));
	FXRBOp unnew=FXRBNew(arg);
	p->args.append(arg);
	unnew.dismiss();
	return *this;
}
FXTransString &FXTransString::arg(double num, FXint fieldwidth, FXchar fmt, int prec)
{
	FXTSArgBase *arg;
	FXERRHM(arg=new FXTSArgDouble(num, fieldwidth, fmt, prec));
	FXRBOp unnew=FXRBNew(arg);
	p->args.append(arg);
	unnew.dismiss();
	return *this;
}

FXTransString::operator FXString()
{	// Threadsafe when using langid functor
	assert(p);
	if(p->translation) return *p->translation;
	FXString translation;
	const FXString *langid=(p->langidfunc) ? (*p->langidfunc)() : p->langid;
	if(me) me->int_translateString(translation, *this, langid);
	else translation=p->text;
	FXTSArgBase *arg;
	for(QPtrVectorIterator<FXTSArgBase> it(p->args); (arg=it.current()); ++it)
	{
		arg->doInsert(translation);
	}
	if(!p->langidfunc) { FXERRHM(p->translation=new FXString(translation)); }
	return translation;
}

//*****************************************************************************************

struct DataInfo
{
	FXIODevice *dev;
	FXGZipDevice *gzipdev;
	bool amBuffer;
	const FXuchar *buffer;
	FXuint len;
	DataInfo() : dev(0), gzipdev(0), amBuffer(false), buffer(0), len(0) { }
private:
	DataInfo &operator=(const DataInfo &);
public:
#ifndef HAVE_MOVECONSTRUCTORS
#ifdef HAVE_CONSTTEMPORARIES
	DataInfo(const DataInfo &o)
#else
	DataInfo(DataInfo &o)
#endif
#else
	DataInfo(DataInfo &&o)
#endif
		: dev(o.dev), gzipdev(o.gzipdev), amBuffer(o.amBuffer), buffer(o.buffer), len(o.len)
	{
		DataInfo *other=(DataInfo *) &o;
		other->dev=0; other->gzipdev=0; other->amBuffer=false; other->buffer=0; other->len=0;
	}
	~DataInfo()
	{
		if(amBuffer)
		{
			FXBuffer *b=(FXBuffer *) dev;
			b->buffer().resetRawData(buffer, len);
		}
		FXDELETE(gzipdev);
		FXDELETE(dev);
	}
};
static DataInfo getData(FXTransEmbeddedFile &data)
{
	DataInfo di;
	if(data.dllpath.empty()) data.dllpath=FXProcess::dllpath(data.staticaddr, &data.dllstart, &data.dllend);
	FXString transfilepath=FXFile::stripExtension(data.dllpath), leaf;
	if(FXFile::exists((leaf=transfilepath+"Trans.txt.gz")))
	{
		FXERRHM(di.dev=new FXFile(leaf));
	}
	else if(FXFile::exists((leaf=transfilepath+"Trans.txt")))
	{
		FXERRHM(di.dev=new FXFile(leaf));
	}
	else
	{
		FXBuffer *b;
		FXERRHM(di.dev=b=new FXBuffer);
		di.buffer=data.buffer; di.len=data.len;
		b->buffer().setRawData(data.buffer, data.len);
		di.amBuffer=true;
	}
	return di;
}


FXTrans::FXTrans(int &argc, char *argv[], FXStream &txtout) : p(0)
{
	FXERRHM(p=new FXTransPrivate);
	me=this; // Done already, but to be sure

	if(embedded)
	{	// Ok load anything statically initialised
		for(QValueList<FXTransEmbeddedFile>::iterator it=embedded->begin(); it!=embedded->end(); ++it)
		{
			addData(*it);
		}
	}

	refresh();
	for(int argi=0; argi<argc; argi++)
	{
		if(0==strcmp(argv[argi], "-sysinfo"))
		{
			FXString temp=FXTrans::tr("FXTrans", "The current language is '%1' and the current region is '%2'\n").arg(FXTrans::language()).arg(FXTrans::country());
			txtout << temp.text();
			temp=FXTrans::tr("FXTrans", "The following language translations are provided:\n");
			txtout << temp.text();
			ProvidedInfoList provd=provided();
			for(ProvidedInfoList::iterator it=provd.begin(); it!=provd.end(); ++it)
			{
				temp=FXTrans::tr("FXTrans", "Module: %1 Language: %2 Country: %3\n")
					.arg((*it).module).arg((*it).language).arg((*it).country.text());
				txtout << temp.text();
			}
			float full, slotsspread, avrgkeysperslot, spread;
			p->dict.spread(&full, &slotsspread, &avrgkeysperslot, &spread);
			temp=FXTrans::tr("FXTrans", "Translation dictionary contains %1 items (%2% full with %3 keys per slot)\n")
				.arg(p->dict.count()).arg(full).arg(avrgkeysperslot);
			txtout << temp.text();
			temp=FXTrans::tr("FXTrans", "    Hashing efficiency is %1% and overall efficiency is %2%\n")
				.arg(slotsspread).arg(spread);
			txtout << temp.text();
		}
		else if(0==strncmp(argv[argi], "-fxtrans-language=", 18))
		{
			overrideLanguage(argv[argi]+18);
		}
		else if(0==strncmp(argv[argi], "-fxtrans-country=", 17))
		{
			overrideCountry(argv[argi]+17);
		}
		else if(0==strncmp(argv[argi], "-fxtrans-dump", 13))
		{
			bool gzip=(0==strncmp(argv[argi], "-fxtrans-dumpgzip", 17));
			FXuint idx=atoi(argv[argi]+((gzip) ? 17 : 13));
			DataInfo di=getData(*embedded->at(idx));
			di.dev->open(IO_ReadOnly|IO_Translate);
			if(gzip)
			{
				FXGZipDevice gzipdev(txtout.device());
				gzipdev.open(txtout.device()->mode());
				FXStream sgzipdev(&gzipdev);
				sgzipdev << *di.dev;
			}
			else
			{
				txtout << *di.dev;
			}
			p->quitNow=true;
		}
		else if(0==strcmp(argv[argi], "-help"))
		{
			FXString temp;
			temp=FXTrans::tr("FXTrans", "  -fxtrans-language=<ll> : Overrides the language to be used by\n                           the application to an ISO639 code\n");
			txtout << temp.text();
			temp=FXTrans::tr("FXTrans", "  -fxtrans-country=<cc>  : Overrides the country to be used by\n                           the application to an ISO3166 code\n");
			txtout << temp.text();
			temp=FXTrans::tr("FXTrans", "  -fxtrans-dump<n>       : Writes the library's embedded\n                           translation file no <n> to stdout\n");
			txtout << temp.text();
			temp=FXTrans::tr("FXTrans", "  -fxtrans-dumpgzip<n>   : Writes the library's embedded\n                           translation file no <n> to stdout in gzip\n");
			txtout << temp.text();
		}
	}
}

static void literalise(FXString &ret, const FXuchar *buffer, FXuint lineno)
{	// Could use FXString::unescape() here?
	bool doCopy=false;
	for(; *buffer; ++buffer)
	{
		if('\\'==*buffer)
		{	// escape mode
			switch(*++buffer)
			{
			case '"':
				ret.append('"');
				break;
			case 'n':
				ret.append('\n');
				break;
			default:
				fxwarning("Unknown \\ insert '%c' at line %d in translation file\n", *buffer, lineno);
			}
		}
		else if('"'==*buffer)
			doCopy=!doCopy;
		else if(doCopy) ret.append(*buffer);
	}
}
static inline FXString &loseQuotes(FXString &s)
{
	s=s.mid(1, s.length()-2);
	return s;
}
class ParserState
{
public:
	struct IndentState
	{
		FXString srcfile, classname, hint;
		QMemArray<FXString> pars;
	};
private:
	QValueList<IndentState> state;
public:
	IndentState &topState()
	{
		return state.front();
	}
	const FXString &srcfile() const
	{
		for(QValueList<IndentState>::const_iterator it=state.begin(); it!=state.end(); ++it)
		{
			if(!(*it).srcfile.empty()) return (*it).srcfile;
		}
		return (*state.begin()).srcfile;
	}
	const FXString &classname() const
	{
		for(QValueList<IndentState>::const_iterator it=state.begin(); it!=state.end(); ++it)
		{
			if(!(*it).classname.empty()) return (*it).classname;
		}
		return (*state.begin()).classname;
	}
	const FXString &hint() const
	{
		for(QValueList<IndentState>::const_iterator it=state.begin(); it!=state.end(); ++it)
		{
			if(!(*it).hint.empty()) return (*it).hint;
		}
		return (*state.begin()).hint;
	}
	const QMemArray<FXString> &pars() const
	{
		for(QValueList<IndentState>::const_iterator it=state.begin(); it!=state.end(); ++it)
		{
			if(!(*it).pars.isEmpty()) return (*it).pars;
		}
		return (*state.begin()).pars;
	}
	void setIndent(FXuint level)
	{
		while(level<state.size())
		{
			state.pop_front();
		}
		while(level+1>state.size())
		{
			state.push_front(IndentState());
		}
	}
};
static FXuchar *mystrchr(const FXuchar *str, FXuchar c)
{
	bool inText=true;
	for(; *str; str++)
	{
		if('\\'==*str) str++;
		else if('"'==*str) inText=!inText;
		else if(inText && *str==c) return const_cast<FXuchar *>(str);
	}
	return 0;
}
static FXuchar *findColon(const FXuchar *buffer)
{
	FXuchar *colon;
	while((colon=(FXuchar *) mystrchr(buffer, ':')) && colon[1]==':');
	return colon;
}

void FXTrans::addData(FXTransEmbeddedFile &data)
{
	DataInfo di=getData(data);
	FXERRHM(di.gzipdev=new FXGZipDevice(di.dev));
	di.gzipdev->open(IO_ReadOnly|IO_Translate);
	FXIODevice *fh=di.gzipdev;
	FXMtxHold h(p, true);
	QDict<FXTransPrivate::EnglishTrans> &dict=p->dict;
	FXuint lineno=0;
	FXTransPrivate::ModuleTrans *ctrans=0;
	int indent=0;
	ParserState state;
	p->noTransFiles=false;
	for(;;)
	{
		FXuchar buffer[MAXLITERALLEN];
		if(fh->readLine((char *) buffer, sizeof(buffer)))
		{
			lineno++; indent=0;
			FXuchar *end=(FXuchar *) strchr((char *) buffer,0)-1;
			while(10==*end || 13==*end || 32==*end) *end--=0;
			if('"'==buffer[0])
			{	// Got English literal
				FXTransPrivate::EnglishTrans *engtrans;
				FXString key; literalise(key, buffer, lineno);
				if(!(engtrans=dict.find(key)))
				{
					FXERRHM(engtrans=new FXTransPrivate::EnglishTrans);
					FXRBOp del=FXRBNew(engtrans);
					dict.insert(key, engtrans);
					del.dismiss();
				}
				if(!(ctrans=engtrans->find(data.staticaddr)))
				{
					FXERRHM(ctrans=new FXTransPrivate::ModuleTrans(data.staticaddr, data.dllstart, data.dllend, data.dllpath));
					FXRBOp del=FXRBNew(ctrans);
					engtrans->append(ctrans);
					del.dismiss();
				}
			}
			else if(0==strncmp((char *) buffer, "version=", 8))
			{	// Check trans file version
				int v=atoi((char *) buffer+8);
				FXERRH(1==v, "Incompatible translation file version", FXTRANS_BADTRANSFILE, FXERRH_ISDEBUG);
			}
			else if(9==buffer[0])
			{	// Add a translation to the current literal (ctrans)
				FXuval idx=0;
				indent=1;
				while(9==buffer[++idx]) indent++;
				state.setIndent(indent);
				while('\n'!=buffer[idx] && buffer[idx])
				{
					if(0==strncmp((char *) &buffer[idx], "srcfile=", 8))
					{
						idx+=8;
						FXuchar *colon=findColon(&buffer[idx]);
						FXERRH(colon, FXString("Missing colon at line %1").arg(lineno), FXTRANS_BADTRANSFILE, FXERRH_ISDEBUG);
						*colon=0;
						FXString temp=FXString((FXchar *) &buffer[idx]);
						state.topState().srcfile=loseQuotes(temp);
						idx=1+(FXuval)colon-(FXuval)buffer;
					}
					else if(0==strncmp((char *) &buffer[idx], "class=", 6))
					{
						idx+=6;
						FXuchar *colon=findColon(&buffer[idx]);
						FXERRH(colon, FXString("Missing colon at line %1").arg(lineno), FXTRANS_BADTRANSFILE, FXERRH_ISDEBUG);
						*colon=0;
						FXString temp=FXString((FXchar *) &buffer[idx]);
						state.topState().classname=loseQuotes(temp);
						idx=1+(FXuval)colon-(FXuval)buffer;
					}
					else if(0==strncmp((char *) &buffer[idx], "hint=", 5))
					{
						idx+=5;
						FXuchar *colon=findColon(&buffer[idx]);
						FXERRH(colon, FXString("Missing colon at line %1").arg(lineno), FXTRANS_BADTRANSFILE, FXERRH_ISDEBUG);
						*colon=0;
						FXString temp=FXString((FXchar *) &buffer[idx]);
						state.topState().hint=loseQuotes(temp);
						idx=1+(FXuval)colon-(FXuval)buffer;
					}
					else if('%'==buffer[idx])
					{	// A parameter insert
						FXuchar *colon=findColon(&buffer[idx]);
						FXERRH(colon, FXString("Missing colon at line %1").arg(lineno), FXTRANS_BADTRANSFILE, FXERRH_ISDEBUG);
						*colon=0;
						char *equals=strchr((char *) &buffer[idx], '=');
						FXERRH(equals, FXString("Missing equals at line %1").arg(lineno), FXTRANS_BADTRANSFILE, FXERRH_ISDEBUG);
						*equals=0;
						FXuint parno=atoi((char *) &buffer[idx+1]);
						QMemArray<FXString> &pars=state.topState().pars;
						pars.resize(FXMAX(parno+1, pars.size()));
						if('"'==equals[1])
							literalise(pars[parno], (FXuchar *) equals+1, lineno);
						else
							pars[parno]=equals+1;
						idx=1+(FXuval)colon-(FXuval)buffer;
					}
					else
					{	// Probably a translation
						FXuchar *colon=findColon(&buffer[idx]);
						FXERRH(colon, FXString("Missing colon at line %1").arg(lineno), FXTRANS_BADTRANSFILE, FXERRH_ISDEBUG);
						*colon=0;
						FXString langid((FXchar *) &buffer[idx]); langid.trim(); langid.upper();
						FXString trans; literalise(trans, colon+1, lineno);
						if(!langid.length())
						{
							int a=1;
						}
						FXERRH(langid.length(), FXString("Language id cannot be null at line %1").arg(lineno), FXTRANS_BADTRANSFILE, FXERRH_ISDEBUG);

						FXTransPrivate::LangTrans *langtrans;
						if(!(langtrans=ctrans->langs.find(langid)))
						{
							FXERRHM(langtrans=new FXTransPrivate::LangTrans);
							FXRBOp del=FXRBNew(langtrans);
							ctrans->langs.insert(langid, langtrans);
							del.dismiss();
						}
						// Ok convert state.topState() into ctrans held strings
						FXTransPrivate::LangTransItem lti;
						lti.srcfile    =ctrans->getLiteral(ctrans->srcfiles,   state.srcfile());
						lti.classname  =ctrans->getLiteral(ctrans->classnames, state.classname());
						lti.hint       =ctrans->getLiteral(ctrans->hints,      state.hint());
						lti.pars       =state.pars();
						assert(trans!=":");
						if(trans.empty())
							lti.translation=langtrans->at(0)->translation;
						else
							lti.translation=ctrans->getLiteral(ctrans->transstrs, trans);
						FXERRH(lti.translation, FXString("'up' at line %1 must refer to a previously declared higher order literal").arg(lineno), FXTRANS_BADTRANSFILE, FXERRH_ISDEBUG);
						langtrans->append(lti);
						// Consume rest of line
						idx=(FXuval)colon-(FXuval)buffer;
					}
				}
			}
			else
			{
				state.setIndent(indent);
				if(!indent)
				{	// Reset
					ctrans=0;
				}
			}
		}
		else break;
	}
}

void FXTrans::removeData(const FXTransEmbeddedFile &data)
{
	FXMtxHold h(p, true);
	QDict<FXTransPrivate::EnglishTrans> &dict=p->dict;
	FXTransPrivate::EnglishTrans *engtrans;
	for(QDictIterator<FXTransPrivate::EnglishTrans> it(dict); (engtrans=it.current()); ++it)
	{
		FXTransPrivate::ModuleTrans *modtrans=engtrans->find(data.staticaddr);
		if(modtrans)
		{
			engtrans->removeRef(modtrans);
			if(engtrans->isEmpty())
			{	// Remove completely
				p->dict.remove(it.currentKey());
			}
		}
	}
}

void FXTrans::int_translateString(FXString &dest, FXTransString &src, const FXString *langid)
{
	FXMtxHold h(p, false);
	bool gotIt=false;
	if(!p->noTransFiles)
	{
		const FXTransPrivate::EnglishTrans *engtrans=p->dict.find(FXString(src.p->text));
		if(engtrans)
		{
			assert(!engtrans->isEmpty());
			const FXTransPrivate::ModuleTrans *modtrans=engtrans->find(src.p->text);
			if(!modtrans) modtrans=engtrans->getFirst();
			if(modtrans)
			{
				FXString _langid;
				if(!langid) { _langid=FXTrans::language()+"_"+FXTrans::country(); langid=&_langid; }
				const FXTransPrivate::LangTrans *langtrans=modtrans->langs.find(*langid);
				if(!langtrans) langtrans=modtrans->langs.find(FXTrans::language());
				if(langtrans)
				{
					QValueList<FXTransPrivate::LangTransItem>::const_iterator it=langtrans->begin();
					const FXTransPrivate::LangTransItem *best=&(*it);
					int bestscore=0;
					for(++it; it!=langtrans->end(); ++it)
					{
						int score=0;
						//if((*it).srcfile   && *(*it).srcfile==src.p->srcfile) score+=300;
						if((*it).classname && *(*it).classname==src.p->context) score+=200;
						if((*it).hint      && *(*it).hint==src.p->hint) score+=100;
						if(!(*it).pars.isEmpty())
						{
							QPtrVectorIterator<FXTSArgBase> sit(src.p->args);
							FXuint idx=1;
							for(FXTSArgBase *arg; (arg=sit.current()) && idx<(*it).pars.size(); ++sit, ++idx)
							{
								if(!(*it).pars[idx].empty())
								{
									FXString temp("%1");
									arg->doInsert(temp);
									if(temp==(*it).pars[idx]) score+=1;
								}
							}
						}
						if(score>bestscore)
						{
							best=&(*it);
							bestscore=score;
						}
					}
					dest=*best->translation;
					gotIt=true;
				}
			}
		}
		if(!gotIt)
		{
#ifndef DEBUG
			if(FXApp::instance() && FXApp::instance()->isInitialized())
#endif
			if(language()!="EN")
				fxwarning("FXTrans: Translation for literal '%s' not found\n", src.p->text);
			dest=src.p->text;
		}
	}
	else dest=src.p->text;
}

FXTrans::~FXTrans()
{ FXEXCEPTIONDESTRUCT1 {
	if(embedded)
	{
		for(QValueList<FXTransEmbeddedFile>::iterator it=embedded->end(); it!=embedded->begin();)
		{
			--it;
			removeData(*it);
		}
		// We deliberately do NOT delete embedded here (it gets deleted later)
	}
	FXDELETE(p);
	me=0; // Done already, but to be sure
} FXEXCEPTIONDESTRUCT2; }

FXTransString FXTrans::tr(const char *context, const char *text, const char *hint)
{
	return FXTransString(context, text, hint);
}

const FXString &FXTrans::language(LanguageType t)
{
	if(!me->p->overrides.language.empty())
		return me->p->overrides.language;
	else
		return me->p->language[(int) t];
}

const FXString &FXTrans::country(CountryType i)
{
	if(!me->p->overrides.country.empty())
		return me->p->overrides.country;
	else
		return me->p->country[(int) i];
}

void FXTrans::refresh()
{
	FXMtxHold h(me->p);
	FXString iso639, iso3166;
#ifdef WIN32
#ifndef LOCALE_SENGCURRNAME
#define LOCALE_SENGCURRNAME 0x1007
#endif
	// MSVC's CRT setlocale() support is awful. So we use Windows directly
	{
		LCID mylcid=GetUserDefaultLCID();
		TCHAR buffer[256];
		FXERRHWIN(GetLocaleInfo(mylcid, LOCALE_SISO639LANGNAME, buffer, sizeof(buffer)/sizeof(TCHAR)));
		iso639=buffer;
		FXERRHWIN(GetLocaleInfo(mylcid, LOCALE_SISO3166CTRYNAME, buffer, sizeof(buffer)/sizeof(TCHAR)));
		iso3166=buffer;
		// Try to emulate GNU/Linux's "@currency" modifier system
		FXERRHWIN(GetLocaleInfo(mylcid, LOCALE_SINTLSYMBOL, buffer, sizeof(buffer)/sizeof(TCHAR)));
		FXString currency=buffer;
		if(0==compare(currency, "EURO", 3)) iso3166+="@EURO";
	}
#else
	do
	{
		char *locale=setlocale(LC_ALL, NULL);
		char *lang=strstr(locale, "LANG=");
		if(!lang) lang=strstr(locale, "LC_MESSAGES=");
		if(!lang) lang=strstr(locale, "LC_CTYPE=");
		if(!lang)
		{	// Try the LANG environment variable
			lang=getenv("LANG");
			if(!lang) break;
		}
		else
		{
			lang=strchr(lang, '=')+1;
			if('"'==*lang) lang++;
		}
		while(*lang!='_')
		{
			iso639.append(*lang);
			lang++;
		}
		lang++;
		while(isalpha(*lang) || '@'==*lang)
		{
			iso3166.append(*lang);
			lang++;
		}
		/*if('@'==*lang)
		{
			while(*lang) modifier.append(*lang++);
		}*/
	} while(0);
#endif
	if(iso639.length()!=2)
	{
		fxwarning("FXTrans: setlocale() did not return ISO639 language id - setting to 'en' instead of '%s'\n", iso639.text());
		iso639="en";
	}
	if(iso3166.length()!=2)
	{
		fxwarning("FXTrans: setlocale() did not return ISO3166 country id - setting to 'eu' instead of '%s'\n", iso3166.text());
		iso3166="eu";
	}
	me->p->language[0]=iso639.upper();
	me->p->country[0]=iso3166.upper();
	// TODO: Generate English and local full names of language and country
}

void FXTrans::overrideLanguage(const FXString &iso639)
{
	FXMtxHold h(me->p);
	me->p->overrides.language=iso639;
	if(me->p->overrides.language.empty())
		refresh();
	else me->p->overrides.language.upper();
}

void FXTrans::overrideCountry(const FXString &iso3166)
{
	FXMtxHold h(me->p);
	me->p->overrides.country=iso3166;
	if(me->p->overrides.country.empty())
		refresh();
	else me->p->overrides.country.upper();
}

static bool operator<(const FXTrans::ProvidedInfo &a, const FXTrans::ProvidedInfo &b)
{
	if(a.module<b.module) return true;
	else if(a.language<b.language) return true;
	else if(a.country<b.country) return true;
	return false;
}
static void addProvidedInfo(QValueList<FXTrans::ProvidedInfo> &ret, FXTrans::ProvidedInfo &newpi)
{
	QValueList<FXTrans::ProvidedInfo>::const_iterator it=ret.begin();
	for(; it!=ret.end(); ++it)
	{
		const FXTrans::ProvidedInfo &pi=*it;
		if(pi.module==newpi.module && pi.language==newpi.language && pi.country==newpi.country)
			break;
	}
	if(it==ret.end()) ret.append(newpi);
}
QValueList<FXTrans::ProvidedInfo> FXTrans::provided()
{
	QValueList<ProvidedInfo> ret;
	FXMtxHold h(me->p, false);
	if(!me->p->noTransFiles)
	{
		QDictIterator<FXTransPrivate::EnglishTrans> engit(me->p->dict);
		for(FXTransPrivate::EnglishTrans *engtrans; (engtrans=engit.current()); ++engit)
		{
			QPtrVectorIterator<FXTransPrivate::ModuleTrans> modit(*engtrans);
			for(FXTransPrivate::ModuleTrans *modtrans; (modtrans=modit.current()); ++modit)
			{
				ProvidedInfo english(modtrans->modulepath, "EN", "");
				addProvidedInfo(ret, english);
				QDictIterator<FXTransPrivate::LangTrans> langit(modtrans->langs);
				for(FXTransPrivate::LangTrans *langtrans; (langtrans=langit.current()); ++langit)
				{
					ProvidedInfo newpi(modtrans->modulepath, langit.currentKey().left(2), langit.currentKey().mid(3,2));
					addProvidedInfo(ret, newpi);
				}
			}
		}
	}
	ret.sort();
	return ret;
}

void FXTrans::int_registerEmbeddedTranslationFile(const char *buffer, FXuint len, void *staticaddr)
{
	if(!embedded)
	{
		FXERRHM(embedded=new QValueList<FXTransEmbeddedFile>);
	}
	FXTransEmbeddedFile ef((const FXuchar *) buffer, len, staticaddr);
	embedded->append(ef);
	if(me) me->addData(ef);
}

void FXTrans::int_deregisterEmbeddedTranslationFile(const char *buffer, FXuint len, void *staticaddr)
{
	FXTransEmbeddedFile ef((const FXuchar *) buffer, len, staticaddr);
	if(me) me->removeData(ef);
	embedded->remove(ef);
	if(embedded->empty())
	{
		FXDELETE(embedded);
	}
}

} // namespace
