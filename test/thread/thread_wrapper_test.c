#include "thread/thread_wrapper.h"
#include "log/xlog.h"

static void* thread_func(void* args) 
{
	int code;
	pthread_setname_np(pthread_self(), "thr_func");
	LOGI("Hello pthread. id:%d", gettid());
	sem_t* psem = args;
	if ((code = sem_wait(psem)))
	{
		LOGE("error on sem_wait, code=%d", code);
	}
	LOGW("pthread now exit");
	return NULL;
}

static int sem_test()
{
	int code;

	sem_t sem;
	memset(&sem, 0, sizeof(sem));
	sem_t* psem = &sem;
	sem_init(psem, 0, 0);

	pthread_t thread;
	pthread_create(&thread, NULL, thread_func, psem);

	sleep(1);
	LOGD("now sem_post...");
	if ((code = sem_post(psem)))
	{
		LOGE("error on sem_post. code=%d", code);
	}
	pthread_join(thread, NULL);

	sem_destroy(psem);
	//sem_unlink(sem);
	//sem_close(sem);
	return 0;
}

#define LOG_TAG_MAIN "MY_TAG"

static void test_log()
{
	LOGD("\n%s\nHello! current log_priority=%d, log_target=%d\n%s", XLOG_STAR_LINE, xlog_get_min_level(), xlog_get_target(), XLOG_STAR_LINE);

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

	TLOGV(LOG_TAG_MAIN, "this log is printed by TLOGV");
	TLOGD(LOG_TAG_MAIN, "this log is printed by TLOGD");
	TLOGI(LOG_TAG_MAIN, "this log is printed by TLOGI");
	TLOGW(LOG_TAG_MAIN, "this log is printed by TLOGW");
	TLOGE(LOG_TAG_MAIN, "this log is printed by TLOGE");

	TLOGV_TRACE(LOG_TAG_MAIN, "this log is printed by TLOGV_TRACE");
	TLOGD_TRACE(LOG_TAG_MAIN, "this log is printed by TLOGD_TRACE");
	TLOGI_TRACE(LOG_TAG_MAIN, "this log is printed by TLOGI_TRACE");
	TLOGW_TRACE(LOG_TAG_MAIN, "this log is printed by TLOGW_TRACE");
	TLOGE_TRACE(LOG_TAG_MAIN, "this log is printed by TLOGE_TRACE");

	char chars[16] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x10, 0xA0, 0xAF, 0xFD, 0xFE, 0xFF };
	TLOGV_HEX(LOG_TAG_MAIN, chars, sizeof(chars));
	LOGI_HEX(chars, sizeof(chars));

	xlog_set_min_level(LOG_LEVEL_ERROR);
	xlog_set_target(LOG_TARGET_ANDROID);
	LOGI("this log won't print because of current level is LOG_LEVEL_ERROR");
	LOGE("this log only print on logcat because of xlog_config_target=LOG_TARGET_ANDROID");

	xlog_set_target(LOG_TARGET_CONSOLE);
	LOGE_TRACE("this log only print on console because of xlog_config_target=LOG_TARGET_CONSOLE");

	xlog_set_min_level(LOG_LEVEL_OFF);
	LOGE("this log won't print because of xlog_config_level = LOG_LEVEL_OFF");

	LOGD("now set log min level to VERBOSE, target to 'LOG_TARGET_ANDROID | LOG_TARGET_CONSOLE'");
	xlog_set_min_level(LOG_LEVEL_VERBOSE);
	xlog_set_target(LOG_TARGET_ANDROID | LOG_TARGET_CONSOLE);
	LOGI("LOG test finished!!!");
}

//#define MYLOG(...) EXPAND_VA_ARGS(LOGD(__VA_ARGS__))
int thread_wrapper_test()
{
	LOGD("Hello World, thread id: %d", gettid());
	test_log();

	int cur_format = xlog_get_format();
	xlog_set_format(LOG_FORMAT_RAW);
	TLOGI("MyTag", "this log only print log msg without timestamp/tag/level/tid");
	xlog_set_format(cur_format);
	LOGI("test LOGI with format(%d)", cur_format);

	LOGW_TRACE("hello, %d, xx=%s", 1234, "WORLD");
	TLOGE("MyTag", "test TLOGE");
	TLOGD_TRACE("MyTag", "test TLOGD_TRACE");

	sem_test();

	return 0;
}
