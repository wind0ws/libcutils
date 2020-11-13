#include "mem/mem_debug.h"
#include "common_macro.h"
#include "thread/thread_wrapper.h"
#include "sys/dlfcn_wrapper.h"
#include "mem/strings.h"
#include "log/xlog.h"
#include "log/file_logger.h"
#include "libcutils.h"
#include <locale.h> /* for setlocale */

static void setup_console();

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
extern int ini_test();

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
extern int str_params_test();

EXTERN_C_END

#define SAVE_LOG 1

#if SAVE_LOG && TEST_FILE_LOGGER == 0
#define STDOUT2FILE() xlog_stdout2file("d:/mylog.log")
#define BACK2STDOUT() xlog_back2stdout()
#else
#define STDOUT2FILE()
#define BACK2STDOUT()
#endif

EXTERN_C
int main(int argc, char* argv[])
{
	setup_console();

	INIT_MEM_CHECK();

	LOGI("hello world: LCU_VER:%s\n", libcutils_get_version());

#if TEST_FILE_LOGGER
	ASSERT(file_logger_test_begin() == 0);
#endif
	STDOUT2FILE();

	//ASSERT_ABORT(1 == 0);

	//RUN_TEST(memleak_test);//this will report mem leak.
	//RUN_TEST(file_util_test);
	//RUN_TEST(ini_test);
	//RUN_TEST(basic_test);
	//RUN_TEST(autocover_buffer_test);
	//RUN_TEST(mplite_test);
	//RUN_TEST(thpool_test);
	//RUN_TEST(string_test);
	RUN_TEST(time_util_test);
	//RUN_TEST(thread_wrapper_test);
	//RUN_TEST(url_encoder_decoder_test);
	//RUN_TEST(base64_test);
	//RUN_TEST(str_params_test);

#if TEST_FILE_LOGGER
	ASSERT(file_logger_test_end() == 0);
#endif
	BACK2STDOUT();

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

static void setup_console()
{
#ifdef _WIN32
	// set locale for support Chinese filename/output.
	// should also add /utf-8 option to compiler and make sure your source file save as utf-8.
	setlocale(LC_CTYPE, ".utf8");
	//FILE* f = fopen(u8"D:\\迅雷下载\\你好.txt", "rb");//ok
	//ASSERT_ABORT(f);
	//fclose(f);
	//LOGD("你好");

	//_CrtSetBreakAlloc(104);
#endif // _WIN32
}
