/* Basic platform-independent macro definitions for mutexes,
   thread-specific data and parameters for malloc.
   Copyright (C) 2003 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#ifndef _POSIX_MALLOC_MACHINE_H
#define _POSIX_MALLOC_MACHINE_H

#include <pthread.h>

typedef pthread_t thread_id;

/* mutex */
#if (defined __i386__ || defined __x86_64__) && defined __GNUC__ && \
    !defined USE_NO_SPINLOCKS

#include <time.h>

/* Use fast inline spinlocks.  */
typedef struct {
  volatile unsigned int lock;
  int pad0_;
} mutex_t;

#define MUTEX_INITIALIZER          { 0 }
#define mutex_init(m)              ((m)->lock = 0)
static inline int mutex_lock(mutex_t *m) {
  int cnt = 0, r;
  struct timespec tm;

  for(;;) {
    __asm__ __volatile__
      ("pause\n\txchgl %0, %1"
       : "=r"(r), "=m"(m->lock)
       : "0"(1), "m"(m->lock)
       : "memory");
    if(!r)
      return 0;
#ifndef FX_SMPBUILD		// Faster not to sleep on multiprocessor machines
    if(cnt < 50) {
      sched_yield();
      cnt++;
    } else
#endif
    {
      tm.tv_sec = 0;
      tm.tv_nsec = 2000001;
      nanosleep(&tm, NULL);
      cnt = 0;
    }
  }
}
static inline int mutex_trylock(mutex_t *m) {
  int r;

  __asm__ __volatile__
    ("xchgl %0, %1"
     : "=r"(r), "=m"(m->lock)
     : "0"(1), "m"(m->lock)
     : "memory");
  return r;
}
static inline int mutex_unlock(mutex_t *m) {
  int r;

  __asm__ __volatile__
    ("xchgl %0, %1"
     : "=r"(r), "=m"(m->lock)
     : "0"(0), "m"(m->lock)
     : "memory");
  return 0;
}

#else

/* Normal pthread mutex.  */
typedef pthread_mutex_t mutex_t;

#define MUTEX_INITIALIZER          PTHREAD_MUTEX_INITIALIZER
#define mutex_init(m)              pthread_mutex_init(m, NULL)
#define mutex_lock(m)              pthread_mutex_lock(m)
#define mutex_trylock(m)           pthread_mutex_trylock(m)
#define mutex_unlock(m)            pthread_mutex_unlock(m)

#endif /* (__i386__ || __x86_64__) && __GNUC__ && !USE_NO_SPINLOCKS */

# define thread_atfork(prepare, parent, child) \
   pthread_atfork (prepare, parent, child)

typedef pthread_key_t tsd_key_t;

#define tsd_key_create(key, destr) pthread_key_create(key, destr)
#define tsd_setspecific(key, data) pthread_setspecific(key, data)
#define tsd_getspecific(key, vptr) (vptr = pthread_getspecific(key))


#ifndef atomic_full_barrier
# define atomic_full_barrier() __asm ("" ::: "memory")
#endif

#ifndef atomic_read_barrier
# define atomic_read_barrier() atomic_full_barrier ()
#endif

#ifndef atomic_write_barrier
# define atomic_write_barrier() atomic_full_barrier ()
#endif

#ifndef DEFAULT_TOP_PAD
# define DEFAULT_TOP_PAD 131072
#endif

#endif /* !defined(_MALLOC_MACHINE_H) */
