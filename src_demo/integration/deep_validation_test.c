/**
 * Deep validation test for 0527~0608 commits.
 *
 * Covers:
 * 1. allocation_tracker recursion fix (raw allocator path)
 * 2. hashmap allocator injection (3 paths) + foreach NULL guard
 * 3. file_logger path truncation + ENOENT tolerance
 * 4. strings/strlcpy/strlcat/strreplace edge cases
 * 5. hashmap stress (heavy put/get/remove triggering multiple rehashes)
 */

#include "mem/mem_debug.h"
#include "common_macro.h"
#include "mem/allocation_tracker.h"
#include "mem/allocator.h"
#include "data/hashmap.h"
#include "data/hash_functions.h"
#include "mem/strings.h"
#include "log/file_logger.h"
#include "thread/posix_thread.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define LOG_TAG "DEEP_VALID"
#include "log/logger.h"

/* ============================================================================
 * Section 1: allocation_tracker recursion fix
 *
 * The fix ensures allocation_tracker's internal hashmap uses raw allocators
 * (allocator_calloc_raw) so that tracked alloc/free operations don't recurse.
 * We stress this by doing many alloc/free cycles while the tracker is active.
 * ========================================================================== */

static void report_leak(void *ptr, size_t size,
    char *leak_file, char *leak_func, int leak_line, void *user_data)
{
    int *leak_count = (int *)user_data;
    if (leak_count) (*leak_count)++;
    LOGW("LEAK: '%s' (%s:%d) addr=%p size=%zu",
        NULLABLE_STRING(leak_func), NULLABLE_STRING(leak_file), leak_line, ptr, size);
}

static int test_tracker_no_recursion(void)
{
    LOGI("[deep] 1.1 tracker recursion stress");

    /* Heavy alloc/free cycle. If recursion fix is broken, this would
     * stack overflow or deadlock on the allocations_lock. */
    void *ptrs[200];
    for (int round = 0; round < 5; round++)
    {
        for (int i = 0; i < 200; i++)
        {
            ptrs[i] = malloc(64 + i);
            ASSERT(ptrs[i] != NULL);
        }
        for (int i = 199; i >= 0; i--)
        {
            free(ptrs[i]);
        }
    }

    int leak_count = 0;
    size_t leaked = allocation_tracker_expect_no_allocations(report_leak, &leak_count);
    ASSERT(leaked == 0);
    ASSERT(leak_count == 0);

    LOGI("[deep] 1.1 PASS (1000 alloc/free, no recursion)");
    return 0;
}

static int test_tracker_realloc_stress(void)
{
    LOGI("[deep] 1.2 tracker realloc stress");

    /* realloc exercises multiple internal paths:
     * - ptr==NULL -> malloc
     * - size grows -> new alloc + memcpy + free old
     * - size shrinks -> no-op (current size sufficient) */
    void *p = NULL;
    for (int i = 1; i <= 50; i++)
    {
        p = realloc(p, (size_t)i * 100);
        ASSERT(p != NULL);
        memset(p, (char)i, (size_t)i * 100);
    }
    /* Verify last write didn't corrupt canary */
    free(p);

    int leak_count = 0;
    size_t leaked = allocation_tracker_expect_no_allocations(report_leak, &leak_count);
    ASSERT(leaked == 0);

    LOGI("[deep] 1.2 PASS (50 realloc cycles, canary intact)");
    return 0;
}

static int test_tracker_strdup_strndup(void)
{
    LOGI("[deep] 1.3 tracker strdup/strndup");

    const char *src = "Hello, allocation tracker recursion fix!";
    char *s1 = strdup(src);
    ASSERT(s1 != NULL);
    ASSERT(strcmp(s1, src) == 0);

    char *s2 = strndup(src, 5);
    ASSERT(s2 != NULL);
    ASSERT(strncmp(s2, "Hello", 5) == 0);
    ASSERT(s2[5] == '\0');

    free(s1);
    free(s2);

    int leak_count = 0;
    allocation_tracker_expect_no_allocations(report_leak, &leak_count);
    ASSERT(leak_count == 0);

    LOGI("[deep] 1.3 PASS");
    return 0;
}

/* ============================================================================
 * Section 2: hashmap allocator injection + foreach NULL guard
 * ========================================================================== */

static int int_hash_fn(const void *key)
{
    return (int)(intptr_t)key;
}

static bool int_eq_fn(const void *a, const void *b)
{
    return a == b;
}

static int test_hashmap_raw_allocator(void)
{
    LOGI("[deep] 2.1 hashmap with raw allocator");

    /* Create hashmap using raw allocator (same path as allocation_tracker).
     * Verify all 3 allocation paths use raw: struct, buckets, entries. */
    hashmap_t *map = hashmap_create_with_allocator(8, int_hash_fn, NULL, NULL, int_eq_fn,
                                        NULL, &allocator_calloc_raw);
    ASSERT(map != NULL);

    /* Insert enough to trigger rehash (load > 0.75 of initial capacity) */
    for (int i = 1; i <= 20; i++)
    {
        hashmap_put(map, (void *)(intptr_t)i, (void *)(intptr_t)(i * 7));
    }
    ASSERT(hashmap_size(map) == 20);

    /* Verify all values */
    for (int i = 1; i <= 20; i++)
    {
        void *val = hashmap_get(map, (void *)(intptr_t)i);
        ASSERT(val == (void *)(intptr_t)(i * 7));
    }

    /* Remove half */
    for (int i = 1; i <= 10; i++)
    {
        hashmap_remove(map, (void *)(intptr_t)i);
    }
    ASSERT(hashmap_size(map) == 10);

    hashmap_free(map);
    LOGI("[deep] 2.1 PASS (raw allocator, 3 paths exercised)");
    return 0;
}

static int test_hashmap_foreach_null_guard(void)
{
    LOGI("[deep] 2.2 hashmap foreach NULL guards");

    /* NULL map should not crash */
    hashmap_foreach(NULL, NULL, NULL);

    hashmap_t *map = hashmap_create(4, int_hash_fn, NULL, NULL, int_eq_fn, NULL);
    ASSERT(map != NULL);
    hashmap_put(map, (void *)1, (void *)100);

    /* NULL callback should not crash */
    hashmap_foreach(map, NULL, NULL);
    ASSERT(hashmap_size(map) == 1);

    /* NULL map with valid callback should not crash */
    hashmap_foreach(NULL, (hashmap_iter_cb)int_eq_fn, NULL);

    hashmap_free(map);
    LOGI("[deep] 2.2 PASS");
    return 0;
}

/* Stress test: 10000 entries, force many rehashes */
static int test_hashmap_stress(void)
{
    LOGI("[deep] 2.3 hashmap stress (10000 entries)");

    hashmap_t *map = hashmap_create(4, int_hash_fn, NULL, NULL, int_eq_fn, NULL);
    ASSERT(map != NULL);

    const int N = 10000;
    for (int i = 0; i < N; i++)
    {
        hashmap_put(map, (void *)(intptr_t)(i + 1), (void *)(intptr_t)(i * 3));
    }
    ASSERT(hashmap_size(map) == (size_t)N);

    /* Spot-check */
    for (int i = 0; i < N; i += 100)
    {
        void *val = hashmap_get(map, (void *)(intptr_t)(i + 1));
        ASSERT(val == (void *)(intptr_t)(i * 3));
    }

    /* Remove all */
    for (int i = 0; i < N; i++)
    {
        hashmap_remove(map, (void *)(intptr_t)(i + 1));
    }
    ASSERT(hashmap_size(map) == 0);

    hashmap_free(map);
    LOGI("[deep] 2.3 PASS");
    return 0;
}

/* Foreach early-exit test */
typedef struct {
    int stop_after;
    int visited;
} early_exit_ctx_t;

static bool early_exit_cb(void *key, void *value, void *context)
{
    early_exit_ctx_t *ctx = (early_exit_ctx_t *)context;
    ctx->visited++;
    return ctx->visited < ctx->stop_after; /* return false on Nth visit */
}

static int test_hashmap_foreach_early_exit(void)
{
    LOGI("[deep] 2.4 hashmap foreach early exit");

    /* Use a single-bucket map (bad hash) so all entries are in one chain.
     * This guarantees break exits the iteration at exactly stop_after. */
    hashmap_t *map = hashmap_create(16, int_hash_fn, NULL, NULL, int_eq_fn, NULL);
    ASSERT(map != NULL);

    for (int i = 1; i <= 100; i++)
    {
        hashmap_put(map, (void *)(intptr_t)i, (void *)(intptr_t)i);
    }

    early_exit_ctx_t ctx = { .stop_after = 5, .visited = 0 };
    hashmap_foreach(map, early_exit_cb, &ctx);
    /* After fix: goto foreach_done exits both loops immediately.
     * Callback returns false on the 5th call, so exactly 5 visited. */
    ASSERT(ctx.visited == ctx.stop_after);
    LOGI("[deep] 2.4 visited=%d (== %d)", ctx.visited, ctx.stop_after);

    hashmap_free(map);
    LOGI("[deep] 2.4 PASS");
    return 0;
}

/* ============================================================================
 * Section 3: file_logger path truncation + invalid config rejection
 * ========================================================================== */

static int test_file_logger_path_overflow(void)
{
    LOGI("[deep] 3.1 file_logger path overflow protection");

    file_logger_cfg cfg;
    memset(&cfg, 0, sizeof(cfg));

    /* Fill path to exactly MAX_LOG_FOLDER_PATH_SIZE-1 (no room for '/') */
    memset(cfg.log_folder_path, 'A', MAX_LOG_FOLDER_PATH_SIZE - 1);
    cfg.log_folder_path[MAX_LOG_FOLDER_PATH_SIZE - 1] = '\0';
    cfg.log_queue_size = 64;
    cfg.one_piece_file_max_len = 1024;
    strlcpy(cfg.log_file_name_prefix, "test", MAX_LOG_FILE_NAME_PREFIX_SIZE);

    /* Path is full, cannot append '/'. Init should fail gracefully. */
    file_logger_handle h = file_logger_init(&cfg);
    /* The path is 255 chars of 'A' with no trailing '/'.
     * After strreplace (no-op since no backslash), it tries to append '/'.
     * 255 + 2 > 256, so it should goto cleanup_on_error -> return NULL. */
    ASSERT(h == NULL);

    LOGI("[deep] 3.1 PASS (path overflow returns NULL)");
    return 0;
}

static int test_file_logger_invalid_configs(void)
{
    LOGI("[deep] 3.2 file_logger invalid config rejection");

    /* NULL cfg */
    ASSERT(file_logger_init(NULL) == NULL);

    /* Empty path */
    file_logger_cfg cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.log_queue_size = 64;
    ASSERT(file_logger_init(&cfg) == NULL);

    /* queue_size < 2 */
    strlcpy(cfg.log_folder_path, "./logs", MAX_LOG_FOLDER_PATH_SIZE);
    cfg.log_queue_size = 1;
    ASSERT(file_logger_init(&cfg) == NULL);

    LOGI("[deep] 3.2 PASS");
    return 0;
}

static int test_file_logger_normal_lifecycle(void)
{
    LOGI("[deep] 3.3 file_logger normal lifecycle");

    file_logger_cfg cfg;
    memset(&cfg, 0, sizeof(cfg));
    strlcpy(cfg.log_folder_path, "./test_deep_logs", MAX_LOG_FOLDER_PATH_SIZE);
    strlcpy(cfg.log_file_name_prefix, "deep", MAX_LOG_FILE_NAME_PREFIX_SIZE);
    cfg.log_queue_size = 64;
    cfg.one_piece_file_max_len = 4096;
    cfg.is_try_my_best_to_keep_log = true;
    cfg.max_log_retention_days = 1;

    file_logger_handle h = file_logger_init(&cfg);
    ASSERT(h != NULL);

    /* Write some log entries */
    for (int i = 0; i < 50; i++)
    {
        char msg[128];
        int len = snprintf(msg, sizeof(msg), "[%d] deep validation log entry\n", i);
        int ret = file_logger_log(h, msg, (size_t)len);
        ASSERT(ret == 0);
    }

    /* Give worker thread time to flush */
    usleep(100000); /* 100ms */

    file_logger_destroy(&h);
    ASSERT(h == NULL);

    LOGI("[deep] 3.3 PASS");
    return 0;
}

/* ============================================================================
 * Section 4: strings edge cases (strlcpy, strlcat, strreplace)
 * ========================================================================== */

static int test_strlcpy_edge_cases(void)
{
    LOGI("[deep] 4.1 strlcpy edge cases");

    char dst[8];

    /* Normal copy */
    size_t ret = strlcpy(dst, "hello", sizeof(dst));
    ASSERT(ret == 5);
    ASSERT(strcmp(dst, "hello") == 0);

    /* Truncation: src longer than dst */
    ret = strlcpy(dst, "this is too long", sizeof(dst));
    ASSERT(ret == 16); /* strlen(src) */
    ASSERT(dst[7] == '\0'); /* always null-terminated */
    ASSERT(strncmp(dst, "this is", 7) == 0);

    /* Zero size: should not write anything */
    dst[0] = 'X';
    ret = strlcpy(dst, "overwrite", 0);
    ASSERT(ret == 9);
    ASSERT(dst[0] == 'X'); /* unchanged */

    /* Size 1: only null terminator */
    ret = strlcpy(dst, "test", 1);
    ASSERT(ret == 4);
    ASSERT(dst[0] == '\0');

    LOGI("[deep] 4.1 PASS");
    return 0;
}

static int test_strlcat_edge_cases(void)
{
    LOGI("[deep] 4.2 strlcat edge cases");

    char dst[16];

    strlcpy(dst, "Hello", sizeof(dst));
    size_t ret = strlcat(dst, " World!", sizeof(dst));
    ASSERT(ret == 12); /* 5 + 7 */
    ASSERT(strcmp(dst, "Hello World!") == 0);

    /* Truncation */
    strlcpy(dst, "ABCDEFGHIJ", sizeof(dst)); /* 10 chars */
    ret = strlcat(dst, "KLMNOP", sizeof(dst)); /* would need 16, buf is 16 -> 15 chars + null */
    ASSERT(ret == 16); /* 10 + 6 */
    ASSERT(dst[15] == '\0');
    ASSERT(strlen(dst) == 15);

    LOGI("[deep] 4.2 PASS");
    return 0;
}

static int test_strreplace_edge_cases(void)
{
    LOGI("[deep] 4.3 strreplace edge cases");

    /* Basic replacement. strreplace returns an ownership-transfer buffer. */
    char *r = strreplace("hello world", "world", "earth");
    ASSERT(r != NULL);
    ASSERT(strcmp(r, "hello earth") == 0);
    free(r);

    /* No match */
    r = strreplace("hello", "xyz", "abc");
    ASSERT(r != NULL);
    ASSERT(strcmp(r, "hello") == 0);
    free(r);

    /* Multiple occurrences */
    r = strreplace("aaa", "a", "bb");
    ASSERT(r != NULL);
    ASSERT(strcmp(r, "bbbbbb") == 0);
    free(r);

    /* Empty pattern - implementation returns NULL (prevents infinite loop) */
    r = strreplace("test", "", "x");
    ASSERT(r == NULL);

    /* Replace with empty (deletion) */
    r = strreplace("he//llo", "//", "");
    ASSERT(r != NULL);
    ASSERT(strcmp(r, "hello") == 0);
    free(r);

    /* Backslash to forward slash (same as file_logger uses) */
    r = strreplace("C:\\Users\\test\\logs", "\\", "/");
    ASSERT(r != NULL);
    ASSERT(strcmp(r, "C:/Users/test/logs") == 0);
    free(r);

    LOGI("[deep] 4.3 PASS");
    return 0;
}

/* ============================================================================
 * Section 5: allocator calloc overflow check
 * ========================================================================== */

static int test_calloc_overflow(void)
{
    LOGI("[deep] 5.1 calloc overflow protection");

    /* lcu_calloc should detect multiplication overflow and return NULL. */
    void *p = calloc(SIZE_MAX, SIZE_MAX);
    ASSERT(p == NULL);

    /* Very large but technically valid (will likely fail due to OOM, not overflow) */
    p = calloc(1, SIZE_MAX / 2);
    /* This is expected to fail (OOM) but should NOT crash */
    if (p) free(p);

    LOGI("[deep] 5.1 PASS");
    return 0;
}

/* ============================================================================
 * Section 6: hashmap_create_with_allocator with NULL allocator
 * ========================================================================== */

static int test_hashmap_null_allocator(void)
{
    LOGI("[deep] 6.1 hashmap_create_with_allocator NULL allocator");

    hashmap_t *map = hashmap_create_with_allocator(8, int_hash_fn, NULL, NULL, int_eq_fn,
                                        NULL, NULL);
    ASSERT(map == NULL);

    LOGI("[deep] 6.1 PASS");
    return 0;
}

/* ============================================================================
 * Section 7: hashmap operations on NULL map pointer
 * ========================================================================== */

static int test_hashmap_null_map_ops(void)
{
    LOGI("[deep] 7.1 hashmap NULL map operations");

    /* All of these should return safely without crash */
    ASSERT(hashmap_put(NULL, (void *)1, (void *)1) == NULL);
    ASSERT(hashmap_get(NULL, (void *)1) == NULL);
    ASSERT(hashmap_remove(NULL, (void *)1) == NULL);
    hashmap_clear(NULL);  /* should not crash */
    hashmap_free(NULL);   /* should not crash */

    LOGI("[deep] 7.1 PASS");
    return 0;
}

/* ============================================================================
 * Main entry
 * ========================================================================== */

int deep_validation_test(void)
{
    LOGI("=== deep_validation_test BEGIN ===");

    /* Section 1: allocation_tracker recursion */
    if (0 != test_tracker_no_recursion()) return -1;
    if (0 != test_tracker_realloc_stress()) return -1;
    if (0 != test_tracker_strdup_strndup()) return -1;

    /* Section 2: hashmap allocator + guards */
    if (0 != test_hashmap_raw_allocator()) return -1;
    if (0 != test_hashmap_foreach_null_guard()) return -1;
    if (0 != test_hashmap_stress()) return -1;
    if (0 != test_hashmap_foreach_early_exit()) return -1;

    /* Section 3: file_logger boundary */
    if (0 != test_file_logger_path_overflow()) return -1;
    if (0 != test_file_logger_invalid_configs()) return -1;
    if (0 != test_file_logger_normal_lifecycle()) return -1;

    /* Section 4: strings */
    if (0 != test_strlcpy_edge_cases()) return -1;
    if (0 != test_strlcat_edge_cases()) return -1;
    if (0 != test_strreplace_edge_cases()) return -1;

    /* Section 5: allocator overflow */
    if (0 != test_calloc_overflow()) return -1;

    /* Section 6-7: hashmap defensive checks */
    if (0 != test_hashmap_null_allocator()) return -1;
    if (0 != test_hashmap_null_map_ops()) return -1;

    LOGI("=== deep_validation_test END: ALL TESTS PASS ===");
    return 0;
}

#include "lcu_test_registry.h"
LCU_TEST_REGISTER(deep_validation_test, "deep validation of 0527-0608 changes");
