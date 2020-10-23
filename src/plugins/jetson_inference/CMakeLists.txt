set(CUDA_DIR /usr/local/cuda-11.1)
link_directories(${CUDA_DIR}/targets/x86_64-linux/lib)
set(include_dirs ${CMAKE_SOURCE_DIR}/src)
set(SOURCES
    operations.cpp
    vaccel.cpp
    util.cpp
    util.h
    ${include_dirs}/vaccel.h
    ${include_dirs}/backend.h)

add_library(vaccel-jetson SHARED ${SOURCES})
target_include_directories(vaccel-jetson PRIVATE ${include_dirs}
	PRIVATE /usr/local/cuda/include)
target_link_libraries(vaccel-jetson PRIVATE jetson-inference jetson-utils cudart)
