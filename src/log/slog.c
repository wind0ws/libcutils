#include "log/slog.h"
#include "mem/strings.h"

//extern 
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
	if (min_level < LOG_LEVEL_OFF || min_level > LOG_LEVEL_ERROR)
	{
		fprintf(stderr, "[slog] (%s:%d) invalid min_level:%d\n", __func__, __LINE__, min_level);
		return; // invalid min_level
	}
	fprintf(stderr, "[slog] (%s:%d) set new log_min_level:%d\n", __func__, __LINE__, min_level);
	_g_slog_min_level = min_level;
}

LogLevel slog_get_min_level()
{
	return _g_slog_min_level;
}

#if(!defined(_LCU_LOGGER_UNSUPPORT_STDOUT_REDIRECT) || 0 == _LCU_LOGGER_UNSUPPORT_STDOUT_REDIRECT)
#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable:4996) //for disable freopen warning
#endif // _WIN32

void slog_stdout2file(char* file_path)
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

void __slog_internal_hex_print(int level, const char* tag, const char* chars, size_t chars_count)
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
