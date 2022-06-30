#include "log/xlog.h"
#include "mem/strings.h"     /* for strlcpy  */
#include "time/time_util.h"  /* for get time */
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
#include <android/log.h>
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

typedef struct xlog_cb_pack
{
	xlog_user_callback_fn cb;
	void* cb_user_data;
}xlog_cb_pack_t;

#define XLOG_DEFAULT_TAG_MAX_SIZE (24)
typedef struct xlog_config
{
	char default_tag[XLOG_DEFAULT_TAG_MAX_SIZE];
	/* user callback data pack */
	xlog_cb_pack_t cb_pack;
	/* if log level bigger than min_level but small than this, transform log level to this */
	LogLevel trigger_up_level;
	/* the file pointer after redirect stdout */
	FILE* fp_out;
	/* min log level: if current level bigger or equal than this. */
	LogLevel min_level;
	/* LogTarget: log target */
	int target;
	/* LogFormat: log format */
	int format;
	/* LogFlushMode */
	LogFlushMode flush_mode;
	/* for calculate locale time */
	int timezone_hour;
} xlog_config_t;

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
	LOG_FLUSH_MODE_AUTO,
	DEFAULT_TIMEZONE_HOUR
};

#define XLOG_IS_TARGET_LOGABLE(log_target) (g_xlog_cfg.target & log_target)
#define XLOG_IS_CONSOLE_LOGABLE XLOG_IS_TARGET_LOGABLE(LOG_TARGET_CONSOLE)
#define XLOG_IS_ANDROID_LOGABLE XLOG_IS_TARGET_LOGABLE(LOG_TARGET_ANDROID)
#define XLOG_IS_USER_CALLBACK_LOGABLE XLOG_IS_TARGET_LOGABLE(LOG_TARGET_USER_CALLBACK)

#define XLOG_IS_LEVEL_LOGABLE(level) (g_xlog_cfg.target && g_xlog_cfg.min_level && level < LOG_LEVEL_UNKNOWN && level >= g_xlog_cfg.min_level)

static char g_map_level_chars[] = {'0', LEVEL_CHAR_V, LEVEL_CHAR_D, LEVEL_CHAR_I, LEVEL_CHAR_W, LEVEL_CHAR_E, '?'};

#if defined(__ANDROID__)
static int g_map_android_level[] = { ANDROID_LOG_ERROR, ANDROID_LOG_VERBOSE, ANDROID_LOG_DEBUG,
			ANDROID_LOG_INFO, ANDROID_LOG_WARN, ANDROID_LOG_ERROR, ANDROID_LOG_ERROR};
#endif // __ANDROID__

#if(!defined(_LCU_LOGGER_UNSUPPORT_PRINTF_REDIRECT) || 0 == _LCU_LOGGER_UNSUPPORT_PRINTF_REDIRECT)
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
	if (g_xlog_cfg.fp_out && (g_xlog_cfg.fp_out != stdout))
	{
		fprintf(stderr, "[XLog] (%s:%d) warn: did you forgot to close the redirect stdout file stream!\n", __func__, __LINE__);
		fflush(g_xlog_cfg.fp_out);
		fclose(g_xlog_cfg.fp_out);
	}
	fflush(stdout);
	g_xlog_cfg.fp_out = freopen(file_path, "w", stdout);
	if (!g_xlog_cfg.fp_out)
	{
		fprintf(stderr, "[XLog] (%s:%d) err: failed on freopen to file(%s)\n", __func__, __LINE__, file_path);
	}
}

void xlog_back2stdout()
{
	if (!g_xlog_cfg.fp_out)
	{
		return;
	}
	// must close current stream first, and then reopen it
	fflush(g_xlog_cfg.fp_out);
	fclose(g_xlog_cfg.fp_out);
	g_xlog_cfg.fp_out = NULL;
	if (!freopen(_STDOUT_NODE, "w", stdout))
	{
		fprintf(stderr, "[XLog] (%s:%d) err: failed on freopen to stdout\n", __func__, __LINE__);
	}
}
#ifdef _WIN32
#pragma warning(pop)
#endif // _WIN32
#endif // !_LCU_LOGGER_UNSUPPORT_PRINTF_REDIRECT

void xlog_auto_level_up(LogLevel trigger_level)
{
	g_xlog_cfg.trigger_up_level = trigger_level;
}

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

void xlog_set_flush_mode(LogFlushMode flush_mode)
{
	if (flush_mode < LOG_FLUSH_MODE_AUTO)
	{
		return;
	}
	g_xlog_cfg.flush_mode = flush_mode;
}

LogFlushMode xlog_get_flush_mode()
{
	return g_xlog_cfg.flush_mode;
}

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable:6386) //for disable buffer overflow
#endif // _WIN32

//our print header func is more efficiently
#define USE_SNPRINTF_HEADER (0)

#if(!defined(USE_SNPRINTF_HEADER) || !USE_SNPRINTF_HEADER)
static inline int print_level_tag(char* buffer, LogLevel level, char* tag)
{
	int fmt_len = 0;
	buffer[fmt_len++] = ' ';
	buffer[fmt_len++] = g_map_level_chars[level];
	buffer[fmt_len++] = '/';
	size_t origin_tag_len = strlen(tag);
	size_t cpy_tag_len = origin_tag_len > (XLOG_DEFAULT_TAG_MAX_SIZE - 2) ?
		(XLOG_DEFAULT_TAG_MAX_SIZE - 2) : origin_tag_len; // max copy 22 chars
#if 1
	size_t align_tag_len = (cpy_tag_len < 8U ? 8U : cpy_tag_len);
	for (size_t i = 0; i < align_tag_len; ++i)
	{
		buffer[fmt_len++] = (i < cpy_tag_len ? tag[i] : ' ');
	}
#else
	if (cpy_tag_len)
	{
		memcpy(buffer + fmt_len, tag, cpy_tag_len);
		fmt_len += cpy_tag_len;
	}
	while (cpy_tag_len++ < 8U)
	{
		buffer[fmt_len++] = ' ';
	}
#endif
	buffer[fmt_len] = '\0';
	return fmt_len;
}

static inline int print_tid(char* buffer, int tid)
{
	buffer[0] = '(';
	++buffer;
#define MAX_TID_WIDTH (5)
	int num_count = MAX_TID_WIDTH;
	for (; num_count > 0 && tid > 0; --num_count, tid /= 10)
	{
		buffer[num_count - 1] = '0' + tid % 10;
	}
	if (num_count)
	{
		memset(buffer, ' ', num_count);
	}
	buffer += MAX_TID_WIDTH;
	buffer[0] = ')';
	buffer[1] = '\0';
	return 7;
}

static inline int int2str(int num, char* str)
{
	int len_str = 0;
	if (num < 0)
	{
		num = -num;
		str[len_str++] = '-';
	}
	do
	{
		str[len_str++] = num % 10 + '0';
		num /= 10;
	} while (num);
	str[len_str] = '\0';

	int index_swap = 0;
	if ('-' == str[0])
	{
		index_swap = 1;
		++len_str;
	}
	for (; index_swap < len_str / 2; ++index_swap)
	{
		str[index_swap] = str[index_swap] + str[len_str - 1 - index_swap];
		str[len_str - 1 - index_swap] = str[index_swap] - str[len_str - 1 - index_swap];
		str[index_swap] = str[index_swap] - str[len_str - 1 - index_swap];
	}

	return len_str;
}

//(func:line) 
static inline int print_func_line(char* buffer, const char* func, int line_num)
{
	char *str = buffer;
	str[0] = '(';
	++str;
	int len_func = strlen(func);
	memcpy(str, func, len_func);
	str += len_func;
	str[0] = ':';
	++str;
	str += int2str(line_num, str);
	str[0] = ')';
	str[1] = ' ';
	str[2] = '\0'; // this is no need, but we have good habit
	str += 2; // not include NUL terminator
	return (int)(str - buffer);
}

#endif // !USE_SNPRINTF_HEADER

void __xlog_internal_print(LogLevel level, char* tag, const char* func_name, int file_line, char* fmt, ...)
{
	va_list va;
	//no need init buffer.
	char default_buffer[DEFAULT_LOG_BUF_SIZE];
	size_t default_buffer_remaining_size;
	char* buffer_log = default_buffer;
	size_t buffer_log_size = sizeof(default_buffer);
	size_t buffer_strlen;
	size_t header_len = 0;
	bool is_log2console;
	bool is_log2usercb;
	if (LOG_TARGET_NONE == g_xlog_cfg.target)
	{
		return;
	}
	if (!XLOG_IS_LEVEL_LOGABLE(level))
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
#if(defined(USE_SNPRINTF_HEADER) && USE_SNPRINTF_HEADER)
			header_len += snprintf(buffer_log + header_len, buffer_log_size - header_len, " %c/%-8s", g_map_level_chars[level], tag);
#else
			header_len += print_level_tag(buffer_log + header_len, level, tag);
#endif // USE_SNPRINTF_HEADER
		}
		if (g_xlog_cfg.format & LOG_FORMAT_WITH_TID)
		{
#if(defined(USE_SNPRINTF_HEADER) && USE_SNPRINTF_HEADER)
			header_len += snprintf(buffer_log + header_len, buffer_log_size - header_len, "(%5d)", XLOG_GETTID());
#else
			header_len += print_tid(buffer_log + header_len, XLOG_GETTID());
#endif // USE_SNPRINTF_HEADER
		}
		if (header_len)
		{
			buffer_log[header_len++] = ':';
			buffer_log[header_len++] = ' ';
			buffer_log[header_len] = '\0'; // general speaking, no need do this, but we have good habit
//#define XLOG_HEADER_COLON (": ")
			//memcpy(buffer_log + header_len, XLOG_HEADER_COLON, sizeof(XLOG_HEADER_COLON));
			//header_len += (sizeof(XLOG_HEADER_COLON) - 1);
		}
	}
	buffer_strlen = header_len;
	if (func_name && file_line > 0 && 
		(default_buffer_remaining_size = buffer_log_size - buffer_strlen - 1) > 20) //normally, buf is enough
	{
#if(defined(USE_SNPRINTF_HEADER) && USE_SNPRINTF_HEADER)
		buffer_strlen += snprintf(buffer_log + buffer_strlen, default_buffer_remaining_size, "(%s:%d) ", func_name, file_line);
#else
		buffer_strlen += print_func_line(buffer_log + buffer_strlen, func_name, file_line);
#endif // USE_SNPRINTF_HEADER
	}
	
	default_buffer_remaining_size = buffer_log_size - buffer_strlen;
	va_start(va, fmt);
	// we get the formatted string length. ('\0' is not belong of the length)
	int ret_vsn = vsnprintf(buffer_log + buffer_strlen, default_buffer_remaining_size, fmt, va);
	va_end(va);
	if (ret_vsn < 0)
	{
		fprintf(stderr, "[XLOG] (%s:%d) failed(%d) on measure log format length\n", __func__, __LINE__, ret_vsn);
		return;
	}
	if (ret_vsn < (int)default_buffer_remaining_size)
	{
		buffer_strlen += ret_vsn;
	}
	else
	{
		const size_t need_fmt_str_size = (size_t)ret_vsn + 1U;
		buffer_log_size = buffer_strlen + need_fmt_str_size;
		if (!(buffer_log = (char*)malloc(buffer_log_size)))
		{
			fprintf(stderr, "[XLOG] (%s:%d) failed malloc %zu byte on xlog\n", __func__, __LINE__, buffer_log_size);
			return; // oops, out of memory
		} 
		// here we only copy header to new buffer
		strlcpy(buffer_log, default_buffer, buffer_strlen + 1U); 
		
		va_start(va, fmt);
		// vsnprintf ensure '\0' in string.
		ret_vsn = vsnprintf(buffer_log + buffer_strlen, need_fmt_str_size, fmt, va);
		va_end(va);
		buffer_strlen += ret_vsn;
	}

#if defined(__ANDROID__)
	if (XLOG_IS_ANDROID_LOGABLE)
	{   // android log no need our own log header, just skip it
		__android_log_print(g_map_android_level[level], tag, "%s", buffer_log + header_len);
	}
#endif // __ANDROID__

	if (is_log2console)
	{
		_PRINTF_FUNC("%s" _SUFFIX_LOG, buffer_log);
		if (g_xlog_cfg.flush_mode)
		{
			fflush(g_xlog_cfg.fp_out ? g_xlog_cfg.fp_out : stdout);
		}
	}

	if (is_log2usercb && g_xlog_cfg.cb_pack.cb)
	{
		g_xlog_cfg.cb_pack.cb(level, buffer_log, buffer_strlen + 1U, g_xlog_cfg.cb_pack.cb_user_data);
	}

	if (buffer_log != default_buffer)
	{
		free(buffer_log);
	}
}
#ifdef _WIN32
#pragma warning(pop)
#endif // _WIN32

void __xlog_internal_hex_print(LogLevel level, char* tag, char* chars, size_t chars_count)
{
	char hex_str[DEFAULT_LOG_BUF_SIZE];
	if (!XLOG_IS_LEVEL_LOGABLE(level))
	{
		return;
	}
	str_char2hex(hex_str, sizeof(hex_str), chars, chars_count);
	__xlog_internal_print(level, tag, NULL, -1, "%s", hex_str);
}
