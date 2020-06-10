//
// Created by Administrator on 2019/9/6.
//

#ifndef C11STUDY_TIME_RFC1123_H
#define C11STUDY_TIME_RFC1123_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

#define TIME_RFC1123_STR_LEN (29)

int time_now_rfc1123(char *out_time_str, size_t out_time_str_len);

#ifdef __cplusplus
};
#endif

#endif //C11STUDY_TIME_RFC1123_H
