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

add_executable(tf_model tf_model.c)
target_include_directories(tf_model PRIVATE ${include_dirs})
target_link_libraries(tf_model PRIVATE vaccel dl)

add_library(mytestlib SHARED mytestlib.c)
target_compile_options(mytestlib PUBLIC -Wall -Wextra )
set_target_properties(mytestlib PROPERTIES ENABLE_EXPORTS on)

add_executable(tf_inference tf_inference.c)
target_include_directories(tf_inference PRIVATE ${include_dirs})
target_link_libraries(tf_inference PRIVATE vaccel dl)

add_executable(tf_saved_model tf_saved_model.c)
target_include_directories(tf_saved_model PRIVATE ${include_dirs})
target_link_libraries(tf_saved_model PRIVATE vaccel dl)

# Install the examples
install(TARGETS
	classify
	classify_generic
	noop exec
	exec_generic
	tf_model
	tf_inference
	tf_saved_model
	DESTINATION ${CMAKE_INSTALL_BINDIR})
install(TARGETS mytestlib DESTINATION ${CMAKE_INSTALL_LIBDIR})

# Install misc files
install(DIRECTORY ${CMAKE_SOURCE_DIR}/examples/models/tf
	DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/models)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/examples/images
	DESTINATION ${CMAKE_INSTALL_DATAROOTDIR})
