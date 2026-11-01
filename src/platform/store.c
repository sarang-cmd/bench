/* store.c - filesystem access for Bench.  See store.h.
 *
 * Every BFile call runs inside gint_world_switch(), because gint's drivers are
 * inactive while the OS owns the hardware (gint/bfile.h, top comment).  The
 * call's argument lives in a struct that outlives the switch. */

#include "store.h"

#include <gint/bfile.h>
#include <gint/gint.h>

#include <string.h>

#include "core/filelist.h"

#define PATH_MAX_U16 128

static uint16_t path_buf[PATH_MAX_U16];
static int path_len = 0;

/* ---- path building ------------------------------------------------------ */

static int append_ascii(uint16_t *dst, int at, char const *s)
{
    while (*s && at < PATH_MAX_U16 - 1)
    {
        char c = *s++;
        dst[at++] = (uint16_t)(c == '/' ? '\\' : c);
    }
    dst[at] = 0;
    return at;
}

/* Build \\fls0\Bench\<rel> into path_buf.  `rel` may be NULL or "" for the
 * Bench folder itself. */
static int build_path(char const *rel)
{
    path_len = append_ascii(path_buf, 0, "\\\\fls0\\Bench");
    if (rel && *rel)
    {
        path_len = append_ascii(path_buf, path_len, "\\");
        path_len = append_ascii(path_buf, path_len, rel);
    }
    return path_len;
}

/* ---- the world-switched bodies ------------------------------------------ */

typedef struct {
    uint16_t const *path;
    int type;            /* BFile_File / BFile_Folder */
    int size;
    int result;
} create_args_t;

static int body_create(void *arg)
{
    create_args_t *a = arg;
    int size = a->size;

    return BFile_Create(a->path, a->type, a->type == BFile_File ? &size : NULL);
}

typedef struct {
    uint16_t const *path;
    char *buf;
    int size;
    int len;
} read_args_t;

static int body_read(void *arg)
{
    read_args_t *a = arg;
    int fd;
    int size;
    int total = 0;

    fd = BFile_Open(a->path, BFile_ReadOnly);
    if (fd < 0)
        return STORE_ERR_OPEN;
    /* The OS initialises a new file with 0xFF and only the bytes written are
     * real; BFile_Size() reports the allocated size.  Trim at the first 0xFF
     * and at a NUL, which is what a text file can never contain. */
    size = BFile_Size(fd);
    if (size < 0)
    {
        BFile_Close(fd);
        return STORE_ERR_READ;
    }
    if (size > a->size)
    {
        BFile_Close(fd);
        a->len = size;
        return STORE_ERR_TOO_BIG;
    }
    while (total < size)
    {
        int n = BFile_Read(fd, a->buf + total, size - total, -1);

        /* With CASIOWIN the return value is "bytes readable", with Fugue it is
         * "bytes read".  Either way a non-positive value ends the loop. */
        if (n <= 0)
            break;
        total += (n > size - total) ? (size - total) : n;
    }
    BFile_Close(fd);
    /* Trim the padding. */
    {
        int end = total;

        while (end > 0 && (a->buf[end - 1] == (char)0xFF || a->buf[end - 1] == 0))
            end--;
        total = end;
    }
    a->len = total;
    return STORE_OK;
}

typedef struct {
    uint16_t const *path;
    char const *data;
    int len;
} write_args_t;

static int body_write(void *arg)
{
    write_args_t *a = arg;
    int size = a->len;
    int fd;
    int w;

    BFile_Remove(a->path);          /* a second write needs a fresh file */
    if (BFile_Create(a->path, BFile_File, &size) < 0)
        return STORE_ERR_WRITE;

    fd = BFile_Open(a->path, BFile_WriteOnly);
    if (fd < 0)
        return STORE_ERR_OPEN;

    /* Even-sized writes only (CASIOWIN requirement).  store_write() already
     * padded the data, so `len` is even here. */
    w = BFile_Write(fd, a->data, a->len);
    BFile_Close(fd);
    return w < 0 ? STORE_ERR_WRITE : STORE_OK;
}

typedef struct {
    uint16_t const *from;
    uint16_t const *to;
} rename_args_t;

static int body_rename(void *arg)
{
    rename_args_t *a = arg;

    BFile_Remove(a->to);
    return BFile_Rename(a->from, a->to) < 0 ? STORE_ERR_WRITE : STORE_OK;
}

static int body_remove(void *arg)
{
    uint16_t const *path = arg;

    BFile_Remove(path);
    return STORE_OK;
}

typedef struct {
    uint16_t const *path;
    int size;
} size_args_t;

static int body_size(void *arg)
{
    size_args_t *a = arg;
    int fd = BFile_Open(a->path, BFile_ReadOnly);
    int size;

    if (fd < 0)
        return -1;
    size = BFile_Size(fd);
    BFile_Close(fd);
    a->size = size;
    return size;
}

typedef struct {
    uint16_t const *glob;
    store_entry_t *out;
    int max;
    int count;
    char const *ext;
} list_args_t;

/* The two wildcard spellings a listing tries.  CASIOWIN's BFile_FindFirst
 * returns nothing for a bare "*" on some OS versions, so a folder that looks
 * empty is listed again as "*.*" and filtered in C. */
typedef struct {
    uint16_t glob[2][PATH_MAX_U16];
    int globs;
} glob_pair_t;

/* BFile_FindFirst/FindNext write the *full path* of the match into this buffer,
 * not just the name, and gint's bfile.h asks for 262 FONTCHARACTER of room.
 * A shorter buffer worked on the host preview, where names are short, and is
 * memory corruption on the calculator - which is exactly the kind of thing that
 * shows up as "it cannot see my files" and nowhere else. */
#define FIND_NAME_MAX 264

typedef struct {
    struct BFile_FileInfo info;
    uint16_t name[FIND_NAME_MAX];
} find_rec_t;

static void base_name(uint16_t const *path, char *out, int size)
{
    int start = 0;
    int i = 0;

    while (path[i])
    {
        if (path[i] == '\\')
            start = i + 1;
        i++;
    }
    i = 0;
    while (path[start + i] && i < size - 1)
    {
        unsigned int c = path[start + i];

        out[i] = (c >= 32 && c < 127) ? (char)c : '?';
        i++;
    }
    out[i] = 0;
}

static find_rec_t find_rec;

static int body_list(void *arg)
{
    list_args_t *a = arg;
    int shandle = 0;
    int rc;
    int count = 0;
    bool more;

    rc = BFile_FindFirst(a->glob, &shandle, find_rec.name, &find_rec.info);
    more = rc == 0;
    while (more)
    {
        if (find_rec.info.type == BFile_File && count < a->max)
        {
            char name[STORE_NAME_MAX];

            base_name(find_rec.name, name, (int)sizeof name);
            /* The rules are in core/filelist.c, where they are tested: the
             * name has to be a real name, and the extension match ignores case
             * so a Windows copy saved as ".MD" still appears. */
            if (filelist_name_ok(name) && filelist_has_ext(name, a->ext))
            {
                strncpy(a->out[count].name, name, STORE_NAME_MAX - 1);
                a->out[count].name[STORE_NAME_MAX - 1] = 0;
                a->out[count].size = (int)find_rec.info.file_size;
                count++;
            }
        }
        more = BFile_FindNext(shandle, find_rec.name, &find_rec.info) == 0;
    }
    BFile_FindClose(shandle);
    a->count = count;
    return count;
}

/* ---- public API --------------------------------------------------------- */

int store_init(void)
{
    static char const *folders[] = { "", "notes", "cards", "state", "study",
                                     "formulas" };

    for (unsigned int i = 0; i < sizeof folders / sizeof folders[0]; i++)
    {
        create_args_t a;

        build_path(folders[i]);
        a.path = path_buf;
        a.type = BFile_Folder;
        a.size = 0;
        a.result = 0;
        gint_world_switch(GINT_CALL(body_create, (void *)&a));
    }
    return STORE_OK;
}

typedef struct {
    uint16_t const *path;
} exists_args_t;

static int body_exists(void *arg)
{
    exists_args_t *a = arg;
    int fd = BFile_Open(a->path, BFile_ReadOnly);

    if (fd < 0)
        return 0;
    BFile_Close(fd);
    return 1;
}

bool store_exists(char const *rel)
{
    exists_args_t a;

    if (!rel || !*rel)
        return false;
    build_path(rel);
    a.path = path_buf;
    return gint_world_switch(GINT_CALL(body_exists, (void *)&a)) > 0;
}

int store_read(char const *rel, char *buf, int size, int *len)
{
    read_args_t a;

    if (len)
        *len = 0;
    if (!rel || !buf || size <= 0)
        return STORE_ERR_PATH;

    build_path(rel);
    a.path = path_buf;
    a.buf = buf;
    a.size = size;
    a.len = 0;
    int rc = gint_world_switch(GINT_CALL(body_read, (void *)&a));
    if (len)
        *len = a.len;
    return rc;
}

int store_size(char const *rel)
{
    size_args_t a;

    if (!rel)
        return -1;
    build_path(rel);
    a.path = path_buf;
    a.size = -1;
    return gint_world_switch(GINT_CALL(body_size, (void *)&a));
}

int store_write(char const *rel, char const *data, int len)
{
    char tmp_path[STORE_NAME_MAX + 8];
    static char padded[8192];
    write_args_t a;
    int n;

    if (!rel || !data || len < 0)
        return STORE_ERR_PATH;
    if (len + 1 > (int)sizeof padded)
        return STORE_ERR_TOO_BIG;

    for (int i = 0; i < len; i++)
        padded[i] = data[i];
    n = len;
    if (n & 1)
        padded[n++] = '\n';        /* even-sized writes, see store.h */

    /* 1. write "<rel>.tmp" */
    if ((int)strlen(rel) + 5 >= (int)sizeof tmp_path)
        return STORE_ERR_PATH;
    strcpy(tmp_path, rel);
    strcat(tmp_path, ".tmp");

    build_path(tmp_path);
    a.path = path_buf;
    a.data = padded;
    a.len = n;
    int rc = gint_world_switch(GINT_CALL(body_write, (void *)&a));
    if (rc != STORE_OK)
        return rc;

    /* 2. rename it over the real file */
    {
        static uint16_t to_buf[PATH_MAX_U16];
        rename_args_t r;

        build_path(rel);
        memcpy(to_buf, path_buf, (size_t)(path_len + 1) * sizeof(uint16_t));

        build_path(tmp_path);
        r.from = path_buf;
        r.to = to_buf;
        return gint_world_switch(GINT_CALL(body_rename, (void *)&r));
    }
}

int store_remove(char const *rel)
{
    if (!rel)
        return STORE_ERR_PATH;
    build_path(rel);
    return gint_world_switch(GINT_CALL(body_remove, (void *)path_buf));
}

int store_join(char *buf, int size, char const *folder, char const *name)
{
    int n = 0;

    if (!buf || size <= 1 || !folder || !name)
        return STORE_ERR_PATH;
    while (*folder && n < size - 1)
        buf[n++] = *folder++;
    if (n < size - 1)
        buf[n++] = '/';
    while (*name && n < size - 1)
        buf[n++] = *name++;
    buf[n] = 0;
    return n >= size - 1 ? STORE_ERR_PATH : STORE_OK;
}

/* The roots, in the order they are tried.  All three are relative to the drive
 * the calculator shows as a letter in USB Flash mode. */
static char const *root_path(store_location_t at)
{
    switch (at)
    {
        case STORE_AT_SAVEF: return "\\\\fls0\\SAVE-F\\Bench";
        case STORE_AT_ROOT:
        case STORE_AT_LOOSE: return "\\\\fls0";
        default:             return "\\\\fls0\\Bench";
    }
}

/* A "loose" place has the files directly in it, with no folder of their own:
 * someone who copies `photosynthesis.md` onto D:\\ rather than into
 * D:\\Bench\\notes should still see it in Notes. */
static bool location_is_flat(store_location_t at)
{
    return at == STORE_AT_LOOSE;
}

static int list_from(store_location_t at, char const *folder, char const *ext,
                     store_entry_t *out, int max)
{
    char const *root = root_path(at);
    static glob_pair_t globs;
    list_args_t a;
    static store_entry_t tmp[STORE_LIST_MAX];
    char tail[16], alt[16];
    int count = 0;

    if (!folder || !out || max <= 0)
        return STORE_ERR_PATH;
    if (max > STORE_LIST_MAX)
        max = STORE_LIST_MAX;
    if (filelist_glob(ext, tail, (int)sizeof tail) < 0)
        return STORE_ERR_PATH;

    /* \\fls0\Bench\<folder>\<glob>, then the same with the other spelling. */
    globs.globs = 0;
    for (int pass = 0; pass < 2; pass++)
    {
        char const *pattern = tail;

        if (pass == 1)
        {
            if (filelist_glob_alt(ext, alt, (int)sizeof alt) < 0)
                break;
            if (strcmp(alt, tail) == 0)
                break;          /* same wildcard twice is a waste of a scan */
            pattern = alt;
        }

        path_len = append_ascii(globs.glob[globs.globs], 0, root);
        path_len = append_ascii(globs.glob[globs.globs], path_len, "\\");
        if (!location_is_flat(at))
        {
            path_len = append_ascii(globs.glob[globs.globs], path_len, folder);
            path_len = append_ascii(globs.glob[globs.globs], path_len, "\\");
        }
        append_ascii(globs.glob[globs.globs], path_len, pattern);
        globs.globs++;
    }

    for (int pass = 0; pass < globs.globs && count == 0; pass++)
    {
        a.glob = globs.glob[pass];
        a.out = tmp;
        a.max = max;
        a.ext = ext;
        a.count = 0;
        count = gint_world_switch(GINT_CALL(body_list, (void *)&a));
        if (count < 0)
            count = 0;          /* no folder yet: treat as empty */
    }
    if (count <= 0)
        return 0;

    /* Insertion sort by name: at most 32 entries. */
    for (int i = 0; i < count; i++)
    {
        out[i] = tmp[i];
        out[i].at = (uint8_t)at;
    }
    for (int i = 1; i < count; i++)
    {
        store_entry_t key = out[i];
        int j = i - 1;

        while (j >= 0 && strcmp(out[j].name, key.name) > 0)
        {
            out[j + 1] = out[j];
            j--;
        }
        out[j + 1] = key;
    }
    return count;
}

static store_location_t last_list_at = STORE_AT_BENCH;

/* The first place that has anything wins, so one screen never mixes files from
 * two folders without saying so (and the reader then opens them from there). */
int store_list(char const *folder, char const *ext, store_entry_t *out,
               int max)
{
    /* Deliberately not reset here: a screen lists ".md" and then ".txt", and
     * the second call finding nothing must not make the caption claim the files
     * are in Bench when the first call took them from SAVE-F. */
    for (int at = STORE_AT_BENCH; at < STORE_AT_COUNT; at++)
    {
        int n;

        /* The drive root also holds the .g3a and whatever else lives there, so
         * it is only scanned when the caller named an extension to want. */
        if (at == STORE_AT_LOOSE && (!ext || !*ext))
            continue;
        n = list_from((store_location_t)at, folder, ext, out, max);

        if (n > 0)
        {
            last_list_at = (store_location_t)at;
            return n;
        }
    }
    /* Nothing anywhere: the caller gets the same 0 it always got, and the
     * empty-state screens behave as they did before. */
    return 0;
}

store_location_t store_list_location(void)
{
    return last_list_at;
}

char const *store_location_name(store_location_t at)
{
    switch (at)
    {
        case STORE_AT_SAVEF: return "SAVE-F";
        case STORE_AT_ROOT:  return "card root";
        default:             return "Bench";
    }
}

/* Build \\fls0\\<root>\\<folder>\\<name> for one location.  `at` of
 * STORE_AT_BENCH reuses build_path(), which is the same path spelled once. */
static void build_at(store_location_t at, char const *folder, char const *name)
{
    char const *root = root_path(at);

    path_len = append_ascii(path_buf, 0, root);
    path_len = append_ascii(path_buf, path_len, "\\");
    if (!location_is_flat(at))
    {
        path_len = append_ascii(path_buf, path_len, folder);
        path_len = append_ascii(path_buf, path_len, "\\");
    }
    append_ascii(path_buf, path_len, name);
}

int store_read_at(int at, char const *folder, char const *name, char *buf,
                  int size, int *len)
{
    read_args_t a;

    if (len)
        *len = 0;
    if (!folder || !name || !buf || size <= 0)
        return STORE_ERR_PATH;

    for (int pass = 0; pass < STORE_AT_COUNT; pass++)
    {
        store_location_t here;
        int rc;

        if (at != STORE_AT_ANY && pass > 0)
            break;
        here = (at == STORE_AT_ANY) ? (store_location_t)pass
                                    : (store_location_t)at;

        build_at(here, folder, name);
        a.path = path_buf;
        a.buf = buf;
        a.size = size;
        a.len = 0;
        rc = gint_world_switch(GINT_CALL(body_read, (void *)&a));
        if (len)
            *len = a.len;
        if (rc != STORE_ERR_OPEN)
            return rc;             /* read, or too big: both are answers */
        buf[0] = 0;
    }
    return STORE_ERR_OPEN;
}

bool store_exists_any(char const *folder, char const *name)
{
    if (!folder || !name)
        return false;
    for (int at = STORE_AT_BENCH; at < STORE_AT_COUNT; at++)
    {
        exists_args_t a;

        build_at((store_location_t)at, folder, name);
        a.path = path_buf;
        if (gint_world_switch(GINT_CALL(body_exists, (void *)&a)) > 0)
            return true;
    }
    return false;
}

void store_probe(char const *folder, char const *ext,
                 int counts[STORE_AT_COUNT])
{
    static store_entry_t one[STORE_LIST_MAX];

    if (!counts)
        return;
    for (int at = 0; at < STORE_AT_COUNT; at++)
        counts[at] = list_from((store_location_t)at, folder, ext, one,
                               STORE_LIST_MAX);
}

bool store_elsewhere_exists(char const *folder)
{
    static store_entry_t one[1];

    if (!folder || !*folder)
        return false;
    return list_from(STORE_AT_SAVEF, folder, "", one, 1) > 0 ||
           list_from(STORE_AT_ROOT, folder, "", one, 1) > 0;
}
