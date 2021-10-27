#pragma once
#ifndef LCU_STRINGBUILDER_H
#define LCU_STRINGBUILDER_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

	typedef struct stringbuilder stringbuilder_t;

	/**
	 * create stringbuilder object.
	 * if init_buf_size is non-zero, use it as init buffer size.
	 * if init_buf_size is zero, default init buf size will used.
	 */
	stringbuilder_t* stringbuilder_create(size_t init_buf_size);

	/**
	 * free stringbuilder memory.
	 */
	void stringbuilder_destroy(stringbuilder_t** sb_p);

	/**
	 * append one char at end of stringbuilder
	 */
	int stringbuilder_appendchar(stringbuilder_t* sb, char c);
	/**
	 * append full string on stringbuilder
	 */
	int stringbuilder_appendstr(stringbuilder_t* sb, const char* str);
	/**
	 * append max len string on stringbuilder.
	 * if len bigger than string len, we append string real len.
	 * if len smaller than string len, we append 'len'
	 */
	int stringbuilder_appendnstr(stringbuilder_t* sb, const char* str, size_t len);
	/**
	 * append string format on stringbuilder
	 */
	int stringbuilder_appendf(stringbuilder_t* sb, const char* format, ...);

	/**
	 * clear stringbuilder.
	 */
	int stringbuilder_clear(stringbuilder_t* sb);

	/**
	 * current stringbuilder string length
	 */
	size_t stringbuilder_len(const stringbuilder_t* sb);
	/**
	 * print current stringbuilder. copy it if you bring returned string to other place.
	 * this function has no effect on internal data structure, 
	 * therefore you can continuous append string on stringbuilder.
	 * WARN: DO NOT FREE OR MODIFY DATA on the return pointer. this pointer point to internal buffer.
	 */
	const char* stringbuilder_print(const stringbuilder_t* sb);

#ifdef __cplusplus
};
#endif // __cplusplus

#endif // !LCU_STRINGBUILDER_H
