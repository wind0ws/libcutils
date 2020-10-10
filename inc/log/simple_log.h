#pragma once
#ifndef __LCU_SIMPLE_LOG_H
#define __LCU_SIMPLE_LOG_H

#include <stdio.h>

#ifdef _WIN32

//msvc-doesnt-expand-va-args-correctly https://stackoverflow.com/questions/5134523/msvc-doesnt-expand-va-args-correctly
#define __SIMPLELOG_WIN_EXPAND_VA_ARGS( x ) x
//#define F(x, ...) X = x and VA_ARGS = __VA_ARGS__
//#define G(...) __RING_EXPAND_VA_ARGS( F(__VA_ARGS__) )

#define __CONSOLE_LOG_HELPER(level, ...)   printf( __VA_ARGS__ )
#define __CONSOLE_LOG(level, ...)          __SIMPLELOG_WIN_EXPAND_VA_ARGS( __CONSOLE_LOG_HELPER(level, __VA_ARGS__) )
#define __CONSOLE_LOGV(...)                __CONSOLE_LOG(1, ##__VA_ARGS__)
#define __CONSOLE_LOGD(...)                __CONSOLE_LOG(2, ##__VA_ARGS__)
#define __CONSOLE_LOGI(...)                __CONSOLE_LOG(3, ##__VA_ARGS__)
#define __CONSOLE_LOGW(...)                __CONSOLE_LOG(4, ##__VA_ARGS__)
#define __CONSOLE_LOGE(...)                __CONSOLE_LOG(5, ##__VA_ARGS__)

#define __CONSOLE_TLOGV(tag, fmt, ...)     __CONSOLE_LOG(1,"[V][%s] " fmt "\n", tag, ##__VA_ARGS__)
#define __CONSOLE_TLOGD(tag, fmt, ...)     __CONSOLE_LOG(2,"[D][%s] " fmt "\n", tag, ##__VA_ARGS__)
#define __CONSOLE_TLOGI(tag, fmt, ...)     __CONSOLE_LOG(3,"[I][%s] " fmt "\n", tag, ##__VA_ARGS__)
#define __CONSOLE_TLOGW(tag, fmt, ...)     __CONSOLE_LOG(4,"[W][%s] " fmt "\n", tag, ##__VA_ARGS__)
#define __CONSOLE_TLOGE(tag, fmt, ...)     __CONSOLE_LOG(5,"[E][%s] " fmt "\n", tag, ##__VA_ARGS__)

#define __LOG_HELPER_V(tag, fmt, ...)      __CONSOLE_TLOGV(tag, fmt, ##__VA_ARGS__)
#define __LOG_HELPER_D(tag, fmt, ...)      __CONSOLE_TLOGD(tag, fmt, ##__VA_ARGS__)
#define __LOG_HELPER_I(tag, fmt, ...)      __CONSOLE_TLOGI(tag, fmt, ##__VA_ARGS__)
#define __LOG_HELPER_W(tag, fmt, ...)      __CONSOLE_TLOGW(tag, fmt, ##__VA_ARGS__)
#define __LOG_HELPER_E(tag, fmt, ...)      __CONSOLE_TLOGE(tag, fmt, ##__VA_ARGS__)

#elif(defined(__ANDROID__)) //android

#include <android/log.h>
#define __LOG_HELPER_V(tag, fmt,...)       __android_log_print(ANDROID_LOG_VERBOSE, tag, fmt, ##__VA_ARGS__)
#define __LOG_HELPER_D(tag, fmt,...)       __android_log_print(ANDROID_LOG_DEBUG, tag, fmt, ##__VA_ARGS__)
#define __LOG_HELPER_I(tag, fmt,...)       __android_log_print(ANDROID_LOG_INFO, tag, fmt, ##__VA_ARGS__)
#define __LOG_HELPER_W(tag, fmt,...)       __android_log_print(ANDROID_LOG_WARN, tag, fmt, ##__VA_ARGS__)
#define __LOG_HELPER_E(tag, fmt,...)       __android_log_print(ANDROID_LOG_ERROR, tag, fmt, ##__VA_ARGS__)

#else //unix printf

#define __LOG_HELPER(...)                  printf(__VA_ARGS__)
#define __LOG_HELPER_V(tag, fmt,...)       __LOG_HELPER("[V][%s] "fmt"\n", tag, ##__VA_ARGS__)
#define __LOG_HELPER_D(tag, fmt,...)       __LOG_HELPER("[D][%s] "fmt"\n", tag, ##__VA_ARGS__)
#define __LOG_HELPER_I(tag, fmt,...)       __LOG_HELPER("[I][%s] "fmt"\n", tag, ##__VA_ARGS__)
#define __LOG_HELPER_W(tag, fmt,...)       __LOG_HELPER("[W][%s] "fmt"\n", tag, ##__VA_ARGS__)
#define __LOG_HELPER_E(tag, fmt,...)       __LOG_HELPER("[E][%s] "fmt"\n", tag, ##__VA_ARGS__)

#endif // _WIN32

#define SIMPLE_LOGV(tag, fmt,...)          __LOG_HELPER_V(tag, fmt, ##__VA_ARGS__)
#define SIMPLE_LOGD(tag, fmt,...)          __LOG_HELPER_D(tag, fmt, ##__VA_ARGS__)
#define SIMPLE_LOGI(tag, fmt,...)          __LOG_HELPER_I(tag, fmt, ##__VA_ARGS__)
#define SIMPLE_LOGW(tag, fmt,...)          __LOG_HELPER_W(tag, fmt, ##__VA_ARGS__)
#define SIMPLE_LOGE(tag, fmt,...)          __LOG_HELPER_E(tag, fmt, ##__VA_ARGS__)

#endif // __LCU_SIMPLE_LOG_H
