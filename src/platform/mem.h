/* mem.h - buffers bigger than a static array.
 *
 * Spec: docs/plan.md section 2 ("gint offers extra heap arenas") and section
 * 6.4 (the note buffer cap).
 *
 * gint 2.10.0 facts (kmalloc.h): kmalloc(size, arena_name), kfree(ptr),
 * kmalloc_max(&size, arena_name).  The arenas on fx-CG50 are "_uram" (from the
 * OS heap) and "_ostk" (the OS stack area).  The default arena is NULL.
 *
 * The add-in allocates its few large buffers once, at the start of a screen,
 * and frees them when it leaves.  Nothing allocates inside a loop. */

#ifndef BENCH_MEM_H
#define BENCH_MEM_H

#include <stdbool.h>
#include <stddef.h>

#ifndef BENCH_NOTE_MAX
#define BENCH_NOTE_MAX 32768      /* plan section 6.4: 32 KiB cap for v1 */
#endif

/* Allocate `size` bytes from the first arena that can serve it
 * (NULL, then "_uram", then "_ostk").  Returns NULL when memory is short. */
void *mem_alloc(size_t size);

/* Free a pointer from mem_alloc().  NULL is ignored. */
void mem_free(void *ptr);

/* Largest block available, for the About screen and for the note cap. */
size_t mem_largest(void);

#endif /* BENCH_MEM_H */
