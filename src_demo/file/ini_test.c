#include "mem/mem_debug.h"
#include "file/ini_reader.h"
#include "file/ini_parser.h"
#include "mem/strings.h"
#include "common_macro.h"

#define LOG_TAG "INI_TEST"
#include "log/logger.h"

static const char* test_ini_str = "\
[config]\r\n\
#this is comment\r\n\
#number=1\r\n\
# 测试key前面多个空格\r\n\
 nNum1 = 6\r\n\
test=\r\n\
#中文注释以及没有return符\n\
  nNum2 = 2\n\
#test double number\r\n\
nNum3=0.035\r\n\
# 测试16进制数字解析\r\n\
nNum4 = 0xFF\n\
nNum5 = 0Xff\n\
\r\n\
[config2]\r\n\
#test true false\r\n\
auto_start = FALSE\r\n\
enable_state=true \r\n\
number_bool_state = 0 \r\n\
\r\n\
;test semicolon comment\r\n\
[config3]\r\n\
path= /sdcard/Android/data/  \r\n\
\r\n\
[config4]\r\n\
#test empty value \r\n\
run_mode =  \r\n\
\r\n";

static ini_parser_handle create_parser_from_string(void)
{
	ini_parser_handle parser = ini_parser_parse_str(test_ini_str);
	ASSERT(parser);
	return parser;
}

static void test_ini_parser_is_file_path(void)
{
	ASSERT(true == ini_parser_is_file_path("C:/tmp/config.ini"));
	ASSERT(true == ini_parser_is_file_path("relative/path/config.ini"));
	ASSERT(false == ini_parser_is_file_path("[config]\nkey=value"));
	ASSERT(false == ini_parser_is_file_path(NULL));
}

static void test_ini_parser_create_destroy(void)
{
	ini_parser_handle parser = ini_parser_create();
	ASSERT(parser);
	ASSERT(INI_PARSER_CODE_NOT_FOUND_SECTION_KEY == ini_parser_has_section(parser, "missing"));
	ASSERT(INI_PARSER_CODE_INVALID_PARAM == ini_parser_put_string(parser, NULL, "key", "value"));
	ASSERT(INI_PARSER_CODE_INVALID_PARAM == ini_parser_put_string(parser, "section", NULL, "value"));
	ASSERT(INI_PARSER_CODE_INVALID_PARAM == ini_parser_get_string(parser, NULL, "key", NULL, 0));
	ASSERT(INI_PARSER_CODE_SUCCEED == ini_parser_put_string(parser, "section", "key", "value"));
	ASSERT(INI_PARSER_CODE_SUCCEED == ini_parser_has_section_key(parser, "section", "key"));
	ASSERT(INI_PARSER_CODE_SUCCEED == ini_parser_destroy(&parser));
	ASSERT(NULL == parser);
	ASSERT(INI_PARSER_CODE_INVALID_PARAM == ini_parser_destroy(&parser));
	ASSERT(NULL == ini_parser_parse_file(NULL));
	const char* non_exist_path = "__ini_parser_non_exist__.ini";
	remove(non_exist_path);
	ASSERT(NULL == ini_parser_parse_file(non_exist_path));
}

typedef struct
{
	int count;
	int stop_after;
} foreach_counter_t;

static int counting_handler(const char* section, const char* key, const char* value, void* user)
{
	UNUSED(section);
	UNUSED(key);
	UNUSED(value);
	foreach_counter_t* ctx = (foreach_counter_t*)user;
	if (ctx)
	{
		ctx->count++;
		if (ctx->stop_after > 0 && ctx->count >= ctx->stop_after)
		{
			return 1;
		}
	}
	return 0;
}

static void test_ini_parser_foreach_behavior(void)
{
	ini_parser_handle parser = create_parser_from_string();
	foreach_counter_t ctx = {0, 0};
	ASSERT(INI_PARSER_CODE_SUCCEED == ini_parser_foreach(parser, counting_handler, &ctx));
	ASSERT(ctx.count > 0);
	foreach_counter_t early_stop = {0, 1};
	ASSERT(INI_PARSER_CODE_SUCCEED == ini_parser_foreach(parser, counting_handler, &early_stop));
	ASSERT(early_stop.count == 1);
	ini_parser_destroy(&parser);
}

static void test_ini_parser_basic_rw(void)
{
	ini_parser_handle parser = create_parser_from_string();
	ASSERT(INI_PARSER_CODE_SUCCEED == ini_parser_put_string(parser, "config", "added_key", "value1"));
	ASSERT(INI_PARSER_CODE_SUCCEED == ini_parser_put_string(parser, "config", "added_key", "value2"));
	ASSERT(INI_PARSER_CODE_SUCCEED == ini_parser_put_string(parser, "new_section", "new_key", "new_value"));
	char value_buf[32] = {0};
	ASSERT(INI_PARSER_CODE_SUCCEED == ini_parser_get_string(parser, "config", "added_key", value_buf, sizeof(value_buf)));
	ASSERT(strcmp(value_buf, "value2") == 0);
	char small_buf[4] = {0};
	ASSERT(INI_PARSER_CODE_NO_ENOUGH_MEMORY == ini_parser_get_string(parser, "config", "added_key", small_buf, sizeof(small_buf)));
	ASSERT(INI_PARSER_CODE_NOT_FOUND_SECTION_KEY == ini_parser_get_string(parser, "config", "missing", value_buf, sizeof(value_buf)));
	ASSERT(INI_PARSER_CODE_SUCCEED == ini_parser_has_section(parser, "config"));
	ASSERT(INI_PARSER_CODE_SUCCEED == ini_parser_has_section_key(parser, "new_section", "new_key"));
	ini_parser_destroy(&parser);
}

static void test_ini_parser_numeric(void)
{
	ini_parser_handle parser = create_parser_from_string();
	double dbl_val = 0.0;
	float flt_val = 0.0f;
	int int_val = 0;
	long nnum2_long = 0;
	long long hex_val = 0;
	long long hex_val2 = 0;
	bool bool_val = false;
	ASSERT(INI_PARSER_CODE_SUCCEED == ini_parser_get_double(parser, "config", "nNum3", &dbl_val));
	ASSERT(dbl_val > 0.03 && dbl_val < 0.04);
	ASSERT(INI_PARSER_CODE_SUCCEED == ini_parser_get_float(parser, "config", "nNum3", &flt_val));
	ASSERT(flt_val > 0.03f && flt_val < 0.04f);
	ASSERT(INI_PARSER_CODE_SUCCEED == ini_parser_get_int(parser, "config", "nNum1", &int_val));
	ASSERT(int_val == 6);
	ASSERT(INI_PARSER_CODE_SUCCEED == ini_parser_get_long(parser, "config", "nNum2", &nnum2_long));
	ASSERT(nnum2_long == 2);
	ASSERT(INI_PARSER_CODE_SUCCEED == ini_parser_get_long_long(parser, "config", "nNum4", &hex_val));
	ASSERT(INI_PARSER_CODE_SUCCEED == ini_parser_get_long_long(parser, "config", "nNum5", &hex_val2));
	ASSERT(hex_val == hex_val2);
	ASSERT(INI_PARSER_CODE_SUCCEED == ini_parser_get_bool(parser, "config2", "auto_start", &bool_val));
	ASSERT(false == bool_val);
	ASSERT(INI_PARSER_CODE_SUCCEED == ini_parser_get_bool(parser, "config2", "enable_state", &bool_val));
	ASSERT(true == bool_val);
	ASSERT(INI_PARSER_CODE_SUCCEED == ini_parser_get_bool(parser, "config2", "number_bool_state", &bool_val));
	ASSERT(false == bool_val);
	ASSERT(INI_PARSER_CODE_SUCCEED == ini_parser_put_string(parser, "config2", "invalid_bool", "maybe"));
	ASSERT(INI_PARSER_CODE_FAILED == ini_parser_get_bool(parser, "config2", "invalid_bool", &bool_val));
	ini_parser_destroy(&parser);
}

static void test_ini_parser_dump_api(void)
{
	ini_parser_handle parser = create_parser_from_string();
	char small_mem[8] = {0};
	size_t small_size = sizeof(small_mem);
	ASSERT(INI_PARSER_CODE_NO_ENOUGH_MEMORY == ini_parser_dump_to_mem(parser, small_mem, &small_size));
	char mem_area[1024] = {0};
	size_t mem_size = sizeof(mem_area);
	ASSERT(INI_PARSER_CODE_SUCCEED == ini_parser_dump_to_mem(parser, mem_area, &mem_size));
	ASSERT(strlen(mem_area) + 1U == mem_size);
	char* dumped = ini_parser_dump(parser);
	ASSERT(dumped);
	ASSERT(NULL != strstr(dumped, "[config]"));
	free(dumped);
	ini_parser_destroy(&parser);
}

static void test_ini_parser_delete_ops(void)
{
	ini_parser_handle parser = create_parser_from_string();
	ASSERT(INI_PARSER_CODE_NOT_FOUND_SECTION_KEY == ini_parser_delete_by_section_key(parser, "config", "not_exist"));
	ASSERT(INI_PARSER_CODE_SUCCEED == ini_parser_delete_by_section_key(parser, "config", "test"));
	ASSERT(INI_PARSER_CODE_NOT_FOUND_SECTION_KEY == ini_parser_has_section_key(parser, "config", "test"));
	ASSERT(INI_PARSER_CODE_SUCCEED == ini_parser_delete_section(parser, "config4"));
	ASSERT(INI_PARSER_CODE_NOT_FOUND_SECTION_KEY == ini_parser_has_section(parser, "config4"));
	ASSERT(INI_PARSER_CODE_NOT_FOUND_SECTION_KEY == ini_parser_delete_section(parser, "config4"));
	ini_parser_destroy(&parser);
}

static void test_ini_parser_save_and_parse_file(void)
{
	const char* tmp_file = "ini_parser_test_output.ini";
	ini_parser_handle parser = create_parser_from_string();
	remove(tmp_file);
	ASSERT(INI_PARSER_CODE_SUCCEED == ini_parser_save(parser, tmp_file));
	ini_parser_handle file_parser = ini_parser_parse_file(tmp_file);
	ASSERT(file_parser);
	char buffer[64] = {0};
	ASSERT(INI_PARSER_CODE_SUCCEED == ini_parser_get_string(file_parser, "config", "nNum1", buffer, sizeof(buffer)));
	ASSERT(strcmp(buffer, "6") == 0);
	ini_parser_destroy(&file_parser);
	remove(tmp_file);
	ini_parser_destroy(&parser);
}

static void test_ini_parser_error_paths(void)
{
	ini_parser_handle parser = create_parser_from_string();
	ASSERT(INI_PARSER_CODE_INVALID_PARAM == ini_parser_get_string(parser, NULL, NULL, NULL, 0));
	ASSERT(INI_PARSER_CODE_INVALID_PARAM == ini_parser_put_string(NULL, "config", "key", "value"));
	ASSERT(INI_PARSER_CODE_INVALID_PARAM == ini_parser_delete_by_section_key(NULL, "config", "key"));
	ini_parser_destroy(&parser);
}

/**
 * ini parse callback
 *   return true continue,
 *   return false will cause 'ini_reader_parse' returned non-zero code.
 */
static int my_ini_reader_handler(void* user,
	const char* section, const char* key, const char* value
#if INI_HANDLER_LINENO
	, int lineno
#endif // INI_HANDLER_LINENO
)
{
	if (!section || !key)
	{
		return true; // just ignore and continue
	}
	LOGD("[%s] %s=%s", section, key, value);
#define INI_MATCH(s, k) (strcmp(section, (s)) == 0 && strcmp(key, (k)) == 0)
	if (INI_MATCH("config", "nNum1"))
	{
		LOGD("  ~~ hi ~~ detect nNum1=%d", atoi(value));
	}
	return true;
}

static int ini_reader_test()
{
	int ret = ini_reader_parse_string(test_ini_str, my_ini_reader_handler, NULL);
	if (ret)
	{
		LOGE("parse ini string occurrd error. %d", ret);
		return ret;
	}
	ASSERT(0 == ret);
	LOGI("succeed parse ini string.");
	return ret;
}

static int ini_parser_test()
{
	LOGD("  -> run ini_parser_is_file_path tests");
	test_ini_parser_is_file_path();
	LOGD("  -> run ini_parser_create_destroy tests");
	test_ini_parser_create_destroy();
	LOGD("  -> run ini_parser_foreach tests");
	test_ini_parser_foreach_behavior();
	LOGD("  -> run ini_parser_basic_rw tests");
	test_ini_parser_basic_rw();
	LOGD("  -> run ini_parser_numeric tests");
	test_ini_parser_numeric();
	LOGD("  -> run ini_parser_dump_api tests");
	test_ini_parser_dump_api();
	LOGD("  -> run ini_parser_delete_ops tests");
	test_ini_parser_delete_ops();
	LOGD("  -> run ini_parser_save_and_parse_file tests");
	test_ini_parser_save_and_parse_file();
	LOGD("  -> run ini_parser_error_paths tests");
	test_ini_parser_error_paths();
	return 0;
}

int ini_test()
{
	int ret = 0;
	LOGD("  --> now run ini_reader_test");
	ret = ini_reader_test();
	LOGD("  <-- ini_reader_test result: %d", ret);
	if (ret)
	{
		return ret;
	}
	LOGD(LOG_STAR_LINE "%d", ret);

	LOGD("  --> now run ini_parser_test");
	ret = ini_parser_test();
	LOGD("  <-- ini_parser_test result: %d", ret);
	return ret;
}
