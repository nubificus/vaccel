if (BUILD_PLUGIN_NOOP)
	add_subdirectory(noop)
endif(BUILD_PLUGIN_NOOP)

if (BUILD_PLUGIN_JETSON)
	add_subdirectory(jetson_inference)
endif(BUILD_PLUGIN_JETSON)

if (BUILD_PLUGIN_CORAL)
	add_subdirectory(google_coral)
endif(BUILD_PLUGIN_CORAL)

if (BUILD_PLUGIN_VIRTIO)
	add_subdirectory(virtio)
endif(BUILD_PLUGIN_VIRTIO)

if (BUILD_PLUGIN_VSOCK)
	include(ExternalProject)

	set(VSOCK_DIR "${CMAKE_SOURCE_DIR}/plugins/vsock")

	set_directory_properties(PROPERTIES EP_PREFIX ${CMAKE_BINARY_DIR}/Rust)

	ExternalProject_Add(
		vsock_plugin
		DOWNLOAD_COMMAND ""
		CONFIGURE_COMMAND ""
		BUILD_COMMAND cargo build COMMAND cargo build --release
		BINARY_DIR "${VSOCK_DIR}"
		INSTALL_COMMAND ""
		LOG_BUILD ON)

	add_library(vaccel-vsock SHARED ${VSOCK_DIR}/src/glue.c)
	add_dependencies(vaccel-vsock vsock_plugin)
	target_include_directories(vaccel-vsock PRIVATE ${CMAKE_SOURCE_DIR}/src)
	target_link_libraries(
		vaccel-vsock
		debug "${VSOCK_DIR}/target/debug/libvaccel_vsock.a"
		optimized "${VSOCK_DIR}/target/release/libvaccel_vsock.a"
		rt)
	install(TARGETS vaccel-vsock DESTINATION "${lib_path}")
endif(BUILD_PLUGIN_VSOCK)
