/* test_demo_data.c - the demo that travels inside the .g3a.
 * Spec: tools/gen_demo.py, and the request to bundle the demo with the add-in.
 *
 * demo_data.c is generated, so this test has two jobs:
 *
 *   1. the table is well formed - safe to hand to store_write() on a card
 *   2. every entry still matches the file in demo/ (or docs/samples/formulas/)
 *      that it was generated from, so the copy in the add-in cannot quietly
 *      drift away from the copy in the zip
 *
 * The second job is why the paths below are spelled out: a rename in one place
 * must fail here rather than ship a note nobody can find. */

#include <stdio.h>
#include <string.h>

#include "test_util.h"

#include "demo_data.h"

/* The folders store_init() creates under \\fls0\Bench. */
static char const *const FOLDERS[] = { "notes", "cards", "study", "formulas" };

/* Same guard as the generator: this is a demo, not a library. */
#define BUDGET (48 * 1024)

/* One file, joined from its parts the way src/platform/demo.c does it. */
static char joined[BUDGET];

static int join(int index)
{
    int len = 0;

    joined[0] = '\0';
    if (index < 0 || index >= demo_file_count)
        return -1;
    for (int part = 0; demo_files[index].parts[part]; part++)
    {
        int n = (int)strlen(demo_files[index].parts[part]);

        if (len + n >= (int)sizeof joined)
            return -1;
        memcpy(joined + len, demo_files[index].parts[part], (size_t)n);
        len += n;
    }
    joined[len] = '\0';
    return len;
}

static int folder_known(char const *folder)
{
    for (unsigned i = 0; i < sizeof FOLDERS / sizeof FOLDERS[0]; i++)
        if (strcmp(folder, FOLDERS[i]) == 0)
            return 1;
    return 0;
}

static int has_name(char const *folder, char const *name)
{
    for (int i = 0; i < demo_file_count; i++)
        if (strcmp(demo_files[i].folder, folder) == 0 &&
            strcmp(demo_files[i].name, name) == 0)
            return 1;
    return 0;
}

static void test_table(void)
{
    int total = 0;

    T_SECTION("the embedded demo table");

    T_INT(demo_file_count, 10);

    for (int i = 0; i < demo_file_count; i++)
    {
        demo_file_t const *file = &demo_files[i];
        int len;

        T_TRUE(file->folder != NULL);
        T_TRUE(file->name != NULL);
        T_TRUE(file->parts != NULL);
        T_TRUE(file->parts != NULL && file->parts[0] != NULL);
        if (!file->folder || !file->name || !file->parts)
            continue;

        T_TRUE(folder_known(file->folder));
        T_TRUE(file->name[0] != '\0');
        T_TRUE(strchr(file->name, '/') == NULL);
        T_TRUE(strchr(file->name, '\\') == NULL);

        /* len is the byte count the add-in writes; it must be the real length
         * of the joined parts, or store_write() writes part of a note. */
        len = join(i);
        T_INT(len, file->len);
        T_TRUE(file->len > 0);

        /* The calculator font stops at 0x7E, and the generator refuses the
         * rest - check it again, because a hand edit would skip the generator. */
        int bad = 0;
        for (int j = 0; j < len; j++)
        {
            unsigned char c = (unsigned char)joined[j];

            if (c != '\n' && (c < 0x20 || c > 0x7E))
                bad++;
        }
        T_CHECK(bad == 0, "%d non-printable byte(s) in %s/%s", bad,
                file->folder, file->name);
        total += file->len;
    }

    T_TRUE(total <= BUDGET);
}

static void test_unique(void)
{
    T_SECTION("no demo file is listed twice");

    for (int i = 0; i < demo_file_count; i++)
        for (int j = i + 1; j < demo_file_count; j++)
            T_FALSE(strcmp(demo_files[i].folder, demo_files[j].folder) == 0 &&
                    strcmp(demo_files[i].name, demo_files[j].name) == 0);
}

/* Every folder the lists can show gets at least one file, or a fresh install
 * opens a screen that says "nothing yet" and looks broken. */
static void test_every_folder(void)
{
    T_SECTION("every folder gets something on a fresh card");

    T_TRUE(has_name("notes", "constant-acceleration.md"));
    T_TRUE(has_name("notes", "photosynthesis.md"));
    T_TRUE(has_name("notes", "uncertainty-rules.md"));
    T_TRUE(has_name("cards", "mechanics.txt"));
    T_TRUE(has_name("cards", "quadratics.txt"));
    T_TRUE(has_name("cards", "moles-and-concentration.txt"));
    T_TRUE(has_name("study", "log.txt"));
    T_TRUE(has_name("formulas", "aahl.txt"));
    T_TRUE(has_name("formulas", "chemistry.txt"));
    T_TRUE(has_name("formulas", "physics.txt"));

    /* Nothing from the proofreading-pending folder may ship: the add-in would
     * be handing a student text nobody has read yet. */
    for (int i = 0; i < demo_file_count; i++)
    {
        join(i);
        T_TRUE(strstr(joined, "needs-human-proofreading") == NULL);
    }
}

/* Where each entry lives in the repository.  Formulas come from the sample
 * booklets, which are also what tools/make_demo_zip.py packs. */
static void source_path(char *buf, int size, char const *folder, char const *name)
{
    if (strcmp(folder, "formulas") == 0)
        snprintf(buf, (size_t)size, "docs/samples/formulas/%s", name);
    else
        snprintf(buf, (size_t)size, "demo/%s/%s", folder, name);
}

static void test_matches_source(void)
{
    static char text[BUDGET];
    char path[128];

    T_SECTION("the embedded copy still matches the file it came from");

    for (int i = 0; i < demo_file_count; i++)
    {
        demo_file_t const *file = &demo_files[i];
        size_t want, have;
        FILE *f;

        source_path(path, (int)sizeof path, file->folder, file->name);
        f = fopen(path, "rb");
        if (!f)
        {
            T_CHECK(0, "demo source missing: %s (renamed or moved?)", path);
            continue;
        }

        size_t len = fread(text, 1, sizeof text - 1, f);
        fclose(f);
        text[len] = '\0';

        /* The generator adds a final newline to a file that lacks one, so
         * compare after trimming trailing newlines on both sides. */
        want = len;
        while (want > 0 && text[want - 1] == '\n')
            want--;

        join(i);
        have = (size_t)file->len;
        while (have > 0 && joined[have - 1] == '\n')
            have--;

        if (want != have)
            printf("    stale: %s (repo %d B, embedded %d B)\n", path, (int)want,
                   (int)have);
        T_INT((int)have, (int)want);
        T_TRUE(strncmp(text, joined, have) == 0);
    }
}

int main(void)
{
    t_suite = "demo_data";
    printf("demo_data\n");
    test_table();
    test_unique();
    test_every_folder();
    test_matches_source();
    return t_report();
}
