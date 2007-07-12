#ifdef FX_FOXCOMPAT

/********************************************************************************
*                                                                               *
*                 M u l i t h r e a d i n g   S u p p o r t                     *
*                                                                               *
*********************************************************************************
* Copyright (C) 2004,2006 by Jeroen van der Zijp.   All Rights Reserved.        *
*********************************************************************************
* This library is free software; you can redistribute it and/or                 *
* modify it under the terms of the GNU Lesser General Public                    *
* License as published by the Free Software Foundation; either                  *
* version 2.1 of the License, or (at your option) any later version.            *
*                                                                               *
* This library is distributed in the hope that it will be useful,               *
* but WITHOUT ANY WARRANTY; without even the implied warranty of                *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU             *
* Lesser General Public License for more details.                               *
*                                                                               *
* You should have received a copy of the GNU Lesser General Public              *
* License along with this library; if not, write to the Free Software           *
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.    *
*********************************************************************************
* $Id: FXThread.cpp,v 1.53.2.7 2006/07/28 05:30:49 fox Exp $                    *
********************************************************************************/
#include "xincs.h"
#include "fxver.h"
#include "fxdefs.h"
#include "FXThread.h"
#include "QThread.h"
#include "foxcompatlayer.h"


#ifndef WIN32
#ifdef APPLE
#include <mach/mach_init.h>
#include <mach/semaphore.h>
#include <mach/task.h>
#include <pthread.h>
#else
#include <pthread.h>
#include <semaphore.h>
#endif
#else
#include <process.h>
#endif


/*
  Notes:

  - We have a amorphous blob of memory reserved for the mutex implementation.
    Since we're trying to avoid having to include platform-specific headers
    in application code, we can't easily know how much to allocate for
    pthread_mutex_t [or CRITICAL_SECTION].

  - We don't want to allocate dynamically because of the performance
    issues, and also because obviously, since heap memory is shared between
    threads, a malloc itself involves locking another mutex, leaving a
    potential for an unexpected deadlock.

  - So we just reserve some memory which we will hope to be enough.  If it
    ever turns out its not, the assert should trigger and we'll just have
    to change the source a bit.

  - If you run into this, try to figure out sizeof(pthread_mutex_t) and
    let me know about it (jeroen@fox-toolkit.org).

  - I do recommend running this in debug mode first time around on a
    new platform.

  - Picked unsigned long so as to ensure alignment issues are taken
    care off.

  - I now believe its safe to set tid=0 after run returns; if FXThread
    is destroyed then the execution is stopped immediately; if the thread
    exits, tid is also set to 0.  If the thread is cancelled, tid is also
    set to 0.  In no circumstance I can see is it possible for run() to
    return when FXThread no longer exists.
*/

using namespace FX;


namespace FX {

/*******************************************************************************/

// Unix implementation

#ifndef WIN32


// Initialize mutex
FXMutex::FXMutex(FXbool recursive){
  pthread_mutexattr_t mutexatt;
  // If this fails on your machine, determine what value
  // of sizeof(pthread_mutex_t) is supposed to be on your
  // machine and mail it to: jeroen@fox-toolkit.org!!
  //FXTRACE((150,"sizeof(pthread_mutex_t)=%d\n",sizeof(pthread_mutex_t)));
  FXASSERT(sizeof(data)>=sizeof(pthread_mutex_t));
  pthread_mutexattr_init(&mutexatt);
  pthread_mutexattr_settype(&mutexatt,recursive?PTHREAD_MUTEX_RECURSIVE:PTHREAD_MUTEX_DEFAULT);
  pthread_mutex_init((pthread_mutex_t*)data,&mutexatt);
  pthread_mutexattr_destroy(&mutexatt);
  }


// Lock the mutex
void FXMutex::lock(){
  pthread_mutex_lock((pthread_mutex_t*)data);
  }


// Try lock the mutex
FXbool FXMutex::trylock(){
  return pthread_mutex_trylock((pthread_mutex_t*)data)==0;
  }


// Unlock mutex
void FXMutex::unlock(){
  pthread_mutex_unlock((pthread_mutex_t*)data);
  }


// Test if locked
FXbool FXMutex::locked(){
  if(pthread_mutex_trylock((pthread_mutex_t*)data)==0){
    pthread_mutex_unlock((pthread_mutex_t*)data);
    return false;
    }
  return true;
  }


// Delete mutex
FXMutex::~FXMutex(){
  pthread_mutex_destroy((pthread_mutex_t*)data);
  }


/*******************************************************************************/


#ifdef __APPLE__


// Initialize semaphore
FXSemaphore::FXSemaphore(FXint initial){
  // If this fails on your machine, determine what value
  // of sizeof(MPSemaphoreID*) is supposed to be on your
  // machine and mail it to: jeroen@fox-toolkit.org!!
  //FXTRACE((150,"sizeof(MPSemaphoreID*)=%d\n",sizeof(MPSemaphoreID*)));
  FXASSERT(sizeof(data)>=sizeof(MPSemaphoreID*));
  MPCreateSemaphore(2147483647,initial,(MPSemaphoreID*)data);
  }


// Decrement semaphore
void FXSemaphore::wait(){
  MPWaitOnSemaphore(*((MPSemaphoreID*)data),kDurationForever);
  }


// Decrement semaphore but don't block
FXbool FXSemaphore::trywait(){
  return MPWaitOnSemaphore(*((MPSemaphoreID*)data),kDurationImmediate)==noErr;
  }


// Increment semaphore
void FXSemaphore::post(){
  MPSignalSemaphore(*((MPSemaphoreID*)data));
  }


// Delete semaphore
FXSemaphore::~FXSemaphore(){
  MPDeleteSemaphore(*((MPSemaphoreID*)data));
  }

#else

// Initialize semaphore
FXSemaphore::FXSemaphore(FXint initial){
  // If this fails on your machine, determine what value
  // of sizeof(sem_t) is supposed to be on your
  // machine and mail it to: jeroen@fox-toolkit.org!!
  //FXTRACE((150,"sizeof(sem_t)=%d\n",sizeof(sem_t)));
  FXASSERT(sizeof(data)>=sizeof(sem_t));
  sem_init((sem_t*)data,0,(unsigned int)initial);
  }


// Decrement semaphore
void FXSemaphore::wait(){
  sem_wait((sem_t*)data);
  }


// Decrement semaphore but don't block
FXbool FXSemaphore::trywait(){
  return sem_trywait((sem_t*)data)==0;
  }


// Increment semaphore
void FXSemaphore::post(){
  sem_post((sem_t*)data);
  }


// Delete semaphore
FXSemaphore::~FXSemaphore(){
  sem_destroy((sem_t*)data);
  }

#endif

/*******************************************************************************/


// Initialize condition
FXCondition::FXCondition(){
  // If this fails on your machine, determine what value
  // of sizeof(pthread_cond_t) is supposed to be on your
  // machine and mail it to: jeroen@fox-toolkit.org!!
  //FXTRACE((150,"sizeof(pthread_cond_t)=%d\n",sizeof(pthread_cond_t)));
  FXASSERT(sizeof(data)>=sizeof(pthread_cond_t));
  pthread_cond_init((pthread_cond_t*)data,NULL);
  }


// Wake up one single waiting thread
void FXCondition::signal(){
  pthread_cond_signal((pthread_cond_t*)data);
  }


// Wake up all waiting threads
void FXCondition::broadcast(){
  pthread_cond_broadcast((pthread_cond_t*)data);
  }


// Wait for condition indefinitely
void FXCondition::wait(FXMutex& mtx){
  pthread_cond_wait((pthread_cond_t*)data,(pthread_mutex_t*)mtx.data);
  }


// Wait for condition but fall through after timeout
FXbool FXCondition::wait(FXMutex& mtx,FXlong nsec){
  register int result;
  struct timespec ts;
  ts.tv_sec=nsec/1000000000;
  ts.tv_nsec=nsec%1000000000;
x:result=pthread_cond_timedwait((pthread_cond_t*)data,(pthread_mutex_t*)mtx.data,&ts);
  if(result==EINTR) goto x;
  return result!=ETIMEDOUT;
  }


// Delete condition
FXCondition::~FXCondition(){
  pthread_cond_destroy((pthread_cond_t*)data);
  }



/*******************************************************************************/

// Windows implementation

#else

// Initialize mutex
FXMutex::FXMutex(FXbool){
  // If this fails on your machine, determine what value
  // of sizeof(CRITICAL_SECTION) is supposed to be on your
  // machine and mail it to: jeroen@fox-toolkit.org!!
  //FXTRACE((150,"sizeof(CRITICAL_SECTION)=%d\n",sizeof(CRITICAL_SECTION)));
  FXASSERT(sizeof(data)>=sizeof(CRITICAL_SECTION));
  InitializeCriticalSection((CRITICAL_SECTION*)data);
  }


// Lock the mutex
void FXMutex::lock(){
  EnterCriticalSection((CRITICAL_SECTION*)data);
  }



// Try lock the mutex
FXbool FXMutex::trylock(){
#if(_WIN32_WINNT >= 0x0400)
  return TryEnterCriticalSection((CRITICAL_SECTION*)data)!=0;
#else
  return FALSE;
#endif
  }


// Unlock mutex
void FXMutex::unlock(){
  LeaveCriticalSection((CRITICAL_SECTION*)data);
  }


// Test if locked
FXbool FXMutex::locked(){
#if(_WIN32_WINNT >= 0x0400)
  if(TryEnterCriticalSection((CRITICAL_SECTION*)data)!=0){
    LeaveCriticalSection((CRITICAL_SECTION*)data);
    return false;
    }
#endif
  return true;
  }


// Delete mutex
FXMutex::~FXMutex(){
  DeleteCriticalSection((CRITICAL_SECTION*)data);
  }


/*******************************************************************************/


// Initialize semaphore
FXSemaphore::FXSemaphore(FXint initial){
  data[0]=(FXuval)CreateSemaphore(NULL,initial,0x7fffffff,NULL);
  }


// Decrement semaphore
void FXSemaphore::wait(){
  WaitForSingleObject((HANDLE)data[0],INFINITE);
  }


// Non-blocking semaphore decrement
FXbool FXSemaphore::trywait(){
  return WaitForSingleObject((HANDLE)data[0],0)==WAIT_OBJECT_0;
  }


// Increment semaphore
void FXSemaphore::post(){
  ReleaseSemaphore((HANDLE)data[0],1,NULL);
  }


// Delete semaphore
FXSemaphore::~FXSemaphore(){
  CloseHandle((HANDLE)data[0]);
  }


/*******************************************************************************/


// This is the solution according to Schmidt, the win32-threads
// implementation thereof which is found inside GCC somewhere.
// See: (http://www.cs.wustl.edu/~schmidt/win32-cv-1.html).
//
// Our implementation however initializes the Event objects in
// the constructor, under the assumption that you wouldn't be creating
// a condition object if you weren't planning to use them somewhere.


// Initialize condition
FXCondition::FXCondition(){
  // If this fails on your machine, notify jeroen@fox-toolkit.org!
  FXASSERT(sizeof(data)>=sizeof(CRITICAL_SECTION)+sizeof(HANDLE)+sizeof(HANDLE)+sizeof(FXuval));
  data[0]=(FXuval)CreateEvent(NULL,0,0,NULL);                   // Wakes one, autoreset
  data[1]=(FXuval)CreateEvent(NULL,1,0,NULL);                   // Wakes all, manual reset
  data[2]=0;                                                    // Blocked count
  InitializeCriticalSection((CRITICAL_SECTION*)&data[3]);       // Critical section
  }


// Wake up one single waiting thread
void FXCondition::signal(){
  EnterCriticalSection((CRITICAL_SECTION*)&data[3]);
  int blocked=(data[2]>0);
  LeaveCriticalSection((CRITICAL_SECTION*)&data[3]);
  if(blocked) SetEvent((HANDLE)data[0]);
  }


// Wake up all waiting threads
void FXCondition::broadcast(){
  EnterCriticalSection((CRITICAL_SECTION*)&data[3]);
  int blocked=(data[2]>0);
  LeaveCriticalSection((CRITICAL_SECTION*)&data[3]);
  if(blocked) SetEvent((HANDLE)data[1]);
  }


// Wait
void FXCondition::wait(FXMutex& mtx){
  EnterCriticalSection((CRITICAL_SECTION*)&data[3]);
  data[2]++;
  LeaveCriticalSection((CRITICAL_SECTION*)&data[3]);
  mtx.unlock();
  DWORD result=WaitForMultipleObjects(2,(HANDLE*)data,0,INFINITE);
  EnterCriticalSection((CRITICAL_SECTION*)&data[3]);
  data[2]--;
  int last_waiter=(result==WAIT_OBJECT_0+1)&&(data[2]==0);      // Unblocked by broadcast & no other blocked threads
  LeaveCriticalSection((CRITICAL_SECTION*)&data[3]);
  if(last_waiter) ResetEvent((HANDLE)data[1]);                  // Reset signal
  mtx.lock();
  }


// Wait using single global mutex
FXbool FXCondition::wait(FXMutex& mtx,FXlong nsec){
  EnterCriticalSection((CRITICAL_SECTION*)&data[3]);
  data[2]++;
  LeaveCriticalSection((CRITICAL_SECTION*)&data[3]);
  mtx.unlock();
  nsec-=FXThread::time();
  DWORD result=WaitForMultipleObjects(2,(HANDLE*)data,0,nsec/1000000);
  EnterCriticalSection((CRITICAL_SECTION*)&data[3]);
  data[2]--;
  int last_waiter=(result==WAIT_OBJECT_0+1)&&(data[2]==0);      // Unblocked by broadcast & no other blocked threads
  LeaveCriticalSection((CRITICAL_SECTION*)&data[3]);
  if(last_waiter) ResetEvent((HANDLE)data[1]);                  // Reset signal
  mtx.lock();
  return result!=WAIT_TIMEOUT;
  }


// Delete condition
FXCondition::~FXCondition(){
  CloseHandle((HANDLE)data[0]);
  CloseHandle((HANDLE)data[1]);
  DeleteCriticalSection((CRITICAL_SECTION*)&data[3]);
  }

#endif


/*******************************************************************************/

class QThreadFXThread : public QThread
{
public:
	FXThread *parent;
	FXint retcode;
	QThreadFXThread(FXThread *_parent) : QThread("FOX compatible thread"), parent(_parent), retcode(0) { }
	virtual void run()
	{
		retcode=parent->run();
	}
	virtual void *cleanup()
	{
		return (void *) retcode;
	}
};


// Initialize thread
FXThread::FXThread():r(0){
  FXEXCEPTION_FOXCOMPAT1
  FXERRHM(r=new QThreadFXThread(this));
  FXEXCEPTION_FOXCOMPAT2
  }


// Return thread id of this thread object.
// Purposefully NOT inlined, the tid may be changed by another
// thread and therefore we must force the compiler to fetch
// this value fresh each time it is needed!
FXThreadID FXThread::id() const {
  return (FXThreadID) r->myId();
  }


// Return TRUE if this thread is running
FXbool FXThread::running() const {
  return r->running();
  }


// Start thread; make sure that stacksize >= PTHREAD_STACK_MIN.
// We can't check for it because not all machines have this the
// PTHREAD_STACK_MIN definition.
FXbool FXThread::start(unsigned long stacksize){
  FXbool ret=false;
  FXEXCEPTION_FOXCOMPAT1
  r->setStackSize(stacksize);
  r->start();
  FXEXCEPTION_FOXCOMPAT2
  return ret;
  }


// Suspend calling thread until thread is done
FXbool FXThread::join(FXint& code){
  FXbool ret=false;
  FXEXCEPTION_FOXCOMPAT1
  r->wait();
  ret=true;
  code=(FXint)(FXuval) r->result();
  FXEXCEPTION_FOXCOMPAT2
  return ret;
  }


// Suspend calling thread until thread is done
FXbool FXThread::join(){
  FXbool ret=false;
  FXEXCEPTION_FOXCOMPAT1
  r->wait();
  ret=true;
  FXEXCEPTION_FOXCOMPAT2
  return ret;
  }


// Cancel the thread
FXbool FXThread::cancel(){
  FXbool ret=false;
  FXEXCEPTION_FOXCOMPAT1
  r->requestTermination();
  r->wait();
  ret=true;
  FXEXCEPTION_FOXCOMPAT2
  return ret;
  }


// Detach thread
FXbool FXThread::detach(){
  return true;
  }


// Exit calling thread
void FXThread::exit(FXint code){
  QThread::exit((void *) code);
  }


// Yield the thread
void FXThread::yield(){
  FXEXCEPTION_FOXCOMPAT1
  QThread::yield();
  FXEXCEPTION_FOXCOMPAT2
  }


// Get time in nanoseconds since Epoch
FXlong FXThread::time(){
#ifndef WIN32
#ifdef __USE_POSIX199309
  const FXlong seconds=1000000000;
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME,&ts);
  return ts.tv_sec*seconds+ts.tv_nsec;
#else
  const FXlong seconds=1000000000;
  const FXlong microseconds=1000;
  struct timeval tv;
  gettimeofday(&tv,NULL);
  return tv.tv_sec*seconds+tv.tv_usec*microseconds;
#endif
#else
  FXlong now;
  GetSystemTimeAsFileTime((FILETIME*)&now);
#if defined(__CYGWIN__) || defined(__MINGW32__) || defined(__SC__)
  return (now-116444736000000000LL)*100LL;
#else
  return (now-116444736000000000L)*100L;
#endif
#endif
  }


// Sleep for some time
void FXThread::sleep(FXlong nsec){
  FXEXCEPTION_FOXCOMPAT1
  QThread::msleep(nsec/1000000);
  FXEXCEPTION_FOXCOMPAT2
  }


// Wake at appointed time
void FXThread::wakeat(FXlong nsec){
#ifndef WIN32
#ifdef __USE_POSIX199309
  const FXlong seconds=1000000000;
  struct timespec value;
#ifdef __USE_XOPEN2K
  value.tv_sec=nsec/seconds;
  value.tv_nsec=nsec%seconds;
  clock_nanosleep(CLOCK_REALTIME,TIMER_ABSTIME,&value,NULL);
#else
  nsec-=FXThread::time();
  if(nsec<0) nsec=0;
  value.tv_sec=nsec/seconds;
  value.tv_nsec=nsec%seconds;
  nanosleep(&value,NULL);
#endif
#else
  const FXlong seconds=1000000000;
  const FXlong microseconds=1000;
  const FXlong milliseconds=1000000;
  struct timeval value;
  if(nsec<0) nsec=0;
  value.tv_usec=(nsec/microseconds)%milliseconds;
  value.tv_sec=nsec/seconds;
  select(1,0,0,0,&value);
#endif
#else
  nsec-=FXThread::time();
  if(nsec<0) nsec=0;
  Sleep(nsec/1000000);
#endif
  }


// Return thread id of caller
FXThreadID FXThread::current(){
  return (FXThreadID) QThread::current();
  }


// Return pointer to calling thread's instance
FXThread* FXThread::self(){
  QThreadFXThread *ret=0;
  FXEXCEPTION_FOXCOMPAT1
  ret=dynamic_cast<QThreadFXThread *>(QThread::current());
  FXEXCEPTION_FOXCOMPAT2
  if(ret)
    return ret->parent;
  else
    return 0;
  }


// Set thread priority
void FXThread::priority(FXint prio){
  r->setPriority((signed char) prio);
  }


// Return thread priority
FXint FXThread::priority(){
  FXint ret=0;
  FXEXCEPTION_FOXCOMPAT1
  ret=r->priority();
  FXEXCEPTION_FOXCOMPAT2
  return ret;
  }


// Destroy; if it was running, stop it
FXThread::~FXThread(){
  FXDELETE(r);
  }


}

#endif
