#include "time_util.h"
#include "time_rfc2822.h"

static void time_now_strf(char* out_time_str, size_t out_time_str_len, const char* timeFmt) {
	struct tm tm_time;
	time_t current;
	out_time_str[0] = '\0';

	time(&current);
	strftime(
		out_time_str,
		out_time_str_len,
		timeFmt,
		gmtime_r(&current, &tm_time)
	);
}

int time_now_rfc2822(char* out_time_str, size_t out_time_str_len) {
	if (!out_time_str || out_time_str_len <= TIME_RFC2822_STR_LEN) {
		return -1;
	}
	time_now_strf(out_time_str, out_time_str_len, "%a, %d %b %Y %T %z");
	return 0;
}

int time_now_rfc2822_utc(char* out_time_str, size_t out_time_str_len) {
	if (!out_time_str || out_time_str_len <= TIME_RFC2822_UTC_STR_LEN) {
		return -1;
	}
	time_now_strf(out_time_str, out_time_str_len, "%a, %d %b %Y %T UTC");
	return 0;
}