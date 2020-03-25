#include <malloc.h>
#include <assert.h>
#include "xlog.h"

#define RUN_TEST(func_name) do \
{\
LOGD("\r\n%s\r\nNow run %s()",LOG_LINE_STAR, #func_name);\
int ret = func_name(); \
LOGD("%s() run result=%d\r\n%s\r\n", #func_name, ret, LOG_LINE_STAR);\
assert(ret == 0);\
} while (0);

extern int thread_wrapper_test();

int main()
{
    LOGI("hello world");

    RUN_TEST(thread_wrapper_test)

    LOGI("bye bye...");
    return 0;
}

