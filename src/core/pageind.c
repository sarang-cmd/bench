/* pageind.c - see pageind.h. */

#include "pageind.h"

#include <stddef.h>

int pageind_label(int page, int pages, char *out, int size)
{
    char digits[24];
    int n = 0;
    int p;

    if (!out || size <= 0 || pages < 1 || page < 0 || page >= pages)
        return -1;

    /* Page numbers are small on this screen; two digits of headroom is plenty
     * and keeps the buffer arithmetic obvious. */
    p = page + 1;
    if (p >= 10)
        digits[n++] = (char)('0' + p / 10 % 10);
    digits[n++] = (char)('0' + p % 10);
    digits[n++] = ' ';
    digits[n++] = '/';
    digits[n++] = ' ';

    p = pages;
    if (p >= 10)
        digits[n++] = (char)('0' + p / 10 % 10);
    digits[n++] = (char)('0' + p % 10);

    if (n >= size)
        return -1;
    for (int i = 0; i < n; i++)
        out[i] = digits[i];
    out[n] = 0;
    return n;
}

int pageind_left(int page)
{
    return page > 0;
}

int pageind_right(int page, int pages)
{
    return page + 1 < pages;
}

int pageind_step(int page, int pages, int dir)
{
    if (pages < 1)
        return 0;
    if (page < 0)
        page = 0;
    if (page >= pages)
        page = pages - 1;
    if (dir > 0 && page + 1 < pages)
        page++;
    else if (dir < 0 && page > 0)
        page--;
    return page;
}
