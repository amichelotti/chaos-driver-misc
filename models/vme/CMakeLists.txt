cmake_minimum_required(VERSION 2.8)
project(chaos_driver_misc_vme)


IF (NOT CERN_ROOT_PREFIX)
  SET(CERN_ROOT_PREFIX ${CERN_ROOT})
ENDIF()

INCLUDE_DIRECTORIES(${CERN_ROOT_PREFIX}/include)
LINK_DIRECTORIES(${CERN_ROOT_PREFIX}/lib)

ADD_LIBRARY(${PROJECT_NAME} SHARED RTVme.cpp)

TARGET_LINK_LIBRARIES(${PROJECT_NAME} chaos_common  chaos_cutoolkit common_vme vme)
INSTALL_TARGETS(/lib ${PROJECT_NAME})
installHeaders(.)

