# [options: defaults: includes]
include(FetchContent)
include(GNUInstallDirs)

# [options: defaults: sources and prerequisites]
set(SOURCES "../../src/watcher/main.cpp")
set(INCLUDE_PATH)
# [options: cc: error on everything]
if(MSVC)
# It's not that we don't want these, it's just that I hate Windows.
# Also, MSVC doesn't support some of these arguments, so it's not possible.
set(COMPILE_OPTIONS)
else()
set(COMPILE_OPTIONS
  "-Wall"
  "-Wextra"
  "-Werror"
  "-Wno-unused-function"
  "-Wno-unneeded-internal-declaration")
endif()

if(MSVC)
set(RELEASE_COMPILE_OPTIONS "-O2" "${COMPILE_OPTIONS}")
else()
set(RELEASE_COMPILE_OPTIONS "-O3" "${COMPILE_OPTIONS}")
endif()

set(LINK_OPTIONS)

# Android's stdlib ("bionic") comes with threads.h and pthread.h
if(CMAKE_SYSTEM_NAME STREQUAL "Android")
  set(LINK_LIBRARIES)
else()
  find_package(Threads REQUIRED)
  set(LINK_LIBRARIES "Threads::Threads")
endif()

if(APPLE)
  list(APPEND LINK_LIBRARIES "-framework CoreFoundation" "-framework CoreServices")
endif()

# [options: parsing]
option(WTR_WATCHER_USE_RELEASE        "Build with all optimizations"                ON)
option(WTR_WATCHER_USE_SINGLE_INCLUDE "Build with a single header"                  OFF)
option(WTR_WATCHER_USE_TINY_MAIN      "Build the tiny main program"                 OFF)
option(WTR_WATCHER_USE_TEST           "Build the test programs"                     OFF)
option(WTR_WATCHER_USE_NOSAN          "This option does nothing"                    OFF)
option(WTR_WATCHER_USE_ASAN           "Build with the address sanitizer"            OFF)
option(WTR_WATCHER_USE_MSAN           "Build with the memory sanitizer"             OFF)
option(WTR_WATCHER_USE_TSAN           "Build with the thread sanitizer"             OFF)
option(WTR_WATCHER_USE_UBSAN          "Build with the undefined behavior sanitizer" OFF)
option(WTR_WATCHER_USE_STACKSAN       "Build with the stack safety sanitizer"       OFF)
option(WTR_WATCHER_USE_DATAFLOWSAN    "Build with the data flow sanitizer"          OFF)
option(WTR_WATCHER_USE_CFISAN         "Build with the cfi sanitizer"                OFF)
option(WTR_WATCHER_USE_KCFISAN        "Build with the kernel cfi sanitizer"         OFF)

# [options: meaning: include]
if(WTR_WATCHER_USE_SINGLE_INCLUDE)
  list(APPEND INCLUDE_PATH    "../../include")
else()
  list(APPEND INCLUDE_PATH    "../../devel/include")
endif()

# [options: meaning: source]
if(WTR_WATCHER_USE_TINY_MAIN)
  set(SOURCES                 "../../src/tiny_watcher/main.cpp")
endif()

# [options: meaning: sanitizer]
if(WTR_WATCHER_USE_NOSAN)
  set(COMPILE_OPTIONS         "${RELEASE_COMPILE_OPTIONS}")
  set(LINK_OPTIONS            "${RELEASE_LINK_OPTIONS}")
endif()
if(WTR_WATCHER_USE_ASAN)
  list(APPEND COMPILE_OPTIONS "-fno-omit-frame-pointer" "-fsanitize=address")
  list(APPEND LINK_OPTIONS    "-fno-omit-frame-pointer" "-fsanitize=address")
endif()
if(WTR_WATCHER_USE_MSAN)
  list(APPEND COMPILE_OPTIONS "-fno-omit-frame-pointer" "-fsanitize=memory")
  list(APPEND LINK_OPTIONS    "-fno-omit-frame-pointer" "-fsanitize=memory")
endif()
if(WTR_WATCHER_USE_TSAN)
  list(APPEND COMPILE_OPTIONS "-fno-omit-frame-pointer" "-fsanitize=thread")
  list(APPEND LINK_OPTIONS    "-fno-omit-frame-pointer" "-fsanitize=thread")
endif()
if(WTR_WATCHER_USE_UBSAN)
  list(APPEND COMPILE_OPTIONS "-fno-omit-frame-pointer" "-fsanitize=undefined")
  list(APPEND LINK_OPTIONS    "-fno-omit-frame-pointer" "-fsanitize=undefined")
endif()
if(WTR_WATCHER_USE_STACKSAN)
  list(APPEND COMPILE_OPTIONS "-fno-omit-frame-pointer" "-fsanitize=safe-stack")
  list(APPEND LINK_OPTIONS    "-fno-omit-frame-pointer" "-fsanitize=safe-stack")
endif()
if(WTR_WATCHER_USE_DATAFLOWSAN)
  list(APPEND COMPILE_OPTIONS "-fno-omit-frame-pointer" "-fsanitize=dataflow")
  list(APPEND LINK_OPTIONS    "-fno-omit-frame-pointer" "-fsanitize=dataflow")
endif()
if(WTR_WATCHER_USE_CFISAN)
  list(APPEND COMPILE_OPTIONS "-fno-omit-frame-pointer" "-fsanitize=cfi")
  list(APPEND LINK_OPTIONS    "-fno-omit-frame-pointer" "-fsanitize=cfi")
endif()
if(WTR_WATCHER_USE_KCFISAN)
  list(APPEND COMPILE_OPTIONS "-fno-omit-frame-pointer" "-fsanitize=kcfi")
  list(APPEND LINK_OPTIONS    "-fno-omit-frame-pointer" "-fsanitize=kcfi")
endif()

