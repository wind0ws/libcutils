#include "lcu.h"
#include "config/lcu_version.h"
#include "time/time_util.h"

static volatile unsigned int g_init_times = 0;

char* lcu_get_version()
{
	return LCU_VERSION;
}

int lcu_init()
{
	if (g_init_times++ > 0)
	{
		return 0;
	}
	int ret = time_util_global_init();
	return ret;
}

int lcu_deinit()
{
	if (0 == g_init_times || --g_init_times > 0)
	{
		return 0;
	}
	int ret = time_util_global_deinit();
	return ret;
}
