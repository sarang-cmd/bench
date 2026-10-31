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

test-all: test py-test

clean:
	rm -rf build
	rm -rf tools/__pycache__

.PHONY: all test test-all py-test clean
