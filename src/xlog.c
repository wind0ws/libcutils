#include "xlog.h"
#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif// __cplusplus

	int xlog_config_level = LOG_LEVEL_VERBOSE;
#ifdef _WIN32
	int xlog_config_target = LOG_TARGET_CONSOLE;
#else
	int xlog_config_target = (LOG_TARGET_ANDROID | LOG_TARGET_CONSOLE); // NOLINT(hicpp-signed-bitwise)
#endif // _WIN32	

	void xlog_chars2hex(char *out_hex_str, size_t out_hex_str_capacity, const char* chars, size_t chars_len)
	{
		//char out_hex_str[sizeof(char) * chars_len * 3 + 1];
		//char out_hex_str[1024] = { '\0' };
		if (chars_len * 3 > out_hex_str_capacity)
		{
			strcat(out_hex_str, "hex is truncated:");
			chars_len = out_hex_str_capacity / 3 - 6;
		}
		char hex[4] = { '\0' };
		for (size_t i = 0; i < chars_len; ++i) {
			//        printf(" %02hhx", (unsigned char)(*(chars + i)));
			snprintf(hex, 4, " %02hhx", (unsigned char)(*(chars + i)));
			//        printf(" %s",hex);
			strcat(out_hex_str, hex);
		}
		//TLOGD(tag, "%s", out_hex_str);
		//    printf("%s\n", out_hex_str);
	}

#ifdef __cplusplus
};
#endif // __cplusplus
