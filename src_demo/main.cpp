#include "mem/mem_debug.h"
#include "common_macro.h"
#include "thread/posix_thread.h"
#include "sys/dlfcn_wrapper.h"
#include "mem/strings.h"

#define LOG_TAG "MAIN"
#include "log/logger.h"
#include "log/file_logger.h"
#include "lcu.h"             /* for lcu_get_version */
#include <locale.h>          /* for setlocale */
#ifdef _WIN32
#include <conio.h>           /* for kbhit */
#endif // _WIN32

#define KB_TIMEOUT      (5)   // seconds

typedef int (*func_prototype_test_case_t)();
#define DECLARE_TEST_FUNC(func_name) extern int func_name()

#define RUN_TEST(func_name) do                                                            \
{                                                                                         \
   LOGD("\n%s\n--> %s() executing...\n", LOG_STAR_LINE, #func_name);                      \
   int ret_##func_name = func_name();                                                     \
   LOGD("\n<-- %s() finished with %d\n%s\n", #func_name, ret_##func_name, LOG_STAR_LINE); \
   ASSERT_ABORT(0 == ret_##func_name);                                                    \
} while (0)

typedef struct
{
	func_prototype_test_case_t p_func;
	const char* str_description;
} test_case_t;

static void setup_console();
static int run_test_case_from_user();
static int memleak_test();

EXTERN_C_START

DECLARE_TEST_FUNC(allocator_test);

#define TEST_FILE_LOGGER (0)
#if(_LCU_LOGGER_TYPE_XLOG != LCU_LOGGER_SELECTOR && 0 != TEST_FILE_LOGGER)
#error "FILE_LOGGER only support XLOG!"
#endif
DECLARE_TEST_FUNC(file_logger_test_begin);
DECLARE_TEST_FUNC(file_logger_test_end);
DECLARE_TEST_FUNC(ini_test);

DECLARE_TEST_FUNC(posix_thread_test);
DECLARE_TEST_FUNC(basic_test);
DECLARE_TEST_FUNC(autocover_buffer_test);
DECLARE_TEST_FUNC(mplite_test);
DECLARE_TEST_FUNC(file_util_test);
DECLARE_TEST_FUNC(thpool_test);
DECLARE_TEST_FUNC(string_test);
DECLARE_TEST_FUNC(time_util_test);

DECLARE_TEST_FUNC(url_encoder_decoder_test);
DECLARE_TEST_FUNC(base64_test);
DECLARE_TEST_FUNC(str_params_test);
DECLARE_TEST_FUNC(msg_queue_handler_test);
DECLARE_TEST_FUNC(integer_test);
DECLARE_TEST_FUNC(list_test);

EXTERN_C_END

static test_case_t g_all_test_cases[] =
{
	{ ini_test, "test ini" },
	{ basic_test, "simple fwrite test case" },
	{ autocover_buffer_test, "test auto-cover buffer" },
	{ mplite_test, "test mem-pool" },
	{ file_util_test, "test file util" },
	{ posix_thread_test, "test posix_thread_test and xlog"},
	{ thpool_test, "test thread pool" },
	{ string_test, "test string op" },
	{ time_util_test, "test time op" },
	{ url_encoder_decoder_test, "test url encoder/decoder" },
	{ base64_test, "test base64" },
	{ str_params_test, "test string params" },
	{ msg_queue_handler_test, "test msg queue handler" },
	{ integer_test, "test integer" },
	{ list_test, "test list" },
};


#define TEST_SAVE_LOG    (0)

#if( TEST_SAVE_LOG && 0 == TEST_FILE_LOGGER )
#ifdef _WIN32
#define LOG_PATH ("d:/mylog.log")
#else
#define LOG_PATH ("mylog.log")
#endif // _WIN32
#define STDOUT2FILE() do{ fprintf(stderr, "\n ==> redirect print to file \n"); LOG_STD2FILE(LOG_PATH); } while(0)
#define BACK2STDOUT() do{ LOG_BACK2STD(); fprintf(stderr, "\n <== now redirect print to console \n"); } while(0)
#else
#define STDOUT2FILE() do { } while (0)
#define BACK2STDOUT() do { } while (0)
#endif

EXTERN_C
int main(int argc, char* argv[])
{
	int ret = 0;
	MEM_CHECK_INIT();
	lcu_init();
	setup_console();
	LOGI("hello world: LCU_VER:%s\n", lcu_get_version());

	bool is_press_kb = true; // default status is true for unix
#if _WIN32
	LOGI("after %d seconds, it will run automatically. if you want to choose test case, just press any key", KB_TIMEOUT);
	clock_t tstart = clock();
	int pressed_char = 'y';                   // default key press
	while ((clock() - tstart) / CLOCKS_PER_SEC < KB_TIMEOUT)
	{
		if ((is_press_kb = (0 != _kbhit())))
		{
			pressed_char = _getch();
			break;
		}
		usleep(500000);
	}
	//if (tolower(pressed_char) == 'y')
	//	printf("you pressed y\n");
#endif //_WIN32

#if TEST_FILE_LOGGER
	ASSERT(file_logger_test_begin() == 0);
#endif
	STDOUT2FILE();

	do
	{
		//ASSERT_ABORT(1 == 0);
		if (is_press_kb)
		{
			ret = run_test_case_from_user();
			break;
		}
		
#if TEST_SAVE_LOG
		fprintf(stderr, "\n  ====auto run test case====  \n");
#endif // TEST_SAVE_LOG
		LOGI("  ====auto run test case====  ");
		//RUN_TEST(memleak_test);//this will report mem leak.
		//RUN_TEST(file_util_test);
		//RUN_TEST(ini_test);
		//RUN_TEST(basic_test);
		//RUN_TEST(autocover_buffer_test);
		//RUN_TEST(mplite_test);
		//RUN_TEST(thpool_test);
		//RUN_TEST(string_test);
		//RUN_TEST(time_util_test);
		RUN_TEST(posix_thread_test);
		//RUN_TEST(url_encoder_decoder_test);
		//RUN_TEST(base64_test);
		//RUN_TEST(str_params_test);
		//RUN_TEST(msg_queue_handler_test);
		//RUN_TEST(integer_test);
		//RUN_TEST(list_test);
	} while (0);

#if TEST_FILE_LOGGER
	ASSERT(file_logger_test_end() == 0);
#endif
	BACK2STDOUT();

	LOGI("...bye bye...  %d\n", ret);

	LOG_DEINIT(NULL);
	lcu_deinit();
	MEM_CHECK_DEINIT();
	return ret;
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

	//_CrtSetBreakAlloc(99);
#endif // _WIN32
	LOG_INIT(NULL);
	LOG_SET_MIN_LEVEL(LOG_LEVEL_VERBOSE);
#if(_LCU_LOGGER_TYPE_XLOG == LCU_LOGGER_SELECTOR)
	xlog_set_format(LOG_FORMAT_WITH_TIMESTAMP | LOG_FORMAT_WITH_TAG_LEVEL | LOG_FORMAT_WITH_TID);
	//xlog_set_format(LOG_FORMAT_WITH_TIMESTAMP | LOG_FORMAT_WITH_TAG_LEVEL);
	//xlog_set_format(LOG_FORMAT_WITH_TAG_LEVEL | LOG_FORMAT_WITH_TID);
	//xlog_set_format(LOG_FORMAT_WITH_TIMESTAMP);
	//xlog_set_format(LOG_FORMAT_WITH_TAG_LEVEL);
	//xlog_set_format(LOG_FORMAT_RAW);
#endif // XLOG
}

static int get_testcase_from_kb(int* p_testcases, int test_case_size)
{
	char buffer[1024] = { 0 };
	int retry_counter = 0;
	do 
	{
		fprintf(stderr, "%s please input your choose: ", retry_counter > 1 ? "invalid input, retry." : "");
		if (!fgets(buffer, sizeof(buffer) - 1, stdin))
		{
			fprintf(stderr, "failed on get run info");
			return -1;
		}
		++retry_counter;
	} while (buffer[0] == '\n' || buffer[0] > ('0' + 9));
	fprintf(stderr, "your choose is: %s\n", buffer);
	size_t str_len = strlen(buffer);
	if (str_len < 1) return -2;
	char* p_str_end = buffer + strlen(buffer);
	char* p_loc = NULL;
	if (NULL != (p_loc = strchr(buffer, '\n')))
	{
		*p_loc = '\0'; // remove \n
	}
	char* str_start = buffer;
	int case_count = 0;
	while (case_count < test_case_size)
	{
		if (NULL != (p_loc = strchr(str_start, ' ')))
		{
			*p_loc = '\0'; // cut string
		}
		char* end_ptr = NULL;
		int num = (int)strtol(str_start, &end_ptr, 10);
		if (end_ptr == str_start)
		{
			break; // no parse performed
		}
		p_testcases[case_count] = num;
		fprintf(stderr, "you select test cases[%d]=%d\n", case_count, p_testcases[case_count]);
		++case_count;
		if (p_loc == NULL || (str_start = p_loc + 1) >= p_str_end)
		{
			break; // no string need continue parsing
		}
	}
	return case_count;
}

static void show_menu_test_case()
{
	char str_menu[4096] = { 0 };
	size_t str_len = 0;
	size_t buffer_left_len;
	for (size_t i = 0; (buffer_left_len = sizeof(str_menu) - 1 - str_len) > 0
		&& i < (sizeof(g_all_test_cases) / sizeof(g_all_test_cases[0])); ++i)
	{
		str_len += snprintf(str_menu + str_len, buffer_left_len, "  %02zu : %s\n", i, g_all_test_cases[i].str_description);
	}
	fprintf(stderr, "input test case numbers (split by space),\n press enter to submit task:\n%s\n", str_menu);
}

static int run_test_case_from_user()
{
	int ret = 0;
	int test_cases[64] = { -1 };
	int test_case_count = 0;
	show_menu_test_case();
	if ((test_case_count = get_testcase_from_kb(test_cases, sizeof(test_cases) / sizeof(test_cases[0]))) < 1)
	{
		fprintf(stderr, "failed on get test cases\n");
		ret = 1;
	}
	fprintf(stderr, "you selected test case count=%d\n    Please wait...\n", test_case_count);
	int all_available_case_count = sizeof(g_all_test_cases) / sizeof(g_all_test_cases[0]);
	int case_number;
	for (int i = 0; i < test_case_count && (case_number = test_cases[i]) >= 0 
		&& case_number < all_available_case_count; ++i)
	{
		test_case_t* p_case = &g_all_test_cases[case_number];
		fprintf(stderr, "\n\n==> Now Run testcase[%d]: %s\n", i, p_case->str_description);
		LOGI("==> Now Run testcase[%d]: %s\n", i, p_case->str_description);
		ret = p_case->p_func();
		LOGI("<== End Run testcase[%d]: %s, ret=%d\n", i, p_case->str_description, ret);
		fprintf(stderr, "\n<== End Run testcase[%d]: %s, ret=%d\n\n", i, p_case->str_description, ret);
		if (ret)
		{
			LOGE("failed(%d) run last test case, break", ret);
			break;
		}
	}
	return ret;
}
