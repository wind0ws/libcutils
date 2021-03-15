#pragma once
#ifndef LCU_XLOG_H
#define LCU_XLOG_H

#include <stddef.h>  /* for size_t */

#ifdef _WIN32
//sigh: wish some day windows support __func__ AND __PRETTY_FUNCTION__
#ifndef __func__
#define __func__ __FUNCTION__
#endif // !__func__
#ifndef __PRETTY_FUNCTION__
#define __PRETTY_FUNCTION__ __FUNCSIG__ 
#endif // !__PRETTY_FUNCTION__
#elif(defined(__ANDROID__))
#include <android/log.h>
#endif // _WIN32

#define XLOG_STAR_LINE "****************************************************************"

typedef enum
{
	LOG_LEVEL_OFF = 0,
	LOG_LEVEL_VERBOSE = 1,
	LOG_LEVEL_DEBUG = 2,
	LOG_LEVEL_INFO = 3,
	LOG_LEVEL_WARN = 4,
	LOG_LEVEL_ERROR = 5,
	LOG_LEVEL_UNKNOWN
} LogLevel;

typedef void (*xlog_user_callback_fn)(void* log_msg, void* user_data);

typedef enum
{
	LOG_TARGET_NONE = 0,          // NOLINT(hicpp-signed-bitwise)
	LOG_TARGET_ANDROID = (0x1 << 1), // NOLINT(hicpp-signed-bitwise)
	LOG_TARGET_CONSOLE = (0x1 << 2), // NOLINT(hicpp-signed-bitwise)
	LOG_TARGET_USER_CALLBACK = (0x1 << 3)  // NOLINT(hicpp-signed-bitwise)
} LogTarget;

typedef enum
{
	/* only output pure log message */
	LOG_FORMAT_RAW = 0,
	/* output log message with timestamp */
	LOG_FORMAT_WITH_TIMESTAMP = (0x1 << 1),
	/* output log message with tag and level */
	LOG_FORMAT_WITH_TAG_LEVEL = (0x1 << 2),
	/* output log message with thread id */
	LOG_FORMAT_WITH_TID = (0x1 << 3)
} LogFormat;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

	/**
	 * auto increase log level if current log level below trigger_level
	 * this is useful when some Android devices have restrictions on the log level below LOG_LEVEL_INFO.
	 */
	void xlog_auto_level_up(LogLevel trigger_level);

	/**
	 * redirect stdout from console window to file.
	 */
	void xlog_stdout2file(char* file_path);

	/**
	 * let stdout print on console window.
	 */
	void xlog_back2stdout();

	/**
	 * set default log tag. use this tag if you not passed TAG.
	 * note: tag length should below 31.
	 */
	void xlog_set_default_tag(char* tag);

	/**
	 * timezone_hour used by generate log time.
	 * default timezone_hour is 8.
	 * example: In china, we are in +8 timezone area, so here set it to 8.
	 */
	void xlog_set_timezone(int timezone_hour);

	/**
	 * set user callback. when log performed, callback will called.
	 * you can do your own log logic on callback.
	 * note: log target should include LOG_TARGET_USER_CALLBACK, otherwise callback won't trigged
	 */
	void xlog_set_user_callback(xlog_user_callback_fn user_cb, void* user_data);

	/**
	* set the min log level. only output log if current level greater or equal to this min_level
	*/
	void xlog_set_min_level(LogLevel min_level);

	LogLevel xlog_get_min_level();

	/**
	 * set log target which you want to output.
	 * default: on Android: target is LOG_TARGET_ANDROID, otherwise target is LOG_TARGET_CONSOLE
	 */
	void xlog_set_target(int target);

	int xlog_get_target();

	/**
	 * set log format.
	 * default format is (LOG_FORMAT_WITH_TIMESTAMP | LOG_FORMAT_WITH_TAG_LEVEL)
	 */
	void xlog_set_format(int format);

	int xlog_get_format();

	/**
	 * transform char to hex.
	 * note: just transform for you, not print for you.
	 * @param out_hex_str: place hex result
	 * @param out_hex_str_capacity: the length of out_hex_str you provide
	 * @param chars: the chars you want to transform to hex
	 * @param chars_len: the length of chars
	 */
	void xlog_chars2hex(char* out_hex_str, size_t out_hex_str_capacity, const char* chars, size_t chars_len);

	/**
	 * DO NOT call this method directly.(for xlog internal use only)
	 * USE LOGX or TLOGX macro instead.
	 */
	void __xlog_internal_log(LogLevel level, char* tag, const char* func_name, int file_line, char* fmt, ...);

	/**
	 * DO NOT call this method directly.(for xlog internal use only)
	 * USE TLOGX_HEX or LOGX_HEX macro instead.
	 */
	void __xlog_hex_helper(LogLevel level, char* tag, char* chars, size_t chars_len);

#ifdef __cplusplus
}
#endif

#define LOGV(fmt, ...) __xlog_internal_log(LOG_LEVEL_VERBOSE, NULL, NULL, 0, fmt, ##__VA_ARGS__)
#define LOGD(fmt, ...) __xlog_internal_log(LOG_LEVEL_DEBUG, NULL, NULL, 0, fmt, ##__VA_ARGS__)
#define LOGI(fmt, ...) __xlog_internal_log(LOG_LEVEL_INFO, NULL, NULL, 0, fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) __xlog_internal_log(LOG_LEVEL_WARN, NULL, NULL, 0, fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...) __xlog_internal_log(LOG_LEVEL_ERROR, NULL, NULL, 0, fmt, ##__VA_ARGS__)

#define LOGV_TRACE(fmt, ...) __xlog_internal_log(LOG_LEVEL_VERBOSE, NULL, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define LOGD_TRACE(fmt, ...) __xlog_internal_log(LOG_LEVEL_DEBUG, NULL,__func__, __LINE__, fmt, ##__VA_ARGS__)
#define LOGI_TRACE(fmt, ...) __xlog_internal_log(LOG_LEVEL_INFO, NULL, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define LOGW_TRACE(fmt, ...) __xlog_internal_log(LOG_LEVEL_WARN, NULL, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define LOGE_TRACE(fmt, ...) __xlog_internal_log(LOG_LEVEL_ERROR, NULL, __func__, __LINE__, fmt, ##__VA_ARGS__)

#define TLOGV(tag, fmt, ...) __xlog_internal_log(LOG_LEVEL_VERBOSE, tag, NULL, 0, fmt, ##__VA_ARGS__)
#define TLOGD(tag, fmt, ...) __xlog_internal_log(LOG_LEVEL_DEBUG, tag, NULL, 0, fmt, ##__VA_ARGS__)
#define TLOGI(tag, fmt, ...) __xlog_internal_log(LOG_LEVEL_INFO, tag, NULL, 0, fmt, ##__VA_ARGS__)
#define TLOGW(tag, fmt, ...) __xlog_internal_log(LOG_LEVEL_WARN, tag, NULL, 0, fmt, ##__VA_ARGS__)
#define TLOGE(tag, fmt, ...) __xlog_internal_log(LOG_LEVEL_ERROR, tag, NULL, 0, fmt, ##__VA_ARGS__)

#define TLOGV_TRACE(tag, fmt, ...) __xlog_internal_log(LOG_LEVEL_VERBOSE, tag, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define TLOGD_TRACE(tag, fmt, ...) __xlog_internal_log(LOG_LEVEL_DEBUG, tag, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define TLOGI_TRACE(tag, fmt, ...) __xlog_internal_log(LOG_LEVEL_INFO, tag, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define TLOGW_TRACE(tag, fmt, ...) __xlog_internal_log(LOG_LEVEL_WARN, tag, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define TLOGE_TRACE(tag, fmt, ...) __xlog_internal_log(LOG_LEVEL_ERROR, tag, __func__, __LINE__, fmt, ##__VA_ARGS__)

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

#endif // LCU_XLOG_H
