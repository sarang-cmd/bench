# Bench - one add-in for the Casio fx-CG 50.
#
#   fxsdk build-cg                      -> Bench-1.0.0.g3a
#   fxsdk build-cg -DBENCH_VERSION=0.0.3   -> Bench-0.0.3.g3a
#
# The add-in is one .g3a with five tools behind a home screen (Pacer,
# Uncertainty, Cards, Notes, Number theory) plus Settings and About.
#
# src/core is plain C with no gint includes and is tested on a PC by
# `make test`; src/platform, src/ui and src/apps hold every gint call.
# The host preview of the same screens is `make sim` (tools/sim).

cmake_minimum_required(VERSION 3.15)
project(Bench LANGUAGES C)

set(CMAKE_C_STANDARD 11)

# Version: -DBENCH_VERSION=x.y.z overrides; otherwise read from version.txt so
# the .g3a name always matches CHANGELOG.md and the git tag.
if(NOT DEFINED BENCH_VERSION)
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/version.txt" BENCH_VERSION)
  string(STRIP "${BENCH_VERSION}" BENCH_VERSION)
endif()
set(BENCH_VERSION "${BENCH_VERSION}" CACHE STRING "Bench version, e.g. 1.0.0")
message(STATUS "Bench version ${BENCH_VERSION}")

include(GenerateG3A)
include(Fxconv)
find_package(Gint 2.9 REQUIRED)

set(SOURCES
  src/main.c

  src/core/cards_parse.c
  src/core/color.c
  src/core/expr.c
  src/core/exprs.c
  src/core/help.c
  src/core/leitner.c
  src/core/mathfn.c
  src/core/md_layout.c
  src/core/md_parse.c
  src/core/numfmt.c
  src/core/numparse.c
  src/core/numtheory.c
  src/core/poly.c
  src/core/pacer.c
  src/core/settings.c
  src/core/sigfig.c
  src/core/triangle.c
  src/core/textsearch.c
  src/core/uncert.c

  src/platform/input.c
  src/platform/mem.c
  src/platform/screen.c
  src/platform/store.c

  src/ui/numtext.c
  src/ui/shapes.c
  src/ui/widgets.c

  src/apps/app_about.c
  src/apps/app_cards.c
  src/apps/app_function.c
  src/apps/app_help.c
  src/apps/app_menu.c
  src/apps/app_notes.c
  src/apps/app_numtheory.c
  src/apps/app_pacer.c
  src/apps/app_poly.c
  src/apps/app_settings.c
  src/apps/app_sigfig.c
  src/apps/app_triangle.c
  src/apps/app_uncert.c
  src/apps/registry.c
)

add_executable(bench ${SOURCES})

target_include_directories(bench PRIVATE
  src
  src/apps
  src/core
  src/ui
)

target_compile_definitions(bench PRIVATE BENCH_VERSION="${BENCH_VERSION}")
target_compile_options(bench PRIVATE -Wall -Wextra -Os)
target_link_libraries(bench Gint::Gint)

if("${FXSDK_PLATFORM_LONG}" STREQUAL fxCG50)
  generate_g3a(TARGET bench
    OUTPUT "Bench-${BENCH_VERSION}.g3a"
    NAME "Bench"
    VERSION "${BENCH_VERSION}"
    ICONS assets-cg/icon-uns.png assets-cg/icon-sel.png)
else()
  message(FATAL_ERROR
    "Bench targets the fx-CG 50 only (384x216 window, 396x224 framebuffer).")
endif()

# A copy of the .g3a at the top of the build folder, for the "drag it to the
# calculator" step in docs/toolchain.md.
#
# generate_g3a() writes the file next to the sources (its OUTPUT is resolved
# against CMAKE_CURRENT_SOURCE_DIR), so the path here has to be absolute: the
# command runs in the build directory, where a relative name does not exist and
# copy_if_different fails the link step.
add_custom_command(TARGET bench POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${CMAKE_CURRENT_SOURCE_DIR}/Bench-${BENCH_VERSION}.g3a"
          "${CMAKE_BINARY_DIR}/Bench.g3a")
