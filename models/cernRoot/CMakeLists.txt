cmake_minimum_required(VERSION 3.1)
project(chaos_driver_misc_cernroot)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
IF (NOT CERN_ROOT_PREFIX)
  SET(CERN_ROOT_PREFIX ${CERN_ROOT})
ENDIF()

INCLUDE_DIRECTORIES(${CERN_ROOT_PREFIX}/include)
LINK_DIRECTORIES(${CERN_ROOT_PREFIX}/lib)

ADD_EXECUTABLE(chaosRoot chaosRootapp.cpp)
ADD_EXECUTABLE(chaos2tree chaos2tree.cpp)
ADD_EXECUTABLE(histo2tree histo2tree.cpp)
SET (src rootUtil.cpp ChaosRoot.cpp)
SET(ADDITIONAL_LIBS)
IF(OPENCV)
  ADD_DEFINITIONS(-DOPENCV)
  SET(ADDITIONAL_LIBS opencv_core  opencv_highgui opencv_imgproc opencv_imgcodecs)
  SET (src ${src} rootGaussianImage2dFit.cpp)

ENDIF()
ADD_LIBRARY(${PROJECT_NAME} SHARED ${src})
TARGET_LINK_LIBRARIES(${PROJECT_NAME}  chaos_driver_misc chaos_metadata_service_client chaos_common  common_misc_data common_misc_scheduler common_debug Core RIO Net Hist Graf Graf3d Gpad Tree Rint Imt Postscript Matrix MathCore Thread MultiProc ${FrameworkLib} ${ADDITIONAL_LIBS} ) 
#TARGET_LINK_LIBRARIES(chaosRoot ${PROJECT_NAME} chaos_metadata_service_client chaos_common  common_misc_data common_misc_scheduler common_debug Core RIO Net Hist Graf Graf3d Gpad Tree Rint Postscript Matrix MathCore Thread MultiProc ${FrameworkLib} ) 
TARGET_LINK_LIBRARIES(chaosRoot ${PROJECT_NAME}) 
TARGET_LINK_LIBRARIES(chaos2tree ${PROJECT_NAME}) 
TARGET_LINK_LIBRARIES(histo2tree ${PROJECT_NAME}) 
#INSTALL_TARGETS(/bin histo2tree)
INSTALL(TARGETS histo2tree
        DESTINATION "bin"
        PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

#INSTALL_TARGETS(/bin chaosRoot)
INSTALL(TARGETS chaosRoot
        DESTINATION "bin"
        PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
#INSTALL_TARGETS(/bin chaos2tree)
INSTALL(TARGETS chaos2tree
        DESTINATION "bin"
        PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

#INSTALL_TARGETS(/lib ${PROJECT_NAME})
INSTALL(TARGETS ${PROJECT_NAME}
        DESTINATION "lib"
        PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
installHeaders(.)

