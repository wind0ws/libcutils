#include "mem/mem_debug.h"
#include "log/xlog.h"
#include "mem/strings.h"             /* for strlcpy        */
#include "thread/portable_thread.h"  /* for portable_mutex */
#include "time/time_util.h"          /* for get time       */
#include <malloc.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

#if(defined(__ANDROID__))
#include <android/log.h>
#endif

#define XLOG_GETTID()      (int)GETTID()

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
} xlog_cb_parcel_t;

#define XLOG_DEFAULT_TAG_MAX_SIZE (24)
typedef struct xlog_config
{
	/* min log level: log level must bigger or equal than this or will be discard. */
	LogLevel min_level;
	/* if log level bigger than min_level but small than this, transform log level to this */
	LogLevel trigger_up_level;
	/* if you don't pass in the TAG, use the default TAG. */
	char default_tag[XLOG_DEFAULT_TAG_MAX_SIZE];
	/* user callback data pack */
	xlog_cb_parcel_t cb_pack;
	/* LogTarget: log target */
	int target;
	/* LogFormat: log format */
	int format;
	/* the file pointer after redirect stdout */
	FILE* fp_out;
	/* LogFlushMode */
	LogFlushMode flush_mode;
	/* for calculate locale time */
	int timezone_hour;

	struct 
	{
		bool log2android;
		bool log2console;
		bool log2usercb;
	} cache_tgt;
	struct  
	{
		bool timestamp;
		bool tag_level;
		bool tid;
	} cache_fmt;
} xlog_config_t;

#define DEFAULT_TIMEZONE_HOUR (8)
static xlog_config_t g_xlog_cfg =
{
	LOG_LEVEL_VERBOSE,
	LOG_LEVEL_OFF,
	"XLog",
	{NULL, NULL},
#if defined(__ANDROID__)
	LOG_TARGET_ANDROID,
#else
	LOG_TARGET_CONSOLE,
#endif // __ANDROID__
	(LOG_FORMAT_WITH_TIMESTAMP | LOG_FORMAT_WITH_TAG_LEVEL),
	NULL,
	LOG_FLUSH_MODE_AUTO,
	DEFAULT_TIMEZONE_HOUR,
	{
#if defined(__ANDROID__)
		.log2android = true,
		.log2console = false,
#else
		.log2android = false,
		.log2console = true,
#endif
        .log2usercb = false
	},
	{
		.timestamp = true,
		.tag_level = true,
		.tid = false,
	},
};

static portable_mutex_t g_xlog_mutex = NULL;
#define XLOG_LOCK()     do { if (g_xlog_mutex) { portable_mutex_lock(&g_xlog_mutex);} } while (0)
#define XLOG_UNLOCK()   do { if (g_xlog_mutex) { portable_mutex_unlock(&g_xlog_mutex);} } while (0)

/* ===================== P2-6: xlog_global_init 线程安全 =====================
 * 问题: 原 xlog_global_init 是 check-then-act (if(mutex)return; init), 多线程并发
 *       (例如 logger_facade_xlog.h 的 _LOG_INIT_IMPL 宏可能绕过 lcu_global_init 直接调用)
 *       会创建多份 mutex 并泄漏.
 * 方案: 用平台 once 机制初始化一个"守护锁" g_xlog_guard (创建一次, 进程生命周期内不销毁),
 *       守护锁保护真正的 g_xlog_mutex 的 init/cleanup. 这样:
 *         1. 消除 init 竞态 (once 保证守护锁只建一次);
 *         2. 保留 init→cleanup→init 的 reinit 能力 (g_xlog_mutex 可反复创建/销毁);
 *         3. 优先用轻量 call_once / InitOnceExecuteOnce (fast-path 近乎零开销),
 *            pthread_once 仅作 POSIX 兜底.
 * 三层 fallback 顺序: C11 call_once > Win32 InitOnceExecuteOnce > POSIX pthread_once. */

#if !defined(_WIN32) && defined(LCU_HAVE_C11_THREADS_H) && defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
    /* Tier 1: C11 threads.h call_once */
    #include <threads.h>
    static portable_mutex_t g_xlog_guard = NULL;
    static once_flag g_xlog_guard_once = ONCE_FLAG_INIT;
    static void pri_xlog_guard_init(void) { portable_mutex_init(&g_xlog_guard, NULL); }
    static void pri_xlog_ensure_guard(void) { call_once(&g_xlog_guard_once, pri_xlog_guard_init); }

#elif defined(_WIN32)
    /* Tier 2: Win32 InitOnceExecuteOnce */
    #include <synchapi.h>
    static portable_mutex_t g_xlog_guard = NULL;
    static INIT_ONCE g_xlog_guard_once = INIT_ONCE_STATIC_INIT;
    static BOOL CALLBACK pri_xlog_guard_init(PINIT_ONCE i, PVOID p, PVOID* c)
    {
        (void)i; (void)p; (void)c;
        portable_mutex_init(&g_xlog_guard, NULL);
        return TRUE;
    }
    static void pri_xlog_ensure_guard(void) { InitOnceExecuteOnce(&g_xlog_guard_once, pri_xlog_guard_init, NULL, NULL); }

#else
    /* Tier 3: POSIX pthread_once 兜底 (uclibc/musl/老 NDK) */
    #include <pthread.h>
    static portable_mutex_t g_xlog_guard = NULL;
    static pthread_once_t g_xlog_guard_once = PTHREAD_ONCE_INIT;
    static void pri_xlog_guard_init(void) { portable_mutex_init(&g_xlog_guard, NULL); }
    static void pri_xlog_ensure_guard(void) { pthread_once(&g_xlog_guard_once, pri_xlog_guard_init); }
#endif

int xlog_global_init()
{
	pri_xlog_ensure_guard();
	if (NULL == g_xlog_guard)
	{
		/* 守护锁创建失败 (OOM 等极端情况), 退化为原行为, 尽力而为 */
		if (g_xlog_mutex) { return 1; }
		portable_mutex_init(&g_xlog_mutex, NULL);
		return 0;
	}
	portable_mutex_lock(&g_xlog_guard);
	int ret = 1; /* 1: 已初始化(幂等) */
	if (NULL == g_xlog_mutex)
	{
		portable_mutex_init(&g_xlog_mutex, NULL);
		ret = 0;
	}
	portable_mutex_unlock(&g_xlog_guard);
	return ret;
}

int xlog_global_cleanup()
{
	if (NULL == g_xlog_guard)
	{
		if (NULL == g_xlog_mutex) { return 1; }
		portable_mutex_destroy(&g_xlog_mutex);
		g_xlog_mutex = NULL;
		return 0;
	}
	portable_mutex_lock(&g_xlog_guard);
	int ret = 1;
	if (g_xlog_mutex)
	{
		portable_mutex_destroy(&g_xlog_mutex);
		g_xlog_mutex = NULL;
		ret = 0;
	}
	portable_mutex_unlock(&g_xlog_guard);
	return ret;
}

#define _XLOG_IS_TARGET_LOGABLE(log_target) (g_xlog_cfg.target & (log_target))
#define XLOG_IS_CONSOLE_LOGABLE             _XLOG_IS_TARGET_LOGABLE(LOG_TARGET_CONSOLE)
#define XLOG_IS_ANDROID_LOGABLE             _XLOG_IS_TARGET_LOGABLE(LOG_TARGET_ANDROID)
#define XLOG_IS_USER_CALLBACK_LOGABLE       _XLOG_IS_TARGET_LOGABLE(LOG_TARGET_USER_CALLBACK)

#define XLOG_IS_LEVEL_LOGABLE(level) (g_xlog_cfg.target && g_xlog_cfg.min_level && level < LOG_LEVEL_UNKNOWN && level >= g_xlog_cfg.min_level)

static const char g_map_level_chars[] = { '0', LEVEL_CHAR_V, LEVEL_CHAR_D, LEVEL_CHAR_I, LEVEL_CHAR_W, LEVEL_CHAR_E, '?' };

#if defined(__ANDROID__)
static const int g_map_android_level_chars[] = { ANDROID_LOG_ERROR, ANDROID_LOG_VERBOSE, ANDROID_LOG_DEBUG,
			ANDROID_LOG_INFO, ANDROID_LOG_WARN, ANDROID_LOG_ERROR, ANDROID_LOG_ERROR };
#endif // __ANDROID__

#if(!defined(_LCU_LOGGER_UNSUPPORT_STDOUT_REDIRECT) || 0 == _LCU_LOGGER_UNSUPPORT_STDOUT_REDIRECT)
#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable:4996) //for disable freopen warning
#endif // _WIN32
void xlog_stdout2file(char* file_path)
{
	if (NULL == file_path || '\0' == file_path[0])
	{
		return;
	}
	XLOG_LOCK();
	if (g_xlog_cfg.fp_out)
	{
		fprintf(stderr, "[XLog] (%s:%d) warn: did you forgot to close the redirect stdout file stream!" _LOG_SUFFIX, __func__, __LINE__);
		fflush(g_xlog_cfg.fp_out);
		fclose(g_xlog_cfg.fp_out);
	}
	fflush(stdout);
	g_xlog_cfg.fp_out = freopen(file_path, "w", stdout);
	if (!g_xlog_cfg.fp_out)
	{
		fprintf(stderr, "[XLog] (%s:%d) err: failed on freopen stdout to file(%s)" _LOG_SUFFIX, __func__, __LINE__, file_path);
	}
	XLOG_UNLOCK();
}

void xlog_back2stdout()
{
	if (!g_xlog_cfg.fp_out)
	{
		return;
	}
	XLOG_LOCK();
	// must close current stream first, and then reopen it
	fflush(g_xlog_cfg.fp_out);
	fclose(g_xlog_cfg.fp_out);
	g_xlog_cfg.fp_out = NULL;
	if (!freopen(_STDOUT_NODE, "w", stdout))
	{
		fprintf(stderr, "[XLog] (%s:%d) err: failed on freopen to stdout" _LOG_SUFFIX, __func__, __LINE__);
	}
	XLOG_UNLOCK();
}
#ifdef _WIN32
#pragma warning(pop)
#endif // _WIN32
#endif // !_LCU_LOGGER_UNSUPPORT_STDOUT_REDIRECT

void xlog_auto_level_up(LogLevel trigger_level)
{
	XLOG_LOCK();
	g_xlog_cfg.trigger_up_level = trigger_level;
	XLOG_UNLOCK();
}

void xlog_set_default_tag(char* tag)
{
	if (NULL == tag)
	{
		return;
	}
	XLOG_LOCK();
	strlcpy(g_xlog_cfg.default_tag, tag, XLOG_DEFAULT_TAG_MAX_SIZE);
	XLOG_UNLOCK();
}

void xlog_set_timezone(int timezone_hour)
{
	if (timezone_hour > 12 || timezone_hour < -12)
	{
		return;
	}
	XLOG_LOCK();
	g_xlog_cfg.timezone_hour = timezone_hour;
	XLOG_UNLOCK();
}

void xlog_set_user_callback(xlog_user_callback_fn user_cb, void* user_data)
{
	XLOG_LOCK();
	g_xlog_cfg.cb_pack.cb = user_cb;
	g_xlog_cfg.cb_pack.cb_user_data = user_data;
	XLOG_UNLOCK();
}

void xlog_set_target(int target)
{
	if (target < LOG_TARGET_NONE)
	{
		return;
	}
	XLOG_LOCK();
	g_xlog_cfg.target = target;
	g_xlog_cfg.cache_tgt.log2android = XLOG_IS_ANDROID_LOGABLE;
	g_xlog_cfg.cache_tgt.log2console = XLOG_IS_CONSOLE_LOGABLE;
	g_xlog_cfg.cache_tgt.log2usercb = XLOG_IS_USER_CALLBACK_LOGABLE;
	XLOG_UNLOCK();
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
	XLOG_LOCK();
	g_xlog_cfg.min_level = min_level;
	XLOG_UNLOCK();
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
	XLOG_LOCK();
	g_xlog_cfg.format = format;
	g_xlog_cfg.cache_fmt.timestamp = (g_xlog_cfg.format & LOG_FORMAT_WITH_TIMESTAMP);
	g_xlog_cfg.cache_fmt.tag_level = (g_xlog_cfg.format & LOG_FORMAT_WITH_TAG_LEVEL);
	g_xlog_cfg.cache_fmt.tid = (g_xlog_cfg.format & LOG_FORMAT_WITH_TID);
	XLOG_UNLOCK();
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
	XLOG_LOCK();
	g_xlog_cfg.flush_mode = flush_mode;
	XLOG_UNLOCK();
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
static inline int print_level_tag(char* buffer, const LogLevel level, const char* tag)
{
	int fmt_len = 0;
	buffer[fmt_len] = ' ';
	++fmt_len;
	buffer[fmt_len] = g_map_level_chars[level];
	++fmt_len;
	buffer[fmt_len] = '/';
	++fmt_len;
#if 0
	const size_t origin_tag_len = strlen(tag);
#else
	char* tag_walker = (char *)tag;
	size_t origin_tag_len = 0;
	while (*tag_walker++ != '\0')
	{
		++origin_tag_len;
	}
#endif
	const size_t cpy_tag_len = (const size_t)origin_tag_len > (XLOG_DEFAULT_TAG_MAX_SIZE - 2) ?
		(XLOG_DEFAULT_TAG_MAX_SIZE - 2) : (const size_t)origin_tag_len; // max copy 22 chars
#if 1
	const size_t align_tag_len = (cpy_tag_len < 8U ? 8U : cpy_tag_len);
	for (size_t i = 0; i < align_tag_len; ++i)
	{
		buffer[fmt_len] = (i < cpy_tag_len ? tag[i] : ' ');
		++fmt_len;
	}
#else
	if (cpy_tag_len)
	{
		memcpy(buffer + fmt_len, tag, cpy_tag_len);
		fmt_len += cpy_tag_len;
	}
	while (cpy_tag_len++ < 8U)
	{
		buffer[fmt_len] = ' ';
		++fmt_len;
	}
#endif
	buffer[fmt_len] = '\0';
	return fmt_len;
}

static inline int print_tid(char* buffer, int tid)
{
	long long tid_value = tid;
	if (tid_value < 0)
	{
		tid_value = -tid_value; // keep formatting consistent for negative ids
	}
	buffer[0] = '(';
	++buffer;
#define MAX_TID_WIDTH (5)
	char digits[MAX_TID_WIDTH];
	size_t digit_count = 0U;
	do
	{
		if (digit_count < MAX_TID_WIDTH)
		{
			digits[digit_count] = (char)('0' + (int)(tid_value % 10LL));
			++digit_count;
		}
		tid_value /= 10LL;
	} while (tid_value);
	size_t pad_len = (digit_count < MAX_TID_WIDTH) ? (MAX_TID_WIDTH - digit_count) : 0U;
	if (pad_len)
	{
		memset(buffer, ' ', pad_len);
		buffer += pad_len;
	}
	while (digit_count)
	{
		--digit_count;
		*buffer = digits[digit_count];
		++buffer;
	}
	*buffer = ')';
	++buffer;
	*buffer = '\0';
	return (int)(MAX_TID_WIDTH + 2U);
}

static inline int my_int2str(int num, char* str)
{
	char* cursor = str;
	long long value = num;
	if (value < 0)
	{
		*cursor = '-';
		++cursor;
		value = -value;
	}
	char tmp_digits[16];
	size_t digit_count = 0U;
	do
	{
		tmp_digits[digit_count] = (char)('0' + (int)(value % 10LL));
		++digit_count;
		value /= 10LL;
	} while (value);
	while (digit_count)
	{
		--digit_count;
		*cursor = tmp_digits[digit_count];
		++cursor;
	}
	*cursor = '\0';
	return (int)(cursor - str);
}

// (func:line)  
/* P2-12: 接受 remaining_size 参数, 截断超长 func 名 (例如 C++ __PRETTY_FUNCTION__
 * 展开后可能上百字节). 修复前: 仅在调用方判断剩余空间 > 32, func 超长时栈溢出. */
static inline int print_func_line(char* buffer, size_t remaining_size, const char* func, int line_num)
{
	/* 至少需要: '(' + 至少 1 字节 func + ':' + digits(<=11) + ')' + ' ' + '\0' = 6 + digits */
	if (remaining_size < 16U) /* 太小直接放弃 */
	{
		return 0;
	}
	char *str = buffer;
	str[0] = '(';
	++str;
	remaining_size -= 1;
	size_t len_func = strlen(func);
	/* 至少给 ':d) \0' 留 16 字节余量 */
	if (len_func > remaining_size - 16U)
	{
		len_func = remaining_size - 16U;
	}
	memcpy(str, func, len_func);
	str += len_func;
	str[0] = ':';
	++str;
	str += my_int2str(line_num, str);
	str[0] = ')';
	str[1] = ' ';
	str[2] = '\0'; // this is no need, but we have good habit
	str += 2; // not include NUL terminator
	return (int)(str - buffer);
}

#endif // !USE_SNPRINTF_HEADER

PRINTF_FMT_CHK_GNUC(5, 6)
void __xlog_internal_print(LogLevel level, const char* tag, const char* func_name, int file_line,
	PRINTF_FMT_CHK_MSC const char* fmt, ...)
{
	va_list va;
	char default_buffer[DEFAULT_LOG_BUF_SIZE] = { 0 };
	size_t default_buffer_remaining_size;
	char* buffer_log = default_buffer;
	size_t buffer_log_size = sizeof(default_buffer);
	size_t buffer_strlen;
	size_t header_len = 0;
	
	if (!XLOG_IS_LEVEL_LOGABLE(level))
	{
		return;
	}

	XLOG_LOCK();
	do 
	{
		const bool is_log2console = g_xlog_cfg.cache_tgt.log2console;
		const bool is_log2usercb = g_xlog_cfg.cache_tgt.log2usercb;
		if (g_xlog_cfg.trigger_up_level && level < g_xlog_cfg.trigger_up_level)
		{
			level = g_xlog_cfg.trigger_up_level;
		}
		if (NULL == tag)
		{
			tag = g_xlog_cfg.default_tag;
		}
		if (g_xlog_cfg.format && (is_log2console || is_log2usercb))
		{
			if (g_xlog_cfg.cache_fmt.timestamp /* g_xlog_cfg.format & LOG_FORMAT_WITH_TIMESTAMP */)
			{
				header_len += time_util_get_time_str_current(buffer_log, g_xlog_cfg.timezone_hour);
			}
			if (g_xlog_cfg.cache_fmt.tag_level /* g_xlog_cfg.format & LOG_FORMAT_WITH_TAG_LEVEL */)
			{
#if(defined(USE_SNPRINTF_HEADER) && USE_SNPRINTF_HEADER)
				header_len += snprintf(buffer_log + header_len, buffer_log_size - header_len, " %c/%-8s", g_map_level_chars[level], tag);
#else
				header_len += print_level_tag(buffer_log + header_len, level, tag);
#endif // USE_SNPRINTF_HEADER
		    }
			if (g_xlog_cfg.cache_fmt.tid /* g_xlog_cfg.format & LOG_FORMAT_WITH_TID */)
			{
#if(defined(USE_SNPRINTF_HEADER) && USE_SNPRINTF_HEADER)
				header_len += snprintf(buffer_log + header_len, buffer_log_size - header_len, "(%5d)", XLOG_GETTID());
#else
				header_len += print_tid(buffer_log + header_len, XLOG_GETTID());
#endif // USE_SNPRINTF_HEADER
	        }
			if (header_len)
			{
				buffer_log[header_len] = ':';
				++header_len;
				buffer_log[header_len] = ' ';
				++header_len;
				buffer_log[header_len] = '\0'; // general speaking, no need do this, but we have good habit
				//#define XLOG_HEADER_COLON (": ")
				//memcpy(buffer_log + header_len, XLOG_HEADER_COLON, sizeof(XLOG_HEADER_COLON));
				//header_len += (sizeof(XLOG_HEADER_COLON) - 1);
			}
        }
		buffer_strlen = header_len;
		if (func_name && file_line > 0 &&
			(default_buffer_remaining_size = buffer_log_size - buffer_strlen - 1) > 32U) //we should count func_name and line number, but normally, 32 bytes is enough
		{
#if(defined(USE_SNPRINTF_HEADER) && USE_SNPRINTF_HEADER)
			buffer_strlen += snprintf(buffer_log + buffer_strlen, default_buffer_remaining_size, "(%s:%d) ", func_name, file_line);
#else
			buffer_strlen += print_func_line(buffer_log + buffer_strlen, default_buffer_remaining_size, func_name, file_line);
#endif // USE_SNPRINTF_HEADER
	    }

		default_buffer_remaining_size = buffer_log_size - buffer_strlen;
		va_start(va, fmt);
		// we get the formatted string length. ('\0' is not belong of the length)
		int ret_vsn = vsnprintf(buffer_log + buffer_strlen, default_buffer_remaining_size, fmt, va);
		va_end(va);
		if (ret_vsn < 0)
		{
			fprintf(stderr, "[XLOG] (%s:%d) failed(%d) on measure log format length" _LOG_SUFFIX, __func__, __LINE__, ret_vsn);
			break;
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
				fprintf(stderr, "[XLOG] (%s:%d) failed malloc(%zu) byte on xlog" _LOG_SUFFIX, __func__, __LINE__, buffer_log_size);
				break; // oops, out of memory
			}
			// here we only copy header to new buffer
			strlcpy(buffer_log, default_buffer, buffer_strlen + 1U);

			va_start(va, fmt);
			// vsnprintf ensure '\0' at end of string.
			ret_vsn = vsnprintf(buffer_log + buffer_strlen, need_fmt_str_size, fmt, va);
			va_end(va);
			buffer_strlen += ret_vsn;
		}

#if defined(__ANDROID__)
		if (g_xlog_cfg.cache_tgt.log2android)
		{   // android platform log_print does not need our header information, just skip it
			__android_log_print(g_map_android_level_chars[level], tag, "%s", buffer_log + header_len);
	    }
#endif // __ANDROID__

		if (is_log2console)
		{
			_PRINTF_FUNC("%s" _LOG_SUFFIX, buffer_log);
			if (g_xlog_cfg.flush_mode)
			{
				fflush(g_xlog_cfg.fp_out ? g_xlog_cfg.fp_out : stdout);
			}
		}

		if (is_log2usercb && g_xlog_cfg.cb_pack.cb)
		{
			g_xlog_cfg.cb_pack.cb(level, buffer_log, buffer_strlen + 1U, g_xlog_cfg.cb_pack.cb_user_data);
		}
	} while (0);
	XLOG_UNLOCK();

	if (buffer_log != default_buffer)
	{
		free(buffer_log);
	}
}
#ifdef _WIN32
#pragma warning(pop)
#endif // _WIN32

void __xlog_internal_hex_print(LogLevel level, const char* tag, const char* chars, size_t chars_count)
{
	char hex_str[DEFAULT_LOG_BUF_SIZE];
	if (!XLOG_IS_LEVEL_LOGABLE(level))
	{
		return;
	}
	str_char2hex(hex_str, sizeof(hex_str), chars, chars_count);
	__xlog_internal_print(level, tag, NULL, -1, "%s", hex_str);
}
