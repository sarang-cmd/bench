/* store.h - the only place that touches the calculator's filesystem.
 *
 * Spec: docs/plan.md section 4 (folder layout, write rules) and section 3
 * ("store.h interface hides every file call from the rest of the code").
 *
 * Folder layout, relative to \\fls0\Bench:
 *     notes/<name>.md, notes/<name>.txt
 *     cards/<name>.txt
 *     state/cards.sav
 *     state/settings.cfg
 *     formulas/<name>.txt
 *
 * gint 2.10.0 facts this file is built on (headers checked):
 *   BFile_Create(path, BFile_File|BFile_Folder, &size), BFile_Open/Read/Write/
 *   Seek/Close/Size/Remove/Rename, BFile_FindFirst/FindNext/FindClose with a
 *   glob pattern (four search handles, must be closed), struct BFile_FileInfo.
 *   Every BFile call needs gint_world_switch(); paths are FONTCHARACTER
 *   (uint16_t) with a "\\\\fls0\\" prefix.
 *
 * Write rules, from the plan:
 *   - a file is written once, then a rename replaces the old one
 *   - the CASIOWIN filesystem requires even-sized writes, so an odd-length
 *     text file is padded with a newline (harmless for every format here)
 *   - nothing is written inside a loop or on a timer
 */

#ifndef BENCH_STORE_H
#define BENCH_STORE_H

#include <stdbool.h>

/* Result codes.  Negative values are errors, 0 is success. */
#define STORE_OK 0
#define STORE_ERR_PATH (-1)     /* bad argument or path too long */
#define STORE_ERR_OPEN (-2)
#define STORE_ERR_READ (-3)
#define STORE_ERR_WRITE (-4)
#define STORE_ERR_TOO_BIG (-5)  /* the file does not fit the caller's buffer */
#define STORE_ERR_EXISTS (-6)

/* Largest file Bench will read into memory.  Notes are capped far lower; this
 * is the ceiling for a formula booklet, which is the biggest thing the add-in
 * ever loads, and it is checked before a single byte is read. */
#define STORE_FILE_MAX 24576

#define STORE_NAME_MAX 72
#define STORE_LIST_MAX 32

typedef struct {
    char name[STORE_NAME_MAX];   /* file name only, e.g. "unit3.md" */
    int size;                    /* bytes */
} store_entry_t;

/* Create Bench, Bench/notes, Bench/cards and Bench/state when missing.
 * Call once from main(); safe to call again. */
int store_init(void);

/* Read a whole file into `buf`.  `len` may be NULL.  STORE_ERR_TOO_BIG when
 * the file is larger than `size` (the buffer is left untouched). */
int store_read(char const *rel, char *buf, int size, int *len);

/* Whole file as bytes, or -1 when it does not exist. */
int store_size(char const *rel);

/* Write `len` bytes to `rel`, via "<rel>.tmp" and a rename.
 * Returns STORE_OK or an error. */
int store_write(char const *rel, char const *data, int len);

/* Delete a file.  Missing files count as success. */
int store_remove(char const *rel);

/* List files in `folder` ("notes", "cards", "state") whose name ends with
 * `ext` (".md", ".txt"; NULL for every file).  Sorted by name.  Returns the
 * number written, or a negative error. */
int store_list(char const *folder, char const *ext, store_entry_t *out,
               int max);

/* Build "folder/name" into buf (for store_read/store_write). */
int store_join(char *buf, int size, char const *folder, char const *name);

#endif /* BENCH_STORE_H */
