#pragma once
#ifndef LCU_INI_PARSER_H
#define LCU_INI_PARSER_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

	typedef enum
	{
		INI_PARSER_ERR_SUCCEED = 0,
		INI_PARSER_ERR_INVALID_HANDLE,
		INI_PARSER_ERR_SECTION_KEY_NOT_FOUND,
		INI_PARSER_ERR_NOT_ENOUGH_MEMORY,
		INI_PARSER_ERR_SECTION_KEY_TOO_LONG,
		INI_PARSER_ERR_FAILED
	}INI_PARSER_ERROR_CODE;

	typedef struct _ini_parser ini_parser_t, * ini_parser_ptr;

	ini_parser_ptr ini_parser_parse_str(const char* ini_content);

	ini_parser_ptr ini_parser_parse_file(const char* ini_file);

	INI_PARSER_ERROR_CODE ini_parser_get_string(ini_parser_ptr parser_p, const char* section, const char* key, 
		char* value, const size_t value_size);

	INI_PARSER_ERROR_CODE ini_parser_has_key(ini_parser_ptr parser_p, const char* section, const char* key);

	INI_PARSER_ERROR_CODE ini_parser_get_int(ini_parser_ptr parser_p, const char* section, const char* key, int* value);

	INI_PARSER_ERROR_CODE ini_parser_get_long(ini_parser_ptr parser_p, const char* section, const char* key, long* value);

	INI_PARSER_ERROR_CODE ini_parser_get_double(ini_parser_ptr parser_p, const char* section, const char* key, double* value);

	INI_PARSER_ERROR_CODE ini_parser_destory(ini_parser_ptr* parser_pp);

#ifdef __cplusplus
};
#endif // __cplusplus

#endif // LCU_INI_PARSER_H
