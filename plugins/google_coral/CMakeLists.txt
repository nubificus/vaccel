set(CORAL_LIB /root/aarch64/examples)
link_directories(${CORAL_LIB})

set(include_dirs ${CMAKE_SOURCE_DIR}/src)
message("Include directories: ${include_dirs}")
set(SOURCES
    operations.cpp
    vaccel.cpp
    ${include_dirs}/vaccel.h
    ${include_dirs}/plugin.h)

# expose rpath for libraries when installing
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

add_library(vaccel-coral SHARED ${SOURCES})
target_include_directories(vaccel-coral PRIVATE ${include_dirs})
target_link_libraries(vaccel-coral PRIVATE classify)

# Setup make install
install(TARGETS vaccel-coral DESTINATION "${lib_path}")
