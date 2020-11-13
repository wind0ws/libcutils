#pragma once
#ifndef LCU_COMMON_MACRO_H
#define LCU_COMMON_MACRO_H

#include <stdbool.h>      /* for true/false    */
#include <stddef.h>       /* for size_t        */
#include <stdint.h>       /* for int32_t       */
#include <stdlib.h>       /* for abort         */
#include <stdio.h>        /* for FILE          */
#include <assert.h>       /* for assert        */
#include <sys/types.h>    /* for ssize_t       */

#if(defined(__linux__) || defined(__ANDROID__))
#include <sys/cdefs.h>    /* for __BEGIN_DECLS */
#endif

#ifdef __ANDROID__
#include <android/log.h>  /* for log error     */
#endif // __ANDROID__

#ifdef _WIN32
#ifndef __func__
#define __func__ __FUNCTION__
#endif // !__func__
#ifndef __PRETTY_FUNCTION__
#define __PRETTY_FUNCTION__ __FUNCSIG__ 
#endif // !__PRETTY_FUNCTION__
#include <crtdbg.h> /* for _ASSERT_AND_INVOKE_WATSON */
#include <sal.h>
#else
typedef void*               HANDLE;
typedef void*               PVOID;
typedef unsigned long       DWORD;
typedef int                 BOOL;
typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef float               FLOAT;
#endif // _WIN32

#ifndef UNUSED
#define UNUSED(x)           (void)(x)
#endif // !UNUSED
#ifndef UNUSED_ATTR
#ifdef _WIN32
#define UNUSED_ATTR 
#else
#define UNUSED_ATTR         __attribute__((unused))
#endif // _WIN32
#endif // !UNUSED_ATTR

//for mark parameters
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
#define API_HIDDEN 
#elif defined(__GNUC__) //  GCC
#define API_EXPORT __attribute__((visibility("default")))
#define API_IMPORT
#define API_HIDDEN __attribute__((visibility("hidden")))
#else
//  do nothing and hope for the best?
#define API_EXPORT
#define API_IMPORT
#define API_HIDDEN
#pragma warning Unknown dynamic link import/export/hidden semantics.
#endif

#ifndef EXTERN_C
#ifdef __cplusplus
#define EXTERN_C         extern "C"
#else
#define EXTERN_C         extern
#endif // __cplusplus
#endif // !EXTERN_C

#ifndef EXTERN_C_START
#ifdef __cplusplus
#define EXTERN_C_START   extern "C" {
#define EXTERN_C_END     }
#else
#define EXTERN_C_START
#define EXTERN_C_END
#endif // __cplusplus
#endif // !EXTERN_C_START

#ifndef __BEGIN_DECLS
#define __BEGIN_DECLS    EXTERN_C_START
#define __END_DECLS      EXTERN_C_END
#endif // !__BEGIN_DECLS

//for size_t ssize_t. Note: in _WIN64 build system, _WIN32 is also defined.
//sigh: windows ONLY defined size_t on vcruntime.h. so we need define ssize_t
#ifdef _WIN32
#if !defined(_SSIZE_T_) && !defined(_SSIZE_T_DEFINED)
typedef intptr_t ssize_t;
# define SSIZE_MAX INTPTR_MAX
# define _SSIZE_T_        // mark ssize_t defined
# define _SSIZE_T_DEFINED // mark ssize_t defined
#endif // !_SSIZE_T_ && !_SSIZE_T_DEFINED
#if(defined(_MSC_VER) && _MSC_VER < 1800)
/* __LP64__ is defined by compiler. If set to 1 means this is 64bit build system */
#ifdef __LP64__
#define SIZE_T_FORMAT   "%lu"
#define SSIZE_T_FORMAT  "%ld"
#else
#define SIZE_T_FORMAT   "%u"
#define SSIZE_T_FORMAT  "%d"
#endif
#else
#define SIZE_T_FORMAT   "%zu"
#define SSIZE_T_FORMAT  "%zd"
#endif // _MSC_VER
#endif // _WIN32

// widely useful macros. pay attention to the influence of double computation!
#ifndef __max
#define __max(a,b) (((a) > (b)) ? (a) : (b))
#endif // !__max
#ifndef __min
#define __min(a,b) (((a) < (b)) ? (a) : (b))
#endif // !__min
#ifndef __abs
#define __abs(x)   ((x) >= 0 ? (x) : -(x))  
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
#ifndef STRINGFY
#define STRINGFY(a) #a
#endif // !STRINGFY
#ifndef NULLABLE_STRING
#define NULLABLE_STRING(a) ((a) ? (a) : "(null)")
#endif // !NULLABLE_STRING

// Use during compile time to check conditional values
// NOTE: The the failures will present as a generic error
// "error: initialization makes pointer from integer without a cast"
// but the file and line number will present the condition that failed
#define DUMMY_COUNTER(c) CONCAT(__lcu_dummy_, c)
#define DUMMY_PTR DUMMY_COUNTER(__COUNTER__)

#ifndef STATIC_ASSERT
#define STATIC_ASSERT_WITH_MSG(expr, msg) typedef char __static_assert_t_##msg[(expr) != 0]
#define __TEMP_FOR_EXPAND_STATIC_ASSERT_WITH_MSG(expr, msg) STATIC_ASSERT_WITH_MSG(expr, msg)
#define STATIC_ASSERT(expr) __TEMP_FOR_EXPAND_STATIC_ASSERT_WITH_MSG(expr, __LINE__)
#endif  // !STATIC_ASSERT

#ifdef __ANDROID__
#define __EMERGENCY_LOG_FOR_PLATFORM(fmt, ...) __android_log_print(ANDROID_LOG_ERROR, "DEBUG", fmt, ##__VA_ARGS__)
#else
#define __EMERGENCY_LOG_FOR_PLATFORM(fmt, ...) 
#endif //__ANDROID__

//for log emergency error message.
#define EMERGENCY_LOG(fmt, ...)                                                      \
     {                                                                               \
        __EMERGENCY_LOG_FOR_PLATFORM(fmt, ##__VA_ARGS__);                            \
        printf("[DEBUG] " fmt "\n", ##__VA_ARGS__);                                  \
        fflush(stdout);                                                              \
     }

#ifndef ASSERT
#if(defined(NDEBUG) || !defined(_DEBUG))
#define ASSERT(expr)  (void)(expr) 
#else
#if _WIN32
//why we not use _ASSERT_AND_INVOKE_WATSON directly? Because we don't want to be affected by double computation!
#define __TEMP_FOR_ASSERT_AND_INVOKE_WATSON(expr, line)                              \
    {                                                                                \
        bool expr_##line = !!(expr);                                                 \
        _ASSERT_EXPR(expr_##line, _CRT_WIDE(#expr));                                 \
        if (!expr_##line)                                                            \
        {                                                                            \
           _invoke_watson(_CRT_WIDE(#expr), __FUNCTIONW__, __FILEW__, __LINE__, 0);  \
        }                                                                            \
    }
#define __TEMP_FOR_EXPAND_ASSERT_AND_INVOKE_WATSON(expr, line) __TEMP_FOR_ASSERT_AND_INVOKE_WATSON(expr, line)
#define ASSERT(expr) __TEMP_FOR_EXPAND_ASSERT_AND_INVOKE_WATSON(expr, __LINE__)
#else
#define ASSERT(expr) assert(expr)
#endif // _WIN32
#endif // NDEBUG || !_DEBUG
#endif // !ASSERT

#define __TEMP_FOR_ASSERT_ABORT(expr, line)                                          \
    {                                                                                \
        ASSERT(expr);                                                                \
        bool is_expr_true##line = !!(expr);                                          \
        if (!is_expr_true##line)                                                     \
        {                                                                            \
           EMERGENCY_LOG("API check '%s' failed at %s (%s:%d)",                   \
                  #expr, __func__, __FILE__, line);                              \
           abort();                                                                  \
        }                                                                            \
     }
#define __TEMP_FOR_EXPAND_ASSERT_ABORT(expr, line)  __TEMP_FOR_ASSERT_ABORT(expr, line)
/**
 * In debug mode, expression check failure will catch by ASSERT.
 * In release mode, it will log error to stdout/logcat and abort program.
 */
#define ASSERT_ABORT(expr)  __TEMP_FOR_EXPAND_ASSERT_ABORT(expr, __LINE__)

#ifndef __cplusplus
 // Macros for safe integer to pointer conversion. In the C language, data is
 // commonly cast to opaque pointer containers and back for generic parameter
 // passing in callbacks. These macros should be used sparingly in new code
 // (never in C++ code). Whenever integers need to be passed as a pointer, use
 // these macros.
#define PTR_TO_UINT(p)  ((unsigned int) ((uintptr_t) (p)))
#define UINT_TO_PTR(u)  ((void *) ((uintptr_t) (u)))
#define PTR_TO_INT(p)   ((int) ((intptr_t) (p)))
#define INT_TO_PTR(i)   ((void *) ((intptr_t) (i)))
#endif // __cplusplus

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#define F_OK 0
#define X_OK 1
#define W_OK 2
#define R_OK 4
//just to make MSC happy
#define access(path_name, mode)   _access(path_name, mode)
#define mkdir(path, mode)         _mkdir(path)
#define read(fd, buf, count)      _read(fd, buf, count)
#define write(fd, buf, count)     _write(fd, buf, count)

static inline FILE* __fopen_safe(char const* _FileName, char const* _Mode)
{
	FILE* _ftemp = NULL;
	fopen_s(&_ftemp, _FileName, _Mode);
	return _ftemp;
}
//to make MSC happy
#define fopen(file_name, mode) __fopen_safe(file_name, mode)
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif // _WIN32
#define fclose(fp) if(fp){ fclose(fp); (fp) = NULL; }

#ifndef RANDOM
#define RANDOM_INIT(seed)  srand(seed)
#define RANDOM(a,b)        (rand() % (b - a) + a)
#endif // RANDOM

#endif // LCU_COMMON_MACRO_H
