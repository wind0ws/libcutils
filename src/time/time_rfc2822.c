#include "time/time_util.h"
#include "time/time_rfc2822.h"

static inline void time_strf(time_t *the_time, char* out_time_str, size_t out_time_str_size, const char* time_fmt)
{
	struct tm tm_time;
	out_time_str[0] = '\0';

	gmtime_r(the_time, &tm_time);
	strftime(
		out_time_str,
		out_time_str_size,
		time_fmt,
		&tm_time
	);
}

int time_rfc2822(time_t* the_time, char* out_time_str, size_t out_time_str_size)
{
	if (!out_time_str || out_time_str_size < TIME_RFC2822_STR_SIZE) 
	{
		return -1;
	}
	time_strf(the_time, out_time_str, out_time_str_size, "%a, %d %b %Y %T %z");
	return 0;
}

int time_rfc2822_utc(time_t* the_time, char* out_time_str, size_t out_time_str_size)
{
	if (!out_time_str || out_time_str_size < TIME_RFC2822_UTC_STR_SIZE) 
	{
		return -1;
	}
	time_strf(the_time, out_time_str, out_time_str_size, "%a, %d %b %Y %T UTC");
	return 0;
}
