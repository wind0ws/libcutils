#pragma once
#ifndef LCU_LOGGER_FACADE_SLOG_H
#define LCU_LOGGER_FACADE_SLOG_H

#include "log/slog.h"

#define _LOG_INIT_IMPL(params)             do { (void)(params); } while(0)
#define _LOG_DEINIT_IMPL(params)           do { (void)(params); } while(0)
#define _LOG_SET_MIN_LEVEL_IMPL(min_level) SLOG_SET_MIN_LEVEL(min_level)
#define _LOG_GET_MIN_LEVEL_IMPL()          SLOG_GET_MIN_LEVEL()
#define _LOG_STD2FILE_IMPL(file_path)      SLOG_STD2FILE(file_path)
#define _LOG_BACK2STD_IMPL()               SLOG_BACK2STD()   

#define _LOGV_TRACE_IMPL(tag, func_name, line_num, fmt, ...) _SLOGV_TRACE_IMPL(tag, func_name, line_num, fmt, ##__VA_ARGS__)
#define _LOGD_TRACE_IMPL(tag, func_name, line_num, fmt, ...) _SLOGD_TRACE_IMPL(tag, func_name, line_num, fmt, ##__VA_ARGS__)
#define _LOGI_TRACE_IMPL(tag, func_name, line_num, fmt, ...) _SLOGI_TRACE_IMPL(tag, func_name, line_num, fmt, ##__VA_ARGS__)
#define _LOGW_TRACE_IMPL(tag, func_name, line_num, fmt, ...) _SLOGW_TRACE_IMPL(tag, func_name, line_num, fmt, ##__VA_ARGS__)
#define _LOGE_TRACE_IMPL(tag, func_name, line_num, fmt, ...) _SLOGE_TRACE_IMPL(tag, func_name, line_num, fmt, ##__VA_ARGS__)

#define _LOGV_IMPL(tag, fmt, ...) SLOGV(tag, fmt, ##__VA_ARGS__)
#define _LOGD_IMPL(tag, fmt, ...) SLOGD(tag, fmt, ##__VA_ARGS__)
#define _LOGI_IMPL(tag, fmt, ...) SLOGI(tag, fmt, ##__VA_ARGS__)
#define _LOGW_IMPL(tag, fmt, ...) SLOGW(tag, fmt, ##__VA_ARGS__)
#define _LOGE_IMPL(tag, fmt, ...) SLOGE(tag, fmt, ##__VA_ARGS__)

#define _LOGV_HEX_IMPL(tag, chars, chars_count) SLOGV_HEX(tag, chars, chars_count)
#define _LOGD_HEX_IMPL(tag, chars, chars_count) SLOGD_HEX(tag, chars, chars_count)
#define _LOGI_HEX_IMPL(tag, chars, chars_count) SLOGI_HEX(tag, chars, chars_count)
#define _LOGW_HEX_IMPL(tag, chars, chars_count) SLOGW_HEX(tag, chars, chars_count)
#define _LOGE_HEX_IMPL(tag, chars, chars_count) SLOGE_HEX(tag, chars, chars_count)


#include "logger_facade.h"

#endif // !LCU_LOGGER_FACADE_SLOG_H
