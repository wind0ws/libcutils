#include "mem/mem_debug.h"
#include "common_macro.h"
#include "thread/thread_wrapper.h"
#include "sys/dlfcn_wrapper.h"
#include "mem/strings.h"
#include "log/xlog.h"
#include "log/file_logger.h"
#include "libcutils.h"
#include <locale.h> /* for setlocale */
#include <malloc.h>

#ifdef _WIN32
static void setup_console();
#endif // _WIN32

#define RUN_TEST(func_name) do                                             \
{                                                                          \
   LOGD("\n%s\nNow run --> %s()\n", LOG_LINE_STAR, #func_name);            \
   int ret = func_name();                                                  \
   printf("\n");                                                           \
   LOGD("<-- %s() run result=%d\n%s\n", #func_name, ret, LOG_LINE_STAR);   \
   ASSERT(ret == 0);                                                       \
} while (0)


static int memleak_test();

EXTERN_C_START

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

EXTERN_C_END

EXTERN_C
int main(int argc, char* argv[])
{
#ifdef _WIN32
	setup_console();
#ifdef _DEBUG
	// create log file, do not close it at end of main, because crt will write log to it.
	HANDLE hLogFile = CreateFile("./memleak.log", GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ,
		NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	// dump is in warn level. let warn log to file and debug console.
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
	_CrtSetReportFile(_CRT_WARN, hLogFile);
#endif // _DEBUG
#endif // _WIN32

	INIT_MEM_CHECK();

#if TEST_FILE_LOGGER
	ASSERT(file_logger_test_begin() == 0);
#endif

	LOGI("hello world: LCU_VER:%s\n", libcutils_get_version());
	//ASSERT_ABORT(1 == 0);

	//RUN_TEST(memleak_test);//this will report mem leak.
	//RUN_TEST(file_util_test);
	//RUN_TEST(basic_test);
	//RUN_TEST(autocover_buffer_test);
	//RUN_TEST(strings_test);
	//RUN_TEST(mplite_test);
	//RUN_TEST(thpool_test);
	//RUN_TEST(string_test);
	//RUN_TEST(time_util_test);
	//RUN_TEST(thread_wrapper_test);
	RUN_TEST(url_encoder_decoder_test);
	RUN_TEST(base64_test);

	
#if TEST_FILE_LOGGER
	ASSERT(file_logger_test_end() == 0);
#endif

	LOGI("...bye bye...\n");

	DEINIT_MEM_CHECK();
	return 0;
}

static int memleak_test()
{
	int ret = allocator_test();
	char* leak_mem = new char[16];
	memset(leak_mem, 0xFF, 16);
	//memset(leak_mem, 0xFF, 18); // will report memory corruption
	LOGD("leak_mem=0x%p, leak_mem[0]=%d", &leak_mem[0], leak_mem[0]);
	//delete[] leak_mem;
	return ret;
}

#ifdef _WIN32

static void setup_console()
{
	// set locale for support Chinese filename/output.
	// should also add /utf-8 option to compiler and make sure your source file save as utf-8.
	setlocale(LC_CTYPE, ".utf8");
	//FILE* f = fopen(u8"D:\\迅雷下载\\你好.txt", "rb");//ok
	//ASSERT_ABORT(f);
	//fclose(f);
	//LOGD("你好");
}

#endif // _WIN32