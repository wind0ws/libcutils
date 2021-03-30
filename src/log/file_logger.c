#include <malloc.h>
#include "log/file_logger.h"
#include "ring/fixed_msg_queue_handler.h"
#include "mem/strings.h"
#include "common_macro.h"
//for mkdir
#include "file/file_util.h"
//for sleep 
#include "thread/thread_wrapper.h"
//for timestamp file name
#include "time/time_util.h"

typedef struct file_logger
{
	file_logger_cfg cfg;
	fixed_msg_queue_handler msg_queue;
	size_t cur_log_file_size_counter;
	FILE* cur_fp;
	int timezone_hour;
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
static void handle_log_queue_msg(fixed_msg_t* msg_p, void* user_data);

file_logger_handle file_logger_init(file_logger_cfg cfg)
{
	if (strlen(cfg.log_folder_path) < 2) //log folder path is abnormal
	{
		return NULL;
	}
	if (cfg.max_log_queue_size < 2)
	{
		return NULL;
	}
	file_logger_handle handle = calloc(1, sizeof(file_logger_t));
	if (NULL == handle)
	{
		return NULL;
	}
	handle->msg_queue = fixed_msg_queue_handler_create((uint32_t)cfg.max_log_queue_size, handle_log_queue_msg, handle);
	if (NULL == handle->msg_queue)
	{
		free(handle);
		handle = NULL;
		return NULL;
	}
	char* log_folder_path_formatted = strreplace(cfg.log_folder_path, "\\", "/");
	strlcpy(cfg.log_folder_path, log_folder_path_formatted, MAX_LOG_FOLDER_PATH_LEN);
	free(log_folder_path_formatted);
	size_t log_folder_path_len = strlen(cfg.log_folder_path);
	if (cfg.log_folder_path[log_folder_path_len - 1] != '/')
	{
		size_t slash_location = (log_folder_path_len + 1) < MAX_LOG_FOLDER_PATH_LEN ?
			log_folder_path_len : (log_folder_path_len - 1);
		cfg.log_folder_path[slash_location] = '/';
		cfg.log_folder_path[slash_location + 1] = '\0';
	}
	if (cfg.one_piece_file_max_len < 1024) // piece too small
	{
		//0 that means won't auto create new log file.
		cfg.one_piece_file_max_len = 0;
	}
	if (cfg.max_log_queue_size < 64)
	{
		cfg.max_log_queue_size = 64;
	}
	handle->timezone_hour = time_util_zone_offset_seconds_to_utc() / 3600;
	handle->cfg = cfg;
	return handle;
}

void file_logger_log(file_logger_handle handle, void* log_msg)
{
#define MAX_RETRY_LOG_TIMES_IF_FAIL (2)
	fixed_msg_t msg = { 0 };
	if (strlcpy(msg.obj.data, log_msg, MSG_OBJ_MAX_CAPACITY) < 1)
	{
		return;
	}
	int status = 0;
	int retry_counter = 0;

	FILE_LOGGER_LOCK(handle);
	do
	{
		if ((status = fixed_msg_queue_handler_send(handle->msg_queue, &msg)) == 0)
		{
			break;//send complete
		}
		RING_LOGE("failed on send log to queue. maybe queue is full! %d", status);
		if (handle->cfg.is_try_my_best_to_keep_log == false)
		{
			break;
		}
		RING_LOGE("try again after 1ms");
		++retry_counter;
		usleep(1000);
	} while (status != 0 && retry_counter < MAX_RETRY_LOG_TIMES_IF_FAIL && handle->cfg.is_try_my_best_to_keep_log);
	//final safety
	if (status != 0 && handle->cfg.is_try_my_best_to_keep_log)
	{
		char cur_time[TIME_STR_LEN];
		time_util_get_current_time_str_for_file_name(cur_time, handle->timezone_hour);
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
		fixed_msg_queue_handler_destroy(&(handle->msg_queue));
	}
	if (handle->cur_fp)
	{
		fclose(handle->cur_fp);
		handle->cur_fp = NULL;
	}
	free(handle);
	*handle_p = NULL;
}

static void handle_log_queue_msg(fixed_msg_t* msg_p, void* user_data)
{
	file_logger_handle handle = (file_logger_handle)user_data;
	if (!handle->cur_fp)
	{
		file_util_mkdirs(handle->cfg.log_folder_path);
		char cur_time[TIME_STR_LEN];
		time_util_get_current_time_str_for_file_name(cur_time, handle->timezone_hour);
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
	int write_len = fprintf(handle->cur_fp, "%.*s\n", MSG_OBJ_MAX_CAPACITY, msg_p->obj.data);
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
