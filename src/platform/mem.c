/* mem.c - allocations behind one small interface.  See mem.h. */

#include "mem.h"

#include <gint/kmalloc.h>

/* Order: the OS heap first (it is the biggest), then the two named arenas. */
static char const *const arenas[] = { NULL, "_uram", "_ostk" };

void *mem_alloc(size_t size)
{
    if (size == 0)
        return NULL;
    for (unsigned int i = 0; i < sizeof arenas / sizeof arenas[0]; i++)
    {
        void *p = kmalloc(size, arenas[i]);

        if (p)
            return p;
    }
    return NULL;
}

void mem_free(void *ptr)
{
    if (ptr)
        kfree(ptr);
}

size_t mem_largest(void)
{
    size_t best = 0;

    for (unsigned int i = 0; i < sizeof arenas / sizeof arenas[0]; i++)
    {
        size_t got = 0;
        void *p = kmalloc_max(&got, arenas[i]);

        if (got > best)
            best = got;
        if (p)
            kfree(p);
    }
    return best;
}
