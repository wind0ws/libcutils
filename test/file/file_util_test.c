#include "file/file_util.h"
#include "mem/strings.h"
#include "log/xlog.h"

int file_util_test()
{
	char log_path[64] = "./log/sub";
	file_util_append_slash_on_path_if_needed(log_path, sizeof(log_path));
	if (file_util_access(log_path, F_OK))
	{
		file_util_mkdirs(log_path);
	}

#ifdef _WIN32
	if (file_util_access(".\\mylog\\", F_OK))
	{
		file_util_mkdirs(".\\mylog\\");
	}
	strlcpy(log_path, "D:\\MyLog\\sub", sizeof(log_path));
#else
	strlcpy(log_path, "./log/", sizeof(log_path));
#endif // _WIN32

	file_util_append_slash_on_path_if_needed(log_path, sizeof(log_path));
	if (file_util_access(log_path, F_OK))
	{
		file_util_mkdirs(log_path);
	}
#define TEST_FILE "CMakeCache.txt"
	if (file_util_access(TEST_FILE, F_OK) == 0)
	{
		FILE* fs = fopen(TEST_FILE, "rb");
		if (fs)
		{
			long file_size = file_util_size_by_fs(fs);
			LOGD_TRACE(" \"%s\" file size=%d", TEST_FILE, file_size);
			fclose(fs);
		}
	}
	return 0;
}
