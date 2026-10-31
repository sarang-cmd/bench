# Native (PC) build for the Bench core.
#
#   make test        build and run every native test in tests/
#   make py-test     run the tools/ tests
#   make test-all    both of the above
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

test: $(TEST_BIN)
	@echo
	@echo "== native core tests =="
	@fail=0; for t in $(TEST_BIN); do ./$$t || fail=1; done; \
	 if [ $$fail -ne 0 ]; then echo "NATIVE TESTS FAILED"; exit 1; fi; \
	 echo "NATIVE TESTS PASSED ($(words $(TEST_BIN)) test programs)"

py-test:
	@echo "== python tool tests =="
	$(PYTHON) -m unittest discover -s tools -p "test_*.py" -v

# Type-check the platform sources against real gint headers.  Needs the gint
# include folder, which the fxSDK installs (fxsdk path shows where).
# This is not a build: it checks names, signatures and warnings only.
# gint's headers use __VA_OPT__ (a C23/C++20 feature also available in gnu17),
# so this check uses the fxSDK's own language level and no -pedantic.
GINT_WARN := -Wall -Wextra -Werror
GINT_INC ?= $(HOME)/.local/include
gint-check:
	@test -d "$(GINT_INC)/gint" || { \
	  echo "gint headers not found in $(GINT_INC)/gint"; \
	  echo "set GINT_INC=/path/to/gint/include (see docs/toolchain.md)"; \
	  exit 2; }
	@mkdir -p build/gintstub/gint
	@printf '%s\n' \
	  '#ifndef GINT_CONFIG' \
	  '#define GINT_CONFIG' \
	  '#include <gint/defs/types.h>' \
	  '#define GINT_VERSION "2.10.0"' \
	  '#define GINT_HASH 0x0000000' \
	  '#endif' > build/gintstub/gint/config.h
	$(CC) -fsyntax-only -std=gnu17 $(GINT_WARN) -DFXCG50 \
	  -Ibuild/gintstub -I$(GINT_INC) src/main.c
	$(CC) -fsyntax-only -std=gnu17 $(GINT_WARN) -DFXCG50 \
	  -Ibuild/gintstub -I$(GINT_INC) spike/main.c
	@echo "gint header check passed: src/main.c and spike/main.c"

.PHONY: all test test-all py-test gint-check clean
