/*
 * DO NOT include this header directly!
 * @brief: this is a facade of logger,
 *         a logger should implement definition of _LOG_XXX_IMPL
 */
#pragma once
#ifndef LCU_LOGGER_FACADE_H
#define LCU_LOGGER_FACADE_H

//================CLEANUP PREVIOUS DEFINITION================
#undef LOG_STD2FILE
#undef LOG_BACK2STD

#undef TLOGV
#undef TLOGD
#undef TLOGI
#undef TLOGW
#undef TLOGE

#undef TLOGV_TRACE
#undef TLOGD_TRACE
#undef TLOGI_TRACE
#undef TLOGW_TRACE
#undef TLOGE_TRACE

#undef LOGV
#undef LOGD
#undef LOGI
#undef LOGW
#undef LOGE

#undef LOGV_TRACE
#undef LOGD_TRACE
#undef LOGI_TRACE
#undef LOGW_TRACE
#undef LOGE_TRACE

#undef TLOGV_HEX
#undef TLOGD_HEX
#undef TLOGI_HEX
#undef TLOGW_HEX
#undef TLOGE_HEX

#undef LOGV_HEX
#undef LOGD_HEX
#undef LOGI_HEX
#undef LOGW_HEX
#undef LOGE_HEX
//================CLEANUP PREVIOUS DEFINITION================

#undef TLOGX
#undef LOGX
#undef TLOGXV
#undef TLOGXD
#undef TLOGXI
#undef TLOGXW
#undef TLOGXE
#undef LOGXV
#undef LOGXD
#undef LOGXI
#undef LOGXW
#undef LOGXE
#undef TLOGXV_TRACE
#undef TLOGXD_TRACE
#undef TLOGXI_TRACE
#undef TLOGXW_TRACE
#undef TLOGXE_TRACE
#undef LOGXV_TRACE
#undef LOGXD_TRACE
#undef LOGXI_TRACE
#undef LOGXW_TRACE
#undef LOGXE_TRACE
//================CLEANUP LOGX DEFINITION================

#if(!defined(_LCU_LOGGER_DISABLE) || 0 != _LCU_LOGGER_DISABLE)

#ifndef _LOG_TAG
#error "You should define \"_LOG_TAG\" before include <logger_facade_xx.h>"
#endif // !_LOG_TAG

#if(!defined(_LOG_INIT_IMPL) || !defined(_LOG_DEINIT_IMPL))
#error "You should define \"_LOG_INIT/DEINIT_IMPL\" first, did you forget to include implement?"
#elif(!defined(_LOG_SET_MIN_LEVEL_IMPL) || !defined(_LOG_GET_MIN_LEVEL_IMPL))
#error "You should define \"_LOG_SET/GET_MIN_LEVEL_IMPL\""
#elif(!defined(_LOG_SET_FORMAT_IMPL) || !defined(_LOG_GET_FORMAT_IMPL))
#error "You should define \"_LOG_SET/GET_FORMAT_IMPL\""
#elif(!defined(_LOG_STD2FILE_IMPL) || !defined(_LOG_BACK2STD_IMPL))
#error "You should define \"_LOG_STD2FILE/BACK2STD_IMPL\""
#elif(!defined(_LOGV_IMPL) || !defined(_LOGV_TRACE_IMPL))
#error "You should define \"_LOGX_IMPL\" and \"_LOGX_TRACE_IMPL\""
#elif(!defined(_LOGV_HEX_IMPL))
#error "You should define \"_LOGX_HEX_IMPL\""
#elif(!defined(_LOGX_TRACE_IMPL))
#error "You should define \"_LOGX_TRACE_IMPL(level, flags, tag, func_name, line_num, fmt, ...)\""
#endif // _LOG_XX_IMPL definition check

// init at beginning of your app
#define LOG_GLOBAL_INIT(params)      _LOG_INIT_IMPL(params)
// cleanup at ending of you app
#define LOG_GLOBAL_CLEANUP(params)   _LOG_DEINIT_IMPL(params)
#define LOG_SET_MIN_LEVEL(min_level) _LOG_SET_MIN_LEVEL_IMPL(min_level)
#define LOG_GET_MIN_LEVEL()          _LOG_GET_MIN_LEVEL_IMPL()
#define LOG_SET_FORMAT(format)       _LOG_SET_FORMAT_IMPL(format)
#define LOG_GET_FORMAT()             _LOG_GET_FORMAT_IMPL()
#define LOG_STD2FILE(file_path)      _LOG_STD2FILE_IMPL(file_path)
#define LOG_BACK2STD()               _LOG_BACK2STD_IMPL()

#define TLOGV(tag, fmt, ...)     _LOGV_IMPL(tag, fmt, ##__VA_ARGS__)
#define TLOGD(tag, fmt, ...)     _LOGD_IMPL(tag, fmt, ##__VA_ARGS__)
#define TLOGI(tag, fmt, ...)     _LOGI_IMPL(tag, fmt, ##__VA_ARGS__)
#define TLOGW(tag, fmt, ...)     _LOGW_IMPL(tag, fmt, ##__VA_ARGS__)
#define TLOGE(tag, fmt, ...)     _LOGE_IMPL(tag, fmt, ##__VA_ARGS__)

#define TLOGV_TRACE(tag, fmt, ...)     _LOGV_TRACE_IMPL(tag, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define TLOGD_TRACE(tag, fmt, ...)     _LOGD_TRACE_IMPL(tag, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define TLOGI_TRACE(tag, fmt, ...)     _LOGI_TRACE_IMPL(tag, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define TLOGW_TRACE(tag, fmt, ...)     _LOGW_TRACE_IMPL(tag, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define TLOGE_TRACE(tag, fmt, ...)     _LOGE_TRACE_IMPL(tag, __func__, __LINE__, fmt, ##__VA_ARGS__)

#define LOGV(fmt, ...)         TLOGV(_LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGD(fmt, ...)         TLOGD(_LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGI(fmt, ...)         TLOGI(_LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...)         TLOGW(_LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...)         TLOGE(_LOG_TAG, fmt, ##__VA_ARGS__)

#define LOGV_TRACE(fmt, ...)     TLOGV_TRACE(_LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGD_TRACE(fmt, ...)     TLOGD_TRACE(_LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGI_TRACE(fmt, ...)     TLOGI_TRACE(_LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGW_TRACE(fmt, ...)     TLOGW_TRACE(_LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGE_TRACE(fmt, ...)     TLOGE_TRACE(_LOG_TAG, fmt, ##__VA_ARGS__)

#define TLOGV_HEX(tag, chars, chars_count)     _LOGV_HEX_IMPL(tag, chars, chars_count)
#define TLOGD_HEX(tag, chars, chars_count)     _LOGD_HEX_IMPL(tag, chars, chars_count)
#define TLOGI_HEX(tag, chars, chars_count)     _LOGI_HEX_IMPL(tag, chars, chars_count)
#define TLOGW_HEX(tag, chars, chars_count)     _LOGW_HEX_IMPL(tag, chars, chars_count)
#define TLOGE_HEX(tag, chars, chars_count)     _LOGE_HEX_IMPL(tag, chars, chars_count)

#define LOGV_HEX(chars, chars_count)  TLOGV_HEX(_LOG_TAG, chars, chars_count)
#define LOGD_HEX(chars, chars_count)  TLOGD_HEX(_LOG_TAG, chars, chars_count)
#define LOGI_HEX(chars, chars_count)  TLOGI_HEX(_LOG_TAG, chars, chars_count)
#define LOGW_HEX(chars, chars_count)  TLOGW_HEX(_LOG_TAG, chars, chars_count)
#define LOGE_HEX(chars, chars_count)  TLOGE_HEX(_LOG_TAG, chars, chars_count)

//================LOGX EXTENSION FAMILY================
// 低层原语（完整参数，flags=0 时等同普通日志）
#define TLOGX(level, flags, tag, fmt, ...)  _LOGX_TRACE_IMPL(level, flags, tag, _LOG_NULL, 0, fmt, ##__VA_ARGS__)
#define LOGX(level, flags, fmt, ...)        TLOGX(level, flags, _LOG_TAG, fmt, ##__VA_ARGS__)

// 快捷宏（隐含 LOG_FLAG_SANITIZE，自动脱敏敏感字段）
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

// TRACE 变体（带 func:line）
#define TLOGXV_TRACE(tag, fmt, ...)  _LOGX_TRACE_IMPL(LOG_LEVEL_VERBOSE, LOG_FLAG_SANITIZE, tag, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define TLOGXD_TRACE(tag, fmt, ...)  _LOGX_TRACE_IMPL(LOG_LEVEL_DEBUG,   LOG_FLAG_SANITIZE, tag, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define TLOGXI_TRACE(tag, fmt, ...)  _LOGX_TRACE_IMPL(LOG_LEVEL_INFO,    LOG_FLAG_SANITIZE, tag, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define TLOGXW_TRACE(tag, fmt, ...)  _LOGX_TRACE_IMPL(LOG_LEVEL_WARN,    LOG_FLAG_SANITIZE, tag, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define TLOGXE_TRACE(tag, fmt, ...)  _LOGX_TRACE_IMPL(LOG_LEVEL_ERROR,   LOG_FLAG_SANITIZE, tag, __func__, __LINE__, fmt, ##__VA_ARGS__)

#define LOGXV_TRACE(fmt, ...)  TLOGXV_TRACE(_LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGXD_TRACE(fmt, ...)  TLOGXD_TRACE(_LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGXI_TRACE(fmt, ...)  TLOGXI_TRACE(_LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGXW_TRACE(fmt, ...)  TLOGXW_TRACE(_LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGXE_TRACE(fmt, ...)  TLOGXE_TRACE(_LOG_TAG, fmt, ##__VA_ARGS__)
//================LOGX EXTENSION FAMILY================

#else

#ifndef _LOG_NOTHING
#define _LOG_NOTHING()               do { } while (0)
#endif // !_LOG_NOTHING

#define LOG_GLOBAL_INIT(params)      _LOG_NOTHING()
#define LOG_GLOBAL_CLEANUP(params)   _LOG_NOTHING()
#define LOG_SET_MIN_LEVEL(min_level) _LOG_NOTHING()
#define LOG_GET_MIN_LEVEL()          _LOG_NOTHING()
#define LOG_STD2FILE(file_path)      _LOG_NOTHING()
#define LOG_BACK2STD()               _LOG_NOTHING()

#define TLOGV(tag, fmt, ...)         _LOG_NOTHING()
#define TLOGD(tag, fmt, ...)         _LOG_NOTHING()
#define TLOGI(tag, fmt, ...)         _LOG_NOTHING()
#define TLOGW(tag, fmt, ...)         _LOG_NOTHING()
#define TLOGE(tag, fmt, ...)         _LOG_NOTHING()

#define TLOGV_TRACE(tag, fmt, ...)   _LOG_NOTHING()
#define TLOGD_TRACE(tag, fmt, ...)   _LOG_NOTHING()
#define TLOGI_TRACE(tag, fmt, ...)   _LOG_NOTHING()
#define TLOGW_TRACE(tag, fmt, ...)   _LOG_NOTHING()
#define TLOGE_TRACE(tag, fmt, ...)   _LOG_NOTHING()

#define LOGV(fmt, ...)               _LOG_NOTHING()
#define LOGD(fmt, ...)               _LOG_NOTHING()
#define LOGW(fmt, ...)               _LOG_NOTHING()
#define LOGI(fmt, ...)               _LOG_NOTHING()
#define LOGE(fmt, ...)               _LOG_NOTHING()

#define LOGV_TRACE(fmt, ...)         _LOG_NOTHING()
#define LOGD_TRACE(fmt, ...)         _LOG_NOTHING()
#define LOGI_TRACE(fmt, ...)         _LOG_NOTHING()
#define LOGW_TRACE(fmt, ...)         _LOG_NOTHING()
#define LOGE_TRACE(fmt, ...)         _LOG_NOTHING()

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
#define TLOGX(level, flags, tag, fmt, ...)  _LOG_NOTHING()
#define LOGX(level, flags, fmt, ...)        _LOG_NOTHING()
#define TLOGXV(tag, fmt, ...)               _LOG_NOTHING()
#define TLOGXD(tag, fmt, ...)               _LOG_NOTHING()
#define TLOGXI(tag, fmt, ...)               _LOG_NOTHING()
#define TLOGXW(tag, fmt, ...)               _LOG_NOTHING()
#define TLOGXE(tag, fmt, ...)               _LOG_NOTHING()
#define LOGXV(fmt, ...)                     _LOG_NOTHING()
#define LOGXD(fmt, ...)                     _LOG_NOTHING()
#define LOGXI(fmt, ...)                     _LOG_NOTHING()
#define LOGXW(fmt, ...)                     _LOG_NOTHING()
#define LOGXE(fmt, ...)                     _LOG_NOTHING()
#define TLOGXV_TRACE(tag, fmt, ...)         _LOG_NOTHING()
#define TLOGXD_TRACE(tag, fmt, ...)         _LOG_NOTHING()
#define TLOGXI_TRACE(tag, fmt, ...)         _LOG_NOTHING()
#define TLOGXW_TRACE(tag, fmt, ...)         _LOG_NOTHING()
#define TLOGXE_TRACE(tag, fmt, ...)         _LOG_NOTHING()
#define LOGXV_TRACE(fmt, ...)               _LOG_NOTHING()
#define LOGXD_TRACE(fmt, ...)               _LOG_NOTHING()
#define LOGXI_TRACE(fmt, ...)               _LOG_NOTHING()
#define LOGXW_TRACE(fmt, ...)               _LOG_NOTHING()
#define LOGXE_TRACE(fmt, ...)               _LOG_NOTHING()
//================LOGX EXTENSION FAMILY (no-op)================


#endif // !_LCU_LOGGER_DISABLE

#endif // !LCU_LOGGER_FACADE_H
