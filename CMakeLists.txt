#
# The relative source and module paths seem to work inconsistently on Windows
# and Nix (flakes). This file used to live around the build/in and
# build/in/cmake directories. For now, it seems easiest to leave it here, as
# one megafile, in the project's root directory.
#

cmake_policy(SET CMP0048 NEW) # For VERSION in project()
cmake_policy(SET CMP0069 NEW) # For better IPO support
cmake_minimum_required(VERSION 3.9)

project(
  wtr.watcher
  VERSION 0.9.5 # hook: tool/release
  DESCRIPTION "watcher: a filesystem watcher"
  HOMEPAGE_URL "github.com/e-dant/watcher"
  LANGUAGES
    CXX
    C
)

#
# Options, Variable & Constants
#

option(WTR_WATCHER_BUILD_TEST  "Create a target for the test programs"                ON)
option(WTR_WATCHER_BUILD_ASAN  "Create a target for the address sanitizer"            ON)
option(WTR_WATCHER_BUILD_MSAN  "Create a target for the memory sanitizer"             ON)
option(WTR_WATCHER_BUILD_TSAN  "Create a target for the thread sanitizer"             ON)
option(WTR_WATCHER_BUILD_UBSAN "Create a target for the undefined behavior sanitizer" ON)

set(WTR_WATCHER_CXX_STD 17)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

set(IS_CC_CLANG 0)
set(IS_CC_ANYCLANG 0)
set(IS_CC_APPLECLANG 0)
set(IS_CC_GCC 0)
set(IS_CC_MSVC 0)
if(CMAKE_CXX_COMPILER_ID     STREQUAL "MSVC")
  set(IS_CC_MSVC 1)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(IS_CC_GCC 1)
elseif(CMAKE_CXX_COMPILER_ID MATCHES  "Clang")
  set(IS_CC_ANYCLANG 1)
  if(CMAKE_CXX_COMPILER_ID   STREQUAL "AppleClang")
    set(IS_CC_APPLECLANG 1)
  else()
    set(IS_CC_CLANG 1)
  endif()
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

if(NOT IS_CC_MSVC)
  # It's not that we don't want these, it's just that I hate Windows.
  # Also, MSVC doesn't support some of these arguments, so it's not possible.
  set(COMPILE_OPTIONS
    "${COMPILE_OPTIONS}"
    "-Wall"
    "-Wextra"
    "-Werror"
    "-Wno-unused-function"
    "-Wno-unneeded-internal-declaration")
endif()

if(NOT WIN32 AND NOT IS_CC_MSVC)
  # TODO: strict-aliasing
  set(COMPILE_OPTIONS
    "${COMPILE_OPTIONS}"
    "-fno-exceptions"
    "-fno-rtti"
    "-fstrict-enums"
    "-fstrict-overflow")
  if(IS_CC_CLANG)
    set(COMPILE_OPTIONS
      "${COMPILE_OPTIONS}"
      "-fstrict-return"
      "-fstrict-float-cast-overflow")
  endif()
  if(NOT IS_CC_APPLECLANG)
    set(COMPILE_OPTIONS
      "${COMPILE_OPTIONS}"
      "-fexpensive-optimizations"
      "-fwhole-program")
  endif()
endif()

if(ANDROID)
  # Android's stdlib ("bionic") doesn't need to be linked with (p)threads.
  set(LINK_LIBRARIES
    "${LINK_LIBRARIES}")
else()
  find_package(Threads REQUIRED)
  set(LINK_LIBRARIES
    "${LINK_LIBRARIES}"
    "Threads::Threads")
  if(APPLE)
    list(APPEND LINK_LIBRARIES
      "-framework CoreFoundation"
      "-framework CoreServices")
  endif()
endif()

set(WTR_WATCHER_SOURCE_SET
  "src/wtr/watcher/main.cpp")

set(WTR_TEST_WATCHER_SOURCE_SET
  "devel/src/wtr/test_watcher/test_concurrent_watch_targets.cpp"
  "devel/src/wtr/test_watcher/test_watch_targets.cpp"
  "devel/src/wtr/test_watcher/test_new_directories.cpp"
  "devel/src/wtr/test_watcher/test_simple.cpp"
  "devel/src/wtr/test_watcher/test_performance.cpp")

set(INCLUDE_PATH_SINGLE_HEADER
  "include")
set(INCLUDE_PATH_DEVEL
  "devel/include")

set(WTR_WATCHER_ALLOWED_ASAN        0)
set(WTR_WATCHER_ALLOWED_MSAN        0)
set(WTR_WATCHER_ALLOWED_TSAN        0)
set(WTR_WATCHER_ALLOWED_UBSAN       0)
set(WTR_WATCHER_ALLOWED_STACKSAN    0)
set(WTR_WATCHER_ALLOWED_DATAFLOWSAN 0)
set(WTR_WATCHER_ALLOWED_CFISAN      0)
set(WTR_WATCHER_ALLOWED_KCFISAN     0)
if(NOT WIN32)
  set(WTR_WATCHER_ALLOWED_ASAN      1)
endif()
if(IS_CC_CLANG)
  set(WTR_WATCHER_ALLOWED_MSAN      1)
endif()
if(NOT (ANDROID OR WIN32))
  set(WTR_WATCHER_ALLOWED_TSAN      1)
endif()
if(NOT WIN32)
  set(WTR_WATCHER_ALLOWED_UBSAN     1)
endif()
set(WTR_WATCHER_ALLOWED_asan        ${WTR_WATCHER_ALLOWED_ASAN})
set(WTR_WATCHER_ALLOWED_msan        ${WTR_WATCHER_ALLOWED_MSAN})
set(WTR_WATCHER_ALLOWED_tsan        ${WTR_WATCHER_ALLOWED_TSAN})
set(WTR_WATCHER_ALLOWED_ubsan       ${WTR_WATCHER_ALLOWED_UBSAN})
set(WTR_WATCHER_ALLOWED_stacksan    ${WTR_WATCHER_ALLOWED_STACKSAN})
set(WTR_WATCHER_ALLOWED_dataflowsan ${WTR_WATCHER_ALLOWED_DATAFLOWSAN})
set(WTR_WATCHER_ALLOWED_cfisan      ${WTR_WATCHER_ALLOWED_CFISAN})
set(WTR_WATCHER_ALLOWED_kcfisan     ${WTR_WATCHER_ALLOWED_KCFISAN})
set(SAN_NAMES                       "asan" "msan" "tsan" "ubsan")
set(CCLL_EXTOPT_SET_ASAN            "-fno-omit-frame-pointer" "-fsanitize=address")
set(CCLL_EXTOPT_SET_MSAN            "-fno-omit-frame-pointer" "-fsanitize=memory")
set(CCLL_EXTOPT_SET_TSAN            "-fno-omit-frame-pointer" "-fsanitize=thread")
set(CCLL_EXTOPT_SET_UBSAN           "-fno-omit-frame-pointer" "-fsanitize=undefined")
set(CCLL_EXTOPT_SET_STACKSAN        "-fno-omit-frame-pointer" "-fsanitize=safe-stack")
set(CCLL_EXTOPT_SET_DATAFLOWSAN     "-fno-omit-frame-pointer" "-fsanitize=dataflow")
set(CCLL_EXTOPT_SET_CFISAN          "-fno-omit-frame-pointer" "-fsanitize=cfi")
set(CCLL_EXTOPT_SET_KCFISAN         "-fno-omit-frame-pointer" "-fsanitize=kcfi")

set(SAN_SUPPORTED)
foreach(SAN ${SAN_NAMES})
  if(WTR_WATCHER_ALLOWED_${SAN})
    list(APPEND SAN_SUPPORTED ${SAN})
  endif()
endforeach()

message(STATUS "Supported sanitizers on ${CMAKE_SYSTEM}/${CMAKE_CXX_COMPILER_ID}: ${SAN_SUPPORTED}")

#
# Functions
#

function(wtr_add_bin_target
      NAME
      BIN_COMPONENT_NAME
      IS_TEST
      SRC_SET
      CC_OPT_SET
      LL_OPT_SET
      INCLUDE_PATH
      LLIB_SET)
  include(CheckIPOSupported)
  include(FetchContent)
  include(GNUInstallDirs)
  add_executable("${NAME}" "${SRC_SET}")
  set_property(TARGET "${NAME}" PROPERTY CXX_STANDARD "${WTR_WATCHER_CXX_STD}")
  target_compile_options("${NAME}" PRIVATE "${CC_OPT_SET}")
  target_link_options("${NAME}" PRIVATE "${LL_OPT_SET}")
  target_include_directories("${NAME}" PUBLIC "${INCLUDE_PATH}")
  target_link_libraries("${NAME}" PRIVATE "${LLIB_SET}")
  check_ipo_supported(RESULT ipo_supported)
  if(ipo_supported)
    set_property(TARGET "${NAME}" PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
  endif()
  if(APPLE)
  set_property(
    TARGET "${NAME}"
    PROPERTY XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "org.${NAME}")
  endif()

  if(IS_TEST)
    if(DEFINED ENV{WTR_WATCHER_USE_SYSTEM_SNITCH})
      find_package(snitch REQUIRED)
    else()
      FetchContent_Declare(
        snitch
        GIT_REPOSITORY https://github.com/cschreib/snitch.git
        # v1.1.1 Doesn't show time as nice as v1.0.0
        # Tuesday, June 29th, 2023 @ v1.1.1
        GIT_TAG        5ad2fffebf31f3e6d56c2c0ab27bc45d01da2f05
        # Friday, January 20th, 2023 @ v1.0.0
        # GIT_TAG        ea200a0830394f8e0ef732064f0935a77c003bd6
        # Saturday, January 7th, 2023 @ main
        # GIT_TAG        8165d6c85353f9c302ce05f1c1c47dcfdc6aeb2c
        # Tuesday, December 18th, 2022 @ v0.1.3
        # GIT_TAG        f313bccafe98aaef617af3bf457d091d8d50cdcd
        # Friday, December 2nd, 2022 @ main
        # GIT_TAG        c0b6ac4efe4019e4846e8967fe21de864b0cc1ed
      )
      FetchContent_MakeAvailable(snitch)
    endif()
  endif()

  if(BIN_COMPONENT_NAME)
    install(
      TARGETS "${NAME}"
      DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
      COMPONENT "${BIN_COMPONENT_NAME}")
  endif()
endfunction()

function(wtr_add_hdr_target NAME INCLPATH HDR_NAME)
  include(GNUInstallDirs)
  add_library(
    "${NAME}"
    INTERFACE
    "${INCLPATH}/wtr/${HDR_NAME}")
  target_include_directories(
    "${NAME}"
    INTERFACE
    "${INCLPATH}")
  install(
    FILES "${INCLPATH}/wtr/${HDR_NAME}"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/wtr"
    COMPONENT "include")
endfunction()

function(wtr_add_rel_bin_target NAME SRC_SET)
  wtr_add_bin_target(
    "${NAME}"
    "bin"
    "OFF" # is test
    "${SRC_SET}"
    "${COMPILE_OPTIONS}"
    "${LINK_OPTIONS}"
    "${INCLUDE_PATH_SINGLE_HEADER}"
    "${LINK_LIBRARIES}")
endfunction()

function(wtr_add_test_bin_target NAME SRC_SET)
  wtr_add_bin_target(
    "${NAME}"
    "test-bin"
    "ON"  # is test
    "${SRC_SET}"
    "${COMPILE_OPTIONS}"
    "${LINK_OPTIONS}"
    "${INCLUDE_PATH_DEVEL}"
    "${LINK_LIBRARIES};snitch::snitch")
endfunction()

function(wtr_add_san_test_bin_target NAME SRC_SET SAN)
  wtr_add_bin_target(
    "${NAME}.${SAN}"
    "test-bin"
    "ON" # is test
    "${SRC_SET}"
    "${COMPILE_OPTIONS};${CCLL_EXTOPT_SET_${SAN}}"
    "${LINK_OPTIONS};${CCLL_EXTOPT_SET_${SAN}}"
    "${INCLUDE_PATH_DEVEL}"
    "${LINK_LIBRARIES};snitch::snitch")
endfunction()

function(wtr_add_autosan_bin_target NAME SRC_SET)
  wtr_add_rel_bin_target("${NAME}" "${SRC_SET}")
  foreach(SAN ${SAN_NAMES})
    if(WTR_WATCHER_ALLOWED_${SAN})
      wtr_add_bin_target(
        "${NAME}.${SAN}"
        "san-bin"
        "OFF" # is test
        "${SRC_SET}"
        "${COMPILE_OPTIONS};${CCLL_EXTOPT_SET_${SAN}}"
        "${LINK_OPTIONS};${CCLL_EXTOPT_SET_${SAN}}"
        "${INCLUDE_PATH_SINGLE_HEADER}"
        "${LINK_LIBRARIES}")
    endif()
  endforeach()
endfunction()

function(wtr_add_autosan_test_bin_target NAME SRC_SET)
  wtr_add_test_bin_target("${NAME}" "${SRC_SET}")
  foreach(SAN ${SAN_NAMES})
    if(WTR_WATCHER_ALLOWED_${SAN})
      wtr_add_san_test_bin_target("${NAME}" "${SRC_SET}" "${SAN}")
    endif()
  endforeach()
endfunction()

#
# Actual work
#

wtr_add_hdr_target(
  "wtr.hdr_watcher"
  "${INCLUDE_PATH_SINGLE_HEADER}"
  "watcher.hpp")

wtr_add_autosan_bin_target(
  "wtr.watcher"
  "${WTR_WATCHER_SOURCE_SET}")

wtr_add_autosan_test_bin_target(
  "wtr.test_watcher"
  "${WTR_TEST_WATCHER_SOURCE_SET}")

# Used in the tool/test suite. Platforms vary
# in their mv(1) implementations. We smooth
# over that by using the `rename` system call,
# which doesn't vary much at all.
wtr_add_bin_target(
  "portable-destructive-rename"
  "test-bin"
  "OFF"
  "devel/src/portable-destructive-rename/main.c"
  ""
  ""
  ""
  "")

