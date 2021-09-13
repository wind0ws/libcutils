#include "log/xlog.h"
#include "mem/strings.h"
#include "time/time_util.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <malloc.h>

// For gettid.
#if defined(__APPLE__)
#include "AvailabilityMacros.h"  // For MAC_OS_X_VERSION_MAX_ALLOWED
#include <stdint.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>
#elif defined(__linux__) || defined(__ANDROID__)
#include <syscall.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

#ifdef __ANDROID__
#define XLOG_GETTID()      (int)gettid()
#elif defined(__APPLE__)
#define XLOG_GETTID()      (int)syscall(SYS_thread_selfid)
#elif defined(__linux__)
#define XLOG_GETTID()      (int)syscall(__NR_gettid)
#elif defined(_WIN32)
#define XLOG_GETTID()      (int)GetCurrentThreadId()
#endif

#define DEFAULT_LOG_BUF_SIZE (512)

#define LEVEL_CHAR_V ('V')
#define LEVEL_CHAR_D ('D')
#define LEVEL_CHAR_I ('I')
#define LEVEL_CHAR_W ('W')
#define LEVEL_CHAR_E ('E')

#ifdef _WIN32
#define STDOUT_NODE ("CON")
#else
#define STDOUT_NODE ("/dev/tty")
#endif // _WIN32

#define CONSOLE_LOG_CONFIG_METHOD          printf
#define CONSOLE_LOG_CONFIG_NEW_LINE_FORMAT "\n"

typedef struct xlog_cb_pack
{
	xlog_user_callback_fn cb;
	void* cb_user_data;
}xlog_cb_pack_t;

#define XLOG_DEFAULT_TAG_MAX_SIZE (32)
typedef struct xlog_config
{
	char default_tag[XLOG_DEFAULT_TAG_MAX_SIZE];
	/* user callback data pack */
	xlog_cb_pack_t cb_pack;
	/* if level small than this, transform the current level to this */
	LogLevel trigger_up_level;
	/* the file pointer after redirect stdout */
	FILE* fp_stdout;
	/* min log level: if current level bigger or equal than this. */
	LogLevel min_level;
	/* LogTarget: log target */
	int target;
	/* LogFormat: log format */
	int format;
	/* for calculate locale time */
	int timezone_hour;
}xlog_config_t;

#define DEFAULT_TIMEZONE_HOUR (8)
static xlog_config_t g_xlog_cfg =
{
	"XLog",
	{NULL},
	LOG_LEVEL_OFF,
	NULL,
	LOG_LEVEL_VERBOSE,
#if defined(__ANDROID__)
	LOG_TARGET_ANDROID, // NOLINT(hicpp-signed-bitwise)
#else
	LOG_TARGET_CONSOLE,
#endif // __ANDROID__
	(LOG_FORMAT_WITH_TIMESTAMP | LOG_FORMAT_WITH_TAG_LEVEL),
	DEFAULT_TIMEZONE_HOUR
};

#define XLOG_IS_TARGET_LOGABLE(log_target) (g_xlog_cfg.target & log_target)
#define XLOG_IS_CONSOLE_LOGABLE XLOG_IS_TARGET_LOGABLE(LOG_TARGET_CONSOLE)
#define XLOG_IS_ANDROID_LOGABLE XLOG_IS_TARGET_LOGABLE(LOG_TARGET_ANDROID)
#define XLOG_IS_USER_CALLBACK_LOGABLE XLOG_IS_TARGET_LOGABLE(LOG_TARGET_USER_CALLBACK)

#define XLOG_IS_LOGABLE(level) (g_xlog_cfg.min_level && level < LOG_LEVEL_UNKNOWN && level >= g_xlog_cfg.min_level)

static char g_map_level_chars[] = {'0', LEVEL_CHAR_V, LEVEL_CHAR_D, LEVEL_CHAR_I, LEVEL_CHAR_W, LEVEL_CHAR_E, '?'};

#if defined(__ANDROID__)
static int g_map_android_level[] = { ANDROID_LOG_ERROR, ANDROID_LOG_VERBOSE, ANDROID_LOG_DEBUG,
			ANDROID_LOG_INFO, ANDROID_LOG_WARN, ANDROID_LOG_ERROR, ANDROID_LOG_ERROR};
#endif // __ANDROID__

void xlog_auto_level_up(LogLevel trigger_level)
{
	g_xlog_cfg.trigger_up_level = trigger_level;
}

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable:4996) //for disable freopen warning
#endif // _WIN32
void xlog_stdout2file(char* file_path)
{
	if (NULL == file_path)
	{
		return;
	}
	if (g_xlog_cfg.fp_stdout)
	{
		printf("[XLog] [%s:%d] WARN: did you forgot to close stdout file stream?", __func__, __LINE__);
		fflush(g_xlog_cfg.fp_stdout);
		fclose(g_xlog_cfg.fp_stdout);
	}
	fflush(stdout);
	g_xlog_cfg.fp_stdout = freopen(file_path, "w", stdout);
	if (!g_xlog_cfg.fp_stdout)
	{
		printf("[XLog] [%s:%d] Error: failed on freopen to file(%s)\n", __func__, __LINE__, file_path);
	}
}

void xlog_back2stdout()
{
	if (NULL == g_xlog_cfg.fp_stdout)
	{
		return;
	}
	// must close current stream first, and then reopen it
	fflush(g_xlog_cfg.fp_stdout);
	fclose(g_xlog_cfg.fp_stdout);
	g_xlog_cfg.fp_stdout = NULL;
	if (!freopen(STDOUT_NODE, "w", stdout))
	{
		printf("[XLog] [%s:%d] Error: failed on freopen to stdout\n", __func__, __LINE__);
	}
}
#ifdef _WIN32
#pragma warning(pop)
#endif // _WIN32

void xlog_set_default_tag(char* tag)
{
	if (NULL == tag)
	{
		return;
	}
	strlcpy(g_xlog_cfg.default_tag, tag, XLOG_DEFAULT_TAG_MAX_SIZE);
}

void xlog_set_timezone(int timezone_hour)
{
	if (timezone_hour > 12 || timezone_hour < -12)
	{
		return;
	}
	g_xlog_cfg.timezone_hour = timezone_hour;
}

void xlog_set_user_callback(xlog_user_callback_fn user_cb, void* user_data)
{
	g_xlog_cfg.cb_pack.cb = user_cb;
	g_xlog_cfg.cb_pack.cb_user_data = user_data;
}

void xlog_set_target(int target)
{
	if (target < LOG_TARGET_NONE)
	{
		return;
	}
	g_xlog_cfg.target = target;
}

int xlog_get_target()
{
	return g_xlog_cfg.target;
}

void xlog_set_min_level(LogLevel min_level)
{
	if (min_level < LOG_LEVEL_OFF || min_level > LOG_LEVEL_ERROR)
	{
		return;
	}
	g_xlog_cfg.min_level = min_level;
}

LogLevel xlog_get_min_level()
{
	return g_xlog_cfg.min_level;
}

void xlog_set_format(int format)
{
	if (format < LOG_FORMAT_RAW)
	{
		return;
	}
	g_xlog_cfg.format = format;
}

int xlog_get_format()
{
	return g_xlog_cfg.format;
}

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable:6386) //for disable buffer overflow
#endif // _WIN32
void __xlog_internal_print(LogLevel level, char* tag, const char* func_name, int file_line, char* fmt, ...)
{
	va_list va;
	char default_buffer[DEFAULT_LOG_BUF_SIZE] = { 0 };
	size_t default_buffer_remaining_size;
	char* buffer_log = default_buffer;
	size_t buffer_log_size = sizeof(default_buffer);
	size_t buffer_strlen;
	size_t header_len = 0;
	bool is_log2console;
	bool is_log2usercb;
	if (!XLOG_IS_LOGABLE(level))
	{
		return;
	}
	if (g_xlog_cfg.trigger_up_level && level < g_xlog_cfg.trigger_up_level)
	{
		level = g_xlog_cfg.trigger_up_level;
	}
	if (NULL == tag)
	{
		tag = g_xlog_cfg.default_tag;
	}

	is_log2console = XLOG_IS_CONSOLE_LOGABLE;
	is_log2usercb = XLOG_IS_USER_CALLBACK_LOGABLE;
	if (g_xlog_cfg.format && (is_log2console || is_log2usercb))
	{
		if (g_xlog_cfg.format & LOG_FORMAT_WITH_TIMESTAMP)
		{
			header_len += time_util_get_time_str_current(buffer_log, g_xlog_cfg.timezone_hour);
		}
		if (g_xlog_cfg.format & LOG_FORMAT_WITH_TAG_LEVEL)
		{
			header_len += snprintf(buffer_log + header_len, buffer_log_size - header_len, " %c/%-8s", g_map_level_chars[level], tag);
		}
		if (g_xlog_cfg.format & LOG_FORMAT_WITH_TID)
		{
			header_len += snprintf(buffer_log + header_len, buffer_log_size - header_len, "(%5d)", XLOG_GETTID());
		}
		if (header_len)
		{
#define XLOG_HEADER_COLON (": ")
			memcpy(buffer_log + header_len, XLOG_HEADER_COLON, sizeof(XLOG_HEADER_COLON));
			header_len += (sizeof(XLOG_HEADER_COLON) - 1);
		}
	}
	buffer_strlen = header_len;
	if (func_name && file_line > 0 && (default_buffer_remaining_size = buffer_log_size - buffer_strlen) > 0)
	{
		buffer_strlen += snprintf(buffer_log + buffer_strlen, default_buffer_remaining_size, "(%s:%d) ", func_name, file_line);
	}
	
	if ((default_buffer_remaining_size = buffer_log_size - buffer_strlen) > 0)
	{
		va_start(va, fmt);
		//first we get the formatted string length. ('\0' is not belong of the length)
		size_t need_fmt_str_size = vsnprintf(NULL, 0, fmt, va) + 1;
		va_end(va);
		if (default_buffer_remaining_size < need_fmt_str_size)
		{
			buffer_log_size = buffer_strlen + need_fmt_str_size;
			buffer_log = (char *)malloc(buffer_log_size);
			if (!buffer_log)
			{
				return; // oops, out of memory
			}
			strlcpy(buffer_log, default_buffer, buffer_log_size);
		}

		va_start(va, fmt);
		buffer_strlen += vsnprintf(buffer_log + buffer_strlen, need_fmt_str_size, fmt, va);
		va_end(va);
	}
	
	/*if (buffer_strlen == sizeof(buffer_log))
	{
		buffer_log[--buffer_strlen] = '\0'; // vsnprintf ensure '\0' in string.
	}*/

	if (is_log2console)
	{
		CONSOLE_LOG_CONFIG_METHOD("%s"CONSOLE_LOG_CONFIG_NEW_LINE_FORMAT, buffer_log);
	}

#if defined(__ANDROID__)
	if (XLOG_IS_ANDROID_LOGABLE)
	{   // android log no need our own log header, just skip it
		__android_log_print(g_map_android_level[level], tag, "%s", buffer_log + header_len);
	}
#endif // __ANDROID__

	if (is_log2usercb && g_xlog_cfg.cb_pack.cb)
	{
		g_xlog_cfg.cb_pack.cb(buffer_log, buffer_strlen + 1, g_xlog_cfg.cb_pack.cb_user_data);
	}

	if (buffer_log != default_buffer)
	{
		free(buffer_log);
	}
}
#ifdef _WIN32
#pragma warning(pop)
#endif // _WIN32

void xlog_chars2hex(char* out_hex_str, size_t out_hex_str_capacity, const char* chars, size_t chars_size)
{
	out_hex_str[0] = '\0';
	size_t header_len = 0;
	if (chars_size * 3 > out_hex_str_capacity)
	{
		snprintf(out_hex_str, out_hex_str_capacity, "hex is truncated(%zu):", chars_size);
		chars_size = out_hex_str_capacity / 3 - 6;
		header_len = strnlen(out_hex_str, out_hex_str_capacity);
	}
	out_hex_str_capacity -= header_len;
	for (size_t chars_index = 0, str_offset = 0; chars_index < chars_size; ++chars_index, str_offset += 3)
	{
		snprintf(out_hex_str + header_len + str_offset, out_hex_str_capacity - str_offset, " %02hhx", (unsigned char)chars[chars_index]);
	}
}

void __xlog_internal_hex_print(LogLevel level, char* tag, char* chars, size_t chars_size)
{
	char hex_str[DEFAULT_LOG_BUF_SIZE];
	if (!XLOG_IS_LOGABLE(level))
	{
		return;
	}
	xlog_chars2hex(hex_str, sizeof(hex_str), chars, chars_size);
	__xlog_internal_print(level, tag, NULL, -1, "%s", hex_str);
}
