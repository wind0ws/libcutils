#include <malloc.h>
#include "common_macro.h"
#include "thread/thread_wrapper.h"
#include "sys/dlfcn_wrapper.h"
#include "mem/strings.h"
#include "log/xlog.h"
#include "log/file_logger.h"
#include "libcutils.h"

#ifdef _WIN32
#define CRTDBG_MAP_ALLOC    
#include <stdlib.h>    
#include <crtdbg.h>   
static inline void EnableMemLeakCheck()
{
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_LEAK_CHECK_DF);
}
#endif // _WIN32

#define RUN_TEST(func_name) do \
{\
LOGD("\r\n%s\r\nNow run --> %s()\r\n",LOG_LINE_STAR, #func_name);\
int ret = func_name(); \
printf("\r\n"); \
LOGD("<-- %s() run result=%d\r\n%s\r\n", #func_name, ret, LOG_LINE_STAR);\
ASSERT(ret == 0);\
} while (0)

#define TEST_ALLOCATOR (1)
extern int allocator_test_begin();
extern int allocator_test_end();
extern int allocator_test();

#define TEST_FILE_LOGGER (0)
extern int file_logger_test_begin();
extern int file_logger_test_end();

extern int thread_wrapper_test();
extern int basic_test();
extern int autocover_buffer_test();
extern int mplite_test();
extern int file_util_test();
extern int thpool_test();
extern int string_test();
extern int time_util_test();
extern int url_encoder_decoder_test();
extern int base64_test();


int main(int argc, char* argv[])
{
#ifdef _WIN32
	//char* pChars = (char *)malloc(10);
	EnableMemLeakCheck();

   // Set console character encoding, for support Chinese
   // system("chcp 65001"); 
	SetConsoleOutputCP(65001);
	CONSOLE_FONT_INFOEX console_font_info = { 0 };
	console_font_info.cbSize = sizeof(console_font_info);
	console_font_info.dwFontSize.Y = 16; // leave X as zero
	console_font_info.FontWeight = FW_NORMAL;
	wcscpy_s(console_font_info.FaceName, _countof(console_font_info.FaceName), L"Consolas");
	SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), false, &console_font_info);
#endif // _WIN32

#if TEST_ALLOCATOR
	allocator_test_begin();
#endif

#if TEST_FILE_LOGGER
	ASSERT(file_logger_test_begin() == 0);
#endif

	LOGI("hello world: LCU_VER:%s \r\n", libcutils_get_version());

	//RUN_TEST(allocator_test);//this will report mem leak.
	//RUN_TEST(file_util_test);
	//RUN_TEST(basic_test);
	//RUN_TEST(autocover_buffer_test);
	//RUN_TEST(strings_test);
	//RUN_TEST(mplite_test);
	//RUN_TEST(thpool_test);
	//RUN_TEST(string_test);
	//RUN_TEST(time_util_test);
	//RUN_TEST(thread_wrapper_test);
	//RUN_TEST(url_encode_test);
	RUN_TEST(url_encoder_decoder_test);
	RUN_TEST(base64_test);

#if TEST_FILE_LOGGER
	ASSERT(file_logger_test_end() == 0);
#endif
	
	LOGI("...bye bye...");

#if TEST_ALLOCATOR
	allocator_test_end();
#endif
	return 0;
}

