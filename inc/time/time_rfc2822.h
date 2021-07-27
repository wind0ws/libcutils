#pragma once
#ifndef LCU_TIME_RFC2822_H
#define LCU_TIME_RFC2822_H

#include <stddef.h> /* for size_t */
#include <time.h>   /* for time_t */

#ifdef __cplusplus
extern "C" {
#endif

#define TIME_RFC2822_STR_SIZE (32)
#define TIME_RFC2822_UTC_STR_SIZE (30)

/**
 * get rfc2822 time format 
 * how to init time_t : time_t cur_time; time(&cur_time);
 * @return Format : Fri, 11 Jun 2021 03:42:56 +0800 
 */
int time_rfc2822(time_t* the_time, char *out_time_str, size_t out_time_str_size);

/**
 * get rfc2822 UTC time format
 * how to init time_t : time_t cur_time; time(&cur_time);
 * @return Format : Thu, 09 Jan 2020 06:11:13 UTC
 */
int time_rfc2822_utc(time_t* the_time, char* out_time_str, size_t out_time_str_size);

#ifdef __cplusplus
};
#endif

#endif // !LCU_TIME_RFC2822_H
