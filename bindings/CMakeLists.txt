include(ExternalProject)

set(RUST_VACCEL "${CMAKE_SOURCE_DIR}/bindings/rust-vaccel")
set_directory_properties(PROPERTIES EP_PREFIX ${CMAKE_BINARY_DIR}/Rust/rust-vaccel)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
	set(rust_build_type "--release")
endif()

message("Building rust-vaccel with ${rust_build_type} and PKG_CONFIG_PATH=${CMAKE_BINARY_DIR}/src")

ExternalProject_Add(
	rust-vaccel
	DOWNLOAD_COMMAND ""
	CONFIGURE_COMMAND ""
	BUILD_COMMAND bash -c "PKG_CONFIG_PATH=${CMAKE_BINARY_DIR}/src cargo build ${rust_build_type}"
	BINARY_DIR "${RUST_VACCEL}"
	INSTALL_COMMAND ""
	LOG_BUILD ON
)
