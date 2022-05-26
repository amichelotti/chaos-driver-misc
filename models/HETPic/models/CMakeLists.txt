cmake_minimum_required(VERSION 2.9)
add_subdirectory(HetPicSim)
add_subdirectory(HetPicTCPDriver)
SET(DRIVERS_LIB ${DRIVERS_LIB} PARENT_SCOPE)
