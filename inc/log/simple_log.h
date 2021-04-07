#pragma once
#ifndef LCU_SIMPLE_LOG_H
#define LCU_SIMPLE_LOG_H

#include <stdio.h>

#ifdef _WIN32

//msvc-doesnt-expand-va-args-correctly https://stackoverflow.com/questions/5134523/msvc-doesnt-expand-va-args-correctly
#define _SIMPLELOG_WIN_EXPAND_VA_ARGS( x ) x
//#define F(x, ...) X = x and VA_ARGS = __VA_ARGS__
//#define G(...) _SIMPLELOG_WIN_EXPAND_VA_ARGS( F(__VA_ARGS__) )

#define _CONSOLE_LOG_HELPER(level, ...)   printf( __VA_ARGS__ )
#define _CONSOLE_LOG(level, ...)          _SIMPLELOG_WIN_EXPAND_VA_ARGS( _CONSOLE_LOG_HELPER(level, __VA_ARGS__) )
#define _CONSOLE_LOGV(...)                _CONSOLE_LOG(1, ##__VA_ARGS__)
#define _CONSOLE_LOGD(...)                _CONSOLE_LOG(2, ##__VA_ARGS__)
#define _CONSOLE_LOGI(...)                _CONSOLE_LOG(3, ##__VA_ARGS__)
#define _CONSOLE_LOGW(...)                _CONSOLE_LOG(4, ##__VA_ARGS__)
#define _CONSOLE_LOGE(...)                _CONSOLE_LOG(5, ##__VA_ARGS__)

#define _CONSOLE_TLOGV(tag, fmt, ...)     _CONSOLE_LOG(1,"[V][%s] " fmt "\n", tag, ##__VA_ARGS__)
#define _CONSOLE_TLOGD(tag, fmt, ...)     _CONSOLE_LOG(2,"[D][%s] " fmt "\n", tag, ##__VA_ARGS__)
#define _CONSOLE_TLOGI(tag, fmt, ...)     _CONSOLE_LOG(3,"[I][%s] " fmt "\n", tag, ##__VA_ARGS__)
#define _CONSOLE_TLOGW(tag, fmt, ...)     _CONSOLE_LOG(4,"[W][%s] " fmt "\n", tag, ##__VA_ARGS__)
#define _CONSOLE_TLOGE(tag, fmt, ...)     _CONSOLE_LOG(5,"[E][%s] " fmt "\n", tag, ##__VA_ARGS__)

#define _LOG_HELPER_V(tag, fmt, ...)      _CONSOLE_TLOGV(tag, fmt, ##__VA_ARGS__)
#define _LOG_HELPER_D(tag, fmt, ...)      _CONSOLE_TLOGD(tag, fmt, ##__VA_ARGS__)
#define _LOG_HELPER_I(tag, fmt, ...)      _CONSOLE_TLOGI(tag, fmt, ##__VA_ARGS__)
#define _LOG_HELPER_W(tag, fmt, ...)      _CONSOLE_TLOGW(tag, fmt, ##__VA_ARGS__)
#define _LOG_HELPER_E(tag, fmt, ...)      _CONSOLE_TLOGE(tag, fmt, ##__VA_ARGS__)

#elif(defined(__ANDROID__)) //android

#include <android/log.h>
#define _LOG_HELPER_V(tag, fmt,...)       __android_log_print(ANDROID_LOG_VERBOSE, tag, fmt, ##__VA_ARGS__)
#define _LOG_HELPER_D(tag, fmt,...)       __android_log_print(ANDROID_LOG_DEBUG, tag, fmt, ##__VA_ARGS__)
#define _LOG_HELPER_I(tag, fmt,...)       __android_log_print(ANDROID_LOG_INFO, tag, fmt, ##__VA_ARGS__)
#define _LOG_HELPER_W(tag, fmt,...)       __android_log_print(ANDROID_LOG_WARN, tag, fmt, ##__VA_ARGS__)
#define _LOG_HELPER_E(tag, fmt,...)       __android_log_print(ANDROID_LOG_ERROR, tag, fmt, ##__VA_ARGS__)

#else //unix printf

#define _LOG_HELPER(...)                  printf(__VA_ARGS__)
#define _LOG_HELPER_V(tag, fmt,...)       _LOG_HELPER("[V][%s] "fmt"\n", tag, ##__VA_ARGS__)
#define _LOG_HELPER_D(tag, fmt,...)       _LOG_HELPER("[D][%s] "fmt"\n", tag, ##__VA_ARGS__)
#define _LOG_HELPER_I(tag, fmt,...)       _LOG_HELPER("[I][%s] "fmt"\n", tag, ##__VA_ARGS__)
#define _LOG_HELPER_W(tag, fmt,...)       _LOG_HELPER("[W][%s] "fmt"\n", tag, ##__VA_ARGS__)
#define _LOG_HELPER_E(tag, fmt,...)       _LOG_HELPER("[E][%s] "fmt"\n", tag, ##__VA_ARGS__)

#endif // _WIN32

#define SIMPLE_LOGV(tag, fmt,...)          _LOG_HELPER_V(tag, fmt, ##__VA_ARGS__)
#define SIMPLE_LOGD(tag, fmt,...)          _LOG_HELPER_D(tag, fmt, ##__VA_ARGS__)
#define SIMPLE_LOGI(tag, fmt,...)          _LOG_HELPER_I(tag, fmt, ##__VA_ARGS__)
#define SIMPLE_LOGW(tag, fmt,...)          _LOG_HELPER_W(tag, fmt, ##__VA_ARGS__)
#define SIMPLE_LOGE(tag, fmt,...)          _LOG_HELPER_E(tag, fmt, ##__VA_ARGS__)

#endif // LCU_SIMPLE_LOG_H
