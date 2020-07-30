#include "time/time_util.h"
#include <stdio.h>
#include "common_macro.h"

#define TIME_STAMP_FORMT ("%m-%d %H:%M:%S")
#define TIME_STAMP_FORMT_FOR_FILE_NAME ("%m-%d %H_%M_%S")

#ifdef _WIN32

/* FILETIME of Jan 1 1970 00:00:00. */
static const unsigned __int64 epoch = 116444736000000000UL;

/*
 * timezone information is stored outside the kernel so tzp isn't used anymore.
 * (take from https://git.postgresql.org/gitweb/?p=postgresql.git;a=blob;f=src/port/gettimeofday.c)
 * Note: this function is not for Win32 high precision timing purpose. See
 * elapsed_time().
 */
int gettimeofday(struct timeval* tp, struct timezone* tzp)
{
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

static inline int get_current_time_str(char str[TIME_STR_LEN], const char* time_format)
{
	struct tm lt;
	unsigned short milliseconds;
#ifdef _WIN32
	struct timeb tb;
	ftime(&tb);
	milliseconds = tb.millitm;
	localtime_r(&tb.time, &lt);
#else
	struct timeval tv;
	gettimeofday(&tv, NULL); // get current time
	localtime_r((const time_t*)&tv.tv_sec, &lt);
	//return tv.tv_sec * 1000LL + tv.tv_usec / 1000;
	milliseconds = (unsigned short)(tv.tv_usec / 1000);
#endif // _WIN32
	size_t ftime_len = strftime(str, TIME_STR_LEN, time_format, &lt);
	snprintf(str + ftime_len, TIME_STR_LEN - ftime_len, ".%03d", milliseconds);
	return 0;
}

int time_util_get_current_time_str(char str[TIME_STR_LEN])
{
	return get_current_time_str(str, TIME_STAMP_FORMT);
}

int time_util_get_current_time_str_for_file_name(char str[TIME_STR_LEN])
{
	return get_current_time_str(str, TIME_STAMP_FORMT_FOR_FILE_NAME);
}

void time_util_current_milliseconds(int64_t* p_cur_ms)
{
#ifdef _WIN32
	struct timeb rawtime;
	ftime(&rawtime);
	*p_cur_ms = rawtime.time * 1000 + rawtime.millitm;
#else
	STATIC_ASSERT(sizeof(int64_t) == sizeof(struct timeval));
	gettimeofday((struct timeval*)p_cur_ms, NULL);
	*p_cur_ms = ((struct timeval*)p_cur_ms)->tv_sec * 1000LL + ((struct timeval*)p_cur_ms)->tv_usec / 1000;
#endif // _WIN32
}

void time_util_query_performance_ms(int64_t *p_cur_ms)
{
#ifdef _WIN32
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


