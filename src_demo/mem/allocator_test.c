#include "mem/mem_debug.h"
#include "common_macro.h"
#include "mem/allocation_tracker.h"
#include "mem/strings.h"

#define LOG_TAG "ALLOC_TEST"
#include "log/logger.h"

static void report_leak_memory(void* ptr, size_t size,
	char* leak_file, char* leak_func, int leak_line, void* user_data)
{
	LOGE("deteck leak memory at '%s' (%s:%d), address: 0x%p, size: %zu bytes",
		NULLABLE_STRING(leak_func), NULLABLE_STRING(leak_file), leak_line, ptr, size);
}

int allocator_test()
{
	char* str1 = (char *)malloc(16);
	strcpy(str1, "This is str1.");
	LOGD("str1 => %s", str1);
	FREE(str1);
	allocation_tracker_expect_no_allocations(report_leak_memory, NULL);

	char* str2 = (char*)calloc(1, 32);
	strcpy(str2, "This is str2.");
	LOGD("str2 => %s", str2);
	//free(str2);
	//oops, str2 is leaked, here we should report str2 leak.
	allocation_tracker_expect_no_allocations(report_leak_memory, NULL);

	//char* str4 = calloc(1, 32);
	//strcpy(str4, "This is str4.");
	//LOGD("str4 => %s", str4);
	//free(str4);
	//free(str4);

	return 0;
}