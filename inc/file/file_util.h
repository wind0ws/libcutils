#pragma once
#ifndef LCU_FILE_UTIL_H
#define LCU_FILE_UTIL_H

#include "common_macro.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

	/**
	 * append slash("/" or "\\") if "folder_path" NOT end with slash("/" or "\\")
	 * Note: "folder_path" must be readable and writable.
	 * 
	 * @return 0 for success, otherwise fail
	 */
	int file_util_append_slash_on_path_if_needed(__inout char* folder_path, __in const size_t folder_path_size);

	/**
	 * check folder_path is exists.
	 * 
	 * @param path: folder path
	 * @param access_mode: F_OK(0)
	 * @return 0 means access by mode is ok, otherwise it is error code.
	 */
	int file_util_access(__in const char* path, __in const int access_mode);

	/**
	 * mkdirs on "folder_path", if it not exists.
	 * "folder_path" should end with slash("/" or "\\").
     *
     * @param folder_path: the folder path
	 * @return 0 means create succeed, otherwise fail. if "folder_path" already exists, return 0.
	 */
	int file_util_mkdirs(__in const char* folder_path);

	/**
	 * get total file size
     *
     * @param file_path: the file path
	 * @return file size
	 */
	long file_util_get_size_by_path(__in const char* file_path);

	/**
	 * get file size from FILE stream.
	 * after this function call, stream position will set to 0.
	 * 
     * @param fs: the pointer of FILE
	 * @return file size
	 */
	long file_util_get_size_by_fs(__in FILE* fs);

	/**
	 * read on file_handle(file descriptor).
	 * Note: if you read a real file(FILE), use "fread" instead.
     *       this function for low level read, such as SOCKET.
	 * 
     * @param file_handle: handle of file descriptor.
     * @param buffer: the buffer to hold on what we read out.
     * @param max_char_count: byte size that we want to read out, buffer size should bigger than it.
	 * @return read size
	 */
	int file_util_read(__in int file_handle, __out void* buffer, __in size_t max_char_count);

	/**
	 * write on file_handle(file descriptor).
	 * Note: if you write a real file(FILE), use "fwrite" instead.
     *       this function for low level write, such as SOCKET.
	 * 
     * @param file_handle: handle of file descriptor.
     * @param buffer: the buffer that we will read and write to file descriptor.
     * @param max_char_count: byte size that we read from buffer.
	 * @return real write size
	 */
	int file_util_write(__in int file_handle, __in void* buffer, __in size_t max_char_count);

	/**
	 * read text from file line by line.
	 * 
     * @param file_path: the file path.
     * @param handle_txt_line_fn: function that we will callback out each text line.
     * @param user_data: the user pointer that we will pass to callback function.
	 * @return 0 for success, otherwise fail
	 */
	int file_util_read_txt(__in const char* file_path,
		__in int (*handle_txt_line_fn)(int line_num, char* txt, void* user_data),
		__in void* user_data);

	/**
	 * read out all file data.
	 * Note: after you used, you must freed memory(out_alloced_file_data) which read out from file.
     *
	 * @param file_path: the file path
	 * @param out_alloced_file_data: the pointer's pointer of store read out data
	 * @param out_file_byte_len: the pointer of the file byte len
	 * @return 0 for success, otherwise fail
	 */
	int file_util_read_all(__in const char* file_path, __out char **out_alloced_file_data, __out int *out_file_byte_len);

#ifdef __cplusplus
};
#endif // __cplusplus

#endif // !LCU_FILE_UTIL_H
