#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include "log/xlog.h"
#include "mem/strings.h"
#include "time/time_util.h"

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

#define LOG_LEVLE_CHAR_V ('V')
#define LOG_LEVLE_CHAR_D ('D')
#define LOG_LEVLE_CHAR_I ('I')
#define LOG_LEVLE_CHAR_W ('W')
#define LOG_LEVLE_CHAR_E ('E')

#ifdef _WIN32
#define STDOUT ("CON")
#else
#define STDOUT ("/dev/tty")
#endif // _WIN32

#define CONSOLE_LOG_CONFIG_METHOD printf
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
static xlog_config_t xlog_cfg =
{
	"XLog",
	{NULL},
	LOG_LEVEL_OFF,
	NULL,
	LOG_LEVEL_VERBOSE,
#ifdef _WIN32
	LOG_TARGET_CONSOLE,
#else
	(LOG_TARGET_ANDROID | LOG_TARGET_CONSOLE), // NOLINT(hicpp-signed-bitwise)
#endif // _WIN32
	(LOG_FORMAT_WITH_TIMESTAMP | LOG_FORMAT_WITH_TAG_LEVEL),
	DEFAULT_TIMEZONE_HOUR
};

#define XLOG_IS_TARGET_LOGABLE(log_target) (xlog_cfg.target & log_target)
#define XLOG_IS_CONSOLE_LOGABLE XLOG_IS_TARGET_LOGABLE(LOG_TARGET_CONSOLE)
#define XLOG_IS_ANDROID_LOGABLE XLOG_IS_TARGET_LOGABLE(LOG_TARGET_ANDROID)
#define XLOG_IS_USER_CALLBACK_LOGABLE XLOG_IS_TARGET_LOGABLE(LOG_TARGET_USER_CALLBACK)

#define XLOG_IS_LOGABLE(level) (xlog_cfg.min_level && level >= xlog_cfg.min_level)

static inline char get_log_level_char(int level)
{
	char log_level_char;
	switch (level)
	{
	case LOG_LEVEL_VERBOSE:
		log_level_char = LOG_LEVLE_CHAR_V;
		break;
	case LOG_LEVEL_DEBUG:
		log_level_char = LOG_LEVLE_CHAR_D;
		break;
	case LOG_LEVEL_INFO:
		log_level_char = LOG_LEVLE_CHAR_I;
		break;
	case LOG_LEVEL_WARN:
		log_level_char = LOG_LEVLE_CHAR_W;
		break;
	case LOG_LEVEL_ERROR:
		log_level_char = LOG_LEVLE_CHAR_E;
		break;
	default:
	case LOG_LEVEL_OFF:
		log_level_char = 'U';
		break;
	}
	return log_level_char;
}

#if defined(__ANDROID__)
static inline int convert_to_android_log_level(int level)
{
	int android_log_level;
	switch (level)
	{
	case LOG_LEVEL_VERBOSE:
		android_log_level = ANDROID_LOG_VERBOSE;
		break;
	case LOG_LEVEL_DEBUG:
		android_log_level = ANDROID_LOG_DEBUG;
		break;
	case LOG_LEVEL_INFO:
		android_log_level = ANDROID_LOG_INFO;
		break;
	case LOG_LEVEL_WARN:
		android_log_level = ANDROID_LOG_WARN;
		break;
	case LOG_LEVEL_ERROR:
	default:
		android_log_level = ANDROID_LOG_ERROR;
		break;
	}
	return android_log_level;
}
#endif // __ANDROID__

void xlog_auto_level_up(LogLevel trigger_level)
{
	xlog_cfg.trigger_up_level = trigger_level;
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
	if (xlog_cfg.fp_stdout)
	{
		printf("[XLog] [%s:%d] WARN: did you forgot to close stdout file stream?", __func__, __LINE__);
		fclose(xlog_cfg.fp_stdout);
	}
	xlog_cfg.fp_stdout = freopen(file_path, "w", stdout);
	if (!xlog_cfg.fp_stdout)
	{
		printf("[XLog] [%s:%d] Error: failed on freopen to file(%s)\n", __func__, __LINE__, file_path);
	}
}

void xlog_back2stdout()
{
	if (NULL == xlog_cfg.fp_stdout)
	{
		return;
	}
	// must close current stream first, and then reopen it
	fclose(xlog_cfg.fp_stdout);
	xlog_cfg.fp_stdout = NULL;
	if (!freopen(STDOUT, "w", stdout))
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
	strlcpy(xlog_cfg.default_tag, tag, XLOG_DEFAULT_TAG_MAX_SIZE);
}

void xlog_set_timezone(int timezone_hour)
{
	if (timezone_hour > 12 || timezone_hour < -12)
	{
		return;
	}
	xlog_cfg.timezone_hour = timezone_hour;
}

void xlog_set_user_callback(xlog_user_callback_fn user_cb, void* user_data)
{
	xlog_cfg.cb_pack.cb = user_cb;
	xlog_cfg.cb_pack.cb_user_data = user_data;
}

void xlog_set_target(int target)
{
	xlog_cfg.target = target;
}

int xlog_get_target()
{
	return xlog_cfg.target;
}

void xlog_set_min_level(LogLevel min_level)
{
	if (min_level < LOG_LEVEL_OFF || min_level > LOG_LEVEL_ERROR)
	{
		return;
	}
	xlog_cfg.min_level = min_level;
}

LogLevel xlog_get_min_level()
{
	return xlog_cfg.min_level;
}

void xlog_set_format(int format)
{
	xlog_cfg.format = format;
}

int xlog_get_format()
{
	return xlog_cfg.format;
}

void __xlog_internal_log(LogLevel level, char* tag, const char* func_name, int file_line, char* fmt, ...)
{
	va_list args;
	char buffer_log[1024] = { 0 };
	int header_len = 0;
	int header_with_trace_fun_len;
	bool is_log2console;
	bool is_log2usercb;
	if (!XLOG_IS_LOGABLE(level))
	{
		return;
	}
	if (xlog_cfg.trigger_up_level && level < xlog_cfg.trigger_up_level)
	{
		level = xlog_cfg.trigger_up_level;
	}
	if (NULL == tag)
	{
		tag = xlog_cfg.default_tag;
	}

	is_log2console = XLOG_IS_CONSOLE_LOGABLE;
	is_log2usercb = XLOG_IS_USER_CALLBACK_LOGABLE;
	if (xlog_cfg.format && (is_log2console || is_log2usercb))
	{
		if (xlog_cfg.format & LOG_FORMAT_WITH_TIMESTAMP)
		{
			time_util_get_current_time_str(buffer_log, xlog_cfg.timezone_hour);
			header_len = (int)strnlen(buffer_log, TIME_STR_LEN);
		}
		if (xlog_cfg.format & LOG_FORMAT_WITH_TAG_LEVEL)
		{
			snprintf(buffer_log + header_len, sizeof(buffer_log) - header_len, " %c/%-8s", get_log_level_char(level), tag);
			header_len += (int)strlen(buffer_log + header_len);
		}
		if (xlog_cfg.format & LOG_FORMAT_WITH_TID)
		{
			snprintf(buffer_log + header_len, sizeof(buffer_log) - header_len, "(%5d)", XLOG_GETTID());
			header_len += (int)strlen(buffer_log + header_len);
		}
		strcpy(buffer_log + header_len, ": ");
		header_len += 2;
	}

	if (func_name && file_line > 0)
	{
		snprintf(buffer_log + header_len, sizeof(buffer_log) - header_len, "(%s:%d) ", func_name, file_line);
		header_with_trace_fun_len = header_len + (int)strlen(buffer_log + header_len);
	}
	else
	{
		header_with_trace_fun_len = header_len;
	}

	va_start(args, fmt);
	vsnprintf(buffer_log + header_with_trace_fun_len, sizeof(buffer_log) - header_with_trace_fun_len, fmt, args);
	va_end(args);

	if (is_log2console)
	{
		CONSOLE_LOG_CONFIG_METHOD("%s"CONSOLE_LOG_CONFIG_NEW_LINE_FORMAT, buffer_log);
	}

#if defined(__ANDROID__)
	if (XLOG_IS_ANDROID_LOGABLE)
	{
		__android_log_print(convert_to_android_log_level(level), tag, "%s", buffer_log + header_len);
	}
#endif // __ANDROID__

	if (is_log2usercb && xlog_cfg.cb_pack.cb)
	{
		xlog_cfg.cb_pack.cb(buffer_log, xlog_cfg.cb_pack.cb_user_data);
	}
}

void xlog_chars2hex(char* out_hex_str, size_t out_hex_str_capacity, const char* chars, size_t chars_len)
{
	out_hex_str[0] = '\0';
	size_t header_len = 0;
	if (chars_len * 3 > out_hex_str_capacity)
	{
		snprintf(out_hex_str, out_hex_str_capacity, "hex is truncated(%zu):", chars_len);
		chars_len = out_hex_str_capacity / 3 - 6;
		header_len = strnlen(out_hex_str, out_hex_str_capacity);
	}
	out_hex_str_capacity -= header_len;
	for (size_t chars_index = 0, str_offset = 0; chars_index < chars_len; ++chars_index, str_offset += 3)
	{
		snprintf(out_hex_str + header_len + str_offset, out_hex_str_capacity - str_offset, " %02hhx", (unsigned char)chars[chars_index]);
	}
}

void __xlog_internal_hex_helper(LogLevel level, char* tag, char* chars, size_t chars_len)
{
	char hexs[1024];
	if (!XLOG_IS_LOGABLE(level))
	{
		return;
	}
	xlog_chars2hex(hexs, sizeof(hexs), chars, chars_len);
	switch (level)
	{
	case LOG_LEVEL_VERBOSE:
		TLOGV(tag, "%s", hexs);
		break;
	case LOG_LEVEL_DEBUG:
		TLOGD(tag, "%s", hexs);
		break;
	case LOG_LEVEL_INFO:
		TLOGI(tag, "%s", hexs);
		break;
	case LOG_LEVEL_WARN:
		TLOGW(tag, "%s", hexs);
		break;
	default:
	case LOG_LEVEL_ERROR:
		TLOGE(tag, "%s", hexs);
		break;
	}
}
