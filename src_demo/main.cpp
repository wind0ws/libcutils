/*
 * 注：本文件保留为 .cpp 而非 .c，是为了让 mem_debug.h 在 C++ 模式下
 * 重载 operator new/delete[] 生效，从而支持 memleak_test 测试项
 * (验证 new/delete 内存泄漏检测能力)。
 * 见 inc/mem/mem_debug.h:91-127。
 */
#include "mem/mem_debug.h"
#include "common_macro.h"
#include "lcu_test_registry.h"
#include "lcu_test_args.h"
#include "lcu_test_console.h"
#include "lcu_test_glob.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_TAG "MAIN"
#include "log/logger.h"
#include "log/file_logger.h"
#include "lcu.h"
#include "time/time_util.h"

#define KB_TIMEOUT (5)

/* ======================== file_logger 顺序依赖包装 ======================== */

EXTERN_C_START
extern int file_logger_test_begin(void);
extern int file_logger_test_end(void);
extern int time_util_test(void);
extern int allocator_test(void);
EXTERN_C_END

#if (_LCU_LOGGER_TYPE_XLOG != LCU_LOGGER_SELECTOR)
#error "FILE_LOGGER only support XLOG!"
#endif

static int file_logger_test_impl(void)
{
    int ret = file_logger_test_begin();
    if (ret != 0)
    {
        return ret;
    }
    ret = time_util_test();
    if (ret != 0)
    {
        file_logger_test_end();
        return ret;
    }
    ret = file_logger_test_end();
    return ret;
}

extern "C" {
int file_logger_test(void)
{
    return file_logger_test_impl();
}
LCU_TEST_REGISTER(file_logger_test, "test file logger");
}

/* ======================== memleak_test (opt-in，故意 leak) ======================== */

static int memleak_test_impl(void)
{
    int ret = allocator_test();
    char *leak_mem = new char[16];
    memset(leak_mem, 0xFF, 16);
    LOGD("leak_mem=0x%p, leak_mem[0]=%d", &leak_mem[0], leak_mem[0]);
    /* 故意不 delete[] leak_mem，验证检测能力 */
    return ret;
}

extern "C" {
int memleak_test(void)
{
    return memleak_test_impl();
}
LCU_TEST_REGISTER_OPTIONAL(memleak_test, "test mem leak detection (intentionally leaks, opt-in only)");
}

/* ======================== 测试运行核心 ======================== */

#define LOG_STAR_LINE_LOCAL "************************************************************"

typedef struct
{
    const lcu_test_entry_t *entry;
    int ret;
    double duration_sec;
} lcu_test_result_t;

static lcu_test_result_t g_results[256];
static int g_result_count = 0;

static int run_cases(const lcu_test_entry_t **cases, int count,
                     bool fail_fast, lcu_test_stats_t *stats)
{
    g_result_count = 0;
    int last_failure = 0;

    uint64_t total_start = 0;
    time_util_query_performance_ms(&total_start);

    for (int i = 0; i < count && i < (int)(sizeof(g_results) / sizeof(g_results[0])); ++i)
    {
        const lcu_test_entry_t *e = cases[i];
        fprintf(stderr, "\n==> [%d/%d] Run %s: %s\n", i + 1, count, e->name, e->description);
        LOGI("==> [%d/%d] Run %s: %s", i + 1, count, e->name, e->description);
        LOGD("\n%s\n--> %s() executing...\n", LOG_STAR_LINE_LOCAL, e->name);

        uint64_t case_start = 0;
        time_util_query_performance_ms(&case_start);
        int ret = e->fn();
        uint64_t case_end = 0;
        time_util_query_performance_ms(&case_end);
        double dur = (double)(case_end - case_start) / 1000.0;

        LOGD("\n<-- %s() finished with %d (%.3fs)\n%s\n", e->name, ret, dur, LOG_STAR_LINE_LOCAL);
        fprintf(stderr, "<== [%d/%d] %s: %s (%.3fs)\n", i + 1, count, e->name,
                ret == 0 ? "PASS" : "FAIL", dur);

        g_results[g_result_count].entry = e;
        g_results[g_result_count].ret = ret;
        g_results[g_result_count].duration_sec = dur;
        ++g_result_count;

        if (ret == 0)
        {
            ++stats->passed;
        }
        else
        {
            ++stats->failed;
            last_failure = ret;
            if (fail_fast)
            {
                LOGE("--fail-fast: stopping after %s failed (ret=%d)", e->name, ret);
                break;
            }
        }
    }

    uint64_t total_end = 0;
    time_util_query_performance_ms(&total_end);
    stats->elapsed_sec = (double)(total_end - total_start) / 1000.0;
    return last_failure;
}

static void print_summary(const lcu_test_stats_t *stats)
{
    fprintf(stderr, "\n%s\n", LOG_STAR_LINE_LOCAL);
    fprintf(stderr, "Summary: %d passed, %d failed, %d skipped, %.3fs total\n",
            stats->passed, stats->failed, stats->skipped, stats->elapsed_sec);
    if (stats->failed > 0)
    {
        fprintf(stderr, "\nFailed cases:\n");
        for (int i = 0; i < g_result_count; ++i)
        {
            if (g_results[i].ret != 0)
            {
                fprintf(stderr, "  [FAIL] %s (ret=%d, %.3fs)\n",
                        g_results[i].entry->name, g_results[i].ret, g_results[i].duration_sec);
            }
        }
    }
    fprintf(stderr, "%s\n", LOG_STAR_LINE_LOCAL);
}

static void write_junit_report(const char *path, const lcu_test_stats_t *stats)
{
    FILE *fp = fopen(path, "w");
    if (!fp)
    {
        LOGE("failed to open junit file: %s", path);
        return;
    }
    int total = stats->passed + stats->failed + stats->skipped;
    fprintf(fp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    fprintf(fp, "<testsuite name=\"lcu_demo\" tests=\"%d\" failures=\"%d\" skipped=\"%d\" time=\"%.3f\">\n",
            total, stats->failed, stats->skipped, stats->elapsed_sec);
    for (int i = 0; i < g_result_count; ++i)
    {
        const lcu_test_result_t *r = &g_results[i];
        fprintf(fp, "  <testcase classname=\"lcu\" name=\"%s\" time=\"%.3f\">",
                r->entry->name, r->duration_sec);
        if (r->ret != 0)
        {
            fprintf(fp, "<failure message=\"ret=%d\"/>", r->ret);
        }
        fprintf(fp, "</testcase>\n");
    }
    fprintf(fp, "</testsuite>\n");
    fclose(fp);
    LOGI("junit report written: %s", path);
}

/* ======================== 模式分发 ======================== */

static void dispatch_list(void)
{
    size_t count = lcu_test_registry_count();
    fprintf(stdout, "Registered %zu test cases:\n", count);
    size_t i = 0;
    for (const lcu_test_entry_t *e = lcu_test_registry_head(); e; e = e->next, ++i)
    {
        fprintf(stdout, "  %02zu  %-32s%s  %s\n",
                i, e->name,
                e->exclude_from_all ? " [opt-in]" : "         ",
                e->description);
    }
    fprintf(stdout, "\nNote: [opt-in] cases are NOT included in --all, run them by name explicitly.\n");
    fprintf(stdout, "      Numeric index is no longer supported since v1.9.0, use names only.\n");
}

/* B2: ALL/FILTER 不受 64 上限限制，使用 256 静态数组（注册总数远低于此） */
static int collect_all(const lcu_test_entry_t **out, int max)
{
    int n = 0;
    for (const lcu_test_entry_t *cur = lcu_test_registry_head(); cur && n < max; cur = cur->next)
    {
        if (!cur->exclude_from_all)
        {
            out[n++] = cur;
        }
    }
    return n;
}

static int collect_filter(const char *pattern, const lcu_test_entry_t **out, int max)
{
    int n = 0;
    for (const lcu_test_entry_t *cur = lcu_test_registry_head(); cur && n < max; cur = cur->next)
    {
        if (lcu_glob_match(pattern, cur->name))
        {
            out[n++] = cur;
        }
    }
    return n;
}

static int collect_by_name(const lcu_test_run_options_t *opts,
                           const lcu_test_entry_t **out, int max)
{
    int n = 0;
    for (int i = 0; i < opts->name_count && n < max; ++i)
    {
        const char *arg = opts->names[i];
        const lcu_test_entry_t *e = lcu_test_registry_find_by_name(arg);
        if (!e)
        {
            /* D1: 数字参数给友好提示 */
            bool all_digits = (arg[0] != '\0');
            for (const char *p = arg; *p; ++p)
            {
                if (*p < '0' || *p > '9')
                {
                    all_digits = false;
                    break;
                }
            }
            if (all_digits)
            {
                fprintf(stderr,
                    "error: '%s' looks like a numeric index, "
                    "which is no longer supported since v1.9.0. "
                    "Use --list to see test names.\n",
                    arg);
            }
            else
            {
                fprintf(stderr, "error: unknown test '%s' (use --list to see available)\n", arg);
            }
            return -1;
        }
        out[n++] = e;
    }
    return n;
}

/* runner 回调（给 console 用） */
static int main_runner(const lcu_test_entry_t **cases, int count,
                       bool fail_fast, lcu_test_stats_t *stats)
{
    return run_cases(cases, count, fail_fast, stats);
}

/* ======================== main ======================== */

EXTERN_C
int main(int argc, char *argv[])
{
    int ret = 0;
    MEM_CHECK_INIT();
    lcu_global_init();
    lcu_console_setup();
    LOGI("hello world: LCU_VER:%s\n", lcu_get_version());

    lcu_test_run_options_t opts;
    if (lcu_test_parse_args(argc, argv, &opts) != 0)
    {
        ret = 255;
        goto EXIT_CLEANUP;
    }

    if (opts.mode == LCU_TEST_MODE_HELP)
    {
        lcu_test_print_help(argv[0] ? argv[0] : "lcu_demo");
        goto EXIT_CLEANUP;
    }
    if (opts.mode == LCU_TEST_MODE_LIST)
    {
        dispatch_list();
        goto EXIT_CLEANUP;
    }

    {
        const lcu_test_entry_t *cases[256];
        int case_count = 0;
        lcu_test_stats_t stats = {0, 0, 0, 0.0};

        switch (opts.mode)
        {
        case LCU_TEST_MODE_ALL:
            case_count = collect_all(cases, (int)(sizeof(cases) / sizeof(cases[0])));
            break;
        case LCU_TEST_MODE_FILTER:
            case_count = collect_filter(opts.filter, cases, (int)(sizeof(cases) / sizeof(cases[0])));
            if (case_count == 0)
            {
                fprintf(stderr, "no tests matched filter '%s'\n", opts.filter);
                ret = 255;
                goto EXIT_CLEANUP;
            }
            break;
        case LCU_TEST_MODE_BY_NAME:
            case_count = collect_by_name(&opts, cases, (int)(sizeof(cases) / sizeof(cases[0])));
            if (case_count < 0)
            {
                ret = 255;
                goto EXIT_CLEANUP;
            }
            break;
        case LCU_TEST_MODE_DEFAULT:
        default:
        {
            /* H3: 默认行为兼容旧版（只跑 time_util_test） */
            if (lcu_console_wait_interrupt(KB_TIMEOUT))
            {
                int menu_ret = lcu_console_run_from_menu(main_runner, opts.fail_fast, &stats);
                if (menu_ret >= 0)
                {
                    print_summary(&stats);
                    if (opts.junit_path)
                    {
                        write_junit_report(opts.junit_path, &stats);
                    }
                }
                ret = stats.failed > 254 ? 254 : stats.failed;
                goto EXIT_CLEANUP;
            }
            LOGI("  ====smoke test (time_util_test)====  ");
            const lcu_test_entry_t *smoke = lcu_test_registry_find_by_name("time_util_test");
            if (smoke)
            {
                cases[0] = smoke;
                case_count = 1;
            }
            else
            {
                LOGE("smoke test 'time_util_test' not registered!");
                ret = 1;
                goto EXIT_CLEANUP;
            }
            break;
        }
        }

        if (case_count > 0)
        {
            run_cases(cases, case_count, opts.fail_fast, &stats);
            print_summary(&stats);
            if (opts.junit_path)
            {
                write_junit_report(opts.junit_path, &stats);
            }
            ret = stats.failed > 254 ? 254 : stats.failed;
        }
    }

EXIT_CLEANUP:
    LOGI("...bye bye...  %d\n", ret);
    LOG_GLOBAL_CLEANUP(NULL);
    lcu_global_cleanup();
    MEM_CHECK_DEINIT();
    return ret;
}
