#include "mem/mem_debug.h"
#include "lcu_test_glob.h"
#include <string.h>
#include <stddef.h>

#define LOG_TAG "GLOB_TEST"
#include "log/logger.h"

static void fold_stars(const char *pattern, char *out, size_t out_size)
{
    size_t j = 0;
    bool last_was_star = false;
    for (size_t i = 0; pattern[i] && j < out_size - 1; ++i)
    {
        if (pattern[i] == '*')
        {
            if (!last_was_star)
            {
                out[j++] = '*';
                last_was_star = true;
            }
        }
        else
        {
            out[j++] = pattern[i];
            last_was_star = false;
        }
    }
    out[j] = '\0';
}

bool lcu_glob_match(const char *pattern, const char *str)
{
    char folded[256];
    fold_stars(pattern, folded, sizeof(folded));
    const char *p = folded;
    const char *s = str;

    while (*p && *s)
    {
        if (*p == '*')
        {
            ++p;
            if (*p == '\0')
            {
                return true;
            }
            while (*s)
            {
                if (lcu_glob_match(p, s))
                {
                    return true;
                }
                ++s;
            }
            return *p == '\0';
        }
        else if (*p == '?' || *p == *s)
        {
            ++p;
            ++s;
        }
        else
        {
            return false;
        }
    }
    while (*p == '*')
    {
        ++p;
    }
    return (*p == '\0' && *s == '\0');
}

/* ======================== glob_match_test 单测 ======================== */

#include "lcu_test_registry.h"

int glob_match_test(void);

int glob_match_test(void)
{
    struct
    {
        const char *pat;
        const char *str;
        bool expect;
    } cases[] = {
        {"", "", true},
        {"*", "", true},
        {"*", "anything", true},
        {"**", "foo", true},
        {"*foo*bar*", "xfooyybarz", true},
        {"foo?bar", "fooXbar", true},
        {"foo?bar", "foobar", false},
        {"test", "test_long", false},
    };
    size_t total = sizeof(cases) / sizeof(cases[0]);
    int failed = 0;
    for (size_t i = 0; i < total; ++i)
    {
        bool got = lcu_glob_match(cases[i].pat, cases[i].str);
        if (got != cases[i].expect)
        {
            LOGE("case %zu failed: pattern='%s' str='%s' expect=%d got=%d",
                 i, cases[i].pat, cases[i].str, (int)cases[i].expect, (int)got);
            ++failed;
        }
    }
    if (failed)
    {
        return -1;
    }
    LOGI("glob_match_test: all %zu cases passed", total);
    return 0;
}

LCU_TEST_REGISTER(glob_match_test, "test glob pattern matching");
