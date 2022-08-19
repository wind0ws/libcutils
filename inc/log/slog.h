#pragma once
#ifndef LCU_SLOG_H
#define LCU_SLOG_H

#include "log/logger_data.h" /* for common data */
#include <stdio.h>           /* for printf      */
#include <string.h>          /* for strlen      */

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

	void __slog_internal_hex_print(int level, char* tag, char* chars, size_t chars_count);
	
#ifdef __cplusplus
}
#endif // __cplusplus

#ifdef __ANDROID__

#include <android/log.h>
#define _SLOGV_IMPL(tag, fmt, ...)    do { if (_g_slog_min_level && _g_slog_min_level <=  LOG_LEVEL_VERBOSE) __android_log_print(ANDROID_LOG_VERBOSE, tag, fmt, ##__VA_ARGS__); } while (0)
#define _SLOGD_IMPL(tag, fmt, ...)    do { if (_g_slog_min_level && _g_slog_min_level <=  LOG_LEVEL_DEBUG) __android_log_print(ANDROID_LOG_DEBUG,   tag, fmt, ##__VA_ARGS__); } while (0)
#define _SLOGI_IMPL(tag, fmt, ...)    do { if (_g_slog_min_level && _g_slog_min_level <=  LOG_LEVEL_INFO) __android_log_print(ANDROID_LOG_INFO,    tag, fmt, ##__VA_ARGS__); } while (0)
#define _SLOGW_IMPL(tag, fmt, ...)    do { if (_g_slog_min_level && _g_slog_min_level <=  LOG_LEVEL_WARN) __android_log_print(ANDROID_LOG_WARN,    tag, fmt, ##__VA_ARGS__); } while (0)
#define _SLOGE_IMPL(tag, fmt, ...)    do { if (_g_slog_min_level && _g_slog_min_level <=  LOG_LEVEL_ERROR) __android_log_print(ANDROID_LOG_ERROR,   tag, fmt, ##__VA_ARGS__); } while (0)

#define _SLOGV_TRACE_IMPL(tag, func_name, line_num, fmt, ...)  do { if (_g_slog_min_level && _g_slog_min_level <=  LOG_LEVEL_VERBOSE) __android_log_print(ANDROID_LOG_VERBOSE, tag, "(%s:%d) " fmt, func_name, line_num, ##__VA_ARGS__); } while (0)
#define _SLOGD_TRACE_IMPL(tag, func_name, line_num, fmt, ...)  do { if (_g_slog_min_level && _g_slog_min_level <=  LOG_LEVEL_DEBUG) __android_log_print(ANDROID_LOG_DEBUG,   tag, "(%s:%d) " fmt, func_name, line_num, ##__VA_ARGS__); } while (0)
#define _SLOGI_TRACE_IMPL(tag, func_name, line_num, fmt, ...)  do { if (_g_slog_min_level && _g_slog_min_level <=  LOG_LEVEL_INFO) __android_log_print(ANDROID_LOG_INFO,    tag, "(%s:%d) " fmt, func_name, line_num, ##__VA_ARGS__); } while (0)
#define _SLOGW_TRACE_IMPL(tag, func_name, line_num, fmt, ...)  do { if (_g_slog_min_level && _g_slog_min_level <=  LOG_LEVEL_WARN) __android_log_print(ANDROID_LOG_WARN,    tag, "(%s:%d) " fmt, func_name, line_num, ##__VA_ARGS__); } while (0)
#define _SLOGE_TRACE_IMPL(tag, func_name, line_num, fmt, ...)  do { if (_g_slog_min_level && _g_slog_min_level <=  LOG_LEVEL_ERROR) __android_log_print(ANDROID_LOG_ERROR,   tag, "(%s:%d) " fmt, func_name, line_num, ##__VA_ARGS__); } while (0)

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

#define _SLOGV_IMPL(tag, fmt, ...) do { if (_g_slog_min_level && _g_slog_min_level <= LOG_LEVEL_VERBOSE) _CONSOLE_LOG(LOG_LEVEL_VERBOSE, "[V/%s] " fmt _SUFFIX_LOG, tag, ##__VA_ARGS__); } while (0)
#define _SLOGD_IMPL(tag, fmt, ...) do { if (_g_slog_min_level && _g_slog_min_level <= LOG_LEVEL_DEBUG) _CONSOLE_LOG(LOG_LEVEL_DEBUG, "[D/%s] " fmt _SUFFIX_LOG, tag, ##__VA_ARGS__); } while (0)
#define _SLOGI_IMPL(tag, fmt, ...) do { if (_g_slog_min_level && _g_slog_min_level <= LOG_LEVEL_INFO) _CONSOLE_LOG(LOG_LEVEL_INFO, "[I/%s] " fmt _SUFFIX_LOG, tag, ##__VA_ARGS__); } while (0)
#define _SLOGW_IMPL(tag, fmt, ...) do { if (_g_slog_min_level && _g_slog_min_level <= LOG_LEVEL_WARN) _CONSOLE_LOG(LOG_LEVEL_WARN, "[W/%s] " fmt _SUFFIX_LOG, tag, ##__VA_ARGS__); } while (0)
#define _SLOGE_IMPL(tag, fmt, ...) do { if (_g_slog_min_level && _g_slog_min_level <= LOG_LEVEL_ERROR) _CONSOLE_LOG(LOG_LEVEL_ERROR, "[E/%s] " fmt _SUFFIX_LOG, tag, ##__VA_ARGS__); } while (0)

#define _SLOGV_TRACE_IMPL(tag, func_name, line_num, fmt, ...) do { if (_g_slog_min_level && _g_slog_min_level <= LOG_LEVEL_VERBOSE) _CONSOLE_LOG(LOG_LEVEL_VERBOSE, "[V/%s](%s:%d) " fmt _SUFFIX_LOG, tag, func_name, line_num, ##__VA_ARGS__); } while (0)
#define _SLOGD_TRACE_IMPL(tag, func_name, line_num, fmt, ...) do { if (_g_slog_min_level && _g_slog_min_level <= LOG_LEVEL_DEBUG) _CONSOLE_LOG(LOG_LEVEL_DEBUG, "[D/%s](%s:%d) " fmt _SUFFIX_LOG, tag, func_name, line_num, ##__VA_ARGS__); } while (0)
#define _SLOGI_TRACE_IMPL(tag, func_name, line_num, fmt, ...) do { if (_g_slog_min_level && _g_slog_min_level <= LOG_LEVEL_INFO) _CONSOLE_LOG(LOG_LEVEL_INFO, "[I/%s](%s:%d) " fmt _SUFFIX_LOG, tag, func_name, line_num, ##__VA_ARGS__); } while (0)
#define _SLOGW_TRACE_IMPL(tag, func_name, line_num, fmt, ...) do { if (_g_slog_min_level && _g_slog_min_level <= LOG_LEVEL_WARN) _CONSOLE_LOG(LOG_LEVEL_WARN, "[W/%s](%s:%d) " fmt _SUFFIX_LOG, tag, func_name, line_num, ##__VA_ARGS__); } while (0)
#define _SLOGE_TRACE_IMPL(tag, func_name, line_num, fmt, ...) do { if (_g_slog_min_level && _g_slog_min_level <= LOG_LEVEL_ERROR) _CONSOLE_LOG(LOG_LEVEL_ERROR, "[E/%s](%s:%d) " fmt _SUFFIX_LOG, tag, func_name, line_num, ##__VA_ARGS__); } while (0)

#endif // __ANDROID__

#define SLOG_SET_MIN_LEVEL(min_level)  do { slog_set_min_level(min_level); } while(0)
#define SLOG_GET_MIN_LEVEL()           slog_get_min_level()
#if(!defined(_LCU_LOGGER_UNSUPPORT_STDOUT_REDIRECT) || 0 == _LCU_LOGGER_UNSUPPORT_STDOUT_REDIRECT)
#define SLOG_STD2FILE(file_path)       do { slog_stdout2file(file_path); } while(0)
#define SLOG_BACK2STD()                do { slog_back2stdout(); } while(0)   
#else
#define SLOG_STD2FILE(file_path)       do { (void)(file_path); } while (0)
#define SLOG_BACK2STD()                do {} while (0)
#endif // !_LCU_LOGGER_UNSUPPORT_STDOUT_REDIRECT

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

#define SLOGV_HEX(tag, chars, chars_count) do { if (_g_slog_min_level && _g_slog_min_level <=  LOG_LEVEL_VERBOSE) __slog_internal_hex_print(LOG_LEVEL_VERBOSE, tag, chars, chars_count); } while(0) 
#define SLOGD_HEX(tag, chars, chars_count) do { if (_g_slog_min_level && _g_slog_min_level <=  LOG_LEVEL_DEBUG) __slog_internal_hex_print(LOG_LEVEL_DEBUG, tag, chars, chars_count); } while(0) 
#define SLOGI_HEX(tag, chars, chars_count) do { if (_g_slog_min_level && _g_slog_min_level <=  LOG_LEVEL_INFO) __slog_internal_hex_print(LOG_LEVEL_INFO, tag, chars, chars_count); } while(0) 
#define SLOGW_HEX(tag, chars, chars_count) do { if (_g_slog_min_level && _g_slog_min_level <=  LOG_LEVEL_WARN) __slog_internal_hex_print(LOG_LEVEL_WARN, tag, chars, chars_count); } while(0) 
#define SLOGE_HEX(tag, chars, chars_count) do { if (_g_slog_min_level && _g_slog_min_level <=  LOG_LEVEL_ERROR) __slog_internal_hex_print(LOG_LEVEL_ERROR, tag, chars, chars_count); } while(0) 


#endif // !LCU_SLOG_H
