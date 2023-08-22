#include "file/file_util.h"
#include "mem/strings.h"
#include "file/file_iterator.h"

#define LOG_TAG "FILE_UTIL_TEST"
#include "log/logger.h"

static int pri_handle_file_info(file_info_t* p_info, void* user_data);

int file_util_test()
{
	char log_path[64] = { 0 };

#ifdef _WIN32
	if (file_util_access(".\\mylog\\", F_OK))
	{
		file_util_mkdirs(".\\mylog\\");
	}
	strlcpy(log_path, ".\\mylog\\sub ", sizeof(log_path));
#else
	strlcpy(log_path, "./log/ ", sizeof(log_path));
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
			long file_size = file_util_get_size_by_fs(fs);
			LOGD_TRACE(" \"%s\" file size=%d", TEST_FILE, file_size);
			fclose(fs);
		}
	}

	file_iterator_foreach("./", pri_handle_file_info, NULL);

	return 0;
}

static int pri_handle_file_info(file_info_t* p_info, void* user_data)
{
	if (FILE_ITERATOR_TYPE_DIR == p_info->type)
	{
		LOGD("it is dir => %s/%s", p_info->dir, p_info->name);
	}
	else if (FILE_ITERATOR_TYPE_FILE == p_info->type)
	{
		LOGD("it is file => %s/%s, size=%d", p_info->dir, p_info->name, p_info->p_stat->st_size);
	}
	return 0;
}
