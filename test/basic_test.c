#include "common_macro.h"
#include "strings.h"

int basic_test()
{
	FILE* fTest = NULL;
	fTest = fopen("basic_test.txt","wb");
	ASSERT_RET_VALUE(fTest, -1);
	char *str = "abcdefghijklmn";
	fwrite(str, 1, strlen(str), fTest);
	fclose(fTest);
	return 0;
}