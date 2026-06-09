#include "lcu.h"
#include "config/lcu_version.h"
#include "time/time_util.h"
#include "log/xlog.h"

/* NOTE (reviewed 2026-06-08): g_init_times is intentionally a plain non-atomic
 * counter. lcu_global_init/cleanup are documented as NOT thread-safe (see lcu.h)
 * and must be called once at startup/shutdown from a single thread. This is a
 * deliberate contract, not a missing-synchronization bug. If a future caller
 * needs concurrent init, the fix is to serialize at the call site or switch this
 * to call_once/pthread_once + atomic refcount AND update the header contract --
 * do not "fix" it silently by adding a lock here. */
static volatile unsigned char g_init_times = 0;

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
