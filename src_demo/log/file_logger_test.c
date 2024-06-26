#include "log/file_logger.h"
#include "thread/posix_thread.h"
#include "log/xlog.h"
#include "mem/strings.h"

static void my_xlog_custom_user_cb(LogLevel level, void* log_msg, size_t msg_size, void* user_data);

typedef struct
{
	file_logger_cfg logger_cfg;
	file_logger_handle logger_hdl;
} logger_context_t;

static logger_context_t g_logger_ctx;

#ifdef _WIN32
#define FILE_LOGGER_PATH ("D:/temp/log") 
#define STDOUT_FILE_PATH ("D:/stdout2file.log")
#else
#define FILE_LOGGER_PATH ("./log/") 
#define STDOUT_FILE_PATH ("stdout2file.log")
#endif // _WIN32

int file_logger_test_begin()
{
	memset(&g_logger_ctx, 0, sizeof(logger_context_t));
	g_logger_ctx.logger_cfg.log_queue_size = 128U;
	g_logger_ctx.logger_cfg.is_try_my_best_to_keep_log = true;
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
	xlog_set_user_callback(my_xlog_custom_user_cb, (void*)g_logger_ctx.logger_hdl);
	xlog_set_target(LOG_TARGET_ANDROID | LOG_TARGET_CONSOLE | LOG_TARGET_USER_CALLBACK);
	LOGD("Now call xlog_stdout2file");
	LOG_STD2FILE(STDOUT_FILE_PATH);

	return 0;
}

int file_logger_test_end()
{
	LOG_BACK2STD();
	LOGI("Now back to stdout");
	//remove xlog user callback
	xlog_set_user_callback(NULL, NULL);
	xlog_set_target(LOG_TARGET_ANDROID | LOG_TARGET_CONSOLE);
	//optional: give some time to finish log on file
	usleep(10000);
	file_logger_deinit(&g_logger_ctx.logger_hdl);
	
	LOGI("\"%s\" main log content should as same as \"%s\"", STDOUT_FILE_PATH, FILE_LOGGER_PATH);
	return 0;
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
