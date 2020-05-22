#include <malloc.h>
#include "apicheck.h"
#include "common_macro.h"
#include "thread_wrapper.h"
#include "strings.h"
#include "xlog.h"
#include "file_logger.h"

#ifdef _WIN32
#define CRTDBG_MAP_ALLOC    
#include <stdlib.h>    
#include <crtdbg.h>   
//该函数可以放在主函数的任意位置，都能正确的触发内存泄露输出    
static inline void EnableMemLeakCheck()
{
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_LEAK_CHECK_DF);
}
#endif // _WIN32

#define RUN_TEST(func_name) do \
{\
LOGD("\r\n%s\r\nNow run --> %s()",LOG_LINE_STAR, #func_name);\
int ret = func_name(); \
LOGD("\r\n <-- %s() run result=%d\r\n%s\r\n", #func_name, ret, LOG_LINE_STAR);\
api_check_return_val(ret == 0, -1);\
} while (0)

extern int thread_wrapper_test();
extern int basic_test();
extern int autocover_buffer_test();
extern int strings_test();
extern int mplite_test();

static void xlog_custom_user_cb(void* log_msg, void* user_data);
static void my_file_logger_lock(void *arg);
static void my_file_logger_unlock(void *arg);

int main(int argc, char* argv[])
{
#ifdef _WIN32
	//char* pChars = (char *)malloc(10);
	EnableMemLeakCheck();
#endif // _WIN32

	file_logger_cfg f_logger_cfg = { 0 };
	//f_logger_cfg.one_piece_file_max_len = 1024;//auto slice log file
	strcpy(f_logger_cfg.log_folder_path, "D:\\log");
	strcpy(f_logger_cfg.log_file_name_prefix, "libcutils");
	//for multi thread support, we should protect file logger
	//if you call xlog only on one thread, no need lock at all.
	f_logger_cfg.lock.acquire = (void *)&my_file_logger_lock;
	f_logger_cfg.lock.release = (void *)&my_file_logger_unlock;
	pthread_mutex_t log_mutex = {0};
	pthread_mutex_init(&log_mutex, NULL);
	f_logger_cfg.lock.arg = (void *)&log_mutex;

	file_logger_handle f_logger_hdl = file_logger_init(f_logger_cfg);
	if (NULL == f_logger_hdl)
	{
		return 1;
	}
	xlog_set_user_callback(xlog_custom_user_cb, (void *)f_logger_hdl);
	xlog_set_target(LOG_TARGET_ANDROID | LOG_TARGET_CONSOLE | LOG_TARGET_USER_CALLBACK);
	
	LOGI("hello world\r\n");

	//RUN_TEST(basic_test);
	RUN_TEST(thread_wrapper_test);
	//RUN_TEST(autocover_buffer_test);
	//RUN_TEST(strings_test);
	//RUN_TEST(mplite_test);

	LOGI("...bye bye...");

	//optional: give some time to finish log on file
	usleep(10000);
	file_logger_deinit(&f_logger_hdl);
	pthread_mutex_destroy(&log_mutex);
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

static void xlog_custom_user_cb(void* log_msg, void* user_data)
{
	file_logger_handle f_logger_hdl = (file_logger_handle)user_data;
	if (NULL == f_logger_hdl || NULL == log_msg)
	{
		return;
	}
	file_logger_log(f_logger_hdl, log_msg);
}