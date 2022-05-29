cmake_minimum_required(VERSION 2.9)
add_subdirectory(benchmark)

SET(DRIVERS_LIB ${DRIVERS_LIB} PARENT_SCOPE)
IF (CERN_ROOT)
add_subdirectory(cernRoot)
ENDIF()
add_subdirectory(DafnePresenter)
add_subdirectory(GibControl)
add_subdirectory(HETPic)
add_subdirectory(HETController)

