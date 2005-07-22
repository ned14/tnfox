/********************************************************************************
*                                                                               *
*                            Filing system monitor                              *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2003 by Niall Douglas.   All Rights Reserved.            *
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

#include "QThread.h"		// May undefine USE_WINAPI and USE_POSIX
#ifndef USE_POSIX
#define USE_WINAPI
#include "WindowsGubbins.h"
#else
#include <unistd.h>
#include <fam.h>
#define FXERRHFAM(ret) if(ret<0) { FXERRG(FXString("FAM Error: %1 (code %2)").arg(FXString(FamErrlist[FAMErrno])).arg(FAMErrno), FXEXCEPTION_OSSPECIFIC, 0); }
#endif

#include "FXFSMonitor.h"
#include "FXString.h"
#include "FXProcess.h"
#include "FXRollback.h"
#include "FXFile.h"
#include "FXDir.h"
#include "QFileInfo.h"
#include "QTrans.h"
#include "FXErrCodes.h"
#include <qptrlist.h>
#include <qdict.h>
#include <qptrvector.h>
#include <assert.h>
#include "FXMemDbg.h"
#if defined(DEBUG) && defined(FXMEMDBG_H)
static const char *_fxmemdbg_current_file_ = __FILE__;
#endif

namespace FX {

struct FXFSMon : public QMutex
{
	struct Watcher : public QThread
	{
		struct Path
		{
			Watcher *parent;
			FXAutoPtr<FXDir> pathdir;
#ifdef USE_WINAPI
			HANDLE h;
#endif
#ifdef USE_POSIX
			FAMRequest h;
#endif
			struct Change
			{
				FXFSMonitor::Change change;
				const QFileInfo *oldfi, *newfi;
				FXuint myoldfi : 1;
				FXuint mynewfi : 1;
				Change(const QFileInfo *_oldfi, const QFileInfo *_newfi) : oldfi(_oldfi), newfi(_newfi), myoldfi(0), mynewfi(0) { }
				~Change()
				{
					if(myoldfi) { FXDELETE(oldfi); }
					if(mynewfi) { FXDELETE(newfi); }
				}
				bool operator==(const Change &o) const { return oldfi==o.oldfi && newfi==o.newfi; }
				void make_fis()
				{
					if(oldfi)
					{
						FXERRHM(oldfi=new QFileInfo(*oldfi));
						myoldfi=true;
					}
					if(newfi)
					{
						FXERRHM(newfi=new QFileInfo(*newfi));
						mynewfi=true;
					}
				}
				void reset_fis()
				{
					oldfi=0; myoldfi=false;
					newfi=0; mynewfi=false;
				}
			};
			struct Handler
			{
				Path *parent;
				FXFSMonitor::ChangeHandler handler;
				QPtrList<void> callvs;
				Handler(Path *_parent, FXFSMonitor::ChangeHandler _handler) : parent(_parent), handler(_handler) { }
				~Handler();
				void invoke(const QValueList<Change> &changes, QThreadPool::handle callv);
			private:
				Handler(const Handler &);
				Handler &operator=(const Handler &);
			};
			QPtrVector<Handler> handlers;
			Path(Watcher *_parent, const FXString &_path)
				: parent(_parent), handlers(true)
#ifdef USE_WINAPI
				, h(0)
#endif
			{
				FXERRHM(pathdir=new FXDir(_path, "*", FXDir::Unsorted, FXDir::All|FXDir::Hidden));
				pathdir->entryInfoList();
			}
			~Path();
			void resetChanges(QValueList<Change> *changes)
			{
				for(QValueList<Change>::iterator it=changes->begin(); it!=changes->end(); ++it)
					it->reset_fis();
			}
			void callHandlers();
		};
		QDict<Path> paths;
#ifdef USE_WINAPI
		HANDLE latch;
#endif
		Watcher();
		~Watcher();
		void run();
		void *cleanup();
	};
#ifdef USE_POSIX
	bool nofam, fambroken;
	FAMConnection fc;
#endif
	QPtrList<Watcher> watchers;
	FXFSMon();
	~FXFSMon();
	void add(const FXString &path, FXFSMonitor::ChangeHandler handler);
	bool remove(const FXString &path, FXFSMonitor::ChangeHandler handler);
};
static FXProcess_StaticInit<FXFSMon> fxfsmon("FXFSMonitor");

FXFSMon::FXFSMon() : watchers(true)
{
#ifdef USE_POSIX
	nofam=true;
	fambroken=false;
	int ret=FAMOpen(&fc);
	if(ret)
	{
		fxwarning("Disabling FXFSMonitor due to FAMOpen() throwing error %d (%s) syserror: %s\nIs the famd daemon running?\n", FAMErrno, FamErrlist[FAMErrno], strerror(errno));
	}
	else nofam=false;
	//FXERRHOS(ret);
#endif
}

FXFSMon::~FXFSMon()
{ FXEXCEPTIONDESTRUCT1 {
	Watcher *w;
	for(QPtrListIterator<Watcher> it(watchers); (w=it.current()); ++it)
	{
		w->requestTermination();
	}
	for(QPtrListIterator<Watcher> it(watchers); (w=it.current()); ++it)
	{
		w->wait();
	}
	watchers.clear();
#ifdef USE_POSIX
	if(!nofam)
	{
		FXERRHFAM(FAMClose(&fc));
	}
#endif
} FXEXCEPTIONDESTRUCT2; }

FXFSMon::Watcher::Watcher() : QThread("Filing system monitor", false, 128*1024, QThread::InProcess), paths(13, true, true)
#ifdef USE_WINAPI
	, latch(0)
#endif
{
#ifdef USE_WINAPI
	FXERRHWIN(latch=CreateEvent(NULL, FALSE, FALSE, NULL));
#endif
}

FXFSMon::Watcher::~Watcher()
{
	requestTermination();
	wait();
#ifdef USE_WINAPI
	if(latch)
	{
		FXERRHWIN(CloseHandle(latch));
		latch=0;
	}
#endif
}

#ifdef USE_WINAPI
void FXFSMon::Watcher::run()
{
	HANDLE hlist[MAXIMUM_WAIT_OBJECTS];
	hlist[0]=QThread::int_cancelWaiterHandle();
	hlist[1]=latch;
	for(;;)
	{
		FXMtxHold h(fxfsmon);
		Path *p;
		int idx=2;
		for(QDictIterator<Path> it(paths); (p=it.current()); ++it)
		{
			hlist[idx++]=p->h;
		}
		h.unlock();
		DWORD ret=WaitForMultipleObjects(idx, hlist, FALSE, INFINITE);
		if(ret<WAIT_OBJECT_0 || ret>=WAIT_OBJECT_0+idx) { FXERRHWIN(ret); }
		checkForTerminate();
		if(WAIT_OBJECT_0+1==ret) continue;
		ret-=WAIT_OBJECT_0;
		FindNextChangeNotification(hlist[ret]);
		ret-=2;
		h.relock();
		for(QDictIterator<Path> it(paths); (p=it.current()); ++it)
		{
			if(!ret) break;
			--ret;
		}
		if(p)
			p->callHandlers();
	}
}
#endif
#ifdef USE_POSIX
void FXFSMon::Watcher::run()
{
	int ret;
	FAMEvent fe;
	for(;;)
	{
		FXERRH_TRY
		{
			while((ret=FAMNextEvent(&fxfsmon->fc, &fe)))
			{
				FXERRHFAM(ret);
				if(FAMStartExecuting==fe.code || FAMStopExecuting==fe.code
					|| FAMAcknowledge==fe.code) continue;
				FXMtxHold h(fxfsmon);
				Path *p;
				for(QDictIterator<Path> it(paths); (p=it.current()); ++it)
				{
					if(p->h.reqnum==fe.fr.reqnum) break;
				}
				if(p)
					p->callHandlers();
			}
		}
		FXERRH_CATCH(FXException &)
		{
			if(3==FAMErrno)
			{	// libgamin on Fedora Core 3 seems totally broken so exit the thread :(
				fxfsmon->fambroken=true;
				fxwarning("WARNING: FAMNextEvent() returned Connection Failed, assuming broken FAM implementation and exiting thread - FXFSMonitor will no longer work for the remainder of this program's execution\n");
				return;
			}
		}
		FXERRH_ENDTRY
	}
}
#endif
void *FXFSMon::Watcher::cleanup()
{
	return 0;
}

FXFSMon::Watcher::Path::~Path()
{
#ifdef USE_WINAPI
	if(h)
	{
		FXERRHWIN(FindCloseChangeNotification(h));
		h=0;
	}
#endif
#ifdef USE_POSIX
	if(!fxfsmon->fambroken)
	{
		FXERRHFAM(FAMCancelMonitor(&fxfsmon->fc, &h));
	}
#endif
}

FXFSMon::Watcher::Path::Handler::~Handler()
{
	FXMtxHold h(fxfsmon);
	while(!callvs.isEmpty())
	{
		QThreadPool::CancelledState state;
		while(QThreadPool::WasRunning==(state=FXProcess::threadPool().cancel(callvs.getFirst())));
		callvs.removeFirst();
	}
}

void FXFSMon::Watcher::Path::Handler::invoke(const QValueList<Change> &changes, QThreadPool::handle callv)
{
	//fxmessage("FXFSMonitor dispatch %p\n", callv);
	FXMtxHold h(fxfsmon);
	for(QValueList<Change>::const_iterator it=changes.begin(); it!=changes.end(); ++it)
	{
		const Change &ch=*it;
		const QFileInfo &oldfi=ch.oldfi ? *ch.oldfi : QFileInfo();
		const QFileInfo &newfi=ch.newfi ? *ch.newfi : QFileInfo();
#ifdef DEBUG
		{
			FXString file(oldfi.filePath()), chs;
			if(ch.change.modified) chs.append("modified ");
			if(ch.change.created)  { chs.append("created "); file=newfi.filePath(); }
			if(ch.change.deleted)  chs.append("deleted ");
			if(ch.change.renamed)  chs.append("renamed (to "+newfi.filePath()+") ");
			if(ch.change.attrib)   chs.append("attrib ");
			if(ch.change.security) chs.append("security ");
			fxmessage("FXFSMonitor: File %s had changes: %s\n", file.text(), chs.text());
		}
#endif
		callvs.removeRef(callv);
		handler(ch.change, oldfi, newfi);
	}
}


static const QFileInfo *findFIByName(const QFileInfoList *list, const FXString &name)
{
	for(QFileInfoList::const_iterator it=list->begin(); it!=list->end(); ++it)
	{
		const QFileInfo &fi=*it;
		// Need a case sensitive compare
		if(fi.fileName()==name) return &fi;
	}
	return 0;
}
void FXFSMon::Watcher::Path::callHandlers()
{	// Lock is held on entry
	FXAutoPtr<FXDir> newpathdir;
	FXERRHM(newpathdir=new FXDir(pathdir->path(), "*", FXDir::Unsorted, FXDir::All|FXDir::Hidden));
	newpathdir->entryInfoList();
	QStringList rawchanges=FXDir::extractChanges(*pathdir, *newpathdir);
	QValueList<Change> changes;
	for(QStringList::iterator it=rawchanges.begin(); it!=rawchanges.end(); ++it)
	{
		const FXString &name=*it;
		Change ch(findFIByName(pathdir->entryInfoList(), name), findFIByName(newpathdir->entryInfoList(), name));
		if(!ch.oldfi && !ch.newfi)
		{	// Change vanished
			continue;
		}
		else if(ch.oldfi && ch.newfi)
		{	// Same file name
			if(ch.oldfi->created()!=ch.newfi->created())
			{	// File was deleted and recreated, so split into two entries
				Change ch2(ch);
				ch.oldfi=0; ch2.newfi=0;
				changes.append(ch2);
			}
			else
			{
				ch.change.modified=(ch.oldfi->lastModified()!=ch.newfi->lastModified()
					|| ch.oldfi->size()			!=ch.newfi->size());
				ch.change.attrib=(ch.oldfi->isReadable()!=ch.newfi->isReadable()
					|| ch.oldfi->isWriteable()	!=ch.newfi->isWriteable()
					|| ch.oldfi->isExecutable()	!=ch.newfi->isExecutable()
					|| ch.oldfi->isHidden()		!=ch.newfi->isHidden());
				ch.change.security=(ch.oldfi->permissions()!=ch.newfi->permissions());
			}
		}
		changes.append(ch);
	}
	// Try to detect renames
	for(QValueList<Change>::iterator it1=changes.begin(); it1!=changes.end(); ++it1)
	{
		Change &ch1=*it1;
		if(ch1.oldfi && ch1.newfi) continue;
		if(ch1.change.renamed) continue;
		bool disable=false;
		Change *candidate=0, *solution=0;
		for(QValueList<Change>::iterator it2=changes.begin(); it2!=changes.end(); ++it2)
		{
			if(it1==it2) continue;
			Change &ch2=*it2;
			if(ch2.oldfi && ch2.newfi) continue;
			if(ch2.change.renamed) continue;
			const QFileInfo *a=0, *b=0;
			if(ch1.oldfi && ch2.newfi) { a=ch1.oldfi; b=ch2.newfi; }
			else if(ch1.newfi && ch2.oldfi) { a=ch2.oldfi; b=ch1.newfi; }
			else continue;
			if(a->size()==b->size() && a->created()==b->created()
				&& a->lastModified()==b->lastModified()
				&& a->lastRead()==b->lastRead())
			{
				if(candidate) disable=true;
				else
				{
					candidate=&ch1; solution=&ch2;
				}
			}
		}
		if(candidate && !disable)
		{
			if(candidate->newfi && solution->oldfi)
			{
				Change *temp=candidate;
				candidate=solution;
				solution=temp;
			}
			solution->oldfi=candidate->oldfi;
			solution->change.renamed=true;
			if(candidate==&(*it1)) --it1;
			changes.remove(Change(*candidate));
		}
	}
	// Mark off created/deleted
	static FXulong eventcounter=0;
	for(QValueList<Change>::iterator it=changes.begin(); it!=changes.end(); ++it)
	{
		Change &ch=*it;
		ch.change.eventNo=++eventcounter;
		if(ch.oldfi && ch.newfi) continue;
		if(ch.change.renamed) continue;
		ch.change.created=(!ch.oldfi && ch.newfi);
		ch.change.deleted=(ch.oldfi && !ch.newfi);
	}
	// Remove any which don't have something set
	for(QValueList<Change>::iterator it=changes.begin(); it!=changes.end();)
	{
		Change &ch=*it;
		if(!ch.change)
			it=changes.erase(it);
		else
			++it;
	}
	FXRBOp resetchanges=FXRBObj(*this, &FXFSMon::Watcher::Path::resetChanges, &changes);
	// Dispatch
	Watcher::Path::Handler *handler;
	for(QPtrVectorIterator<Watcher::Path::Handler> it(handlers); (handler=it.current()); ++it)
	{
		typedef Generic::TL::create<void, QValueList<Change>, QThreadPool::handle>::value Spec;
		Generic::BoundFunctor<Spec> *functor;
		// Detach changes per dispatch
		for(QValueList<Change>::iterator it=changes.begin(); it!=changes.end(); ++it)
			it->make_fis();
		QThreadPool::handle callv=FXProcess::threadPool().dispatch((functor=new Generic::BoundFunctor<Spec>(Generic::Functor<Spec>(*handler, &Watcher::Path::Handler::invoke), changes, 0)));
		handler->callvs.append(callv);
		// Poke in the callv
		Generic::TL::instance<1>(functor->parameters()).value=callv;
	}
	pathdir=newpathdir;
}

void FXFSMon::add(const FXString &path, FXFSMonitor::ChangeHandler handler)
{
	FXMtxHold lh(fxfsmon);
	Watcher *w;
	for(QPtrListIterator<Watcher> it(watchers); (w=it.current()); ++it)
	{
#ifdef USE_WINAPI
		if(w->paths.count()<MAXIMUM_WAIT_OBJECTS-2) break;
#endif
#ifdef USE_POSIX
		break;
#endif
	}
	if(!w)
	{
		FXERRHM(w=new Watcher);
		FXRBOp unnew=FXRBNew(w);
		w->start();
		watchers.append(w);
		unnew.dismiss();
	}
	Watcher::Path *p=w->paths.find(path);
	if(!p)
	{
		FXERRHM(p=new Watcher::Path(w, path));
		FXRBOp unnew=FXRBNew(p);
#ifdef USE_WINAPI
		HANDLE h;
		FXERRHWIN(INVALID_HANDLE_VALUE!=(h=FindFirstChangeNotification(path.text(), FALSE, FILE_NOTIFY_CHANGE_FILE_NAME
			|FILE_NOTIFY_CHANGE_DIR_NAME|FILE_NOTIFY_CHANGE_ATTRIBUTES|FILE_NOTIFY_CHANGE_SIZE
			/*|FILE_NOTIFY_CHANGE_LAST_WRITE*/|FILE_NOTIFY_CHANGE_SECURITY)));
		p->h=h;
		FXERRHWIN(SetEvent(w->latch));
#endif
#ifdef USE_POSIX
		if(!fambroken)
		{
			FXERRHFAM(FAMMonitorDirectory(&fc, path.text(), &p->h, 0));
		}
#endif
		w->paths.insert(path, p);
		unnew.dismiss();
	}
	Watcher::Path::Handler *h;
	FXERRHM(h=new Watcher::Path::Handler(p, handler));
	FXRBOp unh=FXRBNew(h);
	p->handlers.append(h);
	unh.dismiss();
}

bool FXFSMon::remove(const FXString &path, FXFSMonitor::ChangeHandler handler)
{
	FXMtxHold hl(fxfsmon);
	Watcher *w;
	for(QPtrListIterator<Watcher> it(watchers); (w=it.current()); ++it)
	{
		Watcher::Path *p=w->paths.find(path);
		if(!p) continue;
		Watcher::Path::Handler *h;
		for(QPtrVectorIterator<Watcher::Path::Handler> it2(p->handlers); (h=it2.current()); ++it2)
		{
			if(h->handler==handler)
			{
				p->handlers.takeByIter(it2);
				hl.unlock();
				FXDELETE(h);
				hl.relock();
				h=0;
				if(p->handlers.isEmpty())
				{
#ifdef USE_WINAPI
					FXERRHWIN(SetEvent(w->latch));
#endif
					w->paths.remove(path);
					p=0;
					if(w->paths.isEmpty() && watchers.count()>1)
					{
						watchers.removeByIter(it);
						w=0;
					}
				}
				return true;
			}
		}
	}
	return false;
}

void FXFSMonitor::add(const FXString &_path, FXFSMonitor::ChangeHandler handler)
{
	FXString path=FXFile::absolute(_path);
#ifdef USE_POSIX
	FXERRH(FXFile::exists(path), QTrans::tr("FXFSMonitor", "Path not found"), FXFSMONITOR_PATHNOTFOUND, 0);
	if(fxfsmon->nofam)
	{	// Try starting it again
		if(FAMOpen(&fxfsmon->fc)<0) return;
		fxfsmon->nofam=false;
	}
#endif
	fxfsmon->add(path, handler);
}

bool FXFSMonitor::remove(const FXString &path, FXFSMonitor::ChangeHandler handler)
{
#ifdef USE_POSIX
	if(fxfsmon->nofam) return false;
#endif
	return fxfsmon->remove(path, handler);
}

}
