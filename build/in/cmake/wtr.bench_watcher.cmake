# [dependencies]
include(CTest)
include(FetchContent)
FetchContent_Declare(
  snitch
  GIT_REPOSITORY https://github.com/cschreib/snitch.git
  GIT_TAG        ea200a0830394f8e0ef732064f0935a77c003bd6 # v1.0.0
)
FetchContent_MakeAvailable(snitch)

# [target definitions]
set(BENCH_PROJECT_NAME                     "wtr.bench_watcher")
set(BENCH_CONCURRENT_WATCH_TARGETS_SOURCES "../../src/bench_watcher/bench_concurrent_watch_targets/bench_concurrent_watch_targets.cpp")
set(BENCH_LINK_LIBRARIES                   "${LINK_LIBRARIES}" "snitch::snitch")
set(BENCH_COMPILE_OPTIONS                  "${COMPILE_OPTIONS}")
set(BENCH_LINK_OPTIONS                     "${LINK_OPTIONS}")
set(BENCH_INCLUDE_PATH                     "${INCLUDE_PATH}")

# [targets]
include("${BENCH_PROJECT_NAME}.bench_concurrent_watch_targets")
