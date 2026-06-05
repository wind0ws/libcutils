#include "mem/mem_debug.h"
#include "common_macro.h"
#include "log/file_logger.h"
#include "ring/msg_queue_handler.h"
#include "mem/strings.h"
#include "data/integer.h"		 /* for integer_roundup_pow_of_two */
#include "file/file_util.h"		 /* for mkdir */
#include "sys/dirent.h"			 /* for access dir entry */
#include "thread/posix_thread.h" /* for usleep */
#include "time/time_util.h"		 /* for timestamp file name */

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#ifdef _DEBUG
#define LOG_TAG "FILE_LOGGER"
#include "log/slog.h" // here we shouldn't use xlog to log this module, because may cause infinite loop

#define MY_LOGV(fmt, ...) SLOGV(LOG_TAG, fmt, ##__VA_ARGS__)
#define MY_LOGD(fmt, ...) SLOGD(LOG_TAG, fmt, ##__VA_ARGS__)
#define MY_LOGI(fmt, ...) SLOGI(LOG_TAG, fmt, ##__VA_ARGS__)
#define MY_LOGW(fmt, ...) SLOGW(LOG_TAG, fmt, ##__VA_ARGS__)
#define MY_LOGE(fmt, ...) SLOGE(LOG_TAG, fmt, ##__VA_ARGS__)
#else
#define MY_LOGV(fmt, ...)
#define MY_LOGD(fmt, ...)
#define MY_LOGI(fmt, ...)
#define MY_LOGW(fmt, ...)
#define MY_LOGE(fmt, ...)
#endif // _DEBUG

#define MAX_FULL_PATH_SIZE (256)
// interval to cleanup old log files
#define FILE_LOGGER_CLEANUP_INTERVAL_MS (60000U)

#if defined(_WIN32)
#define FILE_LOGGER_STAT_STRUCT struct _stat64
#define FILE_LOGGER_STAT(path, st_ptr) _stat64((path), (st_ptr))
#else
#define FILE_LOGGER_STAT_STRUCT struct stat
#define FILE_LOGGER_STAT(path, st_ptr) stat((path), (st_ptr))
#endif

#ifndef S_ISREG
#ifdef _WIN32
#define S_ISREG(m) (((m) & _S_IFMT) == _S_IFREG)
#else
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif
#endif // !S_ISREG

typedef struct
{
	char path[MAX_FULL_PATH_SIZE];
	time_t modified_time;
	uint64_t size;
} log_file_entry_t;

typedef struct file_logger_s
{
	int timezone_hour;
	file_logger_cfg cfg;
	msg_queue_handler msg_queue;
	size_t cur_log_file_size_counter;
	FILE *cur_fp;
	char cur_file_path[MAX_FULL_PATH_SIZE];
	queue_msg_t *msg_cache_p;
	size_t cur_msg_obj_capacity;
	uint64_t last_cleanup_ms;
} file_logger_t;

#define FILE_LOGGER_LOCK(logger_handle)                               \
	if (logger_handle->cfg.lock.acquire)                              \
	{                                                                 \
		logger_handle->cfg.lock.acquire(logger_handle->cfg.lock.arg); \
	};

#define FILE_LOGGER_UNLOCK(logger_handle)                             \
	if (logger_handle->cfg.lock.release)                              \
	{                                                                 \
		logger_handle->cfg.lock.release(logger_handle->cfg.lock.arg); \
	};

static int handle_log_queue_msg(queue_msg_t *msg_p, void *user_data);
static void file_logger_try_cleanup(file_logger_t *handle, bool force_now);
static void file_logger_cleanup_logs(file_logger_t *handle);
static bool file_logger_should_skip_cleanup(const file_logger_t *handle);
static bool file_logger_gather_entries(file_logger_t *handle, log_file_entry_t **entries_p, size_t *entry_count_p);
static void file_logger_free_entries(log_file_entry_t **entries_p);
static bool file_logger_entries_push(log_file_entry_t **entries_p, size_t *count_p, size_t *capacity_p, const log_file_entry_t *value_p);
static bool file_logger_is_log_file(const file_logger_t *handle, const char *file_name);
static bool file_logger_remove_file(file_logger_t *handle, const char *path);
static int log_file_entry_compare(const void *lhs, const void *rhs);

static bool file_logger_should_skip_cleanup(const file_logger_t *handle)
{
	return (!handle || (0U == handle->cfg.max_log_retention_days && 0U == handle->cfg.max_total_log_storage_bytes));
}

static void file_logger_try_cleanup(file_logger_t *handle, bool force_now)
{
	if (file_logger_should_skip_cleanup(handle))
	{
		return;
	}
	uint64_t now_ms = 0U;
	time_util_current_ms(&now_ms);
	if (!force_now && 0 != handle->last_cleanup_ms &&
		(now_ms - handle->last_cleanup_ms) < FILE_LOGGER_CLEANUP_INTERVAL_MS)
	{
		return;
	}
	handle->last_cleanup_ms = now_ms;
	file_logger_cleanup_logs(handle);
}

static void file_logger_cleanup_logs(file_logger_t *handle)
{
	if (file_logger_should_skip_cleanup(handle))
	{
		return;
	}
	log_file_entry_t *entries = NULL;
	size_t entry_count = 0U;
	if (!file_logger_gather_entries(handle, &entries, &entry_count) || 0U == entry_count)
	{
		file_logger_free_entries(&entries);
		return;
	}
	const bool has_retention_policy = (handle->cfg.max_log_retention_days > 0U);
	const bool has_size_policy = (handle->cfg.max_total_log_storage_bytes > 0U);
	uint64_t total_size = 0U;
	time_t expire_before = 0;
	if (has_retention_policy)
	{
		time_t now = time(NULL);
		expire_before = now - (time_t)handle->cfg.max_log_retention_days * 24 * 3600;
	}
	size_t write_idx = 0U;
	for (size_t i = 0U; i < entry_count; ++i)
	{
		log_file_entry_t entry = entries[i];
		if (handle->cur_file_path[0] != '\0' && 0 == strcmp(entry.path, handle->cur_file_path))
		{
			total_size += entry.size;
			entries[write_idx++] = entry;
			continue;
		}
		if (has_retention_policy && entry.modified_time < expire_before)
		{
			(void)file_logger_remove_file(handle, entry.path);
			continue;
		}
		total_size += entry.size;
		entries[write_idx++] = entry;
	}
	entry_count = write_idx;
	if (has_size_policy && entry_count > 0U && total_size > handle->cfg.max_total_log_storage_bytes)
	{
		qsort(entries, entry_count, sizeof(log_file_entry_t), log_file_entry_compare);
		for (size_t i = 0U; i < entry_count && total_size > handle->cfg.max_total_log_storage_bytes; ++i)
		{
			if (handle->cur_file_path[0] != '\0' && 0 == strcmp(entries[i].path, handle->cur_file_path))
			{
				continue;
			}
			if (file_logger_remove_file(handle, entries[i].path))
			{
				if (total_size > entries[i].size)
				{
					total_size -= entries[i].size;
				}
				else
				{
					total_size = 0U;
				}
			}
		}
	}
	file_logger_free_entries(&entries);
}

static bool file_logger_gather_entries(file_logger_t *handle, log_file_entry_t **entries_p, size_t *entry_count_p)
{
	if (!entries_p || !entry_count_p || !handle)
	{
		return false;
	}
	file_util_mkdirs(handle->cfg.log_folder_path);
	DIR *dir = opendir(handle->cfg.log_folder_path);
	if (!dir)
	{
		return false;
	}
	size_t capacity = 0U;
	size_t count = 0U;
	struct dirent *dir_entry = NULL;
	while ((dir_entry = readdir(dir)) != NULL)
	{
		if ('.' == dir_entry->d_name[0])
		{
			continue;
		}
		if (!file_logger_is_log_file(handle, dir_entry->d_name))
		{
			continue;
		}
		char full_path[MAX_FULL_PATH_SIZE];
		int written = snprintf(full_path, sizeof(full_path), "%s%s", handle->cfg.log_folder_path, dir_entry->d_name);
		if (written <= 0 || written >= (int)sizeof(full_path))
		{
			continue;
		}
		FILE_LOGGER_STAT_STRUCT file_stat;
		if (0 != FILE_LOGGER_STAT(full_path, &file_stat))
		{
			continue;
		}
		if (!S_ISREG(file_stat.st_mode))
		{
			continue;
		}
		log_file_entry_t entry;
		memset(&entry, 0, sizeof(entry));
		strlcpy(entry.path, full_path, sizeof(entry.path));
		entry.modified_time = file_stat.st_mtime;
		entry.size = (uint64_t)file_stat.st_size;
		if (!file_logger_entries_push(entries_p, &count, &capacity, &entry))
		{
			closedir(dir);
			file_logger_free_entries(entries_p);
			return false;
		}
	}
	closedir(dir);
	*entry_count_p = count;
	return true;
}

static void file_logger_free_entries(log_file_entry_t **entries_p)
{
	if (entries_p && *entries_p)
	{
		free(*entries_p);
		*entries_p = NULL;
	}
}

static bool file_logger_entries_push(log_file_entry_t **entries_p, size_t *count_p, size_t *capacity_p, const log_file_entry_t *value_p)
{
	if (!entries_p || !count_p || !capacity_p || !value_p)
	{
		return false;
	}
	if (*count_p >= *capacity_p)
	{
		size_t new_capacity = (*capacity_p == 0U) ? 8U : (*capacity_p * 2U);
		log_file_entry_t *new_entries = (log_file_entry_t *)realloc(*entries_p, new_capacity * sizeof(log_file_entry_t));
		if (!new_entries)
		{
			return false;
		}
		*entries_p = new_entries;
		*capacity_p = new_capacity;
	}
	(*entries_p)[*count_p] = *value_p;
	(*count_p)++;
	return true;
}

static bool file_logger_is_log_file(const file_logger_t *handle, const char *file_name)
{
	if (!handle || !file_name)
	{
		return false;
	}
	const size_t prefix_len = strlen(handle->cfg.log_file_name_prefix);
	if (prefix_len > 0U && strncmp(file_name, handle->cfg.log_file_name_prefix, prefix_len) != 0)
	{
		return false;
	}
	const char *extension = strrchr(file_name, '.');
	if (!extension || 0 != strcmp(extension, ".log"))
	{
		return false;
	}
	return true;
}

static bool file_logger_remove_file(file_logger_t *handle, const char *path)
{
	if (!path || '\0' == path[0])
	{
		return false;
	}
	if (handle && handle->cur_file_path[0] != '\0' && 0 == strcmp(handle->cur_file_path, path))
	{
		return false;
	}
	if (0 == remove(path))
	{
		MY_LOGI("cleanup removed log file: \"%s\"", path);
		return true;
	}
	/* TOCTOU race: file may have been deleted by external process between
	 * gather_entries scan and this remove call. Treat ENOENT as success
	 * (file is gone, which is the desired outcome). */
	if (errno == ENOENT)
	{
		MY_LOGI("cleanup: log file already removed by external process: \"%s\"", path);
		return true;
	}
	MY_LOGW("cleanup failed to remove log file: \"%s\" (errno=%d)", path, errno);
	return false;
}

static int log_file_entry_compare(const void *lhs, const void *rhs)
{
	const log_file_entry_t *left = (const log_file_entry_t *)lhs;
	const log_file_entry_t *right = (const log_file_entry_t *)rhs;
	if (left->modified_time < right->modified_time)
	{
		return -1;
	}
	if (left->modified_time > right->modified_time)
	{
		return 1;
	}
	if (left->size < right->size)
	{
		return -1;
	}
	if (left->size > right->size)
	{
		return 1;
	}
	return strcmp(left->path, right->path);
}

file_logger_handle file_logger_init(file_logger_cfg *cfg_p)
{
	queue_msg_t *msg = NULL;
	file_logger_handle handle = NULL;
	char *log_folder_path_formatted = NULL;

	if (!cfg_p || '\0' == cfg_p->log_folder_path[0] || cfg_p->log_queue_size < 2U)
	{
		return NULL;
	}
	if (strlen(cfg_p->log_folder_path) < 2U) // log folder path is abnormal
	{
		return NULL;
	}
	const size_t cur_msg_size = 2048U;
	msg = (queue_msg_t *)calloc(1, sizeof(queue_msg_t) + cur_msg_size);
	if (!msg)
	{
		goto cleanup_on_error;
	}
	handle = (file_logger_handle)calloc(1, sizeof(file_logger_t));
	if (!handle)
	{
		goto cleanup_on_error;
	}
	/* P0-1 修订: msg 所有权转移到 handle->msg_cache_p, 立即置 NULL 防双重 free.
	 * 注意: 从此处到 msg_queue_handler_create 之间不允许新增失败点;
	 *       否则 cleanup_on_error 必须考虑 msg_cache_p 是否要保留. */
	handle->msg_cache_p = msg;
	handle->cur_msg_obj_capacity = cur_msg_size;
	msg = NULL;

	/* cfg 前置赋值: worker 启动前 handle 字段必须完整, 否则 cleanup 路径触发
	 * msg_queue_handler_destroy 唤醒 worker 时, worker 可能访问未初始化字段. */
	handle->timezone_hour = time_util_zone_offset_seconds_to_utc() / 3600;
	handle->cfg = *cfg_p;
	handle->cur_file_path[0] = '\0';
	handle->last_cleanup_ms = 0U;

	/* 路径规范化: 将反斜杠替换为正斜杠, 写入 handle 内部副本(不污染调用方 cfg_p). */
	log_folder_path_formatted = strreplace(handle->cfg.log_folder_path, "\\", "/");
	if (!log_folder_path_formatted)
	{
		goto cleanup_on_error;
	}
	strlcpy(handle->cfg.log_folder_path, log_folder_path_formatted, MAX_LOG_FOLDER_PATH_SIZE);
	free(log_folder_path_formatted);
	log_folder_path_formatted = NULL;

	size_t log_folder_path_len = strlen(handle->cfg.log_folder_path);
	if (handle->cfg.log_folder_path[log_folder_path_len - 1] != '/')
	{
		if (log_folder_path_len + 2 > MAX_LOG_FOLDER_PATH_SIZE) // 路径已满，无法追加 '/'
		{
			goto cleanup_on_error;
		}
		handle->cfg.log_folder_path[log_folder_path_len] = '/';
		handle->cfg.log_folder_path[log_folder_path_len + 1] = '\0';
	}
	if (handle->cfg.one_piece_file_max_len && handle->cfg.one_piece_file_max_len < 64U)
	{
		MY_LOGE("one_piece_file_max_len=%zu too small, reset it to 0, which won't cut piece",
				handle->cfg.one_piece_file_max_len);
		handle->cfg.one_piece_file_max_len = 0U;
	}

	/* 此时 cfg 已完全规范化, 可以启动 worker */
	uint32_t log_queue_mem_size = (uint32_t)handle->cfg.log_queue_size * 1024U;
	msg_queue_handler_init_param_t msg_q_init_param =
		{
			.callback =
				{
					.user_data = handle,
					.fn_handle_msg = handle_log_queue_msg,
					.fn_on_status_changed = NULL,
				},
			.cfg =
				{
					.threshold_mem_size_for_alloc_obj = (log_queue_mem_size / 4U),
				},
		};
	handle->msg_queue = msg_queue_handler_create(log_queue_mem_size, &msg_q_init_param);
	if (NULL == handle->msg_queue)
	{
		goto cleanup_on_error;
	}

	file_logger_try_cleanup(handle, true);
	return handle;

cleanup_on_error:
	/* 统一清理路径. 注意分支处理:
	 * - msg 不为 NULL <=> handle 尚未持有 msg_cache_p 所有权(handle 可能是 NULL 或刚 calloc)
	 * - msg 为 NULL && handle 不为 NULL <=> handle->msg_cache_p == 之前的 msg, 由 free(handle->msg_cache_p) 释放
	 * - 局部 log_folder_path_formatted 不为 NULL 时单独 free */
	if (log_folder_path_formatted)
	{
		free(log_folder_path_formatted);
	}
	if (handle)
	{
		if (handle->msg_queue)
		{
			msg_queue_handler_destroy(&(handle->msg_queue), MSG_Q_HANDLER_DESTROY_FLAGS_NORMALLY);
		}
		if (handle->msg_cache_p)
		{
			free(handle->msg_cache_p);
		}
		free(handle);
	}
	else if (msg)
	{
		free(msg);
	}
	return NULL;
}

int file_logger_run_cleanup_now(file_logger_handle handle)
{
	if (!handle)
	{
		return -1;
	}
	file_logger_try_cleanup((file_logger_t *)handle, true);
	return 0;
}

int file_logger_log(file_logger_handle handle, void *log_msg, size_t msg_size)
{
	/* P1-3 修订: retry sleep 平台分支.
	 * Linux/POSIX: usleep(200us) × 10 = ~2ms (粒度细, 响应快)
	 * Windows: usleep 在 inc/thread/posix_thread.h:23 定义为 Sleep(us/1000),
	 *          故 200us 会退化为 Sleep(0) 仅做线程让步, 反而忙等持锁.
	 *          Windows 维持 Sleep(1) × 2 = ~2-30ms (与原行为等价). */
#ifdef _WIN32
#define _LCU_LOG_RETRY_SLEEP_US (1500)
#define _LCU_LOG_RETRY_MAX_TIMES (2)
#else
#define _LCU_LOG_RETRY_SLEEP_US (200)
#define _LCU_LOG_RETRY_MAX_TIMES (10)
#endif
	if (!handle || !log_msg || 0U == msg_size)
	{
		return -1;
	}
	msg_q_code_e status = MSG_Q_CODE_BUF_NOT_ENOUGH;
	int retry_counter = 0;

	FILE_LOGGER_LOCK(handle);
	do
	{
		if (msg_size > handle->cur_msg_obj_capacity)
		{
			uint32_t new_msg_max_size = integer_roundup_pow_of_two((uint32_t)msg_size);
			size_t new_mem_size = sizeof(queue_msg_t) + new_msg_max_size;
			void *new_mem = realloc(handle->msg_cache_p, new_mem_size);
			if (NULL == new_mem)
			{
				MY_LOGE("err on realloc new_mem(%zu) at %s:%d", new_mem_size, __func__, __LINE__);
				break;
			}
			handle->msg_cache_p = (queue_msg_t *)new_mem;
			handle->cur_msg_obj_capacity = (size_t)new_msg_max_size;
		}

		memcpy(handle->msg_cache_p->obj, log_msg, msg_size);
		handle->msg_cache_p->obj_len = (int)msg_size;
		if (MSG_Q_CODE_SUCCESS == (status = msg_queue_handler_push(handle->msg_queue, handle->msg_cache_p)))
		{
			break; // everything all right, sending completed
		}
		if (false == handle->cfg.is_try_my_best_to_keep_log)
		{
			break; // caution: here we must be lost this log message!
		}
		usleep(_LCU_LOG_RETRY_SLEEP_US);
	} while (MSG_Q_CODE_SUCCESS != status && ++retry_counter < _LCU_LOG_RETRY_MAX_TIMES);

	/* P1-3 修订: lost.log 路径准备(在持锁状态下生成路径字符串, 与 cfg 字段访问需要锁保护),
	 * 但实际的 fopen+fprintf+fclose 移到 unlock 之后, 避免持锁同步磁盘 IO 阻塞其他 producer. */
	bool need_write_lost_log = (MSG_Q_CODE_SUCCESS != status && handle->cfg.is_try_my_best_to_keep_log);
	char lost_path_buffer[MAX_FULL_PATH_SIZE];
	if (need_write_lost_log)
	{
		lost_path_buffer[MAX_FULL_PATH_SIZE - 1] = '\0';
		snprintf(lost_path_buffer, sizeof(lost_path_buffer) - 1, "%s%s_lost.log",
				 handle->cfg.log_folder_path, handle->cfg.log_file_name_prefix);
	}

	FILE_LOGGER_UNLOCK(handle);

	/* 解锁后再写 lost.log, fclose 触发的 fsync 不再阻塞其他 producer.
	 * 注意: lost.log 路径上的写入相对其他 producer 失去严格 FIFO,
	 * 但 lost.log 本身就是兜底场景, 这个 trade-off 可接受. */
	if (need_write_lost_log)
	{
		MY_LOGE(" warning: lost log, you can see it on *_lost.log");
		FILE *f_lost = fopen(lost_path_buffer, "a");
		if (f_lost)
		{
			fprintf(f_lost, "%.*s\n\n", (int)msg_size, (char *)log_msg);
			fclose(f_lost);
		}
	}

	return (int)status;
#undef _LCU_LOG_RETRY_SLEEP_US
#undef _LCU_LOG_RETRY_MAX_TIMES
}

int file_logger_destroy(file_logger_handle *handle_p)
{
	if (NULL == handle_p || NULL == *handle_p)
	{
		return -1;
	}
	file_logger_handle handle = *handle_p;
	if (handle->msg_queue)
	{
		msg_queue_handler_destroy(&(handle->msg_queue), MSG_Q_HANDLER_DESTROY_FLAGS_NORMALLY);
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
		handle->cur_file_path[0] = '\0';
	}
	memset(handle, 0, sizeof(file_logger_t));
	free(handle);
	*handle_p = NULL;
	return 0;
}

static int handle_log_queue_msg(queue_msg_t *msg_p, void *user_data)
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
		if (handle->cur_fp)
		{
			strlcpy(handle->cur_file_path, path_buffer, sizeof(handle->cur_file_path));
		}
		else
		{
			handle->cur_file_path[0] = '\0';
		}
	}
	if (!handle->cur_fp)
	{
#if (!defined(NDEBUG) || defined(_DEBUG))
		fprintf(stderr, "[ERROR] log file handle still null at (%s:%d)!!\n", __FILE__, __LINE__);
#endif // !NDEBUG || _DEBUG
		return 0;
	}
	int write_len = fprintf(handle->cur_fp, "%.*s\n", msg_p->obj_len, msg_p->obj);
	if (write_len > 0) // if error, negative number will returned
	{
		handle->cur_log_file_size_counter += write_len;
	}
#if (!defined(NDEBUG) || defined(_DEBUG))
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
		handle->cur_file_path[0] = '\0';
	}
	file_logger_try_cleanup(handle, false);
	return 0;
}
