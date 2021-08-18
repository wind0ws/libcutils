#pragma once
#ifndef LCU_STR_PARAMS_H
#define LCU_STR_PARAMS_H

#include <stdbool.h> /* for true/false */
#include <stddef.h>  /* for size_t     */

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct str_params *str_params_ptr;

/**
 * create str_params.
 * you should call str_params_destroy after use!
 * @param delimiter: can be NULL, default delimiter is semicolon(";")
 */
str_params_ptr str_params_create(const char *delimiter);
/**
 * create str_params with exists param_str.
 * see more details in str_params_create function.
 * @param delimiter: can be NULL, default delimiter is semicolon(";")
 * @param param_str: the exists param str to parse
 */
str_params_ptr str_params_create_str(const char *delimiter, const char *param_str);
/**
 * destroy str_params and free memory.
 */
void str_params_destroy(str_params_ptr params);

/**
 * delete param key value pair by key.
 */
void str_params_del(str_params_ptr params, const char *key);

int str_params_add_str(str_params_ptr params, const char *key,const char *value);
int str_params_add_long(str_params_ptr params, const char* key, long value);
int str_params_add_int(str_params_ptr params, const char *key, int value);
int str_params_add_double(str_params_ptr params, const char* key, double value);
int str_params_add_float(str_params_ptr params, const char *key, float value);

/**
 * Returns true if the str_parms contains the specified key.
 */
bool str_params_has_key(str_params_ptr params, const char *key);
/**
 *  Gets value associated with the specified key (if present), placing it in the buffer
 *  pointed to by the out_val parameter.  Returns the length of the returned string value.
 *  If 'key' isn't in the params, then return -ENOENT (-2) and leave 'out_val' untouched.
 */
int str_params_get_str(str_params_ptr params, const char *key, char *out_val, size_t out_val_size);
int str_params_get_long(str_params_ptr params, const char* key, long* out_val);
int str_params_get_int(str_params_ptr params, const char *key, int *out_val);
int str_params_get_double(str_params_ptr params, const char* key, double* out_val);
int str_params_get_float(str_params_ptr params, const char *key, float *out_val);

/**
 * combine key value pair params to string.
 * the return string is malloc on heap, so you should free after use!
 * or memory leak will occur!
 */
char *str_params_to_str(str_params_ptr params);

/* for debug: log current str_params key value. */
void str_params_dump(str_params_ptr params);

#ifdef __cplusplus
};
#endif // __cplusplus

#endif /* LCU_STR_PARAMS_H */
