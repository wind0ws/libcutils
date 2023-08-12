#pragma once
#ifndef LCU_INI_PARSER_H
#define LCU_INI_PARSER_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

	typedef enum
	{
		INI_PARSER_CODE_SUCCEED = 0,
		/* general error */
		INI_PARSER_CODE_FAILED,
		/* invalid param : maybe null handle */
		INI_PARSER_CODE_INVALID_PARAM,
		/* alloc memory failed */
		INI_PARSER_CODE_NOT_ENOUGH_MEMORY,
		/* section / key not found */
		INI_PARSER_CODE_NOT_FOUND_SECTION_KEY
	} INI_PARSER_CODE;

	typedef struct _ini_parser ini_parser_t, * ini_parser_ptr;

	/**
	 * create empty ini config. equals ini_parser_parse_str(NULL)
	 *
	 * @return ini_parser inst pointer
	 */
	ini_parser_ptr ini_parser_create();

	/**
	 * parse ini string config.
	 *
	 * @param ini_content: string pointer, can be NULL
	 * @return ini_parser inst pointer
	 */
	ini_parser_ptr ini_parser_parse_str(const char* ini_content);

	/**
	 * parse ini file config.
	 *
	 * @param ini_file: ini file path
	 * @return ini_parser inst pointer.
			   returned NULL if file not exists or parse failed.
	 */
	ini_parser_ptr ini_parser_parse_file(const char* ini_file);

	/**
	 * add or update the section key value.
	 *
	 * @param parser_p: ini_parser inst pointer
	 * @param section: ini section
	 * @param key: ini key
	 * @param value: ini value of key
	 * @return see INI_PARSER_CODE
	 */
	INI_PARSER_CODE ini_parser_put_string(ini_parser_ptr parser_p,
		const char* section, const char* key, const char* value);

	/**
	 * get value string of specified section-key from ini
	 */
	INI_PARSER_CODE ini_parser_get_string(ini_parser_ptr parser_p, const char* section,
		const char* key, char* value, const size_t value_size);

	/**
	 * delete specified section-key.
	 *
	 * this operation won't take effect on your ini file, just on memory.
	 * you could call ini_parser_dump and write to ini file if you want.
	 */
	INI_PARSER_CODE ini_parser_delete_by_section_key(ini_parser_ptr parser_p,
		const char* section, const char* key);

	/**
	 * delete target section, all key-value in this section will deleted.
	 *
	 * this operation won't take effect on your ini file.
	 * you could call ini_parser_dump to get it string and save to file.
	 */
	INI_PARSER_CODE ini_parser_delete_section(ini_parser_ptr parser_p, const char* section);

	/**
	 * dump all ini config to string.
	 *
	 * the return string pointer should FREE after use by yourself!!!
	 * return NULL means error occurred.
	 */
	char* ini_parser_dump(ini_parser_ptr parser_p);

	/**
	 * save all ini config to file. 
	 * the target file will be override.
	 */
	INI_PARSER_CODE ini_parser_save(ini_parser_ptr parser_p, const char* file_path);

	INI_PARSER_CODE ini_parser_have_section_key(ini_parser_ptr parser_p, const char* section, const char* key);

	INI_PARSER_CODE ini_parser_have_section(ini_parser_ptr parser_p, const char* section);

	INI_PARSER_CODE ini_parser_get_bool(ini_parser_ptr parser_p, const char* section, const char* key, bool* value);

	INI_PARSER_CODE ini_parser_get_double(ini_parser_ptr parser_p, const char* section, const char* key, double* value);

	INI_PARSER_CODE ini_parser_get_float(ini_parser_ptr parser_p, const char* section, const char* key, float* value);

	INI_PARSER_CODE ini_parser_get_int(ini_parser_ptr parser_p, const char* section, const char* key, int* value);

	INI_PARSER_CODE ini_parser_get_long(ini_parser_ptr parser_p, const char* section, const char* key, long* value);

	INI_PARSER_CODE ini_parser_get_long_long(ini_parser_ptr parser_p, const char* section, const char* key, long long* value);

	/**
	 * destroy ini_parser
	 *
	 * @param parser_pp: the pointer of ini_parser_ptr
	 * @return see INI_PARSER_CODE
	 */
	INI_PARSER_CODE ini_parser_destroy(ini_parser_ptr* parser_pp);

#ifdef __cplusplus
};
#endif // __cplusplus

#endif // !LCU_INI_PARSER_H
