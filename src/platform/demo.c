/* demo.c - put the bundled demo content on the card, once.  See demo.h. */

#include "demo.h"

#include <stdbool.h>
#include <string.h>

#ifdef BENCH_SIM
#include <stdlib.h>
#endif

#include "demo_data.h"
#include "store.h"

/* The demo is 10 files and about 18 KiB.  A cap means a first run can never
 * spend a whole minute in the flash driver if someone grows demo/ later; the
 * rest is written on the next start. */
#define DEMO_MAX_WRITES 12

/* store_write() copies from here into its own padded buffer, so this only has
 * to hold one file.  The generator refuses anything over 6144 bytes. */
#define DEMO_FILE_MAX 6144

int demo_install(void)
{
    static char text[DEMO_FILE_MAX + 1];
    char rel[64];
    int written = 0;

#ifdef BENCH_SIM
    /* The host preview needs an empty card as well as a seeded one: the tour and
     * the screen checks show the "nothing yet" states, which a first run would
     * otherwise fill.  BENCH_NO_DEMO=1 is that switch, and it exists only in the
     * PC build - the add-in has no environment to read. */
    if (getenv("BENCH_NO_DEMO"))
        return 0;
#endif

    for (int i = 0; i < demo_file_count && written < DEMO_MAX_WRITES; i++)
    {
        demo_file_t const *file = &demo_files[i];
        int len = 0;
        bool fits = true;

        if (!file->parts || file->len <= 0)
            continue;
        if (store_join(rel, (int)sizeof rel, file->folder, file->name) != STORE_OK)
            continue;
        /* Never overwrite: the demo is a starting point, and a note the user
         * edited under the same name is theirs. */
        if (store_exists(rel))
            continue;

        /* Join the parts.  The literal limit is why they are parts at all. */
        for (int part = 0; file->parts[part]; part++)
        {
            int n = (int)strlen(file->parts[part]);

            if (len + n > (int)sizeof text - 1)
            {
                fits = false;
                break;
            }
            memcpy(text + len, file->parts[part], (size_t)n);
            len += n;
        }
        if (!fits || len != file->len)
            continue;                  /* generated file would be wrong: skip it */

        /* store_write pads an odd-length write with one newline (the flash
         * driver writes 16-bit units).  Dropping the final newline of a file
         * that ends in one therefore lands the text on the card exactly instead
         * of adding a blank line to a note the user will open later. */
        if ((len & 1) && text[len - 1] == '\n')
            len--;

        if (store_write(rel, text, len) == STORE_OK)
            written++;
    }
    return written;
}
