file(GLOB_RECURSE sources *.c)

# core runtime headers
file(GLOB_RECURSE headers *.h)

add_library(vaccel-python SHARED ${headers} ${sources})
target_compile_options(vaccel-python PUBLIC -Wall -Wextra -Werror -pthread)
#target_include_directories(vaccel-python PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_include_directories(vaccel-python PRIVATE ${CMAKE_SOURCE_DIR}/src/include)

target_include_directories(vaccel-python PRIVATE ${CMAKE_BINARY_DIR}/version)


set_property(TARGET vaccel-python PROPERTY LINK_FLAGS "-pthread")
set_property(TARGET vaccel-python PROPERTY C_STANDARD 11)
target_link_libraries(vaccel-python)

add_dependencies(vaccel-python version)

# Setup make install
install(TARGETS vaccel-python DESTINATION ${CMAKE_INSTALL_LIBDIR})
#install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Create the pkg-config file
set(DEST_DIR "${CMAKE_INSTALL_PREFIX}")
CONFIGURE_FILE("vaccel-python.pc.in" "vaccel-python.pc" @ONLY)

# Install the vaccel.pc file
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/vaccel-python.pc DESTINATION ${CMAKE_INSTALL_DATAROOTDIR})
