/* filelist.h - which files a list shows, and which glob finds them.
 *
 * Spec: the "it cannot see my files" bug.  Someone copied a dozen notes into
 * Bench/notes/ over USB and the add-in showed an empty list.  Three separate
 * causes, all in the matching rules rather than in the filesystem code:
 *
 *   1. the glob was "...\*" plus the extension.  CASIOWIN's BFile_FindFirst
 *      wants a full wildcard - "*.md", or "*.*" for everything - and returns
 *      nothing for a bare "*" on some OS versions.  Both spellings are tried.
 *   2. the extension test was case sensitive, so "NOTES.MD" and a note saved by
 *      a Windows editor as ".MD" were invisible.
 *   3. a name that the OS pads or hides (. .., 8.3 leftovers) was not filtered.
 *
 * None of this can be tested on the calculator, so the rules live here: plain
 * C11, no gint, checked by tests/test_filelist.c.
 */

#ifndef BENCH_FILELIST_H
#define BENCH_FILELIST_H

/* True when `name` ends with `ext`, ignoring case.  A NULL or empty `ext`
 * matches every name, which is what "list everything" asks for. */
int filelist_has_ext(char const *name, char const *ext);

/* True when `name` should appear in a list: not empty, not "." or "..", and
 * carrying no path separator (the OS returns full paths; callers pass the base
 * name, and a stray separator means the split went wrong). */
int filelist_name_ok(char const *name);

/* The extension of `name`, without the dot, lower-cased, or "" when there is
 * none.  Used to label a file in the list. */
int filelist_ext_of(char const *name, char *out, int size);

/* Build the wildcard for a folder listing into `out`:
 *   ext NULL or ""  -> "*.*"   (every file, the spelling CASIOWIN wants)
 *   ext ".md"       -> "*.md"
 * Returns the length written, or -1 when the buffer is too small. */
int filelist_glob(char const *ext, char *out, int size);

/* The second spelling to try when the first finds nothing: "*.*" for the
 * extension case, and "*" otherwise, so both OS behaviours are covered.
 * Returns the length written, or -1. */
int filelist_glob_alt(char const *ext, char *out, int size);

#endif /* BENCH_FILELIST_H */
