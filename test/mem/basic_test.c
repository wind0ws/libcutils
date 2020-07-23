#include "common_macro.h"
#include "mem/strings.h"

int basic_test()
{
	FILE* fTest = fopen("basic_test.txt","wb");
	ASSERT(fTest);
	const char *str = "1234567890abcdefghijklmn";
	fwrite(str, 1, strlen(str), fTest);
	fclose(fTest);
	return 0;
}