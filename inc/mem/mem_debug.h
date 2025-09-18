/**
 * include this header file at your source file first line.
 * On windows, in debug mode, it will use crtdbg help you find memory leak;
 * On other platform, it will use allocator to trace memory.
 *
 * Note: use memory debug will slow your program. Be aware of that.
 *       use it if your program memory keep growing.
 */

#pragma once
#ifndef LCU_MEM_DEBUG_H
#define LCU_MEM_DEBUG_H

// define this macro(_LCU_MEM_CHECK_FEATURE_ENABLE = 1) will enable memory check feature
// suggest user add it to compiler on build if you really want to debug memory.
// don't forget include this file(mem_debug.h) on your source file first line.
// #define _LCU_MEM_CHECK_FEATURE_ENABLE	 1

#ifdef _WIN32
#ifndef __func__
#define __func__ __FUNCTION__
#endif // !__func__
#ifndef __PRETTY_FUNCTION__
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif // !__PRETTY_FUNCTION__

#if (defined(_DEBUG) && !defined(_LCU_MEM_CHECK_FEATURE_ENABLE))
// must keep next 3 line on your top source file,
// otherwise it won't tell you leak memory on which file with line number.
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#pragma warning(push)
#pragma warning(disable : 5105)
#include <windows.h>
#pragma warning(pop)

// Replace _NORMAL_BLOCK with _CLIENT_BLOCK if you want the
// allocations to be of _CLIENT_BLOCK type
#define __MYDEBUG_NEW new (_NORMAL_BLOCK, __FILE__, __LINE__)
#define new __MYDEBUG_NEW

// only need call once on your main function first line!
// create log file, no need close it at end of main function, because crt will write log to it.
// dump is in warn level. let warn log to file, debug console and window .
#define MEM_CHECK_INIT()                                                                                                              \
	do                                                                                                                                \
	{                                                                                                                                 \
		void *_hDbgLogFile = CreateFile(TEXT("./memleak.log"), GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ,                     \
										NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);                                            \
		if (NULL == _hDbgLogFile)                                                                                                     \
		{                                                                                                                             \
			printf("  err: oops! failed on create memleak file!  \n");                                                                \
			break;                                                                                                                    \
		}                                                                                                                             \
		_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG /*| _CRTDBG_MODE_WNDW*/);                                 \
		_CrtSetReportFile(_CRT_WARN, _hDbgLogFile);                                                                                   \
		_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_LEAK_CHECK_DF); \
	} while (0)
// nothing need to do on deinit
#define MEM_CHECK_DEINIT() \
	do                     \
	{                      \
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
#define _USE_LCU_MEM_CHECK
#include "mem/allocator.h"
#include "mem/allocation_tracker.h"

#ifdef __cplusplus
void *operator new(size_t size, const char *fileName, const char *funcName, int line);
void *operator new[](size_t size, const char *fileName, const char *funcName, int line);
void operator delete(void *ptr) noexcept;
void operator delete[](void *ptr) noexcept;

void *operator new(size_t size, const char *fileName, const char *funcName, int line)
{
	// here we are not deal with new(0), but it is acceptable,
	// because if user change return pointer's memory, it will trigger memory corruption on delete it.
	return lcu_malloc_trace(size, fileName, funcName, line);
}

void *operator new[](size_t size, const char *fileName, const char *funcName, int line)
{
	return operator new(size, fileName, funcName, line);
}

void operator delete(void *ptr) noexcept
{
	if (nullptr == ptr)
	{
		return;
	}
	lcu_free(ptr);
}

void operator delete[](void *ptr) noexcept
{
	if (nullptr == ptr)
	{
		return;
	}
	operator delete(ptr);
}

#define new new (__FILE__, __func__, __LINE__)
#endif // __cplusplus

#define MEM_CHECK_INIT() allocation_tracker_init()
#define MEM_CHECK_DEINIT()                                    \
	do                                                        \
	{                                                         \
		allocation_tracker_expect_no_allocations(NULL, NULL); \
		allocation_tracker_uninit();                          \
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
#define MEM_CHECK_INIT() \
	do                   \
	{                    \
	} while (0)
#define MEM_CHECK_DEINIT() \
	do                     \
	{                      \
	} while (0)
#endif // !MEM_CHECK_INIT

#endif // !LCU_MEM_DEBUG_H
