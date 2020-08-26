#include "time/time_util.h"
#include "time/time_rfc1123.h"
#include <string.h>

static const char *DAY_NAMES[] =
        {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static const char *MONTH_NAMES[] =
        {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
         "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

int time_rfc1123_now(char *out_time_str, size_t out_time_str_len) 
{
    if (!out_time_str || out_time_str_len <= TIME_RFC1123_STR_LEN) 
    {
        return -1;
    }
    time_t cur_time;
    struct tm gmtime;
    time(&cur_time);
    gmtime_r(&cur_time, &gmtime);

    strftime(out_time_str, TIME_RFC1123_STR_LEN + 1, "---, %d --- %Y %H:%M:%S GMT", &gmtime);
    memcpy(out_time_str, DAY_NAMES[gmtime.tm_wday], 3);
    memcpy(out_time_str + 8, MONTH_NAMES[gmtime.tm_mon], 3);
    return 0;
}