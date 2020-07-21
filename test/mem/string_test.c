#include "mem/stringbuilder.h"
#include "mem/strings.h"
#include "common_macro.h"
#include "log/xlog.h"
#include "thread/thread_wrapper.h"


#define LOG_TAG_STR_TEST "str_test"

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

	LOGD("strcasecmp=%d, stricmp=%d", strcasecmp(str1, str2), stricmp(str1, str2));
	return 0;
}

int string_test()
{
	LOGD("--> test stringbuilder");
	stringbuilder_test();
	LOGD("<-- stringbuilder test finished\n");

	LOGD("--> test stringcmp");
	stringcmp_test();
	LOGD("<-- stringcmp test finished\n");
	return 0;
}