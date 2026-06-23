/**
 * Deep validation test #2: thpool + ring_buffer + list
 *
 * Covers:
 * 1. thpool: multi-round create/destroy, sticky shutdown speed, concurrent job submission
 * 2. ring_buffer: SPSC concurrent producer/consumer with atomic visibility (P1-5)
 * 3. ring_buffer: boundary conditions (full, empty, wrap-around, peek vs read)
 * 4. list: clear/remove edge cases
 */

#include "mem/mem_debug.h"
#include "common_macro.h"
#include "thread/thpool.h"
#include "thread/posix_thread.h"
#include "ring/ring_buffer.h"
#include "data/list.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#define LOG_TAG "DEEP_VALID2"
#include "log/logger.h"

/* ============================================================================
 * Section 1: thpool stress tests
 * ========================================================================== */

static volatile long g_counter_1 = 0;
static pthread_mutex_t g_lock_1 = PTHREAD_MUTEX_INITIALIZER;

static void job_increment(void *arg)
{
    (void)arg;
    pthread_mutex_lock(&g_lock_1);
    g_counter_1++;
    pthread_mutex_unlock(&g_lock_1);
}

static int test_thpool_multi_round(void)
{
    LOGI("[deep2] 1.1 thpool multi-round create/destroy");

    /* Create and destroy pool multiple times to verify no resource leak or hang */
    for (int round = 0; round < 5; round++)
    {
        g_counter_1 = 0;
        threadpool tp = thpool_init(4);
        ASSERT(tp != NULL);

        const int JOBS = 200;
        for (int i = 0; i < JOBS; i++)
        {
            int ret = thpool_add_work(tp, job_increment, NULL);
            ASSERT(ret == 0);
        }

        thpool_wait(tp);
        ASSERT(g_counter_1 == JOBS);

        clock_t t0 = clock();
        thpool_destroy(tp);
        double ms = (double)(clock() - t0) * 1000.0 / CLOCKS_PER_SEC;
        ASSERT(ms < 500.0); /* sticky shutdown should be near-instant */
    }

    LOGI("[deep2] 1.1 PASS (5 rounds, 200 jobs each)");
    return 0;
}

static volatile long g_counter_2 = 0;
static pthread_mutex_t g_lock_2 = PTHREAD_MUTEX_INITIALIZER;

static void job_heavy_work(void *arg)
{
    /* Simulate varying workload */
    int *val = (int *)arg;
    volatile int dummy = 0;
    for (int i = 0; i < (*val % 50); i++)
    {
        dummy += i;
    }
    (void)dummy;
    pthread_mutex_lock(&g_lock_2);
    g_counter_2++;
    pthread_mutex_unlock(&g_lock_2);
}

static int test_thpool_heavy_concurrent(void)
{
    LOGI("[deep2] 1.2 thpool heavy concurrent jobs");

    g_counter_2 = 0;
    const int NUM_THREADS = 8;
    const int NUM_JOBS = 1000;

    threadpool tp = thpool_init(NUM_THREADS);
    ASSERT(tp != NULL);

    int args[1000];
    for (int i = 0; i < NUM_JOBS; i++)
    {
        args[i] = i;
        thpool_add_work(tp, job_heavy_work, &args[i]);
    }

    thpool_wait(tp);
    ASSERT(g_counter_2 == NUM_JOBS);
    ASSERT(thpool_num_threads_working(tp) == 0);

    thpool_destroy(tp);
    LOGI("[deep2] 1.2 PASS (1000 jobs, 8 threads)");
    return 0;
}

static int test_thpool_wait_no_jobs(void)
{
    LOGI("[deep2] 1.3 thpool_wait with no jobs");

    threadpool tp = thpool_init(2);
    ASSERT(tp != NULL);

    /* Wait on empty pool should return immediately */
    clock_t t0 = clock();
    thpool_wait(tp);
    double ms = (double)(clock() - t0) * 1000.0 / CLOCKS_PER_SEC;
    ASSERT(ms < 100.0);

    thpool_destroy(tp);
    LOGI("[deep2] 1.3 PASS (wait returned in %.1fms)", ms);
    return 0;
}

/* ============================================================================
 * Section 2: ring_buffer boundary tests
 * ========================================================================== */

static int test_ring_buffer_basic(void)
{
    LOGI("[deep2] 2.1 ring_buffer basic operations");

    ring_buffer_handle rb = ring_buffer_create(64);
    ASSERT(rb != NULL);

    /* Fresh buffer should be empty */
    ASSERT(ring_buffer_is_empty(rb) == true);
    ASSERT(ring_buffer_is_full(rb) == false);

    uint32_t cap = ring_buffer_real_capacity(rb);
    ASSERT(cap >= 64); /* rounded up to power of 2 */

    /* Write and read */
    const char *msg = "Hello Ring!";
    uint32_t msg_len = (uint32_t)(strlen(msg) + 1);
    uint32_t written = ring_buffer_write(rb, msg, msg_len);
    ASSERT(written == msg_len);
    ASSERT(ring_buffer_available_read(rb) == msg_len);

    char buf[64] = {0};
    uint32_t read_len = ring_buffer_read(rb, buf, msg_len);
    ASSERT(read_len == msg_len);
    ASSERT(strcmp(buf, msg) == 0);
    ASSERT(ring_buffer_is_empty(rb) == true);

    ring_buffer_destroy(&rb);
    ASSERT(rb == NULL);

    LOGI("[deep2] 2.1 PASS");
    return 0;
}

static int test_ring_buffer_full_and_wrap(void)
{
    LOGI("[deep2] 2.2 ring_buffer full + wrap-around");

    ring_buffer_handle rb = ring_buffer_create(32); /* actual capacity is 32 */
    ASSERT(rb != NULL);
    uint32_t cap = ring_buffer_real_capacity(rb);

    /* Fill buffer completely */
    char *fill_data = (char *)calloc(1, cap);
    ASSERT(fill_data != NULL);
    memset(fill_data, 'X', cap);

    uint32_t written = ring_buffer_write(rb, fill_data, cap);
    ASSERT(written == cap);
    ASSERT(ring_buffer_is_full(rb) == true);
    ASSERT(ring_buffer_available_write(rb) == 0);

    /* Try to write more - should write 0 */
    char extra = 'Y';
    written = ring_buffer_write(rb, &extra, 1);
    ASSERT(written == 0);

    /* Read half */
    uint32_t half = cap / 2;
    char *read_buf = (char *)calloc(1, cap);
    ASSERT(read_buf != NULL);
    uint32_t read_len = ring_buffer_read(rb, read_buf, half);
    ASSERT(read_len == half);

    /* Now available_write should be half */
    ASSERT(ring_buffer_available_write(rb) == half);

    /* Write new data to cause wrap-around */
    memset(fill_data, 'Z', half);
    written = ring_buffer_write(rb, fill_data, half);
    ASSERT(written == half);
    ASSERT(ring_buffer_is_full(rb) == true);

    /* Read all remaining and verify data integrity across wrap */
    char *verify_buf = (char *)calloc(1, cap);
    ASSERT(verify_buf != NULL);
    read_len = ring_buffer_read(rb, verify_buf, cap);
    ASSERT(read_len == cap);

    /* First half should be 'X' (remaining from initial write), second half 'Z' */
    for (uint32_t i = 0; i < half; i++)
    {
        ASSERT(verify_buf[i] == 'X');
    }
    for (uint32_t i = half; i < cap; i++)
    {
        ASSERT(verify_buf[i] == 'Z');
    }

    free(fill_data);
    free(read_buf);
    free(verify_buf);
    ring_buffer_destroy(&rb);

    LOGI("[deep2] 2.2 PASS");
    return 0;
}

static int test_ring_buffer_peek_and_discard(void)
{
    LOGI("[deep2] 2.3 ring_buffer peek + discard");

    ring_buffer_handle rb = ring_buffer_create(128);
    ASSERT(rb != NULL);

    const char *data = "ABCDEFGHIJ"; /* 10 bytes + null */
    ring_buffer_write(rb, data, 10);

    /* Peek should not advance read pointer */
    char peek_buf[16] = {0};
    uint32_t peeked = ring_buffer_peek(rb, peek_buf, 5);
    ASSERT(peeked == 5);
    ASSERT(memcmp(peek_buf, "ABCDE", 5) == 0);
    ASSERT(ring_buffer_available_read(rb) == 10); /* unchanged */

    /* Peek with offset */
    memset(peek_buf, 0, sizeof(peek_buf));
    peeked = ring_buffer_peek_with_offset(rb, 3, peek_buf, 4);
    ASSERT(peeked == 4);
    ASSERT(memcmp(peek_buf, "DEFG", 4) == 0);

    /* Discard 3 bytes */
    uint32_t discarded = ring_buffer_discard(rb, 3);
    ASSERT(discarded == 3);
    ASSERT(ring_buffer_available_read(rb) == 7);

    /* Read remaining */
    char final_buf[16] = {0};
    uint32_t avail = ring_buffer_available_read(rb);
    uint32_t final_read = ring_buffer_read(rb, final_buf, avail);
    ASSERT(final_read == 7);
    ASSERT(memcmp(final_buf, "DEFGHIJ", 7) == 0);

    ring_buffer_destroy(&rb);
    LOGI("[deep2] 2.3 PASS");
    return 0;
}

/* ============================================================================
 * Section 3: ring_buffer SPSC concurrent test
 * ========================================================================== */

typedef struct {
    ring_buffer_handle rb;
    int total_items;
    volatile int producer_done;
} spsc_ctx_t;

static void *spsc_producer(void *arg)
{
    spsc_ctx_t *ctx = (spsc_ctx_t *)arg;
    for (int i = 0; i < ctx->total_items; i++)
    {
        /* Spin until there's room */
        while (ring_buffer_available_write(ctx->rb) < sizeof(int))
        {
            usleep(10);
        }
        ring_buffer_write(ctx->rb, &i, sizeof(int));
    }
    ctx->producer_done = 1;
    return NULL;
}

static int test_ring_buffer_spsc_concurrent(void)
{
    LOGI("[deep2] 3.1 ring_buffer SPSC concurrent (P1-5 atomic visibility)");

    ring_buffer_handle rb = ring_buffer_create(256);
    ASSERT(rb != NULL);

    const int TOTAL = 5000;
    spsc_ctx_t ctx = { .rb = rb, .total_items = TOTAL, .producer_done = 0 };

    pthread_t producer_thread;
    int ret = pthread_create(&producer_thread, NULL, spsc_producer, &ctx);
    ASSERT(ret == 0);

    /* Consumer: read items and verify sequential order */
    int consumed = 0;
    int expected = 0;
    while (consumed < TOTAL)
    {
        if (ring_buffer_available_read(rb) >= sizeof(int))
        {
            int val;
            uint32_t read_sz = ring_buffer_read(rb, &val, sizeof(int));
            ASSERT(read_sz == sizeof(int));
            ASSERT(val == expected);
            expected++;
            consumed++;
        }
        else
        {
            usleep(10);
        }
    }

    pthread_join(producer_thread, NULL);
    ASSERT(consumed == TOTAL);
    ASSERT(ring_buffer_is_empty(rb) == true);

    ring_buffer_destroy(&rb);
    LOGI("[deep2] 3.1 PASS (5000 items, SPSC verified in-order)");
    return 0;
}

/* ============================================================================
 * Section 4: list edge cases
 * ========================================================================== */

static void int_free(void *ptr) { free(ptr); }

static int test_list_empty_operations(void)
{
    LOGI("[deep2] 4.1 list empty operations");

    list_t *lst = list_new(int_free);
    ASSERT(lst != NULL);
    ASSERT(list_is_empty(lst) == true);
    ASSERT(list_length(lst) == 0);

    /* Operations on empty list should not crash */
    ASSERT(list_front(lst) == NULL);
    ASSERT(list_back(lst) == NULL);
    ASSERT(list_remove(lst, (void *)0x1234) == false); /* non-existent */

    /* list_foreach on empty list */
    list_foreach(lst, NULL, NULL); /* NULL callback should not crash */

    list_free(lst);
    LOGI("[deep2] 4.1 PASS");
    return 0;
}

static bool count_iter(void *data, void *context)
{
    (void)data;
    int *count = (int *)context;
    (*count)++;
    return true;
}

static int test_list_stress(void)
{
    LOGI("[deep2] 4.2 list stress (1000 insert/remove)");

    list_t *lst = list_new(int_free);
    ASSERT(lst != NULL);

    /* Append 1000 items */
    for (int i = 0; i < 1000; i++)
    {
        int *p = (int *)malloc(sizeof(int));
        ASSERT(p != NULL);
        *p = i;
        list_append(lst, p);
    }
    ASSERT(list_length(lst) == 1000);

    /* Verify front and back */
    ASSERT(*(int *)list_front(lst) == 0);
    ASSERT(*(int *)list_back(lst) == 999);

    /* Count via foreach */
    int count = 0;
    list_foreach(lst, count_iter, &count);
    ASSERT(count == 1000);

    /* Remove front repeatedly */
    for (int i = 0; i < 500; i++)
    {
        void *front = list_front(lst);
        ASSERT(front != NULL);
        bool ok = list_remove(lst, front);
        ASSERT(ok);
    }
    ASSERT(list_length(lst) == 500);
    ASSERT(*(int *)list_front(lst) == 500);

    list_free(lst);
    LOGI("[deep2] 4.2 PASS");
    return 0;
}

/* ============================================================================
 * Section 5: ring_buffer create_with_mem
 * ========================================================================== */

static int test_ring_buffer_with_mem(void)
{
    LOGI("[deep2] 5.1 ring_buffer create_with_mem");

    /* Allocate aligned buffer large enough: struct header + power-of-2 data area.
     * ring_buffer_create_with_mem takes total buf_size; internally it reserves
     * a struct header and rounds down remaining to power of 2.
     * Use 512 to guarantee enough room. */
    const uint32_t buf_size = 512;
    void *mem = calloc(1, buf_size);
    ASSERT(mem != NULL);

    ring_buffer_handle rb = ring_buffer_create_with_mem(mem, buf_size);
    ASSERT(rb != NULL);

    /* Write and read to verify functionality */
    const char *test_str = "create_with_mem test!";
    uint32_t len = (uint32_t)(strlen(test_str) + 1);
    uint32_t written = ring_buffer_write(rb, test_str, len);
    ASSERT(written == len);

    char readback[64] = {0};
    uint32_t readlen = ring_buffer_read(rb, readback, len);
    ASSERT(readlen == len);
    ASSERT(strcmp(readback, test_str) == 0);

    /* Destroy does NOT free our buffer (need_free_myself == 0) */
    ring_buffer_destroy(&rb);
    ASSERT(rb == NULL);

    free(mem); /* We must free it ourselves */
    LOGI("[deep2] 5.1 PASS");
    return 0;
}

/* ============================================================================
 * Main entry
 * ========================================================================== */

int deep_validation2_test(void)
{
    LOGI("=== deep_validation2_test BEGIN ===");

    /* Section 1: thpool */
    if (0 != test_thpool_multi_round()) return -1;
    if (0 != test_thpool_heavy_concurrent()) return -1;
    if (0 != test_thpool_wait_no_jobs()) return -1;

    /* Section 2: ring_buffer boundaries */
    if (0 != test_ring_buffer_basic()) return -1;
    if (0 != test_ring_buffer_full_and_wrap()) return -1;
    if (0 != test_ring_buffer_peek_and_discard()) return -1;

    /* Section 3: ring_buffer SPSC concurrent */
    if (0 != test_ring_buffer_spsc_concurrent()) return -1;

    /* Section 4: list */
    if (0 != test_list_empty_operations()) return -1;
    if (0 != test_list_stress()) return -1;

    /* Section 5: ring_buffer with external memory */
    if (0 != test_ring_buffer_with_mem()) return -1;

    LOGI("=== deep_validation2_test END: ALL TESTS PASS ===");
    return 0;
}

#include "lcu_test_registry.h"
LCU_TEST_REGISTER(deep_validation2_test, "deep validation of thpool/ring_buffer/list changes");
