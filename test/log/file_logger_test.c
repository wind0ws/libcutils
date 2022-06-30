#include "log/file_logger.h"
#include "thread/thread_wrapper.h"
#include "log/xlog.h"
#include "mem/strings.h"

static void my_xlog_custom_user_cb(LogLevel level, void* log_msg, size_t msg_size, void* user_data);
static void my_file_logger_lock(void* arg);
static void my_file_logger_unlock(void* arg);

typedef struct
{
	file_logger_cfg f_logger_cfg;
	pthread_mutex_t log_mutex;
	file_logger_handle f_logger_hdl;
} logger_context_t;

static logger_context_t g_logge_ctx;

#ifdef _WIN32
#define FILE_LOGGER_PATH ("D:/temp/log") 
#define STDOUT_FILE_PATH ("D:/stdout2file.log")
#else
#define FILE_LOGGER_PATH ("./log/") 
#define STDOUT_FILE_PATH ("stdout2file.log")
#endif // _WIN32

int file_logger_test_begin()
{
	memset(&g_logge_ctx, 0, sizeof(logger_context_t));
	g_logge_ctx.f_logger_cfg.log_queue_size = 128;
	g_logge_ctx.f_logger_cfg.is_try_my_best_to_keep_log = true;
	//g_logger_cfg.f_logger_cfg.one_piece_file_max_len = 1024;//auto slice log file
	strcpy(g_logge_ctx.f_logger_cfg.log_folder_path, FILE_LOGGER_PATH);
	strcpy(g_logge_ctx.f_logger_cfg.log_file_name_prefix, "libcutils");
	//for multi thread support, we should protect file logger
	//if you call xlog only on one thread, no need lock at all.
	g_logge_ctx.f_logger_cfg.lock.acquire = (void*)&my_file_logger_lock;
	g_logge_ctx.f_logger_cfg.lock.release = (void*)&my_file_logger_unlock;

	pthread_mutex_init(&g_logge_ctx.log_mutex, NULL);
	g_logge_ctx.f_logger_cfg.lock.arg = (void*)&g_logge_ctx.log_mutex;

	g_logge_ctx.f_logger_hdl = file_logger_init(&g_logge_ctx.f_logger_cfg);
	if (NULL == g_logge_ctx.f_logger_hdl)
	{
		pthread_mutex_destroy(&g_logge_ctx.log_mutex);
		return 1;
	}
	xlog_set_user_callback(my_xlog_custom_user_cb, (void*)g_logge_ctx.f_logger_hdl);
	xlog_set_target(LOG_TARGET_ANDROID | LOG_TARGET_CONSOLE | LOG_TARGET_USER_CALLBACK);
	LOGD("Now call xlog_stdout2file");
	LOG_STD2FILE(STDOUT_FILE_PATH);

	return 0;
}

int file_logger_test_end()
{
	LOG_BACK2STD();
	LOGI("Now back to stdout");
	//optional: give some time to finish log on file
	usleep(10000);
	file_logger_deinit(&g_logge_ctx.f_logger_hdl);

	pthread_mutex_destroy(&g_logge_ctx.log_mutex);
	//optional: remove xlog user callback
	xlog_set_user_callback(NULL, NULL);

	return 0;
}

static void my_file_logger_lock(void* arg)
{
	pthread_mutex_lock((pthread_mutex_t*)arg);
}

static void my_file_logger_unlock(void* arg)
{
	pthread_mutex_unlock((pthread_mutex_t*)arg);
}

static void my_xlog_custom_user_cb(LogLevel level, void* log_msg, size_t msg_size, void* user_data)
{
	file_logger_handle f_logger_hdl = (file_logger_handle)user_data;
	if (NULL == f_logger_hdl || NULL == log_msg)
	{
		return;
	}
	file_logger_log(f_logger_hdl, log_msg, msg_size);
}
