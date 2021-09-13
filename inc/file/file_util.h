#pragma once
#ifndef __LCU_FILE_UTIL_H
#define __LCU_FILE_UTIL_H

#include "common_macro.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

	/**
	 * append slash("/" or "\\") if "folder_path" NOT end with slash("/" or "\\")
	 */
	int file_util_append_slash_on_path_if_needed(__inout char* folder_path, __in const size_t folder_path_size);

	/**
	 * check folder_path is exists.
	 * @return 0 means access by mode is ok, otherwise it is error code.
	 */
	int file_util_access(__in const char* path, __in const int access_mode);

	/**
	 * mkdirs on "folder_path", if it not exists.
	 * "folder_path" should end with slash("/" or "\\").
	 * @return 0 means create succeed, otherwise fail.
	 */
	int file_util_mkdirs(__in const char* folder_path);

	/**
	 * get total file size
	 * @return file size
	 */
	long file_util_get_size_by_path(__in const char* file_path);

	/**
	 * get file size from FILE stream.
	 * after this function call, stream position will set to 0.
	 * @return file size
	 */
	long file_util_get_size_by_fs(__in FILE* fs);

	/**
	 * read on file_handle(file descriptor).
	 * if you read a real file(FILE), use fread instead.
	 * @return read size
	 */
	int file_util_read(__in int file_handle, __out void* buffer, __in size_t max_char_count);

	/**
	 * write on file_handle(file descriptor).
	 * if you write a real file(FILE), use fwrite instead.
	 * @return write size
	 */
	int file_util_write(__in int file_handle, __in void* buffer, __in size_t max_char_count);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __LCU_FILE_UTIL_H
