# [test dependencies]
include(CTest)
include(FetchContent)
FetchContent_Declare(
  snatch
  GIT_REPOSITORY https://github.com/cschreib/snatch.git
  GIT_TAG        c0b6ac4efe4019e4846e8967fe21de864b0cc1ed # Friday, December 2nd, 2022 @ main
  # f313bccafe98aaef617af3bf457d091d8d50cdcd # 0.1.3
)
FetchContent_MakeAvailable(snatch)

# [test target definitions]
set(TEST_PROJECT_NAME                     "wtr.test_watcher")
set(TEST_REGULAR_FILE_EVENTS_SOURCES      "../../src/test_watcher/test_regular_file_events/test_regular_file_events.cpp")
set(TEST_DIRECTORY_EVENTS_SOURCES         "../../src/test_watcher/test_directory_events/test_directory_events.cpp")
set(TEST_CONCURRENT_WATCH_TARGETS_SOURCES "../../src/test_watcher/test_concurrent_watch_targets/test_concurrent_watch_targets.cpp")
set(TEST_WATCH_TARGETS_SOURCES            "../../src/test_watcher/test_watch_targets/test_watch_targets.cpp")
set(TEST_NEW_DIRECTORIES_SOURCES          "../../src/test_watcher/test_new_directories/test_new_directories.cpp")
set(TEST_SIMPLE_SOURCES                   "../../src/test_watcher/test_simple/test_simple.cpp")
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
