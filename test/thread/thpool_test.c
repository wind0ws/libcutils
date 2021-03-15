#include <malloc.h>
#include <stdio.h>
#include "thread/thpool.h"
#include "thread/thread_wrapper.h"
#include "log/xlog.h"

#define LOG_TAG_THPOOL_TEST "thpool_test"

//===============================TEST POOL==========================================BEGIN
static void task1(void* param) 
{
	TLOGD(LOG_TAG_THPOOL_TEST, "Thread#%u working on task1", (unsigned int)gettid());
}

static void task2(void* param) 
{
	TLOGD(LOG_TAG_THPOOL_TEST, "Thread#%u working on task2", (unsigned int)gettid());
}

static int thpool_test_basic()
{
	TLOGD(LOG_TAG_THPOOL_TEST, "Making threadpool with 4 threads");
	threadpool thpool = thpool_init(4);

	TLOGD(LOG_TAG_THPOOL_TEST, "Adding 40 tasks to threadpool");
	int i;
	for (i = 0; i < 20; i++) 
	{
		thpool_add_work(thpool, (void*)task1, NULL);
		thpool_add_work(thpool, (void*)task2, NULL);
	};

	TLOGI(LOG_TAG_THPOOL_TEST, "Waiting till all the jobs have completed");
	thpool_wait(thpool);
	TLOGD(LOG_TAG_THPOOL_TEST, "Killing threadpool");
	thpool_destroy(thpool);

	return 0;
}
//===============================TEST POOL==========================================END


//===============================TEST WAIT==========================================BEGIN
static void worker_sleep_1(int* secs)
{
	TLOGD(LOG_TAG_THPOOL_TEST, "%d in", gettid());
	sleep(*secs);
	TLOGD(LOG_TAG_THPOOL_TEST, "%d out", gettid());
}

static int thpool_test_wait()
{
	int num_jobs = 8;
	int num_threads = 2;
	int wait_each_job = 0;
	int sleep_per_thread = 2;

	threadpool thpool = thpool_init(num_threads);

	int n;
	for (n = 0; n < num_jobs; n++) 
	{
		thpool_add_work(thpool, (void*)worker_sleep_1, &sleep_per_thread);
		if (wait_each_job)
			thpool_wait(thpool);
	}
	if (!wait_each_job)
		thpool_wait(thpool);

	thpool_destroy(thpool);
	return 0;
}
//===============================TEST WAIT==========================================END


//===============================TEST PAUSE/RESUME==========================================BEGIN
#ifndef _WIN32 /* pause resume only works on linux, because pthread_kill is not work in WIN32  */

static void sleep_4_secs(void* parm) 
{
	const char* task_name = (const char*)parm;
	TLOGD_TRACE(LOG_TAG_THPOOL_TEST, "%s now exec on %d...", task_name, gettid());
	//sleep(4);
	for (int i = 0; i < 100; i++)
	{
		usleep(40000);
		printf("  [%d: %02d ]  ", gettid(), i);
	}
	TLOGD_TRACE(LOG_TAG_THPOOL_TEST, "%s exec finshed on %d...", task_name, gettid());
}

static int thpool_test_pause_resume() 
{
	int num_threads = 3;

	threadpool thpool = thpool_init(num_threads);

	thpool_pause(thpool);

	// Since pool is paused, threads should not start before main's sleep
	thpool_add_work(thpool, (void*)sleep_4_secs, "MyTask0");
	thpool_add_work(thpool, (void*)sleep_4_secs, "MyTask1");

	TLOGD_TRACE(LOG_TAG_THPOOL_TEST, "thpool is paused, 2 work is added. now sleeping 3s...");
	sleep(3);
	TLOGD_TRACE(LOG_TAG_THPOOL_TEST, "awake after 3s... now resume thpool..");

	// Now we will start threads in no-parallel with main
	thpool_resume(thpool);

	sleep(2); // Give some time to threads to get the work

	thpool_destroy(thpool); // Wait for work to finish

	return 0;
}
#endif // !_WIN32
//===============================TEST PAUSE/RESUME==========================================END

int thpool_test()
{
	thpool_test_basic();
	thpool_test_wait();

#ifndef _WIN32
	thpool_test_pause_resume();
#endif

	return 0;
}
