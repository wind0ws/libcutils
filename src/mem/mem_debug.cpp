#include "mem/mem_debug.h"
#include "mem/allocator.h"

#ifndef _CRTDBG_MAP_ALLOC
#undef new

void* operator new(size_t size, const char* fileName, const char* funcName, int line)
{
	return lcu_malloc_trace(size, fileName, funcName, line);
}

void* operator new[](size_t size, const char* fileName, const char* funcName, int line)
{
	return operator new(size, fileName, funcName, line);
}

void operator delete(void* pMem) noexcept
{
	if (pMem == nullptr)
	{
		return;
	}
	lcu_free(pMem);
}

void operator delete[](void* pMem) noexcept
{
	if (pMem == nullptr)
	{
		return;
	}
	operator delete(pMem);
}

#endif // !_CRTDBG_MAP_ALLOC


