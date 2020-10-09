#include "file/ini_reader.h"
#include "file/ini_parser.h"
#include "log/xlog.h"
#include "mem/strings.h"
#include "common_macro.h"

#define LOG_TAG_INI_TEST "INI_TEST"

static const char* test_ini_str = "[config]\r\n\
#this is comment\r\n\
nNum1 = 6\r\n\
test=\r\n\
#中文注释\r\n\
nNum2 = 2\r\n\
#test double number\r\n\
nNum3 = 0.035\r\n\
\r\n\
[config2]\r\n\
#test true false\r\n\
auto_start = true\r\n\
\r\n\
[config3]\r\n\
path=/sdcard/Android/data/\
\r\n";

static int my_ini_handler(void* user, int lineno, 
	const char* section, const char* key, const char* value) 
{
	if (!section || !key)
	{
		return 0;
	}
	TLOGD(LOG_TAG_INI_TEST, "[%s] %s=%s", section, key, value);
	#define MATCH(s, k) strcmp(section, s) == 0 && strcmp(key, k) == 0
	if (MATCH("config", "nNum1"))
	{
		TLOGD(LOG_TAG_INI_TEST, "detect nNum1=%d", atoi(value));
	}
	return 0;
}

static int ini_reader_test()
{
	int ret = ini_parse_string(test_ini_str, my_ini_handler, NULL);
	if (ret)
	{
		TLOGD(LOG_TAG_INI_TEST, "parse ini string occurrd error. %d", ret);
		return ret;
	}
	TLOGI(LOG_TAG_INI_TEST, "succeed parse ini string.");
	return ret;
}

static int ini_parser_test()
{
	ini_parser_ptr parser = ini_parser_parse_str(test_ini_str);
	if (!parser)
	{
		TLOGE(LOG_TAG_INI_TEST, "failed parse ini string");
		return 1;
	}
	double nNum = 0.0;
	INI_PARSER_ERROR_CODE err = ini_parser_get_double(parser, "config", "nNum3", &nNum);
	if (err == INI_PARSER_ERR_SUCCEED)
	{
		TLOGI(LOG_TAG_INI_TEST, "succeed get nNum3=%.3f", nNum);
	}
	else
	{
		TLOGE(LOG_TAG_INI_TEST, "failed get nNum3.  %d", err);
	}
	
	char path[128];
	err = ini_parser_get_string(parser, "config3", "path", path, 128);
	if (err == INI_PARSER_ERR_SUCCEED)
	{
		TLOGI(LOG_TAG_INI_TEST, "succeed get path=%s", path);
	}
	else
	{
		TLOGE(LOG_TAG_INI_TEST, "failed get path.  %d", err);
	}

	ini_parser_destory(&parser);
	return 0;
}

int ini_test()
{
	int ret = 0;
	TLOGD(LOG_TAG_INI_TEST, "  --> Now run ini_reader_test");
	ret = ini_reader_test();
	TLOGD(LOG_TAG_INI_TEST, "  <-- ini_reader_test result: %d", ret);
	if (ret)
	{
		return ret;
	}

	TLOGD(LOG_TAG_INI_TEST, "  --> Now run ini_parser_test");
	ret = ini_parser_test();
	TLOGD(LOG_TAG_INI_TEST, "  <-- ini_parser_test result: %d", ret);
	return ret;
}