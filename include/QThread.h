/********************************************************************************
*                                                                               *
*                 M u l i t h r e a d i n g   S u p p o r t                     *
*                                                                               *
*********************************************************************************
*        Copyright (C) 2002-2010 by Niall Douglas.   All Rights Reserved.       *
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

#ifndef QTHREAD_H
#define QTHREAD_H

#include "FXProcess.h"

namespace FX {

/*! \file QThread.h
\brief Defines classes used in implementation of threads
*/
/*!
\def FXDISABLE_THREADS
Defining this replaces threading classes implementation with static defaults
ie; sufficient to make code run. Note that creating an QThread causes an
exception.
*/
/*!
\def FXINLINE_MUTEX_IMPLEMENTATION
Defining this causes the implementation of FX::FXAtomicInt and FX::QMutex
to be defined inline. This can give substantial speed improvements but also
increases code bloat.
*/

#ifdef FXINLINE_MUTEX_IMPLEMENTATION
 #define QMUTEX_FXAPI
 #define QMUTEX_GLOBALS_FXAPI FXAPI
 // Testing has found that force inlining public declarations makes
 // the code go slower so we leave it up to the compiler
 #define QMUTEX_INLINEI inline FXFORCEINLINE
 #define QMUTEX_INLINEP inline
#else
 #define QMUTEX_FXAPI FXAPI
 #define QMUTEX_GLOBALS_FXAPI
 #define QMUTEX_INLINEI FXFORCEINLINE	// internally inlined inside QThread.cxx
 #define QMUTEX_INLINEP				// publicly inlined
#endif

/*! \class FXAtomicInt
\brief Provides portable interlocked increment, decrement and exchange

Modern processors have direct processor ops for handling atomic
manipulation of an integer in multiprocessor systems (the very least
a multiprocessor capable system needs is an atomic swap instruction).
All synchronisation primitives (such as a semaphore or mutex) require
atomic integer manipulation support in hardware.

There are a number of instances when using QMutex or other TnFOX
synchronisation classes are overkill and inefficient eg; a thread-safe
incrementing counter. In these cases, FXAtomicInt can provide
substantially improved performance. Indeed, QMutex is implemented using it.

With modern compiler help, direct assembler is mostly avoided. On MSVC and GCC,
compiler intrinsics are used to access processor-specific locking instructions.
As GCC has very limited intrinsics in this area requiring a separate lock variable,
on x86 or x64 direct assembler is used on that compiler - however, on other
platforms the operation is simply more inefficient than if native instructions are
used:
\code
3400 Athlon64 uniprocessor on Apple MacOS X with GCC v4.0.1:
                 FXAtomicInt      QMutex
GCC intrinsics:      9159606     2654350
Direct assembler:   47438330     7844367
                      (5.2x)        (3x)
\endcode	
If performance becomes a problem on non x86 platforms, think about adding a
native implementation to this class.

As of v0.85, the macro FX_SMPBUILD when defined causes the CPU to take exclusive
control of the memory bus during the operation which is correct for SMP but overkill
for uniprocessor systems. Setting makeSMPBuild to False in config.py will around
double the FXAtomicInt performance. As an example of how expensive locked memory
bus operations are, write a simple loop incrementing a FXAtomicInt and watch how
slow your machine becomes!

As of v0.86, defining FXINLINE_MUTEX_IMPLEMENTATION defines this class inline
to all code including QThread.h. This can cause substantial performance
improvement at the cost of code size.
*/
class QMUTEX_FXAPI FXAtomicInt
{
	volatile long lock;		// Unused on systems with atomic increment/decrement/exchange-compare
	volatile int value;
	volatile int value2;    // Used for padding
	friend class QMutex;
	friend class QShrdMemMutex;
	QMUTEX_INLINEI FXDLLLOCAL int get() const throw();
	QMUTEX_INLINEI FXDLLLOCAL int set(int i) throw();
	QMUTEX_INLINEI FXDLLLOCAL int incp() throw();
	QMUTEX_INLINEI FXDLLLOCAL int pinc() throw();
	QMUTEX_INLINEI FXDLLLOCAL int finc() throw();
	QMUTEX_INLINEI FXDLLLOCAL int inc(int i) throw();
	QMUTEX_INLINEI FXDLLLOCAL int decp() throw();
	QMUTEX_INLINEI FXDLLLOCAL int pdec() throw();
	QMUTEX_INLINEI FXDLLLOCAL int fdec() throw();
	QMUTEX_INLINEI FXDLLLOCAL int dec(int i) throw();
	QMUTEX_INLINEI FXDLLLOCAL int swapI(int newval) throw();
	QMUTEX_INLINEI FXDLLLOCAL int cmpXI(int compare, int newval) throw();
	QMUTEX_INLINEI FXDLLLOCAL int spinI(int count) throw();
public:
	//! Constructs an atomic int with the specified initial value
	QMUTEX_INLINEP FXAtomicInt(int i=0) throw() : lock(0), value(i), value2(0) { }
	QMUTEX_INLINEP FXAtomicInt(const FXAtomicInt &o) throw() : lock(0), value(o.value), value2(0) { }
	QMUTEX_INLINEP FXAtomicInt &operator=(const FXAtomicInt &o) throw() { lock=0; value=o.value; return *this; }
	//! Returns the current value
	QMUTEX_INLINEP operator int() const throw();
	//! Atomically sets the value, bypassing caches and write buffers
	QMUTEX_INLINEP int operator=(int i) throw();
	//! Atomically post-increments the value
	QMUTEX_INLINEP int operator++(int) throw();
	//! Atomically pre-increments the value
	QMUTEX_INLINEP int operator++() throw();
	/*! Atomically fast pre-increments the value, returning a negative number if
	result is negative, zero if result is zero and a positive number of result is
	positive */
	QMUTEX_INLINEP int fastinc() throw();
	//! Atomically adds an amount to the value
	QMUTEX_INLINEP int operator+=(int i) throw();
	//! Atomically post-decrements the value
	QMUTEX_INLINEP int operator--(int) throw();
	//! Atomically pre-decrements the value
	QMUTEX_INLINEP int operator--() throw();
	/*! Atomically fast pre-decrements the value, returning a negative number if
	result is negative, zero if result is zero and a positive number of result is
	positive */
	QMUTEX_INLINEP int fastdec() throw();
	//! Atomically subtracts an amount from the value
	QMUTEX_INLINEP int operator-=(int i) throw();
	//! Atomically swaps \em newval for the int, returning the previous value
	QMUTEX_INLINEP int swap(int newval) throw();
	//! Atomically compares the int to \em compare which if the same, stores \em newval. Returns the previous value.
	QMUTEX_INLINEP int cmpX(int compare, int newval) throw();
};

/*! \class QShrdMemMutex
\brief A very lightweight mutex object to synchronise access to shared memory regions

What goes for FX::QMutex goes for this. However, there are further restrictions: because
inter-process mutex support is not available on all platforms, this object provides
a working alternative based on FX::FXAtomicInt. It however does not invoke kernel waits
so hence your process will waste processor time spinning on the lock if it is held by
some other process. Hence it is <b>very important</b> that your shared memory region
is very very rarely in contention.

One other problem is what happens if the process dies suddenly while holding the lock.
In this situation the lock must be unlocked at best, but at worst the shared memory
state may be damaged and thus all other applications will be adversely affected. This
object times out and gives you the lock anyway after a second by default, but you
must take care in your code to hold this lock for only the very shortest period of
time possible and do nothing which could cause you to fail to unlock it. Look into
FX::FXDoUndo.

Because of all these problems, it is advisable that for anything more than trivial
use of shared memory, another synchronisation method should be used eg; IPC messaging.

QShrdMemMutex can safely be used without construction which makes it very useful
during process initialisation or destruction. If unconstructed it has a state identical
to:
\code
static QShrdMemMutex lock(FXINFINITE);
\endcode
... which is why you should always declare it as above.

As of v0.86, defining FXINLINE_MUTEX_IMPLEMENTATION defines this class inline
to all code including QThread.h. This can cause substantial performance
improvement at the cost of code size.
*/
class QMUTEX_FXAPI QShrdMemMutex
{
	FXAtomicInt lockvar;
	FXuint timeout;		// =0 for infinite, =timeout+1 otherwise
public:
	//! Constructs an instance. Using FXINFINITE means infinite waits
	QMUTEX_INLINEP QShrdMemMutex(FXuint _timeout=1000) : timeout(_timeout+1) { }
	//! Returns the time after which a lock succeeds anyway
	QMUTEX_INLINEP FXuint timeOut() const throw() { return timeout-1; }
	//! Sets the time after which a lock succeeds anyway
	QMUTEX_INLINEP void setTimeOut(FXuint to) throw() { timeout=to+1; }
	//! Locks the shared memory mutex
	QMUTEX_INLINEP void lock();
	//! Unlocks the shared memory mutex
	QMUTEX_INLINEP void unlock() throw() { lockvar=0; }
	//! Tries the shared memory mutex
	QMUTEX_INLINEP bool tryLock() throw();
};

/*! \class QMutex
\brief A MUTual EXclusion object to synchronise threads accessing the same data (Qt compatible)

Basically, when more than one thing uses data at the same time, corruption inevitably
results because no operation on data is \em atomic eg; a=a+1 may compile into three
instructions on some processors: load a; add one to a; store a; and hence if another
thread is doing exactly the same operation at the same time, one will be added and
not two as is correct. Even if an add operation is atomic on your processor,
calculating an inverse square root won't be.

Because of this, \em threadsafe code must synchronise access to any data accessed
by more than one thread ie; permit only one thing to use the data at a time. On
Win32/64, such a thing is called a <i>critical section</i> (NOT a mutex - they have
much more functionality and are much much slower) and under POSIX such a thing is
called a \em mutex.

Generally QMutex is used by compositing it into your class eg;
\code
class MyProtectedClass : public QMutex
{
	MyProtectedClass() : QMutex()
	{
		...
	}
};
\endcode
The API of QMutex has been designed to be highly non-intrusive so that it may
be subclassed easily.

<h3>Performance</h3>
Mutexes are the single most used item in multithreaded programming and hence
their performance is extremely important. Unfortunately, POSIX threads
implementation of mutexs is mostly woeful, and critical section objects on
Win32 are less than optimal as well.

Hence on Intel x86 architectures, or on all MS Windows, QMutex is implemented
directly using inlined FXAtomicInt operations. This solution has proved in
benchmarks to be as optimal as it gets for the supported platforms. On those
POSIX platforms which allow POSIX semaphores to be used from within signal/cleanup
handlers, interaction with the kernel can be absolutely minimised.

Worst case scenario falls back to the mutex provided by POSIX threads, so
whatever happens you get correct operation.

<h3>Spin counts</h3>
Where implementation is direct (see above), a default <i>spin count</i> of 4000 applies
(a spin count is how often a lock() retries acquisition before invoking a kernel
wait). The value of this is important because it can make a huge difference to performance
and for you to choose it correctly you need to bear some factors in mind.

Kernel waits are extremely expensive - putting a thread to sleep costs tens
of thousands of cycles and thousands to wake it up. If a mutex is rarely
in contention (ie; more than one thing claiming it at once) then it makes sense to retry
the acquisition knowing that the chances are the current holder will relinquish
it very soon.

The obvious crux is that spinning takes up processor time to the exclusion of all
else, so too much spinning slows things down. Conversely too little spinning means
too many kernel waits at possibly more than 100,000 cycles on some systems. You
should set your spin count so it takes the same time to make the count as the
average period the lock is held as so the other processor will have released the
lock before the count is completed.

\note QMutex internally sets spin counts to zero on uniprocessor machines. This is
a runtime check performed during construction and is overriden by setSpinCount() (as
there are some uses for spin counts occasionally on uniprocessor machines).

<h3>Debugging:</h3>
Finding where data is being altered without holding a lock can be difficult - this
is where setting the debug yield flag using QMutex::setMutexDebugYield() can be
useful. When set the mutex calls FX::QThread::yield() directly after any lock
thus ensuring that anything else wanting that lock will go to sleep on it so you
can inspect the situation using the debugger. Obviously this can severely degrade
performance so only set for certain periods around the area you require it.

<h3>Performance:</h3>
Now that QMutex has been hand tuned and optimised, we can present some figures
(which were valid for v0.85):
\code
                        FXAtomicInt   QMutex
    SMP Build, 1 thread :  51203277  18389113
    SMP Build, 2 threads:   4793978   5337603
Non-SMP Build, 1 thread : 103305785  27352297
Non-SMP Build, 2 threads:  54929964  10978153
\endcode
These are the results from TestMutex in the TestSuite directory. As you can see,
atomic int operations upon which QMutex is very reliant nosedive when the
location is in contention with another thread (by around 10.7 times). Indeed, the
very fact that the mutex which does multiple atomic int operations per lock &
unlock is faster than a simple increment shows how the CPU isn't optimised for
running locked data operations.

However we can see that a mutex's performance when free of contention is some 3.4
times faster than when it is in contention. This is in fact very good and quite
superior to other fast mutex implementations. On non-SMP builds, it's a factor of
2.5 but then atomic int operations are around precisely half as you'd expect with
two threads.

Non-SMP builds' QMutex is between 48% and 106% faster than the SMP build's.

As of v0.86, defining FXINLINE_MUTEX_IMPLEMENTATION defines this class inline
to all code including QThread.h. This can cause substantial performance
improvement at the cost of code size.

\sa QRWMutex
*/
struct QMutexPrivate;
class QMUTEX_FXAPI QMutex
{
	QMutexPrivate *p;
	QMutex(const QMutex &);
	QMutex &operator=(const QMutex &);
	friend class QRWMutex;
	QMUTEX_INLINEI FXDLLLOCAL void int_lock();
	QMUTEX_INLINEI FXDLLLOCAL void int_unlock();
public:
	//! Constructs a mutex with the given spin count
	QMUTEX_INLINEP QMutex(FXuint spinCount=4000);
	QMUTEX_INLINEP ~QMutex();
	//! Returns if the mutex is locked
	QMUTEX_INLINEP bool isLocked() const;
	/*! \overload bool QMutex::isLocked()
	For FOX compatibility
	*/
	QMUTEX_INLINEP FXbool locked() const { return (FXbool) isLocked(); }
	//! Returns the current spin count
	QMUTEX_INLINEP FXuint spinCount() const;
	//! Sets the spin count
	QMUTEX_INLINEP void setSpinCount(FXuint c);
	/*! If free, claims the mutex and returns immediately. If not, waits until
	the current holder releases it and then claims it before returning
	\warning Do not use this directly unless \b absolutely necessary. Use QMtxHold instead.
	*/
	QMUTEX_INLINEP void lock();
	/*! Releases the mutex for other to claim. Must be called as many times
	as lock() is called
	\warning Do not use this directly unless \b absolutely necessary. Use QMtxHold instead.
	*/
	QMUTEX_INLINEP void unlock();
	/*! Claims the mutex if free and returns true. If already taken, immediately
	returns false without waiting
	*/
	QMUTEX_INLINEP bool tryLock();
	/*! \overload bool QMutex::tryLock()
	For FOX compatibility
	*/
	QMUTEX_INLINEP FXbool trylock() { return (FXbool) tryLock(); }
	/*! Sets the debugging flag for mutexs in this process. See the main description
	above */
	QMUTEX_INLINEP static bool setMutexDebugYield(bool v);
};


/*! \class QWaitCondition
\brief An OS/2 style event object (Qt compatible)

This wait condition object functions very similarly to an event object in OS/2 or Win32. Essentially, this
type of object acts like a soft drink dispenser - it holds the cans (threads) in a FIFO stack from
which inseting a coin (wakeOne()) takes the bottom one out and dispenses it. You can also
dispense all the cans at once (wakeAll()) which permits any new cans entered in the top to be
dispensed straight away until reset() is called.

To use OS/2 speak, this object is \em signalled when it permits any thread trying to wait on
to continue straight away. This continues until the object is reset to unsignalled, whereupon
any waits will halt either indefinitely or the timeout period specified.

Unlike OS/2 or Win32, you are not guaranteed that the first thread to wait on
the object \b will be the first thread released with wakeOne(). Due to POSIX limitations, the
actual thread released will be random.

\note On all platforms QWaitCondition will cancel the thread during a wait if a cancellation has
been requested. Please consult the documentation for QThread.

\warning POSIX does not guarantee that POSIX wait conditions can be used from cleanup/signal
handlers (most annoyingly). Indeed FreeBSD aborts the process if you try which is even more
serious, so QWaitCondition::wait() issues a warning on all platforms in debug mode.
*/
class QWaitConditionPrivate;
class FXAPI QWaitCondition
{
	QWaitConditionPrivate *p;
	bool isSignalled, isAutoReset;
	QWaitCondition(const QWaitCondition &);
	QWaitCondition &operator=(const QWaitCondition &);
public:
	/*! \param autoreset When true, object automatically reset()'s itself after releasing all threads with a wakeAll()
	\param initialstate When true, object starts life signalled
	*/
	QWaitCondition(bool autoreset=false, bool initialstate=false);
	~QWaitCondition();
	/*! \return True if wait succeeded, false if it timed out
	Waits \em time milliseconds for the object to become signalled. FXINFINITE specifies wait forever.
	*/
	bool wait(FXuint time=FXINFINITE) ;
	//! Wakes a random thread waiting on this object. Never signals object.
	void wakeOne () ;
	/*! Signals object & wakes all threads waiting. If this is an autoreset object,
	object is reset after at least one thread is woken ie; if no threads are waiting
	when wakeAll() is called, object will remain signalled until one thread has
	tried to wait whereupon it will be permitted to continue - however subsequent
	waits will block.
	*/
	void wakeAll () ;
	//! Resets (unsignals) object. Any threads now trying to wait will be put to sleep.
	void reset();
	//! Returns if the object is currently signalled or not
	bool signalled() const { return isSignalled; }
};

/*! \class QRWMutex
\brief A mutex object permitting multiple reads simultaneously

One of these functions quite similarly to a QMutex, except that when you lock() it you can
specify if you want write access in addition to read access. If you do request write access,
only one thread (you) is permitted to use the data. If you want merely read access, anyone
else requesting only read access will be permitted to read without blocking.

QRWMutex is useful when you have many threads mostly reading the same data because it can
in some cases vastly improve performance. It does however arrive with a greater cost
than a QMutex which is in the order of fractions of a microsecond - a QRWMutex will take
possibly up to four to five times longer.

This implementation fully supports recursion of both read and write locks and furthermore
supports you obtaining a read lock first and then a subsequent write lock on top of it, or
obtaining a write lock and then subsequent read lock. It uses an intelligent algorithm
to stall any new readers or writers except those already with a read or write lock who
get preference. Write requests get preference over read requests - a future version
may be configurable to be the opposite.

One inescapable caveat in fully recursive read write mutexes is what happens when
two threads with existing read locks both claim the write lock at the same time. Most
implementations lock up or define it as an impossible state you should not permit -
our implementation instead serialises each thread through the write locked section. Obviously,
this means across the claiming of the write lock when a read lock is already held,
<u>the data may change</u> - and if you do not take care, pointers held from the read
lock period may become dangling or otherwise pre-write lock references become invalid. lock()
returns true when this happens (and the FX::QMtxHold::lockLost() method returns true)
so you can detect when you need to reestablish your data state.

Because this creates hard to detect bugs, let me emphasise here from my own bitter
experience - <b>always assume when requesting write access when already holding a read
lock that you have effectively unlocked completely</b> ie; retest entries have not
been since added to a list, or assume pointers or references remain valid etc.
*/
class QRWMutexPrivate;
class QMtxHold;
class FXAPI QRWMutex
{
	QRWMutexPrivate *p;
	QRWMutex(const QRWMutex &);
	QRWMutex &operator=(const QRWMutex &);
public:
	enum LockedState
	{
		Unlocked=0,
		ReadOnly,
		ReadWrite
	};
private:
	LockedState lockedstate;
public:
	QRWMutex();
	~QRWMutex();
	//! Returns the spin count of the underlying QMutex
	FXuint spinCount() const;
	//! Sets the spin count of the underlying QMutex. It would be rare you'd want to set this as the default is about right.
	void setSpinCount(FXuint c);
	/*! \return True if nested write lock request while read lock was held resulted
	in unlock for other thread (ie; reread all your pointers etc)
	\param write True if you wish to write as well as read

	Claims the read/write mutex for access. Requesting read access will always
	return immediately if only other claimants have requested read access. You
	may nest lock()'s, for example
	\code
	lock(false);
	...
	lock(true);
	...
	unlock(true);
	unlock(false);
	\endcode
	This facility to lock for read, then if necessary a write is quite useful.
	\warning Do not use this directly unless \b absolutely necessary. Use QMtxHold instead.
	*/
	bool lock(bool write=true);
	/*! Release the mutex from its previous lock(). You must use the same parameter
	as in the most recent lock().
	\warning Do not use this directly unless \b absolutely necessary. Use QMtxHold instead.
    */
	void unlock(bool write=true);
	//! Returns the current state of the read/write mutex.
	LockedState isLocked() const { return lockedstate; }
	/*! Claims the mutex if free and returns true. If already taken, immediately
	returns false without waiting
	*/
	bool trylock(bool write);
private:
	//! For future expansion
	bool prefersReaders() const;
	//! For future expansion
	void setReaderPreference(bool);
	inline FXDLLLOCAL bool _lock(QMtxHold &h, bool write);
};

/*! \class QMtxHold
\brief Helper class for using mutexes

QMtxHold simply takes a FX::QMutex, FX::QRWMutex or FX::QShrdMemMutex (or pointers to the same)
as an argument to its constructor and locks the mutex for you. Hence then when QMtxHold is destructed,
the mutex is guaranteed to be freed.

A rarely used other possibility is to pass QMtxHold::UnlockAndRelock as the second argument to the
constructor - this unlocks the mutex and relocks it on destruction.
*/
class QMtxHold
{
	FXuint flags;
	bool locklost;
	union {
		QMutex *mutex;
		QRWMutex *rwmutex;
		QShrdMemMutex *shrdmutex;
	};

	QMtxHold(const QMtxHold &);
	QMtxHold &operator=(const QMtxHold &);
public:
	//! Enumerates kinds of lock hold
	enum Flags
	{
		LockAndUnlock=0,		//!< Locks on construction and unlocks on destruction (the default)
		UnlockAndRelock=1,		//!< The opposite (very rarely used)
		AcceptNullMutex=2,		//!< Accepts a null mutex pointer

		IsShrdMutex=(1<<26),
		IsRWMutex=(1<<27),
		IsLocked=(1<<28),
		IsRWMutexWrite=(1<<29)
	};
	//! Constructs an instance holding the lock to mutex \em m
	FXFORCEINLINE QMtxHold(const QMutex *m, FXuint _flags=LockAndUnlock)
		: flags(_flags), locklost(false)
	{
		mutex=const_cast<QMutex *>(m);
		if((flags & AcceptNullMutex) && !mutex) return;
		if(flags & UnlockAndRelock) mutex->unlock(); else mutex->lock();
		flags|=IsLocked;
	}
	//! \overload
	FXFORCEINLINE QMtxHold(const QMutex &m, FXuint _flags=LockAndUnlock)
		: flags(_flags), locklost(false)
	{
		mutex=const_cast<QMutex *>(&m);
		if(flags & UnlockAndRelock) mutex->unlock(); else mutex->lock();
		flags|=IsLocked;
	}
	//! Constructs and instance holding the lock to read/write mutex \em m
	FXFORCEINLINE QMtxHold(const QRWMutex *m, bool write=true, FXuint _flags=LockAndUnlock)
		: flags(_flags|IsRWMutex|(write ? IsRWMutexWrite : 0)), locklost(false)
	{
		rwmutex=const_cast<QRWMutex *>(m);
		if((flags & AcceptNullMutex) && !rwmutex) return;
		locklost=rwmutex->lock(!!(flags & IsRWMutexWrite));
		flags|=IsLocked;
	}
	//! \overload
	FXFORCEINLINE QMtxHold(const QRWMutex &m, bool write=true, FXuint _flags=LockAndUnlock)
		: flags(_flags|IsRWMutex|(write ? IsRWMutexWrite : 0)), locklost(false)
	{
		rwmutex=const_cast<QRWMutex *>(&m);
		locklost=rwmutex->lock(!!(flags & IsRWMutexWrite));
		flags|=IsLocked;
	}
	//! Constructs an instance holding the lock to mutex \em m
	FXFORCEINLINE QMtxHold(const QShrdMemMutex *m, FXuint _flags=LockAndUnlock)
		: flags(_flags|IsShrdMutex), locklost(false)
	{
		shrdmutex=const_cast<QShrdMemMutex *>(m);
		if((flags & AcceptNullMutex) && !shrdmutex) return;
		if(flags & UnlockAndRelock) shrdmutex->unlock(); else shrdmutex->lock();
		flags|=IsLocked;
	}
	//! \overload
	FXFORCEINLINE QMtxHold(const QShrdMemMutex &m, FXuint _flags=LockAndUnlock)
		: flags(_flags|IsShrdMutex), locklost(false)
	{
		shrdmutex=const_cast<QShrdMemMutex *>(&m);
		if(flags & UnlockAndRelock) shrdmutex->unlock(); else shrdmutex->lock();
		flags|=IsLocked;
	}
	//! Used to unlock the held mutex earlier than destruction.
	FXFORCEINLINE void unlock()
	{
		if((flags & AcceptNullMutex) && !mutex) return;
		if(flags & IsLocked)
		{
			if(flags & IsRWMutex)
				rwmutex->unlock(!!(flags & IsRWMutexWrite));
			else if(flags & IsShrdMutex)
			{
				if(flags & UnlockAndRelock) shrdmutex->lock(); else shrdmutex->unlock();
			}
			else
			{
				if(flags & UnlockAndRelock) mutex->lock(); else mutex->unlock();
			}
			flags&=~IsLocked;
		}
	}
	//! Used to relock a previously unlocked held mutex
	FXFORCEINLINE void relock()
	{
		if((flags & AcceptNullMutex) && !mutex && !rwmutex) return;
		if(!(flags & IsLocked))
		{
			if(flags & IsRWMutex)
				locklost=rwmutex->lock(!!(flags & IsRWMutexWrite));
			else if(flags & IsShrdMutex)
			{
				if(flags & UnlockAndRelock) shrdmutex->unlock(); else shrdmutex->lock();
			}
			else
			{
				if(flags & UnlockAndRelock) mutex->unlock(); else mutex->lock();
			}
			flags|=IsLocked;
		}
	}
	//! Returns true if when during a read-to-write lock transition the lock was lost
	FXFORCEINLINE bool lockLost() const { return locklost; }
	FXFORCEINLINE ~QMtxHold() { unlock(); }
};

// Needs to go here as QMtxHold needs to be defined
namespace Generic {

/*! \struct lockedAccessor
\brief Holds a lock after a function exits

As anyone familiar with multithreaded programming is aware, often one must make a choice
between efficiency and thread-safety when writing member functions of a lockable class eg;
\code
QStringList Class::foo() const
{
	QMtxHold h(this);
	return p->mystrings();
}
\endcode
Above, one sees a whole copy of an entire FX::QStringList because you cannot return a
reference for fear of the list being changed by another thread while the lock isn't held.
Sometimes this is desirable, but what if one merely wanted to check if the list were empty?
Then an entire copy is being performed just for it to be thrown away.

lockedAccessor solves this problem by extending the hold of the lock past the return of
the member function:
\code
Generic::lockedAccessor<const QStringList> Class::foo() const
{
	Generic::lockedAccessor<const QStringList> ret(p->mystrings(), this);
	// You can do stuff to ret as though it were p->mystrings()
	return ret;
}
\endcode
This handles simple cases like \c class->foo().empty(), but you still must be careful -
if you write <tt>QStringList list(class->foo());</tt> then you are safe, but if you
use the return directly then you can lose the lock inadvertently.
*/
template<class type> struct lockedAccessor
{
private:
	QMtxHold *h;
	type &val;
public:
	//! Constructs an instance reflecting \em _val while locking \em m
	lockedAccessor(type &_val, const QMutex *m) : val(_val), h(0) { if(m) { FXERRHM(h=new QMtxHold(m)); } }
	//! \overload
	lockedAccessor(type &_val, const QMutex &m) : val(_val), h(0) { FXERRHM(h=new QMtxHold(m)); }
	//! \overload
	lockedAccessor(type &_val, const QRWMutex *m) : val(_val), h(0) { if(m) { FXERRHM(h=new QMtxHold(m)); } }
	//! \overload
	lockedAccessor(type &_val, const QRWMutex &m) : val(_val), h(0) { FXERRHM(h=new QMtxHold(m)); }
#ifndef HAVE_CPP0XRVALUEREFS
#ifdef HAVE_CONSTTEMPORARIES
	lockedAccessor(const lockedAccessor &_o) : h(_o.h), val(_o.val)
	{
		lockedAccessor &o=const_cast<lockedAccessor &>(_o);
		o.h=0;
	}
	lockedAccessor &operator=(const lockedAccessor &_o)
	{
		lockedAccessor &o=const_cast<lockedAccessor &>(_o);
		this->~lockedAccessor();
		return *new(this) lockedAccessor(o);
	}
#else
	lockedAccessor(lockedAccessor &o) : h(o.h), val(o.val)
	{
		o.h=0;
	}
	lockedAccessor &operator=(lockedAccessor &o)
	{
		this->~lockedAccessor();
		return *new(this) lockedAccessor(o);
	}
#endif
#else
private:
	lockedAccessor(const lockedAccessor &);	// disable copy constructor
	lockedAccessor &operator=(const lockedAccessor &_o);
public:
	lockedAccessor(lockedAccessor &&o) : h(o.h), val(o.val)
	{
		o.h=0;
	}
	lockedAccessor &&operator=(lockedAccessor &&o)
	{
		this->~lockedAccessor();
		return *new(this) lockedAccessor(o);
	}
#endif
	~lockedAccessor()
	{
		FXDELETE(h);
	}

	//! Returns the value
	type &operator*() const { return const_cast<type &>(val); }
	//! Returns the value
	type *operator->() const { return const_cast<type *>(&val); }
};
} // namespace

/*! \class FXZeroedWait
\brief A zero activated wait condition

A very useful and commonly used form of wait condition is one combined with a count
that signals when it reaches zero. You can think of it as similar to an inverse
semaphore, but obviously it's not an exact fit because unlike semaphores there is
no limit to the count.

By default, there is no checking of the count value for speed - however, you may
enable checking for the count falling below zero. The exception is thrown with
FXERRH_ISDEBUG and so is not language translated for the user's benefit.
*/
class FXZeroedWait
{
	FXAtomicInt count;
	bool doChecks;
	QWaitCondition wc;
	void handleCount(int nc)
	{
		if(doChecks)
		{
			if(nc<0) FXERRG("Count should not be below zero", 0, FXERRH_ISDEBUG);
		}
		if(nc>0) wc.reset();
		else if(!nc) wc.wakeAll();
	}
public:
	//! Constructs a FXZeroedWait with initial count \em initcount
	FXZeroedWait(int initcount=0) : count(initcount), doChecks(false), wc(false, !initcount) { }
	//! Returns the count
	operator int() const { return count; }
	//! Sets the value of the count, which if zero frees any waiting threads
	int operator=(int i)	{ int c=(count=i);	handleCount(c);		return c; }
	//! Post-increments the count
	int operator++(int)		{ int c=count++;	handleCount(c+1);	return c; }
	//! Pre-increments the count
	int operator++()		{ int c=++count;	handleCount(c);		return c; }
	//! Increments the count by a given amount
	int operator+=(int i)	{ int c=(count+=i); handleCount(c);		return c; }
	//! Post-decrements the count, which if zero frees any waiting threads
	int operator--(int)		{ int c=count--;	handleCount(c-1);	return c; }
	//! Pre-decrements the count, which if zero frees any waiting threads
	int operator--()		{ int c=--count;	handleCount(c);	return c; }
	//! Decrements the count by a given amount, which if zero frees any waiting threads
	int operator-=(int i)	{ int c=(count-=i);	handleCount(c);		return c; }

	//! Waits for the count to become zero, returning true if wait succeeds
	bool wait(FXuint time=FXINFINITE) { return wc.wait(time); }
	//! Sets if run-time checks are performed on the count value
	void setChecks(bool d) { doChecks=d; }
};

/*! \class QThreadLocalStorage
\brief Permits process-global storage local to a thread only

QThreadLocalStorage represents data which is local to a thread only ie; when queryed, the
data returned is the same across any code in the process but changes according to which thread is
currently executing. Multithreading novices may not be able to think of why you would
ever want to use this (given that using the stack is usually sufficient), but experts will
know there are some problems unsolvable without TLS (Thread Local Storage) nor ability to
retrieve a unique thread id.

Tornado implements TLS using QThreadLocalStorageBase which basically permits setting
and retrieving of a <tt>void *</tt>. QThreadLocalStorage is then implemented as a template
over QThreadLocalStorageBase to make it more C++ friendly. To use QThreadLocalStorage,
pass the \b pointer to the type of the data you wish to make thread-local:
\code
class MyLocalData
{
	int foo;
};
static QThreadLocalStorage<MyLocalDataBase *> thrlocaldata;
...
// NOTE: Very very important you ALWAYS initialise on first use with TLS (think about it!)
if(!thrlocaldata) thrlocaldata=new MyLocalData;
...
if(thrlocaldata->foo>8) ...
\endcode
Generally, though not always, you will use TLS as a static mostly-global variable.
\note Ensure you do not forget to release any memory stored into TLS. Have your thread's
cleanup routine perform the appropriate checks or else use QThread::addCleanupCall().
*/
class QThreadLocalStorageBasePrivate;
class FXAPI QThreadLocalStorageBase
{
	QThreadLocalStorageBasePrivate *p;
	QThreadLocalStorageBase(const QThreadLocalStorageBase &);
	QThreadLocalStorageBase &operator=(const QThreadLocalStorageBase &);
public:
	QThreadLocalStorageBase(void *initval=0);
	~QThreadLocalStorageBase();
	//! Sets the TLS value
	void setPtr(void *val);
	//! Retrieves the TLS value
	void *getPtr() const;
};
template<class type> class QThreadLocalStorage : private QThreadLocalStorageBase
{
public:
	//! Constructs the TLS, optionally storing an initial value (for the current thread) immediately
	QThreadLocalStorage(type *initval=0) : QThreadLocalStorageBase(static_cast<void *>(initval)) { }
	/*! Operator overload allowing the pointer to the type represented by
	this object to be set
	*/
	type *operator=(type *a) { setPtr(static_cast<void *>(a)); return a; }
	/*! Operator overload permitting automatic retrieval of the pointer
	to the type represented by this object
	*/
	operator type *() const { return static_cast<type *>(getPtr()); }
	type *operator ->() const { return static_cast<type *>(getPtr()); }
};


/*! \class QThread
\brief The base class for all threads in FOX (Qt compatible)

QThread is designed to be one of the most comprehensive C++ thread classes around. Some of its many
features include:
\li You can name your threads. This may seem trivial, but we've found it to be a real boon for debugging
\li You can define a POSIX thread cleanup handler to be called upon thread cancellation
\li id() returns something unique on all platforms portably
\li Facilities for Thread Local Storage (TLS) is provided by QThreadLocalStorage
\li A default FXException handler is provided as standard which routes all exceptions to somewhere
capable of displaying GUI stuff where it is then FXERRH_REPORT()'ed
\li Threads can self-destruct using setAutoDelete().
\li You can set thread priority in a portable (though not very reliable) fashion.
\li You can have code called on thread creation plus thread death

<h3>Writing easily cancellable thread code</h3>
On POSIX Unix, operations which waits on the kernel except for mutexes can never return and
instead jump directly to the cleanup() routine (precisely which calls are subject to this vary
substantially between Unices but it's best to assume
that all wait operations are vulnerable - this includes printing things to \c stdio like using
\c fxmessage). Because of this, stack-constructed objects will not be unwound and hence there
will be memory and resource leaks.

To solve this, place \b all data used by your thread during its execution within its private member
variables. Use a system of checking pointers for zero to determine if they need to be delete'd in your
cleanup(). cleanup() is guaranteed to be called irrespective of how the thread ends - naturally, or
by request. Do \b not hold any mutexs as they will remain locked by a dead thread (you shouldn't
hold them anyway as it causes deadlocks).

Obviously one thing you cannot protect against is calling other code which \em does create objects
on the stack and waits on the kernel. For these situations alone you should use disableTermination()
and enableTermination() around the appropriate calls. These two functions can be nested ie; you must
call enableTermination() as many times as disableTermination() for termination to be actually
reenabled. See FX::QThread_DTHold.

My suggestion is to try and keep as much code cancellable as possible. Doing this pays off when
your thread is asked to quit and the user isn't left with a thread that won't quit for five
or ten seconds. While it may seem annoying, you quickly find yourself automatically designing your
code to allow it.

If your thread is too complex to allow arbitrary cancellation like above or you find yourself
constantly disabling termination, firstly consider a redesign. If you can't do that, the traditional
OS/2 or Win32 approach of creating a flag (consider FXAtomicInt as a thread-safe flag) that the
thread polls can work well.

\note On Win32 there is no such thing as cleanup handlers, so QThread emulates them along with special support
by various other classes such as QWaitCondition, QPipe and QBlkSocket. Some FOX file i/o functions
do not have this special support, so it is strongly recommended that for identical cross-platform
behaviour you regularly call checkForTerminate() which on Win32 is implemented rather brutally
by calling the cleanup handler and immediately exiting the thread.

\note On Apple MacOS X, curiously the select() call does not permit thread cancellation despite
being required to do so by the POSIX threads spec. Hence special support code is compiled in
on MacOS X to emulate a similar behaviour. This comes with a cost of an extra two file handles
open per thread object.

\warning Cleanup handlers are invoked on most POSIX platforms via a synchronous signal. This
implies that all the FXRESTRICTions placed on signal handling code also apply to cleanup code -
the most annoying is that QWaitCondition is totally unusable.

<h3>Self-destruction:</h3>
In some circumstances, you may wish to spin off self-managed thread objects which perform their
function before self-destructing when they determine they are no longer necessary. You can not
and \b must not use <tt>delete this</tt> because during destruction the cleanup handler is
invoked by POSIX threads thus reentering object destruction and crashing the program.

Instead, use setAutoDelete() which has QThread safely destroy the object after POSIX threads
has been moved out of the way. Any attempts to <tt>delete this</tt> should be trapped as a
fatal exception.

<h3>Stack sizes:</h3>
Due to some POSIX architectures having ludicrously small default thread stack sizes (eg;
FreeBSD with 64Kb), as of v0.75 TnFOX enforces a default of 512Kb. This may be too little
for your requirements but similarly, a stack of 1Mb can lead to premature virtual address
space exhaustion on 32 bit architectures. Furthermore some systems treat custom stack sizes
specially, causing degraded performance so you can still specify zero to have the system
default. Lastly, note than on Win32 the PE binary's stack size setting is the smallest size
possible - any value specified here smaller than that will be rounded up.

<h3>Thread location:</h3>
On some systems you can choose which scheduler runs your thread, the kernel or the process.
If the kernel, it usually means the thread can run on different processors whereas if the
process, it stays local but with lower overheads. Even if you specify one or the other, it
may be ignored according to what the host OS supports.

In general if your thread doesn't do much heavy processing and spends a lot of time waiting
on other threads, it should be in-process so the threading implementation can avoid
expensive calls into the kernel.

<h3>Signals (POSIX only):</h3>
Signals are a real hash on POSIX threads. Each thread maintains its own signal mask which
determines what signals it can receive. Handlers are installed on a per-process basis and
which thread installed the handler makes no difference. Therefore if two threads are running
two pipes and a \c SIGPIPE is generated, how can the handler know which pipe it refers to?

TnFOX code makes the assumption that a signal is delivered to the thread which caused it
unless it's a process-wide signal like \c SIGTERM. This is not at all guaranteed and so
things will break on some POSIX platforms - however the above situation is unresolvable
without this behaviour, so I'm hoping that most POSIX implementations will do as above (Linux and FreeBSD do).
Certainly for \c SIGPIPE itself it's logical that most implementations of \c write()
raise the signal there & then so QPipe's assumption will be correct.

One major problem is if you are modifying data which a signal handler also modifies.
If the signal is raised right in the middle of your changes then the handler will
also get the lock (as it recurses) and thus corrupt your data. To prevent this
instantiate a FX::QThread_DisableSignals around the critical code.

FXProcess sets up most of the fatal signals to call an internal handler which performs
the fatal exit upcall before printing some info to \c stderr and exiting. This handler
has been designed to be called in the context of any thread at all.

<h3>Qt</h3>
You may notice an uncanny similarity with QThread in Qt. This is because QThread was originally
a drop-in superset replacement for QThread.

\sa QThread_DTHold
*/
class QThreadPrivate;
class FXAPI QThread
{
	friend class QThreadPrivate;
	friend class FXPrimaryThread;
	const char *myname;
	FXuint magic;
	QThreadPrivate *p;
	bool isRunning, isFinished, isInCleanup;
	int termdisablecnt;
	QThread(const QThread &);
	QThread &operator=(const QThread &);
public:
	//! Specifies where this thread shall run
	enum ThreadScheduler
	{
		Auto=0,		//! Place wherever the system feels is best
		InProcess,	//! Try to run in-process
		InKernel	//! Try to run in-kernel
	};
	/*! Constructs a thread object named \em _name. The thread is constructed in the calling thread's
	context and does not actually run the new thread until you call start()
	*/
	QThread(const char *name=0, bool autodelete=false, FXuval stacksize=524288/*512Kb*/, ThreadScheduler schedloc=Auto);
	//! Destructs the thread. Causes an exception if the thread is still running
	virtual ~QThread();
	//! Returns the name of the thread
	const char *name() const throw() { return myname; }
	//! Returns the stack size that the thread is given
	FXuval stackSize() const;
	/*! Sets the stack size that the thread shall use (set before calling start())
	which must be a multiple of FXProcess::pageSize(). Setting zero causes system
	default to be used (usually 1 or 2Mb).
	\note Most systems cannot automatically extend their stack when it runs out.
	This I personally find amazing given Acorn RISC-OS could do it happily fifteen
	years ago but I'd imagine on 64 bit memory architectures it won't matter again
	for another fifteen years or so.
	*/
	void setStackSize(FXuval newsize);
	//! Returns where this thread should run
	ThreadScheduler threadLocation() const;
	//! Sets where this thread should run (set before starting)
	void setThreadLocation(ThreadScheduler threadloc);

	/*! \param time Time in milliseconds to wait. Use FXINFINITE to wait forever
	\return True if thread finished in time alloted, false if wait timed out

	Waits for the thread to finish. A thread waiting on itself will never succeed.
	\warning On POSIX Unix, when specified with something other than FXINFINITE, if the thread died in a way that
	didn't permit QThread to be notified then this may wait forever
	*/
	bool wait(FXuint time=FXINFINITE);
	/*! \param waitTillStarted True if you want the call to wait until the thread is running before returning

	Call this to start the separate parallel execution of this thread object from its run(). Remember
	never to access any data in the QThread object after calling this method without first synchronising
	access. Our suggestion is to also subclass QMutex so then you can simply use lock() and unlock()
	(preferably via QMtxHold) around every access.
	*/
	void start(bool waitTillStarted=false) ;
	//! Returns true if the thread has run and finished
	bool finished () const  throw(){ return isFinished; }
	//! Returns true if the thread is running right now
	bool running () const  throw(){ return isRunning; }
	//! Returns true if the thread is in the process of finishing (in its cleanup handler)
	bool inCleanup() const  throw(){ return isInCleanup; }
	//! Returns true if the thread has been asked to be cancelled
	bool isBeingCancelled() const throw();
	//! Returns true if this object is valid (ie; not deleted)
	bool isValid() const throw();
	/*! Sets if this thread object deletes itself when the thread terminates. Returns the former setting
	\warning You \b must thus allocate the thread object using <tt>new</tt>
	*/
	bool setAutoDelete(bool doso) throw();
	/*! Any other thread may call this to request that this thread object finish at the earliest possible
	moment. This may be extremely inconvenient because under Unix, any system call which involves a wait
	may jump directly to the cleanup routine. On Win32, this must be emulated using regular polls to
	checkForTerminate() which all TnFOX classes involving waits do for you. Note that this call does
	not wait for the thread to terminate, this can be done using wait()
	\sa Detailed Description above
	*/
	void requestTermination();
	//! Returns a unique number identifying this thread within this kernel
	static FXulong id() throw();
	//! Returns a unique number identifying the thread represented by this instance
	FXulong myId() const;
	/*! Returns the currently executing QThread object. Note that the primary thread
	(ie; the one main() was called by first thing) correctly returns a pointer to an internal thread object.
	\warning All QThread usage is non-trivial and thus should not be used during static data initialisation
	and destruction. I've put in an \c assert() to help you catch these in debug mode.
	*/
	static QThread *current();
	//! Returns the primary thread object in the process
	static QThread *primaryThread() throw();
	/*! Returns the QThread which created this thread. Is zero if a non-QThread thread
	created this thread.
	\warning The pointer returned by this function will be invalid if the creating thread
	has since been deleted. Use isValid() to determine any returned pointer's validity.
	*/
	QThread *creator() const;
	/*! Returns the scheduling priority of the thread where -127 is lowest priority
	and 128 is maximum. 0 always equals system default ie; normal.
	\sa setPriority()
	*/
	signed char priority() const;
	/*! Sets the scheduling priority of the thread which can be from -127 to +128. Setting 0 always sets system default.
	/note priority() may not return the value you set if the granularity available on the
	particular system isn't sufficient - and on some systems there are as little as three priorities
	*/
	void setPriority(signed char pri);
	//! Returns the processor affinity of the thread
	FXulong processorAffinity() const;
	/*! Sets the processor affinity of the thread with \em recursive meaning to set
	the same for all threads this thread starts. (FXulong)-1 means all processors.
	*/
	void setProcessorAffinity(FXulong mask=(FXulong)-1, bool recursive=false);
	//! Makes the current thread sleep for \em secs seconds
	static void sleep(FXuint secs);
	//! Makes the current thread sleep for \em millisecs milliseconds
	static void msleep(FXuint millisecs);
	//! Yields the remainder of the current thread's time slice
	static void yield();
	/*! Called at the end when you have enabled self destruction on thread exit using
	setAutoDelete(true). Default implementation simply does \c delete \c this.
	*/
	virtual void selfDestruct() { delete this; }
	//! Used by a thread to exit early with return code \em retcode
	static void exit (void *retcode);
	//! Returns the exit code of the thread (if possible eg; self-destructing threads can't)
	void *result() const throw();
	/*! Callable from any thread, this disables termination of the thread if the reference count is
	above zero. Use enableTermination() to decrease the reference count.
	\sa Detailed Description above
	*/
	void disableTermination();
	/*! Checks if termination has been requested - if so, never returns. You may need to call
	this manually at places to ensure Win32 behaves like POSIX. If thread cancellation has
	been disabled, returns true instead.
	*/
	bool checkForTerminate();
	/*! Callable from any thread, this enables termination of the thread if the reference count is
	reaches zero. Use disableTermination() to increase the reference count.
	\sa Detailed Description above
	*/
	void enableTermination();
	//! The specification type of a thread creation upcall vector
	typedef Generic::Functor<Generic::TL::create<void, QThread *>::value> CreationUpcallSpec;
	/*! Registers code to be called when a thread is created. The form of the code is:
	\code
	void function(QThread *);
	void Object::member(QThread *);
	\endcode
	The most common is a member function as this can be in some arbitrary object instance,
	thus making finding your data much easier. If \em inThread is true, the upcall is made just after setting up
	the new thread and just before waking the thread who called start() if it requested to wait.
	If it's false, the upcall is made just after the thread has been successfully started.
	*/
	static void addCreationUpcall(CreationUpcallSpec upcallv, bool inThread=false);
	//! Unregisters a previously installed call upon thread creation.
	static bool removeCreationUpcall(CreationUpcallSpec upcallv);
	/*! Registers a cleanup handler to be called after the thread's cleanup() is called.
	The thread takes ownership of the handler pointer. The \em inThread parameter when true
	means that the handler is called in the context of the thread just before termination -
	note that if it's a self-deleting thread, that means just before deletion. When false,
	the handler is called when the thread object itself is being deleted (again, be cautious
	of self-deleting threads as these delete themselves before terminating).
	*/
	Generic::BoundFunctorV *addCleanupCall(FXAutoPtr<Generic::BoundFunctorV> handler, bool inThread=false);
	/*! Unregisters a previously installed cleanup handler, returning false if not found. You
	get back ownership of the handler pointer */
	bool removeCleanupCall(Generic::BoundFunctorV *handler);

protected:
	//! Reimplement this in your subclass with the code you wish to run in parallel. 
	virtual void run()=0;
	/*! \return The return code you wish for the thread's exit
	Reimplement this in your subclass with the code you wish to run when the thread dies either
	naturally or because of a request by requestTermination()
	*/
	virtual void *cleanup()=0;
public:
	static FXDLLLOCAL void *int_cancelWaiterHandle();
private:
	friend class QThread_DisableSignals;
	static void *int_disableSignals();
	static void int_enableSignals(void *oldmask);
};

/*! \class QThread_DTHold
\brief Wraps thread termination disables in an exception-proof fashion

This little helper class works similarly to FX::QMtxHold in disabling
thread termination on construction and disabling it on destruction.
Since FX::QThread::disableTermination() works by a reference count
this works nicely. You can use undo() and redo() if needed.
*/
class QThread_DTHold : public Generic::DoUndo<QThread, void (QThread::*)(), void (QThread::*)()>
{
public:
	//! Constructs an instance disabling thread termination for thread \em t
	QThread_DTHold(QThread *t=QThread::current())
		: Generic::DoUndo<QThread, void (QThread::*)(), void (QThread::*)()>(t, &QThread::disableTermination, &QThread::enableTermination) { }
};

/*! \class QThread_DisableSignals
\brief Disables signals in an exception-proof fashion

This little helper class works similarly to FX::QMtxHold in disabling
POSIX signals for the calling thread on construction and reenabling them
on destruction.
*/
class QThread_DisableSignals
{
    void *old_mask;
public:
	//! Constructs an instance disabling signals for the current thread
    QThread_DisableSignals() : old_mask(QThread::int_disableSignals()) { }
    ~QThread_DisableSignals() { QThread::int_enableSignals(old_mask); }
};

/*! \class QThreadPool
\brief Provides a pool of worker threads

Thread pools are an old & common invention used to work around the cost of
creating and destroying a thread. They permit you to gain all the
advantages of multi-threaded programming to a fine-grained level
without many of the costs. Thus, they are ideal for when you regularly
have small tasks to be carried out in parallel with the main execution.

Typically to send a job to a thread pool costs barely more than waking
a sleeping thread. If there are no threads available (ie; they're all
processing a job), the job enters a FIFO queue which is emptied as fast
as the worker threads get through them. You furthermore can delay the
execution of a job for an arbitrary number of milliseconds - thus a
timed call dispatcher comes for free with this class.

Jobs can be waited upon for completion & cancelled. FX::FXProcess provides
a process-wide thread pool which is created upon first-use - certain
functionality such as the filing system monitor (FX::FXFSMonitor) use
the process thread pool to run checks as well as dispatch notifications
of changes.

<h3>Usage:</h3>
\code
FXProcess::threadPool().dispatch(Generic::BindFuncN(obj, method, pars));
\endcode

Cancelling jobs with cancel() can get tricky. If it's a simple case of
cancelling it before it has run, all is good and \c Cancelled is returned.
If the job is already running, you can optionally have cancel() wait until
it's returned but in either case \c WasRunning is returned (if the job
completed, the functor is deleted as usual). Be aware that
if your job reschedules itself, cancel() when returning \c WasRunning won't
actually have cancelled the job and you'll need to call it again. Suggested
code is as follows:
\code
while(QThreadPool::WasRunning==threadpool.cancel(job));
\endcode
*/
struct QThreadPoolPrivate;
class FXAPI QThreadPool
{
	QThreadPoolPrivate *p;
	QThreadPool(const QThreadPool &);
	QThreadPool &operator=(const QThreadPool &);
	void startThreads(FXuint newno);
public:
	//! A handle to a job within the thread pool
	typedef void *handle;
	/*! Constructs a thread pool containing \em total threads, the default
	being the number of processors in the local machine. \em dynamic when
	true means create threads on demand up until \em total.
	*/
	QThreadPool(FXuint total=FXProcess::noOfProcessors(), bool dynamic=false);
	~QThreadPool();
	//! Returns the number of threads in total in the pool
	FXuint total() const throw();
	//! Returns the maximum number of threads permitted
	FXuint maximum() const throw();
	//! Returns the number of free threads in the pool
	FXuint free() const throw();
	/*! Sets the number of threads in the pool, starting new ones
	if necessary or retiring free ones. If the pool is dynamic, merely
	sets the maximum permitted.
	*/
	void setTotal(FXuint newno);
	//! Returns if the pool is dynamic
	bool dynamic() const throw();
	//! Sets if the pool is dynamic
	void setDynamic(bool v);
	//! Dispatch Upcall Type
	enum DispatchUpcallType
	{
		PreDispatch=0,			//!< This upcall is occurring predispatch
		PostDispatch=1,			//!< This upcall is occurring postdispatch
		CancelledPreDispatch=2	//!< This upcall is occurring due to a cancellation predispatch
	};
	//! A dispatch upcall
	typedef Generic::Functor<Generic::TL::create<bool, QThreadPool *, handle, DispatchUpcallType>::value> DispatchUpcallSpec;
	/*! Dispatches a worker thread to execute this job, delaying by \em delay
	milliseconds */
	handle dispatch(FXAutoPtr<Generic::BoundFunctorV> code, FXuint delay=0, DispatchUpcallSpec *upcallv=0);
	//! Cancelled state
	enum CancelledState
	{
		NotFound=0,		//!< Job not found
		Cancelled,		//!< Job cancelled before it could run
		WasRunning		//!< Job was already running
	};
	/*! Cancels a previously dispatched job. If the job is currently being executed and
	\em wait is true, waits for that job to complete before returning. */
	CancelledState cancel(handle code, bool wait=true);
	/*! Resets a timed job to a new delay. If the job is currently executing or has finished,
	returns false. */
	bool reset(handle code, FXuint delay);
	//! Waits for a job to complete
	bool wait(handle code, FXuint period=FXINFINITE);
};

} // namespace

#ifndef FXBEING_INCLUDED_BY_QTHREAD
 #ifdef FXINLINE_MUTEX_IMPLEMENTATION
  #define FXBEING_INCLUDED_BY_QTHREAD
  #include "int_QMutexImpl.h"
  #undef FXBEING_INCLUDED_BY_QTHREAD

  // Also undef stuff from QMutexImpl.h
  #undef USE_WINAPI
  #undef USE_OURMUTEX
  #undef USE_X86
  #undef __CLEANUP_C
  #undef MUTEX_USESEMA
 #endif
 #undef QMUTEX_FXAPI
 #undef QMUTEX_INLINE
 #undef QMUTEX_GLOBALS_FXAPI
#endif

#endif

