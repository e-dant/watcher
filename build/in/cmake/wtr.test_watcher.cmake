# [test dependencies]
include(CTest)
include(FetchContent)
FetchContent_Declare(
  snatch
  GIT_REPOSITORY https://github.com/cschreib/snatch.git
  GIT_TAG        6f3a95e57f9bb4130d308890c19ec3588b0d1c94 # main
  # f313bccafe98aaef617af3bf457d091d8d50cdcd # 0.1.3
)
FetchContent_MakeAvailable(snatch)

# [test target definitions]
set(TEST_PROJECT_NAME                     "wtr.test_watcher")
set(TEST_REGULAR_FILE_EVENTS_SOURCES      "../../src/test_watcher/test_regular_file_events/main.cpp")
set(TEST_DIRECTORY_EVENTS_SOURCES         "../../src/test_watcher/test_directory_events/main.cpp")
set(TEST_CONCURRENT_WATCH_TARGETS_SOURCES "../../src/test_watcher/test_concurrent_watch_targets/main.cpp")
set(TEST_WATCH_TARGETS_SOURCES            "../../src/test_watcher/test_watch_targets/main.cpp")
set(TEST_NEW_DIRECTORIES_SOURCES          "../../src/test_watcher/test_new_directories/main.cpp")
set(TEST_SIMPLE_SOURCES                   "../../src/test_watcher/test_simple/main.cpp")
set(TEST_LINK_LIBRARIES                   "${LINK_LIBRARIES}" "snatch::snatch")
set(TEST_COMPILE_OPTIONS                  "${COMPILE_OPTIONS}")
set(TEST_LINK_OPTIONS                     "${LINK_OPTIONS}")
set(TEST_INCLUDE_PATH                     "${INCLUDE_PATH}")

# [test targets]
include("${TEST_PROJECT_NAME}.test_watch_targets")
include("${TEST_PROJECT_NAME}.test_concurrent_watch_targets")
include("${TEST_PROJECT_NAME}.test_new_directories")
include("${TEST_PROJECT_NAME}.test_simple")
include("${TEST_PROJECT_NAME}.test_directory_events")
include("${TEST_PROJECT_NAME}.test_regular_file_events")
