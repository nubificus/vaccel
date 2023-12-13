set(include_dirs ${CMAKE_SOURCE_DIR}/src/include)
set(SOURCES vaccel.c ${include_dirs}/vaccel.h ${include_dirs}/plugin.h)
set_property(SOURCE ${include_dirs}/vaccel.h PROPERTY GENERATED 1)

add_library(vaccel-mbench SHARED ${SOURCES})
target_include_directories(vaccel-mbench PRIVATE ${include_dirs})

# Setup make install
install(TARGETS vaccel-mbench DESTINATION ${CMAKE_INSTALL_LIBDIR})
