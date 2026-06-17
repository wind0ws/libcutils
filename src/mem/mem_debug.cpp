/**
 * @file mem_debug.cpp
 * @brief Single-TU definitions of the global operator new/delete overrides used
 *        by the lcu memory-check feature.
 *
 * Fix #2 (ODR): these global replaceable allocation functions were previously
 * DEFINED inside mem_debug.h, so every C++ translation unit that included the
 * header (per project convention: "include mem_debug.h on the first line")
 * emitted its own definition -> LNK2005 multiple-definition once two .cpp files
 * were linked. They now live here, compiled exactly once into the library.
 *
 * Fix M3: matching placement operator delete overloads are defined so that a
 * constructor throwing after `new T` (which expands to the placement
 * new(file,func,line) form) reclaims the raw storage instead of leaking it.
 *
 * The ENTIRE body is gated on _USE_LCU_MEM_CHECK, mirroring the header. When the
 * memcheck feature is disabled this file is an empty translation unit and does
 * NOT replace the global operators — so linking the library into a client that
 * did not opt into memcheck leaves that client's new/delete untouched.
 *
 * NOTE: This file intentionally does NOT prepend the `#define new ...` rewrite,
 * so the operator definitions below use the real, un-macro'd operator syntax.
 */

/* Pull in the feature gate + lcu allocator decls WITHOUT activating the
 * `#define new`/`#define malloc` rewrites in our own definitions: include
 * mem_debug.h to obtain _USE_LCU_MEM_CHECK and the operator declarations, then
 * the rewrites are harmless here because we never call new/malloc by name. */
#include "mem/mem_debug.h"

#ifdef _USE_LCU_MEM_CHECK

#include "mem/allocator.h"

/* Undo the macro rewrites so the operator/allocator calls below bind to the
 * real functions, not to themselves. */
#ifdef new
#undef new
#endif
#ifdef malloc
#undef malloc
#endif
#ifdef free
#undef free
#endif

void *operator new(size_t size, const char *fileName, const char *funcName, int line)
{
	// We do not special-case new(0): a 0-size allocation still yields a unique,
	// tracked, canary-guarded pointer; writing through it would be caught as a
	// corruption on delete, which is the desired diagnostic.
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

/* Fix M3: placement deletes. The runtime invokes the overload whose signature
 * matches the placement new it paired with, when the object's constructor
 * throws. Routing them to lcu_free reclaims the tracked storage and closes the
 * throwing-constructor leak. */
void operator delete(void *ptr, const char *fileName, const char *funcName, int line) noexcept
{
	(void)fileName;
	(void)funcName;
	(void)line;
	if (nullptr == ptr)
	{
		return;
	}
	lcu_free(ptr);
}

void operator delete[](void *ptr, const char *fileName, const char *funcName, int line) noexcept
{
	operator delete(ptr, fileName, funcName, line);
}

#endif // _USE_LCU_MEM_CHECK
