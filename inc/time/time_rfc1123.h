#pragma once
#ifndef LCU_TIME_RFC1123_H
#define LCU_TIME_RFC1123_H

#include <time.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TIME_RFC1123_STR_SIZE (30)

/**
 * get rfc1123 time format 
 * how to init time_t : time_t cur_time; time(&cur_time);
 * @return Format : Fri, 11 Jun 2021 03:42:56 GMT
 */
int time_rfc1123(time_t* the_time, char *out_time_str, size_t out_time_str_size);

#ifdef __cplusplus
};
#endif

#endif // !LCU_TIME_RFC1123_H
