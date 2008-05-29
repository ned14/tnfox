/********************************************************************************
*                                                                               *
*                         P r o c e s s   S u p p o r t                         *
*                                                                               *
*********************************************************************************
* Copyright (C) 2002,2003 by Niall Douglas.   All Rights Reserved.              *
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

#ifndef _FXProcess_h_
#define _FXProcess_h_

#include "fxdefs.h"
#include "FXString.h"
#include "FXGenericTools.h"

namespace FX {

/*! \file FXProcess.h
\brief Defines classes used in using processes
*/

/*! \struct timeval
\brief A duplicate of the BSD timeval structure

This is only defined in the FX namespace so that code may always find it -
thus working around a problem on Windows where struct timeval may or may
not be defined
*/
typedef struct timeval
{
	long tv_sec;
	long tv_usec;
} timeval;

class QThreadPool;
class FXACL;

/*! \class FXProcess
\brief Provides process-wide information and control

Where FXApp initialises the GUI subsystem, FXProcess initialises just those parts
sufficient for command-line operation. Probably one of its most useful facilities is
FXProcess_StaticInit which is used by a number of internal classes already.

FXProcess should be initialised first thing into main():
\code
int main(int argc, char *argv[])
{
	FXProcess myprocess(argc, argv);
	FXApp app("name");		// Only for GUI apps
	app.init(argc, argv);	// Only for GUI apps
	...
	app.create();			// Only for GUI apps
	...
	return app.run();		// Only for GUI apps
}
\endcode

<h4>Dynamic library facilities</h4>
You should use dllLoad() to load dynamic libraries - this loads and runs any
FXProcess_StaticInit's the newly loaded library may have. If you manage your own
loading, call runPendingStaticInits(). The search behaviour on POSIX is different
than that for \c dlopen() - if a relative path, it emulates Windows by searching around where the executable
is, then the current directory, then \c dlopen()'s default. It also tries appending
\c .so if one isn't present already as well as prepending \c lib - this behaviour
ensures that equivalent behaviour occurs both on Windows and POSIX. Watch out for
calling DLL's the same as executables - on POSIX, there is no \c .exe extension
to differentiate.

Once loaded you can resolve symbols using dllHandle::dllResolve() which returns
a functor for that API. Note that C++ mangles its API names so you will have
to specify <tt>extern "C"</tt> before their declaration in your DLL.

<h4>Free system resources</h4>
hostOSProcessorLoad(), hostOSMemoryLoad() and hostOSDiscIOLoad() allow your code
to determine how free the system is in those areas. This can be used to dynamically adjust
whether to use lots of memory and a fast algorithm or little memory and a slow
algorithm.

A few parts of TnFOX use the convenience functions processorFull(),
memoryFull() and discIOFull() but this facility is mostly intended for Tn. These
functions return an integer index whereby zero is not full, one is getting full,
two is nearly full and three is full and over. These numbers can be included
in calculations as divisors of buffer sizes etc.

\note hostOSProcessLoad() and hostDiscIOLoad() are currently unimplemented. At
some stage I'll do like FXSecure and maintain a shared memory region with the
last five seconds of data from which meaningful values can be constructed.

<h4>Process-wide thread pool</h4>
A process-wide thread pool consisting of four threads is available at FXProcess::threadPool()
and is created on first use. FX::QThreadPool offers timed callback facilities
which are used by other code such as FX::FXFSMonitor. Once created, the thread pool
lasts until the process terminates.

<h4>Security:</h4>
On POSIX, there is the weird and wonderful world of real and effective user
ids and groups. Basically the real id of a process is under which user's
shell it is running whereas the effective id is which user the process is
impersonating. It gets more complex again on Linux because a process can
impersonate someone different again only when accessing the file system.
This does not in my mind make for easy security - and worst of all, you
can't change these impersonation settings on a per-thread basis, only on
a per-process one.

Hence we are substantially limited in what we can do whilst keeping semantics
similar between NT and POSIX - unfortunately we need the ability to run as
root but act like someone else because the Tn kernel can run as a daemon/service
and the secure heap needs to be able to lock memory pages. I've decided
that if the filing system is accessed like the real uid and TnFOX's ACL
based security support forces the owner on all things it touches to be
the real uid, it's safe to run the process with the suid bit set (ie;
acting like root). Of course, users may not agree with this :)

\note userHandedness() is currently only implemented for Windows
*/
struct FXProcessPrivate;
class FXProcess_StaticInitBase;
class FXProcess_StaticDepend;
template<class type> class QValueList;
template<class type> class QMemArray;
class FXAPI FXProcess
{
	FXProcessPrivate *p;
	FXDLLLOCAL void init(int &argc, char *argv[]);
	FXDLLLOCAL void destroy();
public:
	//! Initialises the process
	FXProcess();
	//! \overload
	FXProcess(int &argc, char *argv[]);
	~FXProcess();
	//! Cleans up the process and exits the process
	static void exit(FXint code=0);
	//! Runs any pending static inits. Only required if you implement your own shared library loading.
	bool runPendingStaticInits(int &argc, char *argv[], FXStream &txtout);
	//! Is application an automated test?
	static bool isAutomatedTest() throw();

	//! Returns a pointer to the TProcess for this process
	static FXProcess *instance();
	//! Returns a millisecond count. This number will wrap approximately every 50 days
	static FXuint getMsCount();
	//! Fills in the specified timeval structure with the current time
	static void getTimeOfDay(struct timeval *ts);
	/*! Returns a nanosecond count. It is highly unlikely that this timer is actually
	nanosecond accurate, but you have a good chance it is at least microsecond accurate.
	On systems which do not support high resolution timers, this returns getMsCount()
	multiplied by one million. You should not rely on this counter having its full
	64 bit range ie; it may wrap sooner on some systems */
	static FXulong getNsCount();
	//! Returns the unique id of this process within the system
	static FXuint id();
	//! Returns the number of processors available to this process
	static FXuint noOfProcessors();
	//! Returns the size of a memory page on this machine
	static FXuint pageSize();
	//! A structure containing information about a mapped file
	struct MappedFileInfo
	{
		FXString path;					//!< Full path to the binary.
		FXuval startaddr, endaddr;		//!< Start and end addresses of where it's mapped to
		FXuval offset;					//!< From which offset in the file
		FXuval length;					//!< Length of mapped section (basically \c endaddr-startaddr)
		bool read, write, execute, copyonwrite;	//!< Reflecting if the section is readable, writeable, executable and/or copy-on-write

		bool operator<(const MappedFileInfo &o) { return startaddr<o.startaddr; }
	};
	//! Defines a list of FXProcess::MappedFileInfo
	typedef QValueList<MappedFileInfo> MappedFileInfoList;
	/*! Returns a list of files currently mapped into this process. As this call is slow, a cache is
	maintained and updated accordingly when dllLoad() and dllUnload() are used. If however you wish
	to bypass the cache, call with \em forceRefresh set to true */
	static MappedFileInfoList mappedFiles(bool forceRefresh=false);
	//! Returns a full path to the currently running executable
	static const FXString &execpath();
	/*! Returns a full path to the binary containing the specified address with
	optional return of start and end addresses of the binary map
	\warning This is not an especially fast call
	*/
	static FXString dllPath(void *addr, void **dllstart=0, void **dllend=0);
	/*! \warning Copies of this object delete the source. If you want a separate copy, call dllLoad() again. */
	class dllHandle
	{
		friend class FXProcess;
		void *h;
		dllHandle(void *_h) : h(_h) { }
	public:
		dllHandle() : h(0) { }
#ifndef HAVE_CPP0XFEATURES
#ifdef HAVE_CONSTTEMPORARIES
		dllHandle(const dllHandle &other) : h(other.h)
		{
			dllHandle &o=const_cast<dllHandle &>(other);
#else
		dllHandle(dllHandle &o) : h(o.h)
		{
#endif
#else
	private:
		dllHandle(const dllHandle &);		// disable copy constructor
	public:
		dllHandle(dllHandle &&o) : h(o.h)
		{
#endif
			o.h=0;
		}
		~dllHandle() { if(h) dllUnload(*this); }
		dllHandle &operator=(dllHandle &o)
		{
			if(h)
			{
				dllUnload(*this);
				h=0;
			}
			h=o.h;
			o.h=0;
			return *this;
		}
		//! Returns true if handle is empty
		bool operator!() const throw() { return !h; }
		/*! Resolves the specified symbol in the given library, returning a functor
		representing that API. Usage might be as follows:
		\code
		// In API definition header file
		int getArgs(int argc, char **argv);
		// To resolve:
		typedef FX::Generic::TL::create<int, int, char **>::value getArgsSpec;
		FX::Generic::Functor<getArgsSpec> getArgs_=dllh.resolve<getArgsSpec>("getArgs");
		// To call:
		getArgs_(2, {"Hello", "World", 0});
		\endcode
		*/
		template<typename parslist> Generic::Functor<parslist> resolve(const char *apiname) const
		{
			return Generic::Functor<parslist>((typename Generic::Functor<parslist>::void_ *) FXProcess::dllResolveBase(*this, apiname));
		}
	};
	/*! Loads in the specified dynamic library and initialises its static data. The
	handle returns automatically calls dllUnload() on destruction if it hadn't been
	already closed.
	*/
	static dllHandle dllLoad(const FXString &path);
	/*! Resolves the specified symbol in the given library to its address.
	The \c dllHandle::resolve() forms should be preferred to this as they are typesafe
	and much easier to use.
	*/
	static void *dllResolveBase(const dllHandle &h, const char *apiname);
	//! Unloads the specified dynamic library
	static void dllUnload(dllHandle &h);
	/*! Returns a short description of the operating system eg; "Win32", "Linux" etc
	that TnFOX was built for with a slash '/' and then the machine architecture TnFOX
	was built for. This can vary from hostOSDescription() below. */
	static FXString hostOS(FXString *myos=0, FXString *architecture=0);
	/*! Returns a textual description of the host operating system. The format
	is as follows:

	<tt>\<API name> (\<kernel name\> [kernel version] kernel) version \<API version\>, \<machine architecture\> architecture</tt>

	<i>API name</i> can be one of the following:
	\li Win32
	\li Win64
	\li POSIX.2

	The <i>kernel name</i> is whatever the operating system says it is (eg; 9x, NT, Linux)
	and the <i>kernel version</i> is whatever the operating system thinks it is. <i>API version</i> for
	Windows generally follows 4.x for Windows 95/NT until Windows 2000 where it becomes 5.x.
	For POSIX, it is always POSIX.2 meaning v2 but the API version is whatever the operating
	system reports itself as compliant with. The point to remember is this function is
	concerned with \b API version which is in fact all you need know about as well even
	in non-portable code.

	Lastly, <i>machine architecture</i> currently can be one of: i486, PowerPC, Alpha, IA64,
	AMD64, unknown.
	*/
	static FXString hostOSDescription(FXString *myapi=0, FXString *kernelname=0, FXString *kernelversion=0,
		FXString *apiversion=0, FXString *architecture=0);
	/*! Returns build information for TnFOX. \em svnrev is the latest SVN revision of the
	repository from which TnFOX was checked out. Note this can be -1 if \c svnversion wasn't
	available during build */
	static void buildInfo(int *svnrev=0);
	/*! Returns a normalised floating-point value whereby \c 1.0 means full. System processors
	are combined so that CPU A at 100% and CPU B at 0% yields the same return as both CPU's
	being at 50%
	\note Currently not implemented
	*/
	static FXfloat hostOSProcessorLoad();
	//! Returns an index 0...3 of how full the processor is
	static FXuint processorFull()
	{
		FXfloat v=hostOSProcessorLoad();
		if(v<0.95) return 0;
		else if(v<0.98) return 1;
		else if(v<1.0) return 2;
		else return 3;
	}
	/*! Returns a normalised floating-point value whereby \c 1.0 means full. On Windows this
	is the total virtual memory allocation versus physical memory installed. On Linux this
	is (used-cached)+swap used versus physical memory installed. If \em totalPhysMem is set,
	returns the total physical memory in the machine */
	static FXfloat hostOSMemoryLoad(FXuval *totalPhysMem=0);
	//! Returns an index 0...3 of how full the memory is
	static FXuint memoryFull()
	{
		FXfloat v=hostOSMemoryLoad();
		if(v<0.95) return 0;
		else if(v<0.98) return 1;
		else if(v<1.0) return 2;
		else return 3;
	}
	/*! Returns a normalised floating-point value whereby \c 1.0 means full.
	\note Currently not implemented
	*/
	static FXfloat hostOSDiscIOLoad(const FXString &path);
	//! \overload
	static FXfloat hostOSDiskIOLoad(const FXString &path) { return hostOSDiscIOLoad(path); }
	//! Returns an index 0...3 of how full the disc i/o is
	static FXuint discIOFull(const FXString &path)
	{
		FXfloat v=hostOSDiscIOLoad(path);
		if(v<0.95) return 0;
		else if(v<0.98) return 1;
		else if(v<1.0) return 2;
		else return 3;
	}
	//! \overload
	static FXuint diskIOFull(const FXString &path) { return discIOFull(path); }
    /*! Lets you override the values returned by the free system resources functions.
    Use a negative number to reset to dynamic calculation */
    static void overrideFreeResources(FXfloat memory=-1, FXfloat processor=-1, FXfloat discio=-1);
	/*! Returns an estimation of virtual address space remaining in this process if chunks
	are allocated in size \em chunk. How this is implemented is highly system specific and
	for some systems is quite hackish so the value can vary by +-5% depending on what thread
	is calling it and such. At least two allocations of size \em chunk will need to be
	temporarily made and the returned value assumes that there are no other allocations
	between that chunk and the end of address space which clearly can be wildly out.
	On all supported platforms actual increases in the memory usage of your process can be
	avoided. You probably want to avoid calling this too frequently as it involves multiple
	calls into kernel space. */
	static FXuval virtualAddrSpaceLeft(FXuval chunk=1);
	//! Information about a mountable partition
	struct MountablePartition
	{
		FXString name;			//!< Mountable partition name
		FXString location;		//!< Where the partition would be mounted (path or drive letter)
		FXString fstype;		//!< Host OS specific string identifying filing system type
		FXuint mounted : 1;		//!< Whether this partition is currently mounted
		FXuint mountable : 1;	//!< Whether this partition can be mounted by the current user
		FXuint readWrite : 1;	//!< Whether this partition is mounted for reading & writing
		char driveLetter;		//!< Drive letter for this partition in upper case (guessed on POSIX)
	};
	/*! Returns a list of partitions on the host operating system. Note that this may not
	be a definitive list eg; if \c /etc/fstab does not map all possible partitions, then
	this list will be incomplete. Superuser permissions is required to get a definitive
	list and it isn't usually needed by applications */
	static QValueList<MountablePartition> mountablePartitions();
	//! Mounts a partition
	static void mountPartition(const FXString &partitionName, const FXString &location=FXString::nullStr(), const FXString &fstype=FXString::nullStr(), bool readWrite=true);
	//! Unmounts a partition
	static void unmountPartition(const FXString &location);

	//! Returns the process-wide thread pool
	static QThreadPool &threadPool();
	//! Indicates which hand the user is
	enum UserHandedness
	{
		UNKNOWN_HANDED=0,
		LEFT_HANDED=1,
		RIGHT_HANDED=2
	};
	//! Returns the "handedness" of the user
	static UserHandedness userHandedness();
	//! Overrides the "handedness" setting of the user
	static void overrideUserHandedness(UserHandedness val);
	/*! Scales your window layout distances according to the screen size - this
	can vary from 400% to 25%. You should not scale content size with this metric! */
	static FXuint screenScale(FXuint value);
	//! Overrides the screen size scaling factor
	static void overrideScreenScale(FXuint percent);
	//! Overrides the screen size (debug only)
	static void overrideScreenSize(FXint w, FXint h);

	//! The specification of the fatal exit vector
	typedef Generic::Functor<Generic::TL::create<void, bool>::value> FatalExitUpcallSpec;
	/*! Registers code to be called when a process exits, even from a hard error such
	as segmentation fault. The form of the code is:
	\code
	void function(bool);
	void Object::member(bool);
	\endcode
	The parameter is true when the exit is fatal, false if the exit is normal.
	As this may be called in the context of a signal, do \b not
	do anything involved at all. If you want to get called on normal process exit,
	consider \c atexit() or FX::FXProcess_StaticInit.
	*/
	static void addFatalExitUpcall(FatalExitUpcallSpec upcallv);
	//! Unregisters a previously installed call upon process exit.
	static bool removeFatalExitUpcall(FatalExitUpcallSpec upcallv);
	//! Returns a list of pages in this process which are locked into memory
	static QMemArray<void *> lockedPages();

	// Do not use these directly
	static void int_addStaticInit(FXProcess_StaticInitBase *o);
	static void int_removeStaticInit(FXProcess_StaticInitBase *o);
	static void int_addStaticDepend(FXProcess_StaticDepend *d);
private:
	friend class FXProcess_MemLock;
	static void *int_lockMem(void *addr, FXuval len);
	static void int_unlockMem(void *h);
	friend class FXWindow;
	friend class FXShell;
	static FXDLLLOCAL bool int_constrainScreen(FXint &x, FXint &y, FXint &w, FXint &h);
	static FXDLLLOCAL bool int_constrainScreen(FXint &w, FXint &h);
};

class FXStream;
class FXAPI FXProcess_StaticInitBase
{
	friend class FXProcess;
	const char *myname;
protected:
	bool done;
	void *dependencies;
public:
	const char *name() const { return myname; }
	FXProcess_StaticInitBase(const char *name) : myname(name), done(false), dependencies(0) { }
	virtual ~FXProcess_StaticInitBase();
	virtual bool create(int &argc, char **argv, FXStream &txtout)=0;
	virtual void destroy()=0;
};

/*! \class FXProcess_StaticInit
\brief Lets you initialise static data in an orderly fashion

Due to the C++ spec not guaranteeing an order of static data construction and furthermore lacking
the ability to portably specify an order, we have provided this class which will construct the
specified object just at the end of the FXProcess constructor and destruct it again just at the
beginning of the FXProcess destructor. This is very useful for objects which require an intact
and functional environment to construct or destruct safely and/or dependencies of other items.

To use, simply do:
\code
static FXProcess_StaticInit<class> name("text name");
FXPROCESS_STATICDEP(name, <other text name>); // As many times as needed
\endcode
For example:
\code
static FXProcess_StaticInit<MyDatabase> thedatabase("The Database");
FXPROCESS_STATICDEP(thedatabase, "The File System");
\endcode
*/
template <class type> class FXProcess_StaticInit : public FXProcess_StaticInitBase
{
protected:
	type *object;
public:
	FXProcess_StaticInit(const char *name) : object(0), FXProcess_StaticInitBase(name)
	{
		FXProcess::int_addStaticInit(this);
	}
	~FXProcess_StaticInit()
	{
		FXDELETE(object);
		FXProcess::int_removeStaticInit(this);
	}
	bool create(int &argc, char **argv, FXStream &txtout)
	{
		if(!object)
		{
			FXERRHM(object=new type);
		}
		return false;
	}
	void destroy()
	{
		FXDELETE(object);
	}
	operator type *() { return object; }
	operator const type *() const { return object; }
	type *operator->() { return object; }
};
class FXAPI FXProcess_StaticDepend
{
	friend class FXProcess;
	FXProcess_StaticInitBase *var;
	const char *onwhat;
	const char *module;
	int lineno;
public:
	FXProcess_StaticDepend(FXProcess_StaticInitBase *_var, const char *_depname, const char *_module, int _lineno) : var(_var), onwhat(_depname), module(_module), lineno(_lineno)
	{
		FXProcess::int_addStaticDepend(this);
	}
};
#define FXPROCESS_UNIQUENAME3(var, no) var##no
#define FXPROCESS_UNIQUENAME2(var, no) FXPROCESS_UNIQUENAME3(var, no)
#define FXPROCESS_UNIQUENAME(var) FXPROCESS_UNIQUENAME2(var, __LINE__)
//! Defines a dependency between the specified static data and some other
#define FXPROCESS_STATICDEP(var, textname) static FXProcess_StaticDepend FXPROCESS_UNIQUENAME(var##_dep) (&var, textname, __FILE__, __LINE__)

/*! \class FXProcess_MemLock
\brief Prevents a region from being paged out for the duration of its existence.

This little class for the duration of its existence locks a range of a process'
address space into memory, thus preventing it from being paged out. This can
be useful when utmost performance is required or more likely to prevent secure
data from being leaked. The FX::Secure namespace heap manages one of these
automatically.

What you specify to this class at construction and what actually gets locked
can vary, not least because locking is done in units of FX::FXProcess::pageSize().
You can determine what pages are locked at any given time using
FX::FXProcess::lockedPages(). Nested locks across multiple threads are fine eg;
\code
FXProcess_LockMem h1(ptr, 16384);       // Thread 1
FXProcess_LockMem h2(ptr+4096, 32768);  // Thread 2
delete h1;                              // Thread 1
\endcode
In the above, the region ptr+4096 to ptr+36864 remains locked.

\warning Many operating systems view locking too many pages in memory as a
malicious act and so FXRESTRICT the maximum per process. In particular on
Windows NT systems it can be a very low amount, perhaps only 200Kb.
*/
class FXAPI FXProcess_MemLock
{
	void *h;
	void *addr;
	FXuval len;
public:
	//! Creates a null lock
	FXProcess_MemLock() : h(0), addr((void *)-1), len(0) { }
	//! Creates an instance locking the specified region
	FXProcess_MemLock(void *startaddr, FXuval length) : h(FXProcess::int_lockMem(startaddr, length)), addr(startaddr), len(length) { }
	FXProcess_MemLock(const FXProcess_MemLock &o) : h(h ? FXProcess::int_lockMem(o.addr, o.len) : 0), addr(o.addr), len(o.len) { }
	FXProcess_MemLock &operator=(const FXProcess_MemLock &o)
	{
		if(h) { FXProcess::int_unlockMem(h); h=0; }
		h=o.h ? FXProcess::int_lockMem(o.addr, o.len) : 0;
		addr=o.addr; len=o.len;
		return *this;
	}
	~FXProcess_MemLock() { if(h) { FXProcess::int_unlockMem(h); h=0; } }
	//! Returns the start of the locked region
	void *location() const throw() { return addr; }
	//! Returns the length of the locked region
	FXuval length() const throw() { return len; }
};

} // namespace

#endif


