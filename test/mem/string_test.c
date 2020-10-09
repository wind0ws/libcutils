#include "mem/stringbuilder.h"
#include "mem/strings.h"
#include "common_macro.h"
#include "log/xlog.h"
#include "thread/thread_wrapper.h"

#define LOG_TAG_STR_TEST "STR_TEST"

static int stringbuilder_test()
{
	stringbuilder_t* sb = stringbuilder_create(8);
	stringbuilder_appendstr(sb, "Hello");
	stringbuilder_appendchar(sb, ',');
	stringbuilder_appendnstr(sb, "World!!!", 6);//here stringbuilder internal buffer will grow, because our stringbuilder init buf size is 8.
	stringbuilder_appendf(sb, " Current thread id:%d. ", gettid());
	LOGD("stringbuilder len:%d, str is:%s", stringbuilder_len(sb), stringbuilder_print(sb));// will print "Hello,World!"
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
	const char* str = "Hello, can you replace this string. can you. can you...";
	char chars[32];
	strlcpy(chars, str, 32);
	char *replaced_str = strreplace(chars,"can you","i can");
	LOGD("replaced string => %s", replaced_str);
	free(replaced_str);
	return 0;
}

static int stringsplit_test()
{
	const char* str = "Hello, can you split this string by comma, can you, can you...";
	char chars[64];
	strlcpy(chars, str, 64);
#define STR_TEST_RECEIVE_SPLIT_PTRS (6)
	size_t receive_split_ptr_nums = STR_TEST_RECEIVE_SPLIT_PTRS;
	char *receive_splited_str_ptrs[STR_TEST_RECEIVE_SPLIT_PTRS];
	strsplit(receive_splited_str_ptrs, &receive_split_ptr_nums, chars, ",");
	for (size_t i = 0; i < receive_split_ptr_nums; i++)
	{
		LOGD("receive_splited_str_ptrs[%d]=%s", i, receive_splited_str_ptrs[i]);
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
	const char* const_str = "AA...AA.a.aa.aHelloWorld     :::";
	char str[64] = {0};
	strlcpy(str, const_str, 64);
	LOGD("origin_str: %s", str);
	strtrim(str, "Aa. :");
	LOGD("trimed_str: %s", str);
	return 0;
}

int string_test()
{
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

	return 0;
}