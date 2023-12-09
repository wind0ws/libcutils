#pragma once
#ifndef LCU_LOGGER_FACADE_XLOG_H
#define LCU_LOGGER_FACADE_XLOG_H

#include "log/xlog.h"

#define _LOG_INIT_IMPL(params)             do { (void)(params); xlog_global_init(); } while(0)
#define _LOG_DEINIT_IMPL(params)           do { (void)(params); xlog_global_cleanup(); } while(0)
#define _LOG_SET_MIN_LEVEL_IMPL(min_level) do { xlog_set_min_level(min_level); } while(0)
#define _LOG_GET_MIN_LEVEL_IMPL()          xlog_get_min_level()

#if(!defined(_LCU_LOGGER_UNSUPPORT_STDOUT_REDIRECT) || 0 == _LCU_LOGGER_UNSUPPORT_STDOUT_REDIRECT)
#define _LOG_STD2FILE_IMPL(file_path)       do { xlog_stdout2file(file_path); } while(0)
#define _LOG_BACK2STD_IMPL()                do { xlog_back2stdout(); } while(0)   
#else
#define _LOG_STD2FILE_IMPL(file_path)       do { (void)(file_path); } while (0)
#define _LOG_BACK2STD_IMPL()                do {} while (0)
#endif // !_LCU_LOGGER_UNSUPPORT_STDOUT_REDIRECT

#define _LOGV_TRACE_IMPL(tag, func_name, line_num, fmt, ...) __xlog_internal_print(LOG_LEVEL_VERBOSE, tag, func_name, line_num, fmt, ##__VA_ARGS__)
#define _LOGD_TRACE_IMPL(tag, func_name, line_num, fmt, ...) __xlog_internal_print(LOG_LEVEL_DEBUG, tag, func_name, line_num, fmt, ##__VA_ARGS__)
#define _LOGI_TRACE_IMPL(tag, func_name, line_num, fmt, ...) __xlog_internal_print(LOG_LEVEL_INFO, tag, func_name, line_num, fmt, ##__VA_ARGS__)
#define _LOGW_TRACE_IMPL(tag, func_name, line_num, fmt, ...) __xlog_internal_print(LOG_LEVEL_WARN, tag, func_name, line_num, fmt, ##__VA_ARGS__)
#define _LOGE_TRACE_IMPL(tag, func_name, line_num, fmt, ...) __xlog_internal_print(LOG_LEVEL_ERROR, tag, func_name, line_num, fmt, ##__VA_ARGS__)

#define _LOGV_IMPL(tag, fmt, ...) _LOGV_TRACE_IMPL(tag, NULL, 0, fmt, ##__VA_ARGS__)
#define _LOGD_IMPL(tag, fmt, ...) _LOGD_TRACE_IMPL(tag, NULL, 0, fmt, ##__VA_ARGS__)
#define _LOGI_IMPL(tag, fmt, ...) _LOGI_TRACE_IMPL(tag, NULL, 0, fmt, ##__VA_ARGS__)
#define _LOGW_IMPL(tag, fmt, ...) _LOGW_TRACE_IMPL(tag, NULL, 0, fmt, ##__VA_ARGS__)
#define _LOGE_IMPL(tag, fmt, ...) _LOGE_TRACE_IMPL(tag, NULL, 0, fmt, ##__VA_ARGS__)

#define _LOGV_HEX_IMPL(tag, chars, chars_count) __xlog_internal_hex_print(LOG_LEVEL_VERBOSE, tag, chars, chars_count)
#define _LOGD_HEX_IMPL(tag, chars, chars_count) __xlog_internal_hex_print(LOG_LEVEL_DEBUG, tag, chars, chars_count)
#define _LOGI_HEX_IMPL(tag, chars, chars_count) __xlog_internal_hex_print(LOG_LEVEL_INFO, tag, chars, chars_count)
#define _LOGW_HEX_IMPL(tag, chars, chars_count) __xlog_internal_hex_print(LOG_LEVEL_WARN, tag, chars, chars_count)
#define _LOGE_HEX_IMPL(tag, chars, chars_count) __xlog_internal_hex_print(LOG_LEVEL_ERROR, tag, chars, chars_count)

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

#include "logger_facade.h"


#endif // !LCU_LOGGER_FACADE_XLOG_H
