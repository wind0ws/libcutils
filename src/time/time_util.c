#include "time/time_util.h"
#include <stdio.h>

#define TIME_STAMP_FORMT ("%m-%d %H:%M:%S")
#define TIME_STAMP_FORMT_FOR_FILE_NAME ("%m-%d %H_%M_%S")

static inline void get_current_time_str(char str[TIME_STR_LEN], const char* time_format)
{
	struct tm* lt;
	int milliseconds;
#ifdef _WIN32
	struct timeb tb;
	ftime(&tb);
#pragma warning(push)
#pragma warning(disable: 4996)
	lt = localtime(&tb.time);
#pragma warning(pop)
	milliseconds = tb.millitm;
#else
	struct timeval tv;
	gettimeofday(&tv, NULL); // get current time
	lt = localtime((const time_t*)&tv.tv_sec);
	//return tv.tv_sec * 1000LL + tv.tv_usec / 1000;
	milliseconds = (int)(tv.tv_usec / 1000);
#endif // _WIN32
	snprintf(str + strftime(str, TIME_STR_LEN, time_format, lt),
		TIME_STR_LEN, ".%03d", milliseconds);
}

void time_util_get_current_time_str(char str[TIME_STR_LEN])
{
	get_current_time_str(str, TIME_STAMP_FORMT);
}

void time_util_get_current_time_str_for_file_name(char str[TIME_STR_LEN])
{
	get_current_time_str(str, TIME_STAMP_FORMT_FOR_FILE_NAME);
}