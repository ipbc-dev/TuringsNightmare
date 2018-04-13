cmake_minimum_required(VERSION 3.11)
project(TuringsNightmare)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

file(GLOB TN_COMMON_SRC
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/misc/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/crypto/*.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/cpu/*.cpp"
)

add_library(tn-common STATIC ${TN_COMMON_SRC})

# TODO: Move common stuff to TN common lib
# add_library(tn-backend-common STATIC ${BACKEND_COMMON_SRC})
# add_library(tn-backend-cpu STATIC ${BACKEND_CPU_SRC})
# target_link_libraries(tn-backend-cpu tn-backend-common)

# TODO: Make OpenCL & CUDA Optional
find_package(OpenCL 1.2)
if(OPENCL_FOUND)
  include_directories(${OpenCL_INCLUDE_DIR})
  file(GLOB TN_DEVICE_OPENCL_SRC "src/opencl/*.cpp")
  add_library(tn-device-opencl STATIC ${TN_DEVICE_OPENCL_SRC})
  target_link_libraries(tn-device-opencl tn-common ${OpenCL_LIBRARY})
endif()

find_package(Cuda 9.1)
if(CUDA_FOUND)
  include_directories(${CUDA_INCLUDE_DIRS})
  file(GLOB TN_DEVICE_CUDA_SRC "src/cuda/*.cu")
  cuda_add_library(tn-device-cuda STATIC ${TN_DEVICE_CUDA_SRC})
  target_link_libraries(tn-device-cuda tn-common ${CUDA_LIBRARIES})
endif()

add_executable(test ${CMAKE_CURRENT_SOURCE_DIR}/src/test.cpp)
target_link_libraries(test tn-common tn-device-opencl tn-device-cuda)