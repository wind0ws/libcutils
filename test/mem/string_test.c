#include "mem/stringbuilder.h"
#include "common_macro.h"
#include "log/xlog.h"
#include "thread/thread_wrapper.h"

#define LOG_TAG_STR_TEST "str_test"

int stringbuilder_test()
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