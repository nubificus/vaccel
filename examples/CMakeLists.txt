set(INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/src/include)
set(BIN_EXAMPLES
	classify
	classify_generic
	detect
	detect_generic
	segment
	segment_generic
	pose
	pose_generic
	depth
	depth_generic
	exec
	exec_generic
	noop
	sgemm
	tf_model
	tf_inference
	tf_saved_model)

foreach(T ${BIN_EXAMPLES})
	add_executable(${T} "${T}.c")
	target_include_directories(${T} PRIVATE ${INCLUDE_DIRS})
	target_link_libraries(${T} PRIVATE vaccel dl)
endforeach()

add_library(mytestlib SHARED mytestlib.c)
target_compile_options(mytestlib PUBLIC -Wall -Wextra )
set_target_properties(mytestlib PROPERTIES ENABLE_EXPORTS on)

install(TARGETS ${BIN_EXAMPLES} DESTINATION ${CMAKE_INSTALL_BINDIR})
install(TARGETS mytestlib DESTINATION ${CMAKE_INSTALL_LIBDIR})

# Install misc files
install(DIRECTORY ${CMAKE_SOURCE_DIR}/examples/models/tf
	DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/models)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/examples/images
	DESTINATION ${CMAKE_INSTALL_DATAROOTDIR})
