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
#include <stdint.h>

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

/* Where a listed file was found.  Bench looks in three places, in this order,
 * because a file that is one folder away is not a missing file:
 *
 *   STORE_AT_BENCH   \\fls0\\Bench\\<folder>       what docs/HARDWARE.md asks for
 *   STORE_AT_SAVEF   \\fls0\\SAVE-F\\Bench\\<folder>  where the OS parks files it
 *                                                  moved out of @MainMem (manual
 *                                                  13-6), and where a Bench
 *                                                  folder copied into @MainMem
 *                                                  ends up
 *   STORE_AT_ROOT    \\fls0\\<folder>             files dropped in the drive root
 *                                                  instead of inside Bench
 *
 * 1.2.x only ever looked at the first, so the list said "nothing yet" while the
 * files sat one folder away - twice, once for @MainMem and once for a folder
 * copied onto the drive root.  The viewer reads from wherever the list came
 * from, and says where that is. */
typedef enum {
    STORE_AT_BENCH = 0,   /* \\fls0\\Bench\\<folder>\\<name>   the documented one  */
    STORE_AT_SAVEF = 1,   /* \\fls0\\SAVE-F\\Bench\\<folder>\\<name>            */
    STORE_AT_ROOT = 2,    /* \\fls0\\<folder>\\<name>   the folder at the drive root */
    STORE_AT_LOOSE = 3,   /* \\fls0\\<name>            the files at the drive root */
    STORE_AT_COUNT = 4,   /* how many places there are, STORE_AT_ANY is -1      */
} store_location_t;

typedef struct {
    char name[STORE_NAME_MAX];   /* file name only, e.g. "unit3.md" */
    int size;                    /* bytes */
    uint8_t at;                  /* store_location_t */
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
/* True when the same folder exists under SAVE-F\Bench instead - the place the
 * calculator parks files it will not keep in main memory (fx-CG 50 manual,
 * 13-6: anything in @MainMem past three levels, and anything it does not
 * recognise, is moved there).  A Bench folder copied into @MainMem ends up
 * here, and without this check the screen just says "nothing yet" while the
 * files sit one folder away. */
bool store_elsewhere_exists(char const *folder);

/* List files in `folder` from the first place that has any: Bench, then
 * SAVE-F\\Bench, then the drive root.  Every entry carries the place it came
 * from, so the reader can open it and the screen can say where it is. */
int store_list(char const *folder, char const *ext, store_entry_t *out,
               int max);

/* Which place the last store_list() read from. */
store_location_t store_list_location(void);

/* Name of a place, for a screen: "Bench", "SAVE-F", "card root". */
char const *store_location_name(store_location_t at);

/* Read "<folder>/<name>" from `at`, or from whichever place has it when `at`
 * is -1 (store_at_any).  Used by the readers, so a file listed from SAVE-F can
 * be opened. */
#define STORE_AT_ANY (-1)
int store_read_at(int at, char const *folder, char const *name, char *buf,
                  int size, int *len);

/* True when a file of that name exists in any of the three places.  The
 * starter pack uses it so seeding never writes a second copy next to a file the
 * user already has somewhere Bench can see. */
bool store_exists_any(char const *folder, char const *name);

/* How many files each place holds, for the card-check report:
 * counts[STORE_AT_BENCH .. STORE_AT_LOOSE], in that order. */
void store_probe(char const *folder, char const *ext, int counts[STORE_AT_COUNT]);

/* True when the file exists.  Used to seed the starter pack without ever overwriting
 * something the user wrote. */
bool store_exists(char const *rel);

/* Build "folder/name" into buf (for store_read/store_write). */
int store_join(char *buf, int size, char const *folder, char const *name);

#endif /* BENCH_STORE_H */
