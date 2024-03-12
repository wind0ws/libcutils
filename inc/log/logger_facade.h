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

#if(!defined(_LCU_LOGGER_DISABLE) || 0 != _LCU_LOGGER_DISABLE)

#ifndef LOG_TAG
#error "You should define \"LOG_TAG\" before include <logger_facade_xx.h>"
#endif // !LOG_TAG

#if(!defined(_LOG_INIT_IMPL) || !defined(_LOG_DEINIT_IMPL))
#error "You should define \"_LOG_INIT/DEINIT_IMPL\" first, did you forget to include implement?"
#elif(!defined(_LOG_SET_MIN_LEVEL_IMPL) || !defined(_LOG_GET_MIN_LEVEL_IMPL))
#error "You should define \"_LOG_SET/GET_MIN_LEVEL_IMPL\""
#elif(!defined(_LOG_STD2FILE_IMPL) || !defined(_LOG_BACK2STD_IMPL))
#error "You should define \"_LOG_STD2FILE/BACK2STD_IMPL\""
#elif(!defined(_LOGV_IMPL) || !defined(_LOGV_TRACE_IMPL))
#error "You should define \"_LOGX_IMPL\" and \"_LOGX_TRACE_IMPL\""
#elif(!defined(_LOGV_HEX_IMPL))
#error "You should define \"_LOGX_HEX_IMPL\""
#endif // _LOG_XX_IMPL definition check

// init at beginning of your app
#define LOG_GLOBAL_INIT(params)      _LOG_INIT_IMPL(params)
// cleanup at ending of you app
#define LOG_GLOBAL_CLEANUP(params)   _LOG_DEINIT_IMPL(params)
#define LOG_SET_MIN_LEVEL(min_level) _LOG_SET_MIN_LEVEL_IMPL(min_level)
#define LOG_GET_MIN_LEVEL()          _LOG_GET_MIN_LEVEL_IMPL()
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

#define LOGV(fmt, ...)         TLOGV(LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGD(fmt, ...)         TLOGD(LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGI(fmt, ...)         TLOGI(LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...)         TLOGW(LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...)         TLOGE(LOG_TAG, fmt, ##__VA_ARGS__)

#define LOGV_TRACE(fmt, ...)     TLOGV_TRACE(LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGD_TRACE(fmt, ...)     TLOGD_TRACE(LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGI_TRACE(fmt, ...)     TLOGI_TRACE(LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGW_TRACE(fmt, ...)     TLOGW_TRACE(LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGE_TRACE(fmt, ...)     TLOGE_TRACE(LOG_TAG, fmt, ##__VA_ARGS__)

#define TLOGV_HEX(tag, chars, chars_count)     _LOGV_HEX_IMPL(tag, chars, chars_count)
#define TLOGD_HEX(tag, chars, chars_count)     _LOGD_HEX_IMPL(tag, chars, chars_count)
#define TLOGI_HEX(tag, chars, chars_count)     _LOGI_HEX_IMPL(tag, chars, chars_count)
#define TLOGW_HEX(tag, chars, chars_count)     _LOGW_HEX_IMPL(tag, chars, chars_count)
#define TLOGE_HEX(tag, chars, chars_count)     _LOGE_HEX_IMPL(tag, chars, chars_count)

#define LOGV_HEX(chars, chars_count)  TLOGV_HEX(LOG_TAG, chars, chars_count)
#define LOGD_HEX(chars, chars_count)  TLOGD_HEX(LOG_TAG, chars, chars_count)
#define LOGI_HEX(chars, chars_count)  TLOGI_HEX(LOG_TAG, chars, chars_count)
#define LOGW_HEX(chars, chars_count)  TLOGW_HEX(LOG_TAG, chars, chars_count)
#define LOGE_HEX(chars, chars_count)  TLOGE_HEX(LOG_TAG, chars, chars_count)

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


#endif // !_LCU_LOGGER_DISABLE

#endif // !LCU_LOGGER_FACADE_H
