set(INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/src/include ${CMAKE_BINARY_DIR}/version)
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
	exec_with_res
	exec_generic
	noop
	sgemm
	sgemm_generic
	minmax
	minmax_generic
	tf_model
	tf_inference
	tf_saved_model
	pynq_vector_add
	pynq_vector_add_generic
	pynq_parallel
	pynq_parallel_generic
	pynq_array_copy
	pynq_array_copy_generic)

foreach(T ${BIN_EXAMPLES})
	add_executable(${T} "${T}.c")
	IF(MUSL_BUILD)
	set_property(TARGET ${T} PROPERTY LINK_FLAGS "-Os")
	ENDIF()
	target_include_directories(${T} PRIVATE ${INCLUDE_DIRS})
	IF(MUSL_BUILD)
	set(CMAKE_FIND_LIBRARY_SUFFIXES .so)
	ENDIF()
	target_link_libraries(${T} PRIVATE vaccel dl)
endforeach()

IF(NOT MUSL_BUILD)
add_executable(torch_inference "torch_inference.cpp")
target_compile_options(torch_inference PUBLIC -fpermissive)
target_include_directories(torch_inference PRIVATE ${INCLUDE_DIRS})
target_link_libraries(torch_inference PRIVATE vaccel dl)
ENDIF()

add_library(mytestlib SHARED mytestlib.c)
target_compile_options(mytestlib PUBLIC -Wall -Wextra )
set_target_properties(mytestlib PROPERTIES ENABLE_EXPORTS on)

IF(NOT MUSL_BUILD)
install(TARGETS ${BIN_EXAMPLES} torch_inference DESTINATION ${CMAKE_INSTALL_BINDIR})
ENDIF()
install(TARGETS mytestlib DESTINATION ${CMAKE_INSTALL_LIBDIR})

# Install misc files
install(DIRECTORY ${CMAKE_SOURCE_DIR}/examples/models/tf
	DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/models)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/examples/models/torch
	DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/models)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/examples/images
	DESTINATION ${CMAKE_INSTALL_DATAROOTDIR})
install(DIRECTORY ${CMAKE_SOURCE_DIR}/examples/input
	DESTINATION ${CMAKE_INSTALL_DATAROOTDIR})
