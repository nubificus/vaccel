if (NOT DEFINED CUDA_DIR)
	set(CUDA_DIR /usr/local/cuda-11.1)
endif()
set(CUDA_INCLUDE ${CUDA_DIR}/targets/x86_64-linux/include)
message("Using CUDA_DIR ${CUDA_DIR}")

if (DEFINED JETSON_DIR)
	message("Using jetson ${JETSON_DIR}")
	set(JETSON_INCLUDE ${JETSON_DIR}/include)
endif()

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

add_library(vaccel-jetson SHARED ${SOURCES})
target_include_directories(vaccel-jetson PRIVATE ${include_dirs} ${JETSON_INCLUDE} ${CUDA_INCLUDE} ${STB_DIR})
target_link_libraries(vaccel-jetson PRIVATE jetson-inference jetson-utils cudart)

# Setup make install
install(TARGETS vaccel-jetson DESTINATION "${lib_path}")
