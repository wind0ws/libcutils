#include "mem/mem_debug.h"
#include "mem/str_params.h"

#define LOG_TAG "str_params"
//#define LOG_NDEBUG 0
//#define _GNU_SOURCE 1
#include <errno.h>
#include <stdint.h>
#include "mem/asprintf.h"
#include "mem/strings.h"
#include "data/hashmap.h"
#include "log/xlog.h"

/* When an object is allocated but not freed in a function,
 * because its ownership is released to other object like a hashmap,
 * call RELEASE_OWNERSHIP to tell the clang analyzer and avoid
 * false warnings about potential memory leak.
 * For now, a "temporary" assignment to global variables
 * is enough to confuse the clang static analyzer.
 */
#ifdef __clang_analyzer__
static void* released_pointer;
#define RELEASE_OWNERSHIP(x) { released_pointer = x; released_pointer = 0; }
#else
#define RELEASE_OWNERSHIP(x)
#endif // __clang_analyzer__

struct str_params
{
	hashmap_t* map;
};

static bool str_eq(const void* key_a, const void* key_b)
{
	return !strcmp((const char*)key_a, (const char*)key_b);
}

/* use djb hash unless we find it inadequate */
#ifdef __clang__
__attribute__((no_sanitize("integer")))
#endif // __clang__
static int str_hash_fn(const void* str)
{
	uint32_t hash = 5381;
	for (char* p = (char*)(str); p && *p; p++)
		hash = ((hash << 5) + hash) + *p;
	return (int)hash;
}

str_params_ptr str_params_create(void)
{
	str_params_ptr s = (str_params_ptr)(calloc(1, sizeof(struct str_params)));
	if (!s) return NULL;
	s->map = hashmap_create(16, str_hash_fn, free, free, str_eq, NULL);
	if (!s->map)
	{
		free(s);
		return NULL;
	}
	return s;
}

void str_params_del(str_params_ptr params, const char* key)
{
	hashmap_remove(params->map, (void*)key);
}

void str_params_destroy(str_params_ptr params)
{
	if (!params || !params->map)
	{
		return;
	}
	hashmap_free(params->map);
	params->map = NULL;
	free(params);
}

str_params_ptr str_params_create_str(const char* param_str)
{
	struct str_params* parms;
	char* str = NULL;
	char* kvpair = NULL;
	char* tmpstr = NULL;
	int items = 0;
	parms = str_params_create();
	if (!parms)
		goto err_create_str_parms;
	str = strdup(param_str);
	if (!str)
		goto err_strdup;
	TLOGV(LOG_TAG, "%s: source string == '%s'", __func__, param_str);
	kvpair = strtok_r(str, ";", &tmpstr);
	while (kvpair && *kvpair)
	{
		char* eq = strchr(kvpair, '='); /* would love strchrnul */
		char* value;
		char* key;
		void* old_val;
		if (eq == kvpair)
			goto next_pair;
		if (eq)
		{
			key = strndup(kvpair, eq - kvpair);
			if (*(++eq))
				value = strdup(eq);
			else
				value = strdup("");
		}
		else
		{
			key = strdup(kvpair);
			value = strdup("");
		}
		/* we replaced a value */
		old_val = hashmap_put(parms->map, key, value);
		RELEASE_OWNERSHIP(value);
		RELEASE_OWNERSHIP(old_val);
		RELEASE_OWNERSHIP(key);

		items++;
	next_pair:
		kvpair = strtok_r(NULL, ";", &tmpstr);
	}
	if (!items)
		TLOGV(LOG_TAG, "%s: no items found in string", __func__);
	free(str);
	return parms;
err_strdup:
	str_params_destroy(parms);
err_create_str_parms:
	return NULL;
}

int str_params_add_str(str_params_ptr params, const char* key, const char* value)
{
	void* tmp_key = NULL;
	void* tmp_val = NULL;
	void* old_val = NULL;
	// strdup and hashmapPut both set errno on failure.
	// Set errno to 0 so we can recognize whether anything went wrong.
	int saved_errno = errno;
	errno = 0;
	tmp_key = strdup(key);
	if (tmp_key == NULL)
	{
		goto clean_up;
	}
	tmp_val = strdup(value);
	if (tmp_val == NULL)
	{
		goto clean_up;
	}
	old_val = hashmap_put(params->map, tmp_key, tmp_val);
	if (old_val == NULL)
	{
		// Did hashmapPut fail?
		if (errno == ENOMEM)
		{
			goto clean_up;
		}
		// For new keys, hashmap takes ownership of tmp_key and tmp_val.
		RELEASE_OWNERSHIP(tmp_key);
		RELEASE_OWNERSHIP(tmp_val);
		tmp_key = tmp_val = NULL;
	}
	else
	{
		// For existing keys, hashmap takes ownership of tmp_val.
		// (It also gives up ownership of old_val.)
		RELEASE_OWNERSHIP(tmp_val);
		RELEASE_OWNERSHIP(old_val);
		old_val = tmp_val = NULL;
	}
clean_up:
	if (tmp_key)
	{
		free(tmp_key);
	}
	if (tmp_val)
	{
		free(tmp_val);
	}
	int result = -errno;
	errno = saved_errno;
	return result;
}

int str_params_add_long(str_params_ptr params, const char* key, long value)
{
	char val_str[24];
	int ret;
	ret = snprintf(val_str, sizeof(val_str), "%ld", value);
	if (ret < 0)
		return -EINVAL;
	ret = str_params_add_str(params, key, val_str);
	return ret;
}

int str_params_add_int(str_params_ptr params, const char* key, int value)
{
	return str_params_add_long(params, key, (long)value);
}

int str_params_add_double(str_params_ptr params, const char* key, double value)
{
	char val_str[32];
	int ret;
	ret = snprintf(val_str, sizeof(val_str), "%.10f", value);
	if (ret < 0)
		return -EINVAL;
	ret = str_params_add_str(params, key, val_str);
	return ret;
}

int str_params_add_float(str_params_ptr params, const char* key, float value)
{
	return str_params_add_double(params, key, (double)value);
}

bool str_params_has_key(str_params_ptr params, const char* key)
{
	return hashmap_get(params->map, (void*)key) != NULL;
}

int str_params_get_str(str_params_ptr params, const char* key, char* out_val, size_t out_val_size)
{
	char* value = (char*)(hashmap_get(params->map, (void*)key));
	if (value)
	{
		if (strlen(value) + 1 > out_val_size)
		{
			return -ENOMEM;
		}
		strlcpy(out_val, value, out_val_size);
		return 0;
	}
	return -ENOENT;
}

int str_params_get_long(str_params_ptr params, const char* key, long* out_val)
{
	char* end;
	char* value = (char*)(hashmap_get(params->map, (void*)key));
	if (!value)
		return -ENOENT;
	long val = strtol(value, &end, 0);
	if (*value == '\0' || *end != '\0')
		return -EINVAL;
	*out_val = val;
	return 0;
}

int str_params_get_int(str_params_ptr params, const char* key, int* out_val)
{
	long long_num;
	int ret = str_params_get_long(params, key, &long_num);
	if (ret == 0)
	{
		*out_val = long_num;
	}
	return ret;
}

int str_params_get_double(str_params_ptr params, const char* key, double* out_val)
{
	double out;
	char* end;
	char* value = (char*)(hashmap_get(params->map, (void*)(key)));
	if (!value)
		return -ENOENT;
	out = strtod(value, &end);
	if (*value == '\0' || *end != '\0')
		return -EINVAL;
	*out_val = out;
	return 0;
}

int str_params_get_float(str_params_ptr params, const char* key, float* out_val)
{
	double double_num;
	int ret = str_params_get_double(params, key, &double_num);
	if (ret == 0)
	{
		*out_val = (float)double_num;
	}
	return ret;
}

static bool combine_strings(void* key, void* value, void* context)
{
	char** old_str = (char**)(context);
	char* new_str;
	int ret = asprintf(&new_str, "%s%s%s=%s",
		*old_str ? *old_str : "",
		*old_str ? ";" : "",
		(char*)key,
		(char*)value);
	if (*old_str) 
	{
		free(*old_str);
	}
	if (ret >= 0)
	{
		*old_str = new_str;
		return true;
	}
	*old_str = NULL;
	return false;
}

char* str_params_to_str(str_params_ptr params)
{
	char* str = NULL;
	hashmap_foreach(params->map, combine_strings, &str);
	return (str != NULL) ? str : strdup("");
}

static bool dump_entry(void* key, void* value, void* context)
{
	TLOGI(LOG_TAG, "key: '%s' value: '%s'", (char*)key, (char*)value);
	return true;
}

void str_params_dump(str_params_ptr params)
{
	hashmap_foreach(params->map, dump_entry, params);
}
