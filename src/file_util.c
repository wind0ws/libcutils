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
#define MAX_PATH_LEN (256)

#ifdef WIN32
#define ACCESS(fileName,accessMode) _access(fileName,accessMode)
#define MKDIR(path) _mkdir(path)
#else
#define ACCESS(fileName,accessMode) access(fileName,accessMode)
#define MKDIR(path) mkdir(path,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)
#endif

void file_util_append_slash_on_path_if_needed(char *folder_path, const size_t folder_path_capacity) {
    const size_t path_len = strnlen(folder_path, folder_path_capacity);
	char slash_char = (NULL == strstr(folder_path, "/")) ? '\\' : '/';	
	if (folder_path[path_len - 1] != slash_char)
	{
		int slash_location = (path_len + 1) < folder_path_capacity ?
			path_len : (path_len - 1);
		folder_path[slash_location] = slash_char;
		folder_path[slash_location + 1] = '\0';
	}
}

int file_util_dir_exists(const char* dirs)
{
	return ACCESS(dirs, 0);
}

// 从左到右依次判断文件夹是否存在,不存在就创建
// example: /home/root/mkdir/1/2/3/4/
// 注意:最后一个如果是文件夹的话,需要加上 '\\' 或者 '/'
int file_util_mkdirs(const char * directory_path)
{
	if (NULL == directory_path)
	{
		return -1;
	}
	int ret;
	size_t dir_path_len = strlen(directory_path);
	if (dir_path_len > MAX_PATH_LEN)
	{
		return -2;
	}
	char tmp_dir_path[MAX_PATH_LEN] = { 0 };
	for (int i = 0; i < dir_path_len; ++i)
	{
		tmp_dir_path[i] = directory_path[i];
		if (tmp_dir_path[i] == '\\' || tmp_dir_path[i] == '/')
		{
			if (ACCESS(tmp_dir_path, 0) != 0)
			{
				ret = MKDIR(tmp_dir_path);
				if (ret != 0)
				{
					return ret;
				}
			}
		}
	}
	return 0;
}
