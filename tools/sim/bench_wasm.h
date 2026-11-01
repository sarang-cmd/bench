/* bench_wasm.h - the three things the browser build needs from the page.
 *
 * tools/sim/sim.c is compiled to WebAssembly by tools/wasm/build.sh with
 * -DBENCH_WASM.  Everything else stays exactly as the desktop build has it; the
 * platform the add-in runs on is still this file.  Only two directions of talk
 * are different:
 *
 *   page -> add-in   bench_wasm_wait_key(), imported from the module "bench".
 *                    It blocks until the visitor presses something and returns
 *                    a gint key code.  Blocking is fine because the module runs
 *                    in a Web Worker, not on the page's main thread.
 *
 *   add-in -> page   bench_wasm_frame(), exported.  The page calls it from
 *                    inside the imported function above, which a WebAssembly
 *                    module allows: same instance, same thread, no re-entry into
 *                    a half-finished call.
 *
 * The callbacks live in site/public/sim/worker.js.  Asyncify is not used and
 * not needed: nothing here returns to the browser mid-call.
 */

#ifndef BENCH_SIM_WASM_H
#define BENCH_SIM_WASM_H

/* An exported function: named in the link step so the page can find it. */
#define BENCH_WASM_EXPORT __attribute__((used, visibility("default")))

/* Keys arrive from the page.  -1 means "the tab is going away, stop". */
__attribute__((import_module("bench"), import_name("wait_key")))
int bench_wasm_wait_key(void);

#endif /* BENCH_SIM_WASM_H */
