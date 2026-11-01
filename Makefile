# Native (PC) build for the Bench core, plus the host preview of the screens.
#
#   make test        build and run every native test in tests/
#   make py-test     run the tools/ tests
#   make test-all    both of the above
#   make sim         build the host preview (tools/sim) of the add-in
#   make shots       render every screen to docs/images (needs `make sim`)
#   make gint-check  type-check the add-in against real gint headers
#   make clean
#
# There is no gint here on purpose: src/core must stay free of gint includes,
# so everything below compiles with the host C compiler only.
# The calculator build is separate: `fxsdk build-cg` (see docs/toolchain.md).

CC      ?= cc
PYTHON  ?= python3
CFLAGS  ?= -std=c11 -O2 -g
# -Werror is on so a warning in src/core fails the PC build before the g3a build.
WARN    := -Wall -Wextra -Wshadow -Wstrict-prototypes -Wmissing-prototypes \
           -Werror -pedantic
CPPFLAGS := -Isrc/core -D_POSIX_C_SOURCE=200809L
LDLIBS  := -lm

CORE_SRC := $(wildcard src/core/*.c)
CORE_HDR := $(wildcard src/core/*.h)
TEST_SRC := $(wildcard tests/test_*.c)
TEST_BIN := $(patsubst tests/%.c,build/tests/%,$(TEST_SRC))

all: test

build/tests:
	@mkdir -p build/tests

build/tests/%: tests/%.c $(CORE_SRC) $(CORE_HDR) tests/test_util.h | build/tests
	$(CC) $(CFLAGS) $(WARN) $(CPPFLAGS) -o $@ $< $(CORE_SRC) $(LDLIBS)

# The bundled demo is generated data, not a core module: this one test links the
# table in as well as src/core.  demo-check runs first, so a stale table fails
# with the generator's message rather than a diff of two literal dumps.
build/tests/test_demo_data: tests/test_demo_data.c src/apps/demo_data.c \
                            src/apps/demo_data.h $(CORE_SRC) $(CORE_HDR) \
                            tests/test_util.h demo-check | build/tests
	$(CC) $(CFLAGS) $(WARN) $(CPPFLAGS) -Isrc/apps -o $@ $< src/apps/demo_data.c \
	      $(CORE_SRC) $(LDLIBS)

test: version-check $(TEST_BIN) check-screens
	@echo
	@echo "== native core tests =="
	@fail=0; for t in $(TEST_BIN); do ./$$t || fail=1; done; \
	 if [ $$fail -ne 0 ]; then echo "NATIVE TESTS FAILED"; exit 1; fi; \
	 echo "NATIVE TESTS PASSED ($(words $(TEST_BIN)) test programs)"

# Rewrite the README's generated badge and stats blocks (CI checks them).
stats:
	@$(PYTHON) tools/gen_readme_stats.py
	@cd site && node scripts/gen-stats.mjs && node scripts/gen-changelog.mjs

# Every change bumps the version (docs/VERSIONING.md).  `make bump-patch` for a
# fix, `bump-minor` for a feature, `bump-major` only for a storage-layout change.
bump-patch bump-minor bump-major:
	@$(PYTHON) tools/bump_version.py $(patsubst bump-%,%,$@)

# Fails when version.txt, version.h, CMakeLists.txt, the changelog and the
# README badge disagree.  Part of `make test`, so a forgotten bump fails a build.
version-check:
	@$(PYTHON) tools/bump_version.py --check

# .coderabbit.yaml: an unknown key, a review rule for a directory that no longer
# exists, or automatic approvals switched on.  CodeRabbit only complains on the
# next pull request, which is too late, so check it here.
coderabbit-check:
	@$(PYTHON) tools/check_coderabbit.py

# The demo travels inside the .g3a (src/apps/demo_data.c, written by
# tools/gen_demo.py from demo/ and docs/samples/formulas).  `make demo-check`
# fails when the generated file no longer matches those sources, so editing a
# demo note without regenerating cannot reach a release.
demo-gen:
	@$(PYTHON) tools/gen_demo.py

demo-check: version-check
	@$(PYTHON) tools/gen_demo.py --check

# The separate download for the same demo (dist/Bench-demo-<version>.zip).  Not
# part of `make test`: the file is renamed on every version bump, so the check
# lives in the release job, after the bump.
demo-zip:
	@$(PYTHON) tools/make_demo_zip.py

demo-zip-check:
	@$(PYTHON) tools/make_demo_zip.py --check

# The spike add-in as one patch file (see docs/spike.md).
spike-patch:
	@$(PYTHON) tools/gen_spike_patch.py

py-test: coderabbit-check
	@echo "== python tool tests =="
	$(PYTHON) -m unittest discover -s tools -p "test_*.py" -v

test-all: test py-test

# ---------------------------------------------------------------------------
# Host preview.  tools/sim/ supplies a small stand-in for gint so the whole
# add-in (platform, ui, apps) compiles and runs on a PC.  It is a development
# toy, not the build the calculator gets; the real build is `fxsdk build-cg`.
# ---------------------------------------------------------------------------

SIM_SRC := $(wildcard src/core/*.c) $(wildcard src/platform/*.c) \
           $(wildcard src/ui/*.c) $(wildcard src/apps/*.c) tools/sim/sim.c
SIM_CFLAGS := -std=gnu11 -O1 -g -Wall -Wextra -Wno-missing-field-initializers \
              -DBENCH_TRACE -DBENCH_SIM \
              -Werror=implicit-function-declaration \
              -Wno-missing-braces \
              -Isrc -Isrc/ui -Isrc/core -Isrc/apps -Itools/sim/include
SIM_HDR := $(wildcard src/*.h src/ui/*.h src/platform/*.h src/apps/*.h \
             tools/sim/include/gint/*.h tools/sim/sim_font.h)

build/sim/bench: $(SIM_SRC) $(SIM_HDR) | build/sim
	$(CC) $(SIM_CFLAGS) -Dmain=bench_main -c -o build/sim/main.o src/main.c
	$(CC) $(SIM_CFLAGS) -o $@ build/sim/main.o $(filter-out src/main.c,$(SIM_SRC)) $(LDLIBS)

build/sim:
	@mkdir -p build/sim

sim: build/sim/bench

# Overlap and contrast checks on every screen, run through the host preview.
# tools/sim records every rectangle the add-in draws and fails on: two labels
# overlapping, a label outside the window, a label in the title bar or softkey
# strip, a label a later panel paints over, a label under WCAG AA, a softkey
# label wider than its cell, or a tile icon that matches its tile.
check-screens: sim
	@tools/sim/check_screens.sh

shots: sim
	@tools/sim/run_tour.sh $(CURDIR)/docs/images

# The browser build: the add-in and the stand-in compiled to WebAssembly with
# zig, then driven through the page's own worker protocol in Node. Needs zig
# (pip install ziglang) and nothing else; ZIG= points at it when it is not on
# PATH. The binary it produces is committed in site/public/sim, because Vercel
# has no C compiler and the deployed simulator needs the bytes in git.
ZIG ?= zig

wasm:
	@ZIG=$(ZIG) tools/wasm/build.sh
	@node tools/wasm/smoke.mjs "down,exe"
	@node tools/wasm/worker-smoke.mjs "down,exe,f2"
	@node site/scripts/copy-wasm.mjs

# Type-check the platform sources against real gint headers.  Needs the gint
# include folder, which the fxSDK installs (fxsdk path shows where).
# This is not a build: it checks names, signatures and warnings only.
# gint's headers use __VA_OPT__ (a C23/C++20 feature also available in gnu17),
# so this check uses the fxSDK's own language level and no -pedantic.
GINT_WARN := -Wall -Wextra -Werror
GINT_INC ?= $(HOME)/.local/include
GINT_SRC := $(wildcard src/*.c) $(wildcard src/platform/*.c) \
            $(wildcard src/ui/*.c) $(wildcard src/apps/*.c) \
            $(wildcard spike/*.c)
gint-check:
	@test -d "$(GINT_INC)/gint" || { \
	  echo "gint headers not found in $(GINT_INC)/gint"; \
	  echo "set GINT_INC=/path/to/gint/include (see docs/toolchain.md)"; \
	  exit 2; }
	@mkdir -p build/gintstub/gint
	@printf '%s\n' \
	  '#ifndef GINT_CONFIG' \
	  '#define GINT_CONFIG' \
	  '#define GINT_OS_FXCG50 1' \
	  '#endif' > build/gintstub/gint/config.h
	@fail=0; for f in $(GINT_SRC); do \
	  printf '  %-28s ' "$$f"; \
	  if $(CC) -fsyntax-only -std=gnu17 $(GINT_WARN) -DFXCG50 \
	     -Ibuild/gintstub -I$(GINT_INC) -Isrc -Isrc/ui -Isrc/core -Isrc/apps \
	     "$$f"; then echo ok; else echo FAILED; fail=1; fi; \
	done; \
	if [ $$fail -ne 0 ]; then echo "GINT CHECK FAILED"; exit 1; fi; \
	echo "GINT CHECK PASSED ($$(echo $(GINT_SRC) | wc -w) files)"

clean:
	rm -rf build
