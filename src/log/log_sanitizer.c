#include "mem/mem_debug.h"
#include "log/log_sanitizer.h"
#include "common_macro.h"
#include "mem/strings.h"   /* for strdup/strlcpy cross-platform mapping */

#include <string.h>

/* 
 * 内置敏感键模式表（裸键名，后缀匹配自动覆盖衍生形式）。
 * 覆盖: token/apikey/secret/password/appid/credential/authorization/session_id
 * 及其大写变体（JSON/HTTP Header 常见形式）。 
 */
static const char* g_builtin_patterns_orig[] = {
	"token", "apikey", "api_key", "secret", "password", "passwd",
	"appid", "credential", "authorization", "session_id",
	"Token", "TOKEN", "ApiKey", "APIKey", "API_KEY", "Secret",
	"Password", "PASSWORD", "AppId", "APPID", "Authorization",
	"SessionId", "SESSION_ID",
	NULL
};
static const char** g_builtin_patterns = g_builtin_patterns_orig;

typedef struct sanitize_pattern_node
{
	char* pattern;
	struct sanitize_pattern_node* next;
} sanitize_pattern_node_t;

static sanitize_pattern_node_t* g_custom_patterns = NULL;

int log_sanitize_register_pattern(const char* pattern)
{
	if (NULL == pattern || '\0' == pattern[0])
	{
		return -1;
	}

	/* 查重（遍历既有链表） */
	for (sanitize_pattern_node_t* p = g_custom_patterns; p; p = p->next)
	{
		if (0 == strcmp(p->pattern, pattern))
		{
			return -1;
		}
	}

	/* strdup + 链表头插（strdup 在 Win 上由 strings.h 映射到 _strdup） */
	sanitize_pattern_node_t* node = (sanitize_pattern_node_t*)malloc(sizeof(sanitize_pattern_node_t));
	if (NULL == node)
	{
		return -1;
	}
	node->pattern = strdup(pattern);
	if (NULL == node->pattern)
	{
		FREE(node);
		return -1;
	}
	node->next = g_custom_patterns;
	g_custom_patterns = node;
	return 0;
}

void log_sanitize_clear_patterns(void)
{
	sanitize_pattern_node_t* p = g_custom_patterns;
	while (NULL != p)
	{
		sanitize_pattern_node_t* next = p->next;
		FREE(p->pattern);
		FREE(p);
		p = next;
	}
	g_custom_patterns = NULL;

	/* 清空内置模式表（设计要求：允许调用方完全自定义） */
	g_builtin_patterns = NULL;
}

/* 恢复内置默认模式表（测试/调试用） */
void log_sanitize_restore_builtins(void)
{
	g_builtin_patterns = g_builtin_patterns_orig;
}

/* 跳过空白（空格/Tab/CR/LF），返回首个非空白字符位置 */
static const char* skip_whitespace(const char* s)
{
	while (' ' == *s || '\t' == *s || '\r' == *s || '\n' == *s)
	{
		++s;
	}
	return s;
}

/* 查找裸值终止位置：遇分隔符/空白/结尾停止 */
static const char* find_bare_value_end(const char* value_start)
{
	const char* p = value_start;
	while ('\0' != *p)
	{
		if (',' == *p || ';' == *p || '&' == *p ||
			'}' == *p || ']' == *p || ')' == *p ||
			'"' == *p || '\'' == *p ||
			' ' == *p || '\t' == *p || '\r' == *p || '\n' == *p)
		{
			break;
		}
		++p;
	}
	return p;
}

/*
 * 单个模式的匹配+原地替换。
 * 返回值: 改写后从 msg 起点算起的新长度。
 */
static size_t sanitize_one_pattern(char* msg, const char* pattern)
{
	const size_t pattern_len = strlen(pattern);
	char* scan = msg;
	char* write = msg;

	while ('\0' != *scan)
	{
		/* strstr 查找模式出现位置 */
		char* match = strstr(scan, pattern);
		if (NULL == match)
		{
			/* 无更多匹配，拷贝剩余部分 */
			size_t remain = strlen(scan);
			if (write != scan)
			{
				memmove(write, scan, remain + 1U);
			}
			return (write - msg) + remain;
		}

		/* 拷贝匹配前的非敏感内容 */
		if (match > scan)
		{
			size_t pre_len = (size_t)(match - scan);
			if (write != scan)
			{
				memmove(write, scan, pre_len);
			}
			write += pre_len;
		}

		/* 验证右侧分隔符：跳过可选空白/引号后必须为 '=' 或 ':'。
		 * 引号出现在 key 与分隔符之间（如 JSON "token":）也算有效分隔，
		 * 否则 JSON 等常见格式无法脱敏。 
		 */
		const char* after_key = skip_whitespace(match + pattern_len);
		char key_quote = ('"' == *after_key || '\'' == *after_key) ? *after_key : '\0';
		if ('\0' != key_quote)
		{
			++after_key;
		}
		char sep = *after_key;
		if ('=' != sep && ':' != sep)
		{
			/* 非有效分隔符：原样拷贝模式，从匹配位置后 1 字节继续
			 * （不能用 match+pattern_len，否则会跳过剩余字符导致漏匹配） 
			 */
			memmove(write, match, pattern_len);
			write += pattern_len;
			scan = match + pattern_len;
			continue;
		}

		/* 定位值起点：分隔符后跳过空白，识别值首引号 */
		const char* after_sep = after_key + 1;
		const char* ws_start = after_sep;
		after_sep = skip_whitespace(after_sep);
		size_t ws_len = (size_t)(after_sep - ws_start);

		const char* value_start = after_sep;
		char quote = ('"' == *value_start || '\'' == *value_start) ? *value_start : '\0';
		if ('\0' != quote)
		{
			++value_start;
		}

		/* JSON 等格式在 value 前存在闭合 key 的引号，输出时需补回 */
		const char* value_end;
		if ('\0' != quote)
		{
			const char* q = value_start;
			while ('\0' != *q && quote != *q)
			{
				++q;
			}
			value_end = q;
		}
		else
		{
			value_end = find_bare_value_end(value_start);
		}

		/* 掩码长度：固定 ***，但值长不足 3 时按值长收敛。
		 * 这保证单字段输出 <= 输入，使原地双指针 write 永不越过 scan
		 * （否则短值替换为 *** 会增长，导致后续 memmove 覆盖已写数据 + 越界）。 
		 */
		const size_t value_len = (size_t)(value_end - value_start);
		const size_t mask_len = (value_len >= 3U) ? 3U : value_len;

		/* 写入 key + (key_quote?) + sep + 保留空白 + 引号? + 掩码 + 引号? */
		memmove(write, match, pattern_len);
		write += pattern_len;
		if ('\0' != key_quote)
		{
			*write++ = key_quote;
		}
		*write++ = sep;
		if (ws_len > 0)
		{
			memmove(write, ws_start, ws_len);
			write += ws_len;
		}
		if ('\0' != quote)
		{
			*write++ = quote;
		}
		memset(write, '*', mask_len);
		write += mask_len;
		if ('\0' != quote && quote == *value_end)
		{
			*write++ = quote;
		}

		/* 跳过原值及可选的右引号 */
		scan = (char*)value_end;
		if ('\0' != quote && quote == *scan)
		{
			++scan;
		}
	}

	*write = '\0';
	return (size_t)(write - msg);
}

size_t log_sanitize_in_place(char* msg, size_t body_offset)
{
	if (NULL == msg)
	{
		return 0;
	}

	/* body_offset 超出字符串长度时直接返回，避免越界 */
	if (body_offset >= strlen(msg))
	{
		return strlen(msg);
	}

	char* body = msg + body_offset;
	size_t body_len = strlen(body);

	/* 两张表都为空时零开销返回。
	 * 注意：仅内置表被清空但注册了自定义模式时仍需处理自定义，不能在此提前返回。 
	 */
	if (NULL == g_builtin_patterns && NULL == g_custom_patterns)
	{
		return body_offset + body_len;
	}

	/* 依次用内置模式脱敏（NULL 时跳过，避免解引用空指针） */
	if (NULL != g_builtin_patterns)
	{
		for (const char** p = g_builtin_patterns; NULL != *p; ++p)
		{
			body_len = sanitize_one_pattern(body, *p);
		}
	}

	/* 依次用自定义模式脱敏 */
	for (sanitize_pattern_node_t* node = g_custom_patterns; NULL != node; node = node->next)
	{
		body_len = sanitize_one_pattern(body, node->pattern);
	}

	return body_offset + body_len;
}
