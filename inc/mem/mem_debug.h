/**
 * include this header file at your source file first line.
 * On Windows Debug builds, CRT reports are written to stderr and the local
 * diagnostics log without opening modal dialog boxes.
 * On other platform, it will use allocator to trace memory.
 *
 * Note: memory debugging slows the program. Enable it only when needed.
 *       use it if your program memory keep growing.
 */

#pragma once
#ifndef LCU_MEM_DEBUG_H
#define LCU_MEM_DEBUG_H

// define this macro(_LCU_MEM_CHECK_FEATURE_ENABLE = 1) will enable memory check feature
// suggest user add it to compiler on build if you really want to debug memory.
// don't forget include this file(mem_debug.h) on your source file first line.
// #define _LCU_MEM_CHECK_FEATURE_ENABLE	 1

// Step 1: Define _CRTDBG_MAP_ALLOC early if needed (before any stdlib.h)
// otherwise it won't tell you leak memory on which file with line number in MSVC.
#if defined(_WIN32) && defined(_DEBUG) && !defined(_LCU_MEM_CHECK_FEATURE_ENABLE)
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

// Step 2: Windows-specific macros and CRT setup
#ifdef _WIN32
#ifndef __func__
#define __func__ __FUNCTION__
#endif // !__func__
#ifndef __PRETTY_FUNCTION__
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif // !__PRETTY_FUNCTION__

#if (defined(_DEBUG) && !defined(_LCU_MEM_CHECK_FEATURE_ENABLE))
// Forward declarations for diagnostics (avoid full header dependency)
#ifdef __cplusplus
extern "C" {
#endif
void lcu_diagnostics_init(void);
void lcu_diagnostics_deinit(void);
#ifdef __cplusplus
}
#endif

#pragma warning(push)
#pragma warning(disable : 5105)
#include <windows.h>
#pragma warning(pop)

// Replace _NORMAL_BLOCK with _CLIENT_BLOCK if you want the allocations to be of _CLIENT_BLOCK type
#define __MYDEBUG_NEW new (_NORMAL_BLOCK, __FILE__, __LINE__)
#define new __MYDEBUG_NEW

// Inline CRT configuration - no linking dependency on diagnostics.c
// Works with any Debug/Release library build (header-only implementation)
// Also calls diagnostics_init to enable log file output (optional, no-op if unavailable)
#define MEM_CHECK_INIT()                                                                   \
	do                                                                                     \
	{                                                                                      \
		lcu_diagnostics_init();                                                            \
		_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);             \
		_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);                                \
		_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);            \
		_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);                               \
		_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);           \
		_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);                              \
		_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);                     \
	} while (0)

#define MEM_CHECK_DEINIT() \
	do                     \
	{                      \
		lcu_diagnostics_deinit(); \
	} while (0)
#endif // _DEBUG && !_LCU_MEM_CHECK_FEATURE_ENABLE
#endif // _WIN32

// common header
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

#if (!defined(_CRTDBG_MAP_ALLOC) && defined(_LCU_MEM_CHECK_FEATURE_ENABLE) && _LCU_MEM_CHECK_FEATURE_ENABLE)
// to mark we really use lcu memory check feature
#define _USE_LCU_MEM_CHECK    1

// Forward declarations to avoid full diagnostics.h dependency
#ifdef __cplusplus
extern "C" {
#endif
void lcu_diagnostics_init(void);
void lcu_diagnostics_deinit(void);
#ifdef __cplusplus
}
#endif

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
#define free(p) lcu_free(p)
#define malloc(s) lcu_malloc_trace(s, __FILE__, __func__, __LINE__)
#define calloc(c, s) lcu_calloc_trace(c, s, __FILE__, __func__, __LINE__)
#define realloc(p, s) lcu_realloc_trace(p, s, __FILE__, __func__, __LINE__)
#define strdup(p) lcu_strdup_trace(p, __FILE__, __func__, __LINE__)
#define strndup(p, s) lcu_strndup_trace(p, s, __FILE__, __func__, __LINE__)

#endif // !_CRTDBG_MAP_ALLOC && _LCU_MEM_CHECK_FEATURE_ENABLE

#ifndef MEM_CHECK_INIT
// Fallback for non-Debug builds or non-Windows: forward to diagnostics
// (requires linking with lcu library, unlike the Windows Debug inline version above)
#ifdef __cplusplus
extern "C" {
#endif
void lcu_diagnostics_init(void);
void lcu_diagnostics_deinit(void);
#ifdef __cplusplus
}
#endif

#define MEM_CHECK_INIT()   lcu_diagnostics_init()
#define MEM_CHECK_DEINIT() lcu_diagnostics_deinit()
#endif // !MEM_CHECK_INIT

#endif // !LCU_MEM_DEBUG_H
