#include "file_util.h"
#include "strings.h"
#include "common_macro.h"

int file_util_mkdirs(const char* dirs) {
#ifdef _WIN32
	return mkdir(dirs, 0777);
#else
	int i, len, code;
	char str[512] = { 0 };
	strncpy(str, dirs, 512);
	len = strlen(str);
	for (i = 0; i < len; i++) {
		if (str[i] != '/') {
			continue;
		}
		str[i] = '\0';
		if (strlen(str) > 0 && access(str, F_OK) != 0) {
			if ((code = mkdir(str, 0777)) != 0) {
				//LOGE("error on mkdir(%s),code=%d", str, code);
				return 1;
			}
		}
		str[i] = '/';
	}
	if (len > 0 && strlen(str) > 0 && access(str, F_OK) != 0) {
		if ((code = mkdir(str, 0777)) != 0) {
			//LOGE("error on mkdir(%s),code=%d", str, code);
			return 2;
		}
	}
	return 0;
#endif
}

