#include "mem/mem_debug.h"
#include "mem/allocator.h"

#ifndef _CRTDBG_MAP_ALLOC
#undef new

void* operator new(size_t size, const char* fileName, const char* funcName, int line)
{
	//here we are not deal with new(0), but it is ok,
	//because if user change return pointer's memory, it will trigger memory corruption on delete it.
	return lcu_malloc_trace(size, fileName, funcName, line);
}

void* operator new[](size_t size, const char* fileName, const char* funcName, int line)
{
	return operator new(size, fileName, funcName, line);
}

void operator delete(void* ptr) noexcept
{
	if (ptr == nullptr)
	{
		return;
	}
	lcu_free(ptr);
}

void operator delete[](void* ptr) noexcept
{
	if (ptr == nullptr)
	{
		return;
	}
	operator delete(ptr);
}

#endif // !_CRTDBG_MAP_ALLOC
