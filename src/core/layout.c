/* layout.c - see layout.h. */

#include "layout.h"

int layout_centre_row(int tiles, int item, int gap, int span)
{
    int used;
    int off;

    if (tiles <= 0 || item <= 0)
        return 0;
    used = tiles * item + (tiles - 1) * gap;
    if (used >= span)
        return 0;
    off = (span - used) / 2;
    return off < 0 ? 0 : off;
}

int layout_row_count(int first, int total, int per_row)
{
    int left;

    if (per_row <= 0 || first < 0 || first >= total)
        return 0;
    left = total - first;
    return left > per_row ? per_row : left;
}
