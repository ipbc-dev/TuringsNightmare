cmake_minimum_required(VERSION 3.11)
project(TuringsNightmare)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

set(TuringsNightmare_source
  "${CMAKE_CURRENT_SOURCE_DIR}/src/jh.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/blake256.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/groestl.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/keccak.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/StringTools.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/TuringsNightmare.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/TuringsNightmareCL.cpp"
)

set(TuringsNightmare_include
  "${CMAKE_CURRENT_SOURCE_DIR}/include/jh.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/blake256.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/groestl.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/groestl_tables.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/keccak.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/StringTools.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/TuringsNightmare.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/TuringsNightmareCL.h"
)

find_package( OpenCL REQUIRED )

include_directories( ${OpenCL_INCLUDE_DIR} )

add_library(TuringsNightmare ${TuringsNightmare_source} ${TuringsNightmare_include} )

add_executable(test ${CMAKE_CURRENT_SOURCE_DIR}/src/test.cpp ${CMAKE_CURRENT_SOURCE_DIR}/include/utility.h)
target_link_libraries(test TuringsNightmare ${OpenCL_LIBRARY})