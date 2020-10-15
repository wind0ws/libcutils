#pragma once
#ifndef LCU_TIME_UTIL_H
#define LCU_TIME_UTIL_H

#include <time.h>
#include <stdint.h>
#ifdef _WIN32
#include <winsock.h>  /* for take struct timeval */
#include <sys/timeb.h>
#else
#include <sys/time.h>
#endif // _WIN32

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#ifdef _WIN32
	static struct tm* gmtime_r(const time_t* timerp, struct tm* result)
	{
		gmtime_s(result, timerp);
		return result;
	}

	static struct tm* localtime_r(const time_t* timerp, struct tm* result)
	{
		localtime_s(result, timerp);
		return result;
	}

	/*
	 * gettimeofday port for windows.
	 * timezone information is stored outside the kernel, so tzp isn't used anymore(should always NULL).
	 *
	 * Note: this function is not for Win32 high precision timing purpose. See
	 * elapsed_time().
	 */
	int gettimeofday(struct timeval* tp, struct timezone* tzp);
#endif // _WIN32

	/**
	 * get current time zone offset.(unit is seconds)
	 */
	int time_util_zone_offset_seconds_to_utc();

	/**
	 * transform seconds(time_t) to local time (struct tm).
	 * timezone_hour is the offset hour to UTC.
	 * this function just like localtime_r, but need caller provide timezone for calculate.
	 * use this function instead of localtime_r, because localtime_r have performance issue on multi thread.
	 */
	int time_util_fast_second2date(const time_t* p_unix_sec, struct tm* lt, int timezone_hour);

#define TIME_STR_LEN (24)

	/**
	 * get current time str, contains milliseconds.
	 * timezone_hour is the offset hour to UTC.
	 */
	int time_util_get_current_time_str(char str[TIME_STR_LEN], int timezone_hour);

	/**
	 * get current time str for file name.
	 * this function just like time_util_get_current_time_str,
	 * but replace colon to underline, because file name can not contains colon.
	 */
	int time_util_get_current_time_str_for_file_name(char str[TIME_STR_LEN], int timezone_hour);

	/**
	 * get current milliseconds. since 1970 Jan 1.
	 * implement by gettimeofday, so timezone is your current locale.
	 */
	void time_util_current_milliseconds(int64_t* p_cur_ms);

	/**
	 * get current milliseconds tick for compare performance.
	 * p_cur_ms value unit is milliseconds, but NOT guarantee equal to current milliseconds,
	 * which means this function is NOT as same as time_util_current_milliseconds.
	 */
	void time_util_query_performance_ms(int64_t* p_cur_ms);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // LCU_TIME_UTIL_H
