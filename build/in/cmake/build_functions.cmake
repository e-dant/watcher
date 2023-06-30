function(WTR_ADD_TARGET
      NAME
      IS_TEST
      IS_INSTALLABLE
      SRC_SET
      COPT_SET
      LOPT_SET
      INCLUDE_PATH
      LLIB_SET)
  add_executable("${NAME}" "${SRC_SET}")
  set_property(TARGET "${NAME}" PROPERTY CXX_STANDARD "${WTR_WATCHER_CXX_STD}")
  target_compile_options("${NAME}" PRIVATE "${COPT_SET}")
  target_link_options("${NAME}" PRIVATE "${LOPT_SET}")
  target_include_directories("${NAME}" PUBLIC "${INCLUDE_PATH}")
  target_link_libraries("${NAME}" PRIVATE "${LLIB_SET}")

  if(APPLE)
    set_property(
      TARGET "${NAME}"
      PROPERTY XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "org.${NAME}")
  endif()

  if(IS_TEST)
    include(CTest)
    include(FetchContent)
    FetchContent_Declare(
      snitch
      GIT_REPOSITORY https://github.com/cschreib/snitch.git
      # Tuesday, June 29th, 2023 @ v1.1.1
      GIT_TAG        5ad2fffebf31f3e6d56c2c0ab27bc45d01da2f05
      # Friday, January 20th, 2023 @ v1.0.0
      # GIT_TAG        ea200a0830394f8e0ef732064f0935a77c003bd6
      # Saturday, January 7th, 2023 @ main
      # GIT_TAG        8165d6c85353f9c302ce05f1c1c47dcfdc6aeb2c
      # Tuesday, December 18th, 2022 @ v0.1.3
      # GIT_TAG        f313bccafe98aaef617af3bf457d091d8d50cdcd
      # Friday, December 2nd, 2022 @ main
      # GIT_TAG        c0b6ac4efe4019e4846e8967fe21de864b0cc1ed
    )
    FetchContent_MakeAvailable(snitch)
    # set(RUNTIME_TEST_FILES "${SRC_SET}") # For Snitch
  endif()

  if(IS_INSTALLABLE)
    include(GNUInstallDirs)
    install(
      TARGETS                   "${NAME}"
      LIBRARY DESTINATION       "${CMAKE_INSTALL_LIBDIR}"
      BUNDLE DESTINATION        "${CMAKE_INSTALL_PREFIX}/bin"
      PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
  endif()
endfunction()

function(WTR_ADD_TEST_TARGET NAME COPT_SET LOPT_SET)
  wtr_add_target(
    "${NAME}"
    "ON"
    "OFF"
    "${WTR_TEST_WATCHER_SOURCE_SET}"
    "${COPT_SET}"
    "${LOPT_SET}"
    "${INCLUDE_PATH_DEVEL}"
    "${TEST_LINK_LIBRARIES}")
endfunction()

function(WTR_ADD_BENCH_TARGET NAME COPT_SET LOPT_SET)
  wtr_add_target(
    "${NAME}"
    "ON"
    "OFF"
    "${WTR_BENCH_WATCHER_SOURCE_SET}"
    "${COPT_SET}"
    "${LOPT_SET}"
    "${INCLUDE_PATH_DEVEL}"
    "${TEST_LINK_LIBRARIES}")
endfunction()