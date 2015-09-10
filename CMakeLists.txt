cmake_minimum_required(VERSION 2.6)
project(misc)
include($ENV{CHAOS_BUNDLE}/tools/project_template/CMakeChaos.txt)
set(LIB chaos_driver_misc)

SET(src src/ChaosDatasetAttribute.cpp  )


IF(BUILD_FORCE_STATIC)
  SET(CMAKE_EXE_LINKER_FLAGS "-static -Wl,--whole-archive -lchaos_common -Wl,--no-whole-archive")
   ADD_LIBRARY(${LIB} STATIC ${src} )
ELSE()
   ADD_LIBRARY(${LIB} SHARED ${src} )	 
ENDIF()

TARGET_LINK_LIBRARIES(${LIB} chaos_cutoolkit chaos_common common_serial ${FrameworkLib})

ADD_EXECUTABLE(testDataSetAttribute test/testDataSetAttribute.cpp)

TARGET_LINK_LIBRARIES(testDataSetAttribute ${LIB} chaos_cutoolkit chaos_common ${FrameworkLib})

INSTALL_TARGETS(/bin testDataSetAttribute)
 

 INSTALL_TARGETS(/lib ${LIB})
 FILE(GLOB ss src/*.h)
 INSTALL(FILES ${ss} DESTINATION include/driver/misc/)
   

