/**
 * include this header file at your source file first line.
 * On windows, in debug mode, it will use crtdbg help you find memory leak.
 * Otherwise, use allocator to trace memory.
 * 
 * Note: use memory debug will slow your program. Be aware of that.
 *       use it if your program memory keep growing.
 */

#pragma once
#ifndef __LCU_MEM_DEBUG_H
#define __LCU_MEM_DEBUG_H

#ifdef _WIN32
#ifndef __func__
#define __func__ __FUNCTION__
#endif // !__func__
#ifndef __PRETTY_FUNCTION__
#define __PRETTY_FUNCTION__ __FUNCSIG__ 
#endif // !__PRETTY_FUNCTION__
#ifdef _DEBUG
 // must keep next 3 line on your top source file,
 // otherwise it won't tell you leak memory on which file with line number.
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

// Replace _NORMAL_BLOCK with _CLIENT_BLOCK if you want the
//allocations to be of _CLIENT_BLOCK type
#define __MYDEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new __MYDEBUG_NEW

//only need call once on your main function first line!
#define INIT_MEM_CHECK()\
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_LEAK_CHECK_DF)

#define DEINIT_MEM_CHECK() 
#endif // _DEBUG
#endif // _WIN32

//common header
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

//define this macro will enable memory check feature
//suggest user add it to compiler on build if you really want to debug memory.
//#define _ENABLE_LCU_MEM_CHECK_FEATURE

#if(!defined(_CRTDBG_MAP_ALLOC) && defined(_ENABLE_LCU_MEM_CHECK_FEATURE))
//to mark we really use lcu mem check feature
#define _USE_LCU_MEM_CHECK
#include "mem/allocator.h"
#include "mem/allocation_tracker.h"

#ifdef __cplusplus
void* operator new(size_t size, const char* fileName, const char* funcName, int line);
void* operator new[](size_t size, const char* fileName, const char* funcName, int line);
void  operator delete(void* ptr) noexcept;
void  operator delete[](void* ptr) noexcept;

#define new new(__FILE__, __func__, __LINE__)
#endif // __cplusplus

#define INIT_MEM_CHECK()   allocation_tracker_init()
#define DEINIT_MEM_CHECK() do{ allocation_tracker_expect_no_allocations(NULL, NULL); allocation_tracker_uninit(); }while (0)

#define free(p)            lcu_free(p)
#define malloc(s)          lcu_malloc_trace(s, __FILE__, __func__, __LINE__)
#define calloc(c, s)       lcu_calloc_trace(c, s, __FILE__, __func__, __LINE__)
#define realloc(p, s)      lcu_realloc_trace(p, s, __FILE__, __func__, __LINE__)
#define strdup(p)          lcu_strdup_trace(p, __FILE__, __func__, __LINE__)
#define strndup(p, s)      lcu_strndup_trace(p, s, __FILE__, __func__, __LINE__)

#endif

#ifndef INIT_MEM_CHECK 
#define INIT_MEM_CHECK() 
#define DEINIT_MEM_CHECK() 
#endif // ! INIT_MEM_CHECK 


#endif // __LCU_MEM_DEBUG_H