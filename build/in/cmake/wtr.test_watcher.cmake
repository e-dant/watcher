# [test dependencies]

include(CTest)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2
  GIT_TAG        5df88da16e276f853cc0c45f4b570419be77dd43 # 3.1.1
)
FetchContent_MakeAvailable(Catch2)
include("${catch2_SOURCE_DIR}/extras/Catch.cmake")

list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")

# [test targets]
include("${TEST_PROJECT_NAME}.test_watch_targets")
include("${TEST_PROJECT_NAME}.test_concurrent_watch_targets")
include("${TEST_PROJECT_NAME}.test_directory_events")
include("${TEST_PROJECT_NAME}.test_regular_file_events")
