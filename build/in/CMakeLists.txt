# [useful links]
#   - https://github.com/aminya/project_options
#   - https://github.com/cpp-best-practices/project_options

cmake_minimum_required(
  VERSION 3.16)

# [DOWNLOAD_EXTRACT_TIMESTAMP]
#   - notes:
#     for local settings maybe, use
#     cmake_policy(SET CMP0135 NEW)
#     for global settings maybe, use
#     set(DOWNLOAD_EXTRACT_TIMESTAMP true)
cmake_policy(SET CMP0135 NEW)

# [fetch]
#     - for:
#       FetchContent_Declare
#       FetchContent_MakeAvailable
#       FetchContent_GetProperties
#       FetchContent_Populate
include(FetchContent)

#   [project_options v0.25.2]
#     - for:
#       dynamic_project_options
#       project_options
#       conan
#       cpack
FetchContent_Declare(
  water_watcher_project_options
  URL "https://github.com/aminya/project_options/archive/refs/tags/v0.25.2.tar.gz"
  DOWNLOAD_EXTRACT_TIMESTAMP
)
FetchContent_MakeAvailable(water_watcher_project_options)

include("${water_watcher_project_options_SOURCE_DIR}/Index.cmake")
include("${water_watcher_project_options_SOURCE_DIR}/src/DynamicProjectOptions.cmake")

# [vcpkg]
#   - notes:
#     - run_vcpkg() should be called before
#       defining project()
#     - to enable, uncomment the following line.
# run_vcpkg()

# [project]
project(
  water.watcher
  VERSION 0.0.1
  DESCRIPTION "watcher: a filesystem watcher"
  HOMEPAGE_URL "https://edant.io"
  LANGUAGES CXX
)

# [sha]
set(GIT_SHA "Unknown" CACHE STRING "SHA this build was generated from")
string(SUBSTRING "${GIT_SHA}" 0 8 GIT_SHORT_SHA)

# [compdb]
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# [project options]
#   [ENABLE_PCH]
#     PCH is disabled by default in developer
#     mode because these headers become globally
#     included and they can mask other errors
#   [PCH_HEADERS]
#     This is a list of headers to pre-compile.
#     These are the common ones.
#   [some other options]
#     ${ENABLE_CLANG_TIDY}
#     ${ENABLE_CPPCHECK}
#     ${ENABLE_DOXYGEN}
#     ${ENABLE_COVERAGE}
#     ENABLE_USER_LINKER
#     ENABLE_BUILD_WITH_TIME_TRACE
#     ENABLE_UNITY
#     ENABLE_VS_ANALYSIS
#     WARNINGS_AS_ERRORS
#     passes a profile to conan
#     see https://docs.conan.io/en/latest/reference/profiles.html
#     CONAN_PROFILE ${profile_path}
#     extra options to pass to conan
#     CONAN_OPTIONS
#     override the defaults for the MSVC warnings
#     MSVC_WARNINGS
#     override the defaults for the CLANG warnings
#     CLANG_WARNINGS
#     override the defaults for the GCC warnings
#     GCC_WARNINGS
dynamic_project_options(
  ENABLE_CACHE
  ENABLE_INTERPROCEDURAL_OPTIMIZATION
  ENABLE_NATIVE_OPTIMIZATION
  #ENABLE_SANITIZER_ADDRESS
  #ENABLE_SANITIZER_UNDEFINED_BEHAVIOR
  #ENABLE_SANITIZER_THREAD
  #ENABLE_SANITIZER_MEMORY
  #ENABLE_INCLUDE_WHAT_YOU_USE
  ENABLE_CONAN
  ENABLE_PCH
  PCH_HEADERS
    <filesystem>
    <functional>
    <iostream>
    <string>
    <thread>
    <unordered_map>
    <chrono>
)

# [consumables]
add_executable(
  "${PROJECT_NAME}"
    ../../src/main.cpp
)

# [include paths]
target_include_directories(
  "${PROJECT_NAME}"
    PRIVATE
      ../../include
)

target_link_libraries(
  "${PROJECT_NAME}"
    PRIVATE
      project_options
      project_warnings
)

if(APPLE)
target_link_libraries(
  "${PROJECT_NAME}"
    PRIVATE
      "-framework CoreFoundation"
      "-framework CoreServices"
)
endif()

# [package]
#   TODO
#package_project(
#  TARGETS "${PROJECT_NAME}"
#  PUBLIC_INCLUDES ../../include # ${INCLUDE_DIR} ?
#  # FIXME: this does not work! ED
#  # CONFIG_EXPORT_DESTINATION "${CMAKE_INSTALL_DATADIR}/${NAME}_cmake_package"
#  # CONFIG_INSTALL_DESTINATION "${CMAKE_INSTALL_DATADIR}/${NAME}_cmake_package"
#  # FIXME: this does not work! CK
#  # PRIVATE_DEPENDENCIES_CONFIGURED
#  # project_options project_warnings
#)

set(CPACK_PACKAGE_FILE_NAME_DIST
    "${CMAKE_PROJECT_NAME}-${CMAKE_PROJECT_VERSION}-${GIT_SHORT_SHA}-${CMAKE_SYSTEM_NAME}"
)
set(CPACK_PACKAGE_FILE_NAME_BUILD
    "${CMAKE_BUILD_TYPE}-${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION}"
)
set(CPACK_PACKAGE_FILE_NAME
    "${CPACK_PACKAGE_FILE_NAME_DIST}-${CPACK_PACKAGE_FILE_NAME_BUILD}"
)

include(CPack)

include(GNUInstallDirs)

install(
  TARGETS ${PROJECT_NAME}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
