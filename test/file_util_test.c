#include "file_util.h"
#include "strings.h"

int file_util_test()
{
	char log_path[64] = "./log/1";
	file_util_append_slash_on_path_if_needed(log_path, 64);
	if (file_util_dir_exists(log_path))
	{
		file_util_mkdirs(log_path);
	}
	if (file_util_dir_exists(".\\mylog\\"))
	{
		file_util_mkdirs(".\\mylog\\");
	}
	strlcpy(log_path, "D:\\MyLog\\sub", 64);
	file_util_append_slash_on_path_if_needed(log_path, 64);
	if (file_util_dir_exists(log_path))
	{
		file_util_mkdirs(log_path);
	}
	return 0;
}