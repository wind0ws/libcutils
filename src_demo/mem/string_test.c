#include "mem/mem_debug.h"
#include "mem/stringbuilder.h"
#include "mem/strings.h"
#include "common_macro.h"
#include "thread/posix_thread.h"

#define LOG_TAG "STR_TEST"
#include "log/logger.h"

static int stringbuilder_test()
{
	stringbuilder_t* sb = stringbuilder_create(8);
	stringbuilder_appendstr(sb, "Hello");
	stringbuilder_appendchar(sb, ',');
	stringbuilder_appendnstr(sb, "World!!!", 6);//here stringbuilder internal buffer will grow, because our stringbuilder init buf size is 8.
	stringbuilder_appendf(sb, " Current thread id:%d. ", GETTID());
	LOGD("stringbuilder len:%zu, str:%s", stringbuilder_len(sb), stringbuilder_print(sb));// will print "Hello,World!"
	stringbuilder_destroy(&sb);
	return 0;
}

static int stringcmp_test()
{
	const char* str1 = "Hello, what's your name?";
	const char* str2 = "Hello, what's your name?";
	LOGD("strcasecmp=%d", strcasecmp(str1, str2));

	const char* str3 = "Hello,123";
	const char* str4 = "Hello,456";
	LOGD("strncasecmp=%d, strnicmp=%d", strncasecmp(str3, str4, 6), strncasecmp(str3, str4, 9));
	return 0;
}

static int stringreplace_test()
{
	const char* str = "Hello, can you replace this string. can you1. can you2...";
	char chars[32];
	strlcpy(chars, str, sizeof(chars));
	char *replaced_str = strreplace(chars, "can you", "i can");
	if (!replaced_str)
	{
		LOGE("failed on strreplace");
		return -1;
	}
	LOGD("replaced string => %s", replaced_str);
	free(replaced_str);
	return 0;
}

static int stringsplit_test()
{
	const char* str = "Hello, can you split this sentence by comma, can you1, can you2...";
	char chars[128];
	strlcpy(chars, str, sizeof(chars));
#define STR_TEST_RECEIVE_SPLIT_PTRS (6)
	size_t receive_split_ptr_nums = STR_TEST_RECEIVE_SPLIT_PTRS;
	char* receive_splited_str_ptrs[STR_TEST_RECEIVE_SPLIT_PTRS] = { NULL };
	strsplit(receive_splited_str_ptrs, &receive_split_ptr_nums, chars, ",");
	for (size_t i = 0; i < receive_split_ptr_nums; ++i)
	{
		if (receive_splited_str_ptrs[i])
		{
			LOGD("receive_splited_str_ptrs[%d]=%s", i, receive_splited_str_ptrs[i]);
		}
	}
	return 0;
}

static int count_utf8str_words_test()
{
	const char nihao[] = { 0xE4,0xBD,0xA0,0xE5,0xA5,0xBD,0xE5,0x91,0x80,0xEF,0xBC,0x81,0x00 };//你好呀！
	size_t bytes_len = strlen(nihao);
	size_t words_count = strnutf8len(nihao, bytes_len);
	LOGD("%s word count:%zu, bytes:%zu", nihao, words_count, bytes_len);
	return 0;
}

static int strtrim_test()
{
	const char* const_str = "AA...AA.a.aa.aHello : World     :::";
	char str[64] = {0};
	strlcpy(str, const_str, sizeof(str)); //strtrim need modify input string.
	LOGD("origin_str: %s", str);
	strtrim(str, "Aa. :");
	LOGD("trimed_str: %s", str);// should be "Hello : World"
	return 0;
}

static int strdup_test()
{
	const char* test_str_fordup = "nihao,hello";
	char *dup0 = strndup(test_str_fordup, 0);
	ASSERT(strcmp("", dup0) == 0);
	free(dup0);
	char *dup5 = strndup(test_str_fordup, 5);
	ASSERT(strncmp(test_str_fordup, dup5, 5) == 0);
	free(dup5);
	return 0;
}

static int strcase_test()
{
	char str[128] = {0};
	strlcpy(str, "hello", sizeof(str));
	LOGD("str=> %s", str);
	STRING2UPPER(str);
	LOGD("after upper => %s", str);
	STRING2LOWER(str);
	LOGD("after lower => %s", str);
	return 0;
}

static int strhex_test()
{
	char chars[] = { 0x01, 0x0A, 0x10, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF };
	LOGI_HEX(chars, sizeof(chars));
	return 0;
}

int string_test()
{
	LOGD("--> test strhex_test");
	strhex_test();
	LOGD("<-- strhex test finished\n");

	LOGD("--> test strdup");
	strdup_test();
	LOGD("<-- strdup test finished\n");

	LOGD("--> test stringbuilder");
	stringbuilder_test();
	LOGD("<-- stringbuilder test finished\n");

	LOGD("--> test strtrim");
	strtrim_test();
	LOGD("<-- strtrim test finished\n");

	LOGD("--> test stringcmp");
	stringcmp_test();
	LOGD("<-- stringcmp test finished\n");

	LOGD("--> test stringreplace_test");
	stringreplace_test();
	LOGD("<-- stringreplace_test finished\n");

	LOGD("--> test stringsplit_test");
	stringsplit_test();
	LOGD("<-- stringsplit_test finished\n");

	LOGD("--> test count_utf8str_words_test");
	count_utf8str_words_test();
	LOGD("<-- count_utf8str_words_test finished\n");

	LOGD("--> test strcase_test");
	strcase_test();
	LOGD("<-- strcase_test finished\n");

	return 0;
}
