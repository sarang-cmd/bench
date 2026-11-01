/* version.h - the add-in version, shown on the About screen and in the .g3a
 * filename.  CMake passes -DBENCH_VERSION="x.y.z"; the fallback keeps a plain
 * `make gint-check` working. */

#ifndef BENCH_VERSION_H
#define BENCH_VERSION_H

#ifndef BENCH_VERSION
#define BENCH_VERSION "1.2.1"   /* keep in step with version.txt */
#endif

#define BENCH_NAME "Bench"

#endif /* BENCH_VERSION_H */
