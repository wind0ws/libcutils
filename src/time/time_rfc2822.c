#include "time/time_util.h"
#include "time/time_rfc2822.h"
#include <string.h>

static inline int time_strf(time_t *the_time, char* out_time_str, size_t out_time_str_size, const char* time_fmt)
{
	struct tm tm_time;
	out_time_str[0] = '\0';

	/* P2-3: 检查 gmtime_r 失败, 避免未初始化的 tm 喂给 strftime. */
	memset(&tm_time, 0, sizeof(tm_time));
	if (NULL == gmtime_r(the_time, &tm_time))
	{
		return -2;
	}
	strftime(
		out_time_str,
		out_time_str_size,
		time_fmt,
		&tm_time
	);
	return 0;
}

int time_rfc2822(time_t* the_time, char* out_time_str, size_t out_time_str_size)
{
	if (!out_time_str || out_time_str_size < TIME_RFC2822_STR_SIZE)
	{
		return -1;
	}
	return time_strf(the_time, out_time_str, out_time_str_size, "%a, %d %b %Y %T %z");
}

int time_rfc2822_utc(time_t* the_time, char* out_time_str, size_t out_time_str_size)
{
	if (!out_time_str || out_time_str_size < TIME_RFC2822_UTC_STR_SIZE)
	{
		return -1;
	}
	return time_strf(the_time, out_time_str, out_time_str_size, "%a, %d %b %Y %T UTC");
}
