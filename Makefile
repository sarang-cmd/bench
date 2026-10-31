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

test: $(TEST_BIN) check-screens
	@echo
	@echo "== native core tests =="
	@fail=0; for t in $(TEST_BIN); do ./$$t || fail=1; done; \
	 if [ $$fail -ne 0 ]; then echo "NATIVE TESTS FAILED"; exit 1; fi; \
	 echo "NATIVE TESTS PASSED ($(words $(TEST_BIN)) test programs)"

# Rewrite the README's generated badge and stats blocks (CI checks them).
stats:
	@$(PYTHON) tools/gen_readme_stats.py

# The spike add-in as one patch file (see docs/spike.md).
spike-patch:
	@$(PYTHON) tools/gen_spike_patch.py

py-test:
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
              -DBENCH_TRACE \
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
