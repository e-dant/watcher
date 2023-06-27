# [test dependencies]
include(CTest)
include(FetchContent)
FetchContent_Declare(
  snitch
  GIT_REPOSITORY https://github.com/cschreib/snitch.git
  GIT_TAG        ea200a0830394f8e0ef732064f0935a77c003bd6 # Friday, January 20th, 2023 @ v1.0.0
  # GIT_TAG        8165d6c85353f9c302ce05f1c1c47dcfdc6aeb2c # Saturday, January 7th, 2023 @ main
  # GIT_TAG        f313bccafe98aaef617af3bf457d091d8d50cdcd # Tuesday, December 18th, 2022 @ v0.1.3
  # GIT_TAG        c0b6ac4efe4019e4846e8967fe21de864b0cc1ed # Friday, December 2nd, 2022 @ main
)
FetchContent_MakeAvailable(snitch)

function(WTR_ADD_TEST_TARGET TEST_PROJECT_NAME SRCS COMPILE_OPTIONS LINK_OPTIONS INCLUDE_PATH LINK_LIBRARIES)
  set(RUNTIME_TEST_FILES "${SRCS}")
  add_executable("${TEST_PROJECT_NAME}" "${SRCS}")
  set_property(TARGET "${TEST_PROJECT_NAME}" PROPERTY CXX_STANDARD 20)
  target_compile_options("${TEST_PROJECT_NAME}" PRIVATE "${COMPILE_OPTIONS}")
  target_link_options("${TEST_PROJECT_NAME}" PRIVATE "${LINK_OPTIONS}")
  target_include_directories("${TEST_PROJECT_NAME}" PRIVATE "${INCLUDE_PATH}")
  target_link_libraries("${TEST_PROJECT_NAME}" PRIVATE "${LINK_LIBRARIES}")
  if(APPLE)
    set_property(
      TARGET "${TEST_PROJECT_NAME}"
      PROPERTY XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "org.${TEST_PROJECT_NAME}")
  endif()
  # install(TARGETS                    "${TEST_PROJECT_NAME}"
  #         LIBRARY DESTINATION        "${CMAKE_INSTALL_LIBDIR}"
  #         BUNDLE DESTINATION         "${CMAKE_INSTALL_PREFIX}/bin"
  #         PUBLIC_HEADER DESTINATION  "${CMAKE_INSTALL_INCLUDEDIR}")
endfunction()

set(TEST_PROJECT_NAME "test_watcher")

set(TEST_SET
  "test_concurrent_watch_targets"
  "test_watch_targets"
  "test_new_directories"
  "test_simple")
list(TRANSFORM TEST_SET PREPEND
  "${WTR_WATCHER_ROOT_SOURCE_DIR}/src/${TEST_PROJECT_NAME}/")
list(TRANSFORM TEST_SET APPEND ".cpp")

wtr_add_test_target(
  "wtr.${TEST_PROJECT_NAME}"
  "${TEST_SET}"
  "${COMPILE_OPTIONS}"
  "${LINK_OPTIONS}"
  "${INCLUDE_PATH}"
  "${LINK_LIBRARIES};snitch::snitch")

