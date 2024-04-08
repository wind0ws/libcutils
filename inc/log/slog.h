#pragma once
#ifndef LCU_SLOG_H
#define LCU_SLOG_H

#include "log/logger_data.h" /* for common data */
#include <stddef.h>          /* for size_t      */

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

	extern LogLevel _g_slog_min_level;

	void slog_set_min_level(LogLevel min_level);

	LogLevel slog_get_min_level();

#if(!defined(_LCU_LOGGER_UNSUPPORT_PRINTF_REDIRECT) || 0 == _LCU_LOGGER_UNSUPPORT_PRINTF_REDIRECT)
	void slog_stdout2file(char* file_path);

	void slog_back2stdout();
#endif // !_LCU_LOGGER_UNSUPPORT_PRINTF_REDIRECT

	void __slog_internal_hex_print(int level, const char* tag, const char* chars, size_t chars_count);
	
#ifdef __cplusplus
}
#endif // __cplusplus

#ifdef __ANDROID__

#include <android/log.h>
#define _SLOGV_IMPL(tag, fmt, ...)    do { if (_g_slog_min_level && _g_slog_min_level <= LOG_LEVEL_VERBOSE) __android_log_print(ANDROID_LOG_VERBOSE, tag, fmt, ##__VA_ARGS__); } while (0)
#define _SLOGD_IMPL(tag, fmt, ...)    do { if (_g_slog_min_level && _g_slog_min_level <= LOG_LEVEL_DEBUG) __android_log_print(ANDROID_LOG_DEBUG,   tag, fmt, ##__VA_ARGS__); } while (0)
#define _SLOGI_IMPL(tag, fmt, ...)    do { if (_g_slog_min_level && _g_slog_min_level <= LOG_LEVEL_INFO) __android_log_print(ANDROID_LOG_INFO,    tag, fmt, ##__VA_ARGS__); } while (0)
#define _SLOGW_IMPL(tag, fmt, ...)    do { if (_g_slog_min_level && _g_slog_min_level <= LOG_LEVEL_WARN) __android_log_print(ANDROID_LOG_WARN,    tag, fmt, ##__VA_ARGS__); } while (0)
#define _SLOGE_IMPL(tag, fmt, ...)    do { if (_g_slog_min_level && _g_slog_min_level <= LOG_LEVEL_ERROR) __android_log_print(ANDROID_LOG_ERROR,   tag, fmt, ##__VA_ARGS__); } while (0)

#define _SLOGV_TRACE_IMPL(tag, func_name, line_num, fmt, ...)  do { if (_g_slog_min_level && _g_slog_min_level <= LOG_LEVEL_VERBOSE) __android_log_print(ANDROID_LOG_VERBOSE, tag, "(%s:%d) " fmt, func_name, line_num, ##__VA_ARGS__); } while (0)
#define _SLOGD_TRACE_IMPL(tag, func_name, line_num, fmt, ...)  do { if (_g_slog_min_level && _g_slog_min_level <= LOG_LEVEL_DEBUG) __android_log_print(ANDROID_LOG_DEBUG,   tag, "(%s:%d) " fmt, func_name, line_num, ##__VA_ARGS__); } while (0)
#define _SLOGI_TRACE_IMPL(tag, func_name, line_num, fmt, ...)  do { if (_g_slog_min_level && _g_slog_min_level <= LOG_LEVEL_INFO) __android_log_print(ANDROID_LOG_INFO,    tag, "(%s:%d) " fmt, func_name, line_num, ##__VA_ARGS__); } while (0)
#define _SLOGW_TRACE_IMPL(tag, func_name, line_num, fmt, ...)  do { if (_g_slog_min_level && _g_slog_min_level <= LOG_LEVEL_WARN) __android_log_print(ANDROID_LOG_WARN,    tag, "(%s:%d) " fmt, func_name, line_num, ##__VA_ARGS__); } while (0)
#define _SLOGE_TRACE_IMPL(tag, func_name, line_num, fmt, ...)  do { if (_g_slog_min_level && _g_slog_min_level <= LOG_LEVEL_ERROR) __android_log_print(ANDROID_LOG_ERROR,   tag, "(%s:%d) " fmt, func_name, line_num, ##__VA_ARGS__); } while (0)

#else

#ifdef _WIN32

//msvc-doesnt-expand-va-args-correctly https://stackoverflow.com/questions/5134523/msvc-doesnt-expand-va-args-correctly
#define _WIN_EXPAND_VA_ARGS( x ) x
//#define F(x, ...) X = x and VA_ARGS = __VA_ARGS__
//#define G(...) _SIMPLELOG_WIN_EXPAND_VA_ARGS( F(__VA_ARGS__) )

#define _CONSOLE_LOG_INTERIM(level, ...)   _PRINTF_FUNC( __VA_ARGS__ )
#define _CONSOLE_LOG(level, ...)           _WIN_EXPAND_VA_ARGS( _CONSOLE_LOG_INTERIM(level, __VA_ARGS__) )

#else //unix printf

#define _CONSOLE_LOG(level, ...)           _PRINTF_FUNC(__VA_ARGS__)

#endif // _WIN32

#define _SLOGV_IMPL(tag, fmt, ...) do { if (_g_slog_min_level && _g_slog_min_level <= LOG_LEVEL_VERBOSE) _CONSOLE_LOG(LOG_LEVEL_VERBOSE, "[V/%s] " fmt _LOG_SUFFIX, tag, ##__VA_ARGS__); } while (0)
#define _SLOGD_IMPL(tag, fmt, ...) do { if (_g_slog_min_level && _g_slog_min_level <= LOG_LEVEL_DEBUG) _CONSOLE_LOG(LOG_LEVEL_DEBUG, "[D/%s] " fmt _LOG_SUFFIX, tag, ##__VA_ARGS__); } while (0)
#define _SLOGI_IMPL(tag, fmt, ...) do { if (_g_slog_min_level && _g_slog_min_level <= LOG_LEVEL_INFO) _CONSOLE_LOG(LOG_LEVEL_INFO, "[I/%s] " fmt _LOG_SUFFIX, tag, ##__VA_ARGS__); } while (0)
#define _SLOGW_IMPL(tag, fmt, ...) do { if (_g_slog_min_level && _g_slog_min_level <= LOG_LEVEL_WARN) _CONSOLE_LOG(LOG_LEVEL_WARN, "[W/%s] " fmt _LOG_SUFFIX, tag, ##__VA_ARGS__); } while (0)
#define _SLOGE_IMPL(tag, fmt, ...) do { if (_g_slog_min_level && _g_slog_min_level <= LOG_LEVEL_ERROR) _CONSOLE_LOG(LOG_LEVEL_ERROR, "[E/%s] " fmt _LOG_SUFFIX, tag, ##__VA_ARGS__); } while (0)

#define _SLOGV_TRACE_IMPL(tag, func_name, line_num, fmt, ...) do { if (_g_slog_min_level && _g_slog_min_level <= LOG_LEVEL_VERBOSE) _CONSOLE_LOG(LOG_LEVEL_VERBOSE, "[V/%s](%s:%d) " fmt _LOG_SUFFIX, tag, func_name, line_num, ##__VA_ARGS__); } while (0)
#define _SLOGD_TRACE_IMPL(tag, func_name, line_num, fmt, ...) do { if (_g_slog_min_level && _g_slog_min_level <= LOG_LEVEL_DEBUG) _CONSOLE_LOG(LOG_LEVEL_DEBUG, "[D/%s](%s:%d) " fmt _LOG_SUFFIX, tag, func_name, line_num, ##__VA_ARGS__); } while (0)
#define _SLOGI_TRACE_IMPL(tag, func_name, line_num, fmt, ...) do { if (_g_slog_min_level && _g_slog_min_level <= LOG_LEVEL_INFO) _CONSOLE_LOG(LOG_LEVEL_INFO, "[I/%s](%s:%d) " fmt _LOG_SUFFIX, tag, func_name, line_num, ##__VA_ARGS__); } while (0)
#define _SLOGW_TRACE_IMPL(tag, func_name, line_num, fmt, ...) do { if (_g_slog_min_level && _g_slog_min_level <= LOG_LEVEL_WARN) _CONSOLE_LOG(LOG_LEVEL_WARN, "[W/%s](%s:%d) " fmt _LOG_SUFFIX, tag, func_name, line_num, ##__VA_ARGS__); } while (0)
#define _SLOGE_TRACE_IMPL(tag, func_name, line_num, fmt, ...) do { if (_g_slog_min_level && _g_slog_min_level <= LOG_LEVEL_ERROR) _CONSOLE_LOG(LOG_LEVEL_ERROR, "[E/%s](%s:%d) " fmt _LOG_SUFFIX, tag, func_name, line_num, ##__VA_ARGS__); } while (0)

#endif // __ANDROID__

#define SLOG_SET_MIN_LEVEL(min_level)  slog_set_min_level(min_level)
#define SLOG_GET_MIN_LEVEL()           slog_get_min_level()
#if(!defined(_LCU_LOGGER_UNSUPPORT_STDOUT_REDIRECT) || 0 == _LCU_LOGGER_UNSUPPORT_STDOUT_REDIRECT)
#define SLOG_STD2FILE(file_path)       slog_stdout2file(file_path)
#define SLOG_BACK2STD()                slog_back2stdout()   
#else
#define SLOG_STD2FILE(file_path)       do { (void)(file_path); } while (0)
#define SLOG_BACK2STD()                do { } while (0)
#endif // !_LCU_LOGGER_UNSUPPORT_STDOUT_REDIRECT

// better define LOG_TAG before include this slog.h
#ifndef LOG_TAG
#define LOG_TAG  "SLOG"
#endif // !LOG_TAG
#ifndef _LOG_TAG
#define _LOG_TAG LOG_TAG
#endif // !_LOG_TAG

#define SLOGV(tag, fmt, ...)          _SLOGV_IMPL(tag, fmt, ##__VA_ARGS__)
#define SLOGD(tag, fmt, ...)          _SLOGD_IMPL(tag, fmt, ##__VA_ARGS__)
#define SLOGI(tag, fmt, ...)          _SLOGI_IMPL(tag, fmt, ##__VA_ARGS__)
#define SLOGW(tag, fmt, ...)          _SLOGW_IMPL(tag, fmt, ##__VA_ARGS__)
#define SLOGE(tag, fmt, ...)          _SLOGE_IMPL(tag, fmt, ##__VA_ARGS__)

#define SLOGV_TRACE(tag, fmt, ...)    _SLOGV_TRACE_IMPL(tag, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define SLOGD_TRACE(tag, fmt, ...)    _SLOGD_TRACE_IMPL(tag, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define SLOGI_TRACE(tag, fmt, ...)    _SLOGI_TRACE_IMPL(tag, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define SLOGW_TRACE(tag, fmt, ...)    _SLOGW_TRACE_IMPL(tag, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define SLOGE_TRACE(tag, fmt, ...)    _SLOGE_TRACE_IMPL(tag, __func__, __LINE__, fmt, ##__VA_ARGS__)

#define SLOGV_HEX(tag, chars, chars_count) do { if (_g_slog_min_level && _g_slog_min_level <= LOG_LEVEL_VERBOSE) __slog_internal_hex_print(LOG_LEVEL_VERBOSE, (const char *)tag, (const char *)chars, chars_count); } while(0) 
#define SLOGD_HEX(tag, chars, chars_count) do { if (_g_slog_min_level && _g_slog_min_level <= LOG_LEVEL_DEBUG) __slog_internal_hex_print(LOG_LEVEL_DEBUG, (const char *)tag, (const char *)chars, chars_count); } while(0) 
#define SLOGI_HEX(tag, chars, chars_count) do { if (_g_slog_min_level && _g_slog_min_level <= LOG_LEVEL_INFO) __slog_internal_hex_print(LOG_LEVEL_INFO, (const char *)tag, (const char *)chars, chars_count); } while(0) 
#define SLOGW_HEX(tag, chars, chars_count) do { if (_g_slog_min_level && _g_slog_min_level <= LOG_LEVEL_WARN) __slog_internal_hex_print(LOG_LEVEL_WARN, (const char *)tag, (const char *)chars, chars_count); } while(0) 
#define SLOGE_HEX(tag, chars, chars_count) do { if (_g_slog_min_level && _g_slog_min_level <= LOG_LEVEL_ERROR) __slog_internal_hex_print(LOG_LEVEL_ERROR, (const char *)tag, (const char *)chars, chars_count); } while(0) 

//===================================================================================================

#define LOGV(fmt, ...) SLOGV(_LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGD(fmt, ...) SLOGD(_LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGI(fmt, ...) SLOGI(_LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) SLOGW(_LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...) SLOGE(_LOG_TAG, fmt, ##__VA_ARGS__)

#define LOGV_TRACE(fmt, ...) SLOGV_TRACE(_LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGD_TRACE(fmt, ...) SLOGD_TRACE(_LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGI_TRACE(fmt, ...) SLOGI_TRACE(_LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGW_TRACE(fmt, ...) SLOGW_TRACE(_LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGE_TRACE(fmt, ...) SLOGE_TRACE(_LOG_TAG, fmt, ##__VA_ARGS__)

#define LOGV_HEX(chars, chars_count) SLOGV_HEX(_LOG_TAG, chars, chars_count)
#define LOGD_HEX(chars, chars_count) SLOGD_HEX(_LOG_TAG, chars, chars_count)
#define LOGI_HEX(chars, chars_count) SLOGI_HEX(_LOG_TAG, chars, chars_count)
#define LOGW_HEX(chars, chars_count) SLOGW_HEX(_LOG_TAG, chars, chars_count)
#define LOGE_HEX(chars, chars_count) SLOGE_HEX(_LOG_TAG, chars, chars_count)

#define TLOGV(tag, fmt, ...)  SLOGV(tag, fmt, ##__VA_ARGS__)
#define TLOGD(tag, fmt, ...)  SLOGD(tag, fmt, ##__VA_ARGS__)
#define TLOGI(tag, fmt, ...)  SLOGI(tag, fmt, ##__VA_ARGS__)
#define TLOGW(tag, fmt, ...)  SLOGW(tag, fmt, ##__VA_ARGS__)
#define TLOGE(tag, fmt, ...)  SLOGE(tag, fmt, ##__VA_ARGS__)

#define TLOGV_TRACE(tag, fmt, ...) 	  SLOGV_TRACE(tag, fmt, ##__VA_ARGS__)
#define TLOGD_TRACE(tag, fmt, ...)	  SLOGD_TRACE(tag, fmt, ##__VA_ARGS__)
#define TLOGI_TRACE(tag, fmt, ...)	  SLOGI_TRACE(tag, fmt, ##__VA_ARGS__)
#define TLOGW_TRACE(tag, fmt, ...)	  SLOGW_TRACE(tag, fmt, ##__VA_ARGS__)
#define TLOGE_TRACE(tag, fmt, ...)	  SLOGE_TRACE(tag, fmt, ##__VA_ARGS__)

#define TLOGV_HEX(tag, chars, chars_count) SLOGV_HEX(tag, chars, chars_count)
#define TLOGD_HEX(tag, chars, chars_count) SLOGD_HEX(tag, chars, chars_count)
#define TLOGI_HEX(tag, chars, chars_count) SLOGI_HEX(tag, chars, chars_count)
#define TLOGW_HEX(tag, chars, chars_count) SLOGW_HEX(tag, chars, chars_count)
#define TLOGE_HEX(tag, chars, chars_count) SLOGE_HEX(tag, chars, chars_count)

//===================================================================================================

#endif // !LCU_SLOG_H
