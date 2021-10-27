#include "common_macro.h"
#include "mem/strings.h"
#include "log/xlog.h"
#include <stdio.h>

#define LOG_TAG "BASIC_TEST"

int basic_test()
{
	TLOGD_TRACE(LOG_TAG, " *** Welcome *** ");
	FILE* fp_test = fopen("basic_test.txt", "wb");
	ASSERT(fp_test);
	const char *str = "0123456789abcdefghijklmnopqrstuvwxyz"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ~!@#$%^&*()_+{}|:<>?/.,';[]\\\n";
	fwrite(str, 1, strlen(str), fp_test);
	fclose(fp_test);
	TLOGD(LOG_TAG, "test long long log : %s %s %s %s %s %s %s %s %s %s",
		str, str, str, str, str, str, str, str, str, str);
	TLOGD(LOG_TAG, "test finished...");
	return 0;
}