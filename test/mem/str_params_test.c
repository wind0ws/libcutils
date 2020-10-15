#include "mem/mem_debug.h"
#include "mem/str_params.h"
#include "log/xlog.h"
#include "common_macro.h"

#define LOG_TAG  "STR_PARAMS_TEST"

//static const char* test_str = "foo=bar;abc=123;bad_key;def=123.456;";
static const char* test_str = "foo=bar,abc=123,bad_key,def=123.456,";

int str_params_test()
{
	str_params_ptr params = str_params_create_str(",", test_str);
	if (!params)
	{
		TLOGE(LOG_TAG, "failed on parse: %s", params);
		return 1;
	}
	char str_value[32];
	int ret = str_params_get_str(params, "foo", str_value, sizeof(str_value));
	if (ret == 0)
	{
		TLOGI(LOG_TAG, "succeed get foo=%s", str_value);
	}
	else
	{
		TLOGE(LOG_TAG, "failed on get foo. %d", ret);
	}
	int abc = 0;
	ret = str_params_get_int(params, "abc", &abc);
	if (ret == 0)
	{
		TLOGI(LOG_TAG, "succeed get abc=%d", abc);
	}
	else
	{
		TLOGE(LOG_TAG, "failed on get abc. %d", ret);
	}
	float def = 0.0f;
	ret = str_params_get_float(params, "def", &def);
	if (ret == 0)
	{
		TLOGI(LOG_TAG, "succeed get def=%.03f", def);
	}
	else
	{
		TLOGE(LOG_TAG, "failed on get def. %d", ret);
	}
	str_params_del(params, "foo");
	str_params_add_str(params, "foo", "bar1");
	str_params_add_int(params, "abc", 456);
	str_params_add_float(params, "my_float", 1.234f);
	str_params_dump(params);

	char* param_str = str_params_to_str(params);
	if (param_str)
	{
		TLOGI(LOG_TAG, "param_str=> %s", param_str);
		free(param_str);
	}

	str_params_destroy(params);
	return 0;
}
