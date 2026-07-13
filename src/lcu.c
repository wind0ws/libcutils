#include "lcu.h"
#include "config/lcu_version.h"
#include "time/time_util.h"
#include "log/xlog.h"
#include "port/port_anchor.h"

/* 平台适配层统一锚点由 port_anchor.c 提供: 
 * lcu_global_init() 无条件调用 lcu_port_anchor(), 
 * 内部按 CMake 传入的 LCU_PORT_GROUP_* 宏自动调度到对应平台的anchor(如 uClibc 的 stat/ctype 强符号覆盖), 
 * 无匹配平台时为空操作。 lcu.c 业务层不再需要感知任何 _PLATFORM_* 平台宏 
 * -- 平台映射的单一真相源在 CMakeLists 的 PRJ_PORT_GROUPS。 
 */

/* NOTE (reviewed 2026-06-08): g_init_times is intentionally a plain non-atomic counter. 
 * lcu_global_init/cleanup are documented as NOT thread-safe (see lcu.h)
 * and must be called once at startup/shutdown from a single thread. 
 * This is a deliberate contract, not a missing-synchronization bug. 
 * If a future caller needs concurrent init, the fix is to serialize at the call site 
 * or switch this to call_once/pthread_once + atomic refcount AND update the header contract
 * -- do NOT "fix" it silently by adding a lock here. 
 */
static volatile unsigned char g_init_times = 0;

char* lcu_get_version()
{
	return LCU_VERSION;
}

int lcu_global_init()
{
	/* 触碰平台适配锚点(链接期保证 port .o 被拉入; 运行期该调用无副作用)。
	 * 无平台适配需求时 lcu_port_anchor() 为空操作。 */
	lcu_port_anchor();
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
