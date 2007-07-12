/********************************************************************************
*                                                                               *
*                        Information about a directory                          *
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

#include "xincs.h"
#include "QDir.h"
#include "FXString.h"
#include "QFileInfo.h"
#include "FXException.h"
#include "FXRollback.h"
#include "FXPath.h"
#include "FXDir.h"
#include "QTrans.h"
#include "QFile.h"
#include "FXStat.h"
#include "FXSystem.h"
#include "FXWinLinks.h"
#include "FXErrCodes.h"
#include "qmemarray.h"
#ifndef USE_POSIX
#define USE_WINAPI
#include "WindowsGubbins.h"
#define THROWPOSTFOX(filename) FXERRHWINFN(0, filename);
#endif
#ifdef USE_POSIX
#define THROWPOSTFOX(filename) FXERRHOSFN(-1, filename);
#endif
#include "FXMemDbg.h"
#if defined(DEBUG) && defined(FXMEMDBG_H)
static const char *_fxmemdbg_current_file_ = __FILE__;
#endif


namespace FX {

#define TESTABS(path) \
	FXERRH(acceptAbs || FXFile::isAbsolute(path)==0, QTrans::tr("QDir", "This is an absolute path"), QDIR_ISABSOLUTEPATH, 0)

struct FXDLLLOCAL FXDirPrivate
{
	FXString path, regex;
	int sortBy, filter;
	bool dirty, allDirs;
	QStringList leafs;
	mutable QFileInfoList *leafinfos;
	FXDirPrivate(const FXString &_path, const FXString &_regex, int _sortBy, int _filter)
		: path(_path), regex(_regex), sortBy(_sortBy), filter(_filter), dirty(true), allDirs(false), leafinfos(0) { }
	FXDirPrivate(const FXDirPrivate &o) : path(o.path), regex(o.regex), sortBy(o.sortBy), filter(o.filter),
		dirty(o.dirty), allDirs(o.allDirs), leafs(o.leafs), leafinfos(0)
	{
		if(o.leafinfos)
		{
			FXERRHM(leafinfos=new QFileInfoList(*o.leafinfos));
		}
	}
	~FXDirPrivate()
	{
		FXDELETE(leafinfos);
	}
	void doLeafInfos();
	void read();
	struct ExpensiveSortRef
	{
		int idx;	// Mostly for debugging purposes
		FXDirPrivate *parent;
		QStringList::iterator sit;
		QFileInfoList::iterator fit;
		ExpensiveSortRef(int _idx, FXDirPrivate *p, const QStringList::iterator &_sit, const QFileInfoList::iterator &_fit) : idx(_idx), parent(p), sit(_sit), fit(_fit) { }
		bool operator<(const ExpensiveSortRef &o) const
		{
			if((parent->sortBy & QDir::SortByMask)==QDir::Time) return (*fit).lastModified()<(*o.fit).lastModified();
			if((parent->sortBy & QDir::SortByMask)==QDir::Size) return (*fit).size()<(*o.fit).size();
			return false;
		}
		bool operator==(const ExpensiveSortRef &o) const
		{
			if((parent->sortBy & QDir::SortByMask)==QDir::Time) return (*fit).lastModified()==(*o.fit).lastModified();
			if((parent->sortBy & QDir::SortByMask)==QDir::Size) return (*fit).size()==(*o.fit).size();
			return false;
		}
	};
	template<class type> struct ExpensiveMover : private Pol::itMove<type>
	{
		void move(QValueList<type> &list, typename QValueList<type>::iterator To, typename QValueList<type>::iterator &From) const
		{
			ExpensiveSortRef &to=*To, &from=*From;
			FXDirPrivate *parent=from.parent;
			// Slightly dodgy this ...
			if(to.parent!=parent)
			{
				to=ExpensiveSortRef(-1, parent, parent->leafs.end(), parent->leafinfos->end());
			}
			//fxmessage("B: %s\n", parent->leafs.join(",").text());
			Pol::itMove<type>::move(parent->leafs, to.sit, from.sit);
			//fxmessage("A: %s\n", parent->leafs.join(",").text());
			Pol::itMove<type>::move(*parent->leafinfos, to.fit, from.fit);
			Pol::itMove<type>::move(list, To, From);
		}
	};
	template<class type> struct ExpensiveSwap : private Pol::itSwap<type>
	{
		template<class I> void ptrSwap(I &A, I &B) const
		{
			I t=A; A=B; B=t;
		}
		void swap(QValueList<type> &list, typename QValueList<type>::iterator &A, typename QValueList<type>::iterator &B) const
		{
			ExpensiveSortRef &a=*A, &b=*B;
			FXDirPrivate *parent=a.parent;
			//fxmessage("B: %s\n", parent->leafs.join(",").text());
			Pol::itSwap<type>::swap(parent->leafs, a.sit, b.sit); ptrSwap(a.sit, b.sit);
			//fxmessage("A: %s\n", parent->leafs.join(",").text());
			Pol::itSwap<type>::swap(*parent->leafinfos, a.fit, b.fit); ptrSwap(a.fit, b.fit);
			Pol::itSwap<type>::swap(list, A, B);
		}
	};
	template<class type> struct NameCompare
	{
		FXDirPrivate *parent;
		template<class L, class I> bool compare(L &list, I &a, I &b) const
		{
			if(parent->sortBy & QDir::IgnoreCase)
				return comparecase(*a, *b)<0;
			else
				return *a<*b;
		}
	};
	template<class type> struct ExpensiveDirsFirst
	{
		template<class L, class I> bool compare(L &list, I &A, I &B) const
		{
			ExpensiveSortRef &a=*A, &b=*B;
			const QFileInfo &fa=*a.fit, &fb=*b.fit;
			return (fa.isDir() && !fb.isDir());
		}
	};
};

void FXDirPrivate::doLeafInfos()
{
	if(!leafinfos)
	{
		FXERRHM(leafinfos=new QFileInfoList);
		for(QStringList::iterator it=leafs.begin(); it!=leafs.end(); ++it)
		{
			leafinfos->append(QFileInfo(FXPath::join(path, *it)));
		}
	}
}
void FXDirPrivate::read()
{
	if(dirty)
	{
		leafs.clear();
		FXDELETE(leafinfos);
		{
			FXString *list=0;
			FXuint findflags=FXDir::CaseFold|FXDir::NoParent;
			if(!(filter & QDir::Dirs) && !allDirs) findflags|=FXDir::NoDirs;
			if(!(filter & QDir::Files)) findflags|=FXDir::NoFiles;
			if(filter & QDir::Hidden) findflags|=FXDir::HiddenFiles|FXDir::HiddenFiles;
			if(allDirs) findflags|=FXDir::AllDirs;
			if(regex.empty()) findflags|=FXDir::AllFiles|FXDir::AllDirs;
			FXint listcnt=FXDir::listFiles(list, path, regex, findflags);
			FXRBOp unlist=FXRBNewA(list);
			for(FXint n=0; n<listcnt; n++)
			{
				leafs.append(list[n]);
			}
		}
		if(!(filter & (QDir::Readable|QDir::Writeable|QDir::Executable)))
			filter|=(QDir::Readable|QDir::Writeable|QDir::Executable);
		bool a,b=false;
		if((a=(filter & QDir::NoSymLinks)!=0)
			|| (b=(filter & (QDir::Readable|QDir::Writeable|QDir::Executable))!=(QDir::Readable|QDir::Writeable|QDir::Executable)))
		{	// Got to generate the file infos
			doLeafInfos();
			QStringList::iterator sit=leafs.begin();
			QFileInfoList::iterator fit=leafinfos->begin();
			for(; sit!=leafs.end(); ++sit, ++fit)
			{
				QFileInfo &fi=*fit;
				if(allDirs && fi.isDir()) continue;
				if(a)
				{
					if((filter & QDir::NoSymLinks) && fi.isSymLink()) goto remove;
				}
				if(b)
				{
					if((filter & QDir::Readable) && !fi.isReadable()) goto remove;
					if((filter & QDir::Writeable) && !fi.isWriteable()) goto remove;
					if((filter & QDir::Executable) && !fi.isExecutable()) goto remove;
				}
				continue;
remove:
				QStringList::iterator sit_=sit;
				QFileInfoList::iterator fit_=fit;
				--sit; --fit;
				leafs.remove(sit);
				leafinfos->remove(fit);
			}
		}
		if((sortBy & QDir::SortByMask)!=QDir::Unsorted || (sortBy & QDir::DirsFirst))
		{
			switch(sortBy & QDir::SortByMask)
			{
			case QDir::Name:
				{	// Name sort
					QValueListQSort<FXString, Pol::itMove, Pol::itSwap, NameCompare> sorter(leafs);
					sorter.parent=this;
					sorter.run();
					break;
				}
			case QDir::Time:
			case QDir::Size:
				{	// Expensive sort
					doLeafInfos();
					QValueList<ExpensiveSortRef> list;
					QStringList::iterator sit=leafs.begin();
					QFileInfoList::iterator fit=leafinfos->begin();
					for(int idx=0; sit!=leafs.end(); ++sit, ++fit)
					{
						list.push_back(ExpensiveSortRef(idx++, this, sit, fit));
					}
					QValueListQSort<ExpensiveSortRef, ExpensiveMover, ExpensiveSwap> sorter(list);
					sorter.run();
					break;
				}
			case QDir::Unsorted:
				break;
			}
			//fxmessage("Hello!\n");
			if(sortBy & QDir::DirsFirst)
			{
				doLeafInfos();
				QValueList<ExpensiveSortRef> list;
				QStringList::iterator sit=leafs.begin();
				QFileInfoList::iterator fit=leafinfos->begin();
				for(int idx=0; sit!=leafs.end(); ++sit, ++fit)
				{
					list.push_back(ExpensiveSortRef(idx++, this, sit, fit));
				}
				QValueListQSort<ExpensiveSortRef, ExpensiveMover, ExpensiveSwap, ExpensiveDirsFirst> sorter(list);
				sorter.run(QVLQSortStable);
			}
			if(sortBy & QDir::Reversed)
			{
				leafs.reverse();
				if(leafinfos) leafinfos->reverse();
			}
		}
		dirty=false;
	}
}


QDir::QDir() : p(0)
{
	FXERRHM(p=new FXDirPrivate(FXString::nullStr(), FXString::nullStr(), Name|IgnoreCase, All));
}
QDir::QDir(const FXString &path, const FXString &regex, int sortBy, int filter) : p(0)
{
	FXERRHM(p=new FXDirPrivate(path, regex, sortBy, filter));
}
QDir::QDir(const QDir &o) : p(0)
{
	FXERRHM(p=new FXDirPrivate(*o.p));
}
QDir &QDir::operator=(const QDir &o)
{
	FXDELETE(p);
	FXERRHM(p=new FXDirPrivate(*o.p));
	return *this;
}
QDir &QDir::operator=(const FXString &path)
{
	if(p->path!=path)
	{
		p->path=path;
		p->dirty=true;
	}
	return *this;
}
QDir::~QDir()
{ FXEXCEPTIONDESTRUCT1 {
	FXDELETE(p);
} FXEXCEPTIONDESTRUCT2; }
bool QDir::operator==(const QDir &o) const
{
	if(comparecase(p->path, o.p->path)!=0) return false;
	if(p->leafinfos && o.p->leafinfos)
	{
		return *p->leafinfos==*o.p->leafinfos;
	}
	return p->leafs==o.p->leafs;
}
bool QDir::operator!=(const QDir &o) const
{
	if(comparecase(p->path, o.p->path)==0) return false;
	if(p->leafinfos && o.p->leafinfos)
	{
		return *p->leafinfos!=*o.p->leafinfos;
	}
	return p->leafs!=o.p->leafs;
}
void QDir::setPath(const FXString &path)
{
	if(p->path!=path)
	{
		p->path=path;
		p->dirty=true;
	}
}
const FXString &QDir::path() const
{
	return p->path;
}
FXString QDir::absPath() const
{
	return FXPath::absolute(p->path);
}
FXString QDir::canonicalPath() const
{
	FXString ret_=cleanDirPath(p->path);
	FXString ret=FXWinJunctionPoint::test(ret_) ? FXWinJunctionPoint::read(ret_) : ret;
	return ret.empty() ? ret_ : ret;
}
FXString QDir::dirName() const
{
	return FXPath::name(p->path);
}
FXString QDir::filePath(const FXString &file, bool acceptAbs) const
{
	if(acceptAbs && FXPath::isAbsolute(file)) return file;
	return p->path+QDir::separator()+file;
}
FXString QDir::absFilePath(const FXString &file, bool acceptAbs) const
{
	return FXPath::absolute(filePath(file, acceptAbs));
}
FXString QDir::convertSeparators(const FXString &path)
{
	FXString ret=path;
#if PATHSEP!='/'
	ret.substitute('/', PATHSEP);
#else
	ret.substitute('\\', PATHSEP);
#endif
	return ret;
}
bool QDir::cd(const FXString &name, bool acceptAbs)
{
	setPath(filePath(name, acceptAbs));
	return exists();
}
bool QDir::cdUp()
{
	setPath(FXPath::upLevel(p->path));
	return exists();
}
const FXString &QDir::nameFilter() const
{
	return p->regex;
}
void QDir::setNameFilter(const FXString &regex)
{
	if(p->regex!=regex)
	{
		p->regex=regex;
		p->dirty=true;
	}
}
QDir::FilterSpec QDir::filter() const
{
	return (FilterSpec) p->filter;
}
void QDir::setFilter(int filter)
{
	if(p->filter!=filter)
	{
		p->filter=filter;
		p->dirty=true;
	}
}
QDir::SortSpec QDir::sorting() const
{
	return (SortSpec) p->sortBy;
}
void QDir::setSorting(int sorting)
{
	if(p->sortBy!=sorting)
	{
		p->sortBy=sorting;
		p->dirty=true;
	}
}
bool QDir::matchAllDirs() const
{
	return p->allDirs;
}
void QDir::setMatchAllDirs(bool matchAll)
{
	if(p->allDirs!=matchAll)
	{
		p->allDirs=matchAll;
		p->dirty=true;
	}
}

FXuint QDir::count() const
{
	p->read();
	return (FXuint) p->leafs.count();
}
const FXString &QDir::operator[](int idx) const
{
	p->read();
	return p->leafs[idx];
}
QStringList QDir::entryList(const FXString &regex, int filter, int sorting)
{
	if((int) DefaultFilter!=filter) setFilter(filter);
	if((int) DefaultSort!=sorting) setSorting(sorting);
	p->read();
	return p->leafs;
}
const QFileInfoList *QDir::entryInfoList(const FXString &regex, int filter, int sorting)
{
	if((int) DefaultFilter!=filter) setFilter(filter);
	if((int) DefaultSort!=sorting) setSorting(sorting);
	p->read();
	p->doLeafInfos();
	return p->leafinfos;
}
QStringList QDir::drives()
{
	QStringList drvs;
#ifdef USE_WINAPI
	TCHAR buffer[256];
	DWORD ret;
	FXERRHWIN(ret=GetLogicalDriveStrings(sizeof(buffer)/sizeof(TCHAR), buffer));
	for(TCHAR *buff=buffer; buff<buffer+ret; ++buff)
	{
		if(buff[0])
		{
			FXString drv=buff;
			drvs.append(drv);
			buff+=drv.length();
		}
	}
#endif
#ifdef USE_POSIX
	drvs.append(FXString("/"));
#endif
	return drvs;
}
void QDir::refresh()
{
	p->dirty=true;
}

bool QDir::mkdir(const FXString &leaf, bool acceptAbs)
{
	FXString path(filePath(leaf, acceptAbs));
	if(!FXDir::create(path))
	{
		THROWPOSTFOX(path);
	}
	else
	{	// Set ACL so only current user has all access
		FXERRH_TRY
		{
			QFile::setPermissions(path, FXACL::default_(FXACL::Directory, false, 4));
		}
		FXERRH_CATCH(...)
		{
		}
		FXERRH_ENDTRY
	}
	return true;
}
bool QDir::rmdir(const FXString &leaf, bool acceptAbs)
{
	FXString path(filePath(leaf, acceptAbs));
	if(!FXDir::remove(path))
		THROWPOSTFOX(path);
	return true;
}
bool QDir::remove(const FXString &leaf, bool acceptAbs)
{
	FXString path(filePath(leaf, acceptAbs));
	if(!FXDir::remove(path))
		THROWPOSTFOX(path);
	return true;
}
bool QDir::rename(const FXString &src, const FXString &dest, bool acceptAbs)
{
	FXString path(filePath(src, acceptAbs));
	if(!FXDir::rename(path, filePath(dest, acceptAbs)))
		THROWPOSTFOX(path);
	return true;
}
bool QDir::exists(const FXString &leaf, bool acceptAbs)
{
	return FXStat::exists(filePath(leaf, acceptAbs))!=0;
}

bool QDir::isReadable() const
{
	return FXStat::isReadable(p->path)!=0;
}
bool QDir::exists() const
{
	return FXStat::exists(p->path)!=0;
}
bool QDir::isRoot() const
{
	return FXPath::root(p->path)==p->path;
}
bool QDir::isRelative() const
{
	return !FXPath::isAbsolute(p->path);
}
void QDir::convertToAbs()
{
	p->path=FXPath::absolute(p->path);
}

FXString QDir::separator() { return PATHSEPSTRING; }
QDir QDir::current()
{
	return QDir(currentDirPath());
}
QDir QDir::home()
{
	return QDir(homeDirPath());
}
QDir QDir::root()
{
	return QDir(rootDirPath());
}
FXString QDir::currentDirPath()
{
	return FXSystem::getCurrentDirectory();
}
FXString QDir::homeDirPath()
{
	return FXSystem::getHomeDirectory();
}
FXString QDir::rootDirPath()
{
#ifdef USE_WINAPI
	return FXPath::root(FXSystem::getEnvironment("SystemRoot"));
#endif
#ifdef USE_POSIX
	return "/";
#endif
}
bool QDir::match(const FXString &filter, const FXString &filename)
{
	return FXPath::match(filter, filename, FILEMATCH_FILE_NAME|FILEMATCH_NOESCAPE|FILEMATCH_CASEFOLD)!=0;
}
FXString QDir::cleanDirPath(const FXString &path)
{
	return FXPath::simplify(path);
}
bool QDir::isRelativePath(const FXString &path)
{
	return !FXPath::isAbsolute(path);
}
QStringList QDir::extractChanges(const QDir &A, const QDir &B)
{
	A.p->doLeafInfos(); B.p->doLeafInfos();
	// Could do with a better algorithm here making use of any sorting
	QMemArray<FXuchar> sameA(A.count()), sameB(B.count());
	FXuint idxA=0;
	for(QFileInfoList::const_iterator itA=A.p->leafinfos->begin(); itA!=A.p->leafinfos->end(); ++idxA, ++itA)
	{
		const QFileInfo &a=*itA;
		FXuint idxB=0;
		for(QFileInfoList::const_iterator itB=B.p->leafinfos->begin(); itB!=B.p->leafinfos->end(); ++idxB, ++itB)
		{
			if(sameB[idxB]) continue;
			const QFileInfo &b=*itB;
			if(a==b)
			{
				sameA[idxA]=1; sameB[idxB]=1;
				break;
			}
		}
	}
	QStringList ret;
	for(idxA=0; idxA<A.count(); idxA++)
	{
		if(!sameA[idxA])
		{
			ret.append(A.p->leafs[idxA]);
		}
	}
	for(idxA=0; idxA<B.count(); idxA++)
	{
		FXString &b=B.p->leafs[idxA];
		if(!sameB[idxA] && -1==ret.findIndex(b))
		{
			ret.append(b);
		}
	}
	return ret;
}

} // namespace

