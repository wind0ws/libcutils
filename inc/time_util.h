#pragma once
#ifndef __TIME_UTIL_H
#define __TIME_UTIL_H

#include <time.h>
#ifdef _WIN32
#include <sys/timeb.h>
#else
#include <sys/time.h>
#endif // _WIN32

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define TIME_STR_LEN (24)
	void time_util_get_current_time_str(char str[TIME_STR_LEN]);

	void time_util_get_current_time_str_for_file_name(char str[TIME_STR_LEN]);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __TIME_UTIL_H


