set(include_dirs ${CMAKE_SOURCE_DIR}/src)

add_executable(classify classification.c)
target_include_directories(classify PRIVATE ${include_dirs})
target_link_libraries(classify PRIVATE vaccel dl)
