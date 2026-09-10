#include "log/slog.h"
#include "log/log_sanitizer.h"
#include "mem/strings.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>   /* for malloc / free */
#include <string.h>   /* for strlen / memcpy */

// 依赖平台对 int 大小变量赋值的原子性，多线程下不加锁属有意设计
LogLevel _g_slog_min_level = LOG_LEVEL_VERBOSE;

#if(!defined(_LCU_LOGGER_UNSUPPORT_STDOUT_REDIRECT) || 0 == _LCU_LOGGER_UNSUPPORT_STDOUT_REDIRECT)
typedef struct slog_config
{
	/* the file pointer after redirect stdout */
	FILE* fp_out;
} slog_config_t;

static slog_config_t g_slog = { NULL };
#endif // !_LCU_LOGGER_UNSUPPORT_STDOUT_REDIRECT

void slog_set_min_level(LogLevel min_level)
{
	if ((int)min_level < (int)LOG_LEVEL_OFF || (int)min_level > (int)LOG_LEVEL_ERROR)
	{
		fprintf(stderr, "[slog] (%s:%d) invalid min_level:%d\n", __func__, __LINE__, min_level);
		return; // invalid min_level
	}
	fprintf(stderr, "[slog] (%s:%d) set new log_min_level:%d\n", __func__, __LINE__, min_level);
	_g_slog_min_level = min_level;
}

LogLevel slog_get_min_level(void)
{
	return _g_slog_min_level;
}

#if(!defined(_LCU_LOGGER_UNSUPPORT_STDOUT_REDIRECT) || 0 == _LCU_LOGGER_UNSUPPORT_STDOUT_REDIRECT)
#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable:4996) //for disable freopen warning
#endif // _WIN32

void slog_stdout2file(const char* file_path)
{
	if (NULL == file_path || '\0' == file_path[0])
	{
		return;
	}
	if (g_slog.fp_out && (g_slog.fp_out != stdout))
	{
		fprintf(stderr, "[slog] (%s:%d) WARN: did you forgot to close the redirect stdout file stream!\n", __func__, __LINE__);
		fflush(g_slog.fp_out);
		fclose(g_slog.fp_out);
	}
	fflush(stdout);
	g_slog.fp_out = freopen(file_path, "w", stdout);
	if (!g_slog.fp_out)
	{
		fprintf(stderr, "[slog] (%s:%d) Error: failed on freopen to file(%s)\n", __func__, __LINE__, file_path);
		freopen(_STDOUT_NODE, "w", stdout);
	}
}

void slog_back2stdout()
{
	if (!g_slog.fp_out)
	{
		return;
	}
	// must close current stream first, and then reopen it
	fflush(g_slog.fp_out);
	fclose(g_slog.fp_out);
	g_slog.fp_out = NULL;
	if (!freopen(_STDOUT_NODE, "w", stdout))
	{
		fprintf(stderr, "[slog] (%s:%d) Error: failed on freopen to stdout\n", __func__, __LINE__);
	}
}

#ifdef _WIN32
#pragma warning(pop)
#endif // _WIN32

#endif // !_LCU_LOGGER_UNSUPPORT_STDOUT_REDIRECT

void _slog_internal_hex_print(int level, const char* tag, const char* chars, size_t chars_count)
{
	char buf[256];// use small stack size
	str_char2hex(buf, sizeof(buf), chars, chars_count);
	switch (level)
	{
	case LOG_LEVEL_VERBOSE:
		SLOGV(tag, "%s", buf);
		break;
	case LOG_LEVEL_DEBUG:
		SLOGD(tag, "%s", buf);
		break;
	case LOG_LEVEL_INFO:
		SLOGI(tag, "%s", buf);
		break;
	case LOG_LEVEL_WARN:
		SLOGW(tag, "%s", buf);
		break;
	case LOG_LEVEL_ERROR:
	default:
		SLOGE(tag, "%s", buf);
		break;
	}
}

/* 内部 v* helper：接收已初始化的 va_list，避免 va_list 穿透变参函数 */
static void _slog_print_ex_v(int level, unsigned int flags, const char* tag,
	const char* func_name, int line_num, const char* fmt, va_list va)
{
	/* 1) 格式化消息体（不含前缀）到 body 缓冲（栈或堆）。
	 *    前缀与 body 使用相互独立的缓冲，避免前缀写入覆盖 body 起始字节。 */
	char stack_buf[512];
	char* body = stack_buf;
	size_t body_size = sizeof(stack_buf);
	bool body_heap = false;
	int ret_vsn;
	{
		va_list va2;
		va_copy(va2, va);
		ret_vsn = vsnprintf(stack_buf, body_size, fmt, va2);
		va_end(va2);

		if (ret_vsn < 0)
		{
			return;
		}

		if (ret_vsn >= (int)body_size)
		{
			body_size = (size_t)ret_vsn + 1U;
			body = (char*)malloc(body_size);
			if (NULL == body)
			{
				return;
			}
			body_heap = true;
			va_copy(va2, va);
			vsnprintf(body, body_size, fmt, va2);
			va_end(va2);
		}
	}

#ifdef __ANDROID__
	/* Android: __android_log_print 接收 tag 参数，无需前缀 */
	if (flags & LOG_FLAG_SANITIZE)
	{
		log_sanitize_in_place(body, 0);
	}
	{
		static const char map_android[] = {
			ANDROID_LOG_UNKNOWN, ANDROID_LOG_VERBOSE, ANDROID_LOG_DEBUG,
			ANDROID_LOG_INFO, ANDROID_LOG_WARN, ANDROID_LOG_ERROR, ANDROID_LOG_ERROR
		};
		int android_level = (level < 0 || level > 5) ? ANDROID_LOG_ERROR : map_android[level];
		__android_log_print(android_level, tag, "%s", body);
	}
#else
	/* console: 前缀写入独立小缓冲，与 body 分离 */
	{
		char level_char = '?';
		switch (level)
		{
		case LOG_LEVEL_VERBOSE: level_char = 'V'; break;
		case LOG_LEVEL_DEBUG:   level_char = 'D'; break;
		case LOG_LEVEL_INFO:    level_char = 'I'; break;
		case LOG_LEVEL_WARN:    level_char = 'W'; break;
		case LOG_LEVEL_ERROR:   level_char = 'E'; break;
		default:                level_char = '?'; break;
		}

		char prefix_buf[256];
		size_t prefix_len = (size_t)snprintf(prefix_buf, sizeof(prefix_buf), "[%c/%s]", level_char, tag);
		if (func_name && line_num > 0)
		{
			prefix_len += (size_t)snprintf(prefix_buf + prefix_len, sizeof(prefix_buf) - prefix_len,
				"(%s:%d) ", func_name, line_num);
		}

		/* 脱敏仅作用于消息体，不碰前缀 */
		if (flags & LOG_FLAG_SANITIZE)
		{
			log_sanitize_in_place(body, 0);
		}

		size_t body_len = strlen(body);
		const size_t full_len = prefix_len + body_len + 1U;
		char full_buf[512];
		char* full = full_buf;
		bool full_heap = false;
		if (full_len <= sizeof(full_buf))
		{
			memcpy(full, prefix_buf, prefix_len);
			memcpy(full + prefix_len, body, body_len + 1U);
		}
		else
		{
			full = (char*)malloc(full_len);
			if (NULL != full)
			{
				full_heap = true;
				memcpy(full, prefix_buf, prefix_len);
				memcpy(full + prefix_len, body, body_len + 1U);
			}
			else
			{
				/* 分配失败：降级只打印消息体 */
				full = body;
			}
		}
		_PRINTF_FUNC("%s\n" _LOG_SUFFIX, full);
		if (full_heap)
		{
			free(full);
		}
	}
#endif

	if (body_heap)
	{
		free(body);
	}
}

void _slog_internal_print_ex(int level, unsigned int flags, const char* tag,
	const char* func_name, int line_num, const char* fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	_slog_print_ex_v(level, flags, tag, func_name, line_num, fmt, va);
	va_end(va);
}
