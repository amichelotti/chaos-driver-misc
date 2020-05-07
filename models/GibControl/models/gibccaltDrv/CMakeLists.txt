cmake_minimum_required(VERSION 2.8)
project(chaos_driver_gibcontrol_gibccaltDrv)
FILE(GLOB src *.cpp)
ADD_LIBRARY(${PROJECT_NAME} ${src})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} common_gibcontrol_gibccaltDrv)
SET (DRIVERS_LIB ${DRIVERS_LIB} ${PROJECT_NAME} PARENT_SCOPE)
FILE(GLOB src *.h)
FILE(GLOB files "*.h")
INSTALL(FILES ${files} DESTINATION include/driver/misc/models/gibcontrol/models/gibccaltDrv/)
INSTALL(TARGETS ${PROJECT_NAME}
        DESTINATION "lib"
        PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
