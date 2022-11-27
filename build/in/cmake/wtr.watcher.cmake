# [target, esp. cli program]
add_executable("${PROJECT_NAME}"
  "${SOURCES}")

# [c++ standard]
set_property(TARGET "${PROJECT_NAME}" PROPERTY
  CXX_STANDARD 20)

# [compile options, esp. sanitizers]
target_compile_options("${PROJECT_NAME}" PRIVATE
  "-O3" "${COMPILE_OPTIONS}")

# [link options, esp. sanitizers]
target_link_options("${PROJECT_NAME}" PRIVATE
  "${LINK_OPTIONS}")

# [include path]
target_include_directories("${PROJECT_NAME}" PUBLIC
  "${INCLUDE_PATH}")

# [system libraries]
target_link_libraries("${PROJECT_NAME}" PRIVATE
  "${LINK_LIBRARIES}")

# [platform specifics]
if(APPLE)
  set_property(TARGET "${PROJECT_NAME}" PROPERTY
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "org.wtr.watcher")
endif()

# [install]
install(
  TARGETS                   "${PROJECT_NAME}"
  LIBRARY DESTINATION       "${CMAKE_INSTALL_LIBDIR}"
  BUNDLE DESTINATION        "${CMAKE_INSTALL_PREFIX}/bin"
  PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
