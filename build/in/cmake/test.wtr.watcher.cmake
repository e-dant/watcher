include(CTest)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2
  GIT_TAG        5df88da16e276f853cc0c45f4b570419be77dd43 # 3.1.1
)
FetchContent_MakeAvailable(Catch2)
include("${catch2_SOURCE_DIR}/extras/Catch.cmake")

list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")

# [regular file test]

add_executable("test_regular_file_events.${PROJECT_NAME}"
  "${TEST_REGULAR_FILE_EVENTS_SOURCES}")
set_property(TARGET "test_regular_file_events.${PROJECT_NAME}" PROPERTY
  CXX_STANDARD 23)
target_compile_options("test_regular_file_events.${PROJECT_NAME}" PRIVATE
  "${TEST_COMPILE_OPTIONS}")
target_link_options("test_regular_file_events.${PROJECT_NAME}" PRIVATE
  "${TEST_LINK_OPTIONS}")
target_include_directories("test_regular_file_events.${PROJECT_NAME}" PUBLIC
  "${TEST_INCLUDE_PATH}")

target_include_directories("test_regular_file_events.${PROJECT_NAME}" PUBLIC
  "${Catch2_SOURCE_DIR}/src")
target_link_libraries("test_regular_file_events.${PROJECT_NAME}" PRIVATE
  "${TEST_LINK_LIBRARIES}")

if(APPLE)
  set_property(TARGET "test_regular_file_events.${PROJECT_NAME}" PROPERTY
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER
    "org.watcher.wtr.test_regular_file_events")
endif()

install(TARGETS                    "test_regular_file_events.${PROJECT_NAME}"
        LIBRARY DESTINATION        "${CMAKE_INSTALL_LIBDIR}"
        BUNDLE DESTINATION         "${CMAKE_INSTALL_PREFIX}/bin"
        PUBLIC_HEADER DESTINATION  "${CMAKE_INSTALL_INCLUDEDIR}")

# [directory test]

add_executable("test_directory_events.${PROJECT_NAME}"
  "${TEST_DIRECTORY_EVENTS_SOURCES}")
set_property(TARGET "test_directory_events.${PROJECT_NAME}" PROPERTY
  CXX_STANDARD 23)
target_compile_options("test_directory_events.${PROJECT_NAME}" PRIVATE
  "${TEST_COMPILE_OPTIONS}")
target_link_options("test_directory_events.${PROJECT_NAME}" PRIVATE
  "${TEST_LINK_OPTIONS}")
target_include_directories("test_directory_events.${PROJECT_NAME}" PUBLIC
  "${TEST_INCLUDE_PATH}")

target_include_directories("test_directory_events.${PROJECT_NAME}" PUBLIC
  "${Catch2_SOURCE_DIR}/src")
target_link_libraries("test_directory_events.${PROJECT_NAME}" PRIVATE
  "${TEST_LINK_LIBRARIES}")

if(APPLE)
  set_property(TARGET "test_directory_events.${PROJECT_NAME}" PROPERTY
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER
    "org.watcher.wtr.test_directory_events")
endif()

install(TARGETS                    "test_directory_events.${PROJECT_NAME}"
        LIBRARY DESTINATION        "${CMAKE_INSTALL_LIBDIR}"
        BUNDLE DESTINATION         "${CMAKE_INSTALL_PREFIX}/bin"
        PUBLIC_HEADER DESTINATION  "${CMAKE_INSTALL_INCLUDEDIR}")