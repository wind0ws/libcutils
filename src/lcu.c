#include "lcu.h"
#include "config/lcu_version.h"
#include "time/time_util.h"
#include "log/xlog.h"

static volatile unsigned int g_init_times = 0;

char* lcu_get_version()
{
	return LCU_VERSION;
}

int lcu_global_init()
{
	if (g_init_times++ > 0)
	{
		return 0;
	}
	// do not use any lcu function before init! such as XLOG, get TIME 
	int ret = time_util_global_init();
	ret |= xlog_global_init();
	return ret;
}

int lcu_global_cleanup()
{
	if (0 == g_init_times || --g_init_times > 0)
	{
		return 0;
	}
	// now cleanup, do not use any lcu function after cleanup!
	int ret = xlog_global_cleanup();
	ret |= time_util_global_cleanup();
	return ret;
}
