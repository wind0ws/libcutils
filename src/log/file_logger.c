#include "mem/mem_debug.h"
#include "log/file_logger.h"
#include "ring/msg_queue_handler.h"
#include "mem/strings.h"
#include "data/integer.h"          /* for integer_roundup_pow_of_two */
#include "common_macro.h"
#include "file/file_util.h"        /* for mkdir */
#include "thread/posix_thread.h"   /* for usleep */ 
#include "time/time_util.h"        /* for timestamp file name */

#define TRACE_FILE_LOGGER          1
#if(TRACE_FILE_LOGGER)
#define LOG_TAG                     "FILE_LOGGER"
#include "log/slog.h"   // here we shouldn't use xlog to log this module, because may cause infinite loop 

#define MY_LOGV(fmt, ...)            SLOGV(LOG_TAG, fmt, ##__VA_ARGS__)
#define MY_LOGD(fmt, ...)            SLOGD(LOG_TAG, fmt, ##__VA_ARGS__)
#define MY_LOGI(fmt, ...)            SLOGI(LOG_TAG, fmt, ##__VA_ARGS__)
#define MY_LOGW(fmt, ...)            SLOGW(LOG_TAG, fmt, ##__VA_ARGS__)
#define MY_LOGE(fmt, ...)            SLOGE(LOG_TAG, fmt, ##__VA_ARGS__)
#else
#define MY_LOGV(fmt, ...)
#define MY_LOGD(fmt, ...)
#define MY_LOGI(fmt, ...)
#define MY_LOGW(fmt, ...)
#define MY_LOGE(fmt, ...)
#endif // TRACE_FILE_LOGGER

#define MAX_FULL_PATH_SIZE (256)

typedef struct file_logger_s
{
	int timezone_hour;
	file_logger_cfg cfg;
	msg_queue_handler msg_queue;
	size_t cur_log_file_size_counter;
	FILE* cur_fp;
	queue_msg_t* msg_cache_p;
	size_t cur_msg_obj_capacity;
} file_logger_t;

#define FILE_LOGGER_LOCK(logger_handle) 	if (logger_handle->cfg.lock.acquire)\
{\
	logger_handle->cfg.lock.acquire(logger_handle->cfg.lock.arg);\
};

#define FILE_LOGGER_UNLOCK(logger_handle) 	if (logger_handle->cfg.lock.release)\
{\
	logger_handle->cfg.lock.release(logger_handle->cfg.lock.arg);\
};

static int handle_log_queue_msg(queue_msg_t* msg_p, void* user_data);

file_logger_handle file_logger_init(file_logger_cfg *cfg_p)
{
	if (!cfg_p || '\0' == cfg_p->log_folder_path[0] || cfg_p->log_queue_size < 2U)
	{
		return NULL;
	}
	if (strlen(cfg_p->log_folder_path) < 2U) //log folder path is abnormal
	{
		return NULL;
	}
	const size_t cur_msg_size = 2048U;
	queue_msg_t* msg = (queue_msg_t*)calloc(1, sizeof(queue_msg_t) + cur_msg_size);
	if (!msg)
	{
		return NULL;
	}
	file_logger_handle handle = (file_logger_handle)calloc(1, sizeof(file_logger_t));
	if (!handle)
	{
		free(msg);
		return NULL;
	}
	handle->msg_cache_p = msg;
	handle->cur_msg_obj_capacity = cur_msg_size;
	msg_queue_handler_init_param_t msg_q_init_param =
	{
		.user_data = handle,
		.fn_handle_msg = handle_log_queue_msg,
		.fn_on_status_changed = NULL,
	};
	handle->msg_queue = msg_queue_handler_create((uint32_t)cfg_p->log_queue_size * 1024U, &msg_q_init_param);
	if (NULL == handle->msg_queue)
	{
		free(msg);
		free(handle);
		handle = NULL;
		return NULL;
	}
	char* log_folder_path_formatted = strreplace(cfg_p->log_folder_path, "\\", "/");
	strlcpy(cfg_p->log_folder_path, log_folder_path_formatted, MAX_LOG_FOLDER_PATH_SIZE);
	free(log_folder_path_formatted);
	size_t log_folder_path_len = strlen(cfg_p->log_folder_path);
	if (cfg_p->log_folder_path[log_folder_path_len - 1] != '/')
	{
		size_t slash_location = (log_folder_path_len + 1) < MAX_LOG_FOLDER_PATH_SIZE ?
			log_folder_path_len : (log_folder_path_len - 1);
		cfg_p->log_folder_path[slash_location] = '/';
		cfg_p->log_folder_path[slash_location + 1] = '\0';
	}
	if (cfg_p->one_piece_file_max_len && cfg_p->one_piece_file_max_len < 64U) // file piece too small
	{
		MY_LOGE("one_piece_file_max_len=%zu too small, reset it to 0, which won't cut piece", cfg_p->one_piece_file_max_len);
		cfg_p->one_piece_file_max_len = 0U;// 0 that means won't create new log file automatically.
	}
	if (cfg_p->log_queue_size < 2U)
	{
		MY_LOGE("too small log_queue_size=%zu. reset it to 64", cfg_p->log_queue_size);
		cfg_p->log_queue_size = 64U;
	}
	handle->timezone_hour = time_util_zone_offset_seconds_to_utc() / 3600;
	handle->cfg = *cfg_p;
	return handle;
}

int file_logger_log(file_logger_handle handle, void* log_msg, size_t msg_size)
{
#define MAX_RETRY_LOG_TIMES_IF_FAIL (2)
	msg_q_code_e status = MSG_Q_CODE_BUF_NOT_ENOUGH;
	int retry_counter = 0;
	
	FILE_LOGGER_LOCK(handle);
	do
	{
		if (msg_size > handle->cur_msg_obj_capacity)
		{
			uint32_t new_msg_max_size = integer_roundup_pow_of_two((uint32_t)msg_size);
			size_t new_mem_size = sizeof(queue_msg_t) + new_msg_max_size;
			void* new_mem = realloc(handle->msg_cache_p, new_mem_size);
			if (NULL == new_mem)
			{
				MY_LOGE("err on realloc new_mem(%zu) at %s:%d", new_mem_size, __func__, __LINE__);
				break;
			}
			handle->msg_cache_p = (queue_msg_t*)new_mem;
			handle->cur_msg_obj_capacity = (size_t)new_msg_max_size;
		}

		memcpy(handle->msg_cache_p->obj, log_msg, msg_size);
		handle->msg_cache_p->obj_len = (int)msg_size;
		if (MSG_Q_CODE_SUCCESS == (status = msg_queue_handler_push(handle->msg_queue, handle->msg_cache_p)))
		{
			break;//everything all right, sending completed
		}
		//MY_LOGE("failed(%d) on send log to queue at this time. queue is full?", status);
		if (false == handle->cfg.is_try_my_best_to_keep_log)
		{
			break;// caution: here we must be lost this log message!
		}
		//MY_LOGE("try put it again later...");
		usleep(1500);//1.5ms
	} while (MSG_Q_CODE_SUCCESS != status && ++retry_counter < MAX_RETRY_LOG_TIMES_IF_FAIL);

	//final safety
	if (MSG_Q_CODE_SUCCESS != status && handle->cfg.is_try_my_best_to_keep_log)
	{
		char path_buffer[MAX_FULL_PATH_SIZE];
		path_buffer[MAX_FULL_PATH_SIZE - 1] = '\0';
		snprintf(path_buffer, sizeof(path_buffer) - 1, "%s%s_lost.log",
			handle->cfg.log_folder_path, handle->cfg.log_file_name_prefix);
		MY_LOGE(" warning: lost log, you can see it on *_lost.log");
		FILE* f_lost = fopen(path_buffer, "a");
		if (f_lost)
		{
			fprintf(f_lost, "%.*s\n\n", (int)msg_size, (char*)log_msg);
			fclose(f_lost);
		}
	}

	FILE_LOGGER_UNLOCK(handle);
	return (int)status;
}

int file_logger_destroy(file_logger_handle* handle_p)
{
	if (NULL == handle_p || NULL == *handle_p)
	{
		return -1;
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
	return 0;
}

static int handle_log_queue_msg(queue_msg_t* msg_p, void* user_data)
{
	if (msg_p->obj_len < 1)
	{
		return 0;
	}
	file_logger_handle handle = (file_logger_handle)user_data;
	if (!handle->cur_fp)
	{
		file_util_mkdirs(handle->cfg.log_folder_path);
		char cur_time[TIME_STR_SIZE];
		time_util_get_time_str_for_file_name_current(cur_time, handle->timezone_hour);
		char path_buffer[MAX_FULL_PATH_SIZE];
		path_buffer[MAX_FULL_PATH_SIZE - 1] = '\0';
		snprintf(path_buffer, MAX_FULL_PATH_SIZE - 1, "%s%s%s.log", 
			handle->cfg.log_folder_path, handle->cfg.log_file_name_prefix, cur_time);
		handle->cur_fp = fopen(path_buffer, "wb");
		handle->cur_log_file_size_counter = 0U;
	}
	if (!handle->cur_fp)
	{
#if(!defined(NDEBUG) || defined(_DEBUG))
		fprintf(stderr, "[ERROR] log file handle still null at (%s:%d)!!\n", __FILE__, __LINE__);
#endif // !NDEBUG || _DEBUG
		return 0;
	}
	int write_len = fprintf(handle->cur_fp, "%.*s\n", msg_p->obj_len, msg_p->obj);
	if (write_len > 0) //if error, negative number will returned
	{
		handle->cur_log_file_size_counter += write_len;
	}
#if(!defined(NDEBUG) || defined(_DEBUG))
	else 
	{
		fprintf(stderr, "[ERROR] fprintf(%d) log file returned %d at (%s:%d)!!\n", 
			msg_p->obj_len, write_len, __FILE__, __LINE__);
    }
#endif // !NDEBUG || _DEBUG
	if (handle->cfg.one_piece_file_max_len &&
		handle->cur_log_file_size_counter >= handle->cfg.one_piece_file_max_len)
	{
		fclose(handle->cur_fp);
		handle->cur_fp = NULL;
	}
	return 0;
}
