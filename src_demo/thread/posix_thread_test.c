#include "thread/portable_thread.h"
#define LOG_TAG "THREAD_TEST"
#include "log/logger.h"
#include <string.h>

static void* thread_test_func(void* args)
{
	if (!args)
	{
		goto EXIT;
	}
	int code;
	THREAD_SETNAME_FOR_CURRENT("thr_test");
	LOGI("Hello pthread. id:%ld, now sem_wait...", (long)GETTID());
	portable_sem_t* psem = (portable_sem_t*)args;
	if (0 != (code = portable_sem_wait(psem)))
	{
		LOGE_TRACE("error on sem_wait, code=%d", code);
	}

EXIT:
	LOGW_TRACE("sem_wait exited, thread now exit");
	return NULL;
}

static int sem_test()
{
	int code;
	portable_sem_t sem;
	LOGD_TRACE(" --> ");

	memset(&sem, 0, sizeof(sem));
	portable_sem_t* psem = &sem;
	portable_sem_init(psem, 0, 0);

	portable_thread_t thread;
	portable_thread_create(&thread, NULL, thread_test_func, psem);

	sleep(1);
	LOGD("now sem_post...");
	if ((code = portable_sem_post(psem)))
	{
		LOGE("error on sem_post. code=%d", code);
	}
	portable_thread_join(thread, NULL);

	portable_sem_destroy(psem);
	LOGD_TRACE(" <-- ");
	return 0;
}

static void test_log()
{
	LOGD("\n%s\nHello! current log_priority=%d\n%s", 
		LOG_STAR_LINE, LOG_GET_MIN_LEVEL(),  LOG_STAR_LINE);

	LOGW_TRACE("hello, %d, xx=%s", 1234, "WORLD");
	TLOGE("MyTag", "test TLOGE");
	TLOGD_TRACE("MyTag", "test TLOGD_TRACE");

	LOGV("this log is printed by LOGV");
	LOGD("this log is printed by LOGD");
	LOGI("this log is printed by LOGI");
	LOGW("this log is printed by LOGW");
	LOGE("this log is printed by LOGE");

	LOGV_TRACE("this log is printed by LOGV_TRACE");
	LOGD_TRACE("this log is printed by LOGD_TRACE");
	LOGI_TRACE("this log is printed by LOGI_TRACE");
	LOGW_TRACE("this log is printed by LOGW_TRACE");
	LOGE_TRACE("this log is printed by LOGE_TRACE");

	TLOGV(LOG_TAG, "this log is printed by TLOGV");
	TLOGD(LOG_TAG, "this log is printed by TLOGD");
	TLOGI(LOG_TAG, "this log is printed by TLOGI");
	TLOGW(LOG_TAG, "this log is printed by TLOGW");
	TLOGE(LOG_TAG, "this log is printed by TLOGE");

	TLOGV_TRACE(LOG_TAG, "this log is printed by TLOGV_TRACE");
	TLOGD_TRACE(LOG_TAG, "this log is printed by TLOGD_TRACE");
	TLOGI_TRACE(LOG_TAG, "this log is printed by TLOGI_TRACE");
	TLOGW_TRACE(LOG_TAG, "this log is printed by TLOGW_TRACE");
	TLOGE_TRACE(LOG_TAG, "this log is printed by TLOGE_TRACE");

	const char chars[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x10, 0xA0, 0xAF, 0xFD, 0xFE, 0xFF };
	TLOGV_HEX("MyTag", chars, sizeof(chars));
	LOGI_HEX(chars, sizeof(chars));

	LOGD("there is a long long story: \n\
		The Three Little Pigs Story \n\
		Once, the three little pigs left their home as it was time they built their own houses. \n\
\n\
		The mother pig advised them to do their best.\n\
\n\
		The first pig took the easiest way and built a house of hay.\n\
\n\
		The second pig built a house with sticks, which was a little stronger than the first one.\n\
\n\
		The third pig remembered mother's advice and built a house with bricks.\n\
\n\
		Soon, a wolf came by. He huffed and blew the hay house.\n\
\n\
		The pig ran to the stick house. The wolf huffed and puffed and the stick house also came down.\n\
\n\
		The two pigs ran to the brick house. The wolf huffed and puffed but the house remained intact.\n\
\n\
		The pigs knew that the wolf would enter through the chimney and kept a pot full of boiling water under it. The wolf fell right into the pot.\n\
\n\
		Burnt, the wolf ran away but the pigs learnt their lesson.");

	LOGD("another long log: \n\
		[main]\n\
		test = 2\n\
		test1 = 2\n\
		test2 = -1\n\
		test3 = 0.070\n\
		test4 = 3\n\
		test5 = true\n\
		test6 = 1\n\
		test7 = false\n\
		test8 = 0\n\
\n\
		[test]\n\
		test_on = false\n\
\n\
		[test]\n\
		test_on = true\n\
		test_debug = false\n\
		test_level = 5\n\
		test_size = 512\n\
		test_path = /data/user/0/com.example/files/test\n\
\n\
		[test1]\n\
		test_on = true\n\
		test_debug = false\n\
		test_int = 20\n\
\n\
		[test2]\n\
		test_debug = false\n\
		test_on = true\n\
\n\
		[test3]\n\
		test_on = true\n\
		test_debug = false\n\
\n\
		[test4]\n\
		test_on = true\n\
		test_overscale = true\n\
\n\
		[test5]\n\
		test_zero = true\n\
		test_on = true\n\
		test_debug = false\n\
		test_post = false\n\
		#test_model00 = /data/user/0/com.example/lib/libtest.so\n\
		test_model11 =\n\
\n\
		test_on = true\n\
		test_model = /data/user/0/com.example/lib/libtest.so\n\
		test_model_rec =\n\
		test_delay_on = false\n\
\n\
		test_on0 = false\n\
		test_model0 = /data/user/0/com.example/lib/libtest.so\n\
\n\
		test_on1 = true\n\
		test_model1 = /data/user/0/com.example/lib/libtest.so\n\
		test_model_type = cldnn\n\
		test_on2 = true\n\
		test_dynamic = true\n\
		test_delay = 5\n\
\n\
		test_on3 = true\n\
		test_model3 = /data/user/0/com.example/lib/libtest.so\n\
		test_on3 = true\n\
		test_model3 = /data/user/0/com.example/lib/libtest.so\n\
		\n");

	LOG_SET_MIN_LEVEL(LOG_LEVEL_OFF);
	LOGE("this log won't print because of min_level = LOG_LEVEL_OFF");
	LOG_SET_MIN_LEVEL(LOG_LEVEL_INFO);
	LOGD("this debug level log won't print because of min_level = LOG_LEVEL_INFO");

#ifdef LCU_XLOG_H
	xlog_set_min_level(LOG_LEVEL_ERROR);
	xlog_set_target(LOG_TARGET_ANDROID);
	LOGI("this log won't print because of current level is LOG_LEVEL_ERROR");
	LOGE("this log only print on logcat because of xlog_config_target=LOG_TARGET_ANDROID");

	xlog_set_target(LOG_TARGET_CONSOLE);
	LOGE_TRACE("this log only print on console because of xlog_config_target=LOG_TARGET_CONSOLE");

	xlog_set_min_level(LOG_LEVEL_VERBOSE);
	xlog_set_target(LOG_TARGET_ANDROID | LOG_TARGET_CONSOLE);
	LOGD("already set min_level to VERBOSE, target to 'LOG_TARGET_ANDROID | LOG_TARGET_CONSOLE'");

	int cur_format = xlog_get_format();
	xlog_set_format(LOG_FORMAT_RAW);
	TLOGI("MyTag", "this log only print log msg without timestamp/tag/level/tid");
	xlog_set_format(cur_format);
	LOGI("test LOGI with format(%d)", cur_format);
#endif // LCU_XLOG_H

	LOGI("LOG test finished!!!");
}

int posix_thread_test()
{
	LOGD("Hello World, thread id: %d", (int)GETTID());
	test_log();

	sem_test();

	return 0;
}
