cmake_policy(SET CMP0048 NEW) # For VERSION in project()
cmake_policy(SET CMP0069 NEW) # For better IPO support
cmake_minimum_required(VERSION 3.9)

project(
  wtr.watcher
  VERSION 0.8.8 # hook: tool/release
  DESCRIPTION "watcher: a filesystem watcher"
  HOMEPAGE_URL "github.com/e-dant/watcher"
  LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "build/in/cmake")

include(build_options)
include(build_constants)
include(build_functions)

wtr_add_autosan_bin_target(
  "wtr.watcher"
  "${WTR_WATCHER_SOURCE_SET}"
  "${COMPILE_OPTIONS}"
  "${LINK_OPTIONS}")

wtr_add_autosan_test_bin_target(
  "wtr.test_watcher"
  "${WTR_TEST_WATCHER_SOURCE_SET}"
  "${COMPILE_OPTIONS}"
  "${LINK_OPTIONS}")

wtr_add_bench_bin_target(
  "wtr.bench_watcher"
  "${WTR_BENCH_WATCHER_SOURCE_SET}"
  "${COMPILE_OPTIONS}"
  "${LINK_OPTIONS}")
