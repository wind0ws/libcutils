/*
 * dlfcn-win32
 * Copyright (c) 2007 Ramiro Polla
 *
 * dlfcn-win32 is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * dlfcn-win32 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with dlfcn-win32; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * Reference: https://github.com/dlfcn-win32/dlfcn-win32
 */
#pragma once
#ifndef LCU_DLFCN_WINDOWS_H
#define LCU_DLFCN_WINDOWS_H

 //do not include this header directly, you should use " #include "dlfcn.h" " instead!
#ifdef _WIN32

#ifdef __cplusplus
extern "C" {
#endif

#if defined(DLFCN_WIN32_SHARED)
#if defined(DLFCN_WIN32_EXPORTS)
#   define DLFCN_EXPORT __declspec(dllexport)
#else
#   define DLFCN_EXPORT __declspec(dllimport)
#endif
#else
#   define DLFCN_EXPORT
#endif

/* Relocations are performed when the object is loaded. */
#define RTLD_NOW    0

/* Relocations are performed at an implementation-defined time.
 * Windows API does not support lazy symbol resolving (when first reference
 * to a given symbol occurs). So RTLD_LAZY implementation is same as RTLD_NOW.
 */
#define RTLD_LAZY   RTLD_NOW

 /* All symbols are available for relocation processing of other modules. */
#define RTLD_GLOBAL (1 << 1)

/* All symbols are not made available for relocation processing by other modules. */
#define RTLD_LOCAL  (1 << 2)

/* These two were added in The Open Group Base Specifications Issue 6.
 * Note: All other RTLD_* flags in any dlfcn.h are not standard compliant.
 */

 /* The symbol lookup happens in the normal global scope. */
#define RTLD_DEFAULT    ((void *)0)

/* Specifies the next object after this one that defines name. */
#define RTLD_NEXT       ((void *)-1)

/* Structure filled in by dladdr() */
typedef struct dl_info
{
	const char* dli_fname;  /* Filename of defining object (thread unsafe and reused on every call to dladdr) */
	void* dli_fbase;  /* Load address of that object */
	const char* dli_sname;  /* Name of nearest lower symbol */
	void* dli_saddr;  /* Exact value of nearest symbol */
} Dl_info;

/* Open a symbol table handle. */
DLFCN_EXPORT void* dlopen(const char* file, int mode);

/* Close a symbol table handle. */
DLFCN_EXPORT int dlclose(void* handle);

/* Get the address of a symbol from a symbol table handle. */
DLFCN_EXPORT void* dlsym(void* handle, const char* name);

/* Get diagnostic information. */
DLFCN_EXPORT char* dlerror(void);

/* Translate address to symbolic information (no POSIX standard) */
DLFCN_EXPORT int dladdr(const void* addr, Dl_info* info);

#ifdef __cplusplus
}
#endif

#endif // _WIN32

#endif /* LCU_DLFCN_WINDOWS_H */
