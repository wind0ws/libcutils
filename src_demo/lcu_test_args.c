#include "mem/mem_debug.h"
#include "lcu_test_args.h"
#include <string.h>
#include <stdio.h>

int lcu_test_parse_args(int argc, char *argv[], lcu_test_run_options_t *options)
{
    memset(options, 0, sizeof(*options));
    options->mode = LCU_TEST_MODE_DEFAULT;

    for (int i = 1; i < argc; ++i)
    {
        const char *arg = argv[i];
        if (!arg || arg[0] == '\0')
        {
            continue;
        }
        if (0 == strcmp(arg, "--"))
        {
            break;
        }
        else if (0 == strcmp(arg, "--help") || 0 == strcmp(arg, "-h"))
        {
            options->mode = LCU_TEST_MODE_HELP;
            return 0;
        }
        else if (0 == strcmp(arg, "--list") || 0 == strcmp(arg, "-l"))
        {
            options->mode = LCU_TEST_MODE_LIST;
            return 0;
        }
        else if (0 == strcmp(arg, "--all") || 0 == strcmp(arg, "-a"))
        {
            options->mode = LCU_TEST_MODE_ALL;
        }
        else if (0 == strcmp(arg, "--fail-fast") || 0 == strcmp(arg, "-f"))
        {
            options->fail_fast = true;
        }
        else if (0 == strcmp(arg, "--junit"))
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "error: --junit requires a file path argument\n");
                return -1;
            }
            options->junit_path = argv[++i];
        }
        else if (0 == strcmp(arg, "--filter"))
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "error: --filter requires a pattern argument\n");
                return -1;
            }
            options->filter = argv[++i];
            if (options->mode == LCU_TEST_MODE_DEFAULT)
            {
                options->mode = LCU_TEST_MODE_FILTER;
            }
        }
        else if (arg[0] == '-')
        {
            fprintf(stderr, "error: unknown option '%s'\n", arg);
            return -1;
        }
        else
        {
            if (options->name_count >= LCU_TEST_MAX_NAMES)
            {
                fprintf(stderr, "error: too many test cases specified (max %d)\n", LCU_TEST_MAX_NAMES);
                return -1;
            }
            options->names[options->name_count++] = arg;
            if (options->mode == LCU_TEST_MODE_DEFAULT)
            {
                options->mode = LCU_TEST_MODE_BY_NAME;
            }
        }
    }

    return 0;
}

void lcu_test_print_help(const char *prog_name)
{
    fprintf(stderr,
        "Usage: %s [OPTIONS] [TEST_NAMES...]\n"
        "\n"
        "Options:\n"
        "  --help, -h          Show this help message\n"
        "  --list, -l          List all registered test cases (prints to stdout)\n"
        "  --all, -a           Run all default test cases\n"
        "  --filter <glob>     Run tests matching glob pattern (e.g. 'thread*')\n"
        "  --fail-fast, -f     Stop on first failure\n"
        "  --junit <file>      Write JUnit XML report to file\n"
        "\n"
        "Test selection:\n"
        "  %s ini_test thpool_test    Run by name\n"
        "  %s --all                   Run all default tests\n"
        "  %s                         tty: interactive menu; non-tty: smoke test\n"
        "\n"
        "Note: Numeric index (e.g. '0 5 8') is no longer supported since v1.9.0.\n"
        "      Use --list to see available test names.\n"
        "\n"
        "ASSERT_ABORT in test body will terminate the entire process immediately.\n"
        "Summary will NOT be printed in that case.\n"
        "\n",
        prog_name, prog_name, prog_name, prog_name);
}
