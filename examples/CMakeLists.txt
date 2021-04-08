set(include_dirs ${CMAKE_SOURCE_DIR}/src)

add_executable(classify classification.c)

target_include_directories(classify PRIVATE ${include_dirs})
target_link_libraries(classify PRIVATE vaccel dl)

add_executable(classify_generic classification_generic.c)
target_include_directories(classify_generic PRIVATE ${include_dirs})
target_link_libraries(classify_generic PRIVATE vaccel dl)

add_executable(exec exec.c)
target_include_directories(exec PRIVATE ${include_dirs})
target_link_libraries(exec PRIVATE vaccel dl)

add_executable(exec_generic exec_generic.c)
target_include_directories(exec_generic PRIVATE ${include_dirs})
target_link_libraries(exec_generic PRIVATE vaccel dl)

add_executable(noop noop.c)
target_include_directories(noop PRIVATE ${include_dirs})
target_link_libraries(noop PRIVATE vaccel dl)

# Install the examples
install(TARGETS classify classify_generic DESTINATION "${bin_path}")
