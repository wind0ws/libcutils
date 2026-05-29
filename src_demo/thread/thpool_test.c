#include "mem/mem_debug.h"
#include <malloc.h>
#include <stdio.h>
#include <time.h>
#include "thread/thpool.h"
#include "thread/posix_thread.h"

#define LOG_TAG "thpool_test"
#include "log/logger.h"

static volatile int g_thpool_test_counter = 0;
static pthread_mutex_t g_thpool_test_lock = PTHREAD_MUTEX_INITIALIZER;

static void pri_thpool_inc_job(void* arg)
{
	(void)arg;
	pthread_mutex_lock(&g_thpool_test_lock);
	g_thpool_test_counter++;
	pthread_mutex_unlock(&g_thpool_test_lock);
}

/* P0-2/P0-3 回归测试: 验证 thpool 任务执行 + 优雅销毁(sticky shutdown). */
int thpool_test(void)
{
	const int NUM_THREADS = 4;
	const int NUM_JOBS = 100;
	g_thpool_test_counter = 0;

	threadpool tp = thpool_init(NUM_THREADS);
	if (!tp)
	{
		LOGE("thpool_init failed");
		return -1;
	}

	for (int i = 0; i < NUM_JOBS; i++)
	{
		thpool_add_work(tp, pri_thpool_inc_job, NULL);
	}

	thpool_wait(tp);

	if (g_thpool_test_counter != NUM_JOBS)
	{
		LOGE("expected %d jobs done, got %d", NUM_JOBS, g_thpool_test_counter);
		thpool_destroy(tp);
		return -2;
	}

	/* P0-2/P0-3 关键验证: destroy 应当快速返回 (sticky shutdown 一次广播即唤醒所有 worker).
	 * 修复前: O(N×1ms) 串行唤醒 + 1 秒 timeout 忙循环.
	 * 修复后: 期望几乎瞬时返回 (< 100ms 即视为优秀). */
	clock_t t_start = clock();
	thpool_destroy(tp);
	double destroy_ms = (double)(clock() - t_start) * 1000.0 / CLOCKS_PER_SEC;
	LOGI("thpool_destroy took %.2f ms (sticky shutdown expected)", destroy_ms);

	if (destroy_ms > 500.0)
	{
		LOGE("thpool_destroy too slow: %.2f ms (sticky shutdown not effective?)", destroy_ms);
		return -3;
	}

	LOGI("thpool_test all passed: counter=%d, destroy=%.2fms", g_thpool_test_counter, destroy_ms);
	return 0;
}

#include "lcu_test_registry.h"
LCU_TEST_REGISTER(thpool_test, "test thread pool");
