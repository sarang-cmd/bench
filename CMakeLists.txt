# Bench - Casio fx-CG50 add-in
#
# Build with the fxSDK:
#     fxsdk build-cg          (see docs/toolchain.md)
# The native core tests are separate: `make test` (no fxSDK needed).
#
# This file builds the M0 blank add-in.  The core modules and the app screens
# join SOURCES at the milestones that introduce them (see docs/plan.md 7).

cmake_minimum_required(VERSION 3.15)
project(Bench LANGUAGES C)

# Bench version; also used for the .g3a output name.  Bump at each milestone:
# 0.0.1 M0, 0.0.2 spike, 0.0.3 shell, 0.1.0 Pacer ... 1.0.0 release.
set(BENCH_VERSION "0.0.1" CACHE STRING "Bench version")

include(GenerateG1A)
include(GenerateG3A)
include(Fxconv)
find_package(Gint 2.9 REQUIRED)

set(SOURCES
  src/main.c
)

# No converted assets yet; the icons go straight to the .g3a.
set(ASSETS
)
set(ASSETS_cg
)

fxconv_declare_assets(${ASSETS} ${ASSETS_cg} WITH_METADATA)

add_executable(bench ${SOURCES} ${ASSETS} ${ASSETS_${FXSDK_PLATFORM}})
target_compile_options(bench PRIVATE -Wall -Wextra -Os)
target_link_libraries(bench Gint::Gint)

if("${FXSDK_PLATFORM_LONG}" STREQUAL fxCG50)
  generate_g3a(TARGET bench OUTPUT "Bench-${BENCH_VERSION}.g3a"
    NAME "Bench" VERSION "${BENCH_VERSION}"
    ICONS assets-cg/icon-uns.png assets-cg/icon-sel.png)
else()
  message(FATAL_ERROR "Bench targets the fx-CG 50 only; build with fxsdk build-cg")
endif()
