#include "common_macro.h"
#include "time/time_util.h"
#include "mem/strings.h"
#include "thread/thread_wrapper.h"
#include <stdio.h>

#if(TIME_STR_SIZE < 24)
#error "TIME_STR_SIZE is too small."
#endif

#define USE_TIME_CACHE            (1)
#define USE_SNPRINTF_MILLISECONDS (0)

#define TIME_STAMP_FORMAT ("%m-%d %H:%M:%S")
#define TIME_STAMP_FORMAT_FOR_FILE_NAME ("%m%d%H%M%S")

#if(defined(USE_TIME_CACHE) && USE_TIME_CACHE)
#define GET_TIME_FORMAT_TYPE(fmt) ((fmt[8] == ':') ? 0 : 1) 

typedef struct
{
	pthread_rwlock_t rw_lock;
	int timezone_hour;
	struct timeval tval;
	char format_cache[TIME_STR_SIZE];
} time_cache_t;

static time_cache_t g_time_caches[2] = 
{
	{.rw_lock = PTHREAD_RWLOCK_INITIALIZER, },
	{.rw_lock = PTHREAD_RWLOCK_INITIALIZER, }
};
#endif // USE_TIME_CACHE

#ifdef _WIN32

// gettimeofday taken from https://doxygen.postgresql.org/gettimeofday_8c_source.html

/* FILETIME of Jan 1 1970 00:00:00, the PostgreSQL epoch */
static const unsigned __int64 epoch = 116444736000000000UL;

/*
 * FILETIME represents the number of 100-nanosecond intervals since
 * January 1, 1601 (UTC).
 */
#define FILETIME_UNITS_PER_SEC  10000000L
#define FILETIME_UNITS_PER_USEC 10

 /*
  * Both GetSystemTimeAsFileTime and GetSystemTimePreciseAsFileTime share a
  * signature, so we can just store a pointer to whichever we find. This
  * is the pointer's type.
  */
typedef VOID(WINAPI* LcuGetSystemTimeFn) (LPFILETIME);

/* One-time initializer function, must match that signature. */
static void WINAPI init_gettimeofday(LPFILETIME lpSystemTimeAsFileTime);

/* Storage for the function we pick at runtime */
static LcuGetSystemTimeFn lcu_get_system_time = &init_gettimeofday;

/*
 * One time initializer.  Determine whether GetSystemTimePreciseAsFileTime
 * is available and if so, plan to use it; if not, fall back to
 * GetSystemTimeAsFileTime.
 */
static void WINAPI init_gettimeofday(LPFILETIME lpSystemTimeAsFileTime)
{
	/*
	 * Because it's guaranteed that kernel32.dll will be linked into our
	 * address space already, we don't need to LoadLibrary it and worry about
	 * closing it afterwards, so we're not using Pg's dlopen/dlsym() wrapper.
	 *
	 * We'll just look up the address of GetSystemTimePreciseAsFileTime if
	 * present.
	 *
	 * While we could look up the Windows version and skip this on Windows
	 * versions below Windows 8 / Windows Server 2012 there isn't much point,
	 * and determining the windows version is its self somewhat Windows
	 * version and development SDK specific...
	 */
	HMODULE kernel32 = GetModuleHandle(TEXT("kernel32.dll"));
	if (kernel32 == NULL || (lcu_get_system_time = (LcuGetSystemTimeFn)GetProcAddress(kernel32,
		"GetSystemTimePreciseAsFileTime")) == NULL)
	{
		/*
		 * The expected error from GetLastError() is ERROR_PROC_NOT_FOUND, if
		 * the function isn't present. No other error should occur.
		 *
		 * We can't report an error here because this might be running in
		 * frontend code; and even if we're in the backend, it's too early to
		 * elog(...) if we get some unexpected error.  Also, it's not a
		 * serious problem, so just silently fall back to
		 * GetSystemTimeAsFileTime irrespective of why the failure occurred.
		 */
		lcu_get_system_time = &GetSystemTimeAsFileTime;
	}

	(*lcu_get_system_time) (lpSystemTimeAsFileTime);
}

/*
 * timezone information is stored outside the kernel so tzp isn't used anymore.
 *
 * Note: this function is not for Win32 high precision timing purposes. See
 * elapsed_time().
 */
int gettimeofday(struct timeval* tp, struct timezone* tzp)
{
	UNUSED(tzp);

	FILETIME    file_time;
	ULARGE_INTEGER ularge;
	unsigned __int64 unix_epoch;

	(*lcu_get_system_time) (&file_time);
	ularge.LowPart = file_time.dwLowDateTime;
	ularge.HighPart = file_time.dwHighDateTime;

	unix_epoch = (unsigned __int64)(ularge.QuadPart - epoch);
	tp->tv_sec = (long)(unix_epoch / FILETIME_UNITS_PER_SEC);
	tp->tv_usec = (long)((unix_epoch % FILETIME_UNITS_PER_SEC) / FILETIME_UNITS_PER_USEC);

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
 * use fast_second2date instead of localtime_r, because locatime_r have performance issue on multi-thread.
 * taken from https://www.cnblogs.com/westfly/p/5139645.html
 */
int time_util_fast_second2date(const time_t* p_unix_sec, struct tm* lt, int timezone_hour)
{
	#define kHoursInDay         (24)
	#define kMinutesInHour      (60)
	#define kDaysFromUnixTime   (2472632)
	#define kDaysFromYear       (153)
	#define kMagicUnkonwnFirst  (146097)
	#define kMagicUnkonwnSec    (1461)
	lt->tm_sec = (*p_unix_sec) % kMinutesInHour;
	int minutes = ((int)(*p_unix_sec) / kMinutesInHour);
	lt->tm_min = minutes % kMinutesInHour; //nn
	int hours = minutes / kMinutesInHour + timezone_hour;
	lt->tm_hour = hours % kHoursInDay; // hh
	lt->tm_mday = hours / kHoursInDay;
	int a = lt->tm_mday + kDaysFromUnixTime;
	int b = (a * 4 + 3) / kMagicUnkonwnFirst;
	int c = (-b * kMagicUnkonwnFirst) / 4 + a;
	int d = ((c * 4 + 3) / kMagicUnkonwnSec);
	int e = (-d * kMagicUnkonwnSec) / 4 + c;
	int m = (5 * e + 2) / kDaysFromYear;
	lt->tm_mday = -(kDaysFromYear * m + 2) / 5 + e + 1;
	lt->tm_mon = (-m / 10) * 12 + m + 2;
	lt->tm_year = b * 100 + d - 6700 + (m / 10);
	return 0;
}

static inline int format_time(char str[TIME_STR_SIZE], time_t* cur_time_p,
	const char* time_format, int timezone_hour)
{
	struct tm lt;
	//localtime_r((const time_t*)(cur_time_p), &lt); //here has performance issues on multithread.
	time_util_fast_second2date((const time_t*)(cur_time_p), &lt, timezone_hour);
	return (int)strftime(str, TIME_STR_SIZE, time_format, &lt);
}

// simple snprintf .%03d : snprintf(buf, BUF_SIZE, ".%03d", num);
static inline int print_millisec(char* buffer, unsigned int num)
{
#if(defined(USE_SNPRINTF_MILLISECONDS) && USE_SNPRINTF_MILLISECONDS)
	snprintf(buffer, 5, ".%03d", num);
#else
	buffer[0] = '.';
	++buffer;
	for (int i = 3; i > 0; --i)
	{
		buffer[i - 1] = '0' + num % 10;
		num /= 10;
	}
	buffer[3] = '\0';
#endif // USE_SNPRINTF_MILLISECONDS
	return 4;
}

static int get_time_str(char str[TIME_STR_SIZE], struct timeval* tval_p,
	const char* time_format, const int timezone_hour, bool use_cache)
{
	int ftime_len = 0;
	time_t cur_time = (time_t)(tval_p->tv_sec);

#if(defined(USE_TIME_CACHE) && USE_TIME_CACHE)
	if (use_cache)
	{
		time_cache_t* cache_p = &g_time_caches[GET_TIME_FORMAT_TYPE(time_format)];
		pthread_rwlock_rdlock(&cache_p->rw_lock); // lock rdlock
		if (cache_p->timezone_hour == timezone_hour && cache_p->tval.tv_sec == tval_p->tv_sec)
		{	// hit cache, just copy whole cached time string
			ftime_len = (int)strlcpy(str, cache_p->format_cache, TIME_STR_SIZE);
			bool update_millis = cache_p->tval.tv_usec != tval_p->tv_usec;
			pthread_rwlock_unlock(&cache_p->rw_lock); // unlock rdlock
			if (update_millis)
			{
				ftime_len -= 4;
				//ftime_len += snprintf(str + ftime_len, TIME_STR_SIZE - ftime_len, ".%03ld", tval_p->tv_usec / 1000);
				ftime_len += print_millisec(str + ftime_len, (unsigned int)(tval_p->tv_usec / 1000));
			}
		}
		else // not hit sec cache
		{
			pthread_rwlock_unlock(&cache_p->rw_lock); // unlock rdlock
#endif // USE_TIME_CACHE

			ftime_len = format_time(str, &cur_time, time_format, timezone_hour);
			//ftime_len += snprintf(str + ftime_len, TIME_STR_SIZE - ftime_len, ".%03ld", tval_p->tv_usec / 1000);
			ftime_len += print_millisec(str + ftime_len, (unsigned int)(tval_p->tv_usec / 1000));

#if(defined(USE_TIME_CACHE) && USE_TIME_CACHE)
			pthread_rwlock_wrlock(&cache_p->rw_lock); // lock wrlock
			if (cache_p->tval.tv_sec != tval_p->tv_sec)
			{
				//printf("now write time cache: %s\n", str);
				strlcpy(cache_p->format_cache, str, TIME_STR_SIZE);
				cache_p->timezone_hour = timezone_hour;
				cache_p->tval.tv_sec = tval_p->tv_sec;
				cache_p->tval.tv_usec = tval_p->tv_usec;
			}
			pthread_rwlock_unlock(&cache_p->rw_lock); // unlock wrlock
		}
	}
	else // not use cache
	{
		ftime_len = format_time(str, &cur_time, time_format, timezone_hour);
		//ftime_len += snprintf(str + ftime_len, TIME_STR_SIZE - ftime_len, ".%03ld", tval_p->tv_usec / 1000);
		ftime_len += print_millisec(str + ftime_len, (unsigned int)(tval_p->tv_usec / 1000));
	}
#endif // USE_TIME_CACHE

	return ftime_len;
}

int time_util_get_time_str(struct timeval* tval_p, char str[TIME_STR_SIZE], int timezone_hour)
{
	return get_time_str(str, tval_p, TIME_STAMP_FORMAT, timezone_hour, false);
}

int time_util_get_time_str_current(char str[TIME_STR_SIZE], int timezone_hour)
{
	struct timeval tv;
	gettimeofday(&tv, NULL); // get current time
	return get_time_str(str, &tv, TIME_STAMP_FORMAT, timezone_hour, true);
}

int time_util_get_time_str_for_file_name(struct timeval* tval_p, char str[TIME_STR_SIZE], int timezone_hour)
{
	return get_time_str(str, tval_p, TIME_STAMP_FORMAT_FOR_FILE_NAME, timezone_hour, false);
}

int time_util_get_time_str_for_file_name_current(char str[TIME_STR_SIZE], int timezone_hour)
{
	struct timeval tv;
	gettimeofday(&tv, NULL); // get current time
	return get_time_str(str, &tv, TIME_STAMP_FORMAT_FOR_FILE_NAME, timezone_hour, true);
}

void time_util_current_ms(uint64_t* p_cur_ms)
{
	struct timeval cur_time;
	gettimeofday(&cur_time, NULL);
	*p_cur_ms = ((uint64_t)cur_time.tv_sec * 1000U + (uint64_t)cur_time.tv_usec / 1000U);
}

#ifdef _WIN32

typedef uint64_t(*fn_get_performance_freq)();

static uint64_t g_cached_milli_perf_freq = 0;

static inline uint64_t get_cached_performance_freq()
{
	return g_cached_milli_perf_freq;
}

//onetime initializer
static uint64_t init_get_milli_perf_freq();

static fn_get_performance_freq get_milli_perf_freq = &init_get_milli_perf_freq;

static uint64_t init_get_milli_perf_freq()
{
	LARGE_INTEGER frequency = { 0 }; // how many clock period on one seconds.
	QueryPerformanceFrequency(&frequency);
	g_cached_milli_perf_freq = frequency.QuadPart / 1000;
	ASSERT(g_cached_milli_perf_freq > 0);

	get_milli_perf_freq = &get_cached_performance_freq;
	return (*get_milli_perf_freq)();
}
#endif // _WIN32

void time_util_query_performance_ms(uint64_t* p_cur_ms)
{
#ifdef _WIN32
	//QueryPerformanceFrequency Retrieves the frequency of the performance counter. 
	//The frequency of the performance counter is fixed at system boot and is consistent across all processors. 
	//Therefore, the frequency need only be queried upon application initialization, and the result can be cached.
	uint64_t milli_frequency = (*get_milli_perf_freq)();
	LARGE_INTEGER cur_counter;
	QueryPerformanceCounter(&cur_counter);
	*p_cur_ms = cur_counter.QuadPart / milli_frequency;
#else
	time_util_current_ms(p_cur_ms);
#endif // _WIN32
}
