set(WTR_WATCHER_CXX_STD 20)

set(IS_CC_CLANG 0)
set(IS_CC_ANYCLANG 0)
set(IS_CC_APPLECLANG 0)
set(IS_CC_GCC 0)
set(IS_CC_MSVC 0)
if(CMAKE_CXX_COMPILER_ID STREQUAL     "MSVC")
  set(IS_CC_MSVC 1)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(IS_CC_GCC 1)
elseif(CMAKE_CXX_COMPILER_ID MATCHES  "Clang")
  set(IS_CC_ANYCLANG 1)
  if(CMAKE_CXX_COMPILER_ID STREQUAL   "AppleClang")
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
  "main") # TODO this should be more clear. We have tons of mains.
list(TRANSFORM WTR_WATCHER_SOURCE_SET PREPEND
  "src/wtr.watcher/") # TODO get wtr. out of here
list(TRANSFORM WTR_WATCHER_SOURCE_SET APPEND ".cpp")

set(WTR_TEST_WATCHER_SOURCE_SET
  "test_concurrent_watch_targets"
  "test_watch_targets"
  "test_new_directories"
  "test_simple")
list(TRANSFORM WTR_TEST_WATCHER_SOURCE_SET PREPEND
  "src/test_watcher/")
list(TRANSFORM WTR_TEST_WATCHER_SOURCE_SET APPEND ".cpp")

set(WTR_BENCH_WATCHER_SOURCE_SET
  "bench_concurrent_watch_targets")
list(TRANSFORM WTR_BENCH_WATCHER_SOURCE_SET PREPEND
  "src/bench_watcher/")
list(TRANSFORM WTR_BENCH_WATCHER_SOURCE_SET APPEND ".cpp")

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
