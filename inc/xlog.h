#pragma once
#ifndef _XLOG_H
#define _XLOG_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdio.h>

	typedef enum {
		LOG_LEVEL_OFF = 0,
		LOG_LEVEL_VERBOSE = 1,
		LOG_LEVEL_DEBUG = 2,
		LOG_LEVEL_INFO = 3,
		LOG_LEVEL_WARN = 4,
		LOG_LEVEL_ERROR = 5
	} LogLevel;

	typedef enum {
		LOG_TARGET_ANDROID = (0x1 << 1), // NOLINT(hicpp-signed-bitwise)
		LOG_TARGET_CONSOLE = (0x1 << 2) // NOLINT(hicpp-signed-bitwise)
	} LogTarget;

#define LOG_LINE_STAR "****************************************************************"
#define LOG_TAG_DEFAULT "XTest"
#define CONSOLE_LOG_CONFIG_METHOD printf
#define CONSOLE_LOG_CONFIG_NEW_LINE_FORMAT "\r\n"
#define IS_LOGABLE(level) (xlog_config_level && level >= xlog_config_level)

    //#define xlog_config_level LOG_LEVEL_VERBOSE
    extern int xlog_config_level;
    //#define xlog_config_target (LOG_TARGET_ANDROID | LOG_TARGET_CONSOLE)
    extern int xlog_config_target;

    void xlog_chars2hex(char* str, size_t str_capacity, const char* chars, size_t chars_len);

#ifdef _WIN32 //for _WIN32

#include <windows.h>
#include <sys/timeb.h>

    static __inline long long time_stamp_current() {
        struct timeb rawtime;
        ftime(&rawtime);
        return rawtime.time * 1000 + rawtime.millitm;
    }

#define __func__ __FUNCTION__

//msvc-doesnt-expand-va-args-correctly https://stackoverflow.com/questions/5134523/msvc-doesnt-expand-va-args-correctly
#define EXPAND_VA_ARGS( x ) x
//#define F(x, ...) X = x and VA_ARGS = __VA_ARGS__
//#define G(...) EXPAND_VA_ARGS( F(__VA_ARGS__) )

#define CONSOLE_LOG_NO_NEW_LINE_HELPER(level, ...) if(IS_LOGABLE(level)) { CONSOLE_LOG_CONFIG_METHOD( __VA_ARGS__ ); }
#define CONSOLE_LOG_NO_NEW_LINE(level, ...) EXPAND_VA_ARGS( CONSOLE_LOG_NO_NEW_LINE_HELPER(level, __VA_ARGS__) )

#else  //for unix console

#include <sys/time.h>

    static inline long long time_stamp_current() {
        struct timeval te;
        gettimeofday(&te, NULL); // get current time
        return te.tv_sec * 1000LL + te.tv_usec / 1000;
    }

#define CONSOLE_LOG_NO_NEW_LINE(level, ...) if(IS_LOGABLE(level)) { CONSOLE_LOG_CONFIG_METHOD( __VA_ARGS__ ); }

#endif // _WIN32

#define CONSOLE_LOGV_NO_NEW_LINE(...) CONSOLE_LOG_NO_NEW_LINE(LOG_LEVEL_VERBOSE, ##__VA_ARGS__)
#define CONSOLE_LOGD_NO_NEW_LINE(...) CONSOLE_LOG_NO_NEW_LINE(LOG_LEVEL_DEBUG, ##__VA_ARGS__)
#define CONSOLE_LOGI_NO_NEW_LINE(...) CONSOLE_LOG_NO_NEW_LINE(LOG_LEVEL_INFO, ##__VA_ARGS__)
#define CONSOLE_LOGW_NO_NEW_LINE(...) CONSOLE_LOG_NO_NEW_LINE(LOG_LEVEL_WARN, ##__VA_ARGS__)
#define CONSOLE_LOGE_NO_NEW_LINE(...) CONSOLE_LOG_NO_NEW_LINE(LOG_LEVEL_ERROR, ##__VA_ARGS__)

#define CONSOLE_TLOGV(tag, fmt, ...)  CONSOLE_LOG_NO_NEW_LINE(LOG_LEVEL_VERBOSE,"[%lld][V][%s] " fmt CONSOLE_LOG_CONFIG_NEW_LINE_FORMAT,time_stamp_current(), tag, ##__VA_ARGS__)
#define CONSOLE_TLOGD(tag, fmt, ...)  CONSOLE_LOG_NO_NEW_LINE(LOG_LEVEL_DEBUG,"[%lld][D][%s] " fmt CONSOLE_LOG_CONFIG_NEW_LINE_FORMAT,time_stamp_current(), tag, ##__VA_ARGS__)
#define CONSOLE_TLOGI(tag, fmt, ...)  CONSOLE_LOG_NO_NEW_LINE(LOG_LEVEL_INFO,"[%lld][I][%s] " fmt CONSOLE_LOG_CONFIG_NEW_LINE_FORMAT,time_stamp_current(), tag, ##__VA_ARGS__)
#define CONSOLE_TLOGW(tag, fmt, ...)  CONSOLE_LOG_NO_NEW_LINE(LOG_LEVEL_WARN,"[%lld][W][%s] " fmt CONSOLE_LOG_CONFIG_NEW_LINE_FORMAT,time_stamp_current(), tag, ##__VA_ARGS__)
#define CONSOLE_TLOGE(tag, fmt, ...)  CONSOLE_LOG_NO_NEW_LINE(LOG_LEVEL_ERROR,"[%lld][E][%s] " fmt CONSOLE_LOG_CONFIG_NEW_LINE_FORMAT,time_stamp_current(), tag, ##__VA_ARGS__)

#define CONSOLE_LOGV(fmt, ...) CONSOLE_TLOGV(LOG_TAG_DEFAULT, fmt, ##__VA_ARGS__)
#define CONSOLE_LOGD(fmt, ...) CONSOLE_TLOGD(LOG_TAG_DEFAULT, fmt, ##__VA_ARGS__)
#define CONSOLE_LOGI(fmt, ...) CONSOLE_TLOGI(LOG_TAG_DEFAULT, fmt, ##__VA_ARGS__)
#define CONSOLE_LOGW(fmt, ...) CONSOLE_TLOGW(LOG_TAG_DEFAULT, fmt, ##__VA_ARGS__)
#define CONSOLE_LOGE(fmt, ...) CONSOLE_TLOGE(LOG_TAG_DEFAULT, fmt, ##__VA_ARGS__)

#define CONSOLE_TLOGV_TRACE(tag, fmt, ...)  CONSOLE_TLOGV(tag, "[%s:%d] " fmt, __func__, __LINE__, ##__VA_ARGS__)
#define CONSOLE_TLOGD_TRACE(tag, fmt, ...)  CONSOLE_TLOGD(tag, "[%s:%d] " fmt, __func__, __LINE__, ##__VA_ARGS__)
#define CONSOLE_TLOGI_TRACE(tag, fmt, ...)  CONSOLE_TLOGI(tag, "[%s:%d] " fmt, __func__, __LINE__, ##__VA_ARGS__)
#define CONSOLE_TLOGW_TRACE(tag, fmt, ...)  CONSOLE_TLOGW(tag, "[%s:%d] " fmt, __func__, __LINE__, ##__VA_ARGS__)
#define CONSOLE_TLOGE_TRACE(tag, fmt, ...)  CONSOLE_TLOGE(tag, "[%s:%d] " fmt, __func__, __LINE__, ##__VA_ARGS__)

#define CONSOLE_LOGV_TRACE(fmt, ...) CONSOLE_TLOGV_TRACE(LOG_TAG_DEFAULT, fmt, ##__VA_ARGS__)
#define CONSOLE_LOGD_TRACE(fmt, ...) CONSOLE_TLOGD_TRACE(LOG_TAG_DEFAULT, fmt, ##__VA_ARGS__)
#define CONSOLE_LOGI_TRACE(fmt, ...) CONSOLE_TLOGI_TRACE(LOG_TAG_DEFAULT, fmt, ##__VA_ARGS__)
#define CONSOLE_LOGW_TRACE(fmt, ...) CONSOLE_TLOGW_TRACE(LOG_TAG_DEFAULT, fmt, ##__VA_ARGS__)
#define CONSOLE_LOGE_TRACE(fmt, ...) CONSOLE_TLOGE_TRACE(LOG_TAG_DEFAULT, fmt, ##__VA_ARGS__)

#if defined(__ANDROID__)

#include <android/log.h>

#define A_TLOGV(tag, fmt, ...) if(IS_LOGABLE(LOG_LEVEL_VERBOSE)) {\
        __android_log_print(ANDROID_LOG_VERBOSE, tag, fmt, ##__VA_ARGS__);\
    }
#define A_TLOGD(tag, fmt, ...) if(IS_LOGABLE(LOG_LEVEL_DEBUG)) {\
        __android_log_print(ANDROID_LOG_DEBUG, tag, fmt, ##__VA_ARGS__);\
    }
#define A_TLOGI(tag, fmt, ...) if(IS_LOGABLE(LOG_LEVEL_INFO)) {\
        __android_log_print(ANDROID_LOG_INFO, tag, fmt, ##__VA_ARGS__);\
    }
#define A_TLOGW(tag, fmt, ...) if(IS_LOGABLE(LOG_LEVEL_WARN)) {\
        __android_log_print(ANDROID_LOG_WARN, tag, fmt, ##__VA_ARGS__);\
    }
#define A_TLOGE(tag, fmt, ...) if(IS_LOGABLE(LOG_LEVEL_ERROR)) {\
        __android_log_print(ANDROID_LOG_ERROR, tag, fmt, ##__VA_ARGS__);\
    }

#define A_TLOGV_TRACE(tag, fmt, ...) A_TLOGV(tag, "[%s:%d] " fmt , __func__, __LINE__, ##__VA_ARGS__)
#define A_TLOGD_TRACE(tag, fmt, ...) A_TLOGD(tag, "[%s:%d] " fmt , __func__, __LINE__, ##__VA_ARGS__)
#define A_TLOGI_TRACE(tag, fmt, ...) A_TLOGI(tag, "[%s:%d] " fmt , __func__, __LINE__, ##__VA_ARGS__)
#define A_TLOGW_TRACE(tag, fmt, ...) A_TLOGW(tag, "[%s:%d] " fmt , __func__, __LINE__, ##__VA_ARGS__)
#define A_TLOGE_TRACE(tag, fmt, ...) A_TLOGE(tag, "[%s:%d] " fmt , __func__, __LINE__, ##__VA_ARGS__)

#define A_LOGV(...) A_TLOGV(LOG_TAG_DEFAULT, ##__VA_ARGS__)
#define A_LOGD(...) A_TLOGD(LOG_TAG_DEFAULT, ##__VA_ARGS__)
#define A_LOGI(...) A_TLOGI(LOG_TAG_DEFAULT, ##__VA_ARGS__)
#define A_LOGW(...) A_TLOGW(LOG_TAG_DEFAULT, ##__VA_ARGS__)
#define A_LOGE(...) A_TLOGE(LOG_TAG_DEFAULT, ##__VA_ARGS__)

#define A_LOGV_TRACE(fmt, ...) A_TLOGV_TRACE(LOG_TAG_DEFAULT, fmt, ##__VA_ARGS__)
#define A_LOGD_TRACE(fmt, ...) A_TLOGD_TRACE(LOG_TAG_DEFAULT, fmt, ##__VA_ARGS__)
#define A_LOGI_TRACE(fmt, ...) A_TLOGI_TRACE(LOG_TAG_DEFAULT, fmt, ##__VA_ARGS__)
#define A_LOGW_TRACE(fmt, ...) A_TLOGW_TRACE(LOG_TAG_DEFAULT, fmt, ##__VA_ARGS__)
#define A_LOGE_TRACE(fmt, ...) A_TLOGE_TRACE(LOG_TAG_DEFAULT, fmt, ##__VA_ARGS__)

#else

#define A_TLOGV(TAG, fmt, ...)
#define A_TLOGD(TAG, fmt, ...)
#define A_TLOGI(TAG, fmt, ...)
#define A_TLOGW(TAG, fmt, ...)
#define A_TLOGE(TAG, fmt, ...)

#define A_TLOGV_TRACE(TAG, fmt, ...)
#define A_TLOGD_TRACE(TAG, fmt, ...)
#define A_TLOGI_TRACE(TAG, fmt, ...)
#define A_TLOGW_TRACE(TAG, fmt, ...)
#define A_TLOGE_TRACE(TAG, fmt, ...)

#define A_LOGV(...)
#define A_LOGD(...)
#define A_LOGI(...)
#define A_LOGW(...)
#define A_LOGE(...)

#define A_LOGV_TRACE(fmt, ...)
#define A_LOGD_TRACE(fmt, ...)
#define A_LOGI_TRACE(fmt, ...)
#define A_LOGW_TRACE(fmt, ...)
#define A_LOGE_TRACE(fmt, ...)

#endif // defined(__ANDROID__)

#define  XLOG_IS_TARGET_ABLE(logTarget) (xlog_config_target & logTarget)
#define XLOG_CONSOLE_ABLE XLOG_IS_TARGET_ABLE(LOG_TARGET_CONSOLE)
#define XLOG_ANDROID_ABLE XLOG_IS_TARGET_ABLE(LOG_TARGET_ANDROID)

#define LOGV(fmt, ...) do{ \
    if(XLOG_CONSOLE_ABLE){CONSOLE_LOGV(fmt, ##__VA_ARGS__);}\
    if(XLOG_ANDROID_ABLE){A_LOGV(fmt, ##__VA_ARGS__);}\
}while(0);
#define LOGD(fmt, ...) do{ \
    if(XLOG_CONSOLE_ABLE){CONSOLE_LOGD(fmt, ##__VA_ARGS__);}\
    if(XLOG_ANDROID_ABLE){A_LOGD(fmt, ##__VA_ARGS__);}\
}while(0);
#define LOGI(fmt, ...) do{ \
    if(XLOG_CONSOLE_ABLE){CONSOLE_LOGI(fmt, ##__VA_ARGS__);}\
    if(XLOG_ANDROID_ABLE){A_LOGI(fmt, ##__VA_ARGS__);}\
}while(0);
#define LOGW(fmt, ...) do{ \
    if(XLOG_CONSOLE_ABLE){CONSOLE_LOGW(fmt, ##__VA_ARGS__);}\
    if(XLOG_ANDROID_ABLE){A_LOGW(fmt, ##__VA_ARGS__);}\
}while(0);
#define LOGE(fmt, ...) do{ \
    if(XLOG_CONSOLE_ABLE){CONSOLE_LOGE(fmt, ##__VA_ARGS__);}\
    if(XLOG_ANDROID_ABLE){A_LOGE(fmt, ##__VA_ARGS__);}\
}while(0);

#define LOGV_TRACE(fmt, ...)  do{ \
    if(XLOG_CONSOLE_ABLE){CONSOLE_LOGV_TRACE( fmt, ##__VA_ARGS__);}\
    if(XLOG_ANDROID_ABLE){A_LOGV_TRACE( fmt, ##__VA_ARGS__);}\
}while(0);
#define LOGD_TRACE(fmt, ...)  do{ \
    if(XLOG_CONSOLE_ABLE){CONSOLE_LOGD_TRACE( fmt, ##__VA_ARGS__);}\
    if(XLOG_ANDROID_ABLE){A_LOGD_TRACE( fmt, ##__VA_ARGS__);}\
}while(0);
#define LOGI_TRACE(fmt, ...)  do{ \
    if(XLOG_CONSOLE_ABLE){CONSOLE_LOGI_TRACE( fmt, ##__VA_ARGS__);}\
    if(XLOG_ANDROID_ABLE){A_LOGI_TRACE( fmt, ##__VA_ARGS__);}\
}while(0);
#define LOGW_TRACE(fmt, ...)  do{ \
    if(XLOG_CONSOLE_ABLE){CONSOLE_LOGW_TRACE( fmt, ##__VA_ARGS__);}\
    if(XLOG_ANDROID_ABLE){A_LOGW_TRACE( fmt, ##__VA_ARGS__);}\
}while(0);
#define LOGE_TRACE(fmt, ...)  do{ \
    if(XLOG_CONSOLE_ABLE){CONSOLE_LOGE_TRACE( fmt, ##__VA_ARGS__);}\
    if(XLOG_ANDROID_ABLE){A_LOGE_TRACE(fmt, ##__VA_ARGS__);}\
}while(0);

#define TLOGV(tag, fmt, ...)  do{ \
    if(XLOG_CONSOLE_ABLE){CONSOLE_TLOGV(tag, fmt, ##__VA_ARGS__);}\
    if(XLOG_ANDROID_ABLE){A_TLOGV(tag, fmt, ##__VA_ARGS__);}\
}while(0);
#define TLOGD(tag, fmt, ...)  do{ \
    if(XLOG_CONSOLE_ABLE){CONSOLE_TLOGD(tag, fmt, ##__VA_ARGS__);}\
    if(XLOG_ANDROID_ABLE){A_TLOGD(tag, fmt, ##__VA_ARGS__);}\
}while(0);
#define TLOGI(tag, fmt, ...)  do{ \
    if(XLOG_CONSOLE_ABLE){CONSOLE_TLOGI(tag, fmt, ##__VA_ARGS__);}\
    if(XLOG_ANDROID_ABLE){A_TLOGI(tag, fmt, ##__VA_ARGS__);}\
}while(0);
#define TLOGW(tag, fmt, ...)  do{ \
    if(XLOG_CONSOLE_ABLE){CONSOLE_TLOGW(tag, fmt, ##__VA_ARGS__);}\
    if(XLOG_ANDROID_ABLE){A_TLOGW(tag, fmt, ##__VA_ARGS__);}\
}while(0);
#define TLOGE(tag, fmt, ...)  do{ \
    if(XLOG_CONSOLE_ABLE){CONSOLE_TLOGE(tag, fmt, ##__VA_ARGS__);}\
    if(XLOG_ANDROID_ABLE){A_TLOGE(tag, fmt, ##__VA_ARGS__);}\
}while(0);

#define TLOGV_TRACE(tag, fmt, ...)  do{ \
    if(XLOG_CONSOLE_ABLE){CONSOLE_TLOGV_TRACE(tag, fmt, ##__VA_ARGS__);}\
    if(XLOG_ANDROID_ABLE){A_TLOGV_TRACE(tag, fmt, ##__VA_ARGS__);}\
}while(0);
#define TLOGD_TRACE(tag, fmt, ...)  do{ \
    if(XLOG_CONSOLE_ABLE){CONSOLE_TLOGD_TRACE(tag, fmt, ##__VA_ARGS__);}\
    if(XLOG_ANDROID_ABLE){A_TLOGD_TRACE(tag, fmt, ##__VA_ARGS__);}\
}while(0);
#define TLOGI_TRACE(tag, fmt, ...)  do{ \
    if(XLOG_CONSOLE_ABLE){CONSOLE_TLOGI_TRACE(tag, fmt, ##__VA_ARGS__);}\
    if(XLOG_ANDROID_ABLE){A_TLOGI_TRACE(tag, fmt, ##__VA_ARGS__);}\
}while(0);
#define TLOGW_TRACE(tag, fmt, ...)  do{ \
    if(XLOG_CONSOLE_ABLE){CONSOLE_TLOGW_TRACE(tag, fmt, ##__VA_ARGS__);}\
    if(XLOG_ANDROID_ABLE){A_TLOGW_TRACE(tag, fmt, ##__VA_ARGS__);}\
}while(0);
#define TLOGE_TRACE(tag, fmt, ...)  do{ \
    if(XLOG_CONSOLE_ABLE){CONSOLE_TLOGE_TRACE(tag, fmt, ##__VA_ARGS__);}\
    if(XLOG_ANDROID_ABLE){A_TLOGE_TRACE(tag, fmt, ##__VA_ARGS__);}\
}while(0);

#ifdef __cplusplus
}
#endif

#endif //_XLOG_H