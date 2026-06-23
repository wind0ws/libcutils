#include "mem/mem_debug.h"
#include "lcu_test_registry.h"
#include <string.h>

static lcu_test_entry_t *s_head = NULL;
static lcu_test_entry_t *s_tail = NULL;
static size_t s_count = 0;

void lcu_test_registry_link(lcu_test_entry_t *entry)
{
    if (!entry)
    {
        return;
    }
    entry->next = NULL;
    if (s_tail)
    {
        s_tail->next = entry;
    }
    else
    {
        s_head = entry;
    }
    s_tail = entry;
    ++s_count;
}

size_t lcu_test_registry_count(void)
{
    return s_count;
}

const lcu_test_entry_t *lcu_test_registry_head(void)
{
    return s_head;
}

const lcu_test_entry_t *lcu_test_registry_get(size_t idx)
{
    if (idx >= s_count)
    {
        return NULL;
    }
    const lcu_test_entry_t *cur = s_head;
    for (size_t i = 0; i < idx && cur; ++i)
    {
        cur = cur->next;
    }
    return cur;
}

const lcu_test_entry_t *lcu_test_registry_find_by_name(const char *name)
{
    if (!name || name[0] == '\0')
    {
        return NULL;
    }

    const lcu_test_entry_t *cur = s_head;
    while (cur)
    {
        if (0 == strcmp(cur->name, name))
        {
            return cur;
        }
        cur = cur->next;
    }

    size_t len = strlen(name);
    if (len > 120)
    {
        return NULL;
    }
    char buf[128];
    memcpy(buf, name, len);
    memcpy(buf + len, "_test", 6);

    cur = s_head;
    while (cur)
    {
        if (0 == strcmp(cur->name, buf))
        {
            return cur;
        }
        cur = cur->next;
    }
    return NULL;
}
