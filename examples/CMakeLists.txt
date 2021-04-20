set(include_dirs ${CMAKE_SOURCE_DIR}/src/include)

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

add_library(mytestlib SHARED mytestlib.c)
target_compile_options(mytestlib PUBLIC -Wall -Wextra )
set_target_properties(mytestlib PROPERTIES ENABLE_EXPORTS on)

# Install the examples
install(TARGETS classify classify_generic noop exec exec_generic DESTINATION "${bin_path}")
install(TARGETS mytestlib DESTINATION "${lib_path}")
