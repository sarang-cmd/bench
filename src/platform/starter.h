/* starter.h - the starter pack notes, decks, study log and formula booklets travel inside
 * the .g3a, and land on the card the first time it runs.
 *
 * Spec: the request to bundle the starter pack with the add-in.  A .g3a is one file, so
 * the text is embedded by tools/gen_starterpack.py (src/apps/starter_data.c) and written
 * by this module.  Rules it follows:
 *
 *   - only on start-up, and only files that are not there yet: nothing the user
 *     wrote is ever overwritten, and a second run writes nothing at all
 *   - at most STARTER_MAX_WRITES files per run, so a first start cannot stall
 *   - failures are ignored on purpose: a card that is full or write-protected
 *     still gets a working add-in, just without the starter pack
 */

#ifndef BENCH_STARTER_H
#define BENCH_STARTER_H

/* Write whatever is missing.  Returns how many files were written. */
int starter_install(void);

#endif /* BENCH_STARTER_H */
