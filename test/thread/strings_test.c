#include "mem/strings.h"
#include "log/xlog.h"
#include <malloc.h>

static int test_strsplit()
{
	const char str_from_some_where[] = "Hello, World, My name is Threshold, can you split this string by comma?";
	int recv_splited_nums = 4;
	char** recv_splited_str = (char**)calloc(recv_splited_nums, sizeof(char*));
	
	//char to_splited_str[256];
	char* to_splited_str = (char *)malloc(256 * sizeof(char));
	strlcpy(to_splited_str, str_from_some_where, /* size of dest */256);
	strsplit(recv_splited_str, &recv_splited_nums, to_splited_str, ",");

	for (int i = 0; i < recv_splited_nums && recv_splited_str[i]; ++i)
	{
		LOGD("strsplit[%d]=%s\n", i, recv_splited_str[i]);
	}

	free(recv_splited_str);
	free(to_splited_str);
	return 0;
}

static int test_strreplace()
{
	const char* str_origin = "hello world, can you replace this.";
	LOGD("to_replace_str is: %s", str_origin);
	char *str_replaced = strreplace(str_origin, "can you", "i can");
	LOGD("replaced_str is: %s", str_replaced);
	free(str_replaced);
	return 0;
}

int strings_test()
{
	test_strsplit();
	test_strreplace();
	return 0;
}