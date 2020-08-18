#include "time/time_util.h"
#include <stdio.h>
#include "common_macro.h"

#define TIME_STAMP_FORMAT ("%m-%d %H:%M:%S")
#define TIME_STAMP_FORMAT_FOR_FILE_NAME ("%m-%d %H_%M_%S")

#ifdef _WIN32

/* FILETIME of Jan 1 1970 00:00:00. */
static const unsigned __int64 epoch = 116444736000000000UL;

/*
 * timezone information is stored outside the kernel so tzp isn't used anymore.
 * (take from https://git.postgresql.org/gitweb/?p=postgresql.git;a=blob;f=src/port/gettimeofday.c)
 * Note: this function is not for Win32 high precision timing purpose. 
 * See elapsed_time().
 */
int gettimeofday(struct timeval* tp, struct timezone* tzp)
{
	UNUSED(tzp);
	FILETIME    file_time;
	SYSTEMTIME  system_time;
	ULARGE_INTEGER ularge;

	GetSystemTime(&system_time);
	SystemTimeToFileTime(&system_time, &file_time);
	ularge.LowPart = file_time.dwLowDateTime;
	ularge.HighPart = file_time.dwHighDateTime;

	tp->tv_sec = (long)((ularge.QuadPart - epoch) / 10000000L);
	tp->tv_usec = (long)(system_time.wMilliseconds * 1000);

	return 0;
}

#endif // _WIN32

int time_util_zone_offset_seconds_to_utc()
{
	time_t rawtime = time(NULL);
	struct tm buf;

#if defined(WIN32)
	gmtime_s(&buf, &rawtime);
#else
	gmtime_r(&rawtime, &buf);
#endif
	// Request that mktime() looksup dst in timezone database
	buf.tm_isdst = -1;
	time_t gmt = mktime(&buf);

	return (int)difftime(rawtime, gmt);//seconds
}

/**
 * use fast_second2date instead of localtime_r, because locatime_r have performance issue on multi thread.
 * take from https://www.cnblogs.com/westfly/p/5139645.html
 */
int time_util_fast_second2date(const time_t* p_unix_sec, struct tm* lt, int timezone_hour)
{
	static const int kHoursInDay = 24;
	static const int kMinutesInHour = 60;
	static const int kDaysFromUnixTime = 2472632;
	static const int kDaysFromYear = 153;
	static const int kMagicUnkonwnFirst = 146097;
	static const int kMagicUnkonwnSec = 1461;
	lt->tm_sec = (*p_unix_sec) % kMinutesInHour;
	int i = ((int)(*p_unix_sec) / kMinutesInHour);
	lt->tm_min = i % kMinutesInHour; //nn
	i /= kMinutesInHour;
	lt->tm_hour = (i + timezone_hour) % kHoursInDay; // hh
	lt->tm_mday = (i + timezone_hour) / kHoursInDay;
	int a = lt->tm_mday + kDaysFromUnixTime;
	int b = (a * 4 + 3) / kMagicUnkonwnFirst;
	int c = (-b * kMagicUnkonwnFirst) / 4 + a;
	int d = ((c * 4 + 3) / kMagicUnkonwnSec);
	int e = -d * kMagicUnkonwnSec;
	e = e / 4 + c;
	int m = (5 * e + 2) / kDaysFromYear;
	lt->tm_mday = -(kDaysFromYear * m + 2) / 5 + e + 1;
	lt->tm_mon = (-m / 10) * 12 + m + 2;
	lt->tm_year = b * 100 + d - 6700 + (m / 10);
	return 0;
}

static inline int get_current_time_str(char str[TIME_STR_LEN], const char* time_format, const int timezone_hour)
{
	struct tm lt;
	struct timeval tv;

	gettimeofday(&tv, NULL); // get current time
	//localtime_r((const time_t*)&tv.tv_sec, &lt);//here has performance issues on multithread.
	time_util_fast_second2date((const time_t*)&tv.tv_sec, &lt, timezone_hour);
	//return tv.tv_sec * 1000LL + tv.tv_usec / 1000;

	const size_t ftime_len = strftime(str, TIME_STR_LEN, time_format, &lt);
	snprintf(str + ftime_len, TIME_STR_LEN - ftime_len, ".%03ld", tv.tv_usec / 1000);
	return 0;
}

int time_util_get_current_time_str(char str[TIME_STR_LEN], int timezone_hour)
{
	return get_current_time_str(str, TIME_STAMP_FORMAT, timezone_hour);
}

int time_util_get_current_time_str_for_file_name(char str[TIME_STR_LEN], int timezone_hour)
{
	return get_current_time_str(str, TIME_STAMP_FORMAT_FOR_FILE_NAME, timezone_hour);
}

void time_util_current_milliseconds(int64_t* p_cur_ms)
{
	STATIC_ASSERT(sizeof(int64_t) == sizeof(struct timeval));
	gettimeofday((struct timeval*)p_cur_ms, NULL);
	*p_cur_ms = ((struct timeval*)p_cur_ms)->tv_sec * 1000LL + ((struct timeval*)p_cur_ms)->tv_usec / 1000;
}

void time_util_query_performance_ms(int64_t* p_cur_ms)
{
#ifdef _WIN32
	//although here we have thread safe issue, because we are not lock this static variable,
	//but i think it is acceptable, it is ok to set frequency value twice.
	static int64_t mill_frequency = 0;
	if (mill_frequency == 0)
	{
		LARGE_INTEGER frequency = { 0 }; // how many clock period on one seconds.
		QueryPerformanceFrequency(&frequency);
		mill_frequency = frequency.QuadPart / 1000;
		ASSERT(mill_frequency > 0);
	}
	STATIC_ASSERT(sizeof(LARGE_INTEGER) == sizeof(int64_t));
	QueryPerformanceCounter((LARGE_INTEGER*)p_cur_ms);
	*p_cur_ms = (*p_cur_ms) / mill_frequency;
#else
	time_util_current_milliseconds(p_cur_ms);
#endif // _WIN32
}
