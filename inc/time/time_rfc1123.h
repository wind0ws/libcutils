#pragma once
#ifndef __LCU_TIME_RFC1123_H
#define __LCU_TIME_RFC1123_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#define TIME_RFC1123_STR_LEN (29)

int time_rfc1123_now(char *out_time_str, size_t out_time_str_len);

#ifdef __cplusplus
};
#endif

#endif //__LCU_TIME_RFC1123_H
