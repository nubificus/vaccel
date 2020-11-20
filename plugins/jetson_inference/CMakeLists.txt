# CUDA installation paths
set(CUDA_DIR "/usr/local/cuda-11.1/targets/x86_64-linux" CACHE STRING "Path to CUDA installation")
set(CUDA_INCLUDE ${CUDA_DIR}/include)

# Jetson inference installation paths
set(JETSON_DIR "/usr/local" CACHE STRING "Path to Jetson Inference installation")
set(JETSON_INCLUDE ${JETSON_DIR}/include)

# STB library path
set(STB_DIR "/usr/local" CACHE STRING "Path to STB installation")

link_directories(${CUDA_DIR}/targets/x86_64-linux/lib)
link_directories(${JETSON_DIR}/lib)

set(include_dirs ${CMAKE_SOURCE_DIR}/src)
message("Include directories: ${include_dirs}")
set(SOURCES
    operations.cpp
    vaccel.cpp
    util.cpp
    util.h
    ${include_dirs}/vaccel.h
    ${include_dirs}/backend.h)

# expose rpath for libraries when installing
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

add_library(vaccel-jetson SHARED ${SOURCES})
target_include_directories(vaccel-jetson PRIVATE ${include_dirs} ${JETSON_INCLUDE} ${CUDA_INCLUDE} ${STB_DIR})
target_link_libraries(vaccel-jetson PRIVATE jetson-inference jetson-utils cudart)

# Setup make install
install(TARGETS vaccel-jetson DESTINATION "${lib_path}")
