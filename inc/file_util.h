#ifndef __LCU_FILE_UTIL_H
#define __LCU_FILE_UTIL_H

#include "common_macro.h"

EXTERN_C_START

void file_util_append_slash_on_path_if_needed(char* folder_path, const size_t folder_path_capacity);

/**
 * check folder_path is exists.
 * @return 0 means access by mode is ok, otherwise it is error code.
 */
int file_util_access(const char* path, int access_mode);

int file_util_mkdirs(const char* folder_path);

EXTERN_C_END

#endif // __LCU_FILE_UTIL_H
