# Install script for directory: /Users/edant/dev/watcher/_deps/snitch-src

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/var/empty/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/local/bin/llvm-objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/var/empty/local/include/snitch/snitch.hpp;/var/empty/local/include/snitch/snitch_append.hpp;/var/empty/local/include/snitch/snitch_capture.hpp;/var/empty/local/include/snitch/snitch_cli.hpp;/var/empty/local/include/snitch/snitch_concepts.hpp;/var/empty/local/include/snitch/snitch_console.hpp;/var/empty/local/include/snitch/snitch_error_handling.hpp;/var/empty/local/include/snitch/snitch_expression.hpp;/var/empty/local/include/snitch/snitch_fixed_point.hpp;/var/empty/local/include/snitch/snitch_function.hpp;/var/empty/local/include/snitch/snitch_macros_check.hpp;/var/empty/local/include/snitch/snitch_macros_check_base.hpp;/var/empty/local/include/snitch/snitch_macros_consteval.hpp;/var/empty/local/include/snitch/snitch_macros_constexpr.hpp;/var/empty/local/include/snitch/snitch_macros_exceptions.hpp;/var/empty/local/include/snitch/snitch_macros_misc.hpp;/var/empty/local/include/snitch/snitch_macros_test_case.hpp;/var/empty/local/include/snitch/snitch_macros_utility.hpp;/var/empty/local/include/snitch/snitch_macros_warnings.hpp;/var/empty/local/include/snitch/snitch_matcher.hpp;/var/empty/local/include/snitch/snitch_registry.hpp;/var/empty/local/include/snitch/snitch_section.hpp;/var/empty/local/include/snitch/snitch_string.hpp;/var/empty/local/include/snitch/snitch_string_utility.hpp;/var/empty/local/include/snitch/snitch_teamcity.hpp;/var/empty/local/include/snitch/snitch_test_data.hpp;/var/empty/local/include/snitch/snitch_type_name.hpp;/var/empty/local/include/snitch/snitch_vector.hpp;/var/empty/local/include/snitch/snitch_config.hpp")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/var/empty/local/include/snitch" TYPE FILE FILES
    "/Users/edant/dev/watcher/_deps/snitch-src/include/snitch/snitch.hpp"
    "/Users/edant/dev/watcher/_deps/snitch-src/include/snitch/snitch_append.hpp"
    "/Users/edant/dev/watcher/_deps/snitch-src/include/snitch/snitch_capture.hpp"
    "/Users/edant/dev/watcher/_deps/snitch-src/include/snitch/snitch_cli.hpp"
    "/Users/edant/dev/watcher/_deps/snitch-src/include/snitch/snitch_concepts.hpp"
    "/Users/edant/dev/watcher/_deps/snitch-src/include/snitch/snitch_console.hpp"
    "/Users/edant/dev/watcher/_deps/snitch-src/include/snitch/snitch_error_handling.hpp"
    "/Users/edant/dev/watcher/_deps/snitch-src/include/snitch/snitch_expression.hpp"
    "/Users/edant/dev/watcher/_deps/snitch-src/include/snitch/snitch_fixed_point.hpp"
    "/Users/edant/dev/watcher/_deps/snitch-src/include/snitch/snitch_function.hpp"
    "/Users/edant/dev/watcher/_deps/snitch-src/include/snitch/snitch_macros_check.hpp"
    "/Users/edant/dev/watcher/_deps/snitch-src/include/snitch/snitch_macros_check_base.hpp"
    "/Users/edant/dev/watcher/_deps/snitch-src/include/snitch/snitch_macros_consteval.hpp"
    "/Users/edant/dev/watcher/_deps/snitch-src/include/snitch/snitch_macros_constexpr.hpp"
    "/Users/edant/dev/watcher/_deps/snitch-src/include/snitch/snitch_macros_exceptions.hpp"
    "/Users/edant/dev/watcher/_deps/snitch-src/include/snitch/snitch_macros_misc.hpp"
    "/Users/edant/dev/watcher/_deps/snitch-src/include/snitch/snitch_macros_test_case.hpp"
    "/Users/edant/dev/watcher/_deps/snitch-src/include/snitch/snitch_macros_utility.hpp"
    "/Users/edant/dev/watcher/_deps/snitch-src/include/snitch/snitch_macros_warnings.hpp"
    "/Users/edant/dev/watcher/_deps/snitch-src/include/snitch/snitch_matcher.hpp"
    "/Users/edant/dev/watcher/_deps/snitch-src/include/snitch/snitch_registry.hpp"
    "/Users/edant/dev/watcher/_deps/snitch-src/include/snitch/snitch_section.hpp"
    "/Users/edant/dev/watcher/_deps/snitch-src/include/snitch/snitch_string.hpp"
    "/Users/edant/dev/watcher/_deps/snitch-src/include/snitch/snitch_string_utility.hpp"
    "/Users/edant/dev/watcher/_deps/snitch-src/include/snitch/snitch_teamcity.hpp"
    "/Users/edant/dev/watcher/_deps/snitch-src/include/snitch/snitch_test_data.hpp"
    "/Users/edant/dev/watcher/_deps/snitch-src/include/snitch/snitch_type_name.hpp"
    "/Users/edant/dev/watcher/_deps/snitch-src/include/snitch/snitch_vector.hpp"
    "/Users/edant/dev/watcher/_deps/snitch-build/snitch/snitch_config.hpp"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/var/empty/local/include/snitch/snitch_all.hpp")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/var/empty/local/include/snitch" TYPE FILE FILES "/Users/edant/dev/watcher/_deps/snitch-build/snitch/snitch_all.hpp")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/Users/edant/dev/watcher/_deps/snitch-build/libsnitch.a")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libsnitch.a" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libsnitch.a")
    execute_process(COMMAND "/usr/local/bin/llvm-ranlib" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libsnitch.a")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Development" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}/var/empty/local/lib/cmake/snitch/snitch-targets.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}/var/empty/local/lib/cmake/snitch/snitch-targets.cmake"
         "/Users/edant/dev/watcher/_deps/snitch-build/CMakeFiles/Export/0f7976cc982348fd183688a063210d68/snitch-targets.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}/var/empty/local/lib/cmake/snitch/snitch-targets-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}/var/empty/local/lib/cmake/snitch/snitch-targets.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/var/empty/local/lib/cmake/snitch/snitch-targets.cmake")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/var/empty/local/lib/cmake/snitch" TYPE FILE FILES "/Users/edant/dev/watcher/_deps/snitch-build/CMakeFiles/Export/0f7976cc982348fd183688a063210d68/snitch-targets.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
     "/var/empty/local/lib/cmake/snitch/snitch-targets-release.cmake")
    if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
      message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
    endif()
    if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
      message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
    endif()
    file(INSTALL DESTINATION "/var/empty/local/lib/cmake/snitch" TYPE FILE FILES "/Users/edant/dev/watcher/_deps/snitch-build/CMakeFiles/Export/0f7976cc982348fd183688a063210d68/snitch-targets-release.cmake")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Development" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/var/empty/local/lib/cmake/snitch/snitch-config.cmake")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/var/empty/local/lib/cmake/snitch" TYPE FILE FILES "/Users/edant/dev/watcher/_deps/snitch-build/snitch-config.cmake")
endif()

