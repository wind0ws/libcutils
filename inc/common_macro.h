#pragma once
#ifndef __LCU_COMMON_MACRO_H
#define __LCU_COMMON_MACRO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#ifdef _WIN32
#include <crtdbg.h>
#include <sal.h>
#define UNUSED_ATTR 
#else
#define UNUSED_ATTR __attribute__((unused))
typedef void* HANDLE;
typedef void* PVOID;
typedef unsigned long       DWORD;
typedef int                 BOOL;
typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef float               FLOAT;
#endif // _WIN32

#ifndef __in
#define __in
#endif
#ifndef __out
#define __out
#endif
#ifndef __inout
#define __inout
#endif
#ifndef __in_opt
#define __in_opt
#endif
#ifndef __out_opt
#define __out_opt
#endif
#ifndef __inout_opt
#define __inout_opt
#endif

#if defined(_MSC_VER) //  Microsoft 
#define API_EXPORT __declspec(dllexport)
#define API_IMPORT __declspec(dllimport)
#elif defined(__GNUC__) //  GCC
#define API_EXPORT __attribute__((visibility("default")))
#define API_IMPORT
#else
//  do nothing and hope for the best?
#define API_EXPORT
#define API_IMPORT
#pragma warning Unknown dynamic link import/export semantics.
#endif

#ifndef EXTERN_C
#ifdef __cplusplus
#define EXTERN_C       extern "C"
#else
#define EXTERN_C       extern
#endif // __cplusplus
#endif // !EXTERN_C
#ifndef EXTERN_C_START
#ifdef __cplusplus
#define EXTERN_C_START extern "C" {
#define EXTERN_C_END   }
#else
#define EXTERN_C_START
#define EXTERN_C_END
#endif // __cplusplus
#endif // !EXTERN_C_START

EXTERN_C_START

//for size_t ssize_t. Note: in _WIN64 build system, _WIN32 is also defined.
#ifdef _WIN32
#if !defined(_SSIZE_T_) && !defined(_SSIZE_T_DEFINED)
typedef intptr_t ssize_t;
# define SSIZE_MAX INTPTR_MAX
# define _SSIZE_T_
# define _SSIZE_T_DEFINED
#endif
#endif // _WIN32

#if(defined(_MSC_VER) && _MSC_VER < 1800)
/* __LP64__ is defined by compiler. If set to 1 means this is 64bit build system */
#ifdef __LP64__
#define SIZE_T_FORMAT "%lu"
#define SSIZE_T_FORMAT "%ld"
#else
#define SIZE_T_FORMAT "%u"
#define SSIZE_T_FORMAT "%d"
#endif
#else
#define SIZE_T_FORMAT "%zu"
#define SSIZE_T_FORMAT "%zd"
#endif // _MSC_VER

// widely useful macros. pay attention to the influence of double computation!
#ifndef __max
#define __max(a,b) (((a) > (b)) ? (a) : (b))
#endif // !__max
#ifndef __min
#define __min(a,b) (((a) < (b)) ? (a) : (b))
#endif // !__min
#ifndef __abs
#define __abs(x) ((x) >= 0 ? (x) : -(x))  
#endif // !__abs


#ifndef FREE
#define FREE(ptr) if(ptr) { free(ptr); (ptr) = NULL; }
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif // !ARRAY_SIZE
#ifndef INVALID_FD
#define INVALID_FD (-1)
#endif // !INVALID_FD
#ifndef CONCAT
#define CONCAT(a, b) a##b
#endif // !CONCAT
#ifndef STRING
#define STRING(a) #a
#endif // !STRING

// Use during compile time to check conditional values
// NOTE: The the failures will present as a generic error
// "error: initialization makes pointer from integer without a cast"
// but the file and line number will present the condition that
// failed.
#define DUMMY_COUNTER(c) CONCAT(__osi_dummy_, c)
#define DUMMY_PTR DUMMY_COUNTER(__COUNTER__)

#ifndef STATIC_ASSERT
#define STATIC_ASSERT_WITH_MSG(expr, msg) typedef char __static_assert_t_##msg[(expr) != 0]
#define STATIC_ASSERT(expr) STATIC_ASSERT_WITH_MSG(expr, _)
#endif  // !STATIC_ASSERT

#ifndef ASSERT
#ifdef _WIN32
#define ASSERT(expr) _ASSERT_AND_INVOKE_WATSON(expr)
#else
#define ASSERT(expr) assert(expr)
#endif
#endif // !ASSERT

// Macros for safe integer to pointer conversion. In the C language, data is
// commonly cast to opaque pointer containers and back for generic parameter
// passing in callbacks. These macros should be used sparingly in new code
// (never in C++ code). Whenever integers need to be passed as a pointer, use
// these macros.
#define PTR_TO_UINT(p) ((unsigned int) ((uintptr_t) (p)))
#define UINT_TO_PTR(u) ((void *) ((uintptr_t) (u)))
#define PTR_TO_INT(p) ((int) ((intptr_t) (p)))
#define INT_TO_PTR(i) ((void *) ((intptr_t) (i)))

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#define F_OK 0
#define X_OK 1
#define W_OK 2
#define R_OK 4
//just to make MSC happy
#define access _access
#define mkdir(path, mode) _mkdir(path)
#define read _read
#define write _write

static inline FILE* __fopen_safe(char const* _FileName, char const* _Mode)
{
	FILE* _ftemp = NULL;
	fopen_s(&_ftemp, _FileName, _Mode);
	return _ftemp;
}
//to make MSC happy
#define fopen __fopen_safe
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif // _WIN32
#define fclose(fp) if(fp){ fclose(fp); (fp) = NULL; }


EXTERN_C_END

#endif // __LCU_COMMON_MACRO_H