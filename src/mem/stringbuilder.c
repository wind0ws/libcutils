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
	char* buffer;          /* buffer */
	size_t allocated;      /* buffer size */
	size_t length;         /* current buffer string length */
};

stringbuilder_t* stringbuilder_create(size_t init_buf_size)
{
	stringbuilder_t* sb = (stringbuilder_t*)malloc(sizeof(struct stringbuilder));
	if (!sb)
	{
		return NULL;
	}
	if (init_buf_size < 2)
	{
		init_buf_size = STRING_BUILDER_DEFAULT_SIZE;
	}
	char* buffer = (char *)malloc(init_buf_size);
	if (!buffer)
	{
		free(sb);
		return NULL;
	}
	sb->allocated = init_buf_size;
	sb->buffer = buffer;
	sb->buffer[0] = '\0';
	sb->length = 0;
	return sb;
}

void stringbuilder_destroy(stringbuilder_t** sb_p)
{
	if (sb_p == NULL || *sb_p == NULL)
	{
		return;
	}
	stringbuilder_t* sb = *sb_p;
	free(sb->buffer);
	sb->buffer = NULL;
	free(sb);
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
	size_t should_alloc_buf_size = sb->allocated;
	while (should_alloc_buf_size < sb->length + string_len + 1)
	{
		should_alloc_buf_size <<= 1;
		if (should_alloc_buf_size == 0)
		{
			/* wow, what a huge string! */
			//--should_alloc_buf_size;
			return -3;
		}
	}

	void* new_buf = realloc(sb->buffer, should_alloc_buf_size);
	if (!new_buf)
	{
		return -4;
	}
	sb->allocated = should_alloc_buf_size;
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

const char* stringbuilder_to_string(const stringbuilder_t* sb)
{
	return sb->buffer;
}
