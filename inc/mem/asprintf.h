#pragma once
#ifndef LCU_ASPRINTF_H
#define LCU_ASPRINTF_H

//reference https://stackoverflow.com/questions/40159892/using-asprintf-on-windows/49873938#49873938

#if defined(__GNUC__) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE /* needed for (v)asprintf, affects '#include <stdio.h>' */
#endif // __GNUC__
#include <stdio.h>  /* needed for vsnprintf    */
#include <malloc.h> /* needed for malloc, free */
#include <stdarg.h> /* needed for va_*         */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * vscprintf:
 * MSVC implements this as _vscprintf, thus we just 'symlink' it here
 * GNU-C-compatible compilers do not implement this, thus we implement it here
 */
#ifdef _MSC_VER
#define vscprintf _vscprintf
#endif // _MSC_VER

#ifdef __GNUC__
    int vscprintf(const char* format, va_list ap);
#endif // __GNUC__

/*
 * asprintf, vasprintf:
 * MSVC does not implement these, thus we implement them here
 * GNU-C-compatible compilers implement these with the same names, 
 * thus we don't have to do anything
 */
#ifdef _MSC_VER
    int vasprintf(char** strp, const char* format, va_list ap);

    int asprintf(char** strp, const char* format, ...);
#endif // _MSC_VER

#ifdef __cplusplus
}
#endif

#endif // !LCU_ASPRINTF_H
