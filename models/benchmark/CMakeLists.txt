cmake_minimum_required(VERSION 2.8)
project(chaos_driver_misc_benchmark)

SET(messlib ChaosMESS.cpp DefaultCommand.cpp DummyDriver.cpp CmdCalcTrxDelay.cpp CmdCalcBandwidth.cpp)
SET(MessMonitor_src test/messMonitor.cpp)
SET(MessClient_src test/messClient.cpp)
INCLUDE_DIRECTORIES(. ..)
ADD_DEFINITIONS(-O3)
ADD_EXECUTABLE(MessMonitor ${MessMonitor_src})
ADD_EXECUTABLE(MessClient ${MessClient_src})
MESSAGE(info "libraries:${FrameworkLib}")


IF(BUILD_FORCE_STATIC)
  SET(CMAKE_EXE_LINKER_FLAGS "-static -Wl,--whole-archive -lchaos_common -Wl,--no-whole-archive")
  ADD_LIBRARY(${PROJECT_NAME} STATIC ${messlib})
ELSE()
  ADD_LIBRARY(${PROJECT_NAME} SHARED ${messlib})
ENDIF()

TARGET_LINK_LIBRARIES(MessMonitor ${PROJECT_NAME} chaos_cutoolkit boost_random ${FrameworkLib})
TARGET_LINK_LIBRARIES(MessClient ${PROJECT_NAME} chaos_metadata_service_client chaos_common boost_random ${FrameworkLib})

TARGET_LINK_LIBRARIES(${PROJECT_NAME} pthread chaos_cutoolkit chaos_common boost_random ${FrameworkLib})
SET(DRIVERS_LIB ${DRIVERS_LIB} ${PROJECT_NAME} PARENT_SCOPE)
INSTALL_TARGETS(/bin MessMonitor)
INSTALL_TARGETS(/bin MessClient)
INSTALL_TARGETS(/lib ${PROJECT_NAME})
FILE(GLOB files "*.h")
INSTALL(FILES ${files} DESTINATION include/driver/misc/models/benchmark)



