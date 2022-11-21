# [regular file test]

set(RUNTIME_TEST_FILES
  "${TEST_REGULAR_FILE_EVENTS_SOURCES}")

add_executable("${TEST_PROJECT_NAME}.test_regular_file_events"
  "${TEST_REGULAR_FILE_EVENTS_SOURCES}")

set_property(TARGET "${TEST_PROJECT_NAME}.test_regular_file_events" PROPERTY
  CXX_STANDARD 23)

target_compile_options("${TEST_PROJECT_NAME}.test_regular_file_events" PRIVATE
  "${TEST_COMPILE_OPTIONS}")
target_link_options("${TEST_PROJECT_NAME}.test_regular_file_events" PRIVATE
  "${TEST_LINK_OPTIONS}")

target_include_directories("${TEST_PROJECT_NAME}.test_regular_file_events" PUBLIC
  "${TEST_INCLUDE_PATH}")
target_link_libraries("${TEST_PROJECT_NAME}.test_regular_file_events" PRIVATE
  "${TEST_LINK_LIBRARIES}")

if(APPLE)
  set_property(TARGET "${TEST_PROJECT_NAME}.test_regular_file_events" PROPERTY
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER
    "org.wtr.watcher.test_regular_file_events")
endif()

install(TARGETS                    "${TEST_PROJECT_NAME}.test_regular_file_events"
        LIBRARY DESTINATION        "${CMAKE_INSTALL_LIBDIR}"
        BUNDLE DESTINATION         "${CMAKE_INSTALL_PREFIX}/bin"
        PUBLIC_HEADER DESTINATION  "${CMAKE_INSTALL_INCLUDEDIR}")
