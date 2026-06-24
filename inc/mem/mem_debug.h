/**
 * include this header file at your source file first line.
 * On Windows Debug builds, CRT reports are written to stderr and the local
 * diagnostics log without opening modal dialog boxes.
 * On other platforms, define _LCU_MEM_CHECK_FEATURE_ENABLE=1 to use the LCU
 * allocator tracker.
 *
 * Note: memory debugging slows the program. Enable it only when needed.
 *       use it if your program memory keep growing.
 */

#pragma once
#ifndef LCU_MEM_DEBUG_H
#define LCU_MEM_DEBUG_H

// Define this macro(_LCU_MEM_CHECK_FEATURE_ENABLE = 1) to enable the LCU
// allocator tracker. Prefer adding it to compiler flags when you really want
// tracker-based memory debugging. Do not forget to include this file first.
// #define _LCU_MEM_CHECK_FEATURE_ENABLE	 1

/*
 * Select the implementation explicitly. Do not infer this from
 * _CRTDBG_MAP_ALLOC: callers may define that macro themselves, and MSVC Debug
 * still needs to use CRT diagnostics when _LCU_MEM_CHECK_FEATURE_ENABLE=0.
 */
#if defined(_LCU_MEM_CHECK_FEATURE_ENABLE) && ((_LCU_MEM_CHECK_FEATURE_ENABLE + 0) != 0)
#define _LCU_MEM_DEBUG_IMPL_USE_LCU_TRACKER 1
#else
#define _LCU_MEM_DEBUG_IMPL_USE_LCU_TRACKER 0
#endif

#if defined(_WIN32) && defined(_MSC_VER) && defined(_DEBUG) && !_LCU_MEM_DEBUG_IMPL_USE_LCU_TRACKER
#define _LCU_MEM_DEBUG_IMPL_USE_MSVC_CRT 1
#else
#define _LCU_MEM_DEBUG_IMPL_USE_MSVC_CRT 0
#endif

/*
 * MSVC CRT prelude: this must stay before diagnostics.h and before any other
 * header that can include <stdlib.h>. Otherwise _CRTDBG_MAP_ALLOC is too late
 * and malloc leaks lose the client source file/line in the CRT report.
 */
#if _LCU_MEM_DEBUG_IMPL_USE_MSVC_CRT
#ifndef _CRTDBG_MAP_ALLOC
#define _CRTDBG_MAP_ALLOC
#endif
#include <stdlib.h>
#include <crtdbg.h>
#endif // _LCU_MEM_DEBUG_IMPL_USE_MSVC_CRT

// Common headers must be included before the macro rewrite section below.
#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#include <cstdio>
#include <cstdbool>
#include <cstring>
#else
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#endif // __cplusplus
#include <malloc.h>

// Include diagnostics.h exactly once after the MSVC CRT prelude.
#include "mem/diagnostics.h"

#ifdef _WIN32
#ifndef __func__
#define __func__ __FUNCTION__
#endif // !__func__
#ifndef __PRETTY_FUNCTION__
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif // !__PRETTY_FUNCTION__
#endif // _WIN32

#if _LCU_MEM_DEBUG_IMPL_USE_MSVC_CRT
// Replace _NORMAL_BLOCK with _CLIENT_BLOCK if you want the allocations to be of _CLIENT_BLOCK type
#define _LCU_MEM_DEBUG_NEW new (_NORMAL_BLOCK, __FILE__, __LINE__)
// Macro rewrites intentionally come after all includes above.
#define new _LCU_MEM_DEBUG_NEW

// Register this translation unit's Debug CRT with diagnostics.
#define MEM_CHECK_INIT()   lcu_diagnostics_register_current_crt()
#define MEM_CHECK_DEINIT() lcu_diagnostics_unregister_current_crt()
#endif // _LCU_MEM_DEBUG_IMPL_USE_MSVC_CRT

#if _LCU_MEM_DEBUG_IMPL_USE_LCU_TRACKER
// to mark we really use lcu memory check feature
#define _USE_LCU_MEM_CHECK    1
#include "mem/allocator.h"
#include "mem/allocation_tracker.h"

#ifdef __cplusplus
/* Fix #2 (ODR): These global replaceable allocation/deallocation functions are
 * DECLARED here but DEFINED exactly once in src/mem/mem_debug.cpp. Defining
 * them inline in this header (as before) produced one definition per including
 * C++ TU -> multiple-definition link error (LNK2005) as soon as two .cpp files
 * followed the "include mem_debug.h first" convention.
 *
 * Fix M3: the placement form `operator new(size,file,func,line)` (selected by
 * the `#define new` below) now has MATCHING placement `operator delete`
 * overloads. The C++ runtime calls these automatically if a constructor throws
 * after placement-new allocated storage; without them that storage leaked.
 *
 * WARNING: enabling _LCU_MEM_CHECK_FEATURE_ENABLE replaces the GLOBAL operator
 * new/delete for the whole program/module that links these objects. Do NOT
 * enable it for a shared library consumed by clients that are unaware of the
 * replacement, and keep the library + client on the SAME cross-DLL/CRT heap. */
void *operator new(size_t size, const char *fileName, const char *funcName, int line);
void *operator new[](size_t size, const char *fileName, const char *funcName, int line);
void operator delete(void *ptr) noexcept;
void operator delete[](void *ptr) noexcept;
/* M3: placement deletes matching the placement news above. */
void operator delete(void *ptr, const char *fileName, const char *funcName, int line) noexcept;
void operator delete[](void *ptr, const char *fileName, const char *funcName, int line) noexcept;

// Macro rewrites intentionally come after all includes above.
#define new new (__FILE__, __func__, __LINE__)
#endif // __cplusplus

#define MEM_CHECK_INIT()                     \
	do                                       \
	{                                        \
		lcu_diagnostics_init();              \
		allocation_tracker_init();           \
	} while (0)

#define MEM_CHECK_DEINIT()                                    \
	do                                                        \
	{                                                         \
		allocation_tracker_expect_no_allocations(NULL, NULL); \
		allocation_tracker_uninit();                          \
		lcu_diagnostics_deinit();                             \
	} while (0)

#if (defined(free) || defined(malloc) || defined(calloc) || defined(realloc) || defined(strdup) || defined(strndup))
#error "free/malloc/calloc/realloc/strdup/strndup is defined. you should put \"mem_debug.h\" on your source file first line."
#endif
#define free(p)        lcu_free(p)
#define malloc(s)      lcu_malloc_trace(s, __FILE__, __func__, __LINE__)
#define calloc(c, s)   lcu_calloc_trace(c, s, __FILE__, __func__, __LINE__)
#define realloc(p, s)  lcu_realloc_trace(p, s, __FILE__, __func__, __LINE__)
#define strdup(p)      lcu_strdup_trace(p, __FILE__, __func__, __LINE__)
#define strndup(p, s)  lcu_strndup_trace(p, s, __FILE__, __func__, __LINE__)

#endif // _LCU_MEM_DEBUG_IMPL_USE_LCU_TRACKER

#ifndef MEM_CHECK_INIT
#define MEM_CHECK_INIT()   lcu_diagnostics_init()
#define MEM_CHECK_DEINIT() lcu_diagnostics_deinit()
#endif // !MEM_CHECK_INIT

#endif // !LCU_MEM_DEBUG_H
