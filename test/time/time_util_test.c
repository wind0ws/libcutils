#include "time/time_util.h"
#include "log/xlog.h"
#include "thread/thread_wrapper.h"
#include "common_macro.h"

#define LOG_TAG "TIME_TEST"

static void* thread_worker(void *param) 
{
	const pid_t tid = gettid();
	TLOGI(LOG_TAG, "mytid=%d", tid);
	char time_str[TIME_STR_LEN];
	int counter = 0;
	TLOGI(LOG_TAG, "tid=%d start!", tid);
	uint64_t cur_millis;
	while (counter++ < 100)
	{
		Sleep(RANDOM(1,4));
		time_util_get_current_time_str(time_str, 8);
		time_util_current_milliseconds(&cur_millis);
		//printf("[%d] at %s    %llu", tid, time_str, cur_millis);
		TLOGD(LOG_TAG, "[%06d], at %s    %llu", tid, time_str, cur_millis);
	}
	TLOGI(LOG_TAG, "tid=%d end!", tid);
	return NULL;
}


int time_util_test()
{
	time_t t;
	RANDOM_INIT((unsigned int)time(&t));

	const int timezone_hour = time_util_zone_offset_seconds_to_utc() / 3600;
	char time_str[TIME_STR_LEN];
	time_util_get_current_time_str(time_str, timezone_hour);
	TLOGD(LOG_TAG, "timezone:%d, current time str is：%s", timezone_hour, time_str);

	time_util_get_current_time_str_for_file_name(time_str, timezone_hour);
	TLOGD(LOG_TAG, "current time str for file is: %s", time_str);

	uint64_t cur_milliseconds;
	time_util_current_milliseconds(&cur_milliseconds);
	TLOGD(LOG_TAG, "current milliseconds: %lld", cur_milliseconds);

	uint64_t begin, end;
	time_util_query_performance_ms(&begin);
	//Sleep(1000);//mock heavy calculation
	pthread_t th1, th2;
	pthread_create(&th1, NULL, thread_worker, NULL);
	pthread_create(&th2, NULL, thread_worker, NULL);
	pthread_join(th1, NULL);
	pthread_join(th2, NULL);
	TLOGI(LOG_TAG, "join thread complete");
	time_util_query_performance_ms(&end);
	TLOGD(LOG_TAG, "time_cost: %llums", (end - begin));

	return 0;
}