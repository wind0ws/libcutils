/**
 * Regression test for cross-boundary ownership-transfer memory contract.
 *
 * Background: strreplace / file_util_read_all / asprintf / str_params_to_str all
 * transfer heap ownership to the caller. They were (incorrectly) allocating via the
 * tracked allocator, so when the allocation tracker was ACTIVE the returned pointer
 * was canary-offset/tracked and an external caller's libc free() corrupted the heap
 * (and the entry was falsely reported as a leak). Fix: these APIs now use raw libc
 * malloc; the contract is "release with libc free()".
 *
 * IMPORTANT: This file intentionally does NOT include mem_debug.h, so malloc/free
 * here are the real libc symbols — faithfully simulating an EXTERNAL SDK integrator
 * who only has libc free().
 *
 * Tracker state is controlled by the BUILD, not toggled at runtime (toggling mid-run
 * is invalid: allocations made while the tracker was a given state must be freed in
 * the same state). Build the whole program with -D_LCU_MEM_CHECK_FEATURE_ENABLE=1 to
 * exercise the tracker-ON path — the exact condition that used to crash here. In a
 * normal build the tracker is OFF and this verifies basic correctness. Either way the
 * pass criterion is the same: allocate via these APIs, release with libc free(), and
 * neither crash nor corrupt the heap.
 */

#include "common_macro.h"
#include "mem/strings.h"
#include "file/file_util.h"
#include "mem/asprintf.h"
#include "mem/str_params.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define LOG_TAG "OWNERSHIP_TEST"
#include "log/logger.h"

/* ------------------------------------------------------------------ */
/* 1. strreplace: allocate + libc free                    */
/* ------------------------------------------------------------------ */
static int test_strreplace_libc_free(void)
{
    LOGI("[own] 1 strreplace + libc free");

    for (int i = 0; i < 100; i++)
    {
        char *r = strreplace("C:\\a\\b\\c\\d", "\\", "/");
        ASSERT(r != NULL);
        ASSERT(strcmp(r, "C:/a/b/c/d") == 0);
        free(r); /* libc free — must be safe even with tracker ON */
    }

    LOGI("[own] 1 PASS");
    return 0;
}

/* ------------------------------------------------------------------ */
/* 2. asprintf: allocate + libc free                      */
/* ------------------------------------------------------------------ */
static int test_asprintf_libc_free(void)
{
    LOGI("[own] 2 asprintf + libc free");

    for (int i = 0; i < 100; i++)
    {
        char *s = NULL;
        int n = asprintf(&s, "item-%d-%s", i, "payload");
        ASSERT(n > 0);
        ASSERT(s != NULL);
        ASSERT((int)strlen(s) == n);
        free(s); /* libc free — must be safe even with tracker ON */
    }

    LOGI("[own] 2 PASS");
    return 0;
}

/* ------------------------------------------------------------------ */
/* 3. file_util_read_all: allocate + libc free            */
/* ------------------------------------------------------------------ */
static int test_file_util_read_all_libc_free(void)
{
    LOGI("[own] 3 file_util_read_all + libc free");

    const char *path = "./ownership_test_tmp.txt";
    const char *content = "hello ownership contract\nsecond line\n";

    /* Write a temp file using libc directly */
    FILE *fp = fopen(path, "wb");
    ASSERT(fp != NULL);
    fwrite(content, 1, strlen(content), fp);
    fclose(fp);

    for (int i = 0; i < 50; i++)
    {
        char *data = NULL;
        int len = 0;
        int ret = file_util_read_all(path, &data, &len);
        ASSERT(ret == 0);
        ASSERT(data != NULL);
        ASSERT(len == (int)strlen(content));
        ASSERT(strcmp(data, content) == 0);
        free(data); /* libc free — must be safe even with tracker ON */
    }

    remove(path);
    LOGI("[own] 3 PASS");
    return 0;
}

/* ------------------------------------------------------------------ */
/* 4. str_params_to_str: both non-empty and empty paths + libc free    */
/* ------------------------------------------------------------------ */
static int test_str_params_to_str_libc_free(void)
{
    LOGI("[own] 4 str_params_to_str + libc free");

    /* Non-empty path: result comes from asprintf chain (raw libc) */
    str_params_ptr p = str_params_create(";");
    ASSERT(p != NULL);
    str_params_add_str(p, "k1", "v1");
    str_params_add_int(p, "k2", 42);

    char *s = str_params_to_str(p);
    ASSERT(s != NULL);
    ASSERT(strlen(s) > 0);
    free(s); /* libc free */
    str_params_destroy(p);

    /* Empty path: result comes from the lcu_malloc_raw("") branch */
    str_params_ptr empty = str_params_create(";");
    ASSERT(empty != NULL);
    char *e = str_params_to_str(empty);
    ASSERT(e != NULL);
    ASSERT(e[0] == '\0');
    free(e); /* libc free — empty branch must also be raw */
    str_params_destroy(empty);

    LOGI("[own] 4 PASS");
    return 0;
}

int ownership_contract_test(void)
{
    LOGI("=== ownership_contract_test BEGIN ===");

    int rc = 0;
    if (0 == rc) rc = test_strreplace_libc_free();
    if (0 == rc) rc = test_asprintf_libc_free();
    if (0 == rc) rc = test_file_util_read_all_libc_free();
    if (0 == rc) rc = test_str_params_to_str_libc_free();

    LOGI("=== ownership_contract_test END: %s ===", (0 == rc) ? "ALL PASS" : "FAIL");
    return rc;
}

#include "lcu_test_registry.h"
LCU_TEST_REGISTER(ownership_contract_test, "cross-boundary ownership free contract");
