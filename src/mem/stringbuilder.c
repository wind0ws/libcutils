#include "mem/stringbuilder.h"
#include "common_macro.h"
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define STRING_BUILDER_DEFAULT_SIZE (128)

struct stringbuilder
{
	bool need_free_myself; /* is buffer alloc by our api */
	char* buffer;          /* buffer */
	size_t allocated;      /* buffer size */
	size_t length;         /* current buffer string length */
};

stringbuilder_t* stringbuilder_create_with_mem(char* buf, size_t buf_size)
{
	if (!buf || buf_size < sizeof(stringbuilder_t) + 4)
	{
		return NULL;
	}
	stringbuilder_t* sb = (stringbuilder_t*)buf;
	sb->need_free_myself = false;
	sb->allocated = buf_size - sizeof(stringbuilder_t);
	sb->buffer = buf + sizeof(stringbuilder_t);
	sb->buffer[0] = '\0';
	sb->length = 0;
	return sb;
}

stringbuilder_t* stringbuilder_create(size_t init_buf_size)
{
	if (init_buf_size < 8)
	{
		init_buf_size = STRING_BUILDER_DEFAULT_SIZE;
	}
	const size_t need_buf_size = init_buf_size + sizeof(stringbuilder_t);
	char* raw_buf = malloc(need_buf_size);
	if (!raw_buf)
	{
		return NULL;
	}
	stringbuilder_t* sb = stringbuilder_create_with_mem(raw_buf, need_buf_size);
	if (!sb)
	{
		free(raw_buf);
		return NULL;
	}
	sb->need_free_myself = true;
	return sb;
}

void stringbuilder_destroy(stringbuilder_t** sb_p)
{
	if (sb_p == NULL || *sb_p == NULL)
	{
		return;
	}
	stringbuilder_t* sb = *sb_p;
	sb->buffer = NULL;
	if (sb->need_free_myself)
	{
		sb->need_free_myself = false;
		free(sb);
	}
	*sb_p = NULL;
}

static int sb_ensure_space(stringbuilder_t* sb, size_t string_len)
{
	if (sb == NULL || sb->buffer == NULL || string_len == 0)
	{
		return -1;
	}

	if (sb->allocated >= sb->length + string_len + 1)
	{
		return 0; // buf size is already enough.
	}

	while (sb->allocated < sb->length + string_len + 1)
	{
		sb->allocated <<= 1;
		if (sb->allocated == 0)
		{
			/* wow, what a huge string! */
			//sb->allocated--;
			return -3;
		}
	}

	void* new_buf = realloc(sb->buffer, sb->allocated);
	if (!new_buf)
	{
		return -4;
	}
	sb->buffer = new_buf;
	return 0;
}

int stringbuilder_appendchar(stringbuilder_t* sb, char c)
{
	if (!sb || !sb->buffer)
	{
		return -1;
	}
	if (sb_ensure_space(sb, sizeof(char)) != 0)
	{
		return -1;
	}
	sb->buffer[sb->length] = c;
	++sb->length;
	sb->buffer[sb->length] = '\0';
	return 0;
}

int stringbuilder_appendstr(stringbuilder_t* sb, const char* str)
{
	return stringbuilder_appendnstr(sb, str, 0);
}

int stringbuilder_appendnstr(stringbuilder_t* sb, const char* str, size_t len)
{
	if (!sb || !sb->buffer || !str)
	{
		return -1;
	}
	if (len == 0)
	{
		len = strlen(str);
	}
	if (len == 0)
	{
		return -2;
	}

	if (sb_ensure_space(sb, len) != 0)
	{
		return -3;
	}
	memcpy(sb->buffer + sb->length, str, len);
	sb->length += len;
	sb->buffer[sb->length] = '\0';
	return 0;
}

int stringbuilder_appendf(stringbuilder_t* sb, const char* format, ...)
{
	if (!sb || !sb->buffer || !format)
	{
		return -1;
	}
	va_list va;

	va_start(va, format);
	//first we get the string length that will write. ('\0' is not belong of the length)
	size_t len = vsnprintf(NULL, 0, format, va);
	va_end(va);
	if (len < 1)
	{
		return -2;
	}

	if (sb_ensure_space(sb, len) != 0)
	{
		return -3;
	}

	va_start(va, format);
	vsnprintf(sb->buffer + sb->length, len + 1, format, va);
	va_end(va);

	sb->length += len;
	sb->buffer[sb->length] = '\0';
	return 0;
}

int stringbuilder_clear(stringbuilder_t* sb)
{
	if (!sb || !sb->buffer)
	{
		return -1;
	}
	sb->length = 0;
	sb->buffer[0] = '\0';
	return 0;
}

size_t stringbuilder_len(const stringbuilder_t* sb)
{
	return sb->length;
}

const char* stringbuilder_print(const stringbuilder_t* sb)
{
	return sb->buffer;
}
