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
#include "FXDir.h"
#include "FXString.h"
#include "FXFileInfo.h"
#include "FXException.h"
#include "FXRollback.h"
#include "FXFile.h"
#include "FXTrans.h"
#include "FXErrCodes.h"
#include "qmemarray.h"
#ifndef USE_POSIX
#define USE_WINAPI
#include "WindowsGubbins.h"
#define THROWPOSTFOX FXERRHWIN(0);
#endif
#ifdef USE_POSIX
#define THROWPOSTFOX FXERRHOS(-1);
#endif
#include "FXMemDbg.h"
#if defined(DEBUG) && defined(FXMEMDBG_H)
static const char *_fxmemdbg_current_file_ = __FILE__;
#endif


namespace FX {

#define TESTABS(path) \
	FXERRH(acceptAbs || FXFile::isAbsolute(path)==0, FXTrans::tr("FXDir", "This is an absolute path"), FXDIR_ISABSOLUTEPATH, 0)

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
			if((parent->sortBy & FXDir::SortByMask)==FXDir::Time) return (*fit).lastModified()<(*o.fit).lastModified();
			if((parent->sortBy & FXDir::SortByMask)==FXDir::Size) return (*fit).size()<(*o.fit).size();
			return false;
		}
		bool operator==(const ExpensiveSortRef &o) const
		{
			if((parent->sortBy & FXDir::SortByMask)==FXDir::Time) return (*fit).lastModified()==(*o.fit).lastModified();
			if((parent->sortBy & FXDir::SortByMask)==FXDir::Size) return (*fit).size()==(*o.fit).size();
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
			if(parent->sortBy & FXDir::IgnoreCase)
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
			const FXFileInfo &fa=*a.fit, &fb=*b.fit;
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
			leafinfos->append(FXFileInfo(FXFile::join(path, *it)));
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
			FXuint findflags=LIST_CASEFOLD|LIST_NO_PARENT;
			if(!(filter & FXDir::Dirs) && !allDirs) findflags|=LIST_NO_DIRS;
			if(!(filter & FXDir::Files)) findflags|=LIST_NO_FILES;
			if(filter & FXDir::Hidden) findflags|=LIST_HIDDEN_FILES|LIST_HIDDEN_DIRS;
			if(allDirs) findflags|=LIST_ALL_DIRS;
			if(regex.empty()) findflags|=LIST_ALL_FILES|LIST_ALL_DIRS;
			FXint listcnt=FXFile::listFiles(list, path, regex, findflags);
			FXRBOp unlist=FXRBNewA(list);
			for(FXint n=0; n<listcnt; n++)
			{
				leafs.append(list[n]);
			}
		}
		if(!(filter & (FXDir::Readable|FXDir::Writeable|FXDir::Executable)))
			filter|=(FXDir::Readable|FXDir::Writeable|FXDir::Executable);
		bool a,b=false;
		if((a=(filter & FXDir::NoSymLinks)!=0)
			|| (b=(filter & (FXDir::Readable|FXDir::Writeable|FXDir::Executable))!=(FXDir::Readable|FXDir::Writeable|FXDir::Executable)))
		{	// Got to generate the file infos
			doLeafInfos();
			QStringList::iterator sit=leafs.begin();
			QFileInfoList::iterator fit=leafinfos->begin();
			for(; sit!=leafs.end(); ++sit, ++fit)
			{
				FXFileInfo &fi=*fit;
				if(allDirs && fi.isDir()) continue;
				if(a)
				{
					if((filter & FXDir::NoSymLinks) && fi.isSymLink()) goto remove;
				}
				if(b)
				{
					if((filter & FXDir::Readable) && !fi.isReadable()) goto remove;
					if((filter & FXDir::Writeable) && !fi.isWriteable()) goto remove;
					if((filter & FXDir::Executable) && !fi.isExecutable()) goto remove;
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
		if((sortBy & FXDir::SortByMask)!=FXDir::Unsorted || (sortBy & FXDir::DirsFirst))
		{
			switch(sortBy & FXDir::SortByMask)
			{
			case FXDir::Name:
				{	// Name sort
					QValueListQSort<FXString, Pol::itMove, Pol::itSwap, NameCompare> sorter(leafs);
					sorter.parent=this;
					sorter.run();
					break;
				}
			case FXDir::Time:
			case FXDir::Size:
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
			case FXDir::Unsorted:
				break;
			}
			//fxmessage("Hello!\n");
			if(sortBy & FXDir::DirsFirst)
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
			if(sortBy & FXDir::Reversed)
			{
				leafs.reverse();
				if(leafinfos) leafinfos->reverse();
			}
		}
		dirty=false;
	}
}


FXDir::FXDir() : p(0)
{
	FXERRHM(p=new FXDirPrivate(FXString::nullStr(), FXString::nullStr(), Name|IgnoreCase, All));
}
FXDir::FXDir(const FXString &path, const FXString &regex, int sortBy, int filter) : p(0)
{
	FXERRHM(p=new FXDirPrivate(path, regex, sortBy, filter));
}
FXDir::FXDir(const FXDir &o) : p(0)
{
	FXERRHM(p=new FXDirPrivate(*o.p));
}
FXDir &FXDir::operator=(const FXDir &o)
{
	FXDELETE(p);
	FXERRHM(p=new FXDirPrivate(*o.p));
	return *this;
}
FXDir &FXDir::operator=(const FXString &path)
{
	if(p->path!=path)
	{
		p->path=path;
		p->dirty=true;
	}
	return *this;
}
FXDir::~FXDir()
{ FXEXCEPTIONDESTRUCT1 {
	FXDELETE(p);
} FXEXCEPTIONDESTRUCT2; }
bool FXDir::operator==(const FXDir &o) const
{
	if(comparecase(p->path, o.p->path)!=0) return false;
	if(p->leafinfos && o.p->leafinfos)
	{
		return *p->leafinfos==*o.p->leafinfos;
	}
	return p->leafs==o.p->leafs;
}
bool FXDir::operator!=(const FXDir &o) const
{
	if(comparecase(p->path, o.p->path)==0) return false;
	if(p->leafinfos && o.p->leafinfos)
	{
		return *p->leafinfos!=*o.p->leafinfos;
	}
	return p->leafs!=o.p->leafs;
}
void FXDir::setPath(const FXString &path)
{
	if(p->path!=path)
	{
		p->path=path;
		p->dirty=true;
	}
}
const FXString &FXDir::path() const
{
	return p->path;
}
FXString FXDir::absPath() const
{
	return FXFile::absolute(p->path);
}
FXString FXDir::canonicalPath() const
{
	FXString ret_=cleanDirPath(p->path);
	FXString ret=FXFile::symlink(ret_);
	return ret.empty() ? ret_ : ret;
}
FXString FXDir::dirName() const
{
	return FXFile::name(p->path);
}
FXString FXDir::filePath(const FXString &file, bool acceptAbs) const
{
	if(acceptAbs && FXFile::isAbsolute(file)) return file;
	return p->path+FXDir::separator()+file;
}
FXString FXDir::absFilePath(const FXString &file, bool acceptAbs) const
{
	return FXFile::absolute(filePath(file, acceptAbs));
}
FXString FXDir::convertSeparators(const FXString &path)
{
	FXString ret=path;
#if PATHSEP!='/'
	ret.substitute('/', PATHSEP);
#else
	ret.substitute('\\', PATHSEP);
#endif
	return ret;
}
bool FXDir::cd(const FXString &name, bool acceptAbs)
{
	setPath(filePath(name, acceptAbs));
	return exists();
}
bool FXDir::cdUp()
{
	setPath(FXFile::upLevel(p->path));
	return exists();
}
const FXString &FXDir::nameFilter() const
{
	return p->regex;
}
void FXDir::setNameFilter(const FXString &regex)
{
	if(p->regex!=regex)
	{
		p->regex=regex;
		p->dirty=true;
	}
}
FXDir::FilterSpec FXDir::filter() const
{
	return (FilterSpec) p->filter;
}
void FXDir::setFilter(int filter)
{
	if(p->filter!=filter)
	{
		p->filter=filter;
		p->dirty=true;
	}
}
FXDir::SortSpec FXDir::sorting() const
{
	return (SortSpec) p->sortBy;
}
void FXDir::setSorting(int sorting)
{
	if(p->sortBy!=sorting)
	{
		p->sortBy=sorting;
		p->dirty=true;
	}
}
bool FXDir::matchAllDirs() const
{
	return p->allDirs;
}
void FXDir::setMatchAllDirs(bool matchAll)
{
	if(p->allDirs!=matchAll)
	{
		p->allDirs=matchAll;
		p->dirty=true;
	}
}

FXuint FXDir::count() const
{
	p->read();
	return (FXuint) p->leafs.count();
}
const FXString &FXDir::operator[](int idx) const
{
	p->read();
	return p->leafs[idx];
}
QStringList FXDir::entryList(const FXString &regex, int filter, int sorting)
{
	if(DefaultFilter!=filter) setFilter(filter);
	if(DefaultSort!=sorting) setSorting(sorting);
	p->read();
	return p->leafs;
}
const QFileInfoList *FXDir::entryInfoList(const FXString &regex, int filter, int sorting)
{
	if(DefaultFilter!=filter) setFilter(filter);
	if(DefaultSort!=sorting) setSorting(sorting);
	p->read();
	p->doLeafInfos();
	return p->leafinfos;
}
QStringList FXDir::drives()
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
void FXDir::refresh()
{
	p->dirty=true;
}

bool FXDir::mkdir(const FXString &leaf, bool acceptAbs)
{
	if(!FXFile::createDirectory(filePath(leaf, acceptAbs), S_IREAD|S_IWRITE))
		THROWPOSTFOX;
	return true;
}
bool FXDir::rmdir(const FXString &leaf, bool acceptAbs)
{
	if(!FXFile::remove(filePath(leaf, acceptAbs)))
		THROWPOSTFOX;
	return true;
}
bool FXDir::remove(const FXString &leaf, bool acceptAbs)
{
	if(!FXFile::remove(filePath(leaf, acceptAbs)))
		THROWPOSTFOX;
	return true;
}
bool FXDir::rename(const FXString &src, const FXString &dest, bool acceptAbs)
{
	if(!FXFile::move(filePath(src, acceptAbs), filePath(dest, acceptAbs)))
		THROWPOSTFOX;
	return true;
}
bool FXDir::exists(const FXString &leaf, bool acceptAbs)
{
	return FXFile::exists(filePath(leaf, acceptAbs))!=0;
}

bool FXDir::isReadable() const
{
	return FXFile::isReadable(p->path)!=0;
}
bool FXDir::exists() const
{
	return FXFile::exists(p->path)!=0;
}
bool FXDir::isRoot() const
{
	return FXFile::root(p->path)==p->path;
}
bool FXDir::isRelative() const
{
	return !FXFile::isAbsolute(p->path);
}
void FXDir::convertToAbs()
{
	p->path=FXFile::absolute(p->path);
}

FXString FXDir::separator() { return PATHSEPSTRING; }
FXDir FXDir::current()
{
	return FXDir(currentDirPath());
}
FXDir FXDir::home()
{
	return FXDir(homeDirPath());
}
FXDir FXDir::root()
{
	return FXDir(rootDirPath());
}
FXString FXDir::currentDirPath()
{
	return FXFile::getCurrentDirectory();
}
FXString FXDir::homeDirPath()
{
	return FXFile::getHomeDirectory();
}
FXString FXDir::rootDirPath()
{
#ifdef USE_WINAPI
	return FXFile::root(FXFile::getEnvironment("SystemRoot"));
#endif
#ifdef USE_POSIX
	return "/";
#endif
}
bool FXDir::match(const FXString &filter, const FXString &filename)
{
	return FXFile::match(filter, filename, FILEMATCH_FILE_NAME|FILEMATCH_NOESCAPE|FILEMATCH_CASEFOLD)!=0;
}
FXString FXDir::cleanDirPath(const FXString &path)
{
	return FXFile::simplify(path);
}
bool FXDir::isRelativePath(const FXString &path)
{
	return !FXFile::isAbsolute(path);
}
QStringList FXDir::extractChanges(const FXDir &A, const FXDir &B)
{
	A.p->doLeafInfos(); B.p->doLeafInfos();
	// Could do with a better algorithm here making use of any sorting
	QMemArray<FXuchar> sameA(A.count()), sameB(B.count());
	FXuint idxA=0;
	for(QFileInfoList::const_iterator itA=A.p->leafinfos->begin(); itA!=A.p->leafinfos->end(); ++idxA, ++itA)
	{
		const FXFileInfo &a=*itA;
		FXuint idxB=0;
		for(QFileInfoList::const_iterator itB=B.p->leafinfos->begin(); itB!=B.p->leafinfos->end(); ++idxB, ++itB)
		{
			if(sameB[idxB]) continue;
			const FXFileInfo &b=*itB;
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

