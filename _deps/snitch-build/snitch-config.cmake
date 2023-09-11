
####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was snitch-config.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../" ABSOLUTE)

####################################################################################

include("${CMAKE_CURRENT_LIST_DIR}/snitch-targets.cmake")

if (NOT TARGET snitch::snitch)
    add_library(snitch::snitch ALIAS snitch)
endif ()
