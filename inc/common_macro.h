#pragma once
#ifndef __LCU_COMMON_MACRO_H__
#define __LCU_COMMON_MACRO_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#ifdef _WIN32
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

#ifndef EXTERN_C
#ifdef __cplusplus
#define EXTERN_C       extern "C"
#define EXTERN_C_START extern "C" {
#define EXTERN_C_END   }
#else
#define EXTERN_C       extern
#define EXTERN_C_START
#define EXTERN_C_END
#endif // __cplusplus
#endif // !EXTERN_C

EXTERN_C_START

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

//for size_t ssize_t
#ifdef _WIN32
#ifdef _WIN64
#ifndef _SIZE_T_DEFINED
#define _SIZE_T_DEFINED
typedef unsigned __int64    size_t;
#endif // !_SIZE_T_DEFINED
#ifndef _SSIZE_T_DEFINED
#define _SSIZE_T_DEFINED
typedef __int64             ssize_t;
#endif // !_SSIZE_T_DEFINED
#else
#ifndef _SIZE_T_DEFINED
#define _SIZE_T_DEFINED
typedef unsigned int     size_t;
#endif // !_SIZE_T_DEFINED
#ifndef _SSIZE_T_DEFINED
#define _SSIZE_T_DEFINED
typedef int              ssize_t;
#endif // !_SSIZE_T_DEFINED
#endif // _WIN64
#endif // _WIN32

// Minimum and maximum macros
#ifndef __max
#define __max(a,b) (((a) > (b)) ? (a) : (b))
#endif // !__max
#ifndef __min
#define __min(a,b) (((a) < (b)) ? (a) : (b))
#endif // !__min

#ifndef FREE
#define FREE(ptr) if(ptr) { free(ptr); (ptr) = NULL; }
#endif

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define INVALID_FD (-1)
#define CONCAT(a, b) a##b
#define STRING(a) #a

// Use during compile time to check conditional values
// NOTE: The the failures will present as a generic error
// "error: initialization makes pointer from integer without a cast"
// but the file and line number will present the condition that
// failed.
#define DUMMY_COUNTER(c) CONCAT(__osi_dummy_, c)
#define DUMMY_PTR DUMMY_COUNTER(__COUNTER__)

// base/macros.h defines a COMPILE_ASSERT macro to the C++11 keyword
// "static_assert" (it undef's COMPILE_ASSERT before redefining it).
// C++ code that includes base and osi/include/osi.h can thus easily default to
// the definition from libbase but we should check here to avoid compile errors.
#ifndef COMPILE_ASSERT
#define COMPILE_ASSERT(COND) typedef int failed_compile_assert[(COND) ? 1 : -1] __attribute__ ((unused))
#endif  // !COMPILE_ASSERT

#ifndef ASSERT_RET_VOID
#define ASSERT_RET_VOID(condition) if((condition) == false) { assert(false); return; }
#endif // !ASSERT_VOID
#ifndef ASSERT_RET_VALUE
#define ASSERT_RET_VALUE(condition, ret) if((condition) == false) { assert(false); return ret; }
#endif // !ASSERT_RETURN
#ifndef ASSERT_ABORT
#define ASSERT_ABORT(condition) if ((condition) == false) { abort(); }
#endif //!ASSERT_ABORT

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
        FILE* _ftemp =  NULL;
        fopen_s(&_ftemp, _FileName, _Mode);
        return _ftemp;
    }
//to make MSC happy
#define fopen __fopen_safe
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif
#define fclose(fp) if(fp){ fclose(fp); (fp) = NULL; }

EXTERN_C_END

#endif // __LCU_COMMON_MACRO_H__