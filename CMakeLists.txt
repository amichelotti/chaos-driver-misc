cmake_minimum_required(VERSION 2.6)
project(daq)
include($ENV{CHAOS_BUNDLE}/tools/project_template/CMakeChaos.txt)

IF("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux") 
  message(STATUS "Including Libera")
  SET(CHAOS_INSTALL_DIRS models/Libera models/Libera/cspi)
  ADD_SUBDIRECTORY(models/Libera)
  ADD_DEFINITIONS(-DLIBERA)
ENDIF()


# install(DIRECTORY DESTINATION include/driver/daq)  


