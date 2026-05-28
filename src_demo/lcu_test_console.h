#pragma once
#ifndef LCU_TEST_CONSOLE_H
#define LCU_TEST_CONSOLE_H

#include "common_macro.h"
#include "lcu_test_registry.h"
#include <stdbool.h>

EXTERN_C_START

typedef struct
{
    int passed;
    int failed;
    int skipped;
    double elapsed_sec;
} lcu_test_stats_t;

/* runner 回调：执行 cases[0..count-1]，结果累计到 stats */
typedef int (*lcu_test_runner_fn)(const lcu_test_entry_t **cases,
                                  int count,
                                  bool fail_fast,
                                  lcu_test_stats_t *stats);

void lcu_console_setup(void);
bool lcu_console_wait_interrupt(int timeout_sec);
void lcu_console_show_menu(void);

/* 从菜单读取用户输入，解析为用例数组，调用 runner 执行 */
int lcu_console_run_from_menu(lcu_test_runner_fn runner,
                              bool fail_fast,
                              lcu_test_stats_t *stats);

EXTERN_C_END

#endif /* LCU_TEST_CONSOLE_H */
