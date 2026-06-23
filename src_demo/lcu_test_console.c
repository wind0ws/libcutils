#include "mem/mem_debug.h"
#include "lcu_test_console.h"
#include "lcu_test_args.h"
#include "thread/posix_thread.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <locale.h>

#define LOG_TAG "CONSOLE"
#include "log/logger.h"

#ifdef _WIN32
#include <conio.h>
#else
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#endif

void lcu_console_setup(void)
{
#ifdef _WIN32
    setlocale(LC_CTYPE, ".utf8");
#endif
    LOG_GLOBAL_INIT(NULL);
    LOG_SET_MIN_LEVEL(LOG_LEVEL_VERBOSE);
#if (_LCU_LOGGER_TYPE_XLOG == LCU_LOGGER_SELECTOR)
    xlog_set_format(LOG_FORMAT_WITH_TIMESTAMP | LOG_FORMAT_WITH_TAG_LEVEL | LOG_FORMAT_WITH_TID);
#endif
}

bool lcu_console_wait_interrupt(int timeout_seconds)
{
    if (timeout_seconds < 0)
    {
        timeout_seconds = 0;
    }
#ifdef _WIN32
    LOGI("after %d seconds, it will run automatically. press any key for menu", timeout_seconds);
    if (timeout_seconds == 0)
    {
        if (_kbhit())
        {
            (void)_getch();
            return true;
        }
        return false;
    }
    clock_t tstart = clock();
    while ((clock() - tstart) / CLOCKS_PER_SEC < timeout_seconds)
    {
        if (_kbhit())
        {
            (void)_getch();
            return true;
        }
        usleep(50000);
    }
    return false;
#else
    if (!isatty(STDIN_FILENO))
    {
        return false;
    }
    LOGI("after %d seconds, it will run automatically. press any key for menu", timeout_seconds);
    struct termios old_term;
    if (tcgetattr(STDIN_FILENO, &old_term) != 0)
    {
        return false;
    }
    struct termios new_term = old_term;
    new_term.c_lflag &= ~(ICANON | ECHO);
    if (tcsetattr(STDIN_FILENO, TCSANOW, &new_term) != 0)
    {
        return false;
    }
    int old_flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (old_flags < 0)
    {
        (void)tcsetattr(STDIN_FILENO, TCSANOW, &old_term);
        return false;
    }
    if (fcntl(STDIN_FILENO, F_SETFL, old_flags | O_NONBLOCK) != 0)
    {
        (void)tcsetattr(STDIN_FILENO, TCSANOW, &old_term);
        return false;
    }
    struct timeval tv;
    tv.tv_sec = timeout_seconds;
    tv.tv_usec = 0;
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);
    int res = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv);
    bool pressed = false;
    if (res > 0 && FD_ISSET(STDIN_FILENO, &readfds))
    {
        char ch;
        if (read(STDIN_FILENO, &ch, 1) >= 0)
        {
            pressed = true;
        }
    }
    (void)fcntl(STDIN_FILENO, F_SETFL, old_flags);
    (void)tcsetattr(STDIN_FILENO, TCSANOW, &old_term);
    return pressed;
#endif
}

void lcu_console_show_menu(void)
{
    fprintf(stderr, "input test case names (split by space), press enter to submit:\n");
    size_t i = 0;
    for (const lcu_test_entry_t *e = lcu_test_registry_head(); e; e = e->next, ++i)
    {
        fprintf(stderr, "  %02zu : [%s]%s %s\n",
                i, e->name,
                e->exclude_from_all ? " (opt-in)" : "",
                e->description);
    }
    fprintf(stderr, "\n");
}

int lcu_console_run_from_menu(lcu_test_runner_fn runner,
                              bool fail_fast,
                              lcu_test_stats_t *stats)
{
    if (!runner)
    {
        return -1;
    }
    lcu_console_show_menu();
    char buffer[1024] = {0};
    fprintf(stderr, "please input test names: ");
    if (!fgets(buffer, sizeof(buffer) - 1, stdin))
    {
        fprintf(stderr, "failed to read input\n");
        return -1;
    }

    const lcu_test_entry_t *selected[64];
    int count = 0;
    char *cursor = buffer;
    while (*cursor && count < (int)(sizeof(selected) / sizeof(selected[0])))
    {
        while (*cursor == ' ' || *cursor == '\n' || *cursor == '\r' || *cursor == '\t')
        {
            ++cursor;
        }
        if (*cursor == '\0')
        {
            break;
        }
        char *end = cursor;
        while (*end && *end != ' ' && *end != '\n' && *end != '\r' && *end != '\t')
        {
            ++end;
        }
        char saved = *end;
        *end = '\0';
        const lcu_test_entry_t *e = lcu_test_registry_find_by_name(cursor);
        if (!e)
        {
            fprintf(stderr, "unknown test: '%s' (use --list to see available names)\n", cursor);
            return -1;
        }
        selected[count++] = e;
        *end = saved;
        cursor = end;
    }

    if (count == 0)
    {
        fprintf(stderr, "no test selected\n");
        return -1;
    }
    return runner(selected, count, fail_fast, stats);
}
