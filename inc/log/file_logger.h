#pragma once
#ifndef LCU_FILE_LOGGER_H
#define LCU_FILE_LOGGER_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct file_logger* file_logger_handle;

	typedef struct file_logger_lock 
	{
		void* arg; /**< Argument to be passed to acquire and release function pointers */
		int (*acquire)(void* arg); /**< Function pointer to acquire a lock */
		int (*release)(void* arg); /**< Function pointer to release a lock */
	} file_logger_lock_t;

	typedef struct
	{
#define MAX_LOG_FOLDER_PATH_SIZE (128)
#define MAX_LOG_FILE_NAME_PREFIX_SIZE (16)
		char log_folder_path[MAX_LOG_FOLDER_PATH_SIZE];
		char log_file_name_prefix[MAX_LOG_FILE_NAME_PREFIX_SIZE];
		size_t one_piece_file_max_len;
		size_t max_log_queue_size;
		bool is_try_my_best_to_keep_log;
		file_logger_lock_t lock;
	}file_logger_cfg;

	file_logger_handle file_logger_init(file_logger_cfg cfg);

	void file_logger_log(file_logger_handle handle, void* log_msg);

	void file_logger_deinit(file_logger_handle* handle_p);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // LCU_FILE_LOGGER_H
