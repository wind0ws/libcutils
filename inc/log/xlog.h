#pragma once
#ifndef __XLOG_H
#define __XLOG_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#ifdef _WIN32
#define __func__ __FUNCTION__
#elif(defined(__ANDROID__))
#include <android/log.h>
#endif // __ANDROID__

#define LOG_LINE_STAR "****************************************************************"
#define CONSOLE_LOG_CONFIG_METHOD printf
#define CONSOLE_LOG_CONFIG_NEW_LINE_FORMAT "\r\n"

	typedef enum {
		LOG_LEVEL_OFF = 0,
		LOG_LEVEL_VERBOSE = 1,
		LOG_LEVEL_DEBUG = 2,
		LOG_LEVEL_INFO = 3,
		LOG_LEVEL_WARN = 4,
		LOG_LEVEL_ERROR = 5,
		LOG_LEVEL_UNKNOWN
	} LogLevel;

	typedef void (*xlog_user_callback_fn)(void* log_msg, void* user_data);

	typedef enum {
		LOG_TARGET_NONE = 0,
		LOG_TARGET_ANDROID = (0x1 << 1), // NOLINT(hicpp-signed-bitwise)
		LOG_TARGET_CONSOLE = (0x1 << 2), // NOLINT(hicpp-signed-bitwise)
		LOG_TARGET_USER_CALLBACK = (0x1 << 3)  // NOLINT(hicpp-signed-bitwise)
	} LogTarget;

	/**
	 * auto increase log level if current log level below trigger_level
	 * this is useful when some Android devices have restrictions on the log level below LOG_LEVEL_INFO.
	 */
	void xlog_auto_level_up(LogLevel trigger_level);

	/**
	 * redirect log output from stdout to file.
	 */
	void xlog_stdout2file(char* file_path);

	/**
	 * log output set to stdout.
	 */
	void xlog_back2stdout();

	/**
	 * set default log tag. use it on LOGX macro.
	 */
	void xlog_set_default_tag(char* tag);

	/**
	 * set user callback when log called.
	 * note: log target should include LOG_TARGET_USER_CALLBACK
	 */
	void xlog_set_user_callback(xlog_user_callback_fn user_cb, void* user_data);

	/**
	 * set log target which you want to output.
	 */
	void xlog_set_target(LogTarget target);

	LogTarget xlog_get_target();

	/**
	 * set the min log level. only log if current level greater than or equal to this min_level
	 */
	void xlog_set_min_level(LogLevel min_level);

	LogLevel xlog_get_min_level();

	/**
	 * transform char to hex.
	 * @param str: place hex result
	 * @param str_capacity: the length of str you provide.
	 */
	void xlog_chars2hex(char* str, size_t str_capacity, const char* chars, size_t chars_len);

	/**
	 * DO NOT call this method directly.(for xlog internal use only)
	 * USE LOGX or TLOGX macro instead.
	 */
	void __xlog_internal_log(LogLevel level, char* tag, const char* func_name, int file_line, char* fmt, ...);

	/**
	 * DO NOT call this method directly.(for xlog internal use only)
	 * USE TLOGX_HEX or LOGX_HEX macro instead.
	 */
	void  __xlog_hex_helper(LogLevel level, char* tag, char* chars, size_t chars_len);

#define LOGV(fmt, ...) __xlog_internal_log(LOG_LEVEL_VERBOSE, NULL, NULL, 0, fmt, ##__VA_ARGS__);
#define LOGD(fmt, ...) __xlog_internal_log(LOG_LEVEL_DEBUG, NULL, NULL, 0, fmt, ##__VA_ARGS__);
#define LOGI(fmt, ...) __xlog_internal_log(LOG_LEVEL_INFO, NULL, NULL, 0, fmt, ##__VA_ARGS__);
#define LOGW(fmt, ...) __xlog_internal_log(LOG_LEVEL_WARN, NULL, NULL, 0, fmt, ##__VA_ARGS__);
#define LOGE(fmt, ...) __xlog_internal_log(LOG_LEVEL_ERROR, NULL, NULL, 0, fmt, ##__VA_ARGS__);

#define LOGV_TRACE(fmt, ...) __xlog_internal_log(LOG_LEVEL_VERBOSE, NULL, __func__, __LINE__, fmt, ##__VA_ARGS__);
#define LOGD_TRACE(fmt, ...) __xlog_internal_log(LOG_LEVEL_DEBUG, NULL,__func__, __LINE__, fmt, ##__VA_ARGS__);
#define LOGI_TRACE(fmt, ...) __xlog_internal_log(LOG_LEVEL_INFO, NULL, __func__, __LINE__, fmt, ##__VA_ARGS__);
#define LOGW_TRACE(fmt, ...) __xlog_internal_log(LOG_LEVEL_WARN, NULL, __func__, __LINE__, fmt, ##__VA_ARGS__);
#define LOGE_TRACE(fmt, ...) __xlog_internal_log(LOG_LEVEL_ERROR, NULL, __func__, __LINE__, fmt, ##__VA_ARGS__);

#define TLOGV(tag, fmt, ...) __xlog_internal_log(LOG_LEVEL_VERBOSE, tag, NULL, 0, fmt, ##__VA_ARGS__);
#define TLOGD(tag, fmt, ...) __xlog_internal_log(LOG_LEVEL_DEBUG, tag, NULL, 0, fmt, ##__VA_ARGS__);
#define TLOGI(tag, fmt, ...) __xlog_internal_log(LOG_LEVEL_INFO, tag, NULL, 0, fmt, ##__VA_ARGS__);
#define TLOGW(tag, fmt, ...) __xlog_internal_log(LOG_LEVEL_WARN, tag, NULL, 0, fmt, ##__VA_ARGS__);
#define TLOGE(tag, fmt, ...) __xlog_internal_log(LOG_LEVEL_ERROR, tag, NULL, 0, fmt, ##__VA_ARGS__);

#define TLOGV_TRACE(tag, fmt, ...) __xlog_internal_log(LOG_LEVEL_VERBOSE, tag, __func__, __LINE__, fmt, ##__VA_ARGS__);
#define TLOGD_TRACE(tag, fmt, ...) __xlog_internal_log(LOG_LEVEL_DEBUG, tag, __func__, __LINE__, fmt, ##__VA_ARGS__);
#define TLOGI_TRACE(tag, fmt, ...) __xlog_internal_log(LOG_LEVEL_INFO, tag, __func__, __LINE__, fmt, ##__VA_ARGS__);
#define TLOGW_TRACE(tag, fmt, ...) __xlog_internal_log(LOG_LEVEL_WARN, tag, __func__, __LINE__, fmt, ##__VA_ARGS__);
#define TLOGE_TRACE(tag, fmt, ...) __xlog_internal_log(LOG_LEVEL_ERROR, tag, __func__, __LINE__, fmt, ##__VA_ARGS__);

#define TLOGV_HEX(tag, chars, chars_len) __xlog_hex_helper(LOG_LEVEL_VERBOSE, tag, chars, chars_len)
#define TLOGD_HEX(tag, chars, chars_len) __xlog_hex_helper(LOG_LEVEL_DEBUG, tag, chars, chars_len)
#define TLOGI_HEX(tag, chars, chars_len) __xlog_hex_helper(LOG_LEVEL_INFO, tag, chars, chars_len)
#define TLOGW_HEX(tag, chars, chars_len) __xlog_hex_helper(LOG_LEVEL_WARN, tag, chars, chars_len)
#define TLOGE_HEX(tag, chars, chars_len) __xlog_hex_helper(LOG_LEVEL_ERROR, tag, chars, chars_len)

#define LOGV_HEX(chars, chars_len) TLOGV_HEX(NULL, chars, chars_len)
#define LOGD_HEX(chars, chars_len) TLOGD_HEX(NULL, chars, chars_len)
#define LOGI_HEX(chars, chars_len) TLOGI_HEX(NULL, chars, chars_len)
#define LOGW_HEX(chars, chars_len) TLOGW_HEX(NULL, chars, chars_len)
#define LOGE_HEX(chars, chars_len) TLOGE_HEX(NULL, chars, chars_len)

#ifdef __cplusplus
}
#endif

#endif //__XLOG_H