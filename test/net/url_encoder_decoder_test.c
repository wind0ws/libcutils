#include "net/url_encoder_decoder.h"
#include "log/xlog.h"
#include <string.h>
#include <malloc.h>
#include "common_macro.h"

#define LOG_TAG "URL_EN_DE"

static int url_encoder_test()
{
	const char* url = "http://127.0.0.1:8008/test?user=张三&passsword=1234";
	size_t url_strlen = strlen(url);
	size_t out_encode_buf_size = url_strlen * 3 + 1;
	char* out_encode_str = (char *)malloc(out_encode_buf_size);

	int real_encode_strlen = lcu_url_encode(out_encode_str, out_encode_buf_size, url, url_strlen);
	do 
	{
		if (real_encode_strlen < 1)
		{
			TLOGE(LOG_TAG, "failed on encode url. %d", real_encode_strlen);
			break;
		}
		TLOGI(LOG_TAG, "url(%s) encode succeed: %s ", url, out_encode_str);
	} while (0);

	free(out_encode_str);
	
	return (real_encode_strlen > 0) ? 0 : real_encode_strlen;
}

static int url_decoder_test()
{
	const char* encode_url = "http%3A%2F%2F127.0.0.1%3A8008%2Ftest%3Fuser%3D%E5%BC%A0%E4%B8%89%26passsword%3D1234";
	size_t encode_url_strlen = strlen(encode_url);

	size_t decode_url_buf_size = encode_url_strlen + 1;
	char* decode_url = (char *)malloc(decode_url_buf_size);
	char* last_src_pos = NULL;
	
	int decode_ret;
	do 
	{
		if ((decode_ret = lcu_url_decode(decode_url, decode_url_buf_size, encode_url, encode_url_strlen, &last_src_pos)) < 0)
		{
			TLOGE(LOG_TAG, "failed on decode: %s", encode_url);
			break;
		}
		ASSERT(last_src_pos == (encode_url + encode_url_strlen));
		TLOGI(LOG_TAG, "succeed decode \"%s\" to \"%s\"",encode_url, decode_url);
	} while (0);

	free(decode_url);
	return decode_ret > 0 ? 0 : decode_ret;
}

int url_encoder_decoder_test()
{
	int ret;
	do 
	{
		ret = url_encoder_test();
		if (ret)
		{
			break;
		}
		ret = url_decoder_test();
	} while (0);
	
	return ret;
}