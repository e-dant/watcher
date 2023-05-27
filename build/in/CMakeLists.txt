# [project]
cmake_policy(SET CMP0048 NEW) # For VERSION in project()
cmake_minimum_required(VERSION 3.2)
project(
  wtr.watcher
  VERSION 0.8.7 # hook: tool/release
  DESCRIPTION "watcher: a filesystem watcher"
  HOMEPAGE_URL "github.com/e-dant/watcher"
  LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# The WTR_WATCHER_USE_* options selected here by defaults or by user choice.
# Option WTR_WATCHER_USE_RELEASE is the default.
include(wtr.cmake_options_watcher)

if(WTR_WATCHER_USE_TEST)
  include(wtr.test_watcher)
endif()

if(WTR_WATCHER_USE_BENCH)
  include(wtr.bench_watcher)
endif()

if(WTR_WATCHER_USE_RELEASE)
  include(wtr.watcher)
endif()
