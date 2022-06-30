#pragma once
#ifndef LCU_XLOG_H
#define LCU_XLOG_H

#include "log/logger_data.h" /* for common data */
#include <stddef.h>          /* for size_t      */

//user log callback prototype. log_msg string not end with '\n' automatically. msg_size contains NUL terminator
typedef void (*xlog_user_callback_fn)(LogLevel level, void* log_msg, size_t msg_size, void* user_data);

typedef enum
{
	LOG_TARGET_NONE = 0,
	LOG_TARGET_ANDROID = (0x1 << 1), 
	LOG_TARGET_CONSOLE = (0x1 << 2),
	LOG_TARGET_USER_CALLBACK = (0x1 << 3),
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

typedef enum
{
	LOG_FLUSH_MODE_AUTO = 0,
	LOG_FLUSH_MODE_EVERY
} LogFlushMode;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#if(!defined(_LCU_LOGGER_UNSUPPORT_PRINTF_REDIRECT) || 0 == _LCU_LOGGER_UNSUPPORT_PRINTF_REDIRECT)
	/**
	 * redirect stdout from console to file.
	 * note: NOT THREAD-SAFE. call it on begin of your program.
	 */
	void xlog_stdout2file(char* file_path);

	/**
	 * bring stdout print on console if current print to file.
	 * note: NOT THREAD-SAFE. call it on end of your program.
	 */
	void xlog_back2stdout();
#endif // !_LCU_LOGGER_UNSUPPORT_PRINTF_REDIRECT

	/**
	 * auto increase low level to trigger_level.
	 * if log level bigger than min_level but small than this trigger_level, transform log level to this.
	 * this is useful when some Android devices have restrictions on the log level below LOG_LEVEL_INFO.
	 */
	void xlog_auto_level_up(LogLevel trigger_level);

	/**
	 * set default log tag. use this tag if you not provide, such as call LOGX.
	 * note: tag length should below 24.
	 */
	void xlog_set_default_tag(char* tag);

	/**
	 * timezone_hour used by generate your local log time.
	 * timezone_hour should between -12 ~ 12. default timezone_hour is 8. 
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
	* set the min log level. 
	* only output log if current level greater or equal to this min_level
	*/
	void xlog_set_min_level(LogLevel min_level);

	/**
	 * get current min log level.
	 * default is LOG_LEVEL_VERBOSE
	 */
	LogLevel xlog_get_min_level();

	/**
	 * set log target which you want to output.
	 * default: on Android target is LOG_TARGET_ANDROID, other platform target is LOG_TARGET_CONSOLE
	 * multiple can be combined. 
	 * example: (LOG_TARGET_ANDROID | LOG_TARGET_CONSOLE) or (LOG_TARGET_CONSOLE | LOG_TARGET_USER_CALLBACK)
	 */
	void xlog_set_target(int target);

	/**
	 * get current target
	 */
	int xlog_get_target();

	/**
	 * set log header format.
	 * default format is (LOG_FORMAT_WITH_TIMESTAMP | LOG_FORMAT_WITH_TAG_LEVEL)
	 * android logcat won't use this header format, be aware of that.
	 */
	void xlog_set_format(int format);

	/**
	 * get current format
	 */
	int xlog_get_format();

	/**
	 * set flush mode on xlog.
	 * default mode is LOG_FLUSH_MODE_AUTO: auto flush by system,
	 *    another mode LOG_FLUSH_MODE_EVERY: flush on every log print
	 * this won't effect on android logcat
	 */
	void xlog_set_flush_mode(LogFlushMode flush_mode);

	/**
	 * get current flush mode
	 */
	LogFlushMode xlog_get_flush_mode();

	/**
	 * DO NOT call this method directly.(for xlog internal use only)
	 * USE LOGX or TLOGX macro instead.
	 */
	void __xlog_internal_print(LogLevel level, char* tag, const char* func_name, int file_line, char* fmt, ...);

	/**
	 * DO NOT call this method directly.(for xlog internal use only)
	 * USE LOGX_HEX or TLOGX_HEX macro instead.
	 */
	void __xlog_internal_hex_print(LogLevel level, char* tag, char* chars, size_t chars_count);

#ifdef __cplusplus
}
#endif // __cplusplus

// for remove all of xlog call
//#defined LCU_XLOG_OFF 1
#if(defined(LCU_XLOG_OFF) && LCU_XLOG_OFF)

#ifndef _LOG_NOTHING
#define _LOG_NOTHING()       do { } while (0)
#endif // !_LOG_NOTHING

#define LOG_STD2FILE(file_path)  do { (void)(file_path); } while (0)
#define LOG_BACK2STD()           _LOG_NOTHING

#define TLOGV(tag, fmt, ...) _LOG_NOTHING()
#define TLOGD(tag, fmt, ...) _LOG_NOTHING()
#define TLOGI(tag, fmt, ...) _LOG_NOTHING()
#define TLOGW(tag, fmt, ...) _LOG_NOTHING()
#define TLOGE(tag, fmt, ...) _LOG_NOTHING()

#define LOGV(fmt, ...) _LOG_NOTHING()
#define LOGD(fmt, ...) _LOG_NOTHING()
#define LOGI(fmt, ...) _LOG_NOTHING()
#define LOGW(fmt, ...) _LOG_NOTHING()
#define LOGE(fmt, ...) _LOG_NOTHING()

#define TLOGV_TRACE(tag, fmt, ...) _LOG_NOTHING()
#define TLOGD_TRACE(tag, fmt, ...) _LOG_NOTHING()
#define TLOGI_TRACE(tag, fmt, ...) _LOG_NOTHING()
#define TLOGW_TRACE(tag, fmt, ...) _LOG_NOTHING()
#define TLOGE_TRACE(tag, fmt, ...) _LOG_NOTHING()

#define LOGV_TRACE(fmt, ...) _LOG_NOTHING() 
#define LOGD_TRACE(fmt, ...) _LOG_NOTHING()
#define LOGI_TRACE(fmt, ...) _LOG_NOTHING()
#define LOGW_TRACE(fmt, ...) _LOG_NOTHING()
#define LOGE_TRACE(fmt, ...) _LOG_NOTHING()

#define TLOGV_HEX(tag, chars, chars_count) _LOG_NOTHING()
#define TLOGD_HEX(tag, chars, chars_count) _LOG_NOTHING()
#define TLOGI_HEX(tag, chars, chars_count) _LOG_NOTHING()
#define TLOGW_HEX(tag, chars, chars_count) _LOG_NOTHING()
#define TLOGE_HEX(tag, chars, chars_count) _LOG_NOTHING()

#define LOGV_HEX(chars, chars_count) _LOG_NOTHING()
#define LOGD_HEX(chars, chars_count) _LOG_NOTHING()
#define LOGI_HEX(chars, chars_count) _LOG_NOTHING()
#define LOGW_HEX(chars, chars_count) _LOG_NOTHING()
#define LOGE_HEX(chars, chars_count) _LOG_NOTHING()

#else

#if(!defined(_LCU_LOGGER_UNSUPPORT_PRINTF_REDIRECT) || 0 == _LCU_LOGGER_UNSUPPORT_PRINTF_REDIRECT)
#define LOG_STD2FILE(file_path)       do { xlog_stdout2file(file_path); } while(0)
#define LOG_BACK2STD()                do { xlog_back2stdout(); } while(0)   
#else
#define LOG_STD2FILE(file_path)       do { (void)(file_path); } while (0)
#define LOG_BACK2STD()                do {} while (0)
#endif // !_LCU_LOGGER_UNSUPPORT_PRINTF_REDIRECT

#define TLOGV(tag, fmt, ...) __xlog_internal_print(LOG_LEVEL_VERBOSE, tag, NULL, 0, fmt, ##__VA_ARGS__)
#define TLOGD(tag, fmt, ...) __xlog_internal_print(LOG_LEVEL_DEBUG, tag, NULL, 0, fmt, ##__VA_ARGS__)
#define TLOGI(tag, fmt, ...) __xlog_internal_print(LOG_LEVEL_INFO, tag, NULL, 0, fmt, ##__VA_ARGS__)
#define TLOGW(tag, fmt, ...) __xlog_internal_print(LOG_LEVEL_WARN, tag, NULL, 0, fmt, ##__VA_ARGS__)
#define TLOGE(tag, fmt, ...) __xlog_internal_print(LOG_LEVEL_ERROR, tag, NULL, 0, fmt, ##__VA_ARGS__)

#define LOGV(fmt, ...) TLOGV(NULL, fmt, ##__VA_ARGS__)
#define LOGD(fmt, ...) TLOGD(NULL, fmt, ##__VA_ARGS__)
#define LOGI(fmt, ...) TLOGI(NULL, fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) TLOGW(NULL, fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...) TLOGE(NULL, fmt, ##__VA_ARGS__)

#define TLOGV_TRACE(tag, fmt, ...) __xlog_internal_print(LOG_LEVEL_VERBOSE, tag, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define TLOGD_TRACE(tag, fmt, ...) __xlog_internal_print(LOG_LEVEL_DEBUG, tag, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define TLOGI_TRACE(tag, fmt, ...) __xlog_internal_print(LOG_LEVEL_INFO, tag, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define TLOGW_TRACE(tag, fmt, ...) __xlog_internal_print(LOG_LEVEL_WARN, tag, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define TLOGE_TRACE(tag, fmt, ...) __xlog_internal_print(LOG_LEVEL_ERROR, tag, __func__, __LINE__, fmt, ##__VA_ARGS__)

#define LOGV_TRACE(fmt, ...) TLOGV_TRACE(NULL, fmt, ##__VA_ARGS__)
#define LOGD_TRACE(fmt, ...) TLOGD_TRACE(NULL, fmt, ##__VA_ARGS__)
#define LOGI_TRACE(fmt, ...) TLOGI_TRACE(NULL, fmt, ##__VA_ARGS__)
#define LOGW_TRACE(fmt, ...) TLOGW_TRACE(NULL, fmt, ##__VA_ARGS__)
#define LOGE_TRACE(fmt, ...) TLOGE_TRACE(NULL, fmt, ##__VA_ARGS__)

#define TLOGV_HEX(tag, chars, chars_count) __xlog_internal_hex_print(LOG_LEVEL_VERBOSE, tag, chars, chars_count)
#define TLOGD_HEX(tag, chars, chars_count) __xlog_internal_hex_print(LOG_LEVEL_DEBUG, tag, chars, chars_count)
#define TLOGI_HEX(tag, chars, chars_count) __xlog_internal_hex_print(LOG_LEVEL_INFO, tag, chars, chars_count)
#define TLOGW_HEX(tag, chars, chars_count) __xlog_internal_hex_print(LOG_LEVEL_WARN, tag, chars, chars_count)
#define TLOGE_HEX(tag, chars, chars_count) __xlog_internal_hex_print(LOG_LEVEL_ERROR, tag, chars, chars_count)

#define LOGV_HEX(chars, chars_count) TLOGV_HEX(NULL, chars, chars_count)
#define LOGD_HEX(chars, chars_count) TLOGD_HEX(NULL, chars, chars_count)
#define LOGI_HEX(chars, chars_count) TLOGI_HEX(NULL, chars, chars_count)
#define LOGW_HEX(chars, chars_count) TLOGW_HEX(NULL, chars, chars_count)
#define LOGE_HEX(chars, chars_count) TLOGE_HEX(NULL, chars, chars_count)

#endif // LCU_XLOG_OFF

#endif // !LCU_XLOG_H
