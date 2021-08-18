#pragma once
#ifndef LCU_INI_PARSER_H
#define LCU_INI_PARSER_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

	typedef enum
	{
		INI_PARSER_ERR_SUCCEED = 0,
		/* invalid param : maybe null handle */
		INI_PARSER_ERR_INVALID_PARAM,
		/* section + key not found */
		INI_PARSER_ERR_SECTION_KEY_NOT_FOUND,
		/* alloc memory failed */
		INI_PARSER_ERR_NOT_ENOUGH_MEMORY,
		/* section + key too long */
		INI_PARSER_ERR_SECTION_KEY_TOO_LONG,
		/* general error */
		INI_PARSER_ERR_FAILED
	}INI_PARSER_CODE;

	typedef struct _ini_parser ini_parser_t, * ini_parser_ptr;

	//create empty ini config
	ini_parser_ptr ini_parser_create();

	ini_parser_ptr ini_parser_parse_str(const char* ini_content);

	ini_parser_ptr ini_parser_parse_file(const char* ini_file);

	// add or update the section key value.
	INI_PARSER_CODE ini_parser_put_string(ini_parser_ptr parser_p, 
		const char* section, const char* key, const char* value);

	INI_PARSER_CODE ini_parser_get_string(ini_parser_ptr parser_p, const char* section, 
		const char* key, char* value, const size_t value_size);

	//dump all ini config to string. the return string pointer should FREE after use by yourself!!!
	//return NULL means error occurred.
	char *ini_parser_dump(ini_parser_ptr parser_p);

	INI_PARSER_CODE ini_parser_has_key(ini_parser_ptr parser_p, const char* section, const char* key);

	INI_PARSER_CODE ini_parser_get_int(ini_parser_ptr parser_p, const char* section, const char* key, int* value);

	INI_PARSER_CODE ini_parser_get_long(ini_parser_ptr parser_p, const char* section, const char* key, long* value);

	INI_PARSER_CODE ini_parser_get_double(ini_parser_ptr parser_p, const char* section, const char* key, double* value);

	INI_PARSER_CODE ini_parser_get_bool(ini_parser_ptr parser_p, const char* section, const char* key, bool* value);

	INI_PARSER_CODE ini_parser_destory(ini_parser_ptr* parser_pp);

#ifdef __cplusplus
};
#endif // __cplusplus

#endif // LCU_INI_PARSER_H
