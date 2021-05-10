set(include_dirs ${CMAKE_SOURCE_DIR}/src/include)
set(SOURCES vaccel.c ${include_dirs}/vaccel.h ${include_dirs}/plugin.h)

add_library(vaccel-noop SHARED ${SOURCES})
target_include_directories(vaccel-noop PRIVATE ${include_dirs})

# Setup make install
install(TARGETS vaccel-noop DESTINATION ${CMAKE_INSTALL_LIBDIR})
