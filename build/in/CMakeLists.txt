# [project]
cmake_policy(SET CMP0048 NEW) # For VERSION in project()
cmake_minimum_required(VERSION 3.2)
project(
  wtr.watcher
  VERSION 0.6.0 # hook: tool/release
  DESCRIPTION "watcher: a filesystem watcher"
  HOMEPAGE_URL "github.com/e-dant/watcher"
  LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# [options and configuration, esp. tests and sanitizers]
include(wtr.cmake_options_watcher)

# [build type selection]
if(WTR_WATCHER_USE_TEST)
  include(wtr.test_watcher)
endif()
if(WTR_WATCHER_USE_RELEASE) # This is the default
  include(wtr.watcher)
endif()
