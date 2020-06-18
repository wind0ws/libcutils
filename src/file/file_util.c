#include "file_util.h"
#include "strings.h"
#include "common_macro.h"

#ifdef WIN32
#include <io.h>
#include <direct.h> 
#else
#include <unistd.h>
#include <sys/stat.h>
#endif
#include <stdint.h>
#include <string.h>
#define MAX_FOLDER_PATH_LEN (256)

#ifdef WIN32
#define ACCESS(fileName, accessMode) _access(fileName, accessMode)
#define MKDIR(path) _mkdir(path)
#else
#define ACCESS(fileName, accessMode) access(fileName, accessMode)
#define MKDIR(path) mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)
#endif

int file_util_append_slash_on_path_if_needed(__inout char* folder_path, __in const size_t folder_path_capacity) 
{
	if (!folder_path || folder_path_capacity < 3)
	{
		return -1;
	}
    const size_t path_len = strnlen(folder_path, folder_path_capacity);
	char slash_char = (strstr(folder_path, "/")) ?  '/': '\\';
	if (folder_path[path_len - 1] == slash_char)
	{
		return 0;
	}
	size_t slash_location = (path_len + 1) < folder_path_capacity ?
		path_len : (path_len - 1);
	folder_path[slash_location] = slash_char;
	folder_path[slash_location + 1] = '\0';
	return 0;
}

int file_util_access(__in const char* path, __in const int access_mode)
{
	return ACCESS(path, access_mode);
}

// 从左到右依次判断文件夹是否存在,不存在就创建
// example: /home/root/mkdir/1/2/3/4/
// 注意:最后一个如果是文件夹的话,需要加上 '\\' 或者 '/'
int file_util_mkdirs(__in const char* folder_path)
{
	if (!folder_path)
	{
		return -1;
	}
	int ret;
	size_t dir_path_len = strlen(folder_path);
	if (dir_path_len > MAX_FOLDER_PATH_LEN)
	{
		return -2;
	}
	char tmp_dir_path[MAX_FOLDER_PATH_LEN] = { 0 };
	for (size_t i = 0; i < dir_path_len; ++i)
	{
		tmp_dir_path[i] = folder_path[i];
		if (tmp_dir_path[i] != '\\' && tmp_dir_path[i] != '/')
		{
			//not a folder
			continue;
		}
		if (ACCESS(tmp_dir_path, 0) == 0)
		{
			//folder already exists
			continue;
		}
		ret = MKDIR(tmp_dir_path);
		if (ret != 0)
		{
			//error occurred on mkdir
			return ret;
		}
	}
	return 0;
}

long file_util_size(__in FILE* fs)
{
	if (!fs)
	{
		return -1;
	}
	if (fseek(fs, 0, SEEK_END)) 
	{
		return -2;
	}
	long total_file_size = ftell(fs);
	if (fseek(fs, 0, SEEK_SET))
	{
		return -3;
	}
	return total_file_size;
}