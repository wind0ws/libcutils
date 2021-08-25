#include "mem/mem_debug.h"
#include "file/ini_parser.h"
#include "file/ini_reader.h"
#include "data/list.h"
#include "common_macro.h"
//#include "mem/mplite.h"
#include "mem/strings.h" /* for strcmp */
#include "mem/stringbuilder.h"
#include <stdio.h>       /* for fopen */
#include <stdlib.h>      /* for atoi/atol/atof */

#define INI_VALUE_STACK_SIZE (128)
#define SECTION_NAME_MAX_SIZE INI_VALUE_STACK_SIZE

typedef enum
{
	INI_VALUE_TYPE_INT,
	INI_VALUE_TYPE_LONG,
	INI_VALUE_TYPE_DOUBLE,
	INI_VALUE_TYPE_BOOL
} INI_VALUE_TYPE;

typedef struct
{
	char section_name[SECTION_NAME_MAX_SIZE];
	list_t* plist_section;
} section_info_t;

typedef struct
{
	char key[INI_VALUE_STACK_SIZE];
	char value[INI_VALUE_STACK_SIZE];
} key_value_t;

typedef struct
{
	int ret_code;
	stringbuilder_t* sb;
} dump_ini_context;

struct _ini_parser
{
	/* hold all sections, and each section(list_t *) hold key_value_t */
	list_t* plist_sections;
	/* prediction ini string size */
	size_t prediction_str_size;
	/* last used section  */
	section_info_t *p_last_used_section;
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

static INI_PARSER_CODE ini_parser_get_value(ini_parser_ptr parser_p,
	const char* section, const char* key, void* value, INI_VALUE_TYPE value_type);

//ini reader callback, return true for continue, return false for abort
static bool ini_handler_cb(void* user,
	const char* section, const char* key, const char* value
#if INI_HANDLER_LINENO
	, int lineno
#endif // INI_HANDLER_LINENO
)
{
	if (NULL == section || NULL == key)
	{
		return true;// just continue to parse file
	}
	ini_parser_ptr parser_p = (ini_parser_ptr)user;
	return ini_parser_put_string(parser_p, section, key, value) == INI_PARSER_ERR_SUCCEED;
}

ini_parser_ptr ini_parser_parse_str(const char* ini_content)
{
	list_t *plist_sections = list_new(allocator_my_free);
	if (!plist_sections)
	{
		return NULL;
	}
	ini_parser_ptr parser_p = (ini_parser_ptr)calloc(1, sizeof(struct _ini_parser));
	if (!parser_p)
	{
		list_free(plist_sections);
		return NULL;
	}
	parser_p->plist_sections = plist_sections;
	if (ini_content && ini_parse_string(ini_content, ini_handler_cb, parser_p) != 0)
	{
		ini_parser_destory(&parser_p);
		return NULL;
	}
	return parser_p;
}

ini_parser_ptr ini_parser_create()
{
	return ini_parser_parse_str(NULL);
}

ini_parser_ptr ini_parser_parse_file(const char* ini_file)
{
	if (!ini_file || strlen(ini_file) < 1)
	{
		return NULL;
	}
	ini_parser_ptr parser = NULL;
	char* ini_content = NULL;
	FILE* fp = fopen(ini_file, "rb");
	do
	{
		if (!fp)
		{
			break;
		}
		fseek(fp, 0, SEEK_END);
		long file_size = ftell(fp);
		if (file_size < 6)
		{
			break;
		}
		fseek(fp, 0, SEEK_SET);
		ini_content = (char*)malloc((size_t)file_size);
		if (!ini_content)
		{
			break;
		}
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

// return true for continue iter, false for break iter. 
// so if you find your data, just return false
static bool iter_for_search_target_section(void* data, void* context)
{
	char* section_name = (char *)context;
	section_info_t* p_section_info = (section_info_t*)data;
	return strcasecmp(section_name, p_section_info->section_name) != 0;
}

static section_info_t* search_target_section(ini_parser_ptr parser_p, bool auto_create,
	const char* section, const size_t section_len)
{
	if (!section || section_len == 0) return NULL;
	if (parser_p->p_last_used_section &&
		strcasecmp(section, parser_p->p_last_used_section->section_name) == 0)
	{
		return parser_p->p_last_used_section; //hit cache, no need search on root list.
	}
	if (!auto_create && list_length(parser_p->plist_sections) == 0) return NULL;
	list_node_t *target_section_node = list_foreach((const list_t *)parser_p->plist_sections, 
										iter_for_search_target_section, (void *)section);
	section_info_t* target_section = target_section_node ? (section_info_t*)list_node(target_section_node) : NULL;
	if (auto_create && !target_section)
	{
		target_section = (section_info_t *)allocator_my_calloc(1, sizeof(section_info_t));
		if (!target_section)
		{
			return NULL;
		}
		target_section->plist_section = list_new(allocator_my_free);
		if (target_section->plist_section == NULL)
		{
			allocator_my_free(target_section);
			return NULL;
		}
		strlcpy(target_section->section_name, section, sizeof(target_section->section_name));
		list_append(parser_p->plist_sections, target_section);
		parser_p->prediction_str_size += (section_len + 6U); /* 2 for square brackets, and 4 for 2 \r\n */
		parser_p->p_last_used_section = target_section;
	}
	return target_section;
}

// return true for continue iter, false for break iter. 
// so if you find your data, just return false
static bool iter_for_search_key_value(void* data, void* context)
{
	key_value_t* p_kv = (key_value_t*)data;
	char* key = (char*)context;
	return strcasecmp(key, p_kv->key) != 0;
}

static key_value_t* search_target_kv(list_t* section, const char* key)
{
	if (list_length(section) == 0) return NULL;
	list_node_t* target_node = list_foreach(section, iter_for_search_key_value, (void *)key);
	return target_node ? (key_value_t*)list_node(target_node) : NULL;
}

INI_PARSER_CODE ini_parser_put_string(ini_parser_ptr parser_p,
	const char* section, const char* key, const char* value)
{
	if (!parser_p || !key || key[0] == '\0')
	{
		return INI_PARSER_ERR_INVALID_PARAM;
	}
	const size_t section_len = strlen(section);
	const size_t key_len = strlen(key);
	if (section_len < 1 || key_len < 1)
	{
		return INI_PARSER_ERR_INVALID_PARAM;
	}
	section_info_t* target_section = search_target_section(parser_p, true, section, section_len);
	if (!target_section)
	{
		return INI_PARSER_ERR_NOT_ENOUGH_MEMORY;
	}
	key_value_t* p_kv = search_target_kv(target_section->plist_section, key);
	bool is_update = (p_kv != NULL);
	if (!is_update)
	{
		p_kv = (key_value_t *)allocator_my_calloc(1, sizeof(key_value_t));
		if (!p_kv)
		{
			return INI_PARSER_ERR_NOT_ENOUGH_MEMORY;
		}
		strlcpy(p_kv->key, key, sizeof(p_kv->key));
	}
	strlcpy(p_kv->value, (value ? value : ""), sizeof(p_kv->value));
	if (is_update) return INI_PARSER_ERR_SUCCEED;
	return list_append(target_section->plist_section, p_kv) == true ?
					INI_PARSER_ERR_SUCCEED : INI_PARSER_ERR_FAILED;
}

INI_PARSER_CODE ini_parser_get_string(ini_parser_ptr parser_p,
	const char* section, const char* key, char* value, const size_t value_size)
{
	if (!parser_p || !section || !key)
	{
		return INI_PARSER_ERR_INVALID_PARAM;
	}
	const size_t section_len = strlen(section);
	const size_t key_len = strlen(key);
	if (!section_len || !key_len)
	{
		return INI_PARSER_ERR_INVALID_PARAM;
	}

	section_info_t* target_section = search_target_section(parser_p, false, section, section_len);
	if (!target_section)
	{
		return INI_PARSER_ERR_SECTION_KEY_NOT_FOUND;
	}
	key_value_t* p_kv = search_target_kv(target_section->plist_section, key);
	if (!p_kv)
	{
		return INI_PARSER_ERR_SECTION_KEY_NOT_FOUND;
	}
	if (value && value_size)
	{
		size_t the_value_len = strlen(p_kv->value);
		if (value_size <= the_value_len)
		{
			return INI_PARSER_ERR_NOT_ENOUGH_MEMORY;
		}
		memcpy(value, p_kv->value, the_value_len + 1);
	}
	return INI_PARSER_ERR_SUCCEED;
}

// delete target section key
INI_PARSER_CODE ini_parser_delete_by_section_key(ini_parser_ptr parser_p,
	const char* section, const char* key)
{
	if (!parser_p || !section || !key)
	{
		return INI_PARSER_ERR_INVALID_PARAM;
	}
	section_info_t * p_section = search_target_section(parser_p, false, section, strlen(section));
	if (!p_section)
	{
		return INI_PARSER_ERR_SECTION_KEY_NOT_FOUND;
	}
	key_value_t *p_kv = search_target_kv(p_section->plist_section, key);
	if (!p_kv)
	{
		return INI_PARSER_ERR_SECTION_KEY_NOT_FOUND;
	}
	return list_remove(p_section->plist_section, p_kv) ? INI_PARSER_ERR_SUCCEED : INI_PARSER_ERR_FAILED;
}

// delete target section, all key-value in this section will deleted.
INI_PARSER_CODE ini_parser_delete_section(ini_parser_ptr parser_p, const char* section)
{
	if (!parser_p || !section)
	{
		return INI_PARSER_ERR_INVALID_PARAM;
	}
	section_info_t* p_section = search_target_section(parser_p, false, section, strlen(section));
	if (!p_section)
	{
		return INI_PARSER_ERR_SECTION_KEY_NOT_FOUND;
	}
	list_free(p_section->plist_section);
	return list_remove(parser_p->plist_sections, p_section) ? INI_PARSER_ERR_SUCCEED : INI_PARSER_ERR_FAILED;
}

static bool iter_key_value_for_dump(void* data, void* context)
{
	if (!data || !context) // key is key, value is value.
	{
		return true; // just continue
	}
	key_value_t* p_kv = (key_value_t *)data;
	dump_ini_context* dup_ctx = (dump_ini_context*)context;
	if ((dup_ctx->ret_code = stringbuilder_appendf(dup_ctx->sb, "%s = %s\r\n", p_kv->key, p_kv->value)) != 0)
	{
#if(!defined(NDEBUG) || defined(_DEBUG))
		printf("ERROR[%s:%d]: failed on append %s=%s to stringbuilder. %d",
			__FILE__, __LINE__, p_kv->key, p_kv->value, dup_ctx->ret_code);
#endif // !NDEBUG || _DEBUG
		ASSERT_ABORT(dup_ctx->ret_code);
		return false;
	}
	return true;
}

static bool iter_section_for_dump(void* data, void* context)
{
	if (!data || !context) // data is section_info_t *
	{
		return true;
	}
	section_info_t* p_section = (section_info_t *)data;
	dump_ini_context* dup_ctx = (dump_ini_context*)context;
	if (dup_ctx->ret_code)
	{
		return false; // error occurred on foreach last section, break chain.
	}
	stringbuilder_appendf(dup_ctx->sb, "[%s]\r\n", (char*)p_section->section_name);
	list_foreach(p_section->plist_section, iter_key_value_for_dump, dup_ctx);
	stringbuilder_appendstr(dup_ctx->sb, "\r\n");
	return true;
}

char* ini_parser_dump(ini_parser_ptr parser_p)
{
	if (!parser_p)
	{
		return NULL;
	}
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
	char* ret_str = dump_context.ret_code ? NULL : strdup(stringbuilder_print(sb));
	stringbuilder_destroy(&sb);
	return ret_str;
}

INI_PARSER_CODE ini_parser_has_key(ini_parser_ptr parser_p, const char* section, const char* key)
{
	return ini_parser_get_string(parser_p, section, key, NULL, 0);
}

INI_PARSER_CODE ini_parser_get_int(ini_parser_ptr parser_p, const char* section, const char* key, int* value)
{
	return ini_parser_get_value(parser_p, section, key, value, INI_VALUE_TYPE_INT);
}

INI_PARSER_CODE ini_parser_get_long(ini_parser_ptr parser_p, const char* section, const char* key, long* value)
{
	return ini_parser_get_value(parser_p, section, key, value, INI_VALUE_TYPE_LONG);
}

INI_PARSER_CODE ini_parser_get_double(ini_parser_ptr parser_p, const char* section, const char* key, double* value)
{
	return ini_parser_get_value(parser_p, section, key, value, INI_VALUE_TYPE_DOUBLE);
}

INI_PARSER_CODE ini_parser_get_bool(ini_parser_ptr parser_p, const char* section, const char* key, bool* value)
{
	return ini_parser_get_value(parser_p, section, key, value, INI_VALUE_TYPE_BOOL);
}

static bool iter_for_delete_all_section(void* data, void* context)
{
	if (!data) // data is section_info_t *
	{
		return true;
	}
	section_info_t* p_sec_info = (section_info_t *)data;
	list_free(p_sec_info->plist_section);
	return true;
}

INI_PARSER_CODE ini_parser_destory(ini_parser_ptr* parser_pp)
{
	if (NULL == parser_pp || NULL == *parser_pp)
	{
		return INI_PARSER_ERR_INVALID_PARAM;
	}
	ini_parser_ptr parser_p = *parser_pp;
	if (parser_p->plist_sections)
	{
		//delete all section
		list_foreach(parser_p->plist_sections, iter_for_delete_all_section, NULL);
		list_free(parser_p->plist_sections);
		parser_p->plist_sections = NULL;
	}
	free(parser_p);
	*parser_pp = NULL;
	return INI_PARSER_ERR_SUCCEED;
}

static long parse_str2long(INI_PARSER_CODE* p_err, char* str_value)
{
	char* end_ptr = NULL;
	long result = strtol(str_value, &end_ptr, 10);
	*p_err = (!end_ptr || *end_ptr != '\0') ? INI_PARSER_ERR_FAILED : INI_PARSER_ERR_SUCCEED;
	return result;
}

static INI_PARSER_CODE ini_parser_get_value(ini_parser_ptr parser_p,
	const char* section, const char* key, void* value, INI_VALUE_TYPE value_type)
{
	char str_value[INI_VALUE_STACK_SIZE] = { 0 };
	INI_PARSER_CODE ret = ini_parser_get_string(parser_p, section, key, str_value, INI_VALUE_STACK_SIZE);
	if (ret != INI_PARSER_ERR_SUCCEED)
	{
		return ret;
	}
	if (str_value[0] == '\0')
	{
		return INI_PARSER_ERR_FAILED;
	}
	char* end_ptr = NULL;
	switch (value_type)
	{
	case INI_VALUE_TYPE_INT:
		*((int*)value) = (int)parse_str2long(&ret, str_value);
		break;
	case INI_VALUE_TYPE_LONG:
		*((long*)value) = parse_str2long(&ret, str_value);
		break;
	case INI_VALUE_TYPE_DOUBLE:
	{
		double result = strtod(str_value, &end_ptr);
		if (!end_ptr || *end_ptr != '\0')
		{
			ret = INI_PARSER_ERR_FAILED;
			break;
		}
		*((double*)value) = result;
	}
	break;
	case INI_VALUE_TYPE_BOOL:
	{
		bool result = strncasecmp(str_value, "true", 4) == 0;
		do
		{
			if (result)
			{
				break;
			}
			if (strncasecmp(str_value, "false", 5) == 0)
			{
				result = false;
				break;
			}
			long num = parse_str2long(&ret, str_value);
			if (ret == INI_PARSER_ERR_SUCCEED)
			{
				result = (bool)num;
			}
		} while (0);
		*((bool*)value) = result;
	}
	break;
	default:
		ret = INI_PARSER_ERR_FAILED;
		break;
	}
	return ret;
}
