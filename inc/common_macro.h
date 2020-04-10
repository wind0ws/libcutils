/******************************************************************************
 *
 *  Copyright (C) 2016 Google, Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/
#pragma once
#ifndef __COMMON_MACRO_H__
#define __COMMON_MACRO_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#include <sal.h>
#define UNUSED_ATTR 
#else
#define UNUSED_ATTR __attribute__((unused))
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
    static FILE* _fopen_safe(char const* _FileName, char const* _Mode)
    {
        FILE* _ftemp =  NULL;
        fopen_s(&_ftemp, _FileName, _Mode);
        return _ftemp;
    }
//to make MSC happy
#define fopen _fopen_safe
#define snprintf(buf, buf_size, format, ...) \
        _snprintf_s(buf, buf_size, (buf_size) - 1, format, ## __VA_ARGS__)
#endif
#define fclose(fp) if(fp){ fclose(fp); (fp) = NULL; }

#ifdef __cplusplus
}
#endif
#endif //__COMMON_MACRO_H__