#include "common_macro.h"
#include "mem/strings.h"
#include <stdio.h>

#define LOG_TAG "BASIC_TEST"
#include "log/logger.h"

int basic_test()
{
	LOGD_TRACE(" *** Welcome *** ");
	FILE* fp_test = fopen("basic_test.txt", "wb");
	ASSERT(fp_test);
	const char *str = "0123456789abcdefghijklmnopqrstuvwxyz"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ~!@#$%^&*()_+{}|:<>?/.,';[]\\\n";
	fwrite(str, 1, strlen(str), fp_test);
	fclose(fp_test);
	LOGD("test long long log : %s %s %s %s %s %s %s %s %s %s",
		str, str, str, str, str, str, str, str, str, str);
	LOGD("test finished...");
	return 0;
}