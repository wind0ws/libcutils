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
		INI_PARSER_CODE_NO_ENOUGH_MEMORY,
		/* section / key not found */
		INI_PARSER_CODE_NOT_FOUND_SECTION_KEY
	} INI_PARSER_CODE;

	typedef struct _ini_parser *ini_parser_handle;

	/**
	 * @brief check if the provided string is a file path
	 *
	 * @return true for file_path, otherwise not
	 */
	bool ini_parser_is_file_path(const char* str);

	/**
	 * @brief create empty ini config. 
	 * equals ini_parser_parse_str(NULL)
	 *
	 * @return ini_parser inst pointer
	 */
	ini_parser_handle ini_parser_create();

	/**
	 * @brief parse ini string config.
	 *
	 * @param[in] ini_content: string pointer, can be NULL
	 * 
	 * @return ini_parser inst pointer
	 */
	ini_parser_handle ini_parser_parse_str(const char* ini_content);

	/**
	 * @brief parse ini file config.
	 *
	 * @param[in] ini_file ini file path
	 * 
	 * @return ini_parser inst pointer.
			   returned NULL if file not exists or parse failed.
	 */
	ini_parser_handle ini_parser_parse_file(const char* ini_file);

	/**
	 * @brief add or update the section key value.
	 *
	 * @param[in] parser_p ini_parser inst pointer
	 * @param[in] section ini section
	 * @param[in] key ini key
	 * @param[in] value ini value of key
	 * 
	 * @return see INI_PARSER_CODE
	 */
	INI_PARSER_CODE ini_parser_put_string(ini_parser_handle parser_p,
		const char* section, const char* key, const char* value);

	/**
	 * @brief get value string of specified section-key from ini
	 * 
	 * @return see INI_PARSER_CODE
	 */
	INI_PARSER_CODE ini_parser_get_string(ini_parser_handle parser_p, const char* section,
		const char* key, char* value, const size_t value_size);

	INI_PARSER_CODE ini_parser_has_section_key(ini_parser_handle parser_p, const char* section, const char* key);

	INI_PARSER_CODE ini_parser_has_section(ini_parser_handle parser_p, const char* section);

	INI_PARSER_CODE ini_parser_get_bool(ini_parser_handle parser_p, const char* section, const char* key, bool* value);

	INI_PARSER_CODE ini_parser_get_double(ini_parser_handle parser_p, const char* section, const char* key, double* value);

	INI_PARSER_CODE ini_parser_get_float(ini_parser_handle parser_p, const char* section, const char* key, float* value);

	INI_PARSER_CODE ini_parser_get_int(ini_parser_handle parser_p, const char* section, const char* key, int* value);

	INI_PARSER_CODE ini_parser_get_long(ini_parser_handle parser_p, const char* section, const char* key, long* value);

	INI_PARSER_CODE ini_parser_get_long_long(ini_parser_handle parser_p, const char* section, const char* key, long long* value);

	/**
	 * @brief delete specified section-key.
	 *
	 * this operation won't take effect on your ini file, just on memory.
	 * you could call ini_parser_dump() and write to ini file if you want.
	 * 
	 * @return see INI_PARSER_CODE
	 */
	INI_PARSER_CODE ini_parser_delete_by_section_key(ini_parser_handle parser_p,
		const char* section, const char* key);

	/**
	 * @brief delete target section, all key-value in this section will deleted.
	 *
	 * this operation won't take effect on your ini file.
	 * you could call ini_parser_dump to get it string and save to file.
	 * 
	 * @return see INI_PARSER_CODE
	 */
	INI_PARSER_CODE ini_parser_delete_section(ini_parser_handle parser_p, const char* section);

	/**
	 * @brief dump all ini config to string.
	 *
	 * The returned pointer should be freed after use!
	 * 
	 * @return NULL means error occurred.
	 */
	char* ini_parser_dump(ini_parser_handle parser_p);

	/**
	 * @brief save all ini config to file.
	 * the target file will be override.
	 * 
	 * @return see INI_PARSER_CODE
	 */
	INI_PARSER_CODE ini_parser_save(ini_parser_handle parser_p, const char* file_path);

	/**
	 * @brief destroy ini_parser
	 *
	 * @param[in,out] parser_pp the pointer of ini_parser_ptr
	 * 
	 * @return see INI_PARSER_CODE
	 */
	INI_PARSER_CODE ini_parser_destroy(ini_parser_handle* parser_pp);

#ifdef __cplusplus
};
#endif // __cplusplus

#endif // !LCU_INI_PARSER_H
