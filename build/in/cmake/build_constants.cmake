set(WTR_WATCHER_CXX_STD 20)

if(MSVC)
# It's not that we don't want these, it's just that I hate Windows.
# Also, MSVC doesn't support some of these arguments, so it's not possible.
set(COMPILE_OPTIONS_HIGH_ERR)
else()
set(COMPILE_OPTIONS_HIGH_ERR
  "-Wall"
  "-Wextra"
  "-Werror"
  "-Wno-unused-function"
  "-Wno-unneeded-internal-declaration")
endif()
if(MSVC)
set(COMPILE_OPTIONS_RELEASE
  "-O2"
  "${COMPILE_OPTIONS_HIGH_ERR}")
else()
set(COMPILE_OPTIONS_RELEASE
  "-O3"
  "${COMPILE_OPTIONS_HIGH_ERR}")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Android")
  # Android's stdlib ("bionic") doesn't need
  # to be linked with a pthread-like library.
  set(LINK_LIBRARIES)
else()
  find_package(Threads REQUIRED)
  set(LINK_LIBRARIES "Threads::Threads")
  if(APPLE)
    list(APPEND LINK_LIBRARIES
      "-framework CoreFoundation"
      "-framework CoreServices")
  endif()
endif()

set(TEST_LINK_LIBRARIES
  "${LINK_LIBRARIES}"
  "snitch::snitch")

set(WTR_WATCHER_SOURCE_SET
  "${WTR_WATCHER_ROOT_SOURCE_DIR}/src/wtr.watcher/main.cpp")

set(WTR_TEST_WATCHER_PROJECT_NAME
  "test_watcher")
set(WTR_TEST_WATCHER_SOURCE_SET
  "test_concurrent_watch_targets"
  "test_watch_targets"
  "test_new_directories"
  "test_simple")
list(TRANSFORM WTR_TEST_WATCHER_SOURCE_SET PREPEND
  "${WTR_WATCHER_ROOT_SOURCE_DIR}/src/${WTR_TEST_WATCHER_PROJECT_NAME}/")
list(TRANSFORM WTR_TEST_WATCHER_SOURCE_SET APPEND ".cpp")

set(WTR_BENCH_WATCHER_PROJECT_NAME
  "bench_watcher")
set(WTR_BENCH_WATCHER_SOURCE_SET
  "bench_concurrent_watch_targets")
list(TRANSFORM WTR_BENCH_WATCHER_SOURCE_SET PREPEND
  "${WTR_WATCHER_ROOT_SOURCE_DIR}/src/${WTR_BENCH_WATCHER_PROJECT_NAME}/")
list(TRANSFORM WTR_BENCH_WATCHER_SOURCE_SET APPEND ".cpp")

set(INCLUDE_PATH_SINGLE_HEADER
  "${WTR_WATCHER_ROOT_SOURCE_DIR}/include")
set(INCLUDE_PATH_DEVEL
  "${WTR_WATCHER_ROOT_SOURCE_DIR}/devel/include")

set(LINK_OPTIONS)

set(COMPILE_OPTIONS_HIGH_ERR_ASAN "${COMPILE_OPTIONS_HIGH_ERR}"
  "-fno-omit-frame-pointer" "-fsanitize=address")
set(COMPILE_OPTIONS_HIGH_ERR_MSAN "${COMPILE_OPTIONS_HIGH_ERR}"
  "-fno-omit-frame-pointer" "-fsanitize=memory")
set(COMPILE_OPTIONS_HIGH_ERR_TSAN "${COMPILE_OPTIONS_HIGH_ERR}"
  "-fno-omit-frame-pointer" "-fsanitize=thread")
set(COMPILE_OPTIONS_HIGH_ERR_UBSAN "${COMPILE_OPTIONS_HIGH_ERR}"
  "-fno-omit-frame-pointer" "-fsanitize=undefined")
set(COMPILE_OPTIONS_HIGH_ERR_STACKSAN "${COMPILE_OPTIONS_HIGH_ERR}"
  "-fno-omit-frame-pointer" "-fsanitize=safe-stack")
set(COMPILE_OPTIONS_HIGH_ERR_DATAFLOWSAN "${COMPILE_OPTIONS_HIGH_ERR}"
  "-fno-omit-frame-pointer" "-fsanitize=dataflow")
set(COMPILE_OPTIONS_HIGH_ERR_CFISAN "${COMPILE_OPTIONS_HIGH_ERR}"
  "-fno-omit-frame-pointer" "-fsanitize=cfi")
set(COMPILE_OPTIONS_HIGH_ERR_KCFISAN "${COMPILE_OPTIONS_HIGH_ERR}"
  "-fno-omit-frame-pointer" "-fsanitize=kcfi")
set(COMPILE_OPTIONS_RELEASE_ASAN "${COMPILE_OPTIONS_RELEASE}"
  "-fno-omit-frame-pointer" "-fsanitize=address")
set(COMPILE_OPTIONS_RELEASE_MSAN "${COMPILE_OPTIONS_RELEASE}"
  "-fno-omit-frame-pointer" "-fsanitize=memory")
set(COMPILE_OPTIONS_RELEASE_TSAN "${COMPILE_OPTIONS_RELEASE}"
  "-fno-omit-frame-pointer" "-fsanitize=thread")
set(COMPILE_OPTIONS_RELEASE_UBSAN "${COMPILE_OPTIONS_RELEASE}"
  "-fno-omit-frame-pointer" "-fsanitize=undefined")
set(COMPILE_OPTIONS_RELEASE_STACKSAN "${COMPILE_OPTIONS_RELEASE}"
  "-fno-omit-frame-pointer" "-fsanitize=safe-stack")
set(COMPILE_OPTIONS_RELEASE_DATAFLOWSAN "${COMPILE_OPTIONS_RELEASE}"
  "-fno-omit-frame-pointer" "-fsanitize=dataflow")
set(COMPILE_OPTIONS_RELEASE_CFISAN "${COMPILE_OPTIONS_RELEASE}"
  "-fno-omit-frame-pointer" "-fsanitize=cfi")
set(COMPILE_OPTIONS_RELEASE_KCFISAN "${COMPILE_OPTIONS_RELEASE}"
  "-fno-omit-frame-pointer" "-fsanitize=kcfi")
set(LINK_OPTIONS_ASAN "${LINK_OPTIONS}"
  "-fno-omit-frame-pointer" "-fsanitize=address")
set(LINK_OPTIONS_MSAN "${LINK_OPTIONS}"
  "-fno-omit-frame-pointer" "-fsanitize=memory")
set(LINK_OPTIONS_TSAN "${LINK_OPTIONS}"
  "-fno-omit-frame-pointer" "-fsanitize=thread")
set(LINK_OPTIONS_UBSAN "${LINK_OPTIONS}"
  "-fno-omit-frame-pointer" "-fsanitize=undefined")
set(LINK_OPTIONS_STACKSAN "${LINK_OPTIONS}"
  "-fno-omit-frame-pointer" "-fsanitize=safe-stack")
set(LINK_OPTIONS_DATAFLOWSAN "${LINK_OPTIONS}"
  "-fno-omit-frame-pointer" "-fsanitize=dataflow")
set(LINK_OPTIONS_CFISAN "${LINK_OPTIONS}"
  "-fno-omit-frame-pointer" "-fsanitize=cfi")
set(LINK_OPTIONS_KCFISAN "${LINK_OPTIONS}"
  "-fno-omit-frame-pointer" "-fsanitize=kcfi")