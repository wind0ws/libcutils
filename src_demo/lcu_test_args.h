#pragma once
#ifndef LCU_TEST_ARGS_H
#define LCU_TEST_ARGS_H

#include "common_macro.h"

EXTERN_C_START

#define LCU_TEST_MAX_NAMES 64

typedef enum
{
    LCU_TEST_MODE_DEFAULT = 0,
    LCU_TEST_MODE_ALL,
    LCU_TEST_MODE_LIST,
    LCU_TEST_MODE_HELP,
    LCU_TEST_MODE_BY_NAME,
    LCU_TEST_MODE_FILTER,
} lcu_test_mode_t;

typedef struct
{
    lcu_test_mode_t mode;
    bool fail_fast;
    const char *junit_path;
    const char *filter;
    const char *names[LCU_TEST_MAX_NAMES];
    int name_count;
} lcu_test_run_options_t;

int lcu_test_parse_args(int argc, char *argv[], lcu_test_run_options_t *options);
void lcu_test_print_help(const char *prog_name);

EXTERN_C_END

#endif /* LCU_TEST_ARGS_H */
