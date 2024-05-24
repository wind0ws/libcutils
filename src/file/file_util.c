#include "mem/mem_debug.h"
#include "file/file_util.h"
#include "mem/strings.h"
#include <sys/stat.h>

#ifdef _WIN32
#include <io.h>
#include <direct.h> 
#else
#include <unistd.h>
#endif // _WIN32

#define MAX_FOLDER_PATH_LEN (256)

#ifdef _WIN32
#define ACCESS(file_name, access_mode) _access(file_name, access_mode)
#define MKDIR(path)                    _mkdir(path)
#define READ_FUNC                      _read
#define WRITE_FUNC                     _write
#else								   
#define ACCESS(file_name, access_mode) access(file_name, access_mode)
#define MKDIR(path)                    mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IWOTH | S_IXOTH);
#define READ_FUNC                      read
#define WRITE_FUNC                     write
#endif // WIN32

typedef ssize_t (*rw_func_t)(int file_handle, void* buffer, size_t max_char_count);

static int pri_internal_rw_file(int file_handle, void* buffer, size_t max_char_count, rw_func_t target_func);

int file_util_append_slash_on_path_if_needed(__inout char* folder_path, __in const size_t folder_path_size)
{
	if (!folder_path || '\0' == folder_path[0] || folder_path_size < 3)
	{
		return -1;
	}
	size_t path_len = strnlen(folder_path, folder_path_size);
	while (path_len > 0 && ' ' == folder_path[path_len - 1])
	{
		--path_len;
	}
	if (path_len < 1)
	{
		return -2;
	}
	const char slash_char = (NULL != strstr(folder_path, "\\")) ? '\\' : '/';
	if (folder_path[path_len - 1] == slash_char)
	{
		return 0;
	}
	const size_t slash_location = (path_len + 1) < folder_path_size ?
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
// example: /home/root/my_dir/1/2/3/4/
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
		return -2; // dir too long
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
		if (0 == ACCESS(tmp_dir_path, 0))
		{
			//folder already exists
			continue;
		}
		if (0 != (ret = MKDIR(tmp_dir_path)))
		{
			//error occurred on mkdir
			return ret;
		}
	}
	return 0;
}

long file_util_get_size_by_path(__in const char* file_path)
{
	struct stat buf = { 0 };
	int stat_ret;
	if (0 != (stat_ret = stat(file_path, &buf))) 
	{
		return (long)stat_ret;
	}
	return buf.st_size;
}

long file_util_get_size_by_fs(__in FILE* fs)
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

int file_util_read(__in int file_handle, __out void* buffer, __in size_t max_char_count)
{
	return pri_internal_rw_file(file_handle, buffer, max_char_count, (rw_func_t)&READ_FUNC);
}

int file_util_write(__in int file_handle, __in void* buffer, __in size_t max_char_count)
{
	return pri_internal_rw_file(file_handle, buffer, max_char_count, (rw_func_t)&WRITE_FUNC);
}

static int pri_internal_rw_file(int file_handle, void* buffer, size_t max_char_count, rw_func_t target_func)
{
	int cur_char_count = 0;
	do
	{
		int once_op_size = (int)target_func(file_handle, (char*)buffer + cur_char_count, max_char_count - cur_char_count);
		if (once_op_size < 1)
		{
			//0:normal rw complete, otherwise error occurred
			break;
		}
		cur_char_count += once_op_size;
	} while (cur_char_count != max_char_count);
	return cur_char_count;
}

int file_util_read_txt(__in const char* file_path,
	__in int (*handle_txt_line_fn)(int line_num, char* txt, void* user_data),
	__in void* user_data)
{
	int ret = 0;
	FILE* fp;
	int line_num;
	char* p, buf[1024];

	fp = fopen(file_path, "r");
	if (NULL == fp)
	{
		return -1;
	}
	// if fgets returned not null, it make sure buf end with '\0'
	for (line_num = 0; NULL != fgets(buf, sizeof(buf), fp); ++line_num) 
	{
		if (NULL == (p = strchr(buf, '\n'))) 
		{
			p = buf + strlen(buf);
		}
		if (p > buf && p[-1] == '\r') 
		{
			--p;
		}
		*p = '\0';
		for (p = buf; *p != '\0' && isspace((int)(*p)); ++p) 
		{
			;
		}
		if (*p == '\0' /* || *p == '#' */) 
		{
			continue;
		}

		if (0 != (ret = (*handle_txt_line_fn)(line_num, p, user_data))) 
		{
			//LOGE("WARNING: cannot handle line[%d]=[%s], skipped", line_num, buf);
			break;
		}
	}
	fclose(fp);
	return ret;
}

int file_util_read_all(__in const char* file_path, __out char** out_alloced_file_data, __out int* out_file_byte_len)
{
	if (!file_path || '\0' == file_path[0] || !out_alloced_file_data || !out_file_byte_len)
	{
		if (out_alloced_file_data)
		{
			*out_alloced_file_data = NULL;
		}
		if (out_file_byte_len)
		{
			*out_file_byte_len = -1;
		}
		return -1;
	}
	*out_alloced_file_data = NULL;
	*out_file_byte_len = -1;

	int ret = -1;
	FILE* fp = fopen(file_path, "rb");
	if (!fp)
	{
		//LOGE("failed on fopen \"%s\"", file_path);
		return -2;
	}
	fseek(fp, 0, SEEK_END);
	const long file_size = ftell(fp);
	*out_file_byte_len = (int)file_size;
	do
	{
		if (file_size < 1)
		{
			ret = -3;
			break;
		}
		fseek(fp, 0, SEEK_SET);
		char* mem = malloc((size_t)file_size + 1);
		if (!mem)
		{
			ret = -4;
			break;
		}
		mem[file_size] = '\0'; // place '\0' for string file
		if (file_size != fread(mem, 1, file_size, fp))
		{
			free(mem);
			ret = -5;
			break;
		}
		*out_alloced_file_data = mem;
		ret = 0;
	} while (0);

	fclose(fp);
	return ret;
}

