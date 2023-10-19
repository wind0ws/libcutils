#include "common_macro.h"
#include "time/time_util.h"
#include "time/time_rfc1123.h"
#include "time/time_rfc2822.h"
#include "thread/posix_thread.h"

#define LOG_TAG          ("TIME_TEST")
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
		LOGD("[%06d], %06d at %s    %llu", tid, counter, time_str, cur_millis);
	}
	time_util_current_ms(&cur_millis);
	LOGI_TRACE("tid=%d end! cost %llums", tid, (cur_millis - start_millis));
	printf("\n\n\n");
	return NULL;
}

int time_util_test()
{
	time_t t;
	RANDOM_INIT((unsigned int)time(&t));

	test_rfc_1123_2822();

	const int timezone_hour = time_util_zone_offset_seconds_to_utc() / HOUR_TO_SECONDS;
	char time_str[TIME_STR_SIZE];
	time_util_get_time_str_current(time_str, timezone_hour);
	LOGD("timezone:%d, current time str is: %s", timezone_hour, time_str);

	time_util_get_time_str_for_file_name_current(time_str, timezone_hour);
	LOGD("current time str for file is: %s", time_str);

	uint64_t cur_milliseconds;
	time_util_current_ms(&cur_milliseconds);
	LOGD("current milliseconds: %lld", cur_milliseconds);

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
	LOGD("time_cost: %llums", (end - begin));

	return 0;
}