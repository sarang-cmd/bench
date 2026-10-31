/* sigfig.c - see sigfig.h. */

#include "sigfig.h"

#include <string.h>

#include "numfmt.h"

/* Skip the sign, and copy the mantissa digits into `digits`.  Returns the
 * number of bytes consumed (up to the exponent), or -1. */
static int mantissa(char const *text, char *digits, int *ndigits,
                    int *point_at, bool *has_point, bool *has_nonzero)
{
    int i = 0;
    int n = 0;
    bool point = false;
    bool nonzero = false;

    *point_at = -1;
    if (text[i] == '+' || text[i] == '-')
        i++;
    while (text[i])
    {
        char const c = text[i];

        if (c == '.')
        {
            if (point)
                return -1;
            point = true;
            *point_at = n;
            i++;
            continue;
        }
        if (c >= '0' && c <= '9')
        {
            if (n >= NUMFMT_DIGITS)
                return -1;
            digits[n++] = c;
            if (c != '0')
                nonzero = true;
            i++;
            continue;
        }
        break;
    }
    if (n == 0)
        return -1;
    if (!point)
        *point_at = n;
    *ndigits = n;
    *has_point = point;
    *has_nonzero = nonzero;
    /* Anything after the mantissa must be an exponent. */
    if (text[i] == 'e' || text[i] == 'E')
    {
        int j = i + 1;
        bool digit = false;

        if (text[j] == '+' || text[j] == '-')
            j++;
        while (text[j] >= '0' && text[j] <= '9')
        {
            digit = true;
            j++;
        }
        if (!digit)
            return -1;
        i = j;
    }
    while (text[i] == ' ')
        i++;
    if (text[i])
        return -1;
    return i;
}

int sf_count_text(char const *text)
{
    char digits[NUMFMT_DIGITS];
    int n = 0, point_at = 0;
    bool has_point = false, has_nonzero = false;
    int first = -1;
    int count;

    if (!text)
        return -1;
    if (mantissa(text, digits, &n, &point_at, &has_point, &has_nonzero) < 0)
        return -1;
    if (!has_nonzero)
        return 1;   /* 0, 0.00: one figure, the convention every syllabus uses */

    for (int i = 0; i < n; i++)
        if (digits[i] != '0')
        {
            first = i;
            break;
        }
    count = n - first;
    /* Trailing zeros after the decimal point are significant; before it they
     * are placeholders: 1200 is two figures unless it was written 1200. */
    if (!has_point)
        while (count > 1 && digits[first + count - 1] == '0')
            count--;
    return count;
}

int sf_standard(double value, int sig, char *buf, int size)
{
    numfmt_dec_t d;
    char sci[32];
    char const *e;
    int len = 0;

    if (size < 12)
        return NUMFMT_ERR_SIZE;
    if (numfmt_dec_from_double(value, NUMFMT_DEFAULT_SIG, &d) != NUMFMT_OK)
        return NUMFMT_ERR_ARG;
    if (numfmt_dec_is_zero(&d))
    {
        strcpy(buf, "0");
        return 1;
    }
    if (numfmt_dec_round_sig(&d, sig) != NUMFMT_OK)
        return NUMFMT_ERR_ARG;

    /* numfmt already writes "4.56e-3"; this turns the exponent into the form a
     * student writes on paper. */
    if (numfmt_dec_render_sci(&d, sig, sci, (int)sizeof sci) < 0)
        return NUMFMT_ERR_ARG;
    e = strchr(sci, 'e');
    if (!e)
    {
        /* numfmt leaves values 1..999 without an exponent; standard form
         * always shows one. */
        if ((int)strlen(sci) + 10 >= size)
            return NUMFMT_ERR_SIZE;
        strcpy(buf, sci);
        strcat(buf, " x 10^0");
        return (int)strlen(buf);
    }
    for (char const *p = sci; p < e; p++)
        buf[len++] = *p;
    buf[len] = 0;
    /* " x 10^" and the exponent, minus the 'e'. */
    if (len + 3 + (int)strlen(e + 1) + 4 >= size)
        return NUMFMT_ERR_SIZE;
    strcat(buf, " x 10^");
    strcat(buf, e + 1);
    return (int)strlen(buf);
}

char const *sf_si_prefix(int power)
{
    switch (power)
    {
    case -12: return "p";
    case -9: return "n";
    case -6: return "u";
    case -3: return "m";
    case 0: return "";
    case 3: return "k";
    case 6: return "M";
    case 9: return "G";
    case 12: return "T";
    }
    return "";
}

int sf_si_power(double value)
{
    double v = value < 0.0 ? -value : value;
    int p = 0;

    if (v == 0.0)
        return 0;
    while (v >= 1000.0 && p < 12)
    {
        v /= 1000.0;
        p += 3;
    }
    while (v < 1.0 && p > -12)
    {
        v *= 1000.0;
        p -= 3;
    }
    return p;
}

int sf_si(double value, int sig, char *buf, int size)
{
    int const p = sf_si_power(value);
    double scaled = value;
    char num[32];
    int len;

    for (int i = 0; i < (p < 0 ? -p : p) / 3; i++)
        scaled = p < 0 ? scaled * 1000.0 : scaled / 1000.0;
    if (numfmt_format_sig(scaled, sig, num, (int)sizeof num) < 0)
        return NUMFMT_ERR_ARG;
    len = (int)strlen(num);
    if (len + 4 >= size)
        return NUMFMT_ERR_SIZE;
    strcpy(buf, num);
    strcat(buf, " ");
    strcat(buf, sf_si_prefix(p));
    return (int)strlen(buf);
}
