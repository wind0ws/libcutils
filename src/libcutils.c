#include "libcutils.h"
#include "config/lcu_version.h"
#include "time/time_util.h"

char *libcutils_get_version()
{
	return LCU_VERSION;
}

int libcutils_init()
{
   int ret = time_util_global_init();
   return ret;
}

int libcutils_deinit()
{
	int ret = time_util_global_deinit();
	return ret;
}
