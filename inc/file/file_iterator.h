#pragma once
#ifndef LCU_FILE_ITERATOR_H
#define LCU_FILE_ITERATOR_H

#include <sys/stat.h>

#define FILE_ITERATOR_TYPE_FILE  0
#define FILE_ITERATOR_TYPE_DIR   1

typedef struct
{
	char* dir;
	char* name;
	// see file type above
	int type;

	struct stat *p_stat;
} file_info_t;

typedef int (*file_iterator_handle_file_info_fn)(file_info_t* p_info, void* user_data);

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

	/**
	 * foreach dir's sub-folder and file.
	 * 
	 * @param dir: the folder path
	 * @param handler: callback function to process sub-dir or file info
	 * @param user_data: callback function user data pointer
	 * @return error occurred if return non zero
	 */
	int file_iterator_foreach(char* dir, file_iterator_handle_file_info_fn handler, void* user_data);

#ifdef __cplusplus
};
#endif // __cplusplus

#endif // !LCU_FILE_ITERATOR_H
