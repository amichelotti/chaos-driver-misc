cmake_minimum_required(VERSION 2.8)
project(chaos_driver_misc)
option(CERN_ROOT "Enable Cern Root" OFF)
find_package(chaos MODULE REQUIRED)
include_directories(../../ ${chaos_INCLUDE_DIRS})
include(${CMAKE_INSTALL_PREFIX}/CMakeMacroUtils.txt)

SET(src core/ChaosDatasetAttribute.cpp core/ChaosDatasetAttributeGroup.cpp core/ChaosDatasetAttributeSinchronizer.cpp core/ChaosController.cpp core/CmdSync.cpp core/RTDataSync.cpp core/SCDataSync.cpp core/remoteGroupAccessDriver.cpp core/remoteGroupAccessInterface.cpp core/ChaosDatasetIO.cpp ${optional_src})

add_subdirectory(models)
FILE(WRITE generators/UnitServerIgnore "disabled ${PROJECT_NAME}")
IF (BUILD_FORCE_STATIC)
    SET(CMAKE_EXE_LINKER_FLAGS "-static -Wl,--whole-archive -lchaos_common -Wl,--no-whole-archive")
    ADD_LIBRARY(${PROJECT_NAME} STATIC ${src})
ELSE ()
    ADD_LIBRARY(${PROJECT_NAME} SHARED ${src})
ENDIF()
INCLUDE_DIRECTORIES(./core)


TARGET_LINK_LIBRARIES(${PROJECT_NAME} chaos_cutoolkit chaos_metadata_service_client chaos_common common_misc_data common_misc_scheduler ${optional_libs} common_debug ${FrameworkLib})

ADD_EXECUTABLE(testSumOperatorUI test/testSumOperatorUI.cpp)
ADD_EXECUTABLE(testDataSetIO test/testDatasetIO.cpp)
ADD_EXECUTABLE(UnitServerLite test/UnitServerLite.cpp)


TARGET_LINK_LIBRARIES(testSumOperatorUI ${PROJECT_NAME} chaos_metadata_service_client chaos_common  common_misc_data common_misc_scheduler common_debug ${FrameworkLib})
TARGET_LINK_LIBRARIES(testDataSetIO ${PROJECT_NAME} chaos_metadata_service_client chaos_common  common_misc_data common_misc_scheduler common_debug ${FrameworkLib})
TARGET_LINK_LIBRARIES(UnitServerLite chaos_common  chaos_cutoolkit ${FrameworkLib})

INSTALL_TARGETS(/bin/misc testSumOperatorUI)
INSTALL_TARGETS(/bin/misc testDataSetIO)
INSTALL_TARGETS(/bin UnitServerLite)



INSTALL_TARGETS(/lib ${PROJECT_NAME})
installHeaders(core)

