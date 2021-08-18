//#include "mem/mem_debug.h"
#include "file/ini_parser.h"
#include "file/ini_reader.h"
#include "data/hashmap.h"
#include "data/hash_functions.h"
#include "common_macro.h"
#include "mem/mplite.h"
#include "mem/strings.h" /* for strcmp */
#include "mem/stringbuilder.h"
#include <stdio.h>       /* for fopen */
#include <stdlib.h>      /* for atoi/atol/atof */

#define INI_VALUE_STACK_SIZE (128)
#define INI_MAX_SECTION (64)

typedef enum
{
	INI_VALUE_TYPE_INT,
	INI_VALUE_TYPE_LONG,
	INI_VALUE_TYPE_DOUBLE,
	INI_VALUE_TYPE_BOOL
} INI_VALUE_TYPE;

struct _ini_parser
{
	/* hold all section map, and each section map hold ini key value */
	hashmap_t* ini_root_map_p;
	/* prediction ini string size */
	size_t prediction_str_size;
	/* last used section map  */
	//section_map_t last_used_section_map;
	struct
	{
#define SECTION_NAME_MAX_SIZE INI_VALUE_STACK_SIZE
		char section_name[SECTION_NAME_MAX_SIZE];
		hashmap_t* map;
	} last_used_section_map;
};

static inline void* allocator_hashmap_malloc(size_t size)
{
	return malloc(size);
}

static inline void* allocator_hashmap_calloc(size_t item_count, size_t item_size)
{
	return calloc(item_count, item_size);
}

static inline void* allocator_hashmap_strdup(char *str)
{
	return str ? (strdup(str)) : NULL;
}

static inline void allocator_hashmap_free(void* ptr)
{
	if (ptr)
	{
		free(ptr);
	}
}

static bool map_key_equality(const void* x, const void* y)
{
	return (x == y) || (x && y && strcmp(x, y) == 0);
}

static INI_PARSER_CODE ini_parser_get_value(ini_parser_ptr parser_p,
	const char* section, const char* key, void* value, INI_VALUE_TYPE value_type);

//ini reader callback, return true for continue，return false for abort
static int ini_handler_cb(void* user,
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
	hashmap_t* root_map = hashmap_create(16, hash_function_string, allocator_hashmap_free, 
			NULL /*value is hashmap, already free it on sub hashmap_free*/, map_key_equality, NULL);
	if (!root_map)
	{
		return NULL;
	}
	ini_parser_ptr parser_p = (ini_parser_ptr)calloc(1, sizeof(struct _ini_parser));
	if (!parser_p)
	{
		hashmap_free(root_map);
		return NULL;
	}
	parser_p->ini_root_map_p = root_map;
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

static hashmap_t* search_target_section_map(ini_parser_ptr parser_p, bool auto_create,
	const char* section, const size_t section_len)
{
	if (parser_p->last_used_section_map.map &&
		strcasecmp(section, parser_p->last_used_section_map.section_name) == 0)
	{
		return parser_p->last_used_section_map.map; //hit cache, no need search on root map.
	}

	hashmap_t* section_map = (hashmap_t *)hashmap_get(parser_p->ini_root_map_p, (char *)section);
	if (auto_create && section_map == NULL)
	{
		section_map = hashmap_create(32, hash_function_string, allocator_hashmap_free, 
			allocator_hashmap_free, map_key_equality, NULL);
		if (section_map == NULL)
		{
			return NULL;
		}
		char* alloc_section_map_key = allocator_hashmap_strdup((char *)section);
		hashmap_put(parser_p->ini_root_map_p, alloc_section_map_key, section_map);
		parser_p->prediction_str_size += (section_len + 6); /* 2 for square brackets, and 4 for 2 \r\n */
		strlcpy(parser_p->last_used_section_map.section_name, section, SECTION_NAME_MAX_SIZE);
		parser_p->last_used_section_map.map = section_map;
	}
	return section_map;
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
	hashmap_t* section_map = search_target_section_map(parser_p, true, section, section_len);
	if (!section_map)
	{
		return INI_PARSER_ERR_NOT_ENOUGH_MEMORY;
	}
	char* map_key = (char*)allocator_hashmap_strdup((char *)key);
	if (!map_key)
	{
		return INI_PARSER_ERR_NOT_ENOUGH_MEMORY;
	}
	do
	{
		char* map_value = NULL;
		size_t map_value_size;
		if (value)
		{
			map_value_size = strlen(value) + 1;
			map_value = (char*)allocator_hashmap_malloc(map_value_size);
			if (map_value)
			{
				memcpy(map_value, value, map_value_size);
			}
		}
		else
		{
			map_value_size = 1;
			map_value = (char*)allocator_hashmap_strdup("");
		}
		if (!map_value)
		{
			break; // alloc failed
		}
		hashmap_put(section_map, map_key, map_value);
		parser_p->prediction_str_size += (map_value_size + key_len + 6);
		return INI_PARSER_ERR_SUCCEED;
	} while (0);
	allocator_hashmap_free(map_key);
	return INI_PARSER_ERR_NOT_ENOUGH_MEMORY;
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
		return INI_PARSER_ERR_SECTION_KEY_NOT_FOUND;
	}

	hashmap_t *section_map = search_target_section_map(parser_p, false, section, section_len);
	if (!section_map)
	{
		return INI_PARSER_ERR_SECTION_KEY_NOT_FOUND;
	}
	char* map_value = (char*)hashmap_get(section_map, (void *)key);
	if (!map_value)
	{
		return INI_PARSER_ERR_SECTION_KEY_NOT_FOUND;
	}
	if (value && value_size)
	{
		size_t map_value_len = strlen(map_value);
		if (value_size <= map_value_len)
		{
			return INI_PARSER_ERR_NOT_ENOUGH_MEMORY;
		}
		memcpy(value, map_value, map_value_len + 1);
	}
	return INI_PARSER_ERR_SUCCEED;
}

typedef struct 
{
	int ret_code;
	stringbuilder_t* sb;
} dump_ini_context;

static bool foreach_section_key_value_iter_cb(void* key, void* value, void* context)
{
	if (!key || !value) // key is key, value is value.
	{
		return true; // just continue
	}
	dump_ini_context* dup_ctx = (dump_ini_context*)context;
	if ((dup_ctx->ret_code = stringbuilder_appendf(dup_ctx->sb, "%s = %s\r\n", (char*)key, (char*)value)) != 0)
	{
#if(!defined(NDEBUG) || defined(_DEBUG))
		printf("ERROR[%s:%d]: failed on append %s=%s to stringbuilder. %d",
			__FILE__, __LINE__, (char*)key, (char*)value, dup_ctx->ret_code);
#endif // !NDEBUG || _DEBUG
		ASSERT_ABORT(dup_ctx->ret_code);
		return false;
	}
	return true;
}

static bool foreach_section_iter_cb(void* key, void* value, void* context)
{
	if (!key || !value) // key is section name, value is section map
	{
		return true;
	}
	dump_ini_context* dup_ctx = (dump_ini_context*)context;
	if (dup_ctx->ret_code)
	{
		return false; // error occurred on foreach last section, break chain.
	}
	stringbuilder_appendf(dup_ctx->sb, "[%s]\r\n", (char *)key);
	hashmap_t* section_map = (hashmap_t *)value;
	hashmap_foreach(section_map, foreach_section_key_value_iter_cb, dup_ctx);
	stringbuilder_appendstr(dup_ctx->sb, "\r\n");
	return true;
}

char* ini_parser_dump(ini_parser_ptr parser_p)
{
	if (!parser_p)
	{
		return NULL;
	}
	// why manual alloc memory for stringbuilder: 
	//   to make sure it won't realloc frequently.
	const size_t sb_mem_size = parser_p->prediction_str_size + 64;
	char* sb_memory = (char *)malloc(sb_mem_size);
	if (!sb_memory)
	{
		return NULL;
	}
	stringbuilder_t *sb = stringbuilder_create_with_mem(sb_memory, sb_mem_size);
	if (!sb)
	{
		free(sb_memory);
		return NULL;
	}
	dump_ini_context dump_context =
	{
		.ret_code = 0,
		.sb = sb,
	};
	hashmap_foreach(parser_p->ini_root_map_p, foreach_section_iter_cb, &dump_context);
	char* ret_str = dump_context.ret_code ? NULL : strdup(stringbuilder_print(sb));
	stringbuilder_destroy(&sb);
	free(sb_memory);
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

static bool delete_section_map_iter_cb(void *key, void *value, void *context)
{
	if (!key || !value) // key is section, value is section map.
	{
		return true;
	}
	hashmap_t* section_map = (hashmap_t *)value;
	hashmap_free(section_map);
	return true;
}

INI_PARSER_CODE ini_parser_destory(ini_parser_ptr* parser_pp)
{
	if (NULL == parser_pp || NULL == *parser_pp)
	{
		return INI_PARSER_ERR_INVALID_PARAM;
	}
	ini_parser_ptr parser_p = *parser_pp;
	if (parser_p->ini_root_map_p)
	{
		//delete all section map
		hashmap_foreach(parser_p->ini_root_map_p, delete_section_map_iter_cb, NULL);
		hashmap_free(parser_p->ini_root_map_p);
		parser_p->ini_root_map_p = NULL;
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
