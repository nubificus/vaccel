set(include_dirs ${CMAKE_SOURCE_DIR}/src)

add_executable(classify classification.c)
add_executable(noop noop.c)
add_executable(genop genop.c)

target_include_directories(classify PRIVATE ${include_dirs})
target_link_libraries(classify PRIVATE vaccel dl)

target_include_directories(noop PRIVATE ${include_dirs})
target_link_libraries(noop PRIVATE vaccel dl)

target_include_directories(genop PRIVATE ${include_dirs})
target_link_libraries(genop PRIVATE vaccel dl)

# Install the examples
install(TARGETS classify DESTINATION "${bin_path}")
