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

#include "FXThread.h"		// May undefine USE_WINAPI and USE_POSIX
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
#include "FXFileInfo.h"
#include "FXTrans.h"
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

struct FXFSMon : public FXRWMutex
{
	struct Watcher : public FXThread
	{
		struct Path
		{
			FXAutoPtr<FXDir> pathdir;
#ifdef USE_WINAPI
			HANDLE h;
#endif
#ifdef USE_POSIX
			FAMRequest h;
#endif
			struct Handler
			{
				FXFSMonitor::ChangeHandler handler;
				Generic::BoundFunctorV *volatile callv;
				Handler(FXFSMonitor::ChangeHandler _handler) : handler(_handler), callv(0) { }
				~Handler()
				{
					FXThreadPool::CancelledState state;
					assert(!callv || dynamic_cast<void *>(callv));
					while(FXThreadPool::WasRunning==(state=FXProcess::threadPool().cancel(callv)));
					FXDELETE(callv);
				}
				void invoke(FXFSMonitor::Change change, const FXFileInfo &oldfi, const FXFileInfo &newfi);
			private:
				Handler(const Handler &);
				Handler &operator=(const Handler &);
			};
			QPtrVector<Handler> handlers;
			Path(const FXString &_path)
				: handlers(true)
#ifdef USE_WINAPI
				, h(0)
#endif
			{
				FXERRHM(pathdir=new FXDir(_path, "*", FXDir::Unsorted, FXDir::All|FXDir::Hidden));
				pathdir->entryInfoList();
			}
			~Path();
			void callHandlers();
			struct Change
			{
				FXFSMonitor::Change change;
				const FXFileInfo *oldfi, *newfi;
				Change(const FXFileInfo *_oldfi, const FXFileInfo *_newfi) : oldfi(_oldfi), newfi(_newfi) { }
				bool operator==(const Change &o) const { return oldfi==o.oldfi && newfi==o.newfi; }
			};
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

FXFSMon::FXFSMon() : watchers(true), FXRWMutex()
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

FXFSMon::Watcher::Watcher() : FXThread("Filing system monitor", false, 128*1024, FXThread::InProcess), paths(13, true, true)
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
	hlist[0]=FXThread::int_cancelWaiterHandle();
	hlist[1]=latch;
	for(;;)
	{
		FXMtxHold h(fxfsmon, false);
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
				FXMtxHold h(fxfsmon, false);
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

void FXFSMon::Watcher::Path::Handler::invoke(FXFSMonitor::Change change, const FXFileInfo &oldfi, const FXFileInfo &newfi)
{
	//fxmessage("FXFSMonitor dispatch %p\n", callv);
	fxfsmon->lock();	// necessary as dispatch may execute before callv gets written
	callv=0;
	fxfsmon->unlock();
	handler(change, oldfi, newfi);
}


static const FXFileInfo *findFIByName(const QFileInfoList *list, const FXString &name)
{
	for(QFileInfoList::const_iterator it=list->begin(); it!=list->end(); ++it)
	{
		const FXFileInfo &fi=*it;
		// Need a case sensitive compare
		if(fi.fileName()==name) return &fi;
	}
	return 0;
}
void FXFSMon::Watcher::Path::callHandlers()
{	// Read lock is held on entry
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
			const FXFileInfo *a=0, *b=0;
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
	for(QValueList<Change>::iterator it=changes.begin(); it!=changes.end(); ++it)
	{
		Change &ch=*it;
		if(ch.oldfi && ch.newfi) continue;
		if(ch.change.renamed) continue;
		ch.change.created=(!ch.oldfi && ch.newfi);
		ch.change.deleted=(ch.oldfi && !ch.newfi);
	}
	// Dispatch
	for(QValueList<Change>::iterator it=changes.begin(); it!=changes.end(); ++it)
	{
		Change &ch=*it;
		Watcher::Path::Handler *handler;
		if(*((FXuint *)&ch.change))
		{	// Don't bother if it's just accessed time change (non portable anyway)
			const FXFileInfo &oldfi=ch.oldfi ? *ch.oldfi : FXFileInfo();
			const FXFileInfo &newfi=ch.newfi ? *ch.newfi : FXFileInfo();
#ifdef DEBUG
			//fxmessage("File %s had changes 0x%x old=%s, new=%s\n", oldfi.filePath().text(), *(int *) &ch.change,
			//	oldfi.createdAsString().text(), newfi.createdAsString().text());
#endif
			for(QPtrVectorIterator<Watcher::Path::Handler> it(handlers); (handler=it.current()); ++it)
			{
				if(!handler->callv)
				{
					handler->callv=FXProcess::threadPool().dispatch(new Generic::BoundFunctor<FXFSMonitor::ChangeHandlerPars>(Generic::Functor<FXFSMonitor::ChangeHandlerPars>(*handler, &Watcher::Path::Handler::invoke), ch.change, oldfi, newfi));
#ifdef DEBUG
					fxmessage("Dispatched FXFSMonitor change %p\n", handler->callv);
#endif
				}
			}
		}
	}
	pathdir=newpathdir;
}

void FXFSMon::add(const FXString &path, FXFSMonitor::ChangeHandler handler)
{
	FXMtxHold lh(this, true);
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
		FXERRHM(p=new Watcher::Path(path));
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
	FXERRHM(h=new Watcher::Path::Handler(handler));
	FXRBOp unh=FXRBNew(h);
	p->handlers.append(h);
	unh.dismiss();
}

bool FXFSMon::remove(const FXString &path, FXFSMonitor::ChangeHandler handler)
{
	FXMtxHold hl(this, true);
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
	FXERRH(FXFile::exists(path), FXTrans::tr("FXFSMonitor", "Path not found"), FXFSMONITOR_PATHNOTFOUND, 0);
	if(fxfsmon->nofam)
	{	// Try starting it again
		FXERRHFAM(FAMOpen(&fxfsmon->fc));
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
