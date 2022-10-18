cmake_policy(SET CMP0048 NEW) # VERSION

cmake_minimum_required(VERSION 3.2)

# [project]
project(
  water.watcher
  VERSION 0.2.8 # hook: tool/release
  DESCRIPTION "watcher: a filesystem watcher"
  HOMEPAGE_URL "https://edant.io"
  LANGUAGES CXX)

# [consumables]
add_executable(${PROJECT_NAME}
  ../../src/main.cpp)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 23)

# [include paths]
target_include_directories(${PROJECT_NAME}
  PUBLIC
    ../../sinclude)

if(APPLE)
target_link_libraries(
  ${PROJECT_NAME}
    PRIVATE
      "-framework CoreFoundation"
      "-framework CoreServices")
endif()

include(GNUInstallDirs)
install(
  TARGETS ${PROJECT_NAME}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
