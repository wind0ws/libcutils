#pragma once
#ifndef LCU_TIME_UTIL_H
#define LCU_TIME_UTIL_H

#include <stdint.h>
#include <time.h>
#include <string.h>

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable: 5105)
#include <winsock.h>  /* for take struct timeval */
#include <sys/timeb.h>
#pragma warning(pop)
#ifdef _USE_32BIT_TIME_T
#error "_USE_32BIT_TIME_T is not supported, should remove it."
#endif // _USE_32BIT_TIME_T
#else
#include <sys/time.h>
#endif // _WIN32

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

	/**
	 * transform seconds(time_t) to local time (struct tm).
	 * timezone_hour is the offset hour to UTC.
	 * this function just like localtime_r, but need caller provide timezone for calculate.
	 * use this function instead of localtime_r, because localtime_r have performance issue on multi thread.
	 *
	 * @param p_unix_sec : UTC seconds since 1970,1,1
	 * @param lt : local time
	 * @param timezone_hour : timezone, should between -12 ~ 12
	 */
	void time_util_fast_second2date(const time_t* p_unix_sec, struct tm* lt, int timezone_hour);

	/**
	 * global init once.
	 * 
	 * generally speaking, users do not need to call this method, 
	 * lcu_global_init will call it at the appropriate time.
	 */
	int time_util_global_init();

	/**
	 * global cleanup. 
	 * 
	 * just like above, 
     * lcu_global_cleanup will call it at the appropriate time.
	 */
	int time_util_global_cleanup();

#ifdef _WIN32

	// transform gmtime_s to gmtime_r
	static inline struct tm* gmtime_r(const time_t* timerp, struct tm* result)
	{
		if (NULL == timerp || NULL == result)
		{
			return NULL;
		}
		__time64_t time64 = (__time64_t)(*timerp);
		if (0 != _gmtime64_s(result, &time64))
		{
			time_util_fast_second2date(timerp, result, 0);
		}
		return result;
	}

	// transform localtime_s to localtime_r
	static inline struct tm* localtime_r(const time_t* timerp, struct tm* result)
	{
		if (NULL == timerp || NULL == result)
		{
			return NULL;
		}
		__time64_t time64 = (__time64_t)(*timerp);
		if (0 != _localtime64_s(result, &time64))
		{
			memset(result, 0, sizeof(*result));
			return NULL;
		}
		return result;
	}

	/*
	 * gettimeofday port for windows.
	 * timezone information is stored outside the kernel, so tzp isn't used anymore(should always NULL).
	 *
	 * Note: this function is not for Win32 high precision timing purpose. See elapsed_time().
	 */
	int gettimeofday(struct timeval* tp, struct timezone* tzp);
#endif // _WIN32

	/**
	 * get current time zone offset.(unit is seconds)
	 */
	int time_util_zone_offset_seconds_to_utc();

#define TIME_STR_SIZE (24)

	/**
	 * get time str, contains milliseconds. 
	 * how to get struct timeval : struct timeval tv; gettimeofday(&tv, NULL);
	 */
	int time_util_get_time_str(struct timeval* tval_p, char str[TIME_STR_SIZE], int timezone_hour);

	/**
	 * get current time str, contains milliseconds.
	 * @param timezone_hour : the offset hour to UTC.
	 * @return time string length.
	 */
	int time_util_get_time_str_current(char str[TIME_STR_SIZE], int timezone_hour);

	/**
	 * get time str for file name.
	 * how to get struct timeval : struct timeval tv; gettimeofday(&tv, NULL);
	 */
	int time_util_get_time_str_for_file_name(struct timeval* tval_p, char str[TIME_STR_SIZE], int timezone_hour);

	/**
	 * get current time str for file name.
	 * this function just like time_util_get_time_str_current,
	 * but replace colon to underline, because file name can not contains colon.
	 * @return time string length.
	 */
	int time_util_get_time_str_for_file_name_current(char str[TIME_STR_SIZE], int timezone_hour);

	/**
	 * get current milliseconds. since 1970 Jan 1.
	 * implement by gettimeofday, so timezone is your current locale.
	 */
	void time_util_current_ms(uint64_t* p_cur_ms);

	/**
	 * get current milliseconds tick for compare performance.
	 * p_cur_ms value unit is milliseconds, but NOT guarantee equal to current milliseconds,
	 * which means this function is NOT as same as time_util_current_milliseconds.
	 */
	void time_util_query_performance_ms(uint64_t* p_cur_ms);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // !LCU_TIME_UTIL_H
