#include "xlog.h"

#ifdef __cplusplus
extern "C"
{
#endif// __cplusplus

	int xlog_config_level = LOG_LEVEL_VERBOSE;
#ifdef _MSC_VER
	int xlog_config_target = LOG_TARGET_CONSOLE;
#else
	int xlog_config_target = (LOG_TARGET_ANDROID | LOG_TARGET_CONSOLE); // NOLINT(hicpp-signed-bitwise)
#endif // _MSC_VER	

#ifdef __cplusplus
};
#endif // __cplusplus
