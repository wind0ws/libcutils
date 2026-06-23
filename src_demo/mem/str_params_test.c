#include "mem/mem_debug.h"
#include "mem/str_params.h"
#include "common_macro.h"

#include <errno.h>

#define LOG_TAG  "STR_PARAMS_TEST"
#include "log/logger.h"

//static const char* test_str = "foo=bar;abc=123;bad_key;def=123.456;";
static const char* test_str = "foo=bar,abc=123,bad_key,def=123.456,";

int str_params_test(void)
{
	str_params_ptr params = str_params_create_str(",", test_str);
	if (!params)
	{
		LOGE("failed on parse: %s", test_str);
		return 1;
	}
	char str_value[32];
	int ret = str_params_get_str(params, "foo", str_value, sizeof(str_value));
	if (ret == 0)
	{
		LOGI("succeed get foo=%s", str_value);
	}
	else
	{
		LOGE("failed on get foo. %d", ret);
	}
	int abc = 0;
	ret = str_params_get_int(params, "abc", &abc);
	if (ret == 0)
	{
		LOGI("succeed get abc=%d", abc);
	}
	else
	{
		LOGE("failed on get abc. %d", ret);
	}
	float def = 0.0f;
	if (0 == (ret = str_params_get_float(params, "def", &def)))
	{
		LOGI("succeed get def=%.03f", def);
	}
	else
	{
		LOGE("failed on get def. %d", ret);
	}
	str_params_del(params, "foo");
	str_params_add_str(params, "foo", "bar1");
	str_params_add_int(params, "abc", 456);
	str_params_add_float(params, "my_float", 1.234f);
	str_params_dump(params);

	char* param_str = str_params_to_str(params);
	if (param_str)
	{
		LOGI("param_str=> %s", param_str);
		free(param_str);
	}

	str_params_destroy(params);

	/* Caller-1 回归测试：重复 key 不泄漏 */
	LOGI("[Caller-1] Testing duplicate key handling...");
	str_params_ptr dup_test = str_params_create_str(";", "a=1;a=2;a=3");
	ASSERT(dup_test != NULL);
	char val[32];
	ASSERT(0 == str_params_get_str(dup_test, "a", val, sizeof(val)));
	ASSERT(0 == strcmp(val, "3"));  /* 最后一个值生效 */
	str_params_destroy(dup_test);
	LOGI("[Caller-1] PASS: duplicate keys handled without leak");

	/* Regression: a stale ENOMEM from caller state must not make successful
	 * hashmap insertion look like OOM. */
	LOGI("[Caller-1] Testing stale errno before parse...");
	int saved_errno = errno;
	errno = ENOMEM;
	str_params_ptr errno_test = str_params_create_str(";", "k=v;z=9");
	ASSERT(errno_test != NULL);
	memset(val, 0, sizeof(val));
	ASSERT(0 == str_params_get_str(errno_test, "k", val, sizeof(val)));
	ASSERT(0 == strcmp(val, "v"));
	memset(val, 0, sizeof(val));
	ASSERT(0 == str_params_get_str(errno_test, "z", val, sizeof(val)));
	ASSERT(0 == strcmp(val, "9"));
	str_params_destroy(errno_test);
	errno = saved_errno;
	LOGI("[Caller-1] PASS: stale errno ignored during parse");

	LOGI("[Caller-1] Testing stale errno before add_str...");
	str_params_ptr add_errno_test = str_params_create(";");
	ASSERT(add_errno_test != NULL);
	saved_errno = errno;
	errno = ENOMEM;
	ASSERT(0 == str_params_add_str(add_errno_test, "add_k", "add_v"));
	memset(val, 0, sizeof(val));
	ASSERT(0 == str_params_get_str(add_errno_test, "add_k", val, sizeof(val)));
	ASSERT(0 == strcmp(val, "add_v"));
	str_params_destroy(add_errno_test);
	errno = saved_errno;
	LOGI("[Caller-1] PASS: stale errno ignored during add_str");

	return 0;
}

#include "lcu_test_registry.h"
LCU_TEST_REGISTER(str_params_test, "test string params");
