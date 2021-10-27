/**
 * include this header file at your source file first line.
 * On windows, in debug mode, it will use crtdbg help you find memory leak.
 * Otherwise, use allocator to trace memory.
 * 
 * Note: use memory debug will slow your program. Be aware of that.
 *       use it if your program memory keep growing.
 */

#pragma once
#ifndef LCU_MEM_DEBUG_H
#define LCU_MEM_DEBUG_H

// define this macro(_ENABLE_LCU_MEM_CHECK_FEATURE) will enable memory check feature
// suggest user add it to compiler on build if you really want to debug memory.
// don't forget add this file(mem_debug.h) on your source file first line.
//#define _ENABLE_LCU_MEM_CHECK_FEATURE

#ifdef _WIN32
#ifndef __func__
#define __func__ __FUNCTION__
#endif // !__func__
#ifndef __PRETTY_FUNCTION__
#define __PRETTY_FUNCTION__ __FUNCSIG__ 
#endif // !__PRETTY_FUNCTION__

#if(defined(_DEBUG) && !defined(_ENABLE_LCU_MEM_CHECK_FEATURE))
 // must keep next 3 line on your top source file,
 // otherwise it won't tell you leak memory on which file with line number.
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

// Replace _NORMAL_BLOCK with _CLIENT_BLOCK if you want the
// allocations to be of _CLIENT_BLOCK type
#define __MYDEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new __MYDEBUG_NEW

// only need call once on your main function first line!
// create log file, do not close it at end of main, because crt will write log to it.
// dump is in warn level. let warn log to file, debug console and window .
#define INIT_MEM_CHECK() {\
    void *_hDbgLogFile = CreateFile((LPCSTR)"./memleak.log", GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ,\
	                             NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL); \
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG /*| _CRTDBG_MODE_WNDW*/); \
    _CrtSetReportFile(_CRT_WARN, _hDbgLogFile); \
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_LEAK_CHECK_DF); \
}
// nothing to do on deinit
#define DEINIT_MEM_CHECK() 
#endif // _DEBUG && !_ENABLE_LCU_MEM_CHECK_FEATURE
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

#if(!defined(_CRTDBG_MAP_ALLOC) && defined(_ENABLE_LCU_MEM_CHECK_FEATURE))
// to mark we really use lcu memory check feature
#define _USE_LCU_MEM_CHECK
#include "mem/allocator.h"
#include "mem/allocation_tracker.h"

#ifdef __cplusplus
void* operator new(size_t size, const char* fileName, const char* funcName, int line);
void* operator new[](size_t size, const char* fileName, const char* funcName, int line);
void  operator delete(void* ptr) noexcept;
void  operator delete[](void* ptr) noexcept;

void* operator new(size_t size, const char* fileName, const char* funcName, int line)
{
	// here we are not deal with new(0), but it is ok,
	// because if user change return pointer's memory, it will trigger memory corruption on delete it.
	return lcu_malloc_trace(size, fileName, funcName, line);
}

void* operator new[](size_t size, const char* fileName, const char* funcName, int line)
{
	return operator new(size, fileName, funcName, line);
}

void operator delete(void* ptr) noexcept
{
	if (ptr == nullptr)
	{
		return;
	}
	lcu_free(ptr);
}

void operator delete[](void* ptr) noexcept
{
	if (ptr == nullptr)
	{
		return;
	}
	operator delete(ptr);
}

#define new new(__FILE__, __func__, __LINE__)
#endif // __cplusplus

#define INIT_MEM_CHECK()   allocation_tracker_init()
#define DEINIT_MEM_CHECK() do{ allocation_tracker_expect_no_allocations(NULL, NULL); allocation_tracker_uninit(); }while (0)

#if(defined(free) || defined(malloc) || defined(calloc) || defined(realloc) || defined(strdup) || defined(strndup))
#error free/malloc/calloc/realloc/strdup/strndup is defined. you should put "mem_debug.h" on your source file first line.
#endif 
#define free(p)            lcu_free(p)
#define malloc(s)          lcu_malloc_trace(s, __FILE__, __func__, __LINE__)
#define calloc(c, s)       lcu_calloc_trace(c, s, __FILE__, __func__, __LINE__)
#define realloc(p, s)      lcu_realloc_trace(p, s, __FILE__, __func__, __LINE__)
#define strdup(p)          lcu_strdup_trace(p, __FILE__, __func__, __LINE__)
#define strndup(p, s)      lcu_strndup_trace(p, s, __FILE__, __func__, __LINE__)

#endif // !_CRTDBG_MAP_ALLOC && _ENABLE_LCU_MEM_CHECK_FEATURE

#ifndef INIT_MEM_CHECK 
#define INIT_MEM_CHECK() 
#define DEINIT_MEM_CHECK() 
#endif // ! INIT_MEM_CHECK 

#endif // LCU_MEM_DEBUG_H
