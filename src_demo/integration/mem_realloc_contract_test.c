/**
 * Regression test: lcu_realloc must be symmetric with lcu_free for
 * cross-boundary / untracked pointers (Fix #1 + L-1 data-preservation).
 *
 * Background:
 *   Public ownership-transfer APIs (file_util_read_all / strreplace /
 *   ini_parser_dump / asprintf / str_params_to_str) return RAW libc buffers
 *   that are NOT registered in the allocation tracker. A client that uses
 *   mem_debug.h has its `realloc` rewritten to lcu_realloc_trace. Before the
 *   fix, lcu_realloc_trace unconditionally queried allocation_tracker_ptr_size,
 *   which ASSERT_ABORT'd on an untracked pointer when the tracker was ACTIVE
 *   -> hard process abort (release too). With the tracker OFF, it memcpy'd 0
 *   bytes (L-1) -> silent data loss.
 *
 *   lcu_free already tolerates untracked pointers (falls back to libc free).
 *   lcu_realloc MUST be symmetric: untracked / tracker-inactive -> libc realloc.
 *
 * This file DELIBERATELY includes mem_debug.h first line, exactly like a real
 * client. The pass criterion holds in BOTH builds:
 *   - tracker OFF (default build): realloc maps to libc/CRT realloc OR (memcheck
 *     build) to lcu_realloc_trace with allocations==NULL -> libc fallback.
 *   - tracker ON (-D_LCU_MEM_CHECK_FEATURE_ENABLE=1): realloc maps to
 *     lcu_realloc_trace; untracked raw pointer must NOT abort and must preserve
 *     content. THIS is the exact condition that used to crash.
 */

#include "mem/mem_debug.h"
#include "common_macro.h"
#include "mem/strings.h"
#include "mem/allocator.h"
#include "file/file_util.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define LOG_TAG "REALLOC_CONTRACT"
#include "log/logger.h"

/* ------------------------------------------------------------------ */
/* 1. realloc-grow a RAW buffer returned by file_util_read_all.       */
/*    Must not abort; original content must be preserved.             */
/* ------------------------------------------------------------------ */
static int test_realloc_grow_raw_file_buffer(void)
{
    LOGI("[realloc] 1 grow raw file_util_read_all buffer");

    const char *path = "./realloc_contract_tmp.txt";
    const char *content = "ownership realloc contract payload 0123456789";
    const size_t clen = strlen(content);

    FILE *fp = fopen(path, "wb");
    ASSERT(fp != NULL);
    fwrite(content, 1, clen, fp);
    fclose(fp);

    char *data = NULL;
    int len = 0;
    int ret = file_util_read_all(path, &data, &len);
    ASSERT(ret == 0);
    ASSERT(data != NULL);
    ASSERT((size_t)len == clen);
    ASSERT(strcmp(data, content) == 0);

    /* Grow the RAW buffer. Pre-fix + tracker ON: ASSERT_ABORT in
     * allocation_tracker_ptr_size. Pre-fix + tracker OFF: memcpy(...,0) data loss. */
    size_t newcap = clen + 256;
    char *grown = (char *)realloc(data, newcap);
    ASSERT(grown != NULL);
    /* Original content MUST survive the realloc. */
    ASSERT(strcmp(grown, content) == 0);

    /* Use the freshly grown tail to confirm the buffer is really `newcap`. */
    memset(grown + clen, 'Z', newcap - clen - 1);
    grown[newcap - 1] = '\0';

    free(grown);
    remove(path);

    LOGI("[realloc] 1 PASS");
    return 0;
}

/* ------------------------------------------------------------------ */
/* 2. realloc-grow a RAW buffer returned by strreplace.               */
/* ------------------------------------------------------------------ */
static int test_realloc_grow_raw_strreplace(void)
{
    LOGI("[realloc] 2 grow raw strreplace buffer");

    char *s = strreplace("C:\\a\\b\\c", "\\", "/");
    ASSERT(s != NULL);
    ASSERT(strcmp(s, "C:/a/b/c") == 0);

    size_t want = strlen(s) + 1;
    char *grown = (char *)realloc(s, want + 128);
    ASSERT(grown != NULL);
    ASSERT(strcmp(grown, "C:/a/b/c") == 0);

    free(grown);
    LOGI("[realloc] 2 PASS");
    return 0;
}

/* ------------------------------------------------------------------ */
/* 3. realloc(NULL, n) then grow chain must preserve data (L-1).      */
/*    Exercises the tracker-active grow path and the fallback path.   */
/* ------------------------------------------------------------------ */
static int test_realloc_growth_chain_preserves_data(void)
{
    LOGI("[realloc] 3 growth chain preserves data");

    char *p = (char *)realloc(NULL, 8);
    ASSERT(p != NULL);
    memcpy(p, "abcdefg", 8); /* incl '\0' */

    for (size_t cap = 16; cap <= 4096; cap *= 2)
    {
        char *np = (char *)realloc(p, cap);
        ASSERT(np != NULL);
        /* The 8-byte prefix written before each grow must persist. */
        ASSERT(strcmp(np, "abcdefg") == 0);
        p = np;
    }

    /* realloc(ptr, 0) frees and returns NULL. */
    char *z = (char *)realloc(p, 0);
    ASSERT(z == NULL);

    LOGI("[realloc] 3 PASS");
    return 0;
}

/* ------------------------------------------------------------------ */
/* 4. realloc-shrink: in-bounds use of exactly `size` bytes must NOT  */
/*    raise a false canary-corruption (Fix #4: canary sits at `size`, */
/*    writing [0,size) stays in bounds).                              */
/* ------------------------------------------------------------------ */
static int test_realloc_exact_size_no_false_positive(void)
{
    LOGI("[realloc] 4 exact-size in-bounds write, no false corruption");

    /* malloc via the public tracked path so (in memcheck build) it is tracked
     * and canary-protected, then realloc-shrink and write exactly `n` bytes. */
    size_t n = 200;
    char *p = (char *)malloc(512);
    ASSERT(p != NULL);
    memset(p, 'A', 512 - 1);
    p[512 - 1] = '\0';

    char *sp = (char *)realloc(p, n);
    ASSERT(sp != NULL);
    /* Write EXACTLY n bytes: [0, n). Tail canary must be at >= n so this is
     * in-bounds and must not be flagged at free time. */
    memset(sp, 'B', n);

    free(sp); /* corruption checker runs here in memcheck build; must stay silent */

    LOGI("[realloc] 4 PASS");
    return 0;
}

int mem_realloc_contract_test(void)
{
    LOGI("=== mem_realloc_contract_test BEGIN ===");

    int rc = 0;
    if (0 == rc) rc = test_realloc_grow_raw_file_buffer();
    if (0 == rc) rc = test_realloc_grow_raw_strreplace();
    if (0 == rc) rc = test_realloc_growth_chain_preserves_data();
    if (0 == rc) rc = test_realloc_exact_size_no_false_positive();

    LOGI("=== mem_realloc_contract_test END: %s ===", (0 == rc) ? "ALL PASS" : "FAIL");
    return rc;
}

#include "lcu_test_registry.h"
LCU_TEST_REGISTER(mem_realloc_contract_test, "realloc symmetric untracked-fallback + canary placement");
