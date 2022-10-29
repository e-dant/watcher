
#
# Boilerplate here
#

cmake_policy(SET CMP0048 NEW) # for VERSION in project()

cmake_minimum_required(VERSION 3.2)

# [project]
project(
  water.watcher
  VERSION 0.3.2 # hook: tool/release
  DESCRIPTION "watcher: a filesystem watcher"
  HOMEPAGE_URL "github.com/e-dant/watcher"
  LANGUAGES CXX)

# [options]
option(WATER_WATCHER_USE_SINGLE_INCLUDE
  "Build with a single header" OFF)
option(WATER_WATCHER_USE_TINY_MAIN
  "Build the tiny main program" OFF)
if(WATER_WATCHER_USE_SINGLE_INCLUDE)
  set(WATER_WATCHER_INCLUDE_PATH "../../sinclude")
else()
  set(WATER_WATCHER_INCLUDE_PATH "../../include")
endif()
if(WATER_WATCHER_USE_TINY_MAIN)
  set(WATER_WATCHER_CLI_SOURCE "../../src/tiny-main.cpp")
else()
  set(WATER_WATCHER_CLI_SOURCE "../../src/main.cpp")
endif()

# [system prerequisites]
find_package(Threads REQUIRED)

# [system configuration]
set(WATER_WATCHER_LINK_LIBRARIES
  "Threads::Threads")
if(APPLE)
  list(APPEND WATER_WATCHER_LINK_LIBRARIES
    "-framework CoreFoundation"
    "-framework CoreServices")
endif()

#
# Important parts here
#

# [cli program]
add_executable(${PROJECT_NAME}
  ${WATER_WATCHER_CLI_SOURCE})

# [c++ standard]
set_property(TARGET ${PROJECT_NAME} PROPERTY
  CXX_STANDARD 23)

# [include path]
target_include_directories(${PROJECT_NAME} PUBLIC
  ${WATER_WATCHER_INCLUDE_PATH})

# [system libraries]
target_link_libraries(${PROJECT_NAME} PRIVATE
  ${WATER_WATCHER_LINK_LIBRARIES})

if(APPLE)
  set_property(TARGET ${PROJECT_NAME} PROPERTY
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "org.water.watcher")
endif()

# [install]
include(GNUInstallDirs)
install(
  TARGETS ${PROJECT_NAME}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  BUNDLE DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
