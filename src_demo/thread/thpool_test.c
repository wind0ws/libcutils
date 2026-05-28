#include "mem/mem_debug.h"
#include <malloc.h>
#include <stdio.h>
#include "thread/thpool.h"
#include "thread/posix_thread.h"

#define LOG_TAG "thpool_test"
#include "log/logger.h"


int thpool_test(void)
{


	return 0;
}

#include "lcu_test_registry.h"
LCU_TEST_REGISTER(thpool_test, "test thread pool");
