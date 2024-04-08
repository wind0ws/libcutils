#pragma once
#ifndef LCU_COMMON_MACRO_H
#define LCU_COMMON_MACRO_H

#include <stdbool.h>        /* for true/false                    */
#include <stddef.h>         /* for size_t                        */
#include <stdint.h>         /* for int32_t                       */
#include <stdlib.h>         /* for abort,random,free             */
#include <stdio.h>          /* for FILE                          */
#include <assert.h>         /* for assert                        */
#include <sys/types.h>      /* for ssize_t                       */

#if(defined(__linux__) || defined(__ANDROID__))
#include <sys/cdefs.h>    /* for __BEGIN_DECLS                 */
#endif // __linux__ || __ANDROID__

#ifdef __ANDROID__
#include <android/log.h>  /* for log msg on logcat             */
#endif // __ANDROID__

#ifdef _WIN32
#include <crtdbg.h>       /* for _ASSERT_AND_INVOKE_WATSON     */
#include <sal.h>          /* for annotation: mark parameters   */
#ifndef __func__
#define __func__ __FUNCTION__
#endif // !__func__
#ifndef __PRETTY_FUNCTION__
#define __PRETTY_FUNCTION__ __FUNCSIG__ 
#endif // !__PRETTY_FUNCTION__
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

// annotation: for mark parameters
// on windows, sal.h already defined it.
// param in: which only read
#ifndef __in
#define __in
#endif // !__in
// param out: which only write
#ifndef __out
#define __out
#endif // !__out
// param in,out: both read/write it
#ifndef __inout
#define __inout
#endif // !__inout
// param in, can be NULL
#ifndef __in_opt
#define __in_opt
#endif // !__in_opt
// param out, can be NULL
#ifndef __out_opt
#define __out_opt
#endif // !__out_opt
// param in,out, can be NULL
#ifndef __inout_opt
#define __inout_opt
#endif // !__inout_opt
// mark success path, eg: __success(return == 0)
#ifndef __success
#define __success(expr) 
#endif // !__success

// for API_EXPORT/IMPORT
#if defined(_MSC_VER) //  Microsoft 
#ifdef _EXPORTING
#define API_DECLSPEC __declspec(dllexport)
#else
#define API_DECLSPEC __declspec(dllimport)
#endif // _EXPORTING
#define API_VISIBLE 
#define API_HIDDEN 
#elif defined(__GNUC__) //  GCC
#define API_DECLSPEC 
#define API_VISIBLE __attribute__((visibility("default")))
#define API_HIDDEN  __attribute__((visibility("hidden")))
#else //  do nothing and hope for the best?
#define API_DECLSPEC 
#define API_VISIBLE 
#define API_HIDDEN 
#pragma warning Unknown dynamic link import/export/hidden semantics.
#endif // _MSC_VER

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

//for size_t/ssize_t. Note: in _WIN64 build system, _WIN32 is also defined.
//msvc ONLY defined size_t on vcruntime.h. so we need define ssize_t
#ifdef _WIN32
#if (!defined(_SSIZE_T_) && !defined(_SSIZE_T_DEFINED))
typedef intptr_t ssize_t;
#define SSIZE_MAX INTPTR_MAX
#define _SSIZE_T_      
#define _SSIZE_T_DEFINED 
#endif // !_SSIZE_T_ && !_SSIZE_T_DEFINED
#if (defined(_MSC_VER) && _MSC_VER < 1800)
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
#define FREE(ptr) do{ if(ptr) { free(ptr); (ptr) = NULL; } }while(0)
#endif // !FREE

#ifndef ARRAY_LEN
#define ARRAY_LEN(x) (sizeof(x) / sizeof((x)[0]))
#endif // !ARRAY_LEN
#ifndef INVALID_FD
#define INVALID_FD (-1)
#endif // !INVALID_FD
#ifndef CONCAT
#define CONCAT(a, b) a##b
#endif // !CONCAT
#ifndef STRINGIFY
#define STRINGIFY(a) #a
#endif // !STRINGIFY
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
#define _TEMP_FOR_EXPAND_STATIC_ASSERT_WITH_MSG(expr, msg) STATIC_ASSERT_WITH_MSG(expr, msg)
#define STATIC_ASSERT(expr) _TEMP_FOR_EXPAND_STATIC_ASSERT_WITH_MSG(expr, __LINE__)
#endif  // !STATIC_ASSERT

#ifdef __ANDROID__
#define _EMERGENCY_LOG_FOR_PLATFORM(fmt, ...) __android_log_print(ANDROID_LOG_ERROR, "DEBUG", fmt, ##__VA_ARGS__)
#else
#define _EMERGENCY_LOG_FOR_PLATFORM(fmt, ...) 
#endif // __ANDROID__

//for log emergency error message.
#define EMERGENCY_LOG(fmt, ...)                                                     \
     do {                                                                           \
          _EMERGENCY_LOG_FOR_PLATFORM(fmt, ##__VA_ARGS__);                          \
          printf("[DEBUG] " fmt "\n", ##__VA_ARGS__);                               \
          fflush(stdout);                                                           \
     } while(0)

#ifndef ASSERT
#if(defined(NDEBUG) || !defined(_DEBUG))
#define ASSERT(expr)  (void)(expr)
#define _TEMP_FOR_ASSERT_ABORT(expr, line)                                          \
     do {                                                                           \
          if (expr) break;                                                          \
          EMERGENCY_LOG("API check '%s' failed at '%s' (%s:%d)",                    \
	          #expr, __func__, __FILE__, line);                                     \
	      abort();                                                                  \
     } while(0)
#else
#ifdef _WIN32
//why we not use _ASSERT_AND_INVOKE_WATSON directly? Because we don't want to be affected by double computation!
#define _TEMP_FOR_ASSERT_AND_INVOKE_WATSON(expr, line)                              \
    do {                                                                            \
          bool expr_##line = !!(expr);                                              \
          if(expr_##line) break;                                                    \
          _ASSERT_EXPR(expr_##line, _CRT_WIDE(#expr));                              \
    } while(0)
#define _TEMP_FOR_EXPAND_ASSERT_AND_INVOKE_WATSON(expr, line) _TEMP_FOR_ASSERT_AND_INVOKE_WATSON(expr, line)
#define ASSERT(expr) _TEMP_FOR_EXPAND_ASSERT_AND_INVOKE_WATSON(expr, __LINE__)
#define _TEMP_FOR_ASSERT_ABORT(expr, line)  _TEMP_FOR_EXPAND_ASSERT_AND_INVOKE_WATSON(expr, line)
#else
#define ASSERT(expr) assert(expr)
#define _TEMP_FOR_ASSERT_ABORT(expr, line)  assert(expr)
#endif // _WIN32
#endif // NDEBUG || !_DEBUG
#endif // !ASSERT

#define _TEMP_FOR_EXPAND_ASSERT_ABORT(expr, line)  _TEMP_FOR_ASSERT_ABORT(expr, line)
/**
 * In debug mode, expression check failure will catch by ASSERT.
 * In release mode, it will log error to stdout/logcat and abort program.
 */
#define ASSERT_ABORT(expr)  _TEMP_FOR_EXPAND_ASSERT_ABORT(expr, __LINE__)

#ifndef __cplusplus
 // Macros for safe integer to pointer conversion. In the C language, data is
 // commonly cast to opaque pointer containers and back for generic parameter
 // passing in callbacks. These macros should be used sparingly in new code
 // (never in C++ code). 
 // Whenever integers need to be passed as a pointer, use these macros. 
 // for compatible with 64bit OS, suggest use PTR_TO_LONG64 and LONG64_TO_PTR
#define PTR_TO_UINT(p)    ((uintptr_t) (p)))
#define UINT_TO_PTR(u)    ((void *) ((uintptr_t) (u)))
#define PTR_TO_INT(p)     (((intptr_t) (p))
#define INT_TO_PTR(i)     ((void *) ((intptr_t) (i)))
#define PTR_TO_LONG64(p)  ((long long) ((intptr_t) (p)))
#define LONG64_TO_PTR(i)  ((void *) ((intptr_t) (i)))
#endif // !__cplusplus

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

static inline FILE* _fopen_safe(char const* _FileName, char const* _Mode)
{
	FILE* _ftemp = NULL;
	fopen_s(&_ftemp, _FileName, _Mode);
	return _ftemp;
}
//to make MSC happy
#define fopen(file_name, mode) _fopen_safe(file_name, mode)
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif // _WIN32
#define fclose(fp) do{if(fp){ fclose(fp); (fp) = NULL; }}while(0)

#ifndef RANDOM
#define RANDOM_INIT(seed)   srand((seed))
#define RANDOM(a, b)        (rand() % ((b) - (a)) + (a))
#endif // !RANDOM

//================================DECLARE ENUM AND STRINGS================================
/** how to use:

// def enum and declare enum strings on header
#define FOREACH_STATES_ITEM(GENERATOR)       \
             GENERATOR(STATE_START)          \
             GENERATOR(STATE_STOP)
DEF_ENUM(STATES, FOREACH_STATES_ITEM);
DECLARE_ENUM_STRS(STATES);

// def enum string on source file
DEF_ENUM_STRS(STATES, FOREACH_STATES_ITEM);

// now you can use enum str
printf("STATES[0]=%s\n", STATES_STR[STATE_START]);
printf("STATES[%d]=%s\n", (int)STATE_STOP, STATES_STR[STATE_STOP]);
 */

#define _GENERATOR_ENUM_ITEM(item) item,
#define _GENERATOR_ENUM_STR(item)  #item,
#define _TEMP_FOR_DEF_ENUM(name, foreach_enum, generator_enum_item)       \
        typedef enum name##_ {                                            \
           foreach_enum(generator_enum_item)                              \
        } name;
#define DEF_ENUM(name, foreach_enum) _TEMP_FOR_DEF_ENUM(name, foreach_enum, _GENERATOR_ENUM_ITEM)
#define DECLARE_ENUM_STRS(name)               extern const char *name##_STRS[]
#define _TEMP_FOR_DEF_ENUM_STRS(name, foreach_enum, generator_enum_str)       \
               const char *name##_STRS[] = { foreach_enum(generator_enum_str) };
#define DEF_ENUM_STRS(name, foreach_enum)                                     \
        _TEMP_FOR_DEF_ENUM_STRS(name, foreach_enum, _GENERATOR_ENUM_STR)
//================================DECLARE ENUM AND STRINGS================================


#endif // !LCU_COMMON_MACRO_H
