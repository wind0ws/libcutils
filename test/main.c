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

extern int file_logger_test_begin();
extern int file_logger_test_end();

extern int thread_wrapper_test();
extern int basic_test();
extern int autocover_buffer_test();
extern int strings_test();
extern int mplite_test();
extern int file_util_test();

int main(int argc, char* argv[])
{
#ifdef _WIN32
	//char* pChars = (char *)malloc(10);
	EnableMemLeakCheck();
#endif // _WIN32

	file_logger_test_begin();

	LOGI("hello world\r\n");

	RUN_TEST(file_util_test);
	//RUN_TEST(basic_test);
	//RUN_TEST(thread_wrapper_test);
	//RUN_TEST(autocover_buffer_test);
	//RUN_TEST(strings_test);
	//RUN_TEST(mplite_test);

	file_logger_test_end();
	
	LOGI("...bye bye...");
	return 0;
}

