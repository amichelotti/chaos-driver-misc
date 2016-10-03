cmake_minimum_required(VERSION 2.8)
project(chaos_driver_misc)
SET(src core/ChaosDatasetAttribute.cpp core/ChaosDatasetAttributeGroup.cpp core/ChaosDatasetAttributeSinchronizer.cpp core/ChaosController.cpp core/CmdSync.cpp core/RTDataSync.cpp core/SCDataSync.cpp core/remoteGroupAccessDriver.cpp core/remoteGroupAccessInterface.cpp core/RTVme.cpp)

add_subdirectory(models)
IF (BUILD_FORCE_STATIC)
    SET(CMAKE_EXE_LINKER_FLAGS "-static -Wl,--whole-archive -lchaos_common -Wl,--no-whole-archive")
    ADD_LIBRARY(${PROJECT_NAME} STATIC ${src})
ELSE ()
    ADD_LIBRARY(${PROJECT_NAME} SHARED ${src})
ENDIF ()
INCLUDE_DIRECTORIES(./core)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} chaos_cutoolkit chaos_common chaos_uitoolkit common_misc_data uv common_vme vme common_debug ${FrameworkLib})

ADD_EXECUTABLE(testSumOperatorUI test/testSumOperatorUI.cpp)

TARGET_LINK_LIBRARIES(testSumOperatorUI ${PROJECT_NAME} chaos_cutoolkit chaos_common common_misc_data_cassandra common_misc_data common_debug ${FrameworkLib})

INSTALL_TARGETS(/bin testSumOperatorUI)


INSTALL_TARGETS(/lib ${PROJECT_NAME})
installHeaders(core)

