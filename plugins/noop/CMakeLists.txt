set(include_dirs ${CMAKE_SOURCE_DIR}/src/include )
set(SOURCES vaccel.c ${include_dirs}/vaccel.h ${include_dirs}/plugin.h)
set_property(SOURCE ${include_dirs}/vaccel.h PROPERTY GENERATED 1)

add_library(vaccel-noop SHARED ${SOURCES})
target_include_directories(vaccel-noop PRIVATE ${include_dirs} ${CMAKE_BINARY_DIR}/version)

add_dependencies(vaccel-noop version)

# Setup make install
install(TARGETS vaccel-noop DESTINATION ${CMAKE_INSTALL_LIBDIR})
