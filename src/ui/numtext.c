/* numtext.c - see numtext.h. */

#include "numtext.h"

#include <string.h>

#include "core/numfmt.h"

void nt_append(char *dst, int size, char const *src)
{
    int n = (int)strlen(dst);
    int room;

    if (!dst || size <= 0 || !src)
        return;
    room = size - 1 - n;
    if (room <= 0)
        return;
    while (*src && room-- > 0)
        dst[n++] = *src++;
    dst[n] = 0;
}

void nt_u64(uint64_t v, char *buf, int size)
{
    char tmp[21];
    int n = 0;
    int i = 0;

    if (!buf || size <= 0)
        return;
    if (v == 0)
    {
        if (size > 1)
        {
            buf[0] = '0';
            buf[1] = 0;
        }
        else
            buf[0] = 0;
        return;
    }
    while (v > 0 && n < (int)sizeof tmp)
    {
        tmp[n++] = (char)('0' + (int)(v % 10u));
        v /= 10u;
    }
    while (n > 0 && i < size - 1)
        buf[i++] = tmp[--n];
    buf[i] = 0;
}

void nt_i64(int64_t v, char *buf, int size)
{
    if (!buf || size <= 0)
        return;
    if (v < 0)
    {
        if (size < 2)
        {
            buf[0] = 0;
            return;
        }
        buf[0] = '-';
        nt_u64((uint64_t)(-(v + 1)) + 1u, buf + 1, size - 1);
        return;
    }
    nt_u64((uint64_t)v, buf, size);
}

void nt_fixed(double v, int decimals, char *buf, int size)
{
    if (numfmt_format_fixed(v, decimals, buf, size) < 0 && size > 0)
        buf[0] = 0;
}

void nt_sig(double v, int sig, char *buf, int size)
{
    if (numfmt_format_sig(v, sig, buf, size) < 0 && size > 0)
        buf[0] = 0;
}

void nt_fit(double v, int max_dec, int max_width, char *buf, int size)
{
    numfmt_dec_t d;

    if (numfmt_dec_from_double(v, NUMFMT_DEFAULT_SIG, &d) != NUMFMT_OK)
    {
        if (size > 0)
            buf[0] = 0;
        return;
    }
    /* render_fit returns the length written, or a negative NUMFMT_ERR_* code. */
    if (numfmt_dec_render_fit(&d, 0, max_dec, max_width, buf, size) < 0 &&
        size > 0)
        buf[0] = 0;
}

void nt_clock(double minutes, char *buf, int size)
{
    int total_seconds;
    int hours;
    int mins;
    int secs;
    char tmp[8];

    if (!buf || size <= 0)
        return;
    buf[0] = 0;
    if (!(minutes > 0.0))
        minutes = 0.0;
    if (minutes > 100000.0)
        minutes = 100000.0;
    total_seconds = (int)(minutes * 60.0 + 0.5);
    hours = total_seconds / 3600;
    mins = (total_seconds / 60) % 60;
    secs = total_seconds % 60;

    if (hours > 0)
    {
        nt_u64((uint64_t)hours, tmp, (int)sizeof tmp);
        nt_append(buf, size, tmp);
        nt_append(buf, size, ":");
    }
    tmp[0] = (char)('0' + (mins / 10) % 10);
    tmp[1] = (char)('0' + mins % 10);
    tmp[2] = 0;
    if (hours > 0)
        nt_append(buf, size, tmp);
    else
    {
        /* Plain minutes without padding, so "5:07" reads naturally. */
        nt_u64((uint64_t)mins, tmp, (int)sizeof tmp);
        nt_append(buf, size, tmp);
    }
    nt_append(buf, size, ":");
    tmp[0] = (char)('0' + (secs / 10) % 10);
    tmp[1] = (char)('0' + secs % 10);
    tmp[2] = 0;
    nt_append(buf, size, tmp);
}

void nt_duration(double minutes, char *buf, int size)
{
    int total;
    char num[24];

    if (!buf || size <= 0)
        return;
    buf[0] = 0;
    if (!(minutes > 0.0))
        minutes = 0.0;
    if (minutes > 100000.0)
        minutes = 100000.0;
    total = (int)(minutes * 60.0 + 0.5);

    /* One shape everywhere: "11m15s" under an hour, "1h05m" above. */
    if (total >= 3600)
    {
        int h = total / 3600;
        int m = (total / 60) % 60;

        nt_u64((uint64_t)h, num, (int)sizeof num);
        nt_append(buf, size, num);
        nt_append(buf, size, "h");
        num[0] = (char)('0' + (m / 10) % 10);
        num[1] = (char)('0' + m % 10);
        num[2] = 0;
        nt_append(buf, size, num);
        nt_append(buf, size, "m");
        return;
    }
    {
        int m = total / 60;
        int s = total % 60;

        nt_u64((uint64_t)m, num, (int)sizeof num);
        nt_append(buf, size, num);
        nt_append(buf, size, "m");
        num[0] = (char)('0' + (s / 10) % 10);
        num[1] = (char)('0' + s % 10);
        num[2] = 0;
        nt_append(buf, size, num);
        nt_append(buf, size, "s");
    }
}

void nt_factors(uint64_t const *factors, uint8_t const *exponents, int count,
                char *buf, int size)
{
    char num[24];

    if (!buf || size <= 0)
        return;
    buf[0] = 0;
    if (count <= 0)
    {
        nt_append(buf, size, "1");
        return;
    }
    if (count == 1 && exponents && exponents[0] == 1)
    {
        nt_u64(factors[0], num, (int)sizeof num);
        nt_append(buf, size, num);
        if (factors[0] == 0 || factors[0] == 1)
            return;
        return;
    }
    for (int i = 0; i < count; i++)
    {
        nt_u64(factors[i], num, (int)sizeof num);
        if (i)
            nt_append(buf, size, " x ");
        nt_append(buf, size, num);
        if (exponents && exponents[i] > 1)
        {
            nt_append(buf, size, "^");
            nt_u64((uint64_t)exponents[i], num, (int)sizeof num);
            nt_append(buf, size, num);
        }
    }
}
