# [new directories test]

set(RUNTIME_TEST_FILES
  "${TEST_NEW_DIRECTORIES_SOURCES}")

add_executable("${TEST_PROJECT_NAME}.test_new_directories"
  "${TEST_NEW_DIRECTORIES_SOURCES}")

set_property(TARGET "${TEST_PROJECT_NAME}.test_new_directories" PROPERTY
  CXX_STANDARD 20)

target_compile_options("${TEST_PROJECT_NAME}.test_new_directories" PRIVATE
  "${TEST_COMPILE_OPTIONS}")
target_link_options("${TEST_PROJECT_NAME}.test_new_directories" PRIVATE
  "${TEST_LINK_OPTIONS}")

target_include_directories("${TEST_PROJECT_NAME}.test_new_directories" PUBLIC
  "${TEST_INCLUDE_PATH}")
target_link_libraries("${TEST_PROJECT_NAME}.test_new_directories" PRIVATE
  "${TEST_LINK_LIBRARIES}")

if(APPLE)
  set_property(TARGET "${TEST_PROJECT_NAME}.test_new_directories" PROPERTY
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER
    "org.wtr.watcher.test_new_directories")
endif()

install(TARGETS                    "${TEST_PROJECT_NAME}.test_new_directories"
        LIBRARY DESTINATION        "${CMAKE_INSTALL_LIBDIR}"
        BUNDLE DESTINATION         "${CMAKE_INSTALL_PREFIX}/bin"
        PUBLIC_HEADER DESTINATION  "${CMAKE_INSTALL_INCLUDEDIR}")
