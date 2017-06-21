cmake_minimum_required(VERSION 2.8)
project(chaos_driver_misc_cernroot)


IF (NOT CERN_ROOT_PREFIX)
  SET(CERN_ROOT_PREFIX ${CMAKE_INSTALL_PREFIX})
ENDIF()

INCLUDE_DIRECTORIES(${CERN_ROOT_PREFIX}/include)
LINK_DIRECTORIES(${CERN_ROOT_PREFIX}/lib)

ADD_LIBRARY(${PROJECT_NAME} SHARED rootUtil.cpp chaos.cxx)
ADD_EXECUTABLE(chaosRoot chaosRoot.cpp )

TARGET_LINK_LIBRARIES(${PROJECT_NAME} chaos_driver_misc chaos_metadata_service_client  chaos_common  common_misc_data common_misc_scheduler common_debug Core RIO Net Hist Graf Graf3d Gpad Tree Rint Postscript Matrix MathCore Thread MultiProc ${FrameworkLib} ) 
#TARGET_LINK_LIBRARIES(chaosRoot ${PROJECT_NAME} chaos_metadata_service_client chaos_common  common_misc_data common_misc_scheduler common_debug Core RIO Net Hist Graf Graf3d Gpad Tree Rint Postscript Matrix MathCore Thread MultiProc ${FrameworkLib} ) 
TARGET_LINK_LIBRARIES(chaosRoot ${PROJECT_NAME}) 

INSTALL_TARGETS(/bin chaosRoot) 
INSTALL_TARGETS(/lib ${PROJECT_NAME})
installHeaders(.)

