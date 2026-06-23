#include "time/time_util.h" /* for gmtime_r */
#include "time/time_rfc1123.h"
#include <string.h>

static const char *DAY_NAMES[] =
        {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static const char *MONTH_NAMES[] =
        {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
         "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

int time_rfc1123(time_t* the_time, char* out_time_str, size_t out_time_str_size)
{
    if (!out_time_str || out_time_str_size < TIME_RFC1123_STR_SIZE)
    {
        return -1;
    }
    struct tm gmtime;
    /* P2-3: 检查 gmtime_r 失败 (time_t 越界时返回 NULL/EINVAL, tm 内容未定义). */
    memset(&gmtime, 0, sizeof(gmtime));
    if (NULL == gmtime_r(the_time, &gmtime))
    {
        return -2;
    }

    strftime(out_time_str, TIME_RFC1123_STR_SIZE, "---, %d --- %Y %H:%M:%S GMT", &gmtime);
    /* P2-3: 钳制下标, 防止 tm_wday/tm_mon 异常值越界读全局数组. */
    if ((unsigned)gmtime.tm_wday < 7U)
    {
        memcpy(out_time_str, DAY_NAMES[gmtime.tm_wday], 3U);
    }
    if ((unsigned)gmtime.tm_mon < 12U)
    {
        memcpy(out_time_str + 8, MONTH_NAMES[gmtime.tm_mon], 3U);
    }
    return 0;
}
