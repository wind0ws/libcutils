#include "file_util.h"
#include "strings.h"

int file_util_test()
{
	char log_path[64] = "./log/sub";
	file_util_append_slash_on_path_if_needed(log_path, 64);
	if (file_util_access(log_path))
	{
		file_util_mkdirs(log_path);
	}

#ifdef _WIN32
	if (file_util_access(".\\mylog\\"))
	{
		file_util_mkdirs(".\\mylog\\");
	}
	strlcpy(log_path, "D:\\MyLog\\sub", 64);
#else
	strlcpy(log_path, "./log/", 64);
#endif // _WIN32

	file_util_append_slash_on_path_if_needed(log_path, 64);
	if (file_util_access(log_path))
	{
		file_util_mkdirs(log_path);
	}
	return 0;
}