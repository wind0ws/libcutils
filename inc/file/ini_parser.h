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
	} ini_parser_code_e;

	/**
	 * @brief callback for foreach ini section-key-value
	 * 
	 * @return 0 to continue iterating, non-zero to stop iterating.
	 */
	typedef int (*ini_parser_handler)(const char* section,
		const char* key, const char* value, void* user_data);

	// ini parser handle
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
	 * @brief foreach ini section-key-value
	 * 
	 * @param[in] parser_p ini_parser inst pointer
	 * @param[in] handler see ini_parser_handler 
	 * @param[in] user user param pointer
	 * 
	 * @return see ini_parser_code_e 
	 */
	ini_parser_code_e ini_parser_foreach(ini_parser_handle parser_p, ini_parser_handler handler, void* user_data);

	/**
	 * @brief add or update the section key value.
	 *
	 * @param[in] parser_p ini_parser inst pointer
	 * @param[in] section ini section
	 * @param[in] key ini key
	 * @param[in] value ini value of key
	 * 
	 * @return see ini_parser_code_e
	 */
	ini_parser_code_e ini_parser_put_string(ini_parser_handle parser_p,
		const char* section, const char* key, const char* value);

	/**
	 * @brief get value string of specified section-key from ini
	 * 
	 * @param[in] parser_p ini_parser inst pointer
	 * @param[in] section ini section
	 * @param[in] key ini key
	 * @param[in] value ini value pointer, will write data if get succeed.
	 * @param[in] value_size ini value pointer memory size. 
	 *            if not enough, error(INI_PARSER_CODE_NO_ENOUGH_MEMORY) will occurred.
	 * 
	 * @return see ini_parser_code_e
	 */
	ini_parser_code_e ini_parser_get_string(ini_parser_handle parser_p, const char* section,
		const char* key, char* value, const size_t value_size);

	ini_parser_code_e ini_parser_has_section_key(ini_parser_handle parser_p, const char* section, const char* key);

	ini_parser_code_e ini_parser_has_section(ini_parser_handle parser_p, const char* section);

	ini_parser_code_e ini_parser_get_bool(ini_parser_handle parser_p, const char* section, const char* key, bool* value);

	ini_parser_code_e ini_parser_get_double(ini_parser_handle parser_p, const char* section, const char* key, double* value);

	ini_parser_code_e ini_parser_get_float(ini_parser_handle parser_p, const char* section, const char* key, float* value);

	ini_parser_code_e ini_parser_get_int(ini_parser_handle parser_p, const char* section, const char* key, int* value);

	ini_parser_code_e ini_parser_get_long(ini_parser_handle parser_p, const char* section, const char* key, long* value);

	ini_parser_code_e ini_parser_get_long_long(ini_parser_handle parser_p, const char* section, const char* key, long long* value);

	/**
	 * @brief delete specified section-key.
	 *
	 * this operation won't take effect on your ini file, just on memory.
	 * you could call ini_parser_dump() and write to ini file if you want.
	 * 
	 * @return see ini_parser_code_e
	 */
	ini_parser_code_e ini_parser_delete_by_section_key(ini_parser_handle parser_p,
		const char* section, const char* key);

	/**
	 * @brief delete target section, all key-value in this section will deleted.
	 *
	 * this operation won't take effect on your ini file.
	 * you could call ini_parser_dump to get it string and save to file.
	 * 
	 * @return see ini_parser_code_e
	 */
	ini_parser_code_e ini_parser_delete_section(ini_parser_handle parser_p, const char* section);

	/**
     * @brief dump all ini config string to the provided mem.
     *
	 * @param[in] parser_p ini_parser inst pointer
	 * @param[in] mem the target memory, that will copy config string to it.
	 * @param[in] mem_size_p the pointer of target memory size. if dump succeed, the size will be override.
	 * 
	 * @return see ini_parser_code_e
     */
	ini_parser_code_e ini_parser_dump_to_mem(ini_parser_handle parser_p, char *mem, size_t *mem_size_p);

	/**
	 * @brief dump all ini config to string.
	 *
	 * The returned pointer should be FREED after use!
	 * 
	 * @return NULL means error occurred.
	 */
	char* ini_parser_dump(ini_parser_handle parser_p);

	/**
	 * @brief save all ini config to the file.
	 * the target file will be override.
	 * 
	 * @return see ini_parser_code_e
	 */
	ini_parser_code_e ini_parser_save(ini_parser_handle parser_p, const char* file_path);

	/**
	 * @brief destroy ini_parser
	 *
	 * @param[in,out] parser_pp the pointer of ini_parser_ptr
	 * 
	 * @return see ini_parser_code_e
	 */
	ini_parser_code_e ini_parser_destroy(ini_parser_handle* parser_pp);

#ifdef __cplusplus
};
#endif // __cplusplus

#endif // !LCU_INI_PARSER_H
