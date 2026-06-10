#include "log/file_logger.h"
#include "thread/posix_thread.h"
#include "log/xlog.h"
#include "mem/strings.h"
#include "common_macro.h"
#include "file/file_util.h"
#include "sys/dirent.h"

#ifdef _WIN32
#include <sys/utime.h>
#else
#include <utime.h>
#endif

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>


static void my_xlog_custom_user_cb(LogLevel level, void* log_msg, size_t msg_size, void* user_data);

typedef struct
{
	file_logger_cfg logger_cfg;
	file_logger_handle logger_hdl;
} logger_context_t;

static logger_context_t g_logger_ctx;

#define TEST_LOG_RETENTION_DAYS (2U)
#define TEST_LOG_TOTAL_LIMIT_BYTES (30U * 1024U)
#define TEST_LOG_PATH_BUFFER (512)

static void file_logger_cleanup_feature_tests(void);
static void cleanup_log_directory(void);
static void create_dummy_log_file(const char *file_name, size_t file_size, time_t modified_time);
static bool does_log_file_exist(const char *file_name);
static void set_file_time(const char *full_path, time_t modified_time);
static uint64_t sum_log_files_size(void);
static void test_file_logger_oversized_msg(void);  /* C-2 测试声明 */

#ifdef _WIN32
#define FILE_LOGGER_PATH ("./log/")
#define STDOUT_FILE_PATH ("./stdout2file.log")
#else
#define FILE_LOGGER_PATH ("./log/")
#define STDOUT_FILE_PATH ("stdout2file.log")
#endif // _WIN32

int file_logger_test_begin(void)
{
	memset(&g_logger_ctx, 0, sizeof(logger_context_t));
	g_logger_ctx.logger_cfg.log_queue_size = 128U;
	g_logger_ctx.logger_cfg.is_try_my_best_to_keep_log = true;
	g_logger_ctx.logger_cfg.max_log_retention_days = TEST_LOG_RETENTION_DAYS;
	g_logger_ctx.logger_cfg.max_total_log_storage_bytes = TEST_LOG_TOTAL_LIMIT_BYTES;
	//g_logger_cfg.f_logger_cfg.one_piece_file_max_len = 1024;//auto slice log file
	strcpy(g_logger_ctx.logger_cfg.log_folder_path, FILE_LOGGER_PATH);
	strcpy(g_logger_ctx.logger_cfg.log_file_name_prefix, "lcu_");

	// here we are not provide lock for file_logger,
	// because xlog will ensure printing order.
	g_logger_ctx.logger_hdl = file_logger_init(&g_logger_ctx.logger_cfg);
	if (NULL == g_logger_ctx.logger_hdl)
	{
		return 1;
	}
	file_logger_cleanup_feature_tests();

	/* C-2 测试调用 */
	test_file_logger_oversized_msg();

	xlog_set_user_callback(my_xlog_custom_user_cb, (void*)g_logger_ctx.logger_hdl);
	xlog_set_target(LOG_TARGET_ANDROID | LOG_TARGET_CONSOLE | LOG_TARGET_USER_CALLBACK);
	LOGD("Now call xlog_stdout2file");
	LOG_STD2FILE(STDOUT_FILE_PATH);

	return 0;
}

int file_logger_test_end(void)
{
	LOG_BACK2STD();
	LOGI("Now back to stdout");
	//remove xlog user callback
	xlog_set_user_callback(NULL, NULL);
	xlog_set_target(LOG_TARGET_ANDROID | LOG_TARGET_CONSOLE);
	//optional: give some time to finish log on file
	usleep(10000);
	file_logger_destroy(&g_logger_ctx.logger_hdl);
	
	LOGI("\"%s\" main log content should as same as \"%s\"", STDOUT_FILE_PATH, FILE_LOGGER_PATH);
	return 0;
}

static void file_logger_cleanup_feature_tests(void)
{
	ASSERT(g_logger_ctx.logger_hdl);
	ASSERT(0 == file_util_mkdirs(g_logger_ctx.logger_cfg.log_folder_path));
	cleanup_log_directory();
	time_t now = time(NULL);
	const char *old_file = "lcu_cleanup_old.log";
	const char *recent_file = "lcu_cleanup_recent.log";
	time_t old_time = now - (time_t)(TEST_LOG_RETENTION_DAYS + 1U) * 24 * 3600;
	create_dummy_log_file(old_file, 1024U, old_time);
	create_dummy_log_file(recent_file, 1024U, now);
	ASSERT(does_log_file_exist(old_file));
	ASSERT(does_log_file_exist(recent_file));
	ASSERT(0 == file_logger_run_cleanup_now(g_logger_ctx.logger_hdl));
	ASSERT(false == does_log_file_exist(old_file));
	ASSERT(true == does_log_file_exist(recent_file));
	cleanup_log_directory();
	size_t chunk_size = (TEST_LOG_TOTAL_LIMIT_BYTES / 2U);
	if (0 == chunk_size)
	{
		chunk_size = 1024U;
	}
	create_dummy_log_file("lcu_cleanup_size_a.log", chunk_size, now - 3);
	create_dummy_log_file("lcu_cleanup_size_b.log", chunk_size, now - 2);
	create_dummy_log_file("lcu_cleanup_size_c.log", chunk_size, now - 1);
	ASSERT(0 == file_logger_run_cleanup_now(g_logger_ctx.logger_hdl));
	uint64_t total_size = sum_log_files_size();
	ASSERT(total_size <= TEST_LOG_TOTAL_LIMIT_BYTES);
	ASSERT(false == does_log_file_exist("lcu_cleanup_size_a.log"));
	cleanup_log_directory();
}

static void cleanup_log_directory(void)
{
	file_util_mkdirs(g_logger_ctx.logger_cfg.log_folder_path);
	DIR *dir = opendir(g_logger_ctx.logger_cfg.log_folder_path);
	if (!dir)
	{
		return;
	}
	const size_t prefix_len = strlen(g_logger_ctx.logger_cfg.log_file_name_prefix);
	struct dirent *entry = NULL;
	while ((entry = readdir(dir)) != NULL)
	{
		if ('.' == entry->d_name[0])
		{
			continue;
		}
		if (prefix_len > 0U && strncmp(entry->d_name, g_logger_ctx.logger_cfg.log_file_name_prefix, prefix_len) != 0)
		{
			continue;
		}
		char full_path[TEST_LOG_PATH_BUFFER];
		int written = snprintf(full_path, sizeof(full_path), "%s%s", g_logger_ctx.logger_cfg.log_folder_path, entry->d_name);
		if (written <= 0 || written >= (int)sizeof(full_path))
		{
			continue;
		}
		remove(full_path);
	}
	closedir(dir);
}

static void create_dummy_log_file(const char *file_name, size_t file_size, time_t modified_time)
{
	char full_path[TEST_LOG_PATH_BUFFER];
	int written = snprintf(full_path, sizeof(full_path), "%s%s", g_logger_ctx.logger_cfg.log_folder_path, file_name);
	ASSERT(written > 0 && written < (int)sizeof(full_path));
	FILE *fp = fopen(full_path, "wb");
	ASSERT(fp);
	const char pattern[] = "0123456789ABCDEF";
	size_t remaining = file_size;
	while (remaining > 0U)
	{
		size_t chunk = remaining > sizeof(pattern) ? sizeof(pattern) : remaining;
		ASSERT(chunk == fwrite(pattern, 1, chunk, fp));
		remaining -= chunk;
	}
	fflush(fp);
	fclose(fp);
	set_file_time(full_path, modified_time);
}

static bool does_log_file_exist(const char *file_name)
{
	char full_path[TEST_LOG_PATH_BUFFER];
	int written = snprintf(full_path, sizeof(full_path), "%s%s", g_logger_ctx.logger_cfg.log_folder_path, file_name);
	if (written <= 0 || written >= (int)sizeof(full_path))
	{
		return false;
	}
	FILE *fp = fopen(full_path, "rb");
	if (fp)
	{
		fclose(fp);
		return true;
	}
	return false;
}

static void set_file_time(const char *full_path, time_t modified_time)
{
#ifdef _WIN32
	struct __utimbuf64 new_time = {0};
	new_time.actime = modified_time;
	new_time.modtime = modified_time;
	_utime64(full_path, &new_time);
#else
	struct utimbuf new_time;
	new_time.actime = modified_time;
	new_time.modtime = modified_time;
	utime(full_path, &new_time);
#endif
}

static uint64_t sum_log_files_size(void)
{
	DIR *dir = opendir(g_logger_ctx.logger_cfg.log_folder_path);
	if (!dir)
	{
		return 0U;
	}
	uint64_t total = 0U;
	const size_t prefix_len = strlen(g_logger_ctx.logger_cfg.log_file_name_prefix);
	struct dirent *entry = NULL;
	while ((entry = readdir(dir)) != NULL)
	{
		if ('.' == entry->d_name[0])
		{
			continue;
		}
		if (prefix_len > 0U && strncmp(entry->d_name, g_logger_ctx.logger_cfg.log_file_name_prefix, prefix_len) != 0)
		{
			continue;
		}
		char full_path[TEST_LOG_PATH_BUFFER];
		int written = snprintf(full_path, sizeof(full_path), "%s%s", g_logger_ctx.logger_cfg.log_folder_path, entry->d_name);
		if (written <= 0 || written >= (int)sizeof(full_path))
		{
			continue;
		}
		FILE *fp = fopen(full_path, "rb");
		if (!fp)
		{
			continue;
		}
		if (0 == fseek(fp, 0, SEEK_END))
		{
			long size = ftell(fp);
			if (size > 0)
			{
				total += (uint64_t)size;
			}
		}
		fclose(fp);
	}
	closedir(dir);
	return total;
}

static void my_xlog_custom_user_cb(LogLevel level, void* log_msg, size_t msg_size, void* user_data)
{
	file_logger_handle f_logger_hdl = (file_logger_handle)user_data;
	if (NULL == f_logger_hdl || NULL == log_msg)
	{
		return;
	}
	// let file_logger to write it
	file_logger_log(f_logger_hdl, log_msg, msg_size);
}

/* C-2 + 4.1 回归测试：拒绝超大日志消息 */
static void test_file_logger_oversized_msg(void)
{
	LOGI("[C-2] Testing oversized message rejection...");

	/* 测试 INT_MAX 边界 */
	char small_msg[16] = "test";
	int ret_ok = file_logger_log(g_logger_ctx.logger_hdl, small_msg, sizeof(small_msg));
	ASSERT(ret_ok == 0);  /* 正常消息应成功 */

	/* 测试超大消息（模拟 >INT_MAX，实际分配小 buffer 避免 OOM） */
	size_t huge_size = (size_t)INT_MAX + 1;
	ret_ok = file_logger_log(g_logger_ctx.logger_hdl, small_msg, huge_size);
	ASSERT(ret_ok == -1);  /* 应拒绝 */

	LOGI("[C-2] PASS: oversized message correctly rejected");
}
