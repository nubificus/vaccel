set(include_dirs ${CMAKE_SOURCE_DIR}/src)
set(SOURCES vaccel.c ${include_dirs}/vaccel.h ${include_dirs}/plugin.h)

add_library(vaccel-exec SHARED ${SOURCES})
target_include_directories(vaccel-exec PRIVATE ${include_dirs})

# Setup make install
install(TARGETS vaccel-exec DESTINATION "${lib_path}")
