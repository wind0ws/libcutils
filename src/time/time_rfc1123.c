#include "time/time_util.h"
#include <string.h>
#include "time/time_rfc1123.h"

static const char *DAY_NAMES[] =
        {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static const char *MONTH_NAMES[] =
        {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
         "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

int time_now_rfc1123(char *out_time_str, size_t out_time_str_len) {
    if (!out_time_str || out_time_str_len <= TIME_RFC1123_STR_LEN) {
        return -1;
    }
    time_t t;
    struct tm tm;
//    char * buf = malloc(RFC1123_TIME_LEN+1);
    time(&t);
//    gmtime_s(&tm, &t);
    gmtime_r(&t, &tm);

    strftime(out_time_str, TIME_RFC1123_STR_LEN + 1, "---, %d --- %Y %H:%M:%S GMT", &tm);
    memcpy(out_time_str, DAY_NAMES[tm.tm_wday], 3);
    memcpy(out_time_str + 8, MONTH_NAMES[tm.tm_mon], 3);
    return 0;
}