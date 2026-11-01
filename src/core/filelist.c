/* filelist.c - see filelist.h. */

#include "filelist.h"

#include <stddef.h>

static char lower(char c)
{
    return (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c;
}

static int length(char const *s)
{
    int n = 0;

    if (!s)
        return 0;
    while (s[n])
        n++;
    return n;
}

int filelist_has_ext(char const *name, char const *ext)
{
    int ln = length(name);
    int lf = length(ext);

    if (lf == 0)
        return 1;
    if (ln < lf)
        return 0;
    for (int i = 0; i < lf; i++)
        if (lower(name[ln - lf + i]) != lower(ext[i]))
            return 0;
    return 1;
}

int filelist_name_ok(char const *name)
{
    if (!name || !name[0])
        return 0;
    if (name[0] == '.' && (name[1] == 0 || (name[1] == '.' && name[2] == 0)))
        return 0;
    for (int i = 0; name[i]; i++)
        if (name[i] == '\\' || name[i] == '/')
            return 0;
    return 1;
}

int filelist_ext_of(char const *name, char *out, int size)
{
    int ln = length(name);
    int dot = -1;
    int n = 0;

    if (!out || size <= 0)
        return -1;
    for (int i = 0; i < ln; i++)
        if (name[i] == '.')
            dot = i;
    /* A leading dot is a hidden name, not an extension. */
    if (dot <= 0)
    {
        out[0] = 0;
        return 0;
    }
    for (int i = dot + 1; i < ln && n < size - 1; i++)
        out[n++] = lower(name[i]);
    out[n] = 0;
    return n;
}

int filelist_glob(char const *ext, char *out, int size)
{
    int n = 0;

    if (!out || size < 2)
        return -1;
    out[n++] = '*';
    if (!ext || !ext[0])
    {
        if (n + 2 >= size)
            return -1;
        out[n++] = '.';
        out[n++] = '*';
    }
    else
    {
        int const lf = length(ext);

        if (n + lf >= size)
            return -1;
        for (int i = 0; i < lf; i++)
            out[n++] = ext[i];
    }
    out[n] = 0;
    return n;
}

int filelist_glob_alt(char const *ext, char *out, int size)
{
    /* An extension listing has already asked for the files it wants; the other
     * spelling that can work on CASIOWIN is the full wildcard, filtered again
     * by filelist_has_ext() on the way out. */
    if (ext && ext[0])
        return filelist_glob(NULL, out, size);
    if (!out || size < 2)
        return -1;
    out[0] = '*';
    out[1] = 0;
    return 1;
}
