//#ifdef _MSC_VER
//#define _CRT_SECURE_NO_WARNINGS
//#endif // _MSC_VER 
#include "common_macro.h"
#include "mem/mem_debug.h"
#define LOG_TAG "FILE_ITOR"
#include "log/logger.h"

#include "file/file_iterator.h"
#include <sys/types.h>
//#include <sys/stat.h>
//#include <dirent.h>
#include "sys/dirent_wrapper.h"
#include <errno.h>

#if !defined(PATH_MAX)
#define PATH_MAX 260
#endif // !PATH_MAX

static int pri_get_file_stat(const char* file_name, struct stat *p_statbuf)
{
	memset(p_statbuf, 0, sizeof(struct stat));
	if (0 != stat(file_name, p_statbuf))
	{
		LOGE("error on get stat on \"%s\" : %d\n", file_name, errno);
		return (-1);
	}
	if (S_ISDIR(p_statbuf->st_mode))
	{
		return (FILE_ITERATOR_TYPE_DIR);
	}
	//if (S_ISREG(statbuf.st_mode))
	//LOGD("%s size: %ld bytes, modify at %s ", file_name, p_statbuf->st_size, ctime(&p_statbuf->st_mtime));
	return (FILE_ITERATOR_TYPE_FILE);
}

int file_iterator_foreach(char* dir, file_iterator_handle_file_info_fn handler, void* user_data)
{
	DIR* dirp;
	struct dirent* direntp;
	int stats;
	struct stat statbuf;

	char path_buf[PATH_MAX + 1] = { 0 };

	if ((FILE_ITERATOR_TYPE_FILE == (stats = pri_get_file_stat(dir, &statbuf))) || (-1 == stats))
	{
		LOGE("not a valid folder: %s", dir);
		return 1;
	}

	if (NULL == (dirp = opendir(dir)))
	{
		LOGE("error on open dir \"%s\": %d", dir, errno);
		return 1;
	}

	while (NULL != (direntp = readdir(dirp)))
	{
		size_t d_name_len = strlen(direntp->d_name);
		if (d_name_len < 3)
		{
			if (d_name_len == 1 && '.' == direntp->d_name[0]) // current dir
			{
				continue;
			}
			if (d_name_len == 2 && '.' == direntp->d_name[0] && '.' == direntp->d_name[1]) // parent dir
			{
				continue;
			}
		}
		snprintf(path_buf, sizeof(path_buf) - 1, "%s/%s", dir, direntp->d_name);
		stats = pri_get_file_stat(path_buf, &statbuf);
		if (stats == -1) break;
		/*if (FILE_ITERATOR_TYPE_FILE == stats)
		{
			LOGD("file:%s", path_buf);
		}
		else if (FILE_ITERATOR_TYPE_DIR == stats)
		{
			LOGD("sub_dir:%s", path_buf);
		}*/
		file_info_t info =
		{
			.dir = dir,
			.name = direntp->d_name,
			.type = stats,
			.p_stat = &statbuf,
		};
		if (0 != handler(&info, user_data)) break;
	}
	closedir(dirp);
	return 0;
}
