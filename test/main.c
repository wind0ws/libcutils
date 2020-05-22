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

typedef struct xlog_user_data_pack
{
	file_logger_handle logger_hdl;
	pthread_mutex_t log_mutex;
}xlog_user_data_pack_t;
static void xlog_custom_user_cb(void* log_msg, void* user_data);

int main(int argc, char* argv[])
{
#ifdef _WIN32
	//char* pChars = (char *)malloc(10);
	EnableMemLeakCheck();
#endif // _WIN32
	file_logger_cfg f_logger_cfg = { 0 };
	//f_logger_cfg.one_piece_file_max_len = 1024;
	strcpy(f_logger_cfg.log_folder_path, "D:\\log");
	strcpy(f_logger_cfg.log_file_name_prefix, "libcutils");
	file_logger_handle f_logger_hdl = file_logger_init(f_logger_cfg);
	if (NULL == f_logger_hdl)
	{
		return 1;
	}
	xlog_user_data_pack_t x_user_data_pack = {
		.logger_hdl = f_logger_hdl,
		.log_mutex = {0}
	};
	pthread_mutex_init(&x_user_data_pack.log_mutex, NULL);
	xlog_set_user_callback(xlog_custom_user_cb, (void*)&x_user_data_pack);
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
	pthread_mutex_destroy(&x_user_data_pack.log_mutex);
	//optional: remove xlog user callback
	xlog_set_user_callback(NULL, NULL);
	return 0;
}

static void xlog_custom_user_cb(void* log_msg, void* user_data)
{
	xlog_user_data_pack_t* x_user_data_pack_p = (xlog_user_data_pack_t*)user_data;
	if (NULL == x_user_data_pack_p || NULL == log_msg)
	{
		return;
	}
	file_logger_handle f_logger_hdl = x_user_data_pack_p->logger_hdl;
	pthread_mutex_lock(&x_user_data_pack_p->log_mutex);
	//for multi thread support, we should protect file logger
	//if you call xlog only on one thread, no need lock at all.
	file_logger_log(f_logger_hdl, log_msg);
	pthread_mutex_unlock(&x_user_data_pack_p->log_mutex);
}