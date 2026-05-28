#pragma once
#ifndef LCU_TEST_GLOB_H
#define LCU_TEST_GLOB_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool lcu_glob_match(const char *pattern, const char *str);

#ifdef __cplusplus
}
#endif

#endif /* LCU_TEST_GLOB_H */
