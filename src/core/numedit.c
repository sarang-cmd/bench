/* numedit.c - see numedit.h. */

#include "numedit.h"

#include <string.h>

static int length_of(char const *buf, int size)
{
    int n = 0;

    while (n < size - 1 && buf[n])
        n++;
    return n;
}

bool numedit_put(char *buf, int size, int ch)
{
    int n;
    bool has_dot = false;

    if (!buf || size < 2)
        return false;
    n = length_of(buf, size);
    if (n >= size - 1)
        return false;

    if (ch == '.')
    {
        if (n == 0)
        {
            /* ".5" is fine, but make it "0.5" so the field reads as one number. */
            buf[0] = '0';
            buf[1] = '.';
            buf[2] = 0;
            return true;
        }
        for (int i = 0; i < n; i++)
            if (buf[i] == '.')
                has_dot = true;
        if (has_dot)
            return false;
        buf[n] = '.';
        buf[n + 1] = 0;
        return true;
    }

    if (ch < '0' || ch > '9')
        return false;

    /* A lone zero is replaced, so 0 then 5 is 5 rather than 05. */
    if (n == 1 && buf[0] == '0')
    {
        buf[0] = (char)ch;
        return true;
    }
    buf[n] = (char)ch;
    buf[n + 1] = 0;
    return true;
}

bool numedit_back(char *buf)
{
    int n;

    if (!buf)
        return false;
    n = (int)strlen(buf);
    if (n == 0)
        return false;
    buf[n - 1] = 0;
    return true;
}

void numedit_set(char *buf, int size, char const *text)
{
    int i = 0;

    if (!buf || size < 1)
        return;
    if (text)
        for (; i < size - 1 && text[i]; i++)
            buf[i] = text[i];
    buf[i] = 0;
}

bool numedit_valid(char const *buf)
{
    int n, digits = 0, dots = 0;

    if (!buf)
        return false;
    n = (int)strlen(buf);
    if (n == 0)
        return false;
    for (int i = 0; i < n; i++)
    {
        if (buf[i] == '-' && i == 0)
            continue;
        if (buf[i] == '.')
        {
            dots++;
            continue;
        }
        if (buf[i] < '0' || buf[i] > '9')
            return false;
        digits++;
    }
    return digits > 0 && dots <= 1;
}
