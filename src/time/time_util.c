#include "common_macro.h"
#include "mem/strings.h"
#include "thread/portable_thread.h"
#include "time/time_util.h"
#include <stdio.h>
#include <string.h>

#if (TIME_STR_SIZE < 24)
#error "TIME_STR_SIZE is too small."
#endif

// if platform not support rwlock, just disable cache.
#define USE_TIME_CACHE (1)
#define USE_SNPRINTF_MILLISECONDS (0)

#define TIME_STAMP_FORMAT ("%m-%d %H:%M:%S")
#define TIME_STAMP_FORMAT_FOR_FILE_NAME ("%m%d%H%M%S")

static volatile unsigned char g_init_times = 0;

#if (defined(USE_TIME_CACHE) && USE_TIME_CACHE)
#define GET_TIME_FORMAT_TYPE(fmt) ((fmt[8] == ':') ? 0 : 1)

// read lock
#define TIME_UTIL_RD_LOCK(lock_p)           \
	do                                      \
	{                                       \
		if ((lock_p) && *(lock_p))          \
		{                                   \
			portable_rwlock_rdlock(lock_p); \
		}                                   \
	} while (0)

// write lock
#define TIME_UTIL_WR_LOCK(lock_p)           \
	do                                      \
	{                                       \
		if ((lock_p) && *(lock_p))          \
		{                                   \
			portable_rwlock_wrlock(lock_p); \
		}                                   \
	} while (0)

// unlock
#define TIME_UTIL_RW_UNLOCK(lock_p)         \
	do                                      \
	{                                       \
		if ((lock_p) && *(lock_p))          \
		{                                   \
			portable_rwlock_unlock(lock_p); \
		}                                   \
	} while (0)

typedef struct
{
	portable_rwlock_t rw_lock;
	int timezone_hour;
	struct timeval tval;
	int format_len;
	char format_cache[TIME_STR_SIZE];
} time_cache_t;

static time_cache_t g_time_caches[2] =
	{
		{
			.rw_lock = NULL,
		},
		{
			.rw_lock = NULL,
		},
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
#define FILETIME_UNITS_PER_SEC 10000000L
#define FILETIME_UNITS_PER_USEC 10

/*
 * Both GetSystemTimeAsFileTime and GetSystemTimePreciseAsFileTime share a
 * signature, so we can just store a pointer to whichever we find. This
 * is the pointer's type.
 */
typedef VOID(WINAPI *LcuGetSystemTimeFn)(LPFILETIME);

/* One-time initializer function, must match that signature. */
static void WINAPI init_gettimeofday(LPFILETIME lpSystemTimeAsFileTime);

/* Storage for the function we pick at runtime */
static LcuGetSystemTimeFn g_get_system_time = &init_gettimeofday;

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
	if (NULL == kernel32 ||
		NULL == (g_get_system_time = (LcuGetSystemTimeFn)GetProcAddress(
					 kernel32, "GetSystemTimePreciseAsFileTime")))
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
		g_get_system_time = &GetSystemTimeAsFileTime;
	}

	(*g_get_system_time)(lpSystemTimeAsFileTime);
}

/*
 * timezone information is stored outside the kernel so tzp isn't used anymore.
 *
 * Note: this function is not for Win32 high precision timing purposes.
 * See elapsed_time().
 */
int gettimeofday(struct timeval *tp, struct timezone *tzp)
{
	UNUSED(tzp);

	FILETIME file_time;
	ULARGE_INTEGER ularge;
	unsigned __int64 unix_epoch;

	(*g_get_system_time)(&file_time);
	ularge.LowPart = file_time.dwLowDateTime;
	ularge.HighPart = file_time.dwHighDateTime;

	unix_epoch = (unsigned __int64)(ularge.QuadPart - epoch);
	tp->tv_sec = (long)(unix_epoch / FILETIME_UNITS_PER_SEC);
	tp->tv_usec = (long)((unix_epoch % FILETIME_UNITS_PER_SEC) / FILETIME_UNITS_PER_USEC);

	return 0;
}

#endif // _WIN32

int time_util_global_init()
{
	if (g_init_times++ > 0)
	{
		return 0;
	}
#if (defined(USE_TIME_CACHE) && USE_TIME_CACHE)
	for (int i = 0; i < ARRAY_LEN(g_time_caches); ++i)
	{
		if (g_time_caches[i].rw_lock)
		{
			continue;
		}
		portable_rwlock_init(&(g_time_caches[i].rw_lock), NULL);
	}
#endif // USE_TIME_CACHE
	return 0;
}

int time_util_global_cleanup()
{
	if (0 == g_init_times || --g_init_times > 0)
	{
		return 0;
	}
#if (defined(USE_TIME_CACHE) && USE_TIME_CACHE)
	// destroy it(free it's memory), and reset the pointer
	for (int i = 0; i < ARRAY_LEN(g_time_caches); ++i)
	{
		if (NULL == g_time_caches[i].rw_lock)
		{
			continue;
		}
		portable_rwlock_destroy(&(g_time_caches[i].rw_lock));
		g_time_caches[i].rw_lock = NULL;
	}
#endif // USE_TIME_CACHE
	return 0;
}

int time_util_zone_offset_seconds_to_utc()
{
	time_t rawtime = time(NULL);
	struct tm buf;

#if defined(_MSC_VER)
	gmtime_s(&buf, &rawtime);
#else
	gmtime_r(&rawtime, &buf);
#endif
	// Request that mktime() looksup dst in timezone database
	buf.tm_isdst = -1;
	time_t gmt = mktime(&buf);

	return (int)difftime(rawtime, gmt); // seconds
}

/**
 * use fast_second2date instead of localtime_r, because locatime_r have performance issue on multi-thread.
 * origin inspired from https://www.cnblogs.com/westfly/p/5139645.html  <-- but there is a bug in it(2038).
 * below code is new implementation.
 */
void time_util_fast_second2date(const time_t *p_unix_sec, struct tm *lt, int timezone_hour)
{
#define SECONDS_PER_MINUTE (60)
#define SECONDS_PER_HOUR (60 * SECONDS_PER_MINUTE)
#define SECONDS_PER_DAY (24 * SECONDS_PER_HOUR)
	int64_t local_seconds = (int64_t)(*p_unix_sec) + (int64_t)timezone_hour * SECONDS_PER_HOUR;
	int64_t truncated_days = local_seconds / SECONDS_PER_DAY;
	int64_t days = truncated_days;
	int64_t rem = local_seconds % SECONDS_PER_DAY;
	if (rem < 0)
	{
		rem += SECONDS_PER_DAY;
		--days;
	}

	lt->tm_hour = (int)(rem / SECONDS_PER_HOUR);
	rem %= SECONDS_PER_HOUR;
	lt->tm_min = (int)(rem / SECONDS_PER_MINUTE);
	lt->tm_sec = (int)(rem % SECONDS_PER_MINUTE);

	// 719468 是 0000-03-01 到 1970-01-01 的天数偏移
	// 146097 是400年的天数 (400*365 + 97闰年)
	int64_t z = days + 719468;
	int64_t era = (z >= 0 ? z : z - 146096) / 146097;
	int64_t doe = z - era * 146097;
	int64_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
	int64_t y = yoe + era * 400;
	int64_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
	int64_t mp = (5 * doy + 2) / 153;

	int month = (int)(mp < 10 ? mp + 3 : mp - 9);
	int day = (int)(doy - (153 * mp + 2) / 5 + 1);
	// 调整年份（如果月份在1-2月，年份要+1）
	if (month <= 2)
	{
		++y;
	}

	lt->tm_year = (int)(y - 1900);
	lt->tm_mon = month - 1; // tm_mon是0-11
	lt->tm_mday = day;
	lt->tm_isdst = 0; // 不考虑夏令时

	static const int days_before_month[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
	int64_t is_leap = ((y % 4) == 0 && (y % 100) != 0) || ((y % 400) == 0);
	int yday = days_before_month[lt->tm_mon] + (day - 1);
	if (is_leap && lt->tm_mon > 1) // 闰年且月份在3月之后
	{
		++yday;
	}
	lt->tm_yday = yday;

	// 计算星期几 (0=周日, 1=周一, ..., 6=周六)。
	// Windows CRT 的 gmtime 在处理负时间戳时使用截断除法的天数，
	// 而 POSIX gmtime 基于已经调整的 days. 对两个平台分别匹配以保持一致性.
#if defined(_WIN32)
	int64_t wday_base = truncated_days;
#else
	int64_t wday_base = days;
#endif
	int64_t wday = (wday_base + 4) % 7;
	if (wday < 0)
	{
		wday += 7;
	}
	lt->tm_wday = (int)wday;
}

static inline int format_time(char str[TIME_STR_SIZE], time_t *cur_time_p,
							  const char *time_format, int timezone_hour)
{
	struct tm lt;
	// localtime_r((const time_t*)(cur_time_p), &lt); //here has performance issues on multithread.
	time_util_fast_second2date((const time_t *)(cur_time_p), &lt, timezone_hour);
	return (int)strftime(str, TIME_STR_SIZE, time_format, &lt);
}

// simple snprintf .%03d : snprintf(buf, BUF_SIZE, ".%03d", num);
static inline void print_millisec(char *buffer, unsigned int num)
{
#if (defined(USE_SNPRINTF_MILLISECONDS) && USE_SNPRINTF_MILLISECONDS)
	snprintf(buffer, 5, ".%03d", num);
#else
	buffer[0] = '.';
	++buffer;
	for (int i = 3; i > 0; --i)
	{
		buffer[i - 1] = '0' + ((0 == num) ? 0 : (num % 10));
		num /= 10;
	}
	buffer[3] = '\0';
#endif // USE_SNPRINTF_MILLISECONDS
}

static inline int get_time_str(char str[TIME_STR_SIZE], struct timeval *tval_p,
							   const char *time_format, const int timezone_hour, bool use_cache)
{
	int ftime_len = 0;
	time_t cur_time = (time_t)(tval_p->tv_sec);

#if (defined(USE_TIME_CACHE) && USE_TIME_CACHE)
	do
	{
		if (!use_cache)
		{
			break;
		}
		time_cache_t *cache_p = &g_time_caches[GET_TIME_FORMAT_TYPE(time_format)];
		if (!cache_p->rw_lock)
		{
			break;
		}
		TIME_UTIL_RD_LOCK(&cache_p->rw_lock); // lock rdlock
		if (cache_p->format_len > 0 && cache_p->timezone_hour == timezone_hour && cache_p->tval.tv_sec == tval_p->tv_sec)
		{   // hit cache, just copy whole cached time string
			const int cached_len = cache_p->format_len;
			if (cached_len > 0)
			{
				memcpy(str, cache_p->format_cache, (size_t)cached_len + 1U);
			}
			else
			{
				str[0] = '\0';
			}
			ftime_len = cached_len;
			const bool update_millis = (cache_p->tval.tv_usec != tval_p->tv_usec);
			TIME_UTIL_RW_UNLOCK(&cache_p->rw_lock); // unlock rdlock
			if (update_millis)
			{
				print_millisec(str + (ftime_len - 4), (unsigned int)(tval_p->tv_usec / 1000));
			}
			return ftime_len;
		}

		TIME_UTIL_RW_UNLOCK(&cache_p->rw_lock); // unlock rdlock

		ftime_len = format_time(str, &cur_time, time_format, timezone_hour);
		print_millisec(str + ftime_len, (unsigned int)(tval_p->tv_usec / 1000));
		ftime_len += 4;

		TIME_UTIL_WR_LOCK(&cache_p->rw_lock); // lock wrlock
		if (cache_p->tval.tv_sec != tval_p->tv_sec || timezone_hour != cache_p->timezone_hour || cache_p->format_len <= 0)
		{
			memcpy(cache_p->format_cache, str, (size_t)ftime_len + 1U);
			cache_p->format_len = ftime_len;
			cache_p->timezone_hour = timezone_hour;
			cache_p->tval = *tval_p;
		}
		else
		{
			cache_p->tval.tv_usec = tval_p->tv_usec;
		}
		TIME_UTIL_RW_UNLOCK(&cache_p->rw_lock); // unlock wrlock
		return ftime_len;
	} while (0);
#endif // USE_TIME_CACHE

	ftime_len = format_time(str, &cur_time, time_format, timezone_hour);
	print_millisec(str + ftime_len, (unsigned int)(tval_p->tv_usec / 1000));
	ftime_len += 4;
	return ftime_len;
}

int time_util_get_time_str(struct timeval *tval_p, char str[TIME_STR_SIZE], int timezone_hour)
{
	return get_time_str(str, tval_p, TIME_STAMP_FORMAT, timezone_hour, false);
}

int time_util_get_time_str_current(char str[TIME_STR_SIZE], int timezone_hour)
{
	struct timeval tv;
	gettimeofday(&tv, NULL); // get current time
	return get_time_str(str, &tv, TIME_STAMP_FORMAT, timezone_hour, true);
}

int time_util_get_time_str_for_file_name(struct timeval *tval_p, char str[TIME_STR_SIZE], int timezone_hour)
{
	return get_time_str(str, tval_p, TIME_STAMP_FORMAT_FOR_FILE_NAME, timezone_hour, false);
}

int time_util_get_time_str_for_file_name_current(char str[TIME_STR_SIZE], int timezone_hour)
{
	struct timeval tv;
	gettimeofday(&tv, NULL); // get current time
	return get_time_str(str, &tv, TIME_STAMP_FORMAT_FOR_FILE_NAME, timezone_hour, true);
}

void time_util_current_ms(uint64_t *p_cur_ms)
{
	struct timeval cur_time;
	gettimeofday(&cur_time, NULL);
	*p_cur_ms = ((uint64_t)cur_time.tv_sec * 1000U + (uint64_t)cur_time.tv_usec / 1000U);
}

#ifdef _WIN32

typedef uint64_t (*fn_get_performance_freq)();

static uint64_t g_cached_milli_perf_freq = 0;

static inline uint64_t get_cached_performance_freq()
{
	return g_cached_milli_perf_freq;
}

// onetime initializer
static uint64_t init_get_milli_perf_freq();

static fn_get_performance_freq g_get_milli_perf_freq = &init_get_milli_perf_freq;

static uint64_t init_get_milli_perf_freq()
{
	LARGE_INTEGER frequency = {0}; // how many clock period on one seconds.
	QueryPerformanceFrequency(&frequency);
	g_cached_milli_perf_freq = frequency.QuadPart / 1000;
	ASSERT_ABORT(g_cached_milli_perf_freq > 0);

	g_get_milli_perf_freq = &get_cached_performance_freq;
	return (*g_get_milli_perf_freq)();
}
#endif // _WIN32

void time_util_query_performance_ms(uint64_t *p_cur_ms)
{
#ifdef _WIN32
	// QueryPerformanceFrequency Retrieves the frequency of the performance counter.
	// The frequency of the performance counter is fixed at system boot and is consistent across all processors.
	// Therefore, the frequency need only be queried upon application initialization, and the result can be cached.
	uint64_t milli_frequency = (*g_get_milli_perf_freq)();
	LARGE_INTEGER cur_counter;
	QueryPerformanceCounter(&cur_counter);
	*p_cur_ms = cur_counter.QuadPart / milli_frequency;
#else
	time_util_current_ms(p_cur_ms);
#endif // _WIN32
}
