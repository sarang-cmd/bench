/* test_filelist.c - the rules that decide which files a list shows.
 * Spec: src/core/filelist.h, and the "it cannot see my files" bug. */

#include <string.h>

#include "filelist.h"
#include "test_util.h"

static void test_ext(void)
{
    T_SECTION("extension matching ignores case");

    T_TRUE(filelist_has_ext("cells.md", ".md"));
    T_TRUE(filelist_has_ext("CELLS.MD", ".md"));
    T_TRUE(filelist_has_ext("Cells.Md", ".md"));
    T_TRUE(filelist_has_ext("chapter.md", "md"));      /* no dot either way */
    T_TRUE(filelist_has_ext("chapter.md", ".MD"));

    T_FALSE(filelist_has_ext("cells.txt", ".md"));
    T_FALSE(filelist_has_ext("cells", ".md"));
    T_FALSE(filelist_has_ext("md", ".md"));            /* too short */
    T_FALSE(filelist_has_ext("", ".md"));

    /* No extension asked for means everything. */
    T_TRUE(filelist_has_ext("cells.md", NULL));
    T_TRUE(filelist_has_ext("cells.md", ""));
    T_TRUE(filelist_has_ext("README", NULL));
}

static void test_name(void)
{
    T_SECTION("names that may appear in a list");

    T_TRUE(filelist_name_ok("cells.md"));
    T_TRUE(filelist_name_ok("a"));
    T_TRUE(filelist_name_ok(".hidden"));
    T_TRUE(filelist_name_ok("notes 2026-11-01.txt"));

    T_FALSE(filelist_name_ok(""));
    T_FALSE(filelist_name_ok(NULL));
    T_FALSE(filelist_name_ok("."));
    T_FALSE(filelist_name_ok(".."));
    T_FALSE(filelist_name_ok("notes\\cells.md"));
    T_FALSE(filelist_name_ok("notes/cells.md"));
}

static void test_ext_of(void)
{
    char buf[8];

    T_SECTION("the extension of a name");

    T_INT(filelist_ext_of("cells.md", buf, (int)sizeof buf), 2);
    T_STR(buf, "md");

    T_INT(filelist_ext_of("CELLS.MD", buf, (int)sizeof buf), 2);
    T_STR(buf, "md");

    T_INT(filelist_ext_of("cards.tar.gz", buf, (int)sizeof buf), 2);
    T_STR(buf, "gz");

    T_INT(filelist_ext_of("README", buf, (int)sizeof buf), 0);
    T_STR(buf, "");

    T_INT(filelist_ext_of(".hidden", buf, (int)sizeof buf), 0);
    T_STR(buf, "");

    /* Cut rather than overflow. */
    T_INT(filelist_ext_of("a.abcdefgh", buf, 4), 3);
    T_STR(buf, "abc");
}

static void test_globs(void)
{
    char buf[16];

    T_SECTION("the wildcard handed to the OS");

    T_INT(filelist_glob(".md", buf, (int)sizeof buf), 4);
    T_STR(buf, "*.md");

    T_INT(filelist_glob(NULL, buf, (int)sizeof buf), 3);
    T_STR(buf, "*.*");

    T_INT(filelist_glob("", buf, (int)sizeof buf), 3);
    T_STR(buf, "*.*");

    T_INT(filelist_glob(".txt", buf, (int)sizeof buf), 5);
    T_STR(buf, "*.txt");

    /* The other spelling: everything, then filtered by extension in C. */
    T_INT(filelist_glob_alt(".md", buf, (int)sizeof buf), 3);
    T_STR(buf, "*.*");

    T_INT(filelist_glob_alt(NULL, buf, (int)sizeof buf), 1);
    T_STR(buf, "*");

    /* Too small is an error, never a truncated glob. */
    T_INT(filelist_glob(".md", buf, 3), -1);
}

static void test_windows_copies(void)
{
    char buf[16];

    T_SECTION("what a Windows copy actually produces");

    /* USB Flash mode keeps the names Windows gives the files. */
    T_TRUE(filelist_has_ext("Cell Structure.MD", ".md"));
    T_TRUE(filelist_has_ext("Paper 1 - Mark Scheme.md", ".md"));
    T_TRUE(filelist_has_ext("Untitled.txt", ".txt"));
    T_FALSE(filelist_has_ext("Untitled.txt", ".md"));

    /* Canonical case, and the two entry names the OS returns. */
    T_TRUE(filelist_name_ok("Paper 1 - Mark Scheme.md"));
    T_FALSE(filelist_name_ok("."));
    T_FALSE(filelist_name_ok(".."));

    T_INT(filelist_glob(".md", buf, (int)sizeof buf), 4);
    T_STR(buf, "*.md");
}

static void run_tests(void)
{
    test_ext();
    test_name();
    test_ext_of();
    test_globs();
    test_windows_copies();
}

T_MAIN("test_filelist")
