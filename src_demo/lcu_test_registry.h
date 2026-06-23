#pragma once
#ifndef LCU_TEST_REGISTRY_H
#define LCU_TEST_REGISTRY_H

#include "common_macro.h"
#include <stdbool.h>

EXTERN_C_START

typedef int (*lcu_test_fn)(void);

typedef struct lcu_test_entry
{
    const char *name;
    const char *description;
    lcu_test_fn fn;
    bool exclude_from_all;
    struct lcu_test_entry *next;
} lcu_test_entry_t;

void lcu_test_registry_link(lcu_test_entry_t *entry);
size_t lcu_test_registry_count(void);
const lcu_test_entry_t *lcu_test_registry_head(void);
const lcu_test_entry_t *lcu_test_registry_get(size_t idx);
const lcu_test_entry_t *lcu_test_registry_find_by_name(const char *name);

EXTERN_C_END

/* ======================== 跨平台构造函数宏 ======================== */

#ifdef _MSC_VER
  /* MSVC 注意:
   * - .CRT$XCU 段变量必须是 external linkage（否则 /include: 找不到）
   * - 名字 lcu__reg_<fn>_ptr 已唯一，不会冲突
   */
  #ifdef _M_IX86
    #define LCU_CONSTRUCTOR_DECL(fn)                                    \
        __pragma(section(".CRT$XCU", read))                             \
        static void fn(void);                                          \
        __declspec(allocate(".CRT$XCU"))                                \
        void (*fn##_ptr)(void) = fn;                                    \
        __pragma(comment(linker, "/include:_" #fn "_ptr"))
  #else
    #define LCU_CONSTRUCTOR_DECL(fn)                                    \
        __pragma(section(".CRT$XCU", read))                             \
        static void fn(void);                                          \
        __declspec(allocate(".CRT$XCU"))                                \
        void (*fn##_ptr)(void) = fn;                                    \
        __pragma(comment(linker, "/include:" #fn "_ptr"))
  #endif
  #define LCU_CONSTRUCTOR_BODY(fn) static void fn(void)
#else
  #define LCU_CONSTRUCTOR_DECL(fn)
  #define LCU_CONSTRUCTOR_BODY(fn) \
      __attribute__((constructor, used)) static void fn(void)
#endif

/* ======================== 注册宏 ======================== */

#define LCU_TEST_REGISTER(fn, desc)                                             \
    extern int fn(void);                                                        \
    static lcu_test_entry_t lcu__entry_##fn = {#fn, desc, fn, false, NULL};     \
    LCU_CONSTRUCTOR_DECL(lcu__reg_##fn)                                         \
    LCU_CONSTRUCTOR_BODY(lcu__reg_##fn) { lcu_test_registry_link(&lcu__entry_##fn); }

#define LCU_TEST_REGISTER_OPTIONAL(fn, desc)                                    \
    extern int fn(void);                                                        \
    static lcu_test_entry_t lcu__entry_##fn = {#fn, desc, fn, true, NULL};      \
    LCU_CONSTRUCTOR_DECL(lcu__reg_##fn)                                         \
    LCU_CONSTRUCTOR_BODY(lcu__reg_##fn) { lcu_test_registry_link(&lcu__entry_##fn); }

#endif /* LCU_TEST_REGISTRY_H */
