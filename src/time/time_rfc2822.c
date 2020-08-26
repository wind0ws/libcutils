#include "time/time_util.h"
#include "time/time_rfc2822.h"

static void time_now_strf(char* out_time_str, size_t out_time_str_len, const char* timeFmt) 
{
	struct tm tm_time;
	time_t current;
	out_time_str[0] = '\0';

	time(&current);
	gmtime_r(&current, &tm_time);
	strftime(
		out_time_str,
		out_time_str_len,
		timeFmt,
		&tm_time
	);
}

int time_rfc2822_now(char* out_time_str, size_t out_time_str_len) 
{
	if (!out_time_str || out_time_str_len <= TIME_RFC2822_STR_LEN) 
	{
		return -1;
	}
	time_now_strf(out_time_str, out_time_str_len, "%a, %d %b %Y %T %z");
	return 0;
}

int time_rfc2822_utc_now(char* out_time_str, size_t out_time_str_len) 
{
	if (!out_time_str || out_time_str_len <= TIME_RFC2822_UTC_STR_LEN) 
	{
		return -1;
	}
	time_now_strf(out_time_str, out_time_str_len, "%a, %d %b %Y %T UTC");
	return 0;
}