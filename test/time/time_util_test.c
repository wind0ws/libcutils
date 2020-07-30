#include "time/time_util.h"
#include "log/xlog.h"
#include "thread/thread_wrapper.h"

#define LOG_TAG "TIME_TEST"

int time_util_test()
{
	char time_str[TIME_STR_LEN];
	time_util_get_current_time_str(time_str);
	TLOGD(LOG_TAG, "current time str is：%s", time_str);
	time_util_get_current_time_str_for_file_name(time_str);
	TLOGD(LOG_TAG, "current time str for file is: %s", time_str);
	int64_t cur_milliseconds;
	time_util_current_milliseconds(&cur_milliseconds);
	TLOGD(LOG_TAG, "current milliseconds: %lld", cur_milliseconds);

	int64_t begin, end;
	time_util_query_performance_ms(&begin);
	Sleep(1000);//mock heavy calculation
	time_util_query_performance_ms(&end);
	TLOGD(LOG_TAG, "time_cost: %lldms", end - begin);

	return 0;
}