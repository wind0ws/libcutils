#include "log/file_logger.h"
#include "ring/msg_queue_handler.h"
#include "mem/strings.h"
#include "common_macro.h"
//for mkdir
#include "file/file_util.h"
//for sleep 
#include "thread/thread_wrapper.h"
//for timestamp file name
#include "time/time_util.h"
#include "log/simple_log.h"

#define _LOGGER_TAG                     "FILE_LOGGER"

#define MY_LOGV(fmt,...)                 SIMPLE_LOGV(_LOGGER_TAG, fmt, ##__VA_ARGS__)
#define MY_LOGD(fmt,...)                 SIMPLE_LOGD(_LOGGER_TAG, fmt, ##__VA_ARGS__)
#define MY_LOGI(fmt,...)                 SIMPLE_LOGI(_LOGGER_TAG, fmt, ##__VA_ARGS__)
#define MY_LOGW(fmt,...)                 SIMPLE_LOGW(_LOGGER_TAG, fmt, ##__VA_ARGS__)
#define MY_LOGE(fmt,...)                 SIMPLE_LOGE(_LOGGER_TAG, fmt, ##__VA_ARGS__)

typedef struct file_logger
{
	file_logger_cfg cfg;
	msg_queue_handler msg_queue;
	size_t cur_log_file_size_counter;
	FILE* cur_fp;
	int timezone_hour;
	queue_msg_t* msg_cache_p;
	size_t cur_msg_obj_capacity;
}file_logger_t;

#define FILE_LOGGER_LOCK(logger_handle) 	if (logger_handle->cfg.lock.acquire)\
{\
	logger_handle->cfg.lock.acquire(logger_handle->cfg.lock.arg);\
};
#define FILE_LOGGER_UNLOCK(logger_handle) 	if (logger_handle->cfg.lock.release)\
{\
	logger_handle->cfg.lock.release(logger_handle->cfg.lock.arg);\
};
#define MAX_FULL_PATH_BUFFER (256)
static void handle_log_queue_msg(queue_msg_t* msg_p, void* user_data);

file_logger_handle file_logger_init(file_logger_cfg cfg)
{
	if (strlen(cfg.log_folder_path) < 2) //log folder path is abnormal
	{
		return NULL;
	}
	if (cfg.log_queue_size < 2)
	{
		return NULL;
	}
	size_t cur_msg_size = 2048;
	queue_msg_t* msg = (queue_msg_t*)calloc(1, sizeof(queue_msg_t) + cur_msg_size);
	if (!msg)
	{
		return NULL;
	}
	file_logger_handle handle = calloc(1, sizeof(file_logger_t));
	if (NULL == handle)
	{
		free(msg);
		return NULL;
	}
	handle->msg_cache_p = msg;
	handle->cur_msg_obj_capacity = cur_msg_size;
	handle->msg_queue = msg_queue_handler_create((uint32_t)cfg.log_queue_size * 1024, handle_log_queue_msg, handle);
	if (NULL == handle->msg_queue)
	{
		free(msg);
		free(handle);
		handle = NULL;
		return NULL;
	}
	char* log_folder_path_formatted = strreplace(cfg.log_folder_path, "\\", "/");
	strlcpy(cfg.log_folder_path, log_folder_path_formatted, MAX_LOG_FOLDER_PATH_SIZE);
	free(log_folder_path_formatted);
	size_t log_folder_path_len = strlen(cfg.log_folder_path);
	if (cfg.log_folder_path[log_folder_path_len - 1] != '/')
	{
		size_t slash_location = (log_folder_path_len + 1) < MAX_LOG_FOLDER_PATH_SIZE ?
			log_folder_path_len : (log_folder_path_len - 1);
		cfg.log_folder_path[slash_location] = '/';
		cfg.log_folder_path[slash_location + 1] = '\0';
	}
	if (cfg.one_piece_file_max_len < 1024) // piece too small
	{
		//0 that means won't auto create new log file.
		cfg.one_piece_file_max_len = 0;
	}
	if (cfg.log_queue_size < 64)
	{
		cfg.log_queue_size = 64;
	}
	handle->timezone_hour = time_util_zone_offset_seconds_to_utc() / 3600;
	handle->cfg = cfg;
	return handle;
}

void file_logger_log(file_logger_handle handle, void* log_msg, size_t msg_size)
{
#define MAX_RETRY_LOG_TIMES_IF_FAIL (2)
	MSG_Q_CODE status = 0;
	int retry_counter = 0;
	if (msg_size > handle->cur_msg_obj_capacity)
	{
		return; // should make buffer bigger
	}

	FILE_LOGGER_LOCK(handle);
	memcpy(handle->msg_cache_p->obj, log_msg, msg_size);
	handle->msg_cache_p->obj_len = (int)msg_size;
	do
	{
		if ((status = msg_queue_handler_send(handle->msg_queue, handle->msg_cache_p)) == 0)
		{
			break;//send complete
		}
		MY_LOGE("failed on send log to queue. maybe queue is full! %d", status);
		if (handle->cfg.is_try_my_best_to_keep_log == false)
		{
			break;
		}
		MY_LOGE("try again after 1ms");
		++retry_counter;
		usleep(1000);
	} while (status != 0 && retry_counter < MAX_RETRY_LOG_TIMES_IF_FAIL && handle->cfg.is_try_my_best_to_keep_log);
	//final safety
	if (status != 0 && handle->cfg.is_try_my_best_to_keep_log)
	{
		char cur_time[TIME_STR_SIZE];
		time_util_get_time_str_for_file_name_current(cur_time, handle->timezone_hour);
		char path_buffer[MAX_FULL_PATH_BUFFER];
		snprintf(path_buffer, MAX_FULL_PATH_BUFFER, "%slost_%s_%s.log",
			handle->cfg.log_folder_path, handle->cfg.log_file_name_prefix, cur_time);
		FILE* f_lost = fopen(path_buffer, "wb");
		if (f_lost)
		{
			fprintf(f_lost, "%s\n", (char*)log_msg);
			fclose(f_lost);
		}
	}
	FILE_LOGGER_UNLOCK(handle);
}

void file_logger_deinit(file_logger_handle* handle_p)
{
	if (NULL == handle_p || NULL == *handle_p)
	{
		return;
	}
	file_logger_handle handle = *handle_p;
	if (handle->msg_queue)
	{
		msg_queue_handler_destroy(&(handle->msg_queue));
	}
	if (handle->msg_cache_p)
	{
		free(handle->msg_cache_p);
		handle->msg_cache_p = NULL;
		handle->cur_msg_obj_capacity = 0;
	}
	if (handle->cur_fp)
	{
		fclose(handle->cur_fp);
		handle->cur_fp = NULL;
	}
	free(handle);
	*handle_p = NULL;
}

static void handle_log_queue_msg(queue_msg_t* msg_p, void* user_data)
{
	if (msg_p->obj_len < 1)
	{
		return;
	}
	file_logger_handle handle = (file_logger_handle)user_data;
	if (!handle->cur_fp)
	{
		file_util_mkdirs(handle->cfg.log_folder_path);
		char cur_time[TIME_STR_SIZE];
		time_util_get_time_str_for_file_name_current(cur_time, handle->timezone_hour);
		char path_buffer[MAX_FULL_PATH_BUFFER];
		snprintf(path_buffer, MAX_FULL_PATH_BUFFER, "%s%s_%s.log", handle->cfg.log_folder_path, handle->cfg.log_file_name_prefix, cur_time);
		handle->cur_fp = fopen(path_buffer, "wb");
		handle->cur_log_file_size_counter = 0;
	}
	if (!handle->cur_fp)
	{
#if(!defined(NDEBUG) || defined(_DEBUG))
		printf("[DEBUG] log file handle still null at [%s:%d]!!\n", __FILE__, __LINE__);
#endif // !NDEBUG || _DEBUG
		return;
	}
	int write_len = fprintf(handle->cur_fp, "%.*s\n", msg_p->obj_len, msg_p->obj);
	if (write_len)
	{
		handle->cur_log_file_size_counter += write_len;
	}
	if (handle->cfg.one_piece_file_max_len &&
		handle->cur_log_file_size_counter >= handle->cfg.one_piece_file_max_len)
	{
		fclose(handle->cur_fp);
		handle->cur_fp = NULL;
	}
}
