#pragma once
#ifndef LCU_XLOG_H
#define LCU_XLOG_H

#include "log/logger_data.h" /* for common data */
#include <stddef.h>          /* for size_t      */

/**
 * user log callback prototype.
 * 
 *  log_msg string not end with '\n' automatically.
 *  msg_size contains NUL terminator.
 */
typedef void (*xlog_user_callback_fn)(LogLevel level, void* log_msg, size_t msg_size, void* user_data);

typedef enum
{
	LOG_TARGET_NONE = 0,
	LOG_TARGET_ANDROID = (0x1 << 1), 
	LOG_TARGET_CONSOLE = (0x1 << 2),
	LOG_TARGET_USER_CALLBACK = (0x1 << 3),
	LOG_TARGET_OHOS = (0x1 << 4),
} LogTarget;

/* native 平台合取: Android logcat | 鸿蒙 HiLog。xlog 内 log2native 用此判定。
 * 注: OHOS 构建下设 LOG_TARGET_ANDROID 亦启用 native 输出(HiLog), 因 OHOS 无 logcat,
 *   native 后端为 HiLog——便于现有调用方代码无改迁移, 属 feature。 */
#define LOG_TARGET_NATIVE (LOG_TARGET_ANDROID | LOG_TARGET_OHOS)

typedef enum
{
	/* only output pure log message */
	LOG_FORMAT_RAW = 0,
	/* output log message with timestamp */
	LOG_FORMAT_WITH_TIMESTAMP = (0x1 << 1),
	/* output log message with tag and level */
	LOG_FORMAT_WITH_TAG_LEVEL = (0x1 << 2),
	/* output log message with thread id */
	LOG_FORMAT_WITH_TID = (0x1 << 3),
} LogFormat;

typedef enum
{
	LOG_FLUSH_MODE_AUTO = 0,
	LOG_FLUSH_MODE_EVERY,
} LogFlushMode;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

	/**
     * @brief Global initialization for xlog
     *
     * @note Thread Safety:
     *       This function is thread-safe. Windows serializes initialization with a
     *       static SRW lock; other platforms initialize a guard with call_once or
     *       pthread_once.
     *       Multiple concurrent calls are safe; only the first call performs initialization.
     *
     * @note Recommended Usage:
     *       While lcu_global_init() calls this function internally, some logging macros
     *       (e.g., _LOG_INIT_IMPL in logger_facade_xlog.h) may call it directly.
     *       It is safe to call this function multiple times or from multiple threads.
     *
     * @return 0 when initialized, 1 when already initialized
     */
	int xlog_global_init();

	/**
	 * global cleanup lcu
	 *
	 * call at ending of your app,
	 * otherwise maybe some resource not released.
	 */
	int xlog_global_cleanup();

#if(!defined(_LCU_LOGGER_UNSUPPORT_STDOUT_REDIRECT) || 0 == _LCU_LOGGER_UNSUPPORT_STDOUT_REDIRECT)
	/**
	 * redirect stdout from console to file.
	 * note: NOT THREAD-SAFE. call it at beginning of your program.
	 */
	void xlog_stdout2file(char* file_path);

	/**
	 * bring stdout print on console if current print to file.
	 * note: NOT THREAD-SAFE. call it at ending of your program.
	 */
	void xlog_back2stdout();
#endif // !_LCU_LOGGER_UNSUPPORT_STDOUT_REDIRECT

	/**
	 * auto increase low level to trigger_level.
	 * if log level bigger than min_level but small than this trigger_level, transform log level to this.
	 * this is useful when some Android devices have restrictions on the log level below LOG_LEVEL_INFO.
	 */
	void xlog_auto_level_up(LogLevel trigger_level);

	/**
	 * set default log tag. use this tag if you not provide, such as call LOG<level>.
	 * note: tag length should below 24.
	 */
	void xlog_set_default_tag(char* tag);

	/**
	 * timezone_hour used by generate your local log time.
	 * timezone_hour should between -12 ~ 12. default timezone_hour is 8. 
	 * example: in china, we are in +8 timezone area, so here set it to 8.
	 */
	void xlog_set_timezone(int timezone_hour);

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
	 * set user callback. when log performed, callback will called.
	 * you can do your own log logic on callback.
	 * note: log target should include LOG_TARGET_USER_CALLBACK, otherwise callback won't triggered.
	 */
	void xlog_set_user_callback(xlog_user_callback_fn user_cb, void* user_data);

	/**
	 * @brief set log target which you want to output.
	 * @note  we use internal lock to make sure the right printing order.
	 * 
	 * @param[in]	target : default on Android is LOG_TARGET_ANDROID, other platform is LOG_TARGET_CONSOLE.
	 *                       multiple target can be combined. 
	 *                       e.g., (LOG_TARGET_ANDROID | LOG_TARGET_CONSOLE) 
	 *                       or (LOG_TARGET_CONSOLE | LOG_TARGET_USER_CALLBACK)
	 */
	void xlog_set_target(int target);

	/**
	 * get current target
	 */
	int xlog_get_target();

#if defined(__OHOS__)
	/**
	 * @brief set HiLog domain id for OHOS native log backend.
	 * @param[in] domain : HiLog domain id, valid range 0x0-0xFFFF.
	 *                     default is 0. out-of-range values are ignored.
	 */
	void xlog_set_ohos_domain(unsigned int domain);
#endif // __OHOS__

	/**
	 * set log header format.
	 * default format is (LOG_FORMAT_WITH_TIMESTAMP | LOG_FORMAT_WITH_TAG_LEVEL)
	 * android logcat won't use this header format, just log pure log msg, be aware of that.
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
	 * USE LOG<level> or TLOG<level> macro instead.
	 */
	PRINTF_FMT_CHK_GNUC(5, 6)
	void __xlog_internal_print(LogLevel level, const char* tag, const char* func_name, int file_line,
		PRINTF_FMT_CHK_MSC const char* fmt, ...);

	/**
	 * DO NOT call this method directly.(for xlog internal use only)
	 * USE LOG<level> or TLOG<level> macro instead.
	 */
	PRINTF_FMT_CHK_GNUC(6, 7)
	void xlog_internal_print_ex(LogLevel level, unsigned int flags, const char* tag, const char* func_name, int line_num,
		PRINTF_FMT_CHK_MSC const char* fmt, ...);

	/**
	 * DO NOT call this method directly.(for xlog internal use only)
	 * USE LOG<level>_HEX or TLOG<level>_HEX macro instead.
	 */
	void __xlog_internal_hex_print(LogLevel level, const char* tag, const char* chars, size_t chars_count);

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

//================LOGX EXTENSION FAMILY (no-op)================
#define TLOGX(level, flags, tag, fmt, ...)    _LOG_NOTHING()
#define LOGX(level, flags, fmt, ...)          _LOG_NOTHING()
#define TLOGXV(tag, fmt, ...)                 _LOG_NOTHING()
#define TLOGXD(tag, fmt, ...)                 _LOG_NOTHING()
#define TLOGXI(tag, fmt, ...)                 _LOG_NOTHING()
#define TLOGXW(tag, fmt, ...)                 _LOG_NOTHING()
#define TLOGXE(tag, fmt, ...)                 _LOG_NOTHING()
#define LOGXV(fmt, ...)                       _LOG_NOTHING()
#define LOGXD(fmt, ...)                       _LOG_NOTHING()
#define LOGXI(fmt, ...)                       _LOG_NOTHING()
#define LOGXW(fmt, ...)                       _LOG_NOTHING()
#define LOGXE(fmt, ...)                       _LOG_NOTHING()
#define TLOGXV_TRACE(tag, fmt, ...)           _LOG_NOTHING()
#define TLOGXD_TRACE(tag, fmt, ...)           _LOG_NOTHING()
#define TLOGXI_TRACE(tag, fmt, ...)           _LOG_NOTHING()
#define TLOGXW_TRACE(tag, fmt, ...)           _LOG_NOTHING()
#define TLOGXE_TRACE(tag, fmt, ...)           _LOG_NOTHING()
#define LOGXV_TRACE(fmt, ...)                 _LOG_NOTHING()
#define LOGXD_TRACE(fmt, ...)                 _LOG_NOTHING()
#define LOGXI_TRACE(fmt, ...)                 _LOG_NOTHING()
#define LOGXW_TRACE(fmt, ...)                 _LOG_NOTHING()
#define LOGXE_TRACE(fmt, ...)                 _LOG_NOTHING()
//================LOGX EXTENSION FAMILY (no-op)================

#else

#if(!defined(_LCU_LOGGER_UNSUPPORT_PRINTF_REDIRECT) || 0 == _LCU_LOGGER_UNSUPPORT_PRINTF_REDIRECT)
#define LOG_STD2FILE(file_path)       do { xlog_stdout2file(file_path); } while(0)
#define LOG_BACK2STD()                do { xlog_back2stdout(); } while(0)   
#else
#define LOG_STD2FILE(file_path)       do { (void)(file_path); } while (0)
#define LOG_BACK2STD()                do {} while (0)
#endif // !_LCU_LOGGER_UNSUPPORT_PRINTF_REDIRECT

// better 'define LOG_TAG "xxx"' before include "xlog.h"
// if not, we use default TAG, which you called on xlog_set_default_tag
#ifndef LOG_TAG
#define LOG_TAG  _LOG_NULL
#endif // !LOG_TAG
#ifndef _LOG_TAG
#define _LOG_TAG LOG_TAG
#endif // !_LOG_TAG

#define TLOGV(tag, fmt, ...) __xlog_internal_print(LOG_LEVEL_VERBOSE, tag, _LOG_NULL, 0, fmt, ##__VA_ARGS__)
#define TLOGD(tag, fmt, ...) __xlog_internal_print(LOG_LEVEL_DEBUG, tag, _LOG_NULL, 0, fmt, ##__VA_ARGS__)
#define TLOGI(tag, fmt, ...) __xlog_internal_print(LOG_LEVEL_INFO, tag, _LOG_NULL, 0, fmt, ##__VA_ARGS__)
#define TLOGW(tag, fmt, ...) __xlog_internal_print(LOG_LEVEL_WARN, tag, _LOG_NULL, 0, fmt, ##__VA_ARGS__)
#define TLOGE(tag, fmt, ...) __xlog_internal_print(LOG_LEVEL_ERROR, tag, _LOG_NULL, 0, fmt, ##__VA_ARGS__)

#define LOGV(fmt, ...) TLOGV(_LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGD(fmt, ...) TLOGD(_LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGI(fmt, ...) TLOGI(_LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) TLOGW(_LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...) TLOGE(_LOG_TAG, fmt, ##__VA_ARGS__)

#define TLOGV_TRACE(tag, fmt, ...) __xlog_internal_print(LOG_LEVEL_VERBOSE, tag, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define TLOGD_TRACE(tag, fmt, ...) __xlog_internal_print(LOG_LEVEL_DEBUG, tag, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define TLOGI_TRACE(tag, fmt, ...) __xlog_internal_print(LOG_LEVEL_INFO, tag, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define TLOGW_TRACE(tag, fmt, ...) __xlog_internal_print(LOG_LEVEL_WARN, tag, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define TLOGE_TRACE(tag, fmt, ...) __xlog_internal_print(LOG_LEVEL_ERROR, tag, __func__, __LINE__, fmt, ##__VA_ARGS__)

#define LOGV_TRACE(fmt, ...) TLOGV_TRACE(_LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGD_TRACE(fmt, ...) TLOGD_TRACE(_LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGI_TRACE(fmt, ...) TLOGI_TRACE(_LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGW_TRACE(fmt, ...) TLOGW_TRACE(_LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGE_TRACE(fmt, ...) TLOGE_TRACE(_LOG_TAG, fmt, ##__VA_ARGS__)

#define TLOGV_HEX(tag, chars, chars_count) __xlog_internal_hex_print(LOG_LEVEL_VERBOSE, tag, chars, chars_count)
#define TLOGD_HEX(tag, chars, chars_count) __xlog_internal_hex_print(LOG_LEVEL_DEBUG, tag, chars, chars_count)
#define TLOGI_HEX(tag, chars, chars_count) __xlog_internal_hex_print(LOG_LEVEL_INFO, tag, chars, chars_count)
#define TLOGW_HEX(tag, chars, chars_count) __xlog_internal_hex_print(LOG_LEVEL_WARN, tag, chars, chars_count)
#define TLOGE_HEX(tag, chars, chars_count) __xlog_internal_hex_print(LOG_LEVEL_ERROR, tag, chars, chars_count)

#define LOGV_HEX(chars, chars_count) TLOGV_HEX(_LOG_TAG, chars, chars_count)
#define LOGD_HEX(chars, chars_count) TLOGD_HEX(_LOG_TAG, chars, chars_count)
#define LOGI_HEX(chars, chars_count) TLOGI_HEX(_LOG_TAG, chars, chars_count)
#define LOGW_HEX(chars, chars_count) TLOGW_HEX(_LOG_TAG, chars, chars_count)
#define LOGE_HEX(chars, chars_count) TLOGE_HEX(_LOG_TAG, chars, chars_count)

//================LOGX EXTENSION FAMILY================
#define TLOGX(level, flags, tag, fmt, ...)  xlog_internal_print_ex(level, flags, tag, _LOG_NULL, 0, fmt, ##__VA_ARGS__)
#define LOGX(level, flags, fmt, ...)        TLOGX(level, flags, _LOG_TAG, fmt, ##__VA_ARGS__)

#define TLOGXV(tag, fmt, ...)  TLOGX(LOG_LEVEL_VERBOSE, LOG_FLAG_SANITIZE, tag, fmt, ##__VA_ARGS__)
#define TLOGXD(tag, fmt, ...)  TLOGX(LOG_LEVEL_DEBUG,   LOG_FLAG_SANITIZE, tag, fmt, ##__VA_ARGS__)
#define TLOGXI(tag, fmt, ...)  TLOGX(LOG_LEVEL_INFO,    LOG_FLAG_SANITIZE, tag, fmt, ##__VA_ARGS__)
#define TLOGXW(tag, fmt, ...)  TLOGX(LOG_LEVEL_WARN,    LOG_FLAG_SANITIZE, tag, fmt, ##__VA_ARGS__)
#define TLOGXE(tag, fmt, ...)  TLOGX(LOG_LEVEL_ERROR,   LOG_FLAG_SANITIZE, tag, fmt, ##__VA_ARGS__)

#define LOGXV(fmt, ...)  TLOGXV(_LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGXD(fmt, ...)  TLOGXD(_LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGXI(fmt, ...)  TLOGXI(_LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGXW(fmt, ...)  TLOGXW(_LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGXE(fmt, ...)  TLOGXE(_LOG_TAG, fmt, ##__VA_ARGS__)

#define TLOGXV_TRACE(tag, fmt, ...)  xlog_internal_print_ex(LOG_LEVEL_VERBOSE, LOG_FLAG_SANITIZE, tag, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define TLOGXD_TRACE(tag, fmt, ...)  xlog_internal_print_ex(LOG_LEVEL_DEBUG,   LOG_FLAG_SANITIZE, tag, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define TLOGXI_TRACE(tag, fmt, ...)  xlog_internal_print_ex(LOG_LEVEL_INFO,    LOG_FLAG_SANITIZE, tag, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define TLOGXW_TRACE(tag, fmt, ...)  xlog_internal_print_ex(LOG_LEVEL_WARN,    LOG_FLAG_SANITIZE, tag, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define TLOGXE_TRACE(tag, fmt, ...)  xlog_internal_print_ex(LOG_LEVEL_ERROR,   LOG_FLAG_SANITIZE, tag, __func__, __LINE__, fmt, ##__VA_ARGS__)

#define LOGXV_TRACE(fmt, ...)  TLOGXV_TRACE(_LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGXD_TRACE(fmt, ...)  TLOGXD_TRACE(_LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGXI_TRACE(fmt, ...)  TLOGXI_TRACE(_LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGXW_TRACE(fmt, ...)  TLOGXW_TRACE(_LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGXE_TRACE(fmt, ...)  TLOGXE_TRACE(_LOG_TAG, fmt, ##__VA_ARGS__)
//================LOGX EXTENSION FAMILY================

#endif // close LCU_XLOG_OFF #if

#endif // !LCU_XLOG_H
