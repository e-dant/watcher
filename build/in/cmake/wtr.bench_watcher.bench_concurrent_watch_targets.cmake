# [concurrent watch test]

set(RUNTIME_TEST_FILES
  "${BENCH_CONCURRENT_WATCH_TARGETS_SOURCES}")

add_executable("${BENCH_PROJECT_NAME}.bench_concurrent_watch_targets"
  "${BENCH_CONCURRENT_WATCH_TARGETS_SOURCES}")

set_property(TARGET "${BENCH_PROJECT_NAME}.bench_concurrent_watch_targets" PROPERTY
  CXX_STANDARD 20)

target_compile_options("${BENCH_PROJECT_NAME}.bench_concurrent_watch_targets" PRIVATE
  "${BENCH_COMPILE_OPTIONS}")
target_link_options("${BENCH_PROJECT_NAME}.bench_concurrent_watch_targets" PRIVATE
  "${BENCH_LINK_OPTIONS}")

target_include_directories("${BENCH_PROJECT_NAME}.bench_concurrent_watch_targets" PUBLIC
  "${BENCH_INCLUDE_PATH}")
target_link_libraries("${BENCH_PROJECT_NAME}.bench_concurrent_watch_targets" PRIVATE
  "${BENCH_LINK_LIBRARIES}")

if(APPLE)
  set_property(TARGET "${BENCH_PROJECT_NAME}.bench_concurrent_watch_targets" PROPERTY
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER
    "org.wtr.watcher.bench_concurrent_watch_targets")
endif()

install(TARGETS                    "${BENCH_PROJECT_NAME}.bench_concurrent_watch_targets"
        LIBRARY DESTINATION        "${CMAKE_INSTALL_LIBDIR}"
        BUNDLE DESTINATION         "${CMAKE_INSTALL_PREFIX}/bin"
        PUBLIC_HEADER DESTINATION  "${CMAKE_INSTALL_INCLUDEDIR}")
