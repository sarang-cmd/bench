/* demo.h - the demo notes, decks, study log and formula booklets travel inside
 * the .g3a, and land on the card the first time it runs.
 *
 * Spec: the request to bundle the demo with the add-in.  A .g3a is one file, so
 * the text is embedded by tools/gen_demo.py (src/apps/demo_data.c) and written
 * by this module.  Rules it follows:
 *
 *   - only on start-up, and only files that are not there yet: nothing the user
 *     wrote is ever overwritten, and a second run writes nothing at all
 *   - at most DEMO_MAX_WRITES files per run, so a first start cannot stall
 *   - failures are ignored on purpose: a card that is full or write-protected
 *     still gets a working add-in, just without the demo
 */

#ifndef BENCH_DEMO_H
#define BENCH_DEMO_H

/* Write whatever is missing.  Returns how many files were written. */
int demo_install(void);

#endif /* BENCH_DEMO_H */
