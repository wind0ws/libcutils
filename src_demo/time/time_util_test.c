#include "mem/mem_debug.h"
#include "common_macro.h"
#include "time/time_util.h"
#include "time/time_rfc1123.h"
#include "time/time_rfc2822.h"
#include "thread/posix_thread.h"
#include <string.h>

#define LOG_TAG          "TIME_TEST"
#include "log/logger.h"

#define HOUR_TO_SECONDS  (3600)

static void test_rfc_1123_2822()
{
	LOGD("--> test rfc_1123_2822 start");
	time_t cur_time;
	time(&cur_time);

	char time_str_1123[TIME_RFC1123_STR_SIZE] = { 0 };
	char time_str_2822[TIME_RFC2822_STR_SIZE] = { 0 };
	char time_str_2822_utc[TIME_RFC2822_UTC_STR_SIZE] = { 0 };
	time_rfc1123(&cur_time, time_str_1123, TIME_RFC1123_STR_SIZE);
	time_rfc2822(&cur_time, time_str_2822, TIME_RFC2822_STR_SIZE);
	time_rfc2822_utc(&cur_time, time_str_2822_utc, TIME_RFC2822_UTC_STR_SIZE);

	LOGI("current time: rfc1123=%s, rfc_2822=%s, rfc_2822_utc=%s",
		time_str_1123, time_str_2822, time_str_2822_utc);
	LOGD("<-- test rfc_1123_2822 end");
}

static void assert_tm_equal(const struct tm* actual, const struct tm* expected,
	const time_t unix_sec, int timezone_hour)
{
	if (actual->tm_year != expected->tm_year)
	{
		LOGE("tm_year mismatch sec=%lld tz=%d, actual=%d expected=%d",
			(long long)unix_sec, timezone_hour, actual->tm_year, expected->tm_year);
		ASSERT_ABORT(0);
	}
	if (actual->tm_mon != expected->tm_mon)
	{
		LOGE("tm_mon mismatch sec=%lld tz=%d, actual=%d expected=%d",
			(long long)unix_sec, timezone_hour, actual->tm_mon, expected->tm_mon);
		ASSERT_ABORT(0);
	}
	if (actual->tm_mday != expected->tm_mday)
	{
		LOGE("tm_mday mismatch sec=%lld tz=%d, actual=%d expected=%d",
			(long long)unix_sec, timezone_hour, actual->tm_mday, expected->tm_mday);
		ASSERT_ABORT(0);
	}
	if (actual->tm_hour != expected->tm_hour)
	{
		LOGE("tm_hour mismatch sec=%lld tz=%d, actual=%d expected=%d",
			(long long)unix_sec, timezone_hour, actual->tm_hour, expected->tm_hour);
		ASSERT_ABORT(0);
	}
	if (actual->tm_min != expected->tm_min)
	{
		LOGE("tm_min mismatch sec=%lld tz=%d, actual=%d expected=%d",
			(long long)unix_sec, timezone_hour, actual->tm_min, expected->tm_min);
		ASSERT_ABORT(0);
	}
	if (actual->tm_sec != expected->tm_sec)
	{
		LOGE("tm_sec mismatch sec=%lld tz=%d, actual=%d expected=%d",
			(long long)unix_sec, timezone_hour, actual->tm_sec, expected->tm_sec);
		ASSERT_ABORT(0);
	}
	if (actual->tm_yday != expected->tm_yday)
	{
		LOGE("tm_yday mismatch sec=%lld tz=%d, actual=%d expected=%d",
			(long long)unix_sec, timezone_hour, actual->tm_yday, expected->tm_yday);
		ASSERT_ABORT(0);
	}
	if (actual->tm_wday != expected->tm_wday)
	{
		LOGE("tm_wday mismatch sec=%lld tz=%d, actual=%d expected=%d",
			(long long)unix_sec, timezone_hour, actual->tm_wday, expected->tm_wday);
		ASSERT_ABORT(0);
	}
}

static void test_fast_second2date_cases()
{
	const struct
	{
		time_t unix_sec;
		int timezone_hour;
	} cases[] =
	{
		{0, 0},
		{0, -12},
		{0, 8},
		{86400, -5},
		{-86400, 5},
		{1609459200, 0}, // 2021-01-01 00:00:00 UTC
		{1761278401, 8}, // 2025-10-24 12:00:01 +0800
	};

	for (size_t i = 0; i < ARRAY_LEN(cases); ++i)
	{
		struct tm actual;
		memset(&actual, 0, sizeof(actual));

		time_t cur_sec = cases[i].unix_sec;
		time_util_fast_second2date(&cur_sec, &actual, cases[i].timezone_hour);

		int64_t adjusted = (int64_t)cases[i].unix_sec + (int64_t)cases[i].timezone_hour * HOUR_TO_SECONDS;
		time_t adjusted_sec = (time_t)adjusted;
		struct tm expected;
		gmtime_r(&adjusted_sec, &expected);
		LOGD("case[%u] sec=%lld tz=%d adjusted=%lld -> expected y=%d m=%d d=%d wday=%d", (unsigned)i,
			(long long)cases[i].unix_sec, cases[i].timezone_hour, (long long)adjusted,
			expected.tm_year + 1900, expected.tm_mon + 1, expected.tm_mday, expected.tm_wday);

		assert_tm_equal(&actual, &expected, cases[i].unix_sec, cases[i].timezone_hour);
	}

	LOGD("time_util_fast_second2date basic cases passed");
}

static void test_time_string_helpers()
{
	const struct
	{
		time_t tv_sec;
		long tv_usec;
		int timezone_hour;
		const char* expected_standard;
		const char* expected_fname;
	} cases[] =
	{
		{0, 0, 0, "01-01 00:00:00.000", "0101000000.000"},
		{0, 0, -12, "12-31 12:00:00.000", "1231120000.000"},
		{0, 987000, 0, "01-01 00:00:00.987", "0101000000.987"},
		{1609459200, 0, 8, "01-01 08:00:00.000", "0101080000.000"},
	};

	for (size_t i = 0; i < ARRAY_LEN(cases); ++i)
	{
		struct timeval tv;
		tv.tv_sec = (long)cases[i].tv_sec;
		tv.tv_usec = cases[i].tv_usec;
		char buffer[TIME_STR_SIZE];
		char file_buffer[TIME_STR_SIZE];

		int len = time_util_get_time_str(&tv, buffer, cases[i].timezone_hour);
		ASSERT_ABORT(len == (int)strlen(cases[i].expected_standard));
		ASSERT_ABORT(strcmp(buffer, cases[i].expected_standard) == 0);

		len = time_util_get_time_str_for_file_name(&tv, file_buffer, cases[i].timezone_hour);
		ASSERT_ABORT(len == (int)strlen(cases[i].expected_fname));
		ASSERT_ABORT(strcmp(file_buffer, cases[i].expected_fname) == 0);
	}

	LOGD("time string formatting helpers passed");
}

static void test_time_strings_without_global_init()
{
	char standard[TIME_STR_SIZE];
	char fname[TIME_STR_SIZE];
	// 先故意不调用 time_util_global_init，测试能否正常使用时间字符串接口
	time_util_global_cleanup();

	int len = time_util_get_time_str_current(standard, 0);
	ASSERT_ABORT(len > 0);
	ASSERT_ABORT(strchr(standard, '.') != NULL);

	len = time_util_get_time_str_for_file_name_current(fname, 0);
	ASSERT_ABORT(len > 0);
	ASSERT_ABORT(strchr(fname, '.') != NULL);

	standard[0] = '\0';
	len = time_util_get_time_str_current(standard, 0);
	ASSERT_ABORT(len > 0);
	ASSERT_ABORT(strlen(standard) == (size_t)len);

	LOGD("test_time_strings_without_global_init passed");
}

static void* thread_worker(void *param) 
{
	printf("\n\n\n");
	const int tid = (int)GETTID();
	LOGI("mytid=%d", tid);
	char time_str[TIME_STR_SIZE];
	int counter = 0;
	LOGI("enter %s:%d", __func__, __LINE__);
	LOGI_TRACE("tid=%d start!", tid);
	uint64_t cur_millis, start_millis;
	time_util_current_ms(&start_millis);
	while (counter++ < 100000)
	{
		//Sleep(RANDOM(1,3));
		time_util_get_time_str_current(time_str, 8);
		time_util_current_ms(&cur_millis);
		//printf("[%d] at %s    %llu", tid, time_str, cur_millis);
		LOGD("[%06d], %06d at %s    %lu", tid, counter, time_str, (unsigned long)cur_millis);
	}
	time_util_current_ms(&cur_millis);
	LOGI_TRACE("tid=%d end! cost %lums", tid, (unsigned long)(cur_millis - start_millis));
	printf("\n\n\n");
	return NULL;
}

int time_util_test()
{
	time_t t;
	RANDOM_INIT((unsigned int)time(&t));
	test_time_strings_without_global_init();
	ASSERT_ABORT(0 == time_util_global_init());

	test_fast_second2date_cases();
	test_time_string_helpers();

	test_rfc_1123_2822();

	const int timezone_hour = time_util_zone_offset_seconds_to_utc() / HOUR_TO_SECONDS;
	char time_str[TIME_STR_SIZE];
	time_util_get_time_str_current(time_str, timezone_hour);
	LOGD("timezone:%d, current time str is: %s", timezone_hour, time_str);

	time_util_get_time_str_for_file_name_current(time_str, timezone_hour);
	LOGD("current time str for file is: %s", time_str);

	uint64_t cur_milliseconds;
	time_util_current_ms(&cur_milliseconds);
	LOGD("current milliseconds: %lu", (unsigned long)cur_milliseconds);

	uint64_t begin, end;
	time_util_query_performance_ms(&begin);
	//Sleep(1000);//mock heavy calculation
	pthread_t th1, th2;
	pthread_create(&th1, NULL, thread_worker, NULL);
	pthread_create(&th2, NULL, thread_worker, NULL);
	pthread_join(th1, NULL);
	pthread_join(th2, NULL);
	LOGI("join thread complete");
	time_util_query_performance_ms(&end);
	LOGD("time_cost: %lums", (unsigned long)(end - begin));
	ASSERT_ABORT(0 == time_util_global_cleanup());

	return 0;
}
