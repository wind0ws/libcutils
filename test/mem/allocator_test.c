#include "common_macro.h"
#include "allocation_tracker.h"
#include "allocator.h"
#include "xlog.h"
#include "strings.h"

#define LOG_TAG_ALLOC_TEST "alloc_test"

int allocator_test_begin()
{
	allocation_tracker_init();
	return 0;
}

int allocator_test_end()
{
	allocation_tracker_uninit();
	return 0;
}

static void report_leak_memory(void* ptr, size_t size)
{
	TLOGE(LOG_TAG_ALLOC_TEST, "deteck leak mem. addr: 0x%p, size: %zu bytes", ptr, size);
}

int allocator_test()
{
	char *str1 = lcu_malloc(16);
	strcpy(str1, "This is str1.");
	TLOGD(LOG_TAG_ALLOC_TEST,"str1 => %s", str1);
	lcu_free_and_reset(&str1);
	allocation_tracker_expect_no_allocations(report_leak_memory);
	
	char* str2 = lcu_calloc(1, 32);
	strcpy(str2, "This is str2.");
	TLOGD(LOG_TAG_ALLOC_TEST, "str2 => %s", str2);
	//lcu_free(str2);
	//oops, str2 is leaked, here we should report str2 leak.
	allocation_tracker_expect_no_allocations(report_leak_memory);

	return 0;
}