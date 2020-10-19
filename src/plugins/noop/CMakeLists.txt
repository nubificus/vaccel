set(include_dirs ${CMAKE_SOURCE_DIR}/src)
set(SOURCES vaccel.c ${include_dirs}/vaccel.h ${include_dirs}/backend.h)

add_library(vaccel-noop SHARED ${SOURCES})
target_include_directories(vaccel-noop PRIVATE ${include_dirs})
