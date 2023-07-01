# [project]
cmake_policy(SET CMP0048 NEW) # For VERSION in project()
cmake_minimum_required(VERSION 3.2)
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

if(WTR_WATCHER_BUILD_RELEASE)
  wtr_add_bin_target(
    "wtr.${WTR_WATCHER_PROJECT_NAME}"
    "${WTR_WATCHER_SOURCE_SET}"
    "${COMPILE_OPTIONS_RELEASE}"
    "${LINK_OPTIONS}")
  if(WTR_WATCHER_BUILD_ASAN AND WTR_WATCHER_ALLOWED_ASAN)
    wtr_add_bin_target(
      "wtr.${WTR_WATCHER_PROJECT_NAME}.asan"
      "${WTR_WATCHER_SOURCE_SET}"
      "${COMPILE_OPTIONS_RELEASE_ASAN}"
      "${LINK_OPTIONS_ASAN}")
  endif()
  if(WTR_WATCHER_BUILD_MSAN AND WTR_WATCHER_ALLOWED_MSAN)
    wtr_add_bin_target(
      "wtr.${WTR_WATCHER_PROJECT_NAME}.msan"
      "${WTR_WATCHER_SOURCE_SET}"
      "${COMPILE_OPTIONS_RELEASE_MSAN}"
      "${LINK_OPTIONS_MSAN}")
  endif()
  if(WTR_WATCHER_BUILD_TSAN AND WTR_WATCHER_ALLOWED_TSAN)
    wtr_add_bin_target(
      "wtr.${WTR_WATCHER_PROJECT_NAME}.tsan"
      "${WTR_WATCHER_SOURCE_SET}"
      "${COMPILE_OPTIONS_RELEASE_TSAN}"
      "${LINK_OPTIONS_TSAN}")
  endif()
  if(WTR_WATCHER_BUILD_UBSAN AND WTR_WATCHER_ALLOWED_UBSAN)
    wtr_add_bin_target(
      "wtr.${WTR_WATCHER_PROJECT_NAME}.ubsan"
      "${WTR_WATCHER_SOURCE_SET}"
      "${COMPILE_OPTIONS_RELEASE_UBSAN}"
      "${LINK_OPTIONS_UBSAN}")
  endif()
endif()

if(WTR_WATCHER_BUILD_DEBUG)
  wtr_add_bin_target(
    "wtr.${WTR_WATCHER_PROJECT_NAME}.debug"
    "${WTR_WATCHER_SOURCE_SET}"
    "${COMPILE_OPTIONS_HIGH_ERR}"
    "${LINK_OPTIONS}")
  if(WTR_WATCHER_BUILD_ASAN AND WTR_WATCHER_ALLOWED_ASAN)
    wtr_add_bin_target(
      "wtr.${WTR_WATCHER_PROJECT_NAME}.debug.asan"
      "${WTR_WATCHER_SOURCE_SET}"
      "${COMPILE_OPTIONS_HIGH_ERR_ASAN}"
      "${LINK_OPTIONS_ASAN}")
  endif()
  if(WTR_WATCHER_BUILD_MSAN AND WTR_WATCHER_ALLOWED_MSAN)
    wtr_add_bin_target(
      "wtr.${WTR_WATCHER_PROJECT_NAME}.debug.msan"
      "${WTR_WATCHER_SOURCE_SET}"
      "${COMPILE_OPTIONS_HIGH_ERR_MSAN}"
      "${LINK_OPTIONS_MSAN}")
  endif()
  if(WTR_WATCHER_BUILD_TSAN AND WTR_WATCHER_ALLOWED_TSAN)
    wtr_add_bin_target(
      "wtr.${WTR_WATCHER_PROJECT_NAME}.debug.tsan"
      "${WTR_WATCHER_SOURCE_SET}"
      "${COMPILE_OPTIONS_HIGH_ERR_TSAN}"
      "${LINK_OPTIONS_TSAN}")
  endif()
  if(WTR_WATCHER_BUILD_UBSAN AND WTR_WATCHER_ALLOWED_UBSAN)
    wtr_add_bin_target(
      "wtr.${WTR_WATCHER_PROJECT_NAME}.debug.ubsan"
      "${WTR_WATCHER_SOURCE_SET}"
      "${COMPILE_OPTIONS_HIGH_ERR_UBSAN}"
      "${LINK_OPTIONS_UBSAN}")
  endif()
endif()

if(WTR_WATCHER_BUILD_TEST)
  wtr_add_test_bin_target(
    "wtr.${WTR_TEST_WATCHER_PROJECT_NAME}"
    "${WTR_TEST_WATCHER_SOURCE_SET}"
    "${COMPILE_OPTIONS_HIGH_ERR}"
    "${LINK_OPTIONS}")
  if(WTR_WATCHER_BUILD_ASAN AND WTR_WATCHER_ALLOWED_ASAN)
    wtr_add_test_bin_target(
      "wtr.${WTR_TEST_WATCHER_PROJECT_NAME}.asan"
      "${WTR_TEST_WATCHER_SOURCE_SET}"
      "${COMPILE_OPTIONS_HIGH_ERR_ASAN}"
      "${LINK_OPTIONS_ASAN}")
  endif()
  if(WTR_WATCHER_BUILD_MSAN AND WTR_WATCHER_ALLOWED_MSAN)
    wtr_add_test_bin_target(
      "wtr.${WTR_TEST_WATCHER_PROJECT_NAME}.msan"
      "${WTR_TEST_WATCHER_SOURCE_SET}"
      "${COMPILE_OPTIONS_HIGH_ERR_MSAN}"
      "${LINK_OPTIONS_MSAN}")
  endif()
  if(WTR_WATCHER_BUILD_TSAN AND WTR_WATCHER_ALLOWED_TSAN)
    wtr_add_test_bin_target(
      "wtr.${WTR_TEST_WATCHER_PROJECT_NAME}.tsan"
      "${WTR_TEST_WATCHER_SOURCE_SET}"
      "${COMPILE_OPTIONS_HIGH_ERR_TSAN}"
      "${LINK_OPTIONS_TSAN}")
  endif()
  if(WTR_WATCHER_BUILD_UBSAN AND WTR_WATCHER_ALLOWED_UBSAN)
    wtr_add_test_bin_target(
      "wtr.${WTR_TEST_WATCHER_PROJECT_NAME}.ubsan"
      "${WTR_TEST_WATCHER_SOURCE_SET}"
      "${COMPILE_OPTIONS_HIGH_ERR_UBSAN}"
      "${LINK_OPTIONS_UBSAN}")
  endif()
endif()

if(WTR_WATCHER_BUILD_BENCH)
  wtr_add_bench_bin_target(
    "wtr.${WTR_BENCH_WATCHER_PROJECT_NAME}"
    "${WTR_BENCH_WATCHER_SOURCE_SET}"
    "${COMPILE_OPTIONS_RELEASE}"
    "${LINK_OPTIONS}")
endif()
