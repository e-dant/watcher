# [project]
cmake_policy(SET CMP0048 NEW) # For VERSION in project()
cmake_minimum_required(VERSION 3.2)
project(
  wtr.watcher
  VERSION 0.8.8 # hook: tool/release
  DESCRIPTION "watcher: a filesystem watcher"
  HOMEPAGE_URL "github.com/e-dant/watcher"
  LANGUAGES CXX)

set(WTR_WATCHER_ROOT_SOURCE_DIR # This is an option, despite not being `option()`
  "${CMAKE_CURRENT_SOURCE_DIR}" CACHE STRING # Default value
  "Root source directory of this project")
list(APPEND CMAKE_MODULE_PATH
  "${WTR_WATCHER_ROOT_SOURCE_DIR}/build/in/cmake")

# WTR_WATCHER_ROOT_SOURCE_DIR Must be set before including these.
# (Otherwise, we might not be able to find them.)
include(build_options)
include(build_constants)
include(build_functions)

if(WTR_WATCHER_USE_TEST AND NOT WIN32)
  wtr_add_test_target(
    "wtr.${WTR_TEST_WATCHER_PROJECT_NAME}"
    "${COMPILE_OPTIONS_HIGH_ERR}"
    "${LINK_OPTIONS}")

  wtr_add_test_target(
    "wtr.${WTR_TEST_WATCHER_PROJECT_NAME}.asan"
    "${COMPILE_OPTIONS_HIGH_ERR_ASAN}"
    "${LINK_OPTIONS_ASAN}")

  if(NOT APPLE)
    wtr_add_test_target(
      "wtr.${WTR_TEST_WATCHER_PROJECT_NAME}.msan"
      "${COMPILE_OPTIONS_HIGH_ERR_MSAN}"
      "${LINK_OPTIONS_MSAN}")
  endif()

  wtr_add_test_target(
    "wtr.${WTR_TEST_WATCHER_PROJECT_NAME}.tsan"
    "${COMPILE_OPTIONS_HIGH_ERR_TSAN}"
    "${LINK_OPTIONS_TSAN}")

  wtr_add_test_target(
    "wtr.${WTR_TEST_WATCHER_PROJECT_NAME}.ubsan"
    "${COMPILE_OPTIONS_HIGH_ERR_UBSAN}"
    "${LINK_OPTIONS_UBSAN}")
endif()

if(WTR_WATCHER_USE_BENCH)
  wtr_add_bench_target(
    "wtr.${WTR_BENCH_WATCHER_PROJECT_NAME}"
    "${COMPILE_OPTIONS_RELEASE}"
    "${LINK_OPTIONS}")
endif()

if(WTR_WATCHER_USE_RELEASE)
  add_executable("${PROJECT_NAME}"
    "${WTR_WATCHER_SOURCE_SET}")
  set_property(TARGET "${PROJECT_NAME}" PROPERTY
    CXX_STANDARD "${WTR_WATCHER_CXX_STD}")
  target_compile_options("${PROJECT_NAME}" PRIVATE
    "${COMPILE_OPTIONS_RELEASE}")
  target_link_options("${PROJECT_NAME}" PRIVATE
    "${LINK_OPTIONS}")
  target_include_directories("${PROJECT_NAME}" PUBLIC
    "${INCLUDE_PATH_SINGLE_HEADER}")
  target_link_libraries("${PROJECT_NAME}" PRIVATE
    "${LINK_LIBRARIES}")
  if(APPLE)
    set_property(TARGET "${PROJECT_NAME}" PROPERTY
      XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "org.wtr.watcher")
  endif()
  install(
    TARGETS                   "${PROJECT_NAME}"
    LIBRARY DESTINATION       "${CMAKE_INSTALL_LIBDIR}"
    BUNDLE DESTINATION        "${CMAKE_INSTALL_PREFIX}/bin"
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
endif()
