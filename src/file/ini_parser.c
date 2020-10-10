#include "file/ini_parser.h"
#include "file/ini_reader.h"
#include "data/hash_map.h"
#include "data/hash_functions.h"
#include "common_macro.h"
#include <malloc.h>
#include "mem/strings.h" /* for strcmp */
#include <stdio.h>       /* for fopen */
#include <stdlib.h>      /* for atoi/atol/atof */

#define INI_VALUE_STACK_SIZE (128)

typedef enum
{
	INI_VALUE_TYPE_INT,
	INI_VALUE_TYPE_LONG,
	INI_VALUE_TYPE_DOUBLE,
	INI_VALUE_TYPE_BOOL
}INI_VALUE_TYPE;

struct _ini_parser
{
	hash_map_t* ini_map_p;
};

static INI_PARSER_ERROR_CODE ini_parser_get_value(ini_parser_ptr parser_p,
	const char* section, const char* key, void* value, INI_VALUE_TYPE value_type);

static void map_key_value_free_func(void* p)
{
	if (p)
	{
		free(p);
	}
}

static bool map_key_equality(const void* x, const void* y)
{
	return (x == y) || (x && y && strcmp(x, y) == 0);
}

static void generate_map_key(char* out_map_key, const char* section, const size_t section_len,
	const char* key, const size_t key_len)
{
	memcpy(out_map_key, section, section_len);
	memcpy(out_map_key + section_len, key, key_len + 1);
}

static int ini_handler_cb(void* user, int lineno,
	const char* section, const char* key, const char* value)
{
	if (NULL == section || NULL == key)
	{
		return 0;// just continue to parse file
	}
	ini_parser_ptr parser_p = (ini_parser_ptr)user;
	hash_map_t* ini_map_p = parser_p->ini_map_p;
	//section + key as map key
	size_t section_len = strlen(section);
	size_t key_len = strlen(key);
	char* map_key = (char*)malloc(section_len + key_len + 1);
	if (!map_key)
	{
		return 1;
	}
	generate_map_key(map_key, section, section_len, key, key_len);
	char* map_value = NULL;
	if (value)
	{
		size_t value_size = strlen(value) + 1;
		map_value = (char*)malloc(value_size);
		if (!map_value)
		{
			return 2;
		}
		memcpy(map_value, value, value_size);
	}
	else
	{
		map_value = (char*)calloc(1, 1);
	}
	hash_map_set(ini_map_p, map_key, map_value);
	return 0;
}

ini_parser_ptr ini_parser_parse_str(const char* ini_content)
{
	if (!ini_content || strlen(ini_content) < 6)
	{
		return NULL;
	}
	hash_map_t* ini_map_p = hash_map_new(16, hash_function_string,
		map_key_value_free_func, map_key_value_free_func, map_key_equality);
	if (!ini_map_p)
	{
		return NULL;
	}
	ini_parser_ptr parser_p = (ini_parser_ptr)calloc(sizeof(struct _ini_parser), 1);
	if (!parser_p)
	{
		hash_map_free(ini_map_p);
		return NULL;
	}
	parser_p->ini_map_p = ini_map_p;
	if (ini_parse_string(ini_content, ini_handler_cb, parser_p))
	{
		ini_parser_destory(&parser_p);
		return NULL;
	}
	return parser_p;
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
		if (file_size < 2)
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

INI_PARSER_ERROR_CODE ini_parser_get_string(ini_parser_ptr parser_p,
	const char* section, const char* key, char* value, const size_t value_size)
{
	if (!parser_p || !section || !key)
	{
		return INI_PARSER_ERR_INVALID_HANDLE;
	}
	const size_t section_len = strlen(section);
	const size_t key_len = strlen(key);
	if (!section_len || !key_len)
	{
		return INI_PARSER_ERR_SECTION_KEY_NOT_FOUND;
	}
#define MAX_MAP_KEY_LEN (1024)
	if (section_len + key_len >= MAX_MAP_KEY_LEN)
	{
		return INI_PARSER_ERR_SECTION_KEY_TOO_LONG;
	}
	char map_key[MAX_MAP_KEY_LEN];
	generate_map_key(map_key, section, section_len, key, key_len);
	char* map_value = (char*)hash_map_get(parser_p->ini_map_p, map_key);
	if (!map_value)
	{
		return INI_PARSER_ERR_SECTION_KEY_NOT_FOUND;
	}
	if (value && value_size > 1)
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

INI_PARSER_ERROR_CODE ini_parser_has_key(ini_parser_ptr parser_p, const char* section, const char* key)
{
	return ini_parser_get_string(parser_p, section, key, NULL, 0);
}

INI_PARSER_ERROR_CODE ini_parser_get_int(ini_parser_ptr parser_p, const char* section, const char* key, int* value)
{
	return ini_parser_get_value(parser_p, section, key, value, INI_VALUE_TYPE_INT);
}

INI_PARSER_ERROR_CODE ini_parser_get_long(ini_parser_ptr parser_p, const char* section, const char* key, long* value)
{
	return ini_parser_get_value(parser_p, section, key, value, INI_VALUE_TYPE_LONG);
}

INI_PARSER_ERROR_CODE ini_parser_get_double(ini_parser_ptr parser_p, const char* section, const char* key, double* value)
{
	return ini_parser_get_value(parser_p, section, key, value, INI_VALUE_TYPE_DOUBLE);
}

INI_PARSER_ERROR_CODE ini_parser_get_bool(ini_parser_ptr parser_p, const char* section, const char* key, bool* value)
{
	return ini_parser_get_value(parser_p, section, key, value, INI_VALUE_TYPE_BOOL);
}

INI_PARSER_ERROR_CODE ini_parser_destory(ini_parser_ptr* parser_pp)
{
	if (NULL == parser_pp || NULL == *parser_pp)
	{
		return INI_PARSER_ERR_INVALID_HANDLE;
	}
	ini_parser_ptr parser_p = *parser_pp;
	if (parser_p->ini_map_p)
	{
		hash_map_free(parser_p->ini_map_p);
		parser_p->ini_map_p = NULL;
	}
	free(parser_p);
	*parser_pp = NULL;
	return INI_PARSER_ERR_SUCCEED;
}

static long parse_str2long(INI_PARSER_ERROR_CODE* p_err, char* str_value)
{
	char* end_ptr = NULL;
	long result = strtol(str_value, &end_ptr, 10);
	*p_err = strlen(end_ptr) ? INI_PARSER_ERR_FAILED : INI_PARSER_ERR_SUCCEED;
	return result;
}

static INI_PARSER_ERROR_CODE ini_parser_get_value(ini_parser_ptr parser_p,
	const char* section, const char* key, void* value, INI_VALUE_TYPE value_type)
{
	char str_value[INI_VALUE_STACK_SIZE] = { 0 };
	INI_PARSER_ERROR_CODE ret = ini_parser_get_string(parser_p, section, key, str_value, INI_VALUE_STACK_SIZE);
	if (ret != INI_PARSER_ERR_SUCCEED)
	{
		return ret;
	}
	if (!strlen(str_value))
	{
		return INI_PARSER_ERR_FAILED;
	}
	char* end_ptr = NULL;
	switch (value_type)
	{
	case INI_VALUE_TYPE_INT:
		*((int*)value) = parse_str2long(&ret, str_value);
		break;
	case INI_VALUE_TYPE_LONG:
		*((long*)value) = parse_str2long(&ret, str_value);
		break;
	case INI_VALUE_TYPE_DOUBLE:
	{
		double result = strtod(str_value, &end_ptr);
		if (strlen(end_ptr))
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
