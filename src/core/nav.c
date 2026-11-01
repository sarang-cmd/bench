/* nav.c - the back stack.  See nav.h. */

#include "nav.h"

static int stack[NAV_DEPTH];
static int depth;

void nav_reset(void)
{
    depth = 0;
}

void nav_push(int screen)
{
    if (depth > 0 && stack[depth - 1] == screen)
        return;                 /* already the one EXIT would return to */
    if (depth == NAV_DEPTH)
    {
        /* Full: drop the oldest, so the most recent screens are the ones kept. */
        for (int i = 1; i < NAV_DEPTH; i++)
            stack[i - 1] = stack[i];
        depth--;
    }
    stack[depth++] = screen;
}

int nav_pop(void)
{
    if (depth == 0)
        return -1;
    return stack[--depth];
}

int nav_peek(void)
{
    if (depth == 0)
        return -1;
    return stack[depth - 1];
}

int nav_depth(void)
{
    return depth;
}
