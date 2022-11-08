# [directory test]

add_executable("${TEST_PROJECT_NAME}.test_event_targets"
  "${TEST_DIRECTORY_EVENTS_SOURCES}")
set_property(TARGET "${TEST_PROJECT_NAME}.test_event_targets" PROPERTY
  CXX_STANDARD 23)
target_compile_options("${TEST_PROJECT_NAME}.test_event_targets" PRIVATE
  "${TEST_COMPILE_OPTIONS}")
target_link_options("${TEST_PROJECT_NAME}.test_event_targets" PRIVATE
  "${TEST_LINK_OPTIONS}")
target_include_directories("${TEST_PROJECT_NAME}.test_event_targets" PUBLIC
  "${TEST_INCLUDE_PATH}")

target_include_directories("${TEST_PROJECT_NAME}.test_event_targets" PUBLIC
  "${Catch2_SOURCE_DIR}/src")
target_link_libraries("${TEST_PROJECT_NAME}.test_event_targets" PRIVATE
  "${TEST_LINK_LIBRARIES}")

if(APPLE)
  set_property(TARGET "${TEST_PROJECT_NAME}.test_event_targets" PROPERTY
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER
    "org.watcher.wtr.test_event_targets")
endif()

install(TARGETS                    "${TEST_PROJECT_NAME}.test_event_targets"
        LIBRARY DESTINATION        "${CMAKE_INSTALL_LIBDIR}"
        BUNDLE DESTINATION         "${CMAKE_INSTALL_PREFIX}/bin"
        PUBLIC_HEADER DESTINATION  "${CMAKE_INSTALL_INCLUDEDIR}")