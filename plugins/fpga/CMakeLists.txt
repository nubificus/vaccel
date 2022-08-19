set(include_dirs ${CMAKE_SOURCE_DIR}/src)
set(SOURCES vaccel.c ${include_dirs}/vaccel.h ${include_dirs}/plugin.h)

add_library(vaccel-fpga SHARED ${SOURCES})
target_include_directories(vaccel-fpga PRIVATE ${include_dirs})

# Setup make install
install(TARGETS vaccel-fpga DESTINATION "${lib_path}")