# [options]
option(USE_SINGLE_INCLUDE
  "Build with a single header" OFF)
option(USE_TINY_MAIN
  "Build the tiny main program" OFF)
option(USE_NOSAN
  "This option does nothing" OFF)
option(USE_ASAN
  "Build with the address sanitizer" OFF)
option(USE_MSAN
  "Build with the memory sanitizer" OFF)
option(USE_TSAN
  "Build with the thread sanitizer" OFF)
option(USE_UBSAN
  "Build with the undefined behavior sanitizer" OFF)
option(USE_STACKSAN
  "Build with the stack safety sanitizer" OFF)
option(USE_DATAFLOWSAN
  "Build with the data flow sanitizer" OFF)
option(USE_CFISAN
  "Build with the data flow sanitizer" OFF)
option(USE_KCFISAN
  "Build with the data flow sanitizer" OFF)
if(USE_SINGLE_INCLUDE)
  set(INCLUDE_PATH
    "../../sinclude")
else()
  set(INCLUDE_PATH
    "../../include")
endif()
if(USE_TINY_MAIN)
  set(CLI_SOURCE
    "../../src/tiny-main.cpp")
else()
  set(CLI_SOURCE
    "../../src/main.cpp")
endif()
if(USE_ASAN)
  list(APPEND COMPILE_OPTIONS
    "-fno-omit-frame-pointer" "-fsanitize=address")
  list(APPEND LINK_OPTIONS
    "-fno-omit-frame-pointer" "-fsanitize=address")
else()
  set(COMPILE_OPTIONS)
  set(LINK_OPTIONS)
endif()
if(USE_MSAN)
  list(APPEND COMPILE_OPTIONS
    "-fno-omit-frame-pointer" "-fsanitize=memory")
  list(APPEND LINK_OPTIONS
    "-fno-omit-frame-pointer" "-fsanitize=memory")
else()
  set(COMPILE_OPTIONS)
  set(LINK_OPTIONS)
endif()
if(USE_TSAN)
  list(APPEND COMPILE_OPTIONS
    "-fno-omit-frame-pointer" "-fsanitize=thread")
  list(APPEND LINK_OPTIONS
    "-fno-omit-frame-pointer" "-fsanitize=thread")
else()
  set(COMPILE_OPTIONS)
  set(LINK_OPTIONS)
endif()
if(USE_UBSAN)
  list(APPEND COMPILE_OPTIONS
    "-fno-omit-frame-pointer" "-fsanitize=undefined")
  list(APPEND LINK_OPTIONS
    "-fno-omit-frame-pointer" "-fsanitize=undefined")
else()
  set(COMPILE_OPTIONS)
  set(LINK_OPTIONS)
endif()
if(USE_STACKSAN)
  list(APPEND COMPILE_OPTIONS
    "-fno-omit-frame-pointer" "-fsanitize=safe-stack")
  list(APPEND LINK_OPTIONS
    "-fno-omit-frame-pointer" "-fsanitize=safe-stack")
else()
  set(COMPILE_OPTIONS)
  set(LINK_OPTIONS)
endif()
if(USE_DATAFLOWSAN)
  list(APPEND COMPILE_OPTIONS
    "-fno-omit-frame-pointer" "-fsanitize=dataflow")
  list(APPEND LINK_OPTIONS
    "-fno-omit-frame-pointer" "-fsanitize=dataflow")
else()
  set(COMPILE_OPTIONS)
  set(LINK_OPTIONS)
endif()
if(USE_CFISAN)
  list(APPEND COMPILE_OPTIONS
    "-fno-omit-frame-pointer" "-fsanitize=cfi")
  list(APPEND LINK_OPTIONS
    "-fno-omit-frame-pointer" "-fsanitize=cfi")
else()
  set(COMPILE_OPTIONS)
  set(LINK_OPTIONS)
endif()
if(USE_KCFISAN)
  list(APPEND COMPILE_OPTIONS
    "-fno-omit-frame-pointer" "-fsanitize=kcfi")
  list(APPEND LINK_OPTIONS
    "-fno-omit-frame-pointer" "-fsanitize=kcfi")
else()
  set(COMPILE_OPTIONS)
  set(LINK_OPTIONS)
endif()