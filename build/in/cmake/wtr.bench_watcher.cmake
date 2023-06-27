include(CTest)
include(FetchContent)
FetchContent_Declare(
  snitch
  GIT_REPOSITORY https://github.com/cschreib/snitch.git
  GIT_TAG        ea200a0830394f8e0ef732064f0935a77c003bd6 # v1.0.0
)
FetchContent_MakeAvailable(snitch)

function(WTR_ADD_BENCH_TARGET TITLE SRCS COMPILE_OPTIONS LINK_OPTIONS INCLUDE_PATH LINK_LIBRARIES)
  set(RUNTIME_TEST_FILES "${SRCS}")
  add_executable("wtr.bench_watcher.${TITLE}" "${SRCS}")
  set_property(TARGET "wtr.bench_watcher.${TITLE}" PROPERTY CXX_STANDARD 20)
  target_compile_options("wtr.bench_watcher.${TITLE}" PRIVATE "${COMPILE_OPTIONS}")
  target_link_options("wtr.bench_watcher.${TITLE}" PRIVATE "${LINK_OPTIONS}")
  target_include_directories("wtr.bench_watcher.${TITLE}" PRIVATE "${INCLUDE_PATH}")
  target_link_libraries("wtr.bench_watcher.${TITLE}" PRIVATE "${LINK_LIBRARIES}")
  if(APPLE)
    set_property(
      TARGET "wtr.bench_watcher.${TITLE}"
      PROPERTY XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "org.wtr.watcher.${TITLE}")
  endif()
  install(TARGETS                    "wtr.bench_watcher.${TITLE}"
          LIBRARY DESTINATION        "${CMAKE_INSTALL_LIBDIR}"
          BUNDLE DESTINATION         "${CMAKE_INSTALL_PREFIX}/bin"
          PUBLIC_HEADER DESTINATION  "${CMAKE_INSTALL_INCLUDEDIR}")
endfunction()

wtr_add_bench_target(
  "bench_concurrent_watch_targets"
  "${WTR_WATCHER_ROOT_SOURCE_DIR}/src/bench_watcher/bench_concurrent_watch_targets.cpp"
  "${COMPILE_OPTIONS}"
  "${LINK_OPTIONS}"
  "${INCLUDE_PATH}"
  "${LINK_LIBRARIES};snitch::snitch")

