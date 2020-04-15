#include <malloc.h>
#include <assert.h>
#include "xlog.h"

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
LOGD("\r\n%s\r\nNow run %s()",LOG_LINE_STAR, #func_name);\
int ret = func_name(); \
LOGD("%s() run result=%d\r\n%s\r\n", #func_name, ret, LOG_LINE_STAR);\
assert(ret == 0);\
} while (0);

extern int thread_wrapper_test();
extern int basic_test();
extern int autocover_buffer_test();

int main()
{
#ifdef _WIN32
	//char* pChars = (char *)malloc(10);
    EnableMemLeakCheck();
#endif // _WIN32
    LOGI("hello world\r\n");

    //RUN_TEST(basic_test)
    //RUN_TEST(thread_wrapper_test)
    RUN_TEST(autocover_buffer_test)

    LOGI("...bye bye...");
    return 0;
}

