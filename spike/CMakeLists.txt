# Bench spike add-in (M1).  Throwaway: delete this folder after the spike.
#
# Build:  cd spike && fxsdk build-cg
# Output: Bench-spike-0.0.2.g3a

cmake_minimum_required(VERSION 3.15)
project(BenchSpike LANGUAGES C)

include(GenerateG3A)
include(Fxconv)
find_package(Gint 2.9 REQUIRED)

set(SOURCES
  main.c
)

fxconv_declare_assets()

add_executable(bench_spike ${SOURCES})
target_compile_options(bench_spike PRIVATE -Wall -Wextra -Os)
target_link_libraries(bench_spike Gint::Gint)

if("${FXSDK_PLATFORM_LONG}" STREQUAL fxCG50)
  generate_g3a(TARGET bench_spike OUTPUT "Bench-spike-0.0.2.g3a"
    NAME "BenchSpk" VERSION "0.0.2"
    ICONS ../assets-cg/icon-uns.png ../assets-cg/icon-sel.png)
else()
  message(FATAL_ERROR "the spike targets the fx-CG 50 only")
endif()
