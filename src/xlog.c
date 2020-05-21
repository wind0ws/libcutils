#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <sys/timeb.h>
#include <time.h>
#include "xlog.h"
#include "strings.h"

#define LOG_LEVLE_CHAR_V ('V')
#define LOG_LEVLE_CHAR_D ('D')
#define LOG_LEVLE_CHAR_I ('I')
#define LOG_LEVLE_CHAR_W ('W')
#define LOG_LEVLE_CHAR_E ('E')

typedef struct xlog_cb_pack
{
	xlog_user_callback cb;
	void* cb_user_data;
}xlog_cb_pack_t;

#define XLOG_DEFAULT_TAG_MAX_SIZE (32)
typedef struct xlog_config
{
	char default_tag[XLOG_DEFAULT_TAG_MAX_SIZE];
	xlog_cb_pack_t cb_pack;
	LogLevel min_level;
	LogTarget target;
}xlog_config_t;

static xlog_config_t xlog_cfg = {
	"XLog",
	{NULL},
	LOG_LEVEL_VERBOSE,
#ifdef _WIN32
	LOG_TARGET_CONSOLE
#else
	(LOG_TARGET_ANDROID | LOG_TARGET_CONSOLE) // NOLINT(hicpp-signed-bitwise)
#endif
};

#define  XLOG_IS_TARGET_ABLE(log_target) (xlog_cfg.target & log_target)
#define XLOG_IS_CONSOLE_ABLE XLOG_IS_TARGET_ABLE(LOG_TARGET_CONSOLE)
#define XLOG_IS_ANDROID_ABLE XLOG_IS_TARGET_ABLE(LOG_TARGET_ANDROID)
#define XLOG_IS_USER_CALLBACK_ABLE XLOG_IS_TARGET_ABLE(LOG_TARGET_USER_CALLBACK)

#define XLOG_IS_LOGABLE(level) (xlog_cfg.min_level && level >= xlog_cfg.min_level)

#define TIME_STR_LEN (24)
static inline void get_current_time(char str[TIME_STR_LEN])
{
	struct timeb tb;
	struct tm* lt;
	ftime(&tb);

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable: 4996)
#endif // _WIN32
	lt = localtime(&tb.time);
#ifdef _WIN32
#pragma warning(pop)
#endif // _WIN32

	str[strftime(str, TIME_STR_LEN, "%m-%d %H:%M:%S", lt)] = '\0';
	snprintf(str, TIME_STR_LEN, "%s.%03d", str, tb.millitm);
}

static inline char get_log_level_char(int level)
{
	char levelChar;
	switch (level)
	{
	case LOG_LEVEL_VERBOSE:
		levelChar = LOG_LEVLE_CHAR_V;
		break;
	case LOG_LEVEL_DEBUG:
		levelChar = LOG_LEVLE_CHAR_D;
		break;
	case LOG_LEVEL_INFO:
		levelChar = LOG_LEVLE_CHAR_I;
		break;
	case LOG_LEVEL_WARN:
		levelChar = LOG_LEVLE_CHAR_W;
		break;
	case LOG_LEVEL_ERROR:
		levelChar = LOG_LEVLE_CHAR_E;
		break;
	default:
	case LOG_LEVEL_OFF:
		levelChar = 'U';
		break;
	}
	return levelChar;
}

#if defined(__ANDROID__)

static int convert_to_android_log_level(int level)
{
	int androidLevel;
	switch (level)
	{
	case LOG_LEVEL_VERBOSE:
		androidLevel = ANDROID_LOG_VERBOSE;
		break;
	case LOG_LEVEL_DEBUG:
		androidLevel = ANDROID_LOG_DEBUG;
		break;
	case LOG_LEVEL_INFO:
		androidLevel = ANDROID_LOG_INFO;
		break;
	case LOG_LEVEL_WARN:
		androidLevel = ANDROID_LOG_WARN;
		break;
	default:
	case LOG_LEVEL_ERROR:
		androidLevel = ANDROID_LOG_ERROR;
		break;
	}
	return androidLevel;
}

#endif

void xlog_set_default_tag(char* tag)
{
	if (NULL == tag)
	{
		return;
	}
	strlcpy(xlog_cfg.default_tag, tag, XLOG_DEFAULT_TAG_MAX_SIZE);
}

void xlog_set_user_callback(xlog_user_callback user_cb, void* user_data)
{
	xlog_cfg.cb_pack.cb = user_cb;
	xlog_cfg.cb_pack.cb_user_data = user_data;
}

void xlog_set_target(LogTarget target)
{
	if (target < 0)
	{
		return;
	}
	xlog_cfg.target = target;
}

LogTarget xlog_get_target()
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

void __xlog_internal_log(LogLevel level, char* tag, char* func_name, int file_line, char* fmt, ...)
{
	va_list args;
	char buffer_log[1024] = { 0 };
	char str_time[TIME_STR_LEN];
	int header_len;
	int header_with_trace_fun_len;
	bool isLogToConsole;
	bool isLogToAndroid;
	bool isLogToUserCb;
	if (!XLOG_IS_LOGABLE(level))
	{
		return;
	}
	if (NULL == tag)
	{
		tag = xlog_cfg.default_tag;
	}

	isLogToConsole = XLOG_IS_CONSOLE_ABLE;
	isLogToAndroid = XLOG_IS_ANDROID_ABLE;
	isLogToUserCb = XLOG_IS_USER_CALLBACK_ABLE;
	if (isLogToConsole || isLogToUserCb)
	{
		char level_char = get_log_level_char(level);
		get_current_time(str_time);
		snprintf(buffer_log, sizeof(buffer_log), "[%s][%c][%s] ", str_time, level_char, tag);
		header_len = strlen(buffer_log);
	}
	else
	{
		header_len = 0;
	}
	
	if (func_name && file_line > 0)
	{
		snprintf(buffer_log, sizeof(buffer_log), "%s[%s:%d] ", buffer_log, func_name, file_line);
		header_with_trace_fun_len = strlen(buffer_log);
	}
	else
	{
		header_with_trace_fun_len = header_len;
	}
	
	va_start(args, fmt);
	vsnprintf(buffer_log + header_with_trace_fun_len, sizeof(buffer_log) - header_with_trace_fun_len, fmt, args);
	va_end(args);

	if (isLogToConsole)
	{
		CONSOLE_LOG_CONFIG_METHOD("%s"CONSOLE_LOG_CONFIG_NEW_LINE_FORMAT, buffer_log);
	}

#if defined(__ANDROID__)
	if (isLogToAndroid)
	{
		__android_log_print(convert_to_android_log_level(level), tag, buffer_log + header_len );
	}
#endif // __ANDROID__
	if (xlog_cfg.cb_pack.cb && isLogToUserCb)
	{
		xlog_cfg.cb_pack.cb(buffer_log, xlog_cfg.cb_pack.cb_user_data);
	}
}

void xlog_chars2hex(char* out_hex_str, size_t out_hex_str_capacity, const char* chars, size_t chars_len)
{
	//char out_hex_str[sizeof(char) * chars_len * 3 + 1];
	//char out_hex_str[1024] = { '\0' };
	out_hex_str[0] = '\0';
	if (chars_len * 3 > out_hex_str_capacity)
	{
		strcat(out_hex_str, "hex is truncated:");
		chars_len = out_hex_str_capacity / 3 - 6;
	}
	char hex[4] = { '\0' };
	for (size_t i = 0; i < chars_len; ++i) {
		//        printf(" %02hhx", (unsigned char)(*(chars + i)));
		snprintf(hex, 4, " %02hhx", (unsigned char)(*(chars + i)));
		//        printf(" %s",hex);
		strcat(out_hex_str, hex);
	}
	//    printf("%s\n", out_hex_str);
}

void  __xlog_hex_helper(LogLevel level, char* tag, char* chars, size_t chars_len)
{
	char hexs[1024];
	if (!XLOG_IS_LOGABLE(level))
	{
		return;
	}
	xlog_chars2hex(hexs, 1024, chars, chars_len);
	switch (level) {
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