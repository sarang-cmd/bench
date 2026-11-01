/* pi.c - the digits of pi, and how far a guess got.  See pi.h. */

#include "pi.h"

#include <stddef.h>

/* 200 digits, written in 40s so a mistyped one is easy to spot against the
 * grouped output of the game.  Checked against an independent spigot
 * computation in tests/test_pi.c, not against a copy of itself. */
static char const digits[] =
    "31415926535897932384626433832795028841971693993751"
    "05820974944592307816406286208998628034825342117067"
    "98214808651328230664709384460955058223172535940812"
    "84811174502841027019385211055596446229489549303819";

int pi_digit_count(void)
{
    return (int)(sizeof digits - 1);
}

int pi_digit(int i)
{
    if (i < 0 || i >= pi_digit_count())
        return -1;
    return digits[i] - '0';
}

int pi_match(int from, char const *text, int len)
{
    int n = 0;

    if (!text || len < 0 || from < 0 || from >= pi_digit_count())
        return 0;
    for (int i = 0; i < len; i++)
    {
        int const want = pi_digit(from + i);

        if (want < 0 || text[i] != (char)('0' + want))
            break;
        n++;
    }
    return n;
}

int pi_text(int count, char *out, int size)
{
    int n = 0;
    int used = 0;

    if (!out || size <= 0)
        return -1;
    out[0] = 0;
    if (count < 1 || count > pi_digit_count())
        return -1;
    for (int i = 0; i < count; i++)
    {
        /* "3." then a space after every five decimals: how the digits are
         * grouped when someone reads them off a page. */
        if (i == 1)
        {
            if (n + 1 >= size)
                return -1;
            out[n++] = '.';
        }
        else if (i > 1 && (i - 1) % 5 == 0)
        {
            if (n + 1 >= size)
                return -1;
            out[n++] = ' ';
        }
        if (n + 1 >= size)
            return -1;
        out[n++] = digits[i];
        used++;
    }
    out[n] = 0;
    (void)used;
    return n;
}
