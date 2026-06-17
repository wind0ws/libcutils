/**
 * Regression test (C++ TU): operator new/delete ODR + placement-delete leak.
 *
 * This file is the SECOND C++ translation unit (besides main.cpp) that includes
 * mem_debug.h on its first line, exactly as the project convention demands
 * ("在所有 .c/.cpp 源文件第一行包含 mem/mem_debug.h").
 *
 * It locks down two fixes:
 *
 *  Fix #2 (ODR): Before the fix, mem_debug.h DEFINED the global
 *    operator new/new[]/delete/delete[] (non-inline) inside the header under
 *    _USE_LCU_MEM_CHECK. With two C++ TUs including it, the linker saw two
 *    definitions of the global `operator delete` -> multiple-definition link
 *    error. The mere existence of this file in the build, compiled in the
 *    memcheck configuration, reproduces that link failure. After the fix the
 *    operators live in a single library .cpp and this links cleanly.
 *
 *  Fix M3 (placement delete): mem_debug.h rewrites `new` to the placement form
 *    operator new(size, file, func, line). If a constructor throws, the C++
 *    runtime must call the MATCHING placement operator delete to reclaim the
 *    raw storage. Before the fix no such placement delete existed -> the raw
 *    block leaked on every throwing construction. After the fix the matching
 *    placement delete calls lcu_free and the leak is gone.
 *
 * Both checks only have "teeth" in the memcheck build
 * (-D_LCU_MEM_CHECK_FEATURE_ENABLE=1): only then are the operators active and
 * the tracker live. In the default build this degrades to a basic no-crash /
 * no-throw-escape smoke check, mirroring ownership_contract_test.c.
 */

#include "mem/mem_debug.h"
#include "common_macro.h"
#include "mem/allocation_tracker.h"

#include <stdio.h>
#include <string.h>

#define LOG_TAG "OPNEW_ODR_TEST"
#include "log/logger.h"

namespace
{
    /* A type whose constructor always throws AFTER operator new has allocated
     * the raw storage. This is the exact path that requires a matching
     * placement delete. */
    struct ThrowOnCtor
    {
        char payload[64];
        ThrowOnCtor()
        {
            memset(payload, 0xAB, sizeof(payload));
            throw 42; /* force the runtime to reclaim via placement delete */
        }
    };
}

/* Count currently-live tracked bytes. Returns 0 when the tracker is inactive
 * (default build) — in that case the leak assertion below is vacuously true. */
static size_t live_tracked_bytes(void)
{
    return allocation_tracker_expect_no_allocations(NULL, NULL);
}

extern "C" int opnew_odr_placement_delete_test(void)
{
    LOGI("=== opnew_odr_placement_delete_test BEGIN ===");

    int rc = 0;

    /* --- Fix M3: throwing ctor must not leak the raw storage. --- */
    const int kIterations = 100;
    size_t before = live_tracked_bytes();

    for (int i = 0; i < kIterations; ++i)
    {
        bool caught = false;
        try
        {
            /* `new` is rewritten to placement new(file,func,line) under memcheck. */
            ThrowOnCtor *p = new ThrowOnCtor();
            (void)p; /* unreachable: ctor always throws */
        }
        catch (int)
        {
            caught = true;
        }
        ASSERT(caught);
    }

    size_t after = live_tracked_bytes();

    /* Pre-M3 + memcheck: each throwing new leaks sizeof(ThrowOnCtor) -> after
     * grows by ~kIterations * sizeof. Post-M3: placement delete frees -> equal.
     * Default build: both are 0. */
    if (after != before)
    {
        LOGE("[opnew] LEAK: throwing-ctor leaked %zu bytes over %d iters (before=%zu after=%zu)",
             after - before, kIterations, before, after);
        rc = -1;
    }
    else
    {
        LOGI("[opnew] M3 PASS: no leak across %d throwing constructions", kIterations);
    }

    LOGI("=== opnew_odr_placement_delete_test END: %s ===", (0 == rc) ? "PASS" : "FAIL");
    return rc;
}

#include "lcu_test_registry.h"
extern "C" {
LCU_TEST_REGISTER(opnew_odr_placement_delete_test,
                  "operator new/delete ODR (2nd C++ TU) + placement-delete leak");
}
