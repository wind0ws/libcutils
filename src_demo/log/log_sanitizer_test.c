#include "log/log_sanitizer.h"
#include "lcu_test_registry.h"
#include "common_macro.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define SANITIZER_MAX_BUF (256)

#define CHECK_EQ_STR(expected, actual, msg) \
    do { \
        if (strcmp((expected), (actual)) != 0) { \
            fprintf(stderr, "FAIL [%s]: expected='%s' actual='%s'\n", \
                    (msg), (expected), (actual)); \
            return 1; \
        } \
    } while (0)

static const char* g_test_name = "";

static void test_case_begin(const char* name)
{
	g_test_name = name;
}

/* ---- 辅助：执行脱敏并返回结果字符串指针（buf 必须 >= SANITIZER_MAX_BUF） ---- */
static char* do_sanitize(const char* input, size_t body_offset, size_t* out_len)
{
	static char buf[SANITIZER_MAX_BUF];
	snprintf(buf, sizeof(buf), "%s", input);
	size_t orig_len = strlen(buf);
	size_t new_len = log_sanitize_in_place(buf, body_offset);
	*out_len = new_len;
	return buf;
}

/* ========== 内置模式匹配 ========== */
static int test_builtin_token(void)
{
	test_case_begin("builtin_token");
	size_t len;
	char* out = do_sanitize("token=abc123456", 0, &len);
	CHECK_EQ_STR("token=***", out, g_test_name);
	return 0;
}

static int test_builtin_apikey_colon(void)
{
	test_case_begin("builtin_apikey_colon");
	size_t len;
	char* out = do_sanitize("apikey: secretvalue", 0, &len);
	CHECK_EQ_STR("apikey: ***", out, g_test_name);
	return 0;
}

static int test_builtin_access_token_suffix(void)
{
	test_case_begin("builtin_access_token_suffix");
	size_t len;
	char* out = do_sanitize("access_token=xyz789", 0, &len);
	CHECK_EQ_STR("access_token=***", out, g_test_name);
	return 0;
}

static int test_builtin_password(void)
{
	test_case_begin("builtin_password");
	size_t len;
	char* out = do_sanitize("password=hunter2", 0, &len);
	CHECK_EQ_STR("password=***", out, g_test_name);
	return 0;
}

static int test_builtin_secret(void)
{
	test_case_begin("builtin_secret");
	size_t len;
	char* out = do_sanitize("my_secret=topvalue", 0, &len);
	CHECK_EQ_STR("my_secret=***", out, g_test_name);
	return 0;
}

/* ========== 大小写变体 ========== */
static int test_uppercase_Token(void)
{
	test_case_begin("uppercase_Token");
	size_t len;
	char* out = do_sanitize("Token=ABC123", 0, &len);
	CHECK_EQ_STR("Token=***", out, g_test_name);
	return 0;
}

static int test_uppercase_APIKey(void)
{
	test_case_begin("uppercase_APIKey");
	size_t len;
	char* out = do_sanitize("APIKey=XYZ789", 0, &len);
	CHECK_EQ_STR("APIKey=***", out, g_test_name);
	return 0;
}

static int test_uppercase_TOKEN(void)
{
	test_case_begin("uppercase_TOKEN");
	size_t len;
	char* out = do_sanitize("TOKEN=value1", 0, &len);
	CHECK_EQ_STR("TOKEN=***", out, g_test_name);
	return 0;
}

static int test_authorization_bearer(void)
{
	test_case_begin("authorization_bearer");
	size_t len;
	/* 设计：裸值在空白处终止，"Bearer"被脱敏，剩余文本保留 */
	char* out = do_sanitize("Authorization: Bearer abc.def.ghi", 0, &len);
	CHECK_EQ_STR("Authorization: *** abc.def.ghi", out, g_test_name);
	return 0;
}

/* ========== 边界情况：无分隔符不脱敏 ========== */
static int test_no_separator_unchanged(void)
{
	test_case_begin("no_separator_unchanged");
	size_t len;
	char* out = do_sanitize("token expired, please retry", 0, &len);
	CHECK_EQ_STR("token expired, please retry", out, g_test_name);
	return 0;
}

static int test_wrong_separator_unchanged(void)
{
	test_case_begin("wrong_separator_unchanged");
	size_t len;
	char* out = do_sanitize("token-abc123", 0, &len);
	CHECK_EQ_STR("token-abc123", out, g_test_name);
	return 0;
}

/* ========== JSON 格式 ========== */
static int test_json_quoted_value(void)
{
	test_case_begin("json_quoted_value");
	size_t len;
	char* out = do_sanitize("{\"token\":\"deadbeef\",\"appid\":123}", 0, &len);
	CHECK_EQ_STR("{\"token\":\"***\",\"appid\":***}", out, g_test_name);
	return 0;
}

/* ========== URL query 格式 ========== */
static int test_url_query(void)
{
	test_case_begin("url_query");
	size_t len;
	char* out = do_sanitize("?appid=1001&token=abc&secret=xyz", 0, &len);
	CHECK_EQ_STR("?appid=***&token=***&secret=***", out, g_test_name);
	return 0;
}

/* ========== 多字段同消息 ========== */
static int test_multiple_fields(void)
{
	test_case_begin("multiple_fields");
	size_t len;
	char* out = do_sanitize("user=alice token=abc123 apikey=xyz", 0, &len);
	CHECK_EQ_STR("user=alice token=*** apikey=***", out, g_test_name);
	return 0;
}

/* ========== 引号边界 ========== */
static int test_single_quote(void)
{
	test_case_begin("single_quote");
	size_t len;
	char* out = do_sanitize("token='abc123', secret=def", 0, &len);
	CHECK_EQ_STR("token='***', secret=***", out, g_test_name);
	return 0;
}

static int test_unclosed_quote(void)
{
	test_case_begin("unclosed_quote");
	size_t len;
	char* out = do_sanitize("token=\"abcdef", 0, &len);
	CHECK_EQ_STR("token=\"***", out, g_test_name);
	return 0;
}

/* ========== body_offset 保护头部 ========== */
static int test_body_offset_protects_header(void)
{
	test_case_begin("body_offset_protects_header");
	/* 头部含 "token" 子串但不应被脱敏 */
	const char* input = "[D/Auth](login:12) token=abc123";
	size_t offset = (size_t)snprintf(NULL, 0, "[D/Auth](login:12) ");
	size_t len;
	char* out = do_sanitize(input, offset, &len);
	/* 头部不变，body 被脱敏 */
	CHECK_EQ_STR("[D/Auth](login:12) token=***", out, g_test_name);
	return 0;
}

/* ========== 自定义模式注册 ========== */
static int test_register_custom_pattern(void)
{
	test_case_begin("register_custom_pattern");
	/* 先清空（含内置），再注册新模式 */
	log_sanitize_clear_patterns();
	log_sanitize_register_pattern("device_sn");

	size_t len;
	char* out = do_sanitize("device_sn=SN12345", 0, &len);
	CHECK_EQ_STR("device_sn=***", out, g_test_name);

	/* 内置模式应失效（已清空） */
	out = do_sanitize("token=should_not_match", 0, &len);
	CHECK_EQ_STR("token=should_not_match", out, g_test_name);

	/* 恢复内置模式表 */
	log_sanitize_clear_patterns();
	log_sanitize_restore_builtins();
	return 0;
}

static int test_register_duplicate(void)
{
	test_case_begin("register_duplicate");
	int ret = log_sanitize_register_pattern("dup_key");
	ASSERT(ret == 0);
	ret = log_sanitize_register_pattern("dup_key");
	ASSERT(ret == -1);  /* 重复返回 -1 */

	/* 清理：清空后必须恢复内置表，避免污染后续依赖内置模式的用例 */
	log_sanitize_clear_patterns();
	log_sanitize_restore_builtins();
	return 0;
}

static int test_register_null_pattern(void)
{
	test_case_begin("register_null_pattern");
	int ret = log_sanitize_register_pattern(NULL);
	ASSERT(ret == -1);
	ret = log_sanitize_register_pattern("");
	ASSERT(ret == -1);
	return 0;
}

/* ========== 空消息 / body_offset 越界 ========== */
static int test_empty_message(void)
{
	test_case_begin("empty_message");
	char buf[SANITIZER_MAX_BUF] = "";
	size_t new_len = log_sanitize_in_place(buf, 0);
	ASSERT(new_len == 0);
	return 0;
}

static int test_body_offset_exceeds_len(void)
{
	test_case_begin("body_offset_exceeds_len");
	char buf[SANITIZER_MAX_BUF] = "short";
	size_t new_len = log_sanitize_in_place(buf, 100);
	ASSERT(new_len == strlen("short"));
	return 0;
}

/* ========== 大写变体组合 ========== */
static int test_uppercase_password(void)
{
	test_case_begin("uppercase_password");
	size_t len;
	char* out = do_sanitize("Password=MyPass123", 0, &len);
	CHECK_EQ_STR("Password=***", out, g_test_name);
	return 0;
}

static int test_uppercase_AppId(void)
{
	test_case_begin("uppercase_AppId");
	size_t len;
	char* out = do_sanitize("AppId=1001", 0, &len);
	CHECK_EQ_STR("AppId=***", out, g_test_name);
	return 0;
}

static int test_uppercase_session_id(void)
{
	test_case_begin("uppercase_session_id");
	size_t len;
	char* out = do_sanitize("SESSION_ID=sess123", 0, &len);
	CHECK_EQ_STR("SESSION_ID=***", out, g_test_name);
	return 0;
}

/* ========== 多模式同时命中 ========== */
static int test_multiple_patterns_same_msg(void)
{
	test_case_begin("multiple_patterns_same_msg");
	size_t len;
	char* out = do_sanitize("access_token=abc, token=xyz", 0, &len);
	CHECK_EQ_STR("access_token=***, token=***", out, g_test_name);
	return 0;
}

/* ========== 短值收敛（mask 长度 <= 值长，保证输出不增长） ==========
 * 回归：值长 < 3 时按值长收敛掩码（原固定 "***" 会增长输出，
 * 导致双指针 write 越过 scan，memmove 覆盖已写数据 + 越界）。 */
static int test_short_value_one_char(void)
{
	test_case_begin("short_value_one_char");
	size_t len;
	char* out = do_sanitize("appid=1", 0, &len);
	CHECK_EQ_STR("appid=*", out, g_test_name);
	ASSERT(len == strlen("appid=*"));  /* 输出必须不长于输入 */
	return 0;
}

static int test_short_value_two_chars(void)
{
	test_case_begin("short_value_two_chars");
	size_t len;
	char* out = do_sanitize("token=ab", 0, &len);
	CHECK_EQ_STR("token=**", out, g_test_name);
	ASSERT(len == strlen("token=**"));
	return 0;
}

static int test_short_value_three_chars(void)
{
	test_case_begin("short_value_three_chars");
	size_t len;
	/* 值长恰为 3，掩码保持 "***"，输出等长 */
	char* out = do_sanitize("secret=pw3", 0, &len);
	CHECK_EQ_STR("secret=***", out, g_test_name);
	ASSERT(len == strlen("secret=***"));
	return 0;
}

static int test_url_query_short_values(void)
{
	test_case_begin("url_query_short_values");
	size_t len;
	/* 多字段混合短值/长值：各自按自身长度收敛 */
	char* out = do_sanitize("?appid=1&token=2&secret=xyz", 0, &len);
	CHECK_EQ_STR("?appid=*&token=*&secret=***", out, g_test_name);
	return 0;
}

static int test_short_value_with_quotes(void)
{
	test_case_begin("short_value_with_quotes");
	size_t len;
	char* out = do_sanitize("{\"token\":\"a\"}", 0, &len);
	CHECK_EQ_STR("{\"token\":\"*\"}", out, g_test_name);
	return 0;
}

static int test_short_value_custom_pattern(void)
{
	test_case_begin("short_value_custom_pattern");
	log_sanitize_clear_patterns();
	log_sanitize_register_pattern("device_sn");

	size_t len;
	char* out = do_sanitize("device_sn=9", 0, &len);
	CHECK_EQ_STR("device_sn=*", out, g_test_name);

	log_sanitize_clear_patterns();
	log_sanitize_restore_builtins();
	return 0;
}

/* ========== NULL 安全 ========== */
static int test_null_msg(void)
{
	test_case_begin("null_msg");
	size_t new_len = log_sanitize_in_place(NULL, 0);
	ASSERT(new_len == 0);
	return 0;
}

/* ========== 注册器 ========== */
LCU_TEST_REGISTER(test_builtin_token, "builtin: token=abc -> token=***");
LCU_TEST_REGISTER(test_builtin_apikey_colon, "builtin: apikey: val -> apikey: ***");
LCU_TEST_REGISTER(test_builtin_access_token_suffix, "builtin: access_token=val (suffix match)");
LCU_TEST_REGISTER(test_builtin_password, "builtin: password=val -> password=***");
LCU_TEST_REGISTER(test_builtin_secret, "builtin: my_secret=val -> my_secret=***");
LCU_TEST_REGISTER(test_uppercase_Token, "uppercase: Token=val -> Token=***");
LCU_TEST_REGISTER(test_uppercase_APIKey, "uppercase: APIKey=val -> APIKey=***");
LCU_TEST_REGISTER(test_uppercase_TOKEN, "uppercase: TOKEN=val -> TOKEN=***");
LCU_TEST_REGISTER(test_authorization_bearer, "Authorization: Bearer xxx -> Authorization: Bearer ***");
LCU_TEST_REGISTER(test_no_separator_unchanged, "no separator: 'token expired' unchanged");
LCU_TEST_REGISTER(test_wrong_separator_unchanged, "wrong separator: 'token-abc' unchanged");
LCU_TEST_REGISTER(test_json_quoted_value, "JSON: {\"token\":\"val\"} sanitized");
LCU_TEST_REGISTER(test_url_query, "URL query: ?appid=1&token=2 sanitized");
LCU_TEST_REGISTER(test_multiple_fields, "multiple fields in one message");
LCU_TEST_REGISTER(test_single_quote, "single quote value boundary");
LCU_TEST_REGISTER(test_unclosed_quote, "unclosed quote terminated at string end");
LCU_TEST_REGISTER(test_body_offset_protects_header, "body_offset protects header from sanitize");
LCU_TEST_REGISTER(test_register_custom_pattern, "custom pattern register + clear");
LCU_TEST_REGISTER(test_register_duplicate, "duplicate pattern returns -1");
LCU_TEST_REGISTER(test_register_null_pattern, "NULL/empty pattern rejected");
LCU_TEST_REGISTER(test_empty_message, "empty message returns 0");
LCU_TEST_REGISTER(test_body_offset_exceeds_len, "body_offset > strlen returns strlen");
LCU_TEST_REGISTER(test_uppercase_password, "uppercase: Password=val");
LCU_TEST_REGISTER(test_uppercase_AppId, "uppercase: AppId=val");
LCU_TEST_REGISTER(test_uppercase_session_id, "uppercase: SESSION_ID=val");
LCU_TEST_REGISTER(test_multiple_patterns_same_msg, "multiple patterns hit same message");
LCU_TEST_REGISTER(test_short_value_one_char, "short value (1 char) masked to single '*'");
LCU_TEST_REGISTER(test_short_value_two_chars, "short value (2 chars) masked to '**'");
LCU_TEST_REGISTER(test_short_value_three_chars, "3-char value masked to '***' (equal length)");
LCU_TEST_REGISTER(test_url_query_short_values, "URL query mixed short/long values");
LCU_TEST_REGISTER(test_short_value_with_quotes, "short quoted value masked");
LCU_TEST_REGISTER(test_short_value_custom_pattern, "short value with custom pattern");
LCU_TEST_REGISTER(test_null_msg, "NULL msg returns 0");
