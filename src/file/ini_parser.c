#include "mem/mem_debug.h"
#include "file/ini_parser.h"
#include "file/ini_reader.h"
#include "data/list.h"
#include "common_macro.h"
#include "mem/strings.h" /* for strcmp */
#include "mem/stringbuilder.h"
#include <stdio.h>       /* for fopen */
#include <stdlib.h>      /* for atoi/atol/atof */

#define INI_VALUE_STACK_SIZE (256)
#define SECTION_NAME_MAX_SIZE (INI_VALUE_STACK_SIZE / 4)

typedef enum
{
	INI_VALUE_TYPE_BOOL,
	INI_VALUE_TYPE_DOUBLE,
	INI_VALUE_TYPE_FLOAT,
	INI_VALUE_TYPE_INT,
	INI_VALUE_TYPE_LONG,
	INI_VALUE_TYPE_LONG_LONG,
} INI_VALUE_TYPE;

typedef struct
{
	char section_name[SECTION_NAME_MAX_SIZE];
	list_t* plist_section;
} section_info_t;

typedef struct
{
	char key[SECTION_NAME_MAX_SIZE];
	char value[INI_VALUE_STACK_SIZE];
} key_value_t;

typedef struct
{
	int ret_code;
	stringbuilder_t* sb;
} dump_ini_context;

typedef struct
{
	ini_parser_handler user_handler;
	void* user_param;
	section_info_t* p_section_info;
} ini_foreach_context_t;

struct _ini_parser
{
	/* hold all sections, and each section(list_t *) hold key_value_t */
	list_t* plist_sections;
	/* prediction all ini config string size, for dump ini string */
	size_t prediction_str_size;
	/* last used section  */
	section_info_t* p_last_used_section;
};

static inline void* allocator_my_malloc(size_t size)
{
	return malloc(size);
}

static inline void* allocator_my_calloc(size_t item_count, size_t item_size)
{
	return calloc(item_count, item_size);
}

static inline void* allocator_my_strdup(char* str)
{
	return str ? (strdup(str)) : NULL;
}

static inline void allocator_my_free(void* ptr)
{
	if (ptr) free(ptr);
}

static ini_parser_code_e ini_parser_get_value(ini_parser_handle parser_p,
	const char* section, const char* key, void* value, INI_VALUE_TYPE value_type);

//ini reader callback, return non-zero for error
static int ini_handler_cb(void* user,
	const char* section, const char* key, const char* value
#if INI_HANDLER_LINENO
	, int lineno
#endif // INI_HANDLER_LINENO
)
{
	if (NULL == section || NULL == key)
	{
		return 0;// just continue to parse file
	}
	ini_parser_handle parser_p = (ini_parser_handle)user;
	return INI_PARSER_CODE_SUCCEED == ini_parser_put_string(parser_p, section, key, value);
}

bool ini_parser_is_file_path(const char* str)
{
	if (NULL == str || '\0' == str[0])
	{
		return false;
	}
	bool is_file_path = (NULL == strstr(str, "\n")); // check have new line
	if (is_file_path) // check for this situation: [section] a = b
	{
		size_t str_len = strlen(str);
		for (size_t i = 0; i < str_len; ++i)
		{
			if (isspace((int)str[i]))
			{
				continue;
			}
			if ('#' == str[i] || '[' == str[i])
			{
				is_file_path = false;
			}
			break;
		}
	}
	return is_file_path;
}

ini_parser_handle ini_parser_parse_str(const char* ini_content)
{
	list_t* plist_sections = list_new(allocator_my_free);
	if (!plist_sections)
	{
		return NULL;
	}
	ini_parser_handle parser_p = (ini_parser_handle)calloc(1, sizeof(struct _ini_parser));
	if (!parser_p)
	{
		list_free(plist_sections);
		return NULL;
	}
	parser_p->plist_sections = plist_sections;
	if (ini_content && 0 != ini_reader_parse_string(ini_content, ini_handler_cb, parser_p))
	{
		ini_parser_destroy(&parser_p);
		return NULL;
	}
	return parser_p;
}

ini_parser_handle ini_parser_create()
{
	return ini_parser_parse_str(NULL);
}

ini_parser_handle ini_parser_parse_file(const char* ini_file)
{
	if (!ini_file || strlen(ini_file) < 1)
	{
		return NULL;
	}
	ini_parser_handle parser = NULL;
	char* ini_content = NULL;
	FILE* fp = fopen(ini_file, "rb");
	do
	{
		if (!fp)
		{
			break;
		}
		fseek(fp, 0L, SEEK_END);
		long file_size = ftell(fp);
		if (file_size < 6)
		{
			break;
		}
		fseek(fp, 0L, SEEK_SET);
		ini_content = (char*)malloc((size_t)file_size + 1U);
		if (!ini_content)
		{
			break;
		}
		ini_content[file_size] = '\0';
		if (fread(ini_content, 1, (size_t)file_size, fp) != (size_t)file_size)
		{
			break;
		}
		parser = ini_parser_parse_str(ini_content);
	} while (0);
	if (fp)
	{
		fclose(fp);
	}
	if (ini_content)
	{
		free(ini_content);
	}
	return parser;
}


// foreach key-value of target section
static bool iter_foreach_section_key_value(void* data, void* context)
{
	ini_foreach_context_t* context_p = (ini_foreach_context_t*)context;
	key_value_t* p_kv = (key_value_t*)data;
	if (NULL == p_kv || '\0' == p_kv->key[0])
	{
		return true; // just ignored empty key and continue
	}
	return (0 == context_p->user_handler(context_p->p_section_info->section_name,
		p_kv->key, p_kv->value, context_p->user_param));
}

// foreach sections, for get key-value of section
static bool iter_foreach_section(void* data, void* context)
{
	ini_foreach_context_t* context_p = (ini_foreach_context_t*)context;
	context_p->p_section_info = (section_info_t*)data;
	if (NULL == context_p->p_section_info
		|| NULL == context_p->p_section_info->plist_section
		|| '\0' == context_p->p_section_info->section_name[0])
	{
		return true; // just ignored empty section and continue
	}

	list_foreach(context_p->p_section_info->plist_section,
		iter_foreach_section_key_value, context_p);
	return true;
}

ini_parser_code_e ini_parser_foreach(ini_parser_handle parser_p, ini_parser_handler handler, void* user)
{
	if (!parser_p || !handler)
	{
		return INI_PARSER_CODE_INVALID_PARAM;
	}
	ini_foreach_context_t context =
	{
	  .user_handler = handler,
	  .user_param = user,
	  .p_section_info = NULL,
	};
	// foreach all sections 
	list_foreach(parser_p->plist_sections, iter_foreach_section, &context);
	return 0;
}

// return true for continue iter, false for break iter. 
// so if you find your data, just return false
static bool iter_for_search_target_section(void* data, void* context)
{
	char* section_name = (char*)context;
	section_info_t* p_section_info = (section_info_t*)data;
	return 0 != strcasecmp(section_name, p_section_info->section_name);
}

static section_info_t* search_target_section(ini_parser_handle parser_p, bool auto_create,
	const char* section, const size_t section_len)
{
	if (!section || section_len == 0) return NULL;
	if (parser_p->p_last_used_section &&
		0 == strcasecmp(section, parser_p->p_last_used_section->section_name))
	{
		return parser_p->p_last_used_section; //hit cache, no need search on root list.
	}
	if (!auto_create && 0 == list_length(parser_p->plist_sections)) return NULL;
	list_node_t* target_section_node = list_foreach((const list_t*)parser_p->plist_sections,
		iter_for_search_target_section, (void*)section);
	section_info_t* target_section = target_section_node ? (section_info_t*)list_node(target_section_node) : NULL;
	if (auto_create && !target_section)
	{
		target_section = (section_info_t*)allocator_my_calloc(1, sizeof(section_info_t));
		if (!target_section)
		{
			return NULL;
		}
		target_section->plist_section = list_new(allocator_my_free);
		if (NULL == target_section->plist_section)
		{
			allocator_my_free(target_section);
			return NULL;
		}
		strlcpy(target_section->section_name, section, sizeof(target_section->section_name));
		if (false == list_append(parser_p->plist_sections, target_section))
		{
			allocator_my_free(target_section);
			return NULL;
		}
		parser_p->prediction_str_size += (section_len + 6U); /* 2 for square brackets, and 4 for two \r\n */
		parser_p->p_last_used_section = target_section;
	}
	return target_section;
}

// return true for continue iter, false for break iter. 
// so if you find your data, just return false.
static bool iter_for_search_key_value(void* data, void* context)
{
	key_value_t* p_kv = (key_value_t*)data;
	char* key = (char*)context;
	return 0 != strcasecmp(key, p_kv->key);
}

static key_value_t* iter_section_for_search_target_kv(list_t* section, const char* key)
{
	if (0 == list_length(section)) return NULL;
	list_node_t* target_node = list_foreach(section, iter_for_search_key_value, (void*)key);
	return target_node ? (key_value_t*)list_node(target_node) : NULL;
}

ini_parser_code_e ini_parser_put_string(ini_parser_handle parser_p,
	const char* section, const char* key, const char* value)
{
	if (!parser_p || !key || '\0' == key[0])
	{
		return INI_PARSER_CODE_INVALID_PARAM;
	}
	const size_t section_len = strlen(section);
	const size_t key_len = strlen(key);
	if (section_len < 1 || key_len < 1)
	{
		return INI_PARSER_CODE_INVALID_PARAM;
	}
	section_info_t* target_section = search_target_section(parser_p, true, section, section_len);
	if (!target_section)
	{
		return INI_PARSER_CODE_NO_ENOUGH_MEMORY;
	}
	key_value_t* p_kv = iter_section_for_search_target_kv(target_section->plist_section, key);
	bool is_update = (NULL != p_kv);
	if (!is_update)
	{
		p_kv = (key_value_t*)allocator_my_calloc(1, sizeof(key_value_t));
		if (!p_kv)
		{
			return INI_PARSER_CODE_NO_ENOUGH_MEMORY;
		}
		strlcpy(p_kv->key, key, sizeof(p_kv->key));
	}
	strlcpy(p_kv->value, (value ? value : ""), sizeof(p_kv->value));
	if (is_update) return INI_PARSER_CODE_SUCCEED;
	if (true == list_append(target_section->plist_section, p_kv))
	{
		return INI_PARSER_CODE_SUCCEED;
	}
	else
	{
		allocator_my_free(p_kv);
		return INI_PARSER_CODE_FAILED;
	}
}

ini_parser_code_e ini_parser_get_string(ini_parser_handle parser_p,
	const char* section, const char* key, char* value, const size_t value_size)
{
	if (!parser_p || !section)
	{
		return INI_PARSER_CODE_INVALID_PARAM;
	}
	const size_t section_len = strlen(section);
	if (0 == section_len)
	{
		return INI_PARSER_CODE_INVALID_PARAM;
	}

	section_info_t* target_section = search_target_section(parser_p, false, section, section_len);
	if (!target_section)
	{
		return INI_PARSER_CODE_NOT_FOUND_SECTION_KEY;
	}
	if (!key && !value && 0 == value_size)
	{
		return INI_PARSER_CODE_SUCCEED; //user want to detect section exists
	}
	if (!key || '\0' == key[0])
	{
		return INI_PARSER_CODE_INVALID_PARAM;
	}
	key_value_t* p_kv = iter_section_for_search_target_kv(target_section->plist_section, key);
	if (!p_kv)
	{
		return INI_PARSER_CODE_NOT_FOUND_SECTION_KEY;
	}
	if (value && value_size)
	{
		size_t the_value_len = strlen(p_kv->value);
		if (value_size <= the_value_len)
		{
			return INI_PARSER_CODE_NO_ENOUGH_MEMORY;
		}
		memcpy(value, p_kv->value, the_value_len + 1);
	}
	return INI_PARSER_CODE_SUCCEED;
}

// delete target section key
ini_parser_code_e ini_parser_delete_by_section_key(ini_parser_handle parser_p,
	const char* section, const char* key)
{
	if (!parser_p || !section || !key)
	{
		return INI_PARSER_CODE_INVALID_PARAM;
	}
	section_info_t* p_section = search_target_section(parser_p, false, section, strlen(section));
	if (!p_section)
	{
		return INI_PARSER_CODE_NOT_FOUND_SECTION_KEY;
	}
	key_value_t* p_kv = iter_section_for_search_target_kv(p_section->plist_section, key);
	if (!p_kv)
	{
		return INI_PARSER_CODE_NOT_FOUND_SECTION_KEY;
	}
	return list_remove(p_section->plist_section, p_kv) ? INI_PARSER_CODE_SUCCEED : INI_PARSER_CODE_FAILED;
}

// delete target section, all key-value in this section will deleted.
ini_parser_code_e ini_parser_delete_section(ini_parser_handle parser_p, const char* section)
{
	if (!parser_p || !section)
	{
		return INI_PARSER_CODE_INVALID_PARAM;
	}
	section_info_t* p_section = search_target_section(parser_p, false, section, strlen(section));
	if (!p_section)
	{
		return INI_PARSER_CODE_NOT_FOUND_SECTION_KEY;
	}
	list_free(p_section->plist_section);
	return list_remove(parser_p->plist_sections, p_section) ? INI_PARSER_CODE_SUCCEED : INI_PARSER_CODE_FAILED;
}

static bool iter_key_value_for_dump(void* data, void* context)
{
	if (!data || !context) // data is pointer of key_value_t.
	{
		return true; // just continue
	}
	key_value_t* p_kv = (key_value_t*)data;
	dump_ini_context* dump_ctx = (dump_ini_context*)context;
	if (0 != (dump_ctx->ret_code = stringbuilder_appendf(dump_ctx->sb, "%s = %s\r\n", p_kv->key, p_kv->value)))
	{
#if(!defined(NDEBUG) || defined(_DEBUG))
		printf("ERROR[%s:%d]: failed on append %s=%s to stringbuilder. %d",
			__FILE__, __LINE__, p_kv->key, p_kv->value, dump_ctx->ret_code);
#endif // !NDEBUG || _DEBUG
		ASSERT_ABORT(dump_ctx->ret_code);
		return false;
	}
	return true;
}

static bool iter_section_for_dump(void* data, void* context)
{
	if (!data || !context) // data is pointer of section_info_t.
	{
		return true;
	}
	section_info_t* p_section = (section_info_t*)data;
	dump_ini_context* dump_ctx = (dump_ini_context*)context;
	if (0 != dump_ctx->ret_code)
	{
		return false; // error occurred on foreach last section, break chain.
	}
	stringbuilder_appendf(dump_ctx->sb, "[%s]\r\n", (char*)p_section->section_name);
	list_foreach(p_section->plist_section, iter_key_value_for_dump, dump_ctx);
	stringbuilder_appendstr(dump_ctx->sb, "\r\n");
	return true;
}

static stringbuilder_t* pri_dump_ini(ini_parser_handle parser_p)
{
	const size_t sb_mem_size = parser_p->prediction_str_size + 64U;
	stringbuilder_t* sb = stringbuilder_create(sb_mem_size);
	if (!sb)
	{
		return NULL;
	}
	dump_ini_context dump_context =
	{
		.ret_code = 0,
		.sb = sb,
	};
	list_foreach(parser_p->plist_sections, iter_section_for_dump, &dump_context);
	if (0 != dump_context.ret_code)
	{
		stringbuilder_destroy(&sb);
		return NULL;
	}
	return sb;
}

ini_parser_code_e ini_parser_dump_to_mem(ini_parser_handle parser_p, char* mem, size_t* mem_size_p)
{
	if (!parser_p || !mem || !mem_size_p)
	{
		return INI_PARSER_CODE_INVALID_PARAM;
	}
	if (*mem_size_p < (parser_p->prediction_str_size + 64U))
	{
		return INI_PARSER_CODE_NO_ENOUGH_MEMORY;
	}
	stringbuilder_t* sb = pri_dump_ini(parser_p);
	if (NULL == sb)
	{
		return INI_PARSER_CODE_FAILED;
	}
	ini_parser_code_e parser_code = INI_PARSER_CODE_FAILED;
	const char* ini_string = stringbuilder_to_string(sb);
	do 
	{
		if (NULL == ini_string || '\0' == ini_string[0])
		{
			break;
		}
		size_t ini_string_size = strlen(ini_string) + 1U;
		if (*mem_size_p < ini_string_size)
		{
			parser_code = INI_PARSER_CODE_NO_ENOUGH_MEMORY;
			break;
		}
		memcpy(mem, ini_string, ini_string_size);
		*mem_size_p = ini_string_size;
		parser_code = INI_PARSER_CODE_SUCCEED;
	} while (0);
	stringbuilder_destroy(&sb);
	return parser_code;
}

char* ini_parser_dump(ini_parser_handle parser_p)
{
	if (!parser_p)
	{
		return NULL;
	}
	stringbuilder_t* sb = pri_dump_ini(parser_p);
	if (NULL == sb)
	{
		return NULL;
	}
	char* ini_string = strdup(stringbuilder_to_string(sb));
	stringbuilder_destroy(&sb);
	return ini_string;
}

ini_parser_code_e ini_parser_save(ini_parser_handle parser_p, const char* file_path)
{
	if (!parser_p || !file_path || '\0' == file_path[0])
	{
		return INI_PARSER_CODE_INVALID_PARAM;
	}
	FILE* fp = fopen(file_path, "wb");
	if (!fp)
	{
		return INI_PARSER_CODE_INVALID_PARAM;
	}
	char* ini_str = ini_parser_dump(parser_p);
	if (!ini_str || '\0' == ini_str[0])
	{
		if (ini_str)
		{
			free(ini_str);
		}
		return INI_PARSER_CODE_FAILED;
	}
	size_t ini_str_len = strlen(ini_str);
	fwrite(ini_str, 1, ini_str_len, fp);
	free(ini_str);
	fclose(fp);
	return INI_PARSER_CODE_SUCCEED;
}

ini_parser_code_e ini_parser_has_section_key(ini_parser_handle parser_p, const char* section, const char* key)
{
	return ini_parser_get_string(parser_p, section, key, NULL, 0);
}

ini_parser_code_e ini_parser_has_section(ini_parser_handle parser_p, const char* section)
{
	return ini_parser_has_section_key(parser_p, section, NULL);
}

ini_parser_code_e ini_parser_get_bool(ini_parser_handle parser_p, const char* section, const char* key, bool* value)
{
	return ini_parser_get_value(parser_p, section, key, value, INI_VALUE_TYPE_BOOL);
}

ini_parser_code_e ini_parser_get_double(ini_parser_handle parser_p, const char* section, const char* key, double* value)
{
	return ini_parser_get_value(parser_p, section, key, value, INI_VALUE_TYPE_DOUBLE);
}

ini_parser_code_e ini_parser_get_float(ini_parser_handle parser_p, const char* section, const char* key, float* value)
{
	return ini_parser_get_value(parser_p, section, key, value, INI_VALUE_TYPE_FLOAT);
}

ini_parser_code_e ini_parser_get_int(ini_parser_handle parser_p, const char* section, const char* key, int* value)
{
	return ini_parser_get_value(parser_p, section, key, value, INI_VALUE_TYPE_INT);
}

ini_parser_code_e ini_parser_get_long(ini_parser_handle parser_p, const char* section, const char* key, long* value)
{
	return ini_parser_get_value(parser_p, section, key, value, INI_VALUE_TYPE_LONG);
}

ini_parser_code_e ini_parser_get_long_long(ini_parser_handle parser_p, const char* section, const char* key, long long* value)
{
	return ini_parser_get_value(parser_p, section, key, value, INI_VALUE_TYPE_LONG_LONG);
}

static bool iter_for_delete_all_section(void* data, void* context)
{
	if (!data) // data is section_info_t *
	{
		return true;
	}
	section_info_t* p_sec_info = (section_info_t*)data;
	list_free(p_sec_info->plist_section);
	return true;
}

ini_parser_code_e ini_parser_destroy(ini_parser_handle* parser_pp)
{
	if (NULL == parser_pp || NULL == *parser_pp)
	{
		return INI_PARSER_CODE_INVALID_PARAM;
	}
	ini_parser_handle parser_p = *parser_pp;
	if (parser_p->plist_sections)
	{
		//delete all section
		list_foreach(parser_p->plist_sections, iter_for_delete_all_section, NULL);
		list_free(parser_p->plist_sections);
		parser_p->plist_sections = NULL;
	}
	free(parser_p);
	*parser_pp = NULL;
	return INI_PARSER_CODE_SUCCEED;
}

static long long parse_str2longlong(ini_parser_code_e* code_p, char* str_value)
{
	char* end_ptr = NULL;
	long long result = strtoll(str_value, &end_ptr, 0); // <-- when 0 == radix, it will auto detect radix 
	*code_p = (!end_ptr || '\0' != *end_ptr) ? INI_PARSER_CODE_FAILED : INI_PARSER_CODE_SUCCEED;
	return result;
}

static ini_parser_code_e ini_parser_get_value(ini_parser_handle parser_p,
	const char* section, const char* key, void* value, INI_VALUE_TYPE value_type)
{
	char str_value[INI_VALUE_STACK_SIZE] = { 0 };
	ini_parser_code_e ret = ini_parser_get_string(parser_p, section, key, str_value, sizeof(str_value));
	if (INI_PARSER_CODE_SUCCEED != ret)
	{
		return ret;
	}
	if ('\0' == str_value[0])
	{
		return INI_PARSER_CODE_FAILED;
	}

	char* end_ptr = NULL;

#define STR2NUM_TYPE(type, trans_func) type result = trans_func(str_value, &end_ptr); \
if (!end_ptr || *end_ptr != '\0') { ret = INI_PARSER_CODE_FAILED; break; }\
*((type*)value) = result;

	switch (value_type)
	{
	case INI_VALUE_TYPE_BOOL:
	{
		bool result = (0 == strncasecmp(str_value, "true", 4));
		do
		{
			if (result)
			{
				break;
			}
			if (0 == strncasecmp(str_value, "false", 5))
			{
				result = false;
				break;
			}
			if (0 == strncasecmp(str_value, "yes", 3))
			{
				result = true;
				break;
			}
			if (0 == strncasecmp(str_value, "no", 2))
			{
				result = false;
				break;
			}
			int num = (int)parse_str2longlong(&ret, str_value);
			if (INI_PARSER_CODE_SUCCEED == ret)
			{
				result = (0 == num ? false : true);
			}
		} while (0);
		*((bool*)value) = result;
	}
	break;
	case INI_VALUE_TYPE_DOUBLE:
	{
		STR2NUM_TYPE(double, strtod)
	}
	break;
	case INI_VALUE_TYPE_FLOAT:
	{
		STR2NUM_TYPE(float, strtof)
	}
	break;
	case INI_VALUE_TYPE_INT:
		*((int*)value) = (int)parse_str2longlong(&ret, str_value);
		break;
	case INI_VALUE_TYPE_LONG:
		*((long*)value) = (long)parse_str2longlong(&ret, str_value);
		break;
	case INI_VALUE_TYPE_LONG_LONG:
		*((long long*)value) = parse_str2longlong(&ret, str_value);
		break;
	default:
		ret = INI_PARSER_CODE_FAILED;
		break;
	}
	return ret;
}
