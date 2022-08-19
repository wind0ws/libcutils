#include "data/base64.h"
#include "mem/strings.h"
#include <malloc.h>

#define LOG_TAG "BASE64_TEST"
#include "log/logger.h"

int base64_test()
{
	//helllo, 你好！
	const char plain_utf8_str[] = { 0x68,0x65,0x6C,0x6C,0x6C,0x6F,0x2C,0x20,0xE4,0xBD,0xA0,0xE5,0xA5,0xBD,0xEF,0xBC,0x81,0x00 };
	size_t plain_str_len = strlen(plain_utf8_str);
	size_t encode_buf_len = lcu_base64_encode_buf_size(plain_str_len);
	char* encode_buf = (char*)malloc(encode_buf_len);

	int ret;
	if ((ret = lcu_base64_encode(encode_buf, plain_utf8_str, plain_str_len)) < (int)plain_str_len)
	{
		TLOGE(LOG_TAG, "failed on encode base64");
		free(encode_buf);
		return ret;
	}
	TLOGI(LOG_TAG, "succeed encode \"%s\" ==> \"%s\"", plain_utf8_str, encode_buf);

	size_t decode_buf_len = lcu_base64_decode_buf_size(encode_buf_len);
	char* decode_buf = (char*)malloc(decode_buf_len);

	if ((ret = lcu_base64_decode(decode_buf, encode_buf)) != (int)plain_str_len)
	{
		TLOGE(LOG_TAG, "failed on decode base64");
	}
	else
	{
		TLOGI(LOG_TAG, "succeed decode \"%s\" ==> \"%s\"", encode_buf, decode_buf);
	}

	free(encode_buf);
	free(decode_buf);
	return 0;
}
