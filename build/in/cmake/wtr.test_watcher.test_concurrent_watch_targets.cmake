# [concurrent watch test]

set(RUNTIME_TEST_FILES
  "${TEST_CONCURRENT_WATCH_TARGETS_SOURCES}")

add_executable("${TEST_PROJECT_NAME}.test_concurrent_watch_targets"
  "${TEST_CONCURRENT_WATCH_TARGETS_SOURCES}")

set_property(TARGET "${TEST_PROJECT_NAME}.test_concurrent_watch_targets" PROPERTY
  CXX_STANDARD 20)

target_compile_options("${TEST_PROJECT_NAME}.test_concurrent_watch_targets" PRIVATE
  "${TEST_COMPILE_OPTIONS}")
target_link_options("${TEST_PROJECT_NAME}.test_concurrent_watch_targets" PRIVATE
  "${TEST_LINK_OPTIONS}")

target_include_directories("${TEST_PROJECT_NAME}.test_concurrent_watch_targets" PUBLIC
  "${TEST_INCLUDE_PATH}")
target_link_libraries("${TEST_PROJECT_NAME}.test_concurrent_watch_targets" PRIVATE
  "${TEST_LINK_LIBRARIES}")

if(APPLE)
  set_property(TARGET "${TEST_PROJECT_NAME}.test_concurrent_watch_targets" PROPERTY
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER
    "org.wtr.watcher.test_concurrent_watch_targets")
endif()

install(TARGETS                    "${TEST_PROJECT_NAME}.test_concurrent_watch_targets"
        LIBRARY DESTINATION        "${CMAKE_INSTALL_LIBDIR}"
        BUNDLE DESTINATION         "${CMAKE_INSTALL_PREFIX}/bin"
        PUBLIC_HEADER DESTINATION  "${CMAKE_INSTALL_INCLUDEDIR}")
