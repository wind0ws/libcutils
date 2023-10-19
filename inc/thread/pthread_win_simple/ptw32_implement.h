/*
 * implement.h
 *
 * Definitions that don't need to be public.
 *
 * Keeps all the internals out of pthread.h
 *
 * --------------------------------------------------------------------------
 *
 *      Pthreads-win32 - POSIX Threads Library for Win32
 *      Copyright(C) 1998 John E. Bossom
 *      Copyright(C) 1999,2012 Pthreads-win32 contributors
 *
 *      Contact Email: Ross.Johnson@homemail.com.au
 *
 *      The current list of contributors is contained
 *      in the file CONTRIBUTORS included with the source
 *      code distribution. The list can also be seen at the
 *      following World Wide Web location:
 *      http://sources.redhat.com/pthreads-win32/contributors.html
 *
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2 of the License, or (at your option) any later version.
 *
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 *
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library in the file COPYING.LIB;
 *      if not, write to the Free Software Foundation, Inc.,
 *      59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */
#pragma once
#if !defined(_IMPLEMENT_H)
#define _IMPLEMENT_H

//#if !defined(PTW32_CONFIG_H) && !defined(_PTHREAD_TEST_H_)
//# error "config.h was not #included"
//#endif

#if !defined(_WIN32_WINNT)
# define _WIN32_WINNT 0x0400
#endif

#include "thread/pthread_win_simple/ptw32_mcs_lock.h"
#pragma warning(push)
#pragma warning(disable: 5105)
#include <windows.h>
#pragma warning(pop)

/*
 * In case windows.h doesn't define it (e.g. WinCE perhaps)
 */
#if defined(WINCE)
typedef VOID (APIENTRY *PAPCFUNC)(DWORD dwParam);
#endif

/*
 * Designed to allow error values to be set and retrieved in builds where
 * MSCRT libraries are statically linked to DLLs.
 */
#if ( defined(PTW32_CONFIG_MINGW) && __MSVCRT_VERSION__ >= 0x0800 ) || \
    ( defined(_MSC_VER) && _MSC_VER >= 1400 )  /* MSVC8+ */
#  if defined(PTW32_CONFIG_MINGW)
__attribute__((unused))
#  endif
static int ptw32_get_errno(void) { int err = 0; _get_errno(&err); return err; }
#  define PTW32_GET_ERRNO() ptw32_get_errno()
#  if defined(PTW32_USES_SEPARATE_CRT)
#    if defined(PTW32_CONFIG_MINGW)
__attribute__((unused))
#    endif
static void ptw32_set_errno(int err) { _set_errno(err); SetLastError(err); }
#    define PTW32_SET_ERRNO(err) ptw32_set_errno(err)
#  else
#    define PTW32_SET_ERRNO(err) _set_errno(err)
#  endif
#else
#  define PTW32_GET_ERRNO() (errno)
#  if defined(PTW32_USES_SEPARATE_CRT)
#    if defined(PTW32_CONFIG_MINGW)
__attribute__((unused))
#    endif
static void ptw32_set_errno(int err) { errno = err; SetLastError(err); }
#    define PTW32_SET_ERRNO(err) ptw32_set_errno(err)
#  else
#    define PTW32_SET_ERRNO(err) (errno = (err))
#  endif
#endif

/*
 * note: ETIMEDOUT is correctly defined in winsock.h
 */
#include <winsock.h>

/*
 * In case ETIMEDOUT hasn't been defined above somehow.
 */
#if !defined(ETIMEDOUT)
# define ETIMEDOUT 10060	/* This is the value in winsock.h. */
#endif

#if !defined(malloc)
# include <malloc.h>
#endif

#if defined(__CLEANUP_C)
# include <setjmp.h>
#endif

#if !defined(INT_MAX)
# include <limits.h>
#endif

/* _tcsncat_s() et al: mapping to the correct TCHAR prototypes: */
#include <tchar.h>

/* use local include files during development */
//#include "semaphore.h"
//#include "sched.h"

/* MSVC 7.1 doesn't like complex #if expressions */
#define INLINE
#if defined(PTW32_BUILD_INLINED)
#  if defined(HAVE_C_INLINE) || defined(__cplusplus)
#    undef INLINE
#    define INLINE inline
#  endif
#endif

#if defined(PTW32_CONFIG_MSVC6)
/*
 * MSVC 6 does not use the "volatile" qualifier
 */
# define PTW32_INTERLOCKED_VOLATILE
#else
# define PTW32_INTERLOCKED_VOLATILE volatile
#endif

#define PTW32_INTERLOCKED_LONG long
#if defined(_M_IA64)
#define PTW32_INTERLOCKED_SIZE LONG64
#elif defined(_M_AMD64)
#define PTW32_INTERLOCKED_SIZE LONG64
#elif defined(_WIN64)
#define PTW32_INTERLOCKED_SIZE LONGLONG
#else
#define PTW32_INTERLOCKED_SIZE LONG
#endif
#define PTW32_INTERLOCKED_PVOID PVOID
#define PTW32_INTERLOCKED_LONGPTR PTW32_INTERLOCKED_VOLATILE long*
#if defined(_M_IA64)
#define PTW32_INTERLOCKED_SIZEPTR PTW32_INTERLOCKED_VOLATILE LONG64*
#elif defined(_M_AMD64)
#define PTW32_INTERLOCKED_SIZEPTR PTW32_INTERLOCKED_VOLATILE LONG64*
#elif defined(_WIN64)
#define PTW32_INTERLOCKED_SIZEPTR PTW32_INTERLOCKED_VOLATILE LONGLONG*
#else
#define PTW32_INTERLOCKED_SIZEPTR PTW32_INTERLOCKED_VOLATILE LONG*
#endif
#define PTW32_INTERLOCKED_PVOID_PTR PTW32_INTERLOCKED_VOLATILE PVOID*

#if defined(PTW32_CONFIG_MINGW)
#  include <stdint.h>
#elif defined(__BORLANDC__)
#  define int64_t ULONGLONG
#else
#  define int64_t _int64
#  if defined(PTW32_CONFIG_MSVC6)
     typedef long intptr_t;
#  endif
#endif

/*
 * Don't allow the linker to optimize away autostatic.obj in static builds.
 */
#if defined(PTW32_STATIC_LIB) && defined(PTW32_BUILD)
  void ptw32_autostatic_anchor(void);
# if defined(PTW32_CONFIG_MINGW)
    __attribute__((unused, used))
# endif
  static void (*local_autostatic_anchor)(void) = ptw32_autostatic_anchor;
#endif

/*
 * Special value to mark attribute objects as valid.
 */
#define PTW32_ATTR_VALID ((unsigned long) 0xC4C0FFEE)




#define PTW32_OBJECT_AUTO_INIT ((void *)(size_t) -1)
#define PTW32_OBJECT_INVALID   NULL


/*
 * Possible values, other than PTW32_OBJECT_INVALID,
 * for the "interlock" element in a spinlock.
 *
 * In this implementation, when a spinlock is initialised,
 * the number of cpus available to the process is checked.
 * If there is only one cpu then "interlock" is set equal to
 * PTW32_SPIN_USE_MUTEX and u.mutex is an initialised mutex.
 * If the number of cpus is greater than 1 then "interlock"
 * is set equal to PTW32_SPIN_UNLOCKED and the number is
 * stored in u.cpus. This arrangement allows the spinlock
 * routines to attempt an InterlockedCompareExchange on "interlock"
 * immediately and, if that fails, to try the inferior mutex.
 *
 * "u.cpus" isn't used for anything yet, but could be used at
 * some point to optimise spinlock behaviour.
 */
#define PTW32_SPIN_INVALID     (0)
#define PTW32_SPIN_UNLOCKED    (1)
#define PTW32_SPIN_LOCKED      (2)
#define PTW32_SPIN_USE_MUTEX   (3)


#if defined(__CLEANUP_SEH)
/*
 * --------------------------------------------------------------
 * MAKE_SOFTWARE_EXCEPTION
 *      This macro constructs a software exception code following
 *      the same format as the standard Win32 error codes as defined
 *      in WINERROR.H
 *  Values are 32 bit values laid out as follows:
 *
 *   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *  +---+-+-+-----------------------+-------------------------------+
 *  |Sev|C|R|     Facility          |               Code            |
 *  +---+-+-+-----------------------+-------------------------------+
 *
 * Severity Values:
 */
#define SE_SUCCESS              0x00
#define SE_INFORMATION          0x01
#define SE_WARNING              0x02
#define SE_ERROR                0x03

#define MAKE_SOFTWARE_EXCEPTION( _severity, _facility, _exception ) \
( (DWORD) ( ( (_severity) << 30 ) |     /* Severity code        */ \
            ( 1 << 29 ) |               /* MS=0, User=1         */ \
            ( 0 << 28 ) |               /* Reserved             */ \
            ( (_facility) << 16 ) |     /* Facility Code        */ \
            ( (_exception) <<  0 )      /* Exception Code       */ \
            ) )

/*
 * We choose one specific Facility/Error code combination to
 * identify our software exceptions vs. WIN32 exceptions.
 * We store our actual component and error code within
 * the optional information array.
 */
#define EXCEPTION_PTW32_SERVICES        \
     MAKE_SOFTWARE_EXCEPTION( SE_ERROR, \
                              PTW32_SERVICES_FACILITY, \
                              PTW32_SERVICES_ERROR )

#define PTW32_SERVICES_FACILITY         0xBAD
#define PTW32_SERVICES_ERROR            0xDEED

#endif /* __CLEANUP_SEH */

/*
 * Services available through EXCEPTION_PTW32_SERVICES
 * and also used [as parameters to ptw32_throw()] as
 * generic exception selectors.
 */

#define PTW32_EPS_EXIT                  (1)
#define PTW32_EPS_CANCEL                (2)


/* Useful macros */
#define PTW32_MAX(a,b)  ((a)<(b)?(b):(a))
#define PTW32_MIN(a,b)  ((a)>(b)?(b):(a))


/* Declared in pthread_cancel.c */
extern DWORD (*ptw32_register_cancellation) (PAPCFUNC, HANDLE, DWORD);

/* Thread Reuse stack bottom marker. Must not be NULL or any valid pointer to memory. */
#define PTW32_THREAD_REUSE_EMPTY ((ptw32_thread_t *)(size_t) 1)

extern int ptw32_processInitialized;

extern int ptw32_mutex_default_kind;

extern unsigned __int64 ptw32_threadSeqNumber;

extern int ptw32_concurrency;

extern int ptw32_features;

extern ptw32_mcs_lock_t ptw32_thread_reuse_lock;
extern ptw32_mcs_lock_t ptw32_mutex_test_init_lock;
extern ptw32_mcs_lock_t ptw32_cond_list_lock;
extern ptw32_mcs_lock_t ptw32_cond_test_init_lock;
extern ptw32_mcs_lock_t ptw32_rwlock_test_init_lock;
extern ptw32_mcs_lock_t ptw32_spinlock_test_init_lock;

#if defined(_UWIN)
extern int pthread_count;
#endif


#if defined(_UWIN_)
#   if defined(_MT)
#       if defined(__cplusplus)
extern "C"
{
#       endif
  _CRTIMP unsigned long __cdecl _beginthread (void (__cdecl *) (void *),
					      unsigned, void *);
  _CRTIMP void __cdecl _endthread (void);
  _CRTIMP unsigned long __cdecl _beginthreadex (void *, unsigned,
						unsigned (__stdcall *) (void *),
						void *, unsigned, unsigned *);
  _CRTIMP void __cdecl _endthreadex (unsigned);
#       if defined(__cplusplus)
}
#       endif
#   endif
#else
#       include <process.h>
#   endif


/*
 * Use intrinsic versions wherever possible. VC will do this
 * automatically where possible and GCC define these if available:
 * __GCC_HAVE_SYNC_COMPARE_AND_SWAP_1
 * __GCC_HAVE_SYNC_COMPARE_AND_SWAP_2
 * __GCC_HAVE_SYNC_COMPARE_AND_SWAP_4
 * __GCC_HAVE_SYNC_COMPARE_AND_SWAP_8
 * __GCC_HAVE_SYNC_COMPARE_AND_SWAP_16
 *
 * The full set of Interlocked intrinsics in GCC are (check versions):
 * type __sync_fetch_and_add (type *ptr, type value, ...)
 * type __sync_fetch_and_sub (type *ptr, type value, ...)
 * type __sync_fetch_and_or (type *ptr, type value, ...)
 * type __sync_fetch_and_and (type *ptr, type value, ...)
 * type __sync_fetch_and_xor (type *ptr, type value, ...)
 * type __sync_fetch_and_nand (type *ptr, type value, ...)
 * type __sync_add_and_fetch (type *ptr, type value, ...)
 * type __sync_sub_and_fetch (type *ptr, type value, ...)
 * type __sync_or_and_fetch (type *ptr, type value, ...)
 * type __sync_and_and_fetch (type *ptr, type value, ...)
 * type __sync_xor_and_fetch (type *ptr, type value, ...)
 * type __sync_nand_and_fetch (type *ptr, type value, ...)
 * bool __sync_bool_compare_and_swap (type *ptr, type oldval type newval, ...)
 * type __sync_val_compare_and_swap (type *ptr, type oldval type newval, ...)
 * __sync_synchronize (...) // Full memory barrier
 * type __sync_lock_test_and_set (type *ptr, type value, ...) // Acquire barrier
 * void __sync_lock_release (type *ptr, ...) // Release barrier
 *
 * These are all overloaded and take 1,2,4,8 byte scalar or pointer types.
 *
 * The above aren't available in Mingw32 as of gcc 4.5.2 so define our own.
 */
#if defined(__cplusplus)
# define PTW32_TO_VLONG64PTR(ptr) reinterpret_cast<volatile LONG64 *>(ptr)
#else
# define PTW32_TO_VLONG64PTR(ptr) (ptr)
#endif

#if defined(__GNUC__)
# if defined(_WIN64)
# define PTW32_INTERLOCKED_COMPARE_EXCHANGE_64(location, value, comparand) \
    ({                                                                     \
      __typeof (value) _result;                                            \
      __asm__ __volatile__                                                 \
      (                                                                    \
        "lock\n\t"                                                         \
        "cmpxchgq      %2,(%1)"                                            \
        :"=a" (_result)                                                    \
        :"r"  (location), "r" (value), "a" (comparand)                     \
        :"memory", "cc");                                                  \
      _result;                                                             \
    })
# define PTW32_INTERLOCKED_EXCHANGE_64(location, value)                    \
    ({                                                                     \
      __typeof (value) _result;                                            \
      __asm__ __volatile__                                                 \
      (                                                                    \
        "xchgq	 %0,(%1)"                                                  \
        :"=r" (_result)                                                    \
        :"r" (location), "0" (value)                                       \
        :"memory", "cc");                                                  \
      _result;                                                             \
    })
# define PTW32_INTERLOCKED_EXCHANGE_ADD_64(location, value)                \
    ({                                                                     \
      __typeof (value) _result;                                            \
      __asm__ __volatile__                                                 \
      (                                                                    \
        "lock\n\t"                                                         \
        "xaddq	 %0,(%1)"                                                  \
        :"=r" (_result)                                                    \
        :"r" (location), "0" (value)                                       \
        :"memory", "cc");                                                  \
      _result;                                                             \
    })
# define PTW32_INTERLOCKED_INCREMENT_64(location)                          \
    ({                                                                     \
      PTW32_INTERLOCKED_LONG _temp = 1;                                    \
      __asm__ __volatile__                                                 \
      (                                                                    \
        "lock\n\t"                                                         \
        "xaddq	 %0,(%1)"                                                  \
        :"+r" (_temp)                                                      \
        :"r" (location)                                                    \
        :"memory", "cc");                                                  \
      ++_temp;                                                             \
    })
# define PTW32_INTERLOCKED_DECREMENT_64(location)                          \
    ({                                                                     \
      PTW32_INTERLOCKED_LONG _temp = -1;                                   \
      __asm__ __volatile__                                                 \
      (                                                                    \
        "lock\n\t"                                                         \
        "xaddq	 %2,(%1)"                                                  \
        :"+r" (_temp)                                                      \
        :"r" (location)                                                    \
        :"memory", "cc");                                                  \
      --_temp;                                                             \
    })
#endif
# define PTW32_INTERLOCKED_COMPARE_EXCHANGE_LONG(location, value, comparand) \
    ({                                                                     \
      __typeof (value) _result;                                            \
      __asm__ __volatile__                                                 \
      (                                                                    \
        "lock\n\t"                                                         \
        "cmpxchgl       %2,(%1)"                                           \
        :"=a" (_result)                                                    \
        :"r"  (location), "r" (value), "a" (comparand)                     \
        :"memory", "cc");                                                  \
      _result;                                                             \
    })
# define PTW32_INTERLOCKED_EXCHANGE_LONG(location, value)                  \
    ({                                                                     \
      __typeof (value) _result;                                            \
      __asm__ __volatile__                                                 \
      (                                                                    \
        "xchgl	 %0,(%1)"                                                  \
        :"=r" (_result)                                                    \
        :"r" (location), "0" (value)                                       \
        :"memory", "cc");                                                  \
      _result;                                                             \
    })
# define PTW32_INTERLOCKED_EXCHANGE_ADD_LONG(location, value)              \
    ({                                                                     \
      __typeof (value) _result;                                            \
      __asm__ __volatile__                                                 \
      (                                                                    \
        "lock\n\t"                                                         \
        "xaddl	 %0,(%1)"                                                  \
        :"=r" (_result)                                                    \
        :"r" (location), "0" (value)                                       \
        :"memory", "cc");                                                  \
      _result;                                                             \
    })
# define PTW32_INTERLOCKED_INCREMENT_LONG(location)                        \
    ({                                                                     \
      PTW32_INTERLOCKED_LONG _temp = 1;                                    \
      __asm__ __volatile__                                                 \
      (                                                                    \
        "lock\n\t"                                                         \
        "xaddl	 %0,(%1)"                                                  \
        :"+r" (_temp)                                                      \
        :"r" (location)                                                    \
        :"memory", "cc");                                                  \
      ++_temp;                                                             \
    })
# define PTW32_INTERLOCKED_DECREMENT_LONG(location)                        \
    ({                                                                     \
      PTW32_INTERLOCKED_LONG _temp = -1;                                   \
      __asm__ __volatile__                                                 \
      (                                                                    \
        "lock\n\t"                                                         \
        "xaddl	 %0,(%1)"                                                  \
        :"+r" (_temp)                                                      \
        :"r" (location)                                                    \
        :"memory", "cc");                                                  \
      --_temp;                                                             \
    })
# define PTW32_INTERLOCKED_COMPARE_EXCHANGE_PTR(location, value, comparand) \
    PTW32_INTERLOCKED_COMPARE_EXCHANGE_SIZE((PTW32_INTERLOCKED_SIZEPTR)location, \
                                            (PTW32_INTERLOCKED_SIZE)value, \
                                            (PTW32_INTERLOCKED_SIZE)comparand)
# define PTW32_INTERLOCKED_EXCHANGE_PTR(location, value) \
    PTW32_INTERLOCKED_EXCHANGE_SIZE((PTW32_INTERLOCKED_SIZEPTR)location, \
                                    (PTW32_INTERLOCKED_SIZE)value)
#else
# if defined(_WIN64)
#   define PTW32_INTERLOCKED_COMPARE_EXCHANGE_64(p,v,c) InterlockedCompareExchange64(PTW32_TO_VLONG64PTR(p),(v),(c))
#   define PTW32_INTERLOCKED_EXCHANGE_64(p,v) InterlockedExchange64(PTW32_TO_VLONG64PTR(p),(v))
#   define PTW32_INTERLOCKED_EXCHANGE_ADD_64(p,v) InterlockedExchangeAdd64(PTW32_TO_VLONG64PTR(p),(v))
#   define PTW32_INTERLOCKED_INCREMENT_64(p) InterlockedIncrement64(PTW32_TO_VLONG64PTR(p))
#   define PTW32_INTERLOCKED_DECREMENT_64(p) InterlockedDecrement64(PTW32_TO_VLONG64PTR(p))
# endif
# if defined(PTW32_CONFIG_MSVC6) && !defined(_WIN64)
#  define PTW32_INTERLOCKED_COMPARE_EXCHANGE_LONG(location, value, comparand) \
      ((LONG)InterlockedCompareExchange((PVOID *)(location), (PVOID)(value), (PVOID)(comparand)))
# else
#  define PTW32_INTERLOCKED_COMPARE_EXCHANGE_LONG InterlockedCompareExchange
# endif
# define PTW32_INTERLOCKED_EXCHANGE_LONG(p,v) InterlockedExchange((p),(v))
# define PTW32_INTERLOCKED_EXCHANGE_ADD_LONG(p,v) InterlockedExchangeAdd((p),(v))
# define PTW32_INTERLOCKED_INCREMENT_LONG(p) InterlockedIncrement((p))
# define PTW32_INTERLOCKED_DECREMENT_LONG(p) InterlockedDecrement((p))
# if defined(PTW32_CONFIG_MSVC6) && !defined(_WIN64)
#  define PTW32_INTERLOCKED_COMPARE_EXCHANGE_PTR InterlockedCompareExchange
#  define PTW32_INTERLOCKED_EXCHANGE_PTR(location, value) \
    ((PVOID)InterlockedExchange((LPLONG)(location), (LONG)(value)))
# else
#  define PTW32_INTERLOCKED_COMPARE_EXCHANGE_PTR(p,v,c) InterlockedCompareExchangePointer((p),(v),(c))
#  define PTW32_INTERLOCKED_EXCHANGE_PTR(p,v) InterlockedExchangePointer((p),(v))
# endif
#endif
#if defined(_WIN64)
#   define PTW32_INTERLOCKED_COMPARE_EXCHANGE_SIZE(p,v,c) PTW32_INTERLOCKED_COMPARE_EXCHANGE_64(PTW32_TO_VLONG64PTR(p),(v),(c))
#   define PTW32_INTERLOCKED_EXCHANGE_SIZE(p,v) PTW32_INTERLOCKED_EXCHANGE_64(PTW32_TO_VLONG64PTR(p),(v))
#   define PTW32_INTERLOCKED_EXCHANGE_ADD_SIZE(p,v) PTW32_INTERLOCKED_EXCHANGE_ADD_64(PTW32_TO_VLONG64PTR(p),(v))
#   define PTW32_INTERLOCKED_INCREMENT_SIZE(p) PTW32_INTERLOCKED_INCREMENT_64(PTW32_TO_VLONG64PTR(p))
#   define PTW32_INTERLOCKED_DECREMENT_SIZE(p) PTW32_INTERLOCKED_DECREMENT_64(PTW32_TO_VLONG64PTR(p))
#else
#   define PTW32_INTERLOCKED_COMPARE_EXCHANGE_SIZE(p,v,c) PTW32_INTERLOCKED_COMPARE_EXCHANGE_LONG((p),(v),(c))
#   define PTW32_INTERLOCKED_EXCHANGE_SIZE(p,v) PTW32_INTERLOCKED_EXCHANGE_LONG((p),(v))
#   define PTW32_INTERLOCKED_EXCHANGE_ADD_SIZE(p,v) PTW32_INTERLOCKED_EXCHANGE_ADD_LONG((p),(v))
#   define PTW32_INTERLOCKED_INCREMENT_SIZE(p) PTW32_INTERLOCKED_INCREMENT_LONG((p))
#   define PTW32_INTERLOCKED_DECREMENT_SIZE(p) PTW32_INTERLOCKED_DECREMENT_LONG((p))
#endif

#if defined(NEED_CREATETHREAD)

/*
 * Macro uses args so we can cast start_proc to LPTHREAD_START_ROUTINE
 * in order to avoid warnings because of return type
 */

#define _beginthreadex(security, \
                       stack_size, \
                       start_proc, \
                       arg, \
                       flags, \
                       pid) \
        CreateThread(security, \
                     stack_size, \
                     (LPTHREAD_START_ROUTINE) start_proc, \
                     arg, \
                     flags, \
                     pid)

#define _endthreadex ExitThread

#endif				/* NEED_CREATETHREAD */


#endif				/* _IMPLEMENT_H */
